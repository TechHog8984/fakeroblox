#pragma once

#include "lua.h"

namespace fakeroblox {

class UserInputService {
public:
    static void process(lua_State* L);
};

void rbxInstance_UserInputService_init(lua_State* L);

}; // namespace fakeroblox
