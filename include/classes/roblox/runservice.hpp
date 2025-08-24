#pragma once

#include "lua.h"

namespace fakeroblox {

class RunService {
public:
    static void process(lua_State* L);
};

void rbxInstance_RunService_init(lua_State* L);

}; // namespace fakeroblox
