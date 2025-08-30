#include "libraries/cryptlib.hpp"

#include "base64.hpp"

#include "common.hpp"
#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

static int fr_base64encode(lua_State* L) {
    size_t input_size;
    const char* input = const_cast<char*>(luaL_checklstring(L, 1, &input_size));

    std::string output = b64_encode(reinterpret_cast<const unsigned char*>(input), input_size);

    lua_pushlstring(L, output.c_str(), output.size());
    return 1;
}
static int fr_base64decode(lua_State* L) {
    size_t input_size;
    const char* input = const_cast<char*>(luaL_checklstring(L, 1, &input_size));

    std::string output = b64_decode(reinterpret_cast<const unsigned char*>(input), input_size);

    lua_pushlstring(L, output.c_str(), output.size());
    return 1;
}
void open_cryptlib(lua_State* L) {
    // crypt global
    lua_newtable(L);

    setfunctionfield(L, fr_base64encode, "base64encode");
    setfunctionfield(L, fr_base64decode, "base64decode");

    lua_setglobal(L, "crypt");
}

}; // namespace fakeroblox
