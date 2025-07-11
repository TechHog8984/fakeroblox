#include "classes/color3.hpp"
#include "common.hpp"

#include <cstring>

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushColor3(lua_State* L, double r, double g, double b) {
    Color* color = static_cast<Color*>(lua_newuserdata(L, sizeof(Color)));
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = 255;

    luaL_getmetatable(L, "Color3");
    lua_setmetatable(L, -2);

    return 1;
}
int pushColor3(lua_State* L, Color color) {
    return pushColor3(L, color.r, color.g, color.b);
}

static int Color3_new(lua_State* L) {
    double r = luaL_optnumberrange(L, 1, 0, 1, 0);
    double g = luaL_optnumberrange(L, 2, 0, 1, 0);
    double b = luaL_optnumberrange(L, 3, 0, 1, 0);

    return pushColor3(L, r * 255, g * 255, b * 255);
}
static int Color3_fromRGB(lua_State* L) {
    double r = luaL_optnumberrange(L, 1, 0, 255, 0);
    double g = luaL_optnumberrange(L, 2, 0, 255, 0);
    double b = luaL_optnumberrange(L, 3, 0, 255, 0);

    return pushColor3(L, r, g, b);
}

Color* lua_checkcolor3(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, "Color3");

    return static_cast<Color*>(ud);
}

static int Color3__tostring(lua_State* L) {
    Color* color = static_cast<Color*>(luaL_checkudata(L, 1, "Color3"));

    lua_pushfstringL(L, "%i, %i, %i", color->r, color->g, color->b);
    return 1;
}

static int Color3__index(lua_State* L) {
    Color* color = static_cast<Color*>(luaL_checkudata(L, 1, "Color3"));
    const char* key = luaL_checkstring(L, 2);

    // FIXME: methods
    if (strlen(key) == 1) {
        switch (*key) {
            case 'r':
            case 'R':
                lua_pushnumber(L, color->r);
                break;
            case 'g':
            case 'G':
                lua_pushnumber(L, color->g);
                break;
            case 'b':
            case 'B':
                lua_pushnumber(L, color->b);
                break;
            default:
                goto INVALID;
        }
    }

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of Color3", key);
}
static int Color3__newindex(lua_State* L) {
    // Color* color = static_cast<Color*>(luaL_checkudata(L, 1, "Color3"));
    luaL_checkudata(L, 1, "Color3");
    const char* key = luaL_checkstring(L, 2);

    if (strlen(key) == 1) {
        switch (*key) {
            case 'r':
            case 'R':
            case 'g':
            case 'G':
            case 'b':
            case 'B':
                luaL_error(L, "%s member of Color3 is read-only and cannot be assigned to", key);
            default:
                goto INVALID;
        }
    }

    INVALID:
    luaL_error(L, "%s is not a valid member of Color3", key);

    return 0;
}
static int Color3__namecall(lua_State* L) {
    // Color* color = static_cast<Color*>(luaL_checkudata(L, 1, "Color3"));
    luaL_checkudata(L, 1, "Color3");
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");
    // std::string method_name = namecall;

    luaL_error(L, "INTERNAL ERROR: TODO Color3 methods");
    return 0;
}

void open_color3lib(lua_State *L) {
    // Color3
    lua_newtable(L);

    setfunctionfield(L, Color3_new, "new", true);
    setfunctionfield(L, Color3_fromRGB, "fromRGB", true);

    lua_setglobal(L, "Color3");

    // metatable
    luaL_newmetatable(L, "Color3");

    setfunctionfield(L, Color3__tostring, "__tostring");
    setfunctionfield(L, Color3__index, "__index");
    setfunctionfield(L, Color3__newindex, "__newindex");
    setfunctionfield(L, Color3__namecall, "__namecall");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
