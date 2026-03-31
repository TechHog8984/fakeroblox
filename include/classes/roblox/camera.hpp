#pragma once

#include "classes/roblox/instance.hpp"
#include "lua.h"

namespace frostbyte {

class rbxCamera {
public:
    static Vector2 screen_size;
};

void rbxInstance_Camera_updateViewport(lua_State* L);
void rbxInstance_Camera_init(lua_State* L, std::shared_ptr<rbxInstance> workspace);

}; // namespace frostbyte
