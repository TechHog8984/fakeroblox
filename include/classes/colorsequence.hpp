#pragma once

#include "classes/colorsequencekeypoint.hpp"
#include "lua.h"
#include <vector>

namespace frostbyte {

struct ColorSequence {
    std::vector<ColorSequenceKeypoint> keypoint_list;
};

int pushColorSequence(lua_State* L, const std::vector<ColorSequenceKeypoint>& keypoint_list);
int pushColorSequence(lua_State* L, ColorSequence colorsequence);

bool lua_iscolorsequence(lua_State* L, int index);
ColorSequence* lua_checkcolorsequence(lua_State* L, int index);

void open_colorsequencelib(lua_State* L);

}; // namespace frostbyte
