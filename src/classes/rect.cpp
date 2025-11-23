#include "classes/rect.hpp"
#include "classes/vector2.hpp"
#include "common.hpp"

#include <cmath>
#include <cstring>

#include "lnumutils.h"
#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushRect(lua_State* L, float minx, float miny, float maxx, float maxy) {
    Rect* rect = static_cast<Rect*>(lua_newuserdata(L, sizeof(Rect)));
    rect->minx = minx;
    rect->miny = miny;
    rect->maxx = maxx;
    rect->maxy = maxy;

    luaL_getmetatable(L, "Rect");
    lua_setmetatable(L, -2);

    return 1;
}
int pushRect(lua_State* L, Rect rect) {
    return pushRect(L, rect.minx, rect.miny, rect.maxx, rect.maxy);
}

static int Rect_new(lua_State* L) {
    const int argc = lua_gettop(L);
    if (argc == 0)
        return pushRect(L, 0.f, 0.f, 0.f, 0.f);
    else if (argc == 2) {
        auto min = lua_checkvector2(L, 1);
        auto max = lua_checkvector2(L, 2);

        return pushRect(L, min->x, min->y, max->x, max->y);
    } else if (argc == 4) {
        float minx = luaL_checknumber(L, 1);
        float miny = luaL_checknumber(L, 2);
        float maxx = luaL_checknumber(L, 3);
        float maxy = luaL_checknumber(L, 4);

        return pushRect(L, minx, miny, maxx, maxy);
    }
    luaL_error(L, "Invalid number of arguments: %d", argc);
}

Rect* lua_checkrect(lua_State* L, int narg) {
    void* ud = luaL_checkudatareal(L, narg, "Rect");

    return static_cast<Rect*>(ud);
}

static int Rect__tostring(lua_State* L) {
    Rect* rect = lua_checkrect(L, 1);

    lua_pushfstringL(L, "%.*f, %.*f, %.*f, %.*f", decimalFmt(rect->minx), decimalFmt(rect->miny), decimalFmt(rect->maxx), decimalFmt(rect->maxy));
    return 1;
}

static int Rect__index(lua_State* L) {
    Rect* rect = lua_checkrect(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Width"))
        lua_pushnumber(L, rect->maxx - rect->minx);
    else if (strequal(key, "Height"))
        lua_pushnumber(L, rect->maxy - rect->miny);
    else if (strequal(key, "Min"))
        pushVector2(L, rect->minx, rect->miny);
    else if (strequal(key, "Max"))
        pushVector2(L, rect->maxx, rect->maxy);
    else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of Rect", key);
}
#undef MAGNITUDE

static int Rect__newindex(lua_State* L) {
    lua_checkrect(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Width") || strequal(key, "Height") || strequal(key, "Min") || strequal(key, "Max"))
        luaL_error(L, "%s member of Rect is read-only and cannot be assigned to", key);

    luaL_error(L, "%s is not a valid member of Rect", key);

    return 0;
}

void open_rectlib(lua_State *L) {
    // Rect
    lua_newtable(L);

    setfunctionfield(L, Rect_new, "new", true);

    lua_setglobal(L, "Rect");

    // metatable
    luaL_newmetatable(L, "Rect");

    settypemetafield(L, "Rect");
    setfunctionfield(L, Rect__tostring, "__tostring");
    setfunctionfield(L, Rect__index, "__index");
    setfunctionfield(L, Rect__newindex, "__newindex");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
