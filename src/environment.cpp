#include "environment.hpp"
#include "base64.hpp"
#include "taskscheduler.hpp"

#include "classes/roblox/userinputservice.hpp"

#include <string>

#include "lua.h"
#include "lualib.h"
#include "lgc.h"
#include "lapi.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "Luau/Common.h"
#include "Luau/Compiler.h"

namespace fakeroblox {

static int fr_getreg(lua_State* L) {
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
}
static int fr_getgc(lua_State* L) {
    const bool resume_gc = TaskScheduler::pauseGarbageCollection(L);

    struct VisitUserdata_t {
        bool tables;
        std::vector<GCObject*> list;
    } VisitUserdata;
    VisitUserdata.tables = luaL_optboolean(L, 1, 0);

    luaM_visitgco(L, &VisitUserdata, [](void* ud, lua_Page* page, GCObject* object) {
        auto visit_ud = static_cast<VisitUserdata_t*>(ud);

        if (object->gch.tt == LUA_TTABLE && !visit_ud->tables)
            goto RET;

        switch (object->gch.tt) {
            case LUA_TTABLE:
            case LUA_TFUNCTION:
            case LUA_TUSERDATA:
            case LUA_TTHREAD:
                break;
            default:
                goto RET;
        }

        visit_ud->list.push_back(object);

        RET:
        return false;
    });

    lua_createtable(L, VisitUserdata.list.size(), 0);

    for (size_t i = 0; i < VisitUserdata.list.size(); i++) {
        auto& obj = VisitUserdata.list[i];
        lua_pushnil(L);
        auto new_value = const_cast<TValue*>(luaA_toobject(L, -1));
        switch (obj->gch.tt) {
            case LUA_TTABLE:
                sethvalue(L, new_value, gco2h(obj));
                break;
            case LUA_TFUNCTION:
                setclvalue(L, new_value, gco2cl(obj));
                break;
            case LUA_TUSERDATA:
                setuvalue(L, new_value, gco2u(obj));
                break;
            case LUA_TTHREAD:
                setthvalue(L, new_value, gco2th(obj));
                break;
            default:
                LUAU_UNREACHABLE();
        }
        lua_rawseti(L, -2, i + 1);
    }

    if (resume_gc)
        TaskScheduler::resumeGarbageCollection(L);

    return 1;
}

static int fr_safetostring(lua_State* L) {
    luaL_checkany(L, 1);
    std::string str = safetostring(L, 1);

    lua_pushlstring(L, str.c_str(), str.size());
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

static int fr_rawtfreeze(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_argcheck(L, !lua_getreadonly(L, 1), 1, "table is already frozen");

    lua_setreadonly(L, 1, true);

    lua_pushvalue(L, 1);
    return 1;
}
static int fr_rawtunfreeze(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_argcheck(L, lua_getreadonly(L, 1), 1, "table is not frozen");

    lua_setreadonly(L, 1, false);

    lua_pushvalue(L, 1);
    return 1;
}
static int fr_tsetfrozen(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    const bool frozen = luaL_checkboolean(L, 2);

    lua_setreadonly(L, 1, frozen);

    lua_pushvalue(L, 1);
    return 1;
}

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

static int fr_iswindowactive(lua_State* L) {
    lua_pushboolean(L, UserInputService::is_window_focused);
    return 1;
}

static int fr_setfpscap(lua_State* L) {
    TaskScheduler::setTargetFps(luaL_checknumberrange(L, 1, 0, static_cast<unsigned>(-1)));
    return 0;
}
static int fr_getfpscap(lua_State* L) {
    lua_pushinteger(L, TaskScheduler::target_fps);
    return 1;
}

static int fr_getgenv(lua_State* L) {
    if (TaskScheduler::sandboxing)
        luaL_error(L, "you cannot use getgenv while sandboxing is enabled! rerun fakeroblox with the --nosandbox flag");

    // TODO: should this be environindex ?? i have no idea
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    return 1;
}
static int fr_getrenv(lua_State* L) {
    // TODO: getrenv; should essentially be shared but with all environment values (__index = environment maybe); should be one specific table we keep in registry or something
    lua_newtable(L);
    return 1;
}

void open_fakeroblox_environment(lua_State *L) {
    // methodlookup
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);

    // string list
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, STRINGLOOKUP);

    lua_pushcfunction(L, fr_print, "print");
    lua_setglobal(L, "print");
    lua_pushcfunction(L, fr_warn, "warn");
    lua_setglobal(L, "warn");

    env_expose(getreg)
    env_expose(getgc)

    env_expose(safetostring)

    env_expose(loadstring)

    env_expose(gcstep)
    env_expose(gcfull)

    env_expose(base64encode)
    env_expose(base64decode)

    env_expose(iswindowactive)

    env_expose(setfpscap)
    env_expose(getfpscap)

    env_expose(getgenv)
    env_expose(getrenv)

    lua_getglobal(L, "table");
    lua_pushcfunction(L, fr_rawtfreeze, "rawfreeze");
    lua_rawsetfield(L, -2, "rawfreeze");
    lua_pushcfunction(L, fr_rawtunfreeze, "rawunfreeze");
    lua_rawsetfield(L, -2, "rawunfreeze");
    lua_pushcfunction(L, fr_tsetfrozen, "setfrozen");
    lua_rawsetfield(L, -2, "setfrozen");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
