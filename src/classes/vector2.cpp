#include "classes/vector2.hpp"
#include "common.hpp"

#include <cmath>
#include <cstring>

#include "lnumutils.h"
#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushVector2(lua_State* L, float x, float y) {
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
    float x = luaL_optnumber(L, 1, 0);
    float y = luaL_optnumber(L, 2, 0);

    return pushVector2(L, x, y);
}

Vector2* lua_checkvector2(lua_State* L, int narg) {
    void* ud = luaL_checkudatareal(L, narg, "Vector2");

    return static_cast<Vector2*>(ud);
}

static int Vector2__tostring(lua_State* L) {
    Vector2* vector2 = lua_checkvector2(L, 1);

    lua_pushfstringL(L, "%.f, %.f", vector2->x, vector2->y);
    return 1;
}

#define MAGNITUDE(vector2) (sqrt(vector2->x * vector2->x + vector2->y * vector2->y))

static int Vector2__index(lua_State* L) {
    Vector2* vector2 = lua_checkvector2(L, 1);
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
    } else if (strequal(key, "Magnitude")) {
        lua_pushnumber(L, MAGNITUDE(vector2));
    } else if (strequal(key, "Unit") || strequal(key, "unit")) {
        float magnitude = MAGNITUDE(vector2);
        pushVector2(L, magnitude == 0 ? 0 : vector2->x / magnitude, magnitude == 0 ? 0 : vector2->y / magnitude);
    } else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of Vector2", key);
}
#undef MAGNITUDE

static int Vector2__newindex(lua_State* L) {
    lua_checkvector2(L, 1);
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
    lua_checkvector2(L, 1);
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");
    // std::string method_name = namecall;

    luaL_error(L, "INTERNAL ERROR: TODO Vector2 methods");
    return 0;
}

static int Vector2__add(lua_State* L) {
    Vector2* a = lua_checkvector2(L, 1);
    Vector2* b = lua_checkvector2(L, 2);

    return pushVector2(L, a->x + b->x, a->y + b->y);
}
static int Vector2__sub(lua_State* L) {
    Vector2* a = lua_checkvector2(L, 1);
    Vector2* b = lua_checkvector2(L, 2);

    return pushVector2(L, a->x - b->x, a->y - b->y);
}
static int Vector2__mul(lua_State* L) {
    Vector2* a = lua_checkvector2(L, 1);
    luaL_argcheck(L, lua_isnumber(L, 2) || lua_isuserdata(L, 2), 2, "expected number or userdata for Vector2 mul");
    if (lua_isnumber(L, 2)) {
        float scalar = luaL_checknumber(L, 2);

        return pushVector2(L, a->x * scalar, a->y * scalar);
    }

    Vector2* b = lua_checkvector2(L, 2);

    return pushVector2(L, a->x * b->x, a->y * b->y);
}
static int Vector2__div(lua_State* L) {
    Vector2* a = lua_checkvector2(L, 1);
    luaL_argcheck(L, lua_isnumber(L, 2) || lua_isuserdata(L, 2), 2, "expected number or userdata for Vector2 div");
    if (lua_isnumber(L, 2)) {
        float scalar = luaL_checknumber(L, 2);

        return pushVector2(L, a->x / scalar, a->y / scalar);
    }

    Vector2* b = lua_checkvector2(L, 2);

    return pushVector2(L, a->x / b->x, a->y / b->y);
}
static int Vector2__idiv(lua_State* L) {
    Vector2* a = lua_checkvector2(L, 1);
    luaL_argcheck(L, lua_isnumber(L, 2) || lua_isuserdata(L, 2), 2, "expected number or userdata for Vector2 idiv");
    if (lua_isnumber(L, 2)) {
        float scalar = luaL_checknumber(L, 2);

        return pushVector2(L, luai_numidiv(a->x, scalar), luai_numidiv(a->y, scalar));
    }

    Vector2* b = lua_checkvector2(L, 2);

    return pushVector2(L, luai_numidiv(a->x, b->x), luai_numidiv(a->y, b->y));
}

void open_vector2lib(lua_State *L) {
    // Vector2
    lua_newtable(L);

    setfunctionfield(L, Vector2_new, "new", true);

    lua_setglobal(L, "Vector2");

    // metatable
    luaL_newmetatable(L, "Vector2");

    settypemetafield(L, "Vector2");
    setfunctionfield(L, Vector2__tostring, "__tostring");
    setfunctionfield(L, Vector2__index, "__index");
    setfunctionfield(L, Vector2__newindex, "__newindex");
    setfunctionfield(L, Vector2__namecall, "__namecall");
    setfunctionfield(L, Vector2__add, "__add");
    setfunctionfield(L, Vector2__sub, "__sub");
    setfunctionfield(L, Vector2__mul, "__mul");
    setfunctionfield(L, Vector2__div, "__div");
    setfunctionfield(L, Vector2__idiv, "__idiv");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
