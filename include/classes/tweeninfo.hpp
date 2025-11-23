#pragma once

#include "classes/roblox/datatypes/enum.hpp"

namespace fakeroblox {

struct TweenInfo {
    EnumItemWrapper easing_direction{ .name = "Out", .enum_name = "EasingDirection" };
    double time = 1;
    double delay_time = 0;
    int repeat_count = 0;
    EnumItemWrapper easing_style{ .name = "Quad", .enum_name = "EasingStyle" };
    bool reverses = false;
};

int pushTweenInfo(lua_State* L, TweenInfo tweeninfo);
TweenInfo* lua_checktweeninfo(lua_State* L, int narg);

void open_tweeninfolib(lua_State* L);

}; // namespace fakeroblox
