#pragma once

#include "classes/roblox/instance.hpp"
#include "lua.h"

namespace frostbyte {

#define BINDLIST_KEY "renderstepbindlist"

class RunService {
public:
    static std::shared_ptr<rbxInstance> instance;
    static void process(lua_State* L);
};

void rbxInstance_RunService_init(lua_State* L);

}; // namespace frostbyte
