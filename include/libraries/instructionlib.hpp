#pragma once

#include "lua.h"

namespace fakeroblox {

extern bool enable_stephook;

void open_instructionlib(lua_State* L);

void onEnableStephookChange(lua_State* L);

}; // namespace fakeroblox
