#include "classes/roblox/datamodel.hpp"
#include "classes/roblox/instance.hpp"

#include <algorithm>

#include "http.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

typedef struct {
    std::map<std::string, std::shared_ptr<rbxInstance>> service_map;
} DataModelUserData;

void constructor(std::shared_ptr<rbxInstance> instance) {
    instance->userdata = new DataModelUserData;
}
void destructor(std::shared_ptr<rbxInstance> instance) {
    delete static_cast<DataModelUserData*>(instance->userdata);
}

namespace rbxInstance_DataModel_methods {
    static int findService(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1, "DataModel");
        const char* name = luaL_checkstring(L, 2);

        if (std::find(rbxClass::valid_services.begin(), rbxClass::valid_services.end(), name) == rbxClass::valid_services.end())
            luaL_errorL(L, "'%s' is not a valid Service name", name);

        auto userdata = static_cast<DataModelUserData*>(instance->userdata);
        if (userdata->service_map.find(name) == userdata->service_map.end())
            lua_pushnil(L);
        else
            lua_pushinstance(L, userdata->service_map[name]);

        return 1;
    }
    static int getService(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1, "DataModel");
        const char* name = luaL_checkstring(L, 2);

        if (std::find(rbxClass::valid_services.begin(), rbxClass::valid_services.end(), name) == rbxClass::valid_services.end())
            luaL_errorL(L, "'%s' is not a valid Service name", name);

        auto userdata = static_cast<DataModelUserData*>(instance->userdata);
        if (userdata->service_map.find(name) == userdata->service_map.end())
            userdata->service_map[name] = newInstance(L, name);

        lua_pushinstance(L, userdata->service_map[name]);

        return 1;
    }

    static int httpGet(lua_State* L) {
        // FIXME: yield!
        lua_checkinstance(L, 1, "DataModel");
        const char* url = luaL_checkstring(L, 2);

        struct MemoryStruct chunk;
        int res = newGetRequest(url, &chunk);
        if (res)
            luaL_errorL(L, "failed to make HTTP request (%d)", res);

        lua_pushlstring(L, chunk.memory, chunk.size);
        return 1;

    }
};

void rbxInstance_DataModel_init(lua_State* L) {
    rbxClass::class_map["DataModel"]->constructor = constructor;
    rbxClass::class_map["DataModel"]->destructor = destructor;

    rbxClass::class_map["DataModel"]->newMethod("FindService", rbxInstance_DataModel_methods::findService);
    rbxClass::class_map["DataModel"]->newMethod("GetService", rbxInstance_DataModel_methods::getService);
    rbxClass::class_map["DataModel"]->newMethod("HttpGet", rbxInstance_DataModel_methods::httpGet);
}

};
