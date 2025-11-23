#pragma once

#include "lua.h"

namespace fakeroblox {

struct NumberRange {
    float min;
    float max;
};

int pushNumberRange(lua_State* L, float min, float max);
int pushNumberRange(lua_State* L, NumberRange numberrange);

bool lua_isnumberrange(lua_State* L, int index);
NumberRange* lua_checknumberrange(lua_State* L, int index);

void open_numberrangelib(lua_State* L);

}; // namespace fakeroblox
