#include "classes/numberrange.hpp"
#include "common.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushNumberRange(lua_State* L, float min, float max) {
    NumberRange* numberrange = static_cast<NumberRange*>(lua_newuserdata(L, sizeof(NumberRange)));
    numberrange->min = min;
    numberrange->max = max;

    luaL_getmetatable(L, "NumberRange");
    lua_setmetatable(L, -2);

    return 1;
}
int pushNumberRange(lua_State* L, NumberRange numberrange) {
    return pushNumberRange(L, numberrange.min, numberrange.max);
}

static int NumberRange_new(lua_State* L) {
    const double min = luaL_checknumber(L, 1);
    const double max = luaL_optnumberloose(L, 2, min);

    if (min > max)
        luaL_error(L, "NumberRange: invalid range");

    return pushNumberRange(L, min, max);
}

bool lua_isnumberrange(lua_State* L, int index) {
    return luaL_isudatareal(L, index, "NumberRange");
}
NumberRange* lua_checknumberrange(lua_State* L, int index) {
    void* ud = luaL_checkudatareal(L, index, "NumberRange");

    return static_cast<NumberRange*>(ud);
}

static int NumberRange__tostring(lua_State* L) {
    NumberRange* numberrange = lua_checknumberrange(L, 1);

    lua_pushfstringL(L, "%.*f %.*f", decimalFmt(numberrange->min), decimalFmt(numberrange->max));
    return 1;
}

static int NumberRange__index(lua_State* L) {
    NumberRange* numberrange = lua_checknumberrange(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Min"))
        lua_pushnumber(L, numberrange->min);
    else if (strequal(key, "Max"))
        lua_pushnumber(L, numberrange->max);
    else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of NumberRange", key);
}
static int NumberRange__newindex(lua_State* L) {
    lua_checknumberrange(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Min") || strequal(key, "Max"))
        luaL_error(L, "%s member of NumberRange is read-only and cannot be assigned to", key);

    luaL_error(L, "%s is not a valid member of NumberRange", key);

    return 0;
}

void open_numberrangelib(lua_State *L) {
    // NumberRange
    lua_newtable(L);

    setfunctionfield(L, NumberRange_new, "new", true);

    lua_setglobal(L, "NumberRange");

    // metatable
    luaL_newmetatable(L, "NumberRange");

    settypemetafield(L, "NumberRange");
    setfunctionfield(L, NumberRange__tostring, "__tostring");
    setfunctionfield(L, NumberRange__index, "__index");
    setfunctionfield(L, NumberRange__newindex, "__newindex");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
