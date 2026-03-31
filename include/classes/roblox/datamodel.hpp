#pragma once

#include "classes/roblox/instance.hpp"

namespace frostbyte {

class DataModel {
public:
    static std::shared_ptr<rbxInstance> instance;

    static bool shutdown;

    static void onShutdown(lua_State* L);
};

void rbxInstance_DataModel_init(lua_State* L);

}; // namespace frostbyte
