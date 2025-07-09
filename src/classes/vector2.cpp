#include "classes/vector2.hpp"

#include <cstring>
#include <new>

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

Vector2::Vector2(double x, double y) : x(x), y(y) {}

int Vector2_new(lua_State* L) {
    double x = luaL_optnumber(L, 1, 0);
    double y = luaL_optnumber(L, 2, 0);

    void* ud = lua_newuserdata(L, sizeof(Vector2));
    new (ud) Vector2(x, y);

    luaL_getmetatable(L, "Vector2");
    lua_setmetatable(L, -2);

    return 1;
}

int Vector2__index(lua_State* L) {
    Vector2* v2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    const char* key = luaL_checkstring(L, 2);

    // FIXME: methods
    if (strlen(key) == 1) {
        switch (*key) {
            case 'x':
            case 'X':
                lua_pushnumber(L, v2->x);
                break;
            case 'y':
            case 'Y':
                lua_pushnumber(L, v2->y);
                break;
            default:
                goto INVALID;
        }
    }

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of Vector2", key);
}
int Vector2__newindex(lua_State* L) {
    Vector2* v2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    const char* key = luaL_checkstring(L, 2);

    if (strlen(key) == 1) {
        switch (*key) {
            case 'x':
            case 'X':
            case 'y':
            case 'Y':
                luaL_error(L, "%s member of Vector2 is read-only and cannot be assigned to", key);
            default:
                goto INVALID;
        }
    }

    INVALID:
    luaL_error(L, "%s is not a valid member of Vector2", key);

    return 0;
}
int Vector2__namecall(lua_State* L) {
    Vector2* v2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");
    // std::string method_name = namecall;

    luaL_error(L, "INTERNAL ERROR: TODO Vector2 methods");
    return 0;
}

void open_vector2lib(lua_State *L) {
    // Vector2
    lua_newtable(L);

    lua_pushcfunction(L, Vector2_new, "new");
    lua_setfield(L, -2, "new");

    lua_setglobal(L, "Vector2");

    // metatable
    luaL_newmetatable(L, "Vector2");

    lua_pushcfunction(L, Vector2__index, "__index");
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, Vector2__newindex, "__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, Vector2__namecall, "__namecall");
    lua_setfield(L, -2, "__namecall");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
