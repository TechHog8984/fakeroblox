#include "classes/colorsequencekeypoint.hpp"
#include "classes/color3.hpp"
#include "common.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushColorSequenceKeypoint(lua_State* L, float time, Color value) {
    ColorSequenceKeypoint* colorsequencekeypoint = static_cast<ColorSequenceKeypoint*>(lua_newuserdata(L, sizeof(ColorSequenceKeypoint)));
    colorsequencekeypoint->time = time;
    colorsequencekeypoint->value = value;

    luaL_getmetatable(L, "ColorSequenceKeypoint");
    lua_setmetatable(L, -2);

    return 1;
}
int pushColorSequenceKeypoint(lua_State* L, ColorSequenceKeypoint colorsequencekeypoint) {
    return pushColorSequenceKeypoint(L, colorsequencekeypoint.time, colorsequencekeypoint.value);
}

static int ColorSequenceKeypoint_new(lua_State* L) {
    const double time = luaL_checknumber(L, 1);
    const Color* value = lua_checkcolor(L, 2);

    return pushColorSequenceKeypoint(L, time, *value);
}

bool lua_iscolorsequencekeypoint(lua_State* L, int index) {
    return luaL_isudatareal(L, index, "ColorSequenceKeypoint");
}
ColorSequenceKeypoint* lua_checkcolorsequencekeypoint(lua_State* L, int index) {
    void* ud = luaL_checkudatareal(L, index, "ColorSequenceKeypoint");

    return static_cast<ColorSequenceKeypoint*>(ud);
}

static int ColorSequenceKeypoint__tostring(lua_State* L) {
    ColorSequenceKeypoint* colorsequencekeypoint = lua_checkcolorsequencekeypoint(L, 1);

    lua_pushfstringL(L, "%.*f %.*f %.*f %.*f", decimalFmt(colorsequencekeypoint->time), decimalFmt(colorsequencekeypoint->value.r * 255.f), decimalFmt(colorsequencekeypoint->value.g * 255.f), decimalFmt(colorsequencekeypoint->value.b * 255.f));
    return 1;
}

static int ColorSequenceKeypoint__index(lua_State* L) {
    ColorSequenceKeypoint* colorsequencekeypoint = lua_checkcolorsequencekeypoint(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Time"))
        lua_pushnumber(L, colorsequencekeypoint->time);
    else if (strequal(key, "Value"))
        pushColor(L, colorsequencekeypoint->value);
    else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of ColorSequenceKeypoint", key);
}
static int ColorSequenceKeypoint__newindex(lua_State* L) {
    lua_checkcolorsequencekeypoint(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Time") || strequal(key, "Value"))
        luaL_error(L, "%s member of ColorSequenceKeypoint is read-only and cannot be assigned to", key);

    luaL_error(L, "%s is not a valid member of ColorSequenceKeypoint", key);

    return 0;
}

void open_colorsequencekeypointlib(lua_State *L) {
    // ColorSequenceKeypoint
    lua_newtable(L);

    setfunctionfield(L, ColorSequenceKeypoint_new, "new", true);

    lua_setglobal(L, "ColorSequenceKeypoint");

    // metatable
    luaL_newmetatable(L, "ColorSequenceKeypoint");

    settypemetafield(L, "ColorSequenceKeypoint");
    setfunctionfield(L, ColorSequenceKeypoint__tostring, "__tostring");
    setfunctionfield(L, ColorSequenceKeypoint__index, "__index");
    setfunctionfield(L, ColorSequenceKeypoint__newindex, "__newindex");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
