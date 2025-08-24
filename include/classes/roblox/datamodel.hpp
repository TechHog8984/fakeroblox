#pragma once

#include "lua.h"

namespace fakeroblox {

class DataModel {
public:
    static bool shutdown;

    static void onShutdown(lua_State* L);
};

void rbxInstance_DataModel_init(lua_State* L);

}; // namespace fakeroblox
