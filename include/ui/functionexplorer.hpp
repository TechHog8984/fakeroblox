#pragma once

#include "classes/roblox/instance.hpp"
#include "lobject.h"
#include "lua.h"

namespace frostbyte {

void UI_FunctionExplorer_init(lua_State *L, std::shared_ptr<rbxInstance> datamodel);

void UI_FunctionExplorer_render(lua_State* L);

void UI_FunctionExplorer_setSelectedFunction(lua_State* L, Closure* cl);

}; // namespace frostbyte
