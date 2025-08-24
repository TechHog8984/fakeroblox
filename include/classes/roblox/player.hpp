#pragma once

#include "classes/roblox/instance.hpp"
#include "lua.h"

namespace fakeroblox {

class rbxPlayer {
public:
    static std::shared_ptr<rbxInstance> localplayer;
    static std::shared_ptr<rbxInstance> localmouse;
};

void rbxInstance_Player_init(lua_State* L, std::shared_ptr<rbxInstance> players_service);

}; // namespace fakeroblox
