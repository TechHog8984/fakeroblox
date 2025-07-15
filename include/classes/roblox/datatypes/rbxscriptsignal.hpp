#pragma once

#include "classes/roblox/datatypes/rbxscriptconnection.hpp"

#include <string>
#include <vector>

#include "lua.h"

namespace fakeroblox {

class rbxScriptSignal {
public:
    std::string name;
    std::vector<rbxScriptConnection> connection_list;

    void destroy(lua_State* L);
};

void pushSignalConnectionList(lua_State* L, int narg);
int pushNewRbxScriptSignal(lua_State* L, std::string name);
rbxScriptSignal* lua_checkrbxscriptsignal(lua_State* L, int narg);

// push signal, then args, just like a function
int fireRbxScriptSignal(lua_State* L);
int fireRbxScriptSignalWithFilter(lua_State* L);

void setup_rbxscriptsignal(lua_State* L);

}; // namespace fakeroblox
