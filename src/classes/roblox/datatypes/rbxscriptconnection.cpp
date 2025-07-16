#include "classes/roblox/datatypes/rbxscriptconnection.hpp"
#include "common.hpp"

#include "lualib.h"

namespace fakeroblox {

void rbxScriptConnection::destroy(lua_State* L) {
    if (!alive)
        return;

    lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
    lua_pushnil(L);
    lua_rawseti(L, -2, function_index);

    alive = false;
}

int pushNewRbxScriptConnection(lua_State* L, int func_index) {
    rbxScriptConnection* connection = static_cast<rbxScriptConnection*>(lua_newuserdata(L, sizeof(rbxScriptConnection)));
    new(connection) rbxScriptConnection();

    luaL_getmetatable(L, "RbxScriptConnection");
    lua_setmetatable(L, -2);

    lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
    connection->function_index = addToLookup(L, [&L, &func_index] {
        lua_pushvalue(L, func_index);
    }, false);

    return 1;
}

rbxScriptConnection* lua_checkrbxscriptconnection(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, "RbxScriptConnection");

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
    lua_checkrbxscriptconnection(L, 1);
    const char* key = lua_tostring(L, 2);

    lua_CFunction func = getrbxScriptConnectionMethod(key);
    if (!func)
        luaL_error(L, "%s is not a valid member of RbxScriptConnection", key);

    return pushFunctionFromLookup(L, func, key);
}
static int rbxScriptConnection__namecall(lua_State* L) {
    lua_checkrbxscriptconnection(L, 1);
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");

    lua_CFunction func = getrbxScriptConnectionMethod(namecall);
    if (!func)
        luaL_error(L, "%s is not a valid member of RbxScriptConnection", namecall);

    return func(L);
}

void setup_rbxscriptconnection(lua_State *L) {
    luaL_newmetatable(L, "RbxScriptConnection");

    setfunctionfield(L, rbxScriptConnection__tostring, "__tostring", nullptr);
    setfunctionfield(L, rbxScriptConnection__index, "__index", nullptr);
    // TODO: __newindex that errors 'readonly'
    setfunctionfield(L, rbxScriptConnection__namecall, "__namecall", nullptr);

    lua_pop(L, 1);
}

}; // namespace fakeroblox
