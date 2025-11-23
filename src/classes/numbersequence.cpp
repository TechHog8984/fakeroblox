#include "classes/numbersequence.hpp"
#include "classes/numbersequencekeypoint.hpp"
#include "common.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushNumberSequence(lua_State* L, const std::vector<NumberSequenceKeypoint>& keypoint_list) {
    NumberSequence* numbersequence = static_cast<NumberSequence*>(lua_newuserdatatagged(L, sizeof(NumberSequence), LUA_TAG_NUMBER_SEQUENCE));
    new(numbersequence) NumberSequence;

    numbersequence->keypoint_list = keypoint_list;

    luaL_getmetatable(L, "NumberSequence");
    lua_setmetatable(L, -2);

    return 1;
}
int pushNumberSequence(lua_State* L, NumberSequence numbersequence) {
    return pushNumberSequence(L, numbersequence.keypoint_list);
}

static int NumberSequence_new(lua_State* L) {
    const int t = lua_type(L, 1);
    if (t == LUA_TTABLE) {
        const int len = lua_objlen(L, 1);
        if (len < 2)
            luaL_error(L, "NumberSequence: requires at least 2 keypoints");

        std::vector<NumberSequenceKeypoint> keypoint_list;
        keypoint_list.resize(len);

        for (int i = 0; i < len; i++) {
            lua_rawgeti(L, 1, i + 1);
            auto keypoint = lua_checknumbersequencekeypoint(L, -1);
            keypoint_list[i] = *keypoint;
            lua_pop(L, 1);
        }

        return pushNumberSequence(L, keypoint_list);
    } else if (t == LUA_TNUMBER) {
        std::vector<NumberSequenceKeypoint> keypoint_list;
        keypoint_list.resize(2);

        const float value1 = lua_tonumber(L, 1);
        keypoint_list[0].time = 0;
        keypoint_list[0].value = value1;
        keypoint_list[1].time = 1;
        keypoint_list[1].value = value1;

        if (lua_type(L, 2) == LUA_TNUMBER)
            keypoint_list[1].value = lua_tonumber(L, 2);

        return pushNumberSequence(L, keypoint_list);
    }

    luaL_error(L, " NumberSequence.new(): table of NumberSequenceKeypoints expected.");
}

bool lua_isnumbersequence(lua_State* L, int index) {
    return luaL_isudatareal(L, index, "NumberSequence");
}
NumberSequence* lua_checknumbersequence(lua_State* L, int index) {
    void* ud = luaL_checkudatareal(L, index, "NumberSequence");

    return static_cast<NumberSequence*>(ud);
}

static int NumberSequence__tostring(lua_State* L) {
    NumberSequence* numbersequence = lua_checknumbersequence(L, 1);

    luaL_Strbuf buf;
    luaL_buffinit(L, &buf);

    char numbuf[1000];
    const size_t size = numbersequence->keypoint_list.size();
    for (size_t i = 0; i < size; i++) {
        auto& keypoint = numbersequence->keypoint_list[i];
        snprintf(numbuf, 1000, "%.*f %.*f %.*f", decimalFmt(keypoint.time), decimalFmt(keypoint.value), decimalFmt(keypoint.envelope));
        luaL_addstring(&buf, numbuf);
        if (i < size - 1)
            luaL_addchar(&buf, ' ');
    }

    luaL_pushresult(&buf);
    return 1;
}

static int NumberSequence__index(lua_State* L) {
    NumberSequence* numbersequence = lua_checknumbersequence(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Keypoints")) {
        lua_createtable(L, numbersequence->keypoint_list.size(), 0);

        for (size_t i = 0; i < numbersequence->keypoint_list.size(); i++) {
            pushNumberSequenceKeypoint(L, numbersequence->keypoint_list[i]);
            lua_rawseti(L, -2, i + 1);
        }
    } else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of NumberSequence", key);
}
static int NumberSequence__newindex(lua_State* L) {
    lua_checknumbersequence(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Keypoints"))
        luaL_error(L, "%s member of NumberSequence is read-only and cannot be assigned to", key);

    luaL_error(L, "%s is not a valid member of NumberSequence", key);

    return 0;
}

void open_numbersequencelib(lua_State *L) {
    // NumberSequence
    lua_newtable(L);

    setfunctionfield(L, NumberSequence_new, "new", true);

    lua_setglobal(L, "NumberSequence");

    // metatable
    luaL_newmetatable(L, "NumberSequence");

    settypemetafield(L, "NumberSequence");
    setfunctionfield(L, NumberSequence__tostring, "__tostring");
    setfunctionfield(L, NumberSequence__index, "__index");
    setfunctionfield(L, NumberSequence__newindex, "__newindex");

    lua_pop(L, 1);

    lua_setuserdatadtor(L, LUA_TAG_NUMBER_SEQUENCE, [](lua_State* L, void* ud) {
        NumberSequence* number_sequence = static_cast<NumberSequence*>(ud);
        number_sequence->~NumberSequence();
    });
}

}; // namespace fakeroblox
