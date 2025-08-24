#pragma once

#include "classes/roblox/instance.hpp"

namespace fakeroblox {

class Players {
public:
};

void rbxInstance_Players_init(lua_State* L, std::shared_ptr<rbxInstance> datamodel);

}; // namespace fakeroblox
