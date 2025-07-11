#pragma once

#include <raylib.h>

#include "lua.h"

namespace fakeroblox {

int pushColor3(lua_State* L, double r, double g, double b);
int pushColor3(lua_State* L, Color color);
Color* lua_checkcolor3(lua_State* L, int narg);

void open_color3lib(lua_State* L);

}; // namespace fakeroblox
