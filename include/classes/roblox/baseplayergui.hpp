#pragma once

#include "classes/roblox/instance.hpp"

#include "lua.h"

#include <initializer_list>

namespace fakeroblox {

std::weak_ptr<rbxInstance> getClickableGuiObject();
std::vector<std::weak_ptr<rbxInstance>> getGuiObjectsHovered();

void rbxInstance_BasePlayerGui_process(lua_State* L);
void rbxInstance_BasePlayerGui_init(lua_State* L, std::initializer_list<std::shared_ptr<rbxInstance>> initial_gui_storage_list);

};
