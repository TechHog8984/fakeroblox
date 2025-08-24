#pragma once

#include "classes/udim.hpp"

#include "lua.h"

namespace fakeroblox {

struct UDim2 {
    UDim x;
    UDim y;
};

int pushUDim2(lua_State* L, UDim x, UDim y);
int pushUDim2(lua_State* L, UDim2 udim);
UDim2* lua_checkudim2(lua_State* L, int narg);

void open_udim2lib(lua_State* L);

}; // namespace fakeroblox
