#pragma once

#include "lua.h"

namespace fakeroblox {

#define env_expose(func) {                   \
    lua_pushcfunction(L, fr_##func, #func);  \
    lua_setglobal(L, #func);                 \
}

void open_fakeroblox_environment(lua_State* L);

}; // namespace fakeroblox
