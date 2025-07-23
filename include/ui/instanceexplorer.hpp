#pragma once

#include "classes/roblox/instance.hpp"

#include "lua.h"

namespace fakeroblox {

void UI_InstanceExplorer_init(std::shared_ptr<rbxInstance> datamodel);

void UI_InstanceExplorer_render(lua_State* L);

}; // namespace fakeroblox
