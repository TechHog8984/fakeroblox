#include "common.hpp"
#include "libraries/tasklib.hpp"

#include "lua.h"
#include "lualib.h"
#include <cassert>
#include <cstring>

namespace fakeroblox {

double luaL_checknumberrange(lua_State* L, int narg, double min, double max) {
    double n = luaL_checknumber(L, narg);
    luaL_argcheck(L, n >= min && n <= max, narg, "invalid range");
    return n;
}
double luaL_optnumberrange(lua_State* L, int narg, double min, double max, double def) {
    double n = def;
    if (lua_isnumber(L, narg)) {
        n = lua_tonumber(L, narg);
        luaL_argcheck(L, n >= min && n <= max, narg, "invalid range");
    }
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

int pushFunctionFromLookup(lua_State* L, lua_CFunction func, const char* name, lua_Continuation cont) {
    return pushFromLookup(L, METHODLOOKUP, reinterpret_cast<void*>(func), [&L, func, name, cont] {
        const char* namecstr = name == nullptr ? nullptr : getFromStringLookup(L, addToStringLookup(L, name));

        if (cont)
            lua_pushcclosurek(L, func, namecstr, 0, cont);
        else
            lua_pushcfunction(L, func, namecstr);
    });
}

int addToLookup(lua_State *L, const char* lookup, std::function<void()> pushValue) {
    lua_getfield(L, LUA_REGISTRYINDEX, lookup);
    pushValue();

    lua_getglobal(L, "table");
    lua_getfield(L, -1, "find");
    lua_remove(L, -2); // table

    lua_pushvalue(L, -3); // lookup
    lua_pushvalue(L, -3); // value

    lua_call(L, 2, 1);

    bool cached = lua_isnumber(L, -1);
    int index = cached ? lua_tonumber(L, -1) : lua_objlen(L, -3) + 1;
    lua_pop(L, 1); // table.find result

    if (!cached)
        lua_rawseti(L, -2, index); // lookup[index] = value

    lua_pop(L, 1 + cached); // lookup and value (if cached)

    return index;
}

int addToStringLookup(lua_State *L, std::string string) {
    return addToLookup(L, STRINGLOOKUP, [&L, &string]{
        lua_pushlstring(L, string.c_str(), string.size());
    });
}
const char* getFromStringLookup(lua_State *L, int index) {
    lua_getfield(L, LUA_REGISTRYINDEX, STRINGLOOKUP);
    lua_rawgeti(L, -1, index);

    const char* str = lua_tostring(L, -1);

    lua_pop(L, 2);
    return str;
}

std::string getStackMessage(lua_State* L) {
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
    return message;
}

int log(lua_State* L, Console::Message::Type type) {
    TaskScheduler::getTaskFromThread(L)->console->log(getStackMessage(L), type);
    return 0;
}
// these functions are exposed in environment.cpp
int fr_print(lua_State* L) {
    return log(L, Console::Message::INFO);
}
int fr_warn(lua_State* L) {
    return log(L, Console::Message::WARNING);
}

void setfunctionfield(lua_State *L, lua_CFunction func, const char *method, const char* debugname, bool lookup) {
    assert(lua_istable(L, -1));

    if (lookup)
        pushFunctionFromLookup(L, func, debugname);
    else
        lua_pushcfunction(L, func, debugname);
    lua_setfield(L, -2, method);
}
void setfunctionfield(lua_State *L, lua_CFunction func, const char *method, bool lookup) {
    setfunctionfield(L, func, method, nullptr, lookup);
}

}; // namespace fakeroblox
