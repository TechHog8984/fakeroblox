#include "classes/roblox/runservice.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/instance.hpp"

#include "common.hpp"
#include "console.hpp"
#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

std::shared_ptr<rbxInstance> RunService::instance;

std::shared_mutex bind_list_mutex;

static int compare(lua_State* L) {
    lua_rawgeti(L, 1, 1);
    lua_rawgeti(L, 2, 1);

    lua_pushboolean(L, lua_tointeger(L, 3) < lua_tointeger(L, 4));
    return 1;
}

namespace rbxInstance_RunService_methods {
    static int bindToRenderStep(lua_State* L) {
        std::lock_guard lock(bind_list_mutex);

        lua_checkinstance(L, 1, "RunService");
        const char* name = luaL_checkstring(L, 2);
        luaL_checkinteger(L, 3);
        luaL_checktype(L, 4, LUA_TFUNCTION);

        lua_getglobal(L, "table");
        lua_rawgetfield(L, -1, "sort");
        lua_remove(L, -2);

        lua_rawgetfield(L, LUA_REGISTRYINDEX, BINDLIST_KEY);

        lua_createtable(L, 2, 0);

        lua_pushvalue(L, 3);
        lua_rawseti(L, -2, 1);

        lua_pushvalue(L, 4);
        lua_rawseti(L, -2, 2);

        // FIXME: determine duplicate name behavior
        lua_rawsetfield(L, -2, name);
        pushFunctionFromLookup(L, compare);

        lua_call(L, 2, 0);

        return 0;
    }

    static int isClient(lua_State* L) {
        lua_pushboolean(L, true);
        return 1;
    }
    static int isServer(lua_State* L) {
        lua_pushboolean(L, false);
        return 1;
    }

    static int unbindFromRenderStep(lua_State* L) {
        std::lock_guard lock(bind_list_mutex);

        lua_checkinstance(L, 1, "RunService");
        const char* name = luaL_checkstring(L, 2);

        lua_rawgetfield(L, LUA_REGISTRYINDEX, BINDLIST_KEY);
        lua_rawgetfield(L, -1, name);

        if (lua_isnil(L, -1))
            lua_pop(L, 2);
        else {
            lua_pop(L, 1);
            lua_pushnil(L);
            lua_rawsetfield(L, -2, name);
            lua_pop(L, 1);
        }

        return 0;
    }
}; // namespace rbxInstance_RunService_methods

void fireEventWithDelta(lua_State* L, const char* event, std::shared_ptr<rbxInstance>& instance, double delta) {
    pushFunctionFromLookup(L, fireRBXScriptSignal);
    instance->pushEvent(L, event);
    lua_pushnumber(L, delta);

    lua_call(L, 2, 0);
}

double last_clock = lua_clock();
void RunService::process(lua_State *L) {
    std::lock_guard lock(bind_list_mutex);

    const double clock = lua_clock();
    const double delta = clock - last_clock;
    last_clock = clock;

    lua_rawgetfield(L, LUA_REGISTRYINDEX, BINDLIST_KEY);

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_rawgeti(L, -1, 2);
        lua_pushnumber(L, delta);
        switch (lua_pcall(L, 1, 0, 0)) {
            case LUA_OK:
                break;
            case LUA_ERRRUN:
            case LUA_ERRMEM:
            case LUA_ERRERR:
                Console::ScriptConsole.errorf("RunService::fireRenderStepEarlyFunctions unexpected error while invoking callback: %s", lua_tostring(L, -1));
                lua_pop(L, 1);
                break;
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    fireEventWithDelta(L, "RenderStepped", instance, delta);
    // FIXME: instead of RunService::process, we need a RunService::preRender and RunService::postRender instead of just firing all these events (and the above bindtorenderstep stuff) here
    fireEventWithDelta(L, "PreRender", instance, delta);
}

void rbxInstance_RunService_init(lua_State* L) {
    lua_newtable(L);

    lua_rawsetfield(L, LUA_REGISTRYINDEX, BINDLIST_KEY);

    rbxClass::class_map["RunService"]->methods["BindToRenderStep"].func = rbxInstance_RunService_methods::bindToRenderStep;
    rbxClass::class_map["RunService"]->methods["IsClient"].func = rbxInstance_RunService_methods::isClient;
    rbxClass::class_map["RunService"]->methods["IsServer"].func = rbxInstance_RunService_methods::isServer;
    rbxClass::class_map["RunService"]->methods["UnbindFromRenderStep"].func = rbxInstance_RunService_methods::unbindFromRenderStep;
}

}; // namespace fakeroblox
