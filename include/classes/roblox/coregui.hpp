#pragma once

#include "classes/roblox/instance.hpp"

namespace fakeroblox {

class CoreGui {
public:
    static std::shared_ptr<rbxInstance> notification_frame;
};

void rbxInstance_CoreGui_setup(lua_State* L, std::shared_ptr<rbxInstance> instance);

}; // namespace fakeroblox
