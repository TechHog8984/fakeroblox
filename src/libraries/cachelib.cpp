#include "libraries/cachelib.hpp"
#include "classes/roblox/instance.hpp"
#include "common.hpp"
#include "environment.hpp"
#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int fr_cache_invalidate(lua_State* L) {
    auto instance = lua_checkinstance(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, INSTANCELOOKUP);
    lua_pushlightuserdata(L, instance.get());
    lua_pushnil(L);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    return 0;
}

int fr_cache_iscached(lua_State* L) {
    auto instance = lua_checkinstance(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, INSTANCELOOKUP);
    lua_pushlightuserdata(L, instance.get());
    lua_rawget(L, -2);

    const bool result = !lua_isnil(L, -1);
    lua_pop(L, 2);

    lua_pushboolean(L, result);
    return 1;
}

int fr_cache_replace(lua_State* L) {
    auto instance = lua_checkinstance(L, 1);
    auto replacement = lua_checkinstance(L, 2);

    lua_getfield(L, LUA_REGISTRYINDEX, INSTANCELOOKUP);

    lua_pushlightuserdata(L, instance.get());

    lua_pushlightuserdata(L, replacement.get());
    lua_rawget(L, -3);

    const bool replacement_is_cached = !lua_isnil(L, -1);
    if (!replacement_is_cached) {
        // TODO: better error message?
        luaL_error(L, "the second instance is not cached");
    }

    lua_rawset(L, -3);

    lua_pop(L, 1);

    return 0;
}

static int fr_cloneref(lua_State* L) {
    auto instance = lua_checkinstance(L, 1);

    pushNewSharedPtrObject(L, instance);
    luaL_getmetatable(L, "Instance");
    lua_setmetatable(L, -2);

    return 1;
}

static int fr_compareinstances(lua_State* L) {
    auto a = lua_checkinstance(L, 1);
    auto b = lua_checkinstance(L, 2);

    lua_pushboolean(L, a == b);
    return 1;
}

void open_cachelib(lua_State* L) {
    // cache global
    lua_newtable(L);

    setfunctionfield(L, fr_cache_invalidate, "invalidate");
    setfunctionfield(L, fr_cache_iscached, "iscached");
    setfunctionfield(L, fr_cache_replace, "replace");

    lua_setglobal(L, "cache");

    env_expose(cloneref)
    env_expose(compareinstances)
}

}; // namespace fakeroblox
