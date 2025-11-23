#pragma once

#include "lua.h"

namespace fakeroblox {

struct NumberSequenceKeypoint {
    float envelope;
    float time;
    float value;
};

int pushNumberSequenceKeypoint(lua_State* L, float envelope, float time, float value);
int pushNumberSequenceKeypoint(lua_State* L, NumberSequenceKeypoint numbersequencekeypoint);

bool lua_isnumbersequencekeypoint(lua_State* L, int index);
NumberSequenceKeypoint* lua_checknumbersequencekeypoint(lua_State* L, int index);

void open_numbersequencekeypointlib(lua_State* L);

}; // namespace fakeroblox
