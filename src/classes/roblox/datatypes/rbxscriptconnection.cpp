#include "classes/roblox/datatypes/rbxscriptconnection.hpp"
#include "common.hpp"

#include "lapi.h"
#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

void rbxScriptConnection::destroy(lua_State* L) {
    if (!alive)
        return;

    lua_getfield(L, LUA_REGISTRYINDEX, RBXSCRIPTCONNECTION_METHODLOOKUP);

    lua_rawgeti(L, -1, function_index);
    lua_pushvalue(L, -1);
    lua_rawget(L, -3);
    double n = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    n--;
    if (n) {
        lua_pushnumber(L, n);
        lua_rawset(L, -3);
    } else {
        lua_pushnil(L);
        lua_rawset(L, -3);

        lua_pushnil(L);
        lua_rawseti(L, -2, function_index);
    }
    lua_pop(L, 1);

    alive = false;
}

int pushNewRBXScriptConnection(lua_State* L, std::function<void()> pushValue) {
    rbxScriptConnection* connection = static_cast<rbxScriptConnection*>(lua_newuserdata(L, sizeof(rbxScriptConnection)));
    new(connection) rbxScriptConnection();

    luaL_getmetatable(L, "RBXScriptConnection");
    lua_setmetatable(L, -2);

    lua_getfield(L, LUA_REGISTRYINDEX, RBXSCRIPTCONNECTION_METHODLOOKUP);
    connection->function_index = addToLookup(L, pushValue, true);

    lua_getfield(L, LUA_REGISTRYINDEX, RBXSCRIPTCONNECTION_METHODLOOKUP);

    lua_insert(L, -2);
    lua_pushvalue(L, -1);

    lua_rawget(L, -3);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushnumber(L, 1);
        lua_rawset(L, -3);
    } else {
        // TODO: more efficient way of doing this?
        double value = luaL_checknumber(L, -1) + 1;
        lua_pop(L, 1);
        lua_pushnumber(L, value);
        lua_rawset(L, -3);
    }

    lua_pop(L, 1);

    return 1;
}
int pushNewRBXScriptConnection(lua_State* L, int func_index) {
    func_index = lua_absindex(L, func_index);
    return pushNewRBXScriptConnection(L, [&L, &func_index] () {
        lua_pushvalue(L, func_index);
    });
}

rbxScriptConnection* lua_checkrbxscriptconnection(lua_State* L, int narg) {
    void* ud = luaL_checkudatareal(L, narg, "RBXScriptConnection");

    return static_cast<rbxScriptConnection*>(ud);
}

namespace rbxScriptConnection_methods {
    static int disconnect(lua_State* L) {
        rbxScriptConnection* connection = lua_checkrbxscriptconnection(L, 1);

        connection->destroy(L);

        return 0;
    }
};

lua_CFunction getrbxScriptConnectionMethod(const char* key) {
    if (strequal(key, "Disconnect") || strequal(key, "disconnect"))
        return rbxScriptConnection_methods::disconnect;

    return nullptr;
}

static int rbxScriptConnection__tostring(lua_State* L) {
    lua_checkrbxscriptconnection(L, 1);

    lua_pushstring(L, "Connection");
    return 1;
}

static int rbxScriptConnection__index(lua_State* L) {
    rbxScriptConnection* connection = lua_checkrbxscriptconnection(L, 1);
    const char* key = lua_tostring(L, 2);

    if (strequal(key, "Connected") || strequal(key, "connected")) {
        lua_pushboolean(L, connection->alive);
        return 1;
    }

    lua_CFunction func = getrbxScriptConnectionMethod(key);
    if (!func)
        luaL_error(L, "%s is not a valid member of RBXScriptConnection", key);

    return pushFunctionFromLookup(L, func, key);
}
static int rbxScriptConnection__newindex(lua_State* L) {
    lua_checkrbxscriptconnection(L, 1);
    const char* key = lua_tostring(L, 2);

    luaL_error(L, "%s is not a valid member of RBXScriptConnection", key);
    return 0;
}
static int rbxScriptConnection__namecall(lua_State* L) {
    lua_checkrbxscriptconnection(L, 1);
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");

    lua_CFunction func = getrbxScriptConnectionMethod(namecall);
    if (!func)
        luaL_error(L, "%s is not a valid member of RBXScriptConnection", namecall);

    return func(L);
}

void setup_rbxScriptConnection(lua_State *L) {
    luaL_newmetatable(L, "RBXScriptConnection");

    settypemetafield(L, "RBXScriptConnection");
    setfunctionfield(L, rbxScriptConnection__tostring, "__tostring", nullptr);
    setfunctionfield(L, rbxScriptConnection__index, "__index", nullptr);
    setfunctionfield(L, rbxScriptConnection__newindex, "__newindex", nullptr);
    setfunctionfield(L, rbxScriptConnection__namecall, "__namecall", nullptr);

    lua_pop(L, 1);
}

}; // namespace fakeroblox
