#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/datatypes/rbxscriptconnection.hpp"
#include "common.hpp"
#include "libraries/tasklib.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

void rbxScriptSignal::destroy(lua_State* L) {
    // FIXME: disconnect connections
}

int pushNewRbxScriptSignal(lua_State* L, std::string name) {
    lua_getfield(L, LUA_REGISTRYINDEX, SIGNALCONNECTIONLISTLOOKUP);

    rbxScriptSignal* signal = static_cast<rbxScriptSignal*>(lua_newuserdata(L, sizeof(rbxScriptSignal)));
    new(signal) rbxScriptSignal();

    signal->name.assign(name);

    luaL_getmetatable(L, "RbxScriptSignal");
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -1);
    lua_newtable(L);
    lua_rawset(L, -4);

    lua_remove(L, -2);

    return 1;
}

rbxScriptSignal* lua_checkrbxscriptsignal(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, "RbxScriptSignal");

    return static_cast<rbxScriptSignal*>(ud);
}

void pushSignalConnectionList(lua_State* L, int narg) {
    if (narg < 0 && narg > LUA_REGISTRYINDEX)
        narg--;

    lua_getfield(L, LUA_REGISTRYINDEX, SIGNALCONNECTIONLISTLOOKUP);
    lua_pushvalue(L, narg);
    lua_rawget(L, -2);
    lua_remove(L, -2);
}

namespace rbxScriptSignal_methods {
    static int connect(lua_State* L) {
        lua_checkrbxscriptsignal(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        pushNewRbxScriptConnection(L, 2);

        pushSignalConnectionList(L, 1);
        lua_pushvalue(L, -2);

        lua_rawseti(L, -2, lua_objlen(L, -2) + 1);
        lua_pop(L, 1);

        return 1;
    }
};
lua_CFunction getrbxScriptSignalMethod(const char* key) {
    if (strequal(key, "Connect") || strequal(key, "connect"))
        return rbxScriptSignal_methods::connect;

    return nullptr;
}

static int rbxScriptSignal__index(lua_State* L) {
    lua_checkrbxscriptsignal(L, 1);
    const char* key = lua_tostring(L, 2);

    lua_CFunction func = getrbxScriptSignalMethod(key);
    if (!func)
        luaL_error(L, "%s is not a valid member of RbxScriptSignal", key);

    return pushFunctionFromLookup(L, func, key);
}
static int rbxScriptSignal__namecall(lua_State* L) {
    lua_checkrbxscriptsignal(L, 1);
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");

    lua_CFunction func = getrbxScriptSignalMethod(namecall);
    if (!func)
        luaL_error(L, "%s is not a valid member of RbxScriptSignal", namecall);

    return func(L);
}

int fireRbxScriptSignalWithFilter(lua_State* L) {
    lua_checkrbxscriptsignal(L, 1);
    bool use_filter = !lua_isnil(L, 2);
    if (use_filter)
        luaL_checktype(L, 2, LUA_TFUNCTION);

    int arg_count = lua_gettop(L) - 2;

    pushFunctionFromLookup(L, fakeroblox_task_spawn, "spawn");

    pushSignalConnectionList(L, 1);
    lua_remove(L, 1);

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        lua_pushvalue(L, -4); // task.spawn
        lua_pushvalue(L, -2); // connection

        rbxScriptConnection* connection = lua_checkrbxscriptconnection(L, -1);
        if (!connection->alive) {
            lua_pop(L, 3);
            continue;
        }

        int function_index = connection->function_index;
        lua_remove(L, -1);

        lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
        lua_rawgeti(L, -1, function_index); // function
        lua_remove(L, -2);

        if (use_filter) {
            lua_pushvalue(L, 1); // filter
            lua_pushvalue(L, -2); // function
            lua_call(L, 1, 1);

            bool skip = luaL_optboolean(L, -1, false);
            lua_pop(L, 1);

            if (skip) {
                lua_pop(L, 3);
                continue;
            }
        }

        for (int i = 0; i < arg_count; i++)
            lua_pushvalue(L, 2 + i);

        lua_call(L, arg_count + 1, 0);

        lua_pop(L, 1);
    }

    return 0;
}
int fireRbxScriptSignal(lua_State* L) {
    lua_pushnil(L);
    lua_insert(L, 2);

    return fireRbxScriptSignalWithFilter(L);
}

void setup_rbxscriptsignal(lua_State *L) {
    // signallookup
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, SIGNALLOOKUP);

    // connection list lookup
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, SIGNALCONNECTIONLISTLOOKUP);

    // metatable
    luaL_newmetatable(L, "RbxScriptSignal");

    setfunctionfield(L, rbxScriptSignal__index, "__index", nullptr);
    // TODO: __newindex that errors 'readonly'
    setfunctionfield(L, rbxScriptSignal__namecall, "__namecall", nullptr);

    lua_pop(L, 1);

    setup_rbxscriptconnection(L);
}

}; // namespace fakeroblox
