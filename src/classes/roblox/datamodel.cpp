#include "classes/roblox/datamodel.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/instance.hpp"

#include "common.hpp"
#include "taskscheduler.hpp"
#include "http.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

std::shared_ptr<rbxInstance> DataModel::instance;
bool DataModel::shutdown = false;

#define TOCLOSEBINDS_KEY "datamodeltoclosebinds"

void DataModel::onShutdown(lua_State* L) {
    assert(instance);

    pushFunctionFromLookup(L, fr_task_spawn, "spawn");

    auto& on_close = getInstanceValue<rbxCallback>(instance, "OnClose");

    if (on_close.index != -1) {
        lua_pushvalue(L, -1);
        lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
        lua_rawgeti(L, -1, on_close.index);
        lua_remove(L, -2);

        lua_call(L, 1, 0);
    }

    lua_getfield(L, LUA_REGISTRYINDEX, TOCLOSEBINDS_KEY);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pushvalue(L, -4); // task.spawn
        lua_pushvalue(L, -2); // value

        lua_call(L, 1, 0);

        lua_pop(L, 1);
    }

    pushFunctionFromLookup(L, fireRBXScriptSignal);
    instance->pushEvent(L, "Close");

    lua_call(L, 1, 0);
}

namespace rbxInstance_DataModel_methods {
    static int shutdown(lua_State* L) {
        lua_checkinstance(L, 1, "DataModel");

        DataModel::shutdown = true;

        return 0;
    }
    static int bindToClose(lua_State* L) {
        lua_checkinstance(L, 1, "DataModel");
        luaL_checktype(L, 2, LUA_TFUNCTION);

        lua_getfield(L, LUA_REGISTRYINDEX, TOCLOSEBINDS_KEY);

        lua_pushvalue(L, 2);
        lua_rawseti(L, 3, lua_objlen(L, 3) + 1);

        return 0;
    }
    static int httpGet(lua_State* L) {
        lua_checkinstance(L, 1, "DataModel");
        std::string url = luaL_checkstring(L, 2);

        return TaskScheduler::yieldForWork(L, [url] (lua_State* thread) {
            struct MemoryStruct chunk;

            CURLcode res = newGetRequest(url.c_str(), &chunk);
            if (res)
                luaL_errorL(thread, "failed to make HTTP GET request (%d)", res);

            lua_pushlstring(thread, chunk.memory, chunk.size);
            if (chunk.memory) free(chunk.memory);

            return 1;
        });
    }
}; // namespace rbxInstance_DataModel_methods

void rbxInstance_DataModel_init(lua_State* L) {
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, TOCLOSEBINDS_KEY);

    rbxClass::class_map["DataModel"]->methods["Shutdown"].func = rbxInstance_DataModel_methods::shutdown;
    rbxClass::class_map["DataModel"]->methods["BindToClose"].func = rbxInstance_DataModel_methods::bindToClose;

    rbxClass::class_map["DataModel"]->newMethod("HttpGet", rbxInstance_DataModel_methods::httpGet);
}

};
