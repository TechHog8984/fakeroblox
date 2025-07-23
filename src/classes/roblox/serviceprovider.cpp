#include "classes/roblox/serviceprovider.hpp"
#include "classes/roblox/instance.hpp"

#include <algorithm>

#include "lualib.h"

namespace fakeroblox {

std::map<std::string, std::shared_ptr<rbxInstance>> ServiceProvider::service_map;

void ServiceProvider::registerService(const char* service_name) {
    if (std::find(rbxClass::valid_services.begin(), rbxClass::valid_services.end(), service_name) != rbxClass::valid_services.end())
        return;

    rbxClass::valid_services.push_back(service_name);
}

void ServiceProvider::createService(lua_State* L, std::shared_ptr<rbxInstance> service_provider, const char* service_name) {
    auto service = newInstance(L, service_name);
    ServiceProvider::service_map[service_name] = service;

    setInstanceParent(L, service, service_provider);
    service->parent_locked = true;
}
std::shared_ptr<rbxInstance> ServiceProvider::getService(lua_State* L, std::shared_ptr<rbxInstance> service_provider, const char* service) {
    if (ServiceProvider::service_map.find(service) == ServiceProvider::service_map.end())
        createService(L, service_provider, service);

    return ServiceProvider::service_map[service];
}

namespace rbxInstance_ServiceProvider_methods {
    static int findService(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1, "ServiceProvider");
        const char* service = luaL_checkstring(L, 2);

        if (std::find(rbxClass::valid_services.begin(), rbxClass::valid_services.end(), service) == rbxClass::valid_services.end())
            luaL_errorL(L, "'%s' is not a valid Service name", service);

        if (ServiceProvider::service_map.find(service) == ServiceProvider::service_map.end())
            lua_pushnil(L);
        else
            lua_pushinstance(L, ServiceProvider::service_map[service]);

        return 1;
    }
    static int getService(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1, "ServiceProvider");
        const char* service = luaL_checkstring(L, 2);

        if (std::find(rbxClass::valid_services.begin(), rbxClass::valid_services.end(), service) == rbxClass::valid_services.end())
            luaL_errorL(L, "'%s' is not a valid Service name", service);

        lua_pushinstance(L, ServiceProvider::getService(L, instance, service));

        return 1;
    }
};

void rbxInstance_ServiceProvider_init(lua_State *L) {
    rbxClass::class_map["ServiceProvider"]->methods.at("FindService").func = rbxInstance_ServiceProvider_methods::findService;
    rbxClass::class_map["ServiceProvider"]->methods.at("GetService").func = rbxInstance_ServiceProvider_methods::getService;
  
}

}; // namespace fakeroblox
