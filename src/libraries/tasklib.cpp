#include "libraries/tasklib.hpp"
#include "Luau/Common.h"
#include "common.hpp"
#include "lua.h"
#include "lualib.h"
#include "luacode.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <shared_mutex>
#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace fakeroblox {

const char* taskStatusTostring(TaskStatus status) {
    switch (status) {
        case RUNNING:
            return "running";
        case YIELDING:
            return "yielding";
        case WAITING:
            return "waiting";
        case DEFERRING:
            return "deferring";
        case DELAYING:
            return "delaying";
        default:
            return "unknown";
    }
};

std::shared_mutex TaskScheduler::mutex;
std::vector<Task*> TaskScheduler::task_queue;
std::unordered_map<lua_State*, Task*> TaskScheduler::task_map;

std::vector<lua_State*> killed_tasks;

Task::Task(lua_State* thread, lua_State* parent_thread, int thread_ref, Feedback feedback, TaskTiming timing, OnKill on_kill)
    : thread(thread), parent_thread(parent_thread), thread_ref(thread_ref), feedback(feedback), timing(timing), on_kill(on_kill)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%p", static_cast<void*>(thread));
    identifier = std::string(buffer);

    TaskScheduler::task_map.emplace(thread, this);
}

Task::~Task() {
    if (on_kill)
        on_kill();
}

void TaskScheduler::queueTask(Task* task) {
    std::shared_lock lock(mutex);
    task_queue.push_back(task);
}

//                yielding? / error
typedef std::variant<bool, std::string> ThreadResumeResult;

ThreadResumeResult resumeThread(lua_State* L, Task* task);

void TaskScheduler::resumeTask(lua_State *L, Task* task) {
    std::shared_lock lock(mutex);
    task_queue.erase(std::find(task_queue.begin(), task_queue.end(), task));

    if (task->canceled)
        return;

    ThreadResumeResult result = resumeThread(L, task);
    if (std::holds_alternative<bool>(result)) { // successful
        if (!std::get<bool>(result)) // yielding
            killTask(L, task);
            // ;
    } else
      task->feedback(std::string("failed to resume thread: ").append(std::get<std::string>(result)));
}

void TaskScheduler::killTask(lua_State* L, Task* task) {
    std::shared_lock lock(mutex);

    // FIXME: what if we already iterated to a task in the queue and then it gets removed?; should be fine with the mutex we have now
    auto task_queue_position = std::find(task_queue.begin(), task_queue.end(), task);
    if (task_queue_position != task_queue.end())
        task_queue.erase(task_queue_position);

    lua_State* thread = task->thread;
    int thread_ref = task->thread_ref;

    killed_tasks.push_back(thread);
    task_map.erase(task_map.find(thread));
    lua_unref(L, thread_ref);
    // lua_gc(L, LUA_GCSTEP, 0);
    delete task;
}

Task* TaskScheduler::getTaskFromThread(lua_State* thread) {
    std::shared_lock lock(mutex);

    auto position = task_map.find(thread);
    if (position == task_map.end())
        return nullptr;

    return position->second;
}
bool TaskScheduler::wasThreadKilled(lua_State* thread) {
    std::shared_lock lock(mutex);

    return std::find(killed_tasks.begin(), killed_tasks.end(), thread) != killed_tasks.end();
}

void TaskScheduler::run(lua_State* L) {
    // TODO: maybe clone task_queue instead?
    std::shared_lock lock(mutex);

    std::vector<Task*> filtered_tasks;
    filtered_tasks.reserve(task_queue.size());
    for (size_t i = 0; i < task_queue.size(); i++) {
        Task* task = task_queue[i];
        assert(task->status != RUNNING);

        bool passes = false;
        TaskTiming timing = task->timing;

        switch (timing.type) {
            case TaskTiming::Instant:
                passes = true;
                break;
            case TaskTiming::Seconds:
                double clock = lua_clock();
                passes = clock >= timing.end_time;
                break;
        }

        if (passes)
            filtered_tasks.push_back(task);
    }

    for (size_t i = 0; i < filtered_tasks.size(); i++)
        resumeTask(L, filtered_tasks[i]);
}

void TaskScheduler::cleanup(lua_State* L) {
    std::shared_lock lock(mutex);

    for (auto& pair : task_map)
        killTask(L, pair.second);
    task_map.clear();
}

ThreadResumeResult resumeThread(lua_State* L, Task* task) {
    task->status = RUNNING;
    lua_State* thread = task->thread;

    int status = lua_resume(thread, L, task->arg_count);

    switch (status) {
        case LUA_OK:
            return false;
        case LUA_YIELD:
            return true;
        case LUA_ERRRUN: {
            const char* str = lua_tostring(thread, -1);
            std::string msg = str == nullptr ? "unknown" : str;
            lua_pop(thread, 1);
            return msg;
        }
        default: {
            std::string msg = "unexpected status: ";
            switch (status) {
                case LUA_ERRSYNTAX:
                    msg.append("ERRSYNTAX");
                    break;
                case LUA_ERRMEM:
                    msg.append("ERRMEM");
                    break;
                case LUA_ERRERR:
                    msg.append("ERRERR");
                    break;
                default:
                    break;
            }
            return msg;
        }
    }
}

std::pair<lua_State*, Task*> createThread(lua_State* L, Feedback feedback, OnKill on_kill = nullptr) {
    lua_State* thread = lua_newthread(L);
    int thread_ref = lua_ref(L, -1);
    luaL_sandboxthread(thread);

    Task* task = new Task(thread, L, thread_ref, feedback, {}, on_kill);

    return std::make_pair(thread, task);
}
std::optional<std::string> tryRunThreadMain(lua_State* L, Task* task) {
    if (task->canceled)
        return std::nullopt;

    ThreadResumeResult result = resumeThread(L, task);
    if (std::holds_alternative<bool>(result)) { // success
        if (!std::get<bool>(result)) // yielding
            TaskScheduler::killTask(L, task);
        return std::nullopt;
    }

    TaskScheduler::killTask(L, task);
    return std::string("failed to start thread: ").append(std::get<std::string>(result));
}

std::optional<std::string> tryCreateThreadAndSpawnFunction(lua_State* L, Feedback feedback, Console* console) {
    luaL_checktype(L, lua_gettop(L), LUA_TFUNCTION);
    auto thread_pair = createThread(L, feedback);
    lua_pop(L, 1);

    lua_State* thread = thread_pair.first;
    Task* task = thread_pair.second;

    if (console)
        task->console = console;

    task->arg_count = 0;
    lua_xmove(L, thread, 1);

    return tryRunThreadMain(L, task);
}

std::optional<std::string> tryRunCode(lua_State* L, const char* chunk_name, const char* source, Feedback feedback, OnKill on_kill, Console* console) {
    size_t bytecode_size = 0;
    char* bytecode = luau_compile(source, std::strlen(source), NULL, &bytecode_size);
    if (!bytecode)
        return "failed to allocate memory for compiled bytecode";

    auto thread_pair = createThread(L, feedback, on_kill);
    lua_pop(L, 1);
    lua_State* thread = thread_pair.first;
    Task* task = thread_pair.second;
    if (console)
        task->console = console;

    int r = luau_load(thread, chunk_name, bytecode, bytecode_size, 0);
    free(bytecode);
    if (r) {
        std::string msg = std::string("failed to load chunk: ")
            .append(lua_tostring(thread, -1));
        lua_pop(thread, 1);

        TaskScheduler::killTask(L, task);
        return msg;
    }

    task->arg_count = 0;
    return tryRunThreadMain(L, task);
}

class PreSpawnResult {
public:
    lua_State* thread = nullptr;
    int arg_count = 0;
    Task* task = nullptr;
};

std::optional<PreSpawnResult> preTaskSpawn(lua_State* L, const char* func_name, int arg_offset) {
    Task* parent_task = TaskScheduler::getTaskFromThread(L);

    int arg1 = 1 + arg_offset;
    luaL_checkany(L, arg1);
    int arg1_type = lua_type(L, arg1);

    constexpr int msg_size = 42;
    char msg[msg_size];
    snprintf(msg, msg_size, "expected function or thread, got %s", lua_typename(L, arg1_type));
    luaL_argcheck(L, arg1_type == LUA_TFUNCTION || arg1_type == LUA_TTHREAD, arg1, msg);

    lua_State* thread;
    Task* task;

    int arg_count = lua_gettop(L) - arg_offset;

    switch (arg1_type) {
        case LUA_TTHREAD: {
            thread = lua_tothread(L, arg1);
            task =  TaskScheduler::getTaskFromThread(thread);
            if (task == nullptr) {
                // printf("thread: %p\n", thread);
                // luaL_error(L, "failed to get task from thread");
                return std::nullopt;
            }

            TaskStatus status = task->status;
            switch (status) {
                case RUNNING:
                case YIELDING:
                    break;
                case WAITING:
                case DEFERRING:
                case DELAYING:
                    luaL_error(L, "attempt to call %s on a thread that is %s", func_name, taskStatusTostring(status));
                    return std::nullopt;
            }
            break;
        }
        case LUA_TFUNCTION: {
            auto thread_pair = createThread(L, parent_task->feedback);
            thread = thread_pair.first;
            task = thread_pair.second;
            break;
        }
        default:
            LUAU_UNREACHABLE();
    }

    for (int i = 0; i < arg_count; i++) {
        lua_xpush(L, thread, arg1);
        lua_remove(L, arg1);
    }

    return PreSpawnResult{ .thread = thread, .arg_count = arg_count, .task = task  };
}

double getSeconds(lua_State* L, int arg = 1) {
    double seconds = luaL_optnumber(L, arg, 0.0);
    luaL_argcheck(L, seconds >= 0.0, arg, "seconds must be positive");
    return seconds;
}

int fakeroblox_task_wait(lua_State* L) {
    Task* task = TaskScheduler::getTaskFromThread(L);

    double seconds = getSeconds(L);

    task->status = WAITING;
    task->timing = TaskTiming { .type = TaskTiming::Seconds, .end_time = lua_clock() + seconds };
    task->arg_count = 0;
    TaskScheduler::queueTask(task);

    return lua_yield(L, 0);
}

int fakeroblox_task_spawn(lua_State* L) {
    auto result = preTaskSpawn(L, "spawn", 0);
    if (!result.has_value())
        return 0;

    int arg_count = result->arg_count;
    Task* task = result->task;
    task->arg_count = arg_count - 1;

    auto error = tryRunThreadMain(L, task);
    if (error.has_value())
        task->feedback(*error);

    return 1;
}

int fakeroblox_task_defer(lua_State* L) {
    auto result = preTaskSpawn(L, "defer", 0);
    if (!result.has_value())
        return 0;

    int arg_count = result->arg_count;
    Task* task = result->task;

    task->arg_count = arg_count - 1;
    task->status = DEFERRING;
    task->timing = TaskTiming { .type = TaskTiming::Instant };
    TaskScheduler::queueTask(task);

    return 1;
}

int fakeroblox_task_delay(lua_State* L) {
    auto result = preTaskSpawn(L, "delay", 1);
    if (!result.has_value())
        return 0;

    double seconds = getSeconds(L);
    lua_remove(L, 1);

    int arg_count = result->arg_count;
    Task* task = result->task;

    task->arg_count = arg_count - 1;
    task->status = DELAYING;
    task->timing = TaskTiming { .type = TaskTiming::Seconds, .end_time = lua_clock() + seconds };
    TaskScheduler::queueTask(task);

    return 1;
}

int fakeroblox_task_cancel(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTHREAD);

    lua_State* thread =lua_tothread(L, 1);
    Task* task = TaskScheduler::getTaskFromThread(thread);
    if (task == nullptr)
        luaL_error(L, "failed to get task from thread");

    task->canceled = true;
    return 0;
}

int fakeroblox_task_status(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTHREAD);

    lua_State* thread = lua_tothread(L, 1);
    if (TaskScheduler::wasThreadKilled(thread)) {
        lua_pushstring(L, "killed");
        return 1;
    }

    Task* task = TaskScheduler::getTaskFromThread(thread);
    if (task == nullptr)
        luaL_error(L, "failed to get task from thread");

    lua_pushstring(L, taskStatusTostring(task->status));
    return 1;
}

void open_tasklib(lua_State *L) {
    lua_newtable(L);

    setfunctionfield(L, fakeroblox_task_spawn, "spawn", true);
    setfunctionfield(L, fakeroblox_task_defer, "defer", true);
    setfunctionfield(L, fakeroblox_task_delay, "delay", true);
    setfunctionfield(L, fakeroblox_task_cancel, "cancel", true);
    setfunctionfield(L, fakeroblox_task_status, "status", true);
    setfunctionfield(L, fakeroblox_task_wait, "wait", true);

    lua_setglobal(L, "task");
}

}; // namespace fakeroblox
