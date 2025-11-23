#include "classes/color3.hpp"
#include "common.hpp"

#include <algorithm>
#include <cstring>

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushColor(lua_State* L, double r, double g, double b) {
    Color* color = static_cast<Color*>(lua_newuserdata(L, sizeof(Color)));
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = 255;

    luaL_getmetatable(L, "Color3");
    lua_setmetatable(L, -2);

    return 1;
}
int pushColor(lua_State* L, Color color) {
    return pushColor(L, color.r, color.g, color.b);
}

static int Color3_new(lua_State* L) {
    const float r = luaL_optnumberrange(L, 1, 0, 1, "r");
    const float g = luaL_optnumberrange(L, 2, 0, 1, "g");
    const float b = luaL_optnumberrange(L, 3, 0, 1, "b");

    return pushColor(L, r * 255.f, g * 255.f, b * 255.f);
}
static int Color3_fromRGB(lua_State* L) {
    const float r = luaL_optnumberrange(L, 1, 0, 255, "r");
    const float g = luaL_optnumberrange(L, 2, 0, 255, "g");
    const float b = luaL_optnumberrange(L, 3, 0, 255, "b");

    return pushColor(L, r, g, b);
}
static int Color3_fromHSV(lua_State* L) {
    // FIXME: this is more similar to the behavior on Roblox (in terms of erroring)
    // reflect in other areas (like Color3.fromRGB)
    // it's not 100% tho, so still figure that out too
    const float h = std::max(0.0, luaL_optnumber(L, 1, 0)) * 360.0;
    const float s = std::max(0.0, luaL_optnumber(L, 2, 0));
    const float v = std::max(0.0, luaL_optnumber(L, 3, 0));

    return pushColor(L, ColorFromHSV(h, s, v));
}
static int Color3_fromHex(lua_State* L) {
    const char* hex = luaL_checkstring(L, 1);
    unsigned long number;
    try {
        number = std::stoul(hex, nullptr, 16);
    } catch(std::exception& e) {
        luaL_error(L, "Unable to convert characters to hex value");
    }

    unsigned char r = (number >> 16) & 0xFF;
    unsigned char g = (number >> 8) & 0xFF;
    unsigned char b = number & 0xFF;

    return pushColor(L, Color{ r, g, b, 255 });
}

bool lua_iscolor(lua_State* L, int index) {
    return luaL_isudatareal(L, index, "Color3");
}
Color* lua_checkcolor(lua_State* L, int narg) {
    void* ud = luaL_checkudatareal(L, narg, "Color3");

    return static_cast<Color*>(ud);
}

static int Color3__tostring(lua_State* L) {
    Color* color = lua_checkcolor(L, 1);

    lua_pushfstringL(L, "%i, %i, %i", color->r, color->g, color->b);
    return 1;
}
static int Color3__eq(lua_State* L) {
    Color* a = lua_checkcolor(L, 1);
    Color* b = lua_checkcolor(L, 2);

    lua_pushboolean(L, b->r == a->r && b->g == a->g && b->b == a->b);
    return 1;
}

namespace Color3_methods {
    static int lerp(lua_State* L) {
        Color* a = lua_checkcolor(L, 1);
        Color* b = lua_checkcolor(L, 2);
        double alpha = luaL_checknumberrange(L, 3, 0, 1, "alpha");

        return pushColor(L, ColorLerp(*a, *b, alpha));
    }
    static int toHSV(lua_State* L) {
        Color* color = lua_checkcolor(L, 1);

        auto hsv = ColorToHSV(*color);

        lua_pushnumber(L, hsv.x);
        lua_pushnumber(L, hsv.y);
        lua_pushnumber(L, hsv.z);

        return 3;
    }
    static int toHex(lua_State* L) {
        Color* color = lua_checkcolor(L, 1);

        unsigned int hex = (color->r << 16) | (color->g << 8) | color->b;

        lua_pushfstring(L, "%x", hex);

        return 1;
    }
};

lua_CFunction getColor3Method(Color* entry, const char* key) {
    if (strequal(key, "Lerp"))
        return Color3_methods::lerp;
    else if (strequal(key, "ToHSV"))
        return Color3_methods::toHSV;
    else if (strequal(key, "ToHex"))
        return Color3_methods::toHex;

    return nullptr;
}

static int Color3__index(lua_State* L) {
    Color* color = lua_checkcolor(L, 1);
    const char* key = luaL_checkstring(L, 2);

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

    lua_CFunction func = getColor3Method(color, key);
    if (func)
        return pushFunctionFromLookup(L, func);

    luaL_error(L, "%s is not a valid member of Color3", key);
}
static int Color3__newindex(lua_State* L) {
    lua_checkcolor(L, 1);
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
    Color* color = lua_checkcolor(L, 1);
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");

    lua_CFunction func = getColor3Method(color, namecall);
    if (!func)
        luaL_error(L, "%s is not a valid member of Color3", namecall);

    return func(L);
}

void open_color3lib(lua_State *L) {
    // Color3
    lua_newtable(L);

    // FIXME: test Color3_from*; Hex most likely does not support the RBG shorthand
    setfunctionfield(L, Color3_new, "new", true);
    setfunctionfield(L, Color3_fromRGB, "fromRGB", true);
    setfunctionfield(L, Color3_fromHSV, "fromHSV", true);
    setfunctionfield(L, Color3_fromHex, "fromHex", true);
    setfunctionfield(L, Color3_methods::toHSV, "toHSV", true);

    lua_setglobal(L, "Color3");

    // metatable
    luaL_newmetatable(L, "Color3");

    settypemetafield(L, "Color3");
    setfunctionfield(L, Color3__tostring, "__tostring");
    setfunctionfield(L, Color3__index, "__index");
    setfunctionfield(L, Color3__newindex, "__newindex");
    setfunctionfield(L, Color3__namecall, "__namecall");
    setfunctionfield(L, Color3__eq, "__eq");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
