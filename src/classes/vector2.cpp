#include "classes/vector2.hpp"
#include "common.hpp"

#include <cstring>
#include <raylib.h>

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushVector2(lua_State* L, double x, double y) {
    Vector2* vector2 = static_cast<Vector2*>(lua_newuserdata(L, sizeof(Vector2)));
    vector2->x = x;
    vector2->y = y;

    luaL_getmetatable(L, "Vector2");
    lua_setmetatable(L, -2);

    return 1;
}
int pushVector2(lua_State* L, Vector2 vector2) {
    return pushVector2(L, vector2.x, vector2.y);
}

static int Vector2_new(lua_State* L) {
    double x = luaL_optnumber(L, 1, 0);
    double y = luaL_optnumber(L, 2, 0);

    return pushVector2(L, x, y);
}

Vector2* lua_checkvector2(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, "Vector2");

    return static_cast<Vector2*>(ud);
}

static int Vector2__tostring(lua_State* L) {
    Vector2* vector2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));

    lua_pushfstringL(L, "%.f, %.f", vector2->x, vector2->y);
    return 1;
}

static int Vector2__index(lua_State* L) {
    Vector2* vector2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    const char* key = luaL_checkstring(L, 2);

    // FIXME: methods
    if (strlen(key) == 1) {
        switch (*key) {
            case 'x':
            case 'X':
                lua_pushnumber(L, vector2->x);
                break;
            case 'y':
            case 'Y':
                lua_pushnumber(L, vector2->y);
                break;
            default:
                goto INVALID;
        }
    }

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of Vector2", key);
}
static int Vector2__newindex(lua_State* L) {
    // Vector2* vector2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    luaL_checkudata(L, 1, "Vector2");
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
static int Vector2__namecall(lua_State* L) {
    // Vector2* vector2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    luaL_checkudata(L, 1, "Vector2");
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");
    // std::string method_name = namecall;

    luaL_error(L, "INTERNAL ERROR: TODO Vector2 methods");
    return 0;
}

static int Vector2__add(lua_State* L) {
    Vector2* a = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    Vector2* b = static_cast<Vector2*>(luaL_checkudata(L, 2, "Vector2"));

    return pushVector2(L, a->x + b->x, a->y + b->y);
}
static int Vector2__sub(lua_State* L) {
    Vector2* a = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    Vector2* b = static_cast<Vector2*>(luaL_checkudata(L, 2, "Vector2"));

    return pushVector2(L, a->x - b->x, a->y - b->y);
}
static int Vector2__mul(lua_State* L) {
    Vector2* vector2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    double scalar = luaL_checknumber(L, 2);

    return pushVector2(L, vector2->x * scalar, vector2->y * scalar);
}
static int Vector2__div(lua_State* L) {
    Vector2* vector2 = static_cast<Vector2*>(luaL_checkudata(L, 1, "Vector2"));
    double scalar = luaL_checknumber(L, 2);

    return pushVector2(L, vector2->x / scalar, vector2->y / scalar);
}

void open_vector2lib(lua_State *L) {
    // Vector2
    lua_newtable(L);

    setfunctionfield(L, Vector2_new, "new", true);

    lua_setglobal(L, "Vector2");

    // metatable
    luaL_newmetatable(L, "Vector2");

    setfunctionfield(L, Vector2__tostring, "__tostring");
    setfunctionfield(L, Vector2__index, "__index");
    setfunctionfield(L, Vector2__newindex, "__newindex");
    setfunctionfield(L, Vector2__namecall, "__namecall");
    setfunctionfield(L, Vector2__add, "__add");
    setfunctionfield(L, Vector2__sub, "__sub");
    setfunctionfield(L, Vector2__mul, "__mul");
    setfunctionfield(L, Vector2__div, "__div");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
