#pragma once

#include "lua.h"

namespace fakeroblox {

class Vector2 {
public:
    double x = 0;
    double y = 0;

    Vector2(double x, double y);
};

void open_vector2lib(lua_State* L);

}; // namespace fakeroblox
