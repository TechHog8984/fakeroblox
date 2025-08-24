#pragma once

#include "lua.h"
#include "raylib.h"

namespace fakeroblox {

int pushVector2(lua_State* L, float x, float y);
int pushVector2(lua_State* L, Vector2 vector2);
Vector2* lua_checkvector2(lua_State* L, int narg);

void open_vector2lib(lua_State* L);

}; // namespace fakeroblox
