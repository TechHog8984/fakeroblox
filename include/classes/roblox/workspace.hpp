#pragma once

#include "classes/roblox/instance.hpp"

namespace fakeroblox {

class Workspace {
public:
    static std::shared_ptr<rbxInstance> instance;

};

void rbxInstance_Workspace_init(lua_State* L);

}; // namespace fakeroblox
