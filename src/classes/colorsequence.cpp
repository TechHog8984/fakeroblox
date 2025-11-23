#include "classes/colorsequence.hpp"
#include "classes/color3.hpp"
#include "common.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushColorSequence(lua_State* L, const std::vector<ColorSequenceKeypoint>& keypoint_list) {
    ColorSequence* colorsequence = static_cast<ColorSequence*>(lua_newuserdatatagged(L, sizeof(ColorSequence), LUA_TAG_COLOR_SEQUENCE));
    new(colorsequence) ColorSequence;

    colorsequence->keypoint_list = keypoint_list;

    luaL_getmetatable(L, "ColorSequence");
    lua_setmetatable(L, -2);

    return 1;
}
int pushColorSequence(lua_State* L, ColorSequence colorsequence) {
    return pushColorSequence(L, colorsequence.keypoint_list);
}

static int ColorSequence_new(lua_State* L) {
    if (lua_type(L, 1) == LUA_TTABLE) {
        const int len = lua_objlen(L, 1);
        if (len < 2)
            luaL_error(L, "ColorSequence: requires at least 2 keypoints");

        std::vector<ColorSequenceKeypoint> keypoint_list;
        keypoint_list.resize(len);

        for (int i = 0; i < len; i++) {
            lua_rawgeti(L, 1, i + 1);
            auto keypoint = lua_checkcolorsequencekeypoint(L, -1);
            keypoint_list[i] = *keypoint;
            lua_pop(L, 1);
        }

        return pushColorSequence(L, keypoint_list);
    } else if (lua_iscolor(L, 1)) {
        std::vector<ColorSequenceKeypoint> keypoint_list;
        keypoint_list.resize(2);

        const Color* value1 = lua_checkcolor(L, 1);
        keypoint_list[0].time = 0;
        keypoint_list[0].value = *value1;
        keypoint_list[1].time = 1;
        keypoint_list[1].value = *value1;

        if (lua_iscolor(L, 2))
            keypoint_list[1].value = *lua_checkcolor(L, 2);

        return pushColorSequence(L, keypoint_list);
    }

    luaL_error(L, " ColorSequence.new(): table of ColorSequenceKeypoints expected.");
}

bool lua_iscolorsequence(lua_State* L, int index) {
    return luaL_isudatareal(L, index, "ColorSequence");
}
ColorSequence* lua_checkcolorsequence(lua_State* L, int index) {
    void* ud = luaL_checkudatareal(L, index, "ColorSequence");

    return static_cast<ColorSequence*>(ud);
}

static int ColorSequence__tostring(lua_State* L) {
    ColorSequence* colorsequence = lua_checkcolorsequence(L, 1);

    luaL_Strbuf buf;
    luaL_buffinit(L, &buf);

    char numbuf[1000];
    const size_t size = colorsequence->keypoint_list.size();
    for (size_t i = 0; i < size; i++) {
        auto& keypoint = colorsequence->keypoint_list[i];
        snprintf(numbuf, 1000, "%.*f %.*f %.*f %.*f", decimalFmt(keypoint.time), decimalFmt(keypoint.value.r * 255.f), decimalFmt(keypoint.value.g * 255.f), decimalFmt(keypoint.value.b * 255.f));
        luaL_addstring(&buf, numbuf);
        if (i < size - 1)
            luaL_addchar(&buf, ' ');
    }

    luaL_pushresult(&buf);
    return 1;
}

static int ColorSequence__index(lua_State* L) {
    ColorSequence* colorsequence = lua_checkcolorsequence(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Keypoints")) {
        lua_createtable(L, colorsequence->keypoint_list.size(), 0);

        for (size_t i = 0; i < colorsequence->keypoint_list.size(); i++) {
            pushColorSequenceKeypoint(L, colorsequence->keypoint_list[i]);
            lua_rawseti(L, -2, i + 1);
        }
    } else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of ColorSequence", key);
}
static int ColorSequence__newindex(lua_State* L) {
    lua_checkcolorsequence(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Keypoints"))
        luaL_error(L, "%s member of ColorSequence is read-only and cannot be assigned to", key);

    luaL_error(L, "%s is not a valid member of ColorSequence", key);

    return 0;
}

void open_colorsequencelib(lua_State *L) {
    // ColorSequence
    lua_newtable(L);

    setfunctionfield(L, ColorSequence_new, "new", true);

    lua_setglobal(L, "ColorSequence");

    // metatable
    luaL_newmetatable(L, "ColorSequence");

    settypemetafield(L, "ColorSequence");
    setfunctionfield(L, ColorSequence__tostring, "__tostring");
    setfunctionfield(L, ColorSequence__index, "__index");
    setfunctionfield(L, ColorSequence__newindex, "__newindex");

    lua_pop(L, 1);

    lua_setuserdatadtor(L, LUA_TAG_COLOR_SEQUENCE, [](lua_State* L, void* ud) {
        ColorSequence* color_sequence = static_cast<ColorSequence*>(ud);
        color_sequence->~ColorSequence();
    });
}

}; // namespace fakeroblox
