#pragma once

#include "classes/roblox/instance.hpp"

#include "lua.h"

#include <initializer_list>

namespace fakeroblox {

void rbxInstance_BasePlayerGui_process(lua_State* L);
void rbxInstance_BasePlayerGui_init(lua_State* L, std::initializer_list<std::shared_ptr<rbxInstance>> initial_gui_storage_list);

};
