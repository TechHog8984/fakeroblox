#pragma once

#include "lua.h"

namespace frostbyte {

#define env_expose(func) {                   \
    lua_pushcfunction(L, fr_##func, #func);  \
    lua_setglobal(L, #func);                 \
}

void open_frostbyte_environment(lua_State* L);

}; // namespace frostbyte
