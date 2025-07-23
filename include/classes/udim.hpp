#pragma once

#include "lua.h"
#include <raylib.h>

namespace fakeroblox {

struct UDim {
    float scale;
    float offset;
};

int pushUDim(lua_State* L, double scale, double offset);
int pushUDim(lua_State* L, UDim udim);

bool lua_isudim(lua_State* L, int index);
UDim* lua_checkudim(lua_State* L, int index);

void open_udimlib(lua_State* L);

}; // namespace fakeroblox
