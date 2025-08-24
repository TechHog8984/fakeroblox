#pragma once

#include "lua.h"

#include <cstdio>

namespace fakeroblox {

class rbxScriptConnection {
public:
    ~rbxScriptConnection() { printf("what...."); }

    bool alive = true;
    int function_index;

    void destroy(lua_State* L);
};

int pushNewRBXScriptConnection(lua_State* L, int func_index);
rbxScriptConnection* lua_checkrbxscriptconnection(lua_State* L, int narg);

void setup_rbxScriptConnection(lua_State* L);

};
