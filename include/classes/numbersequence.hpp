#pragma once

#include "classes/numbersequencekeypoint.hpp"
#include "lua.h"
#include <vector>

namespace fakeroblox {

struct NumberSequence {
    std::vector<NumberSequenceKeypoint> keypoint_list;
};

int pushNumberSequence(lua_State* L, const std::vector<NumberSequenceKeypoint>& keypoint_list);
int pushNumberSequence(lua_State* L, NumberSequence numbersequence);

bool lua_isnumbersequence(lua_State* L, int index);
NumberSequence* lua_checknumbersequence(lua_State* L, int index);

void open_numbersequencelib(lua_State* L);

}; // namespace fakeroblox
