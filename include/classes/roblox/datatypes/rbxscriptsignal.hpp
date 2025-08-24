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
};

void pushSignalConnectionList(lua_State* L, int narg);
int pushNewRBXScriptSignal(lua_State* L, std::string name);
rbxScriptSignal* lua_checkrbxscriptsignal(lua_State* L, int narg);

// push signal, then args, just like a function
int fireRBXScriptSignal(lua_State* L);
// push signal, then args, just like a function
int fireRBXScriptSignalWithFilter(lua_State* L);
// push signal
int disconnectAllRBXScriptSignal(lua_State* L);

void setup_rbxScriptSignal(lua_State* L);

}; // namespace fakeroblox
