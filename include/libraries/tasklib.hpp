#pragma once

#include <cassert>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "console.hpp"
#include "lua.h"

#include "common.hpp"

namespace fakeroblox {

class Task;

enum TaskStatus {
    RUNNING,
    YIELDING,
    WAITING,
    DEFERRING,
    DELAYING
};
const char* taskStatusTostring(TaskStatus status);

class TaskScheduler {
public:
    static std::vector<Task*> task_queue;
    static std::shared_mutex task_queue_mutex;
    static std::unordered_map<lua_State*, Task*> task_map;
    static std::shared_mutex task_map_mutex;

    static void queueTask(Task* task);
    static void resumeTask(Task* task);
    static void killTask(Task* task);

    static Task* getTaskFromThread(lua_State* thread);
    static bool wasThreadKilled(lua_State* thread);

    static void run();

    static void cleanup();
};

class TaskTiming {
public:
    enum {
        Instant,
        Wait,
        Delay
    } type = Instant;

    double start_time = 0.0;
    double count = 0.0;
};

class Task {
public:
    lua_State* thread;
    lua_State* parent_thread;
    Console* console = &Console::ScriptConsole;
    std::string identifier;
    int thread_ref;
    Feedback feedback;
    TaskTiming timing;
    OnKill on_kill;

    struct {
        bool open = false;
    } view;

    TaskStatus status = RUNNING;
    int arg_count = 0;    
    bool canceled = false;
    bool finished = false;

    Task(lua_State* thread, lua_State* parent_thread, int thread_ref, Feedback feedback, TaskTiming timing, OnKill on_kill = nullptr);
    ~Task();
};

void open_tasklib(lua_State* L);

int fakeroblox_task_spawn(lua_State* L);
int fakeroblox_task_defer(lua_State* L);
int fakeroblox_task_delay(lua_State* L);
int fakeroblox_task_cancel(lua_State* L);
int fakeroblox_task_status(lua_State* L);
int fakeroblox_task_wait(lua_State* L);

std::pair<lua_State*, Task*> createThread(lua_State* L, Feedback feedback, OnKill on_kill = nullptr);

std::optional<std::string> tryCreateThreadAndSpawnFunction(lua_State* L, Feedback feedback, Console* console = nullptr);
std::optional<std::string> tryRunCode(lua_State* L, const char* chunk_name, const char* source, Feedback feedback, OnKill on_kill = nullptr, Console* console = nullptr);

}; // namespace fakeroblox
