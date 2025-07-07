#include "libraries/signallib.hpp"
#include "lualib.h"

namespace fakeroblox {

// FIXME: signals should be an rbxInstance so we'll come back to signals when we add instance classes

void setup_signallib(lua_State *L) {
    luaL_newmetatable(L, "Signal");

    

    lua_pop(L, 1);
}

}; // namespace fakeroblox
