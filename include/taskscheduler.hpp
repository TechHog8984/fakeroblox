#pragma once

#include <shared_mutex>
#include <vector>

#include "common.hpp"
#include "console.hpp"

#include "lua.h"

namespace fakeroblox {

enum TaskStatus {
    IDLE,

    RUNNING,
    YIELDING,
    WAITING,
    DEFERRING,
    DELAYING
};

struct TaskTiming {
    enum {
        Instant,
        Wait,
        Delay
    } type = Instant;

    double start_time = 0.0;
    double count = 0.0;
};

enum ThreadCapability {
    LOWEST_CAPABILITY,

    NONE = LOWEST_CAPABILITY,
    PLUGIN_SECURITY,
    INVALID_CAPABILITY = 2,
    LOCAL_USER_SECURITY = 3,
    WRITE_PLAYER_SECURITY,
    ROBLOX_SCRIPT_SECURITY,
    ROBLOX_SECURITY,
    NOT_ACCESSIBLE_SECURITY,

    HIGHEST_CAPABILTY = NOT_ACCESSIBLE_SECURITY
};

struct Task {
    TaskStatus status;
    lua_State* parent;

    Console* console = &Console::ScriptConsole;
    std::string identifier;

    bool canceled;
    int ref;
    int arg_count;
    TaskTiming timing;

    // TODO: maybe just always use console and remove feedback
    Feedback feedback;
    OnKill on_kill;

    ThreadCapability capability = NONE;

    struct {
        bool open = false;
    } view;
};

using Workload = std::function<int(lua_State*)>;

class TaskScheduler {
    static std::shared_mutex target_fps_mutex;

    static std::shared_mutex gc_mutex;

    static std::vector<lua_State*> thread_queue;
    static std::shared_mutex thread_queue_mutex;

    static void resumeThread(lua_State* thread);
    static void killThreadUnlocked(lua_State* thread);
public:
    static bool sandboxing;
    static double initial_client_time;

    static void setup(lua_State* L);

    static std::vector<lua_State*> thread_list;
    static std::shared_mutex thread_list_mutex;

    static int target_fps;
    static void setTargetFps(int target);

    static bool pauseGarbageCollection(lua_State* L);
    static void resumeGarbageCollection(lua_State* L);

    static lua_State* newThread(lua_State* L, Feedback feedback, OnKill on_kill = nullptr);
    static void killThread(lua_State* thread);

    static void queueThread(lua_State* thread);
    static void queueForResume(lua_State* thread, int arg_count);

    // TODO: the api here is inconsistent (console instead of on_kill) since we only use this function in one place (tests)
    static void startFunctionOnNewThread(lua_State* L, Feedback feedback, Console* console = nullptr);
    static void startCodeOnNewThread(lua_State* L, const char* chunk_name, const char* code, size_t code_size, Feedback feedback, OnKill on_kill = nullptr, Console* console = nullptr);

    static int yieldThread(lua_State* thread);
    static int yieldForWork(lua_State* thread, Workload work);

    static void run();

    static void cleanup();
};

#define getTask(thread) (static_cast<Task*>(lua_getthreaddata(thread)))

const char* taskStatusTostring(TaskStatus status);
const char* capabilityTostring(ThreadCapability capability);

void open_tasklib(lua_State* L);

int fr_task_spawn(lua_State* L);

}; // namespace fakeroblox
