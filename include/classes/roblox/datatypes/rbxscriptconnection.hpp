#pragma once

#include "lua.h"

namespace fakeroblox {

class rbxScriptConnection {
public:
    bool alive = true;
    int function_index;

    void destroy(lua_State* L);
};

int pushNewRBXScriptConnection(lua_State* L, int func_index);
rbxScriptConnection* lua_checkrbxscriptconnection(lua_State* L, int narg);

void setup_rbxscriptconnection(lua_State* L);

};
