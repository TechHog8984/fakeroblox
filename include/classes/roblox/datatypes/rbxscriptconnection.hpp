#pragma once

#include "lua.h"

#include <cstdio>
#include <functional>

namespace fakeroblox {

class rbxScriptConnection {
public:
    bool alive = true;
    int function_index;

    void destroy(lua_State* L);
};

int pushNewRBXScriptConnection(lua_State* L, std::function<void()> pushValue);
int pushNewRBXScriptConnection(lua_State* L, int func_index);
rbxScriptConnection* lua_checkrbxscriptconnection(lua_State* L, int narg);

void setup_rbxScriptConnection(lua_State* L);

};
