#pragma once

#include "classes/roblox/instance.hpp"

#include "lua.h"

namespace fakeroblox {

class ServiceProvider {
public:
    static std::map<std::string, std::shared_ptr<rbxInstance>> service_map;

    static void registerService(const char* service_name);
    static void createService(lua_State* L, std::shared_ptr<rbxInstance> serviceprovider, const char* service);
    static std::shared_ptr<rbxInstance> getService(lua_State* L, std::shared_ptr<rbxInstance> serviceprovider, const char* service);
};


void rbxInstance_ServiceProvider_init(lua_State* L);

}; // namespace fakeroblox
