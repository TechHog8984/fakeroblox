#include "common.hpp"
#include "libraries/tasklib.hpp"

#include "lualib.h"
#include <cassert>

namespace fakeroblox {

double luaL_checknumberrange(lua_State* L, int narg, double min, double max) {
    double n = luaL_checknumber(L, narg);
    luaL_argcheck(L, n >= min && n <= max, narg, "invalid range");
    return n;
}

int newweaktable(lua_State* L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    lua_pushstring(L, "kvs");
    lua_setfield(L, -2, "__mode");

    return 1;
}
int pushFromLookup(lua_State* L, const char* lookup, void* ptr, std::function<void()> pushValue) {
    lua_getfield(L, LUA_REGISTRYINDEX, lookup);
    lua_pushlightuserdata(L, ptr);
    lua_rawget(L, -2);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        pushValue();

        lua_pushlightuserdata(L, ptr);
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
    }

    lua_remove(L, -2);

    return 1;
}
// FIXME: rbxInstance method names when erroring will appear something like: 'GetFullN[string "script1"]'
int pushFunctionFromLookup(lua_State* L, lua_CFunction func, std::string name, lua_Continuation cont) {
    return pushFromLookup(L, METHODLOOKUP, reinterpret_cast<void*>(func), [&L, &func, &name, &cont] {
        if (cont)
            lua_pushcclosurek(L, func, name.c_str(), 0, cont);
        else
            lua_pushcfunction(L, func, name.c_str());
    });
}

int log(lua_State* L, Console::Message::Type type) {
    std::string message;
    int n = lua_gettop(L);
    for (int i = 0; i < n; i++) {
        size_t l;
        const char* s = luaL_tolstring(L, i + 1, &l);
        if (i > 0)
            message += '\t';
        message.append(s, l);
        lua_pop(L, 1);
    }
    TaskScheduler::getTaskFromThread(L)->console->log(message, type);
    return 0;
}
// these functions are exposed in environment.cpp
int fr_print(lua_State* L) {
    return log(L, Console::Message::INFO);
}
int fr_warn(lua_State* L) {
    return log(L, Console::Message::WARNING);
}

void setfunctionfield(lua_State *L, lua_CFunction func, const char *method, bool lookup) {
    assert(lua_istable(L, -1));

    if (lookup)
        pushFunctionFromLookup(L, func, method);
    else
        lua_pushcfunction(L, func, method);
    lua_setfield(L, -2, method);
}

}; // namespace fakeroblox
