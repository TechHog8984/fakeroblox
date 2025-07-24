#include "environment.hpp"
#include "common.hpp"

#include <string>

#include "lua.h"
#include "lualib.h"
#include "Luau/Compiler.h"

namespace fakeroblox {

#define expose(func) {                       \
    lua_pushcfunction(L, fr_##func, #func);  \
    lua_setglobal(L, #func);                 \
}

static int fr_getreg(lua_State* L) {
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
}

// from Luau/CLI/src/Repl.cpp
static int fr_loadstring(lua_State* L) {
    size_t l = 0;
    const char* s = luaL_checklstring(L, 1, &l);
    const char* chunkname = luaL_optstring(L, 2, "");

    lua_setsafeenv(L, LUA_ENVIRONINDEX, false);

    std::string bytecode = Luau::compile(std::string(s, l));
    if (luau_load(L, chunkname, bytecode.data(), bytecode.size(), 0) == 0)
        return 1;

    lua_pushnil(L);
    lua_insert(L, -2); // put before error message
    return 2;          // return nil plus error message
}

static int fr_gcstep(lua_State* L) {
    lua_gc(L, LUA_GCSTEP, 0);
    return 0;
}
static int fr_gcfull(lua_State* L) {
    lua_gc(L, LUA_GCCOLLECT, 0);
    return 0;
}

void open_fakeroblox_environment(lua_State *L) {
    // methodlookup
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);

    // string list
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, STRINGLOOKUP);

    lua_pushcfunction(L, fakeroblox::fr_print, "print");
    lua_setglobal(L, "print");
    lua_pushcfunction(L, fakeroblox::fr_warn, "warn");
    lua_setglobal(L, "warn");

    expose(getreg)
    expose(loadstring)
    expose(gcstep);
    expose(gcfull);
}

}; // namespace fakeroblox
