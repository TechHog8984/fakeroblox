#include "classes/roblox/httpservice.hpp"
#include "classes/roblox/instance.hpp"

#include "uuid_v4.h"

#include "lualib.h"

namespace fakeroblox {

UUIDv4::UUIDGenerator<std::mt19937_64> uuid_generator;

namespace rbxInstance_HttpService_methods {
    static int generateGUID(lua_State* L) {
        lua_checkinstance(L, 1, "HttpService");

        const bool wrap = luaL_optboolean(L, 2, true);

        UUIDv4::UUID uuid = uuid_generator.getUUID();
        std::string str = uuid.str();

        if (wrap) {
            str.insert(str.begin(), '{');
            str.push_back('}');
        }

        lua_pushstring(L, str.c_str());
        return 1;
    }
}; // namespace rbxInstance_HttpService_methods

void rbxInstance_HttpService_init() {
    rbxClass::class_map["HttpService"]->methods["GenerateGUID"].func = rbxInstance_HttpService_methods::generateGUID;
}

}; // namespace fakeroblox
