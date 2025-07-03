#pragma once

#include <cassert>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

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
    static std::shared_mutex mutex;
    static std::vector<Task*> task_queue;
    static std::unordered_map<lua_State*, Task*> task_map;

    static void queueTask(Task* task);
    static void resumeTask(lua_State* L, Task* task);
    static void killTask(lua_State* L, Task* task);

    static Task* getTaskFromThread(lua_State* thread);
    static bool wasThreadKilled(lua_State* thread);

    static void run(lua_State* L);

    static void cleanup(lua_State* L);
};

class TaskTiming {
public:
    enum {
        Instant,
        Seconds
    } type = Instant;

    double end_time = 0.0;
};

class Task {
public:
    lua_State* thread;
    int thread_ref;
    Feedback feedback;
    TaskTiming timing;

    TaskStatus status = RUNNING;
    int arg_count = 0;    
    bool canceled = false;
    bool finished = false;

    Task(lua_State* thread, int thread_ref, Feedback feedback, TaskTiming timing)
        : thread(thread), thread_ref(thread_ref), feedback(feedback), timing(timing)
    {
        TaskScheduler::task_map.emplace(thread, this);
    }
};

void open_tasklib(lua_State* L);

std::optional<std::string> tryRunCode(lua_State* L, const char* source, Feedback feedback);

}; // namespace fakeroblox
