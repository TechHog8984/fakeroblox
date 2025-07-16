#pragma once

#include "classes/roblox/instance.hpp"

#include "lua.h"

namespace fakeroblox {

void createService(lua_State* L, std::shared_ptr<rbxInstance> serviceprovider, const char* service);
std::shared_ptr<rbxInstance> getService(lua_State* L, std::shared_ptr<rbxInstance> serviceprovider, const char* service);

void rbxInstance_ServiceProvider_init(lua_State* L);

}; // namespace fakeroblox
