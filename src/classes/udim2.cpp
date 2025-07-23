#include "classes/udim2.hpp"
#include "classes/udim.hpp"
#include "common.hpp"

#include <raylib.h>

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushUDim2(lua_State* L, UDim x, UDim y) {
    UDim2* udim2 = static_cast<UDim2*>(lua_newuserdata(L, sizeof(UDim2)));
    udim2->x = x;
    udim2->y = y;

    luaL_getmetatable(L, "UDim2");
    lua_setmetatable(L, -2);

    return 1;
}
int pushUDim2(lua_State* L, UDim2 udim2) {
    return pushUDim2(L, udim2.x, udim2.y);
}

static int UDim2_new(lua_State* L) {
    int argc = lua_gettop(L);
    if (argc < 1)
        return pushUDim2(L, { .scale = 0, .offset = 0 }, { .scale = 0, .offset = 0 });

    UDim x{0, 0};
    UDim y{0, 0};

    if (lua_isudim(L, 1)) {
        x = *lua_checkudim(L, 1);
        if (argc > 1 && lua_isudim(L, 2))
            y = *lua_checkudim(L, 2);
    } else {
        x.scale = luaL_optnumberloose(L, 1, 0.0);
        x.offset = luaL_optnumberloose(L, 2, 0.0);
        y.scale = luaL_optnumberloose(L, 3, 0.0);
        y.offset = luaL_optnumberloose(L, 4, 0.0);
    }

    return pushUDim2(L, x, y);
}

UDim2* lua_checkudim2(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, "UDim2");

    return static_cast<UDim2*>(ud);
}

static int UDim2__tostring(lua_State* L) {
    UDim2* udim2 = static_cast<UDim2*>(luaL_checkudata(L, 1, "UDim2"));

    lua_pushfstringL(L, "{%.f, %.f}, {%.f, %.f}", udim2->x.scale, udim2->x.offset, udim2->y.scale, udim2->y.offset);
    return 1;
}

static int UDim2__index(lua_State* L) {
    UDim2* udim2 = static_cast<UDim2*>(luaL_checkudata(L, 1, "UDim2"));
    const char* key = luaL_checkstring(L, 2);

    // FIXME: methods
    if (strequal(key, "X") || strequal(key, "Width"))
        pushUDim(L, udim2->x);
    else if (strequal(key, "Y") || strequal(key, "Height"))
        pushUDim(L, udim2->y);
    else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of UDim2", key);
}
static int UDim2__newindex(lua_State* L) {
    // UDim2* udim2 = static_cast<UDim2*>(luaL_checkudata(L, 1, "UDim2"));
    luaL_checkudata(L, 1, "UDim2");
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "X") || strequal(key, "Width") ||
        strequal(key, "Y") || strequal(key, "Height")
    )
        luaL_error(L, "%s member of UDim2 is read-only and cannot be assigned to", key);

    luaL_error(L, "%s is not a valid member of UDim2", key);

    return 0;
}
static int UDim2__namecall(lua_State* L) {
    // UDim2* udim2 = static_cast<UDim2*>(luaL_checkudata(L, 1, "UDim2"));
    luaL_checkudata(L, 1, "UDim2");
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");
    // std::string method_name = namecall;

    luaL_error(L, "INTERNAL ERROR: TODO UDim2 methods");
    return 0;
}

static int UDim2__add(lua_State* L) {
    UDim2* a = static_cast<UDim2*>(luaL_checkudata(L, 1, "UDim2"));
    UDim2* b = static_cast<UDim2*>(luaL_checkudata(L, 2, "UDim2"));

    return pushUDim2(L, {
        .scale = a->x.scale + b->x.scale,
        .offset = a->x.offset + b->x.offset
    }, {
        .scale = a->y.scale + b->y.scale,
        .offset = a->y.offset + b->y.offset
    });
}
static int UDim2__sub(lua_State* L) {
    UDim2* a = static_cast<UDim2*>(luaL_checkudata(L, 1, "UDim2"));
    UDim2* b = static_cast<UDim2*>(luaL_checkudata(L, 2, "UDim2"));

    return pushUDim2(L, {
        .scale = a->x.scale - b->x.scale,
        .offset = a->x.offset - b->x.offset
    }, {
        .scale = a->y.scale - b->y.scale,
        .offset = a->y.offset - b->y.offset
    });
}

void open_udim2lib(lua_State *L) {
    // UDim2
    lua_newtable(L);

    setfunctionfield(L, UDim2_new, "new", true);

    lua_setglobal(L, "UDim2");

    // metatable
    luaL_newmetatable(L, "UDim2");

    setfunctionfield(L, UDim2__tostring, "__tostring");
    setfunctionfield(L, UDim2__index, "__index");
    setfunctionfield(L, UDim2__newindex, "__newindex");
    setfunctionfield(L, UDim2__namecall, "__namecall");
    setfunctionfield(L, UDim2__add, "__add");
    setfunctionfield(L, UDim2__sub, "__sub");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
