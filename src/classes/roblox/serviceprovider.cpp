#include "classes/roblox/serviceprovider.hpp"
#include "classes/roblox/instance.hpp"

#include <algorithm>

#include "lualib.h"

namespace fakeroblox {

typedef struct {
    std::map<std::string, std::shared_ptr<rbxInstance>> service_map;
} ServiceProviderUserData;

void constructor(std::shared_ptr<rbxInstance> instance) {
    instance->userdata = new ServiceProviderUserData;
}
void destructor(std::shared_ptr<rbxInstance> instance) {
    delete static_cast<ServiceProviderUserData*>(instance->userdata);
}

void createService(lua_State* L, std::shared_ptr<rbxInstance> service_provider, const char* service) {
    auto userdata = static_cast<ServiceProviderUserData*>(service_provider->userdata);
    userdata->service_map[service] = newInstance(L, service);
}
std::shared_ptr<rbxInstance> getService(lua_State* L, std::shared_ptr<rbxInstance> service_provider, const char* service) {
    auto userdata = static_cast<ServiceProviderUserData*>(service_provider->userdata);
    if (userdata->service_map.find(service) == userdata->service_map.end())
        createService(L, service_provider, service);

    return userdata->service_map[service];
}

namespace rbxInstance_ServiceProvider_methods {
    static int findService(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1, "ServiceProvider");
        const char* service = luaL_checkstring(L, 2);

        if (std::find(rbxClass::valid_services.begin(), rbxClass::valid_services.end(), service) == rbxClass::valid_services.end())
            luaL_errorL(L, "'%s' is not a valid Service name", service);

        auto userdata = static_cast<ServiceProviderUserData*>(instance->userdata);
        if (userdata->service_map.find(service) == userdata->service_map.end())
            lua_pushnil(L);
        else
            lua_pushinstance(L, userdata->service_map[service]);

        return 1;
    }
    static int getService(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1, "ServiceProvider");
        const char* service = luaL_checkstring(L, 2);

        if (std::find(rbxClass::valid_services.begin(), rbxClass::valid_services.end(), service) == rbxClass::valid_services.end())
            luaL_errorL(L, "'%s' is not a valid Service name", service);

        lua_pushinstance(L, getService(L, instance, service));

        return 1;
    }
};

void rbxInstance_ServiceProvider_init(lua_State *L) {
    rbxClass::class_map["ServiceProvider"]->constructor = constructor;
    rbxClass::class_map["ServiceProvider"]->destructor = destructor;

    rbxClass::class_map["ServiceProvider"]->methods.at("FindService").func = rbxInstance_ServiceProvider_methods::findService;
    rbxClass::class_map["ServiceProvider"]->methods.at("GetService").func = rbxInstance_ServiceProvider_methods::getService;
  
}

}; // namespace fakeroblox
