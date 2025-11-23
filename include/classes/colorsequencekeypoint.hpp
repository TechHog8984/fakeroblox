#pragma once

#include "lua.h"
#include <raylib.h>

namespace fakeroblox {

struct ColorSequenceKeypoint {
    float time;
    Color value;
};

int pushColorSequenceKeypoint(lua_State* L, float time, Color value);
int pushColorSequenceKeypoint(lua_State* L, ColorSequenceKeypoint colorsequencekeypoint);

bool lua_iscolorsequencekeypoint(lua_State* L, int index);
ColorSequenceKeypoint* lua_checkcolorsequencekeypoint(lua_State* L, int index);

void open_colorsequencekeypointlib(lua_State* L);

}; // namespace fakeroblox
