#include "classes/roblox/datamodel.hpp"
#include "classes/roblox/instance.hpp"

#include "http.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

namespace rbxInstance_DataModel_methods {
    static int httpGet(lua_State* L) {
        // FIXME: yield!
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
    rbxClass::class_map["DataModel"]->newMethod("HttpGet", rbxInstance_DataModel_methods::httpGet);
    lua_pushcfunction(L, rbxInstance_DataModel_methods::httpGet, "httpget");
    lua_setglobal(L, "httpget");
}

};
