#include "classes/udim.hpp"
#include "common.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushUDim(lua_State* L, double scale, double offset) {
    UDim* udim = static_cast<UDim*>(lua_newuserdata(L, sizeof(UDim)));
    udim->scale = scale;
    udim->offset = offset;

    luaL_getmetatable(L, "UDim");
    lua_setmetatable(L, -2);

    return 1;
}
int pushUDim(lua_State* L, UDim udim) {
    return pushUDim(L, udim.scale, udim.offset);
}

static int UDim_new(lua_State* L) {
    double scale = luaL_optnumberloose(L, 1, 0.0);
    double offset = luaL_optnumberloose(L, 2, 0.0);

    return pushUDim(L, scale, offset);
}

bool lua_isudim(lua_State* L, int index) {
    if (!lua_isuserdata(L, index))
        return false;

    if (!lua_getmetatable(L, index))
        return false;

    luaL_getmetatable(L, "UDim");
    bool result = lua_equal(L, -2, -1);
    lua_pop(L, 2);

    return result;
}
UDim* lua_checkudim(lua_State* L, int index) {
    void* ud = luaL_checkudata(L, index, "UDim");

    return static_cast<UDim*>(ud);
}

static int UDim__tostring(lua_State* L) {
    UDim* udim = static_cast<UDim*>(luaL_checkudata(L, 1, "UDim"));

    lua_pushfstringL(L, "%.f, %.f", udim->scale, udim->offset);
    return 1;
}

static int UDim__index(lua_State* L) {
    UDim* udim = static_cast<UDim*>(luaL_checkudata(L, 1, "UDim"));
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Scale"))
        lua_pushnumber(L, udim->scale);
    else if (strequal(key, "Offset"))
        lua_pushnumber(L, udim->offset);
    else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of UDim", key);
}
static int UDim__newindex(lua_State* L) {
    // UDim* udim = static_cast<UDim*>(luaL_checkudata(L, 1, "UDim"));
    luaL_checkudata(L, 1, "UDim");
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Scale") || strequal(key, "Offset"))
        luaL_error(L, "%s member of UDim is read-only and cannot be assigned to", key);

    luaL_error(L, "%s is not a valid member of UDim", key);

    return 0;
}

static int UDim__add(lua_State* L) {
    UDim* a = static_cast<UDim*>(luaL_checkudata(L, 1, "UDim"));
    UDim* b = static_cast<UDim*>(luaL_checkudata(L, 2, "UDim"));

    return pushUDim(L, a->scale + b->scale, a->offset + b->offset);
}
static int UDim__sub(lua_State* L) {
    UDim* a = static_cast<UDim*>(luaL_checkudata(L, 1, "UDim"));
    UDim* b = static_cast<UDim*>(luaL_checkudata(L, 2, "UDim"));

    return pushUDim(L, a->scale - b->scale, a->offset - b->offset);
}

void open_udimlib(lua_State *L) {
    // UDim
    lua_newtable(L);

    setfunctionfield(L, UDim_new, "new", true);

    lua_setglobal(L, "UDim");

    // metatable
    luaL_newmetatable(L, "UDim");

    settypemetafield(L, "UDim");
    setfunctionfield(L, UDim__tostring, "__tostring");
    setfunctionfield(L, UDim__index, "__index");
    setfunctionfield(L, UDim__newindex, "__newindex");
    setfunctionfield(L, UDim__add, "__add");
    setfunctionfield(L, UDim__sub, "__sub");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
