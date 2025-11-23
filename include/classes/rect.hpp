#pragma once

#include "lua.h"

namespace fakeroblox {

struct Rect {
    float minx;
    float miny;
    float maxx;
    float maxy;
};

int pushRect(lua_State* L, float minx, float miny, float maxx, float maxy);
int pushRect(lua_State* L, Rect rect);
Rect* lua_checkrect(lua_State* L, int narg);

void open_rectlib(lua_State* L);

}; // namespace fakeroblox
