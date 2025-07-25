#include "classes/roblox/datamodel.hpp"
#include "classes/roblox/instance.hpp"
#include "libraries/tasklib.hpp"

#include "http.hpp"

#include "lua.h"
#include "lualib.h"

#include <thread>

namespace fakeroblox {

namespace rbxInstance_DataModel_methods {
    static int httpGet(lua_State* L) {
        lua_checkinstance(L, 1, "DataModel");
        std::string url = luaL_checkstring(L, 2);
        Task* task = TaskScheduler::getTaskFromThread(L);

        std::thread t([L, url, task] () {
            struct MemoryStruct chunk;

            CURLcode res = newGetRequest(url.c_str(), &chunk);
            if (res)
                luaL_errorL(L, "failed to make HTTP request (%d)", res);

            lua_pushlstring(L, chunk.memory, chunk.size);
            if (chunk.memory) free(chunk.memory);

            task->arg_count = 1;
            task->timing = TaskTiming { .type = TaskTiming::Instant };
            TaskScheduler::queueTask(task);
        });
        t.detach();

        task->status = TaskStatus::YIELDING;
        return lua_yield(L, 0);
    }
};

void rbxInstance_DataModel_init(lua_State* L) {
    rbxClass::class_map["DataModel"]->newMethod("HttpGet", rbxInstance_DataModel_methods::httpGet);
}

};
