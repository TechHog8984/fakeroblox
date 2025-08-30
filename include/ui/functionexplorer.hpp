#pragma once

#include "classes/roblox/instance.hpp"
#include "lua.h"

namespace fakeroblox {

void UI_FunctionExplorer_init(lua_State *L, std::shared_ptr<rbxInstance> datamodel);

void UI_FunctionExplorer_render(lua_State* L);

}; // namespace fakeroblox
