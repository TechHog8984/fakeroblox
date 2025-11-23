#include "classes/tweeninfo.hpp"
#include "classes/roblox/datatypes/enum.hpp"
#include "common.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

int pushTweenInfo(lua_State* L, TweenInfo tweeninfo) {
    TweenInfo* new_tweeninfo = static_cast<TweenInfo*>(lua_newuserdatatagged(L, sizeof(TweenInfo), LUA_TAG_TWEENINFO));
    new(new_tweeninfo) TweenInfo;

    *new_tweeninfo = tweeninfo;

    luaL_getmetatable(L, "TweenInfo");
    lua_setmetatable(L, -2);

    return 1;
}

static int TweenInfo_new(lua_State* L) {
    TweenInfo tweeninfo;

    // FIXME: passing nil is not valid in Roblox, but I really like being able to pass nil; this is where I start to reconsider "1:1 error handling"...
    if (!lua_isnoneornil(L, 1))
        tweeninfo.time = luaL_checknumberrange(L, 1, 0, static_cast<uint32_t>(-1), "time");
    if (!lua_isnoneornil(L, 2))
        tweeninfo.easing_style.name = lua_checkenumitem(L, 2, "EasingStyle")->name;
    if (!lua_isnoneornil(L, 3))
        tweeninfo.easing_direction.name = lua_checkenumitem(L, 3, "EasingDirection")->name;
    if (!lua_isnoneornil(L, 4))
        tweeninfo.repeat_count = luaL_checknumberrange(L, 4, 0, static_cast<uint32_t>(-1), "repeatCount");
    if (!lua_isnoneornil(L, 5))
        tweeninfo.reverses = luaL_checkboolean(L, 5);
    if (!lua_isnoneornil(L, 6))
        tweeninfo.delay_time = luaL_checknumberrange(L, 6, 0, static_cast<uint32_t>(-1), "delayTime");

    return pushTweenInfo(L, tweeninfo);
}

TweenInfo* lua_checktweeninfo(lua_State* L, int narg) {
    void* ud = luaL_checkudatareal(L, narg, "TweenInfo");

    return static_cast<TweenInfo*>(ud);
}

static int TweenInfo__tostring(lua_State* L) {
    TweenInfo* tweeninfo = lua_checktweeninfo(L, 1);

    lua_pushfstringL(L, "Time:%.f DelayTime:%.f RepeatCount:%d Reverses:%s EasingDirection:%s EasingStyle:%s", tweeninfo->time, tweeninfo->delay_time, tweeninfo->repeat_count, tweeninfo->reverses ? "True" : "False", tweeninfo->easing_direction.name.c_str(), tweeninfo->easing_style.name.c_str());
    return 1;
}

static int TweenInfo__index(lua_State* L) {
    TweenInfo* tweeninfo = lua_checktweeninfo(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "EasingDirection"))
        pushEnumItem(L, tweeninfo->easing_direction);
    else if (strequal(key, "Time"))
        lua_pushnumber(L, tweeninfo->time);
    else if (strequal(key, "DelayTime"))
        lua_pushnumber(L, tweeninfo->delay_time);
    else if (strequal(key, "RepeatCount"))
        lua_pushnumber(L, tweeninfo->repeat_count);
    else if (strequal(key, "EasingStyle"))
        pushEnumItem(L, tweeninfo->easing_style);
    else if (strequal(key, "Reverses"))
        lua_pushboolean(L, tweeninfo->reverses);
    else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of TweenInfo", key);
}
#undef MAGNITUDE

static int TweenInfo__newindex(lua_State* L) {
    lua_checktweeninfo(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "EasingDirection")
        || strequal(key, "Time")
        || strequal(key, "DelayTime")
        || strequal(key, "RepeatCount")
        || strequal(key, "EasingStyle")
        || strequal(key, "Reverses")
    )
        luaL_error(L, "%s member of TweenInfo is read-only and cannot be assigned to", key);
    else
        goto INVALID;

    INVALID:
    luaL_error(L, "%s is not a valid member of TweenInfo", key);

    return 0;
}

void open_tweeninfolib(lua_State *L) {
    // TweenInfo
    lua_newtable(L);

    setfunctionfield(L, TweenInfo_new, "new");

    lua_setglobal(L, "TweenInfo");

    // metatable
    luaL_newmetatable(L, "TweenInfo");

    settypemetafield(L, "TweenInfo");
    setfunctionfield(L, TweenInfo__tostring, "__tostring");
    setfunctionfield(L, TweenInfo__index, "__index");
    setfunctionfield(L, TweenInfo__newindex, "__newindex");

    lua_pop(L, 1);

    lua_setuserdatadtor(L, LUA_TAG_TWEENINFO, [](lua_State* L, void* ud) {
        TweenInfo* tweeninfo_ptr = static_cast<TweenInfo*>(ud);
        tweeninfo_ptr->~TweenInfo();
    });
}

}; // namespace fakeroblox
