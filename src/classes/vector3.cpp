#include "classes/vector3.hpp"
#include "common.hpp"

#include <cstring>

#include "lnumutils.h"
#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushVector3(lua_State* L, float x, float y, float z) {
    Vector3* vector3 = static_cast<Vector3*>(lua_newuserdata(L, sizeof(Vector3)));
    vector3->x = x;
    vector3->y = y;
    vector3->z = z;

    luaL_getmetatable(L, "Vector3");
    lua_setmetatable(L, -2);

    return 1;
}
int pushVector3(lua_State* L, Vector3 vector3) {
    return pushVector3(L, vector3.x, vector3.y, vector3.z);
}

static int Vector3_new(lua_State* L) {
    float x = luaL_optnumber(L, 1, 0);
    float y = luaL_optnumber(L, 2, 0);
    float z = luaL_optnumber(L, 3, 0);

    return pushVector3(L, x, y, z);
}

Vector3* lua_checkvector3(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, "Vector3");

    return static_cast<Vector3*>(ud);
}

static int Vector3__tostring(lua_State* L) {
    Vector3* vector3 = lua_checkvector3(L, 1);

    lua_pushfstringL(L, "%.f, %.f, %.f", vector3->x, vector3->y, vector3->z);
    return 1;
}

#define MAGNITUDE(vector3) (sqrt(vector3->x * vector3->x + vector3->y * vector3->y + vector3->z * vector3->z))
static int Vector3__index(lua_State* L) {
    Vector3* vector3 = lua_checkvector3(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // FIXME: methods
    if (strlen(key) == 1) {
        switch (*key) {
            case 'x':
            case 'X':
                lua_pushnumber(L, vector3->x);
                break;
            case 'y':
            case 'Y':
                lua_pushnumber(L, vector3->y);
                break;
            case 'z':
            case 'Z':
                lua_pushnumber(L, vector3->z);
                break;
            default:
                goto INVALID;
        }
    } else if (strequal(key, "Magnitude")) {
        lua_pushnumber(L, MAGNITUDE(vector3));
    } else if (strequal(key, "Unit") || strequal(key, "unit")) {
        float magnitude = MAGNITUDE(vector3);
        pushVector3(L, magnitude == 0 ? 0 : vector3->x / magnitude, magnitude == 0 ? 0 : vector3->y / magnitude, magnitude == 0 ? 0 : vector3->z / magnitude);
    } else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of Vector3", key);
}
#undef MAGNITUDE

static int Vector3__newindex(lua_State* L) {
    // Vector3* vector3 = lua_checkvector3(L, 1);
    luaL_checkudata(L, 1, "Vector3");
    const char* key = luaL_checkstring(L, 2);

    if (strlen(key) == 1) {
        switch (*key) {
            case 'x':
            case 'X':
            case 'y':
            case 'Y':
            case 'z':
            case 'Z':
                luaL_error(L, "%s member of Vector3 is read-only and cannot be assigned to", key);
            default:
                goto INVALID;
        }
    }

    INVALID:
    luaL_error(L, "%s is not a valid member of Vector3", key);

    return 0;
}
static int Vector3__namecall(lua_State* L) {
    // Vector3* vector3 = lua_checkvector3(L, 1);
    luaL_checkudata(L, 1, "Vector3");
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");
    // std::string method_name = namecall;

    luaL_error(L, "INTERNAL ERROR: TODO Vector3 methods");
    return 0;
}

static int Vector3__add(lua_State* L) {
    Vector3* a = lua_checkvector3(L, 1);
    Vector3* b = lua_checkvector3(L, 2);

    return pushVector3(L, a->x + b->x, a->y + b->y, a->z + b->z);
}
static int Vector3__sub(lua_State* L) {
    Vector3* a = lua_checkvector3(L, 1);
    Vector3* b = lua_checkvector3(L, 2);

    return pushVector3(L, a->x - b->x, a->y - b->y, a->z + b->z);
}
static int Vector3__mul(lua_State* L) {
    Vector3* a = lua_checkvector3(L, 1);
    luaL_argcheck(L, lua_isnumber(L, 2) || lua_isuserdata(L, 2), 2, "expected number or userdata for Vector3 mul");
    if (lua_isnumber(L, 2)) {
        float scalar = luaL_checknumber(L, 2);

        return pushVector3(L, a->x * scalar, a->y * scalar, a->z * scalar);
    }

    Vector3* b = lua_checkvector3(L, 2);
    return pushVector3(L, a->x * b->x, a->y * b->y, a->z * b->z);
}
static int Vector3__div(lua_State* L) {
    Vector3* a = lua_checkvector3(L, 1);
    luaL_argcheck(L, lua_isnumber(L, 2) || lua_isuserdata(L, 2), 2, "expected number or userdata for Vector3 div");
    if (lua_isnumber(L, 2)) {
        float scalar = luaL_checknumber(L, 2);
        return pushVector3(L, a->x / scalar, a->y / scalar, a->z / scalar);
    }

    Vector3* b = lua_checkvector3(L, 2);
    return pushVector3(L, a->x / b->x, a->y / b->y, a->z / b->z);
}
static int Vector3__idiv(lua_State* L) {
    Vector3* a = lua_checkvector3(L, 1);
    luaL_argcheck(L, lua_isnumber(L, 2) || lua_isuserdata(L, 2), 2, "expected number or userdata for Vector3 idiv");
    if (lua_isnumber(L, 2)) {
        float scalar = luaL_checknumber(L, 2);
        return pushVector3(L, luai_numidiv(a->x, scalar), luai_numidiv(a->y, scalar), luai_numidiv(a->z, scalar));
    }

    Vector3* b = lua_checkvector3(L, 2);
    return pushVector3(L, luai_numidiv(a->x, b->x), luai_numidiv(a->y, b->y), luai_numidiv(a->z, b->z));
}

void open_vector3lib(lua_State *L) {
    // Vector3
    lua_newtable(L);

    setfunctionfield(L, Vector3_new, "new", true);

    lua_setglobal(L, "Vector3");

    // metatable
    luaL_newmetatable(L, "Vector3");

    settypemetafield(L, "Vector3");
    setfunctionfield(L, Vector3__tostring, "__tostring");
    setfunctionfield(L, Vector3__index, "__index");
    setfunctionfield(L, Vector3__newindex, "__newindex");
    setfunctionfield(L, Vector3__namecall, "__namecall");
    // TODO: Vector3 operations
    setfunctionfield(L, Vector3__add, "__add");
    setfunctionfield(L, Vector3__sub, "__sub");
    setfunctionfield(L, Vector3__mul, "__mul");
    setfunctionfield(L, Vector3__div, "__div");
    setfunctionfield(L, Vector3__idiv, "__idiv");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
