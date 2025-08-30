#pragma once

#include "classes/roblox/instance.hpp"
#include "lua.h"

namespace fakeroblox {

void ImGuiService_init(lua_State *L, std::shared_ptr<rbxInstance> datamodel);

void ImGuiService_render(lua_State* L);

}; // namespace fakeroblox
