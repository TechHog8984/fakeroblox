#pragma once

#include <functional>
#include <string>
#include <cstring> // for strcmp for strequal
#include "nlohmann/json.hpp"

#include "lua.h"

using json = nlohmann::json;

namespace fakeroblox {

// NOTE: this is hard-coded sorry
#define MAX_KEYBOARD_KEYS 512

#define strequal(str1, str2) (strcmp(str1, str2) == 0)

#define luaL_optnumberloose(L, narg, def) (luaL_opt(L, lua_tonumber, narg, def))

#define LUA_TAG_RBXINSTANCE 1
#define LUA_TAG_ENUM 2
#define LUA_TAG_ENUMITEM 3

using Feedback = std::function<void(std::string)>;
using OnKill = std::function<void()>;

double luaL_checknumberrange(lua_State* L, int narg, double min, double max);
double luaL_optnumberrange(lua_State* L, int narg, double min, double max, double def = 0);

int newweaktable(lua_State* L);

int pushFromLookup(lua_State* L, const char* lookup, std::function<void()> pushKey, std::function<void()> pushValue);
int pushFromLookup(lua_State* L, const char* lookup, void* ptr, std::function<void()> pushValue);

int pushFunctionFromLookup(lua_State* L, lua_CFunction func, const char* name = nullptr, lua_Continuation cont = nullptr);
// push lookup, call addToLookup, lookup is popped by addToLookup
int addToLookup(lua_State *L, std::function<void()> pushValue, bool keep_value = false);

#define INSTANCELOOKUP "instancelookup"
#define METHODLOOKUP "methodlookup"
#define STRINGLOOKUP "stringlookup"
#define SIGNALLOOKUP "signallookup"
#define SIGNALCONNECTIONLISTLOOKUP "signalconnectionlistlookup"
#define ENUMLOOKUP "enumlookup"

// TODO: replace as many std::string functon parameters as are feasible
int addToStringLookup(lua_State* L, std::string string);
const char* getFromStringLookup(lua_State* L, int index);

std::string getStackMessage(lua_State* L);

int fr_print(lua_State* L);
int fr_warn(lua_State* L);

// if you keep lookup as false, NOTE that ProtoExplorer will ensure functions are in the lookup when explored
void setfunctionfield(lua_State* L, lua_CFunction func, const char* method, const char* debugname, bool lookup = false);
// if you keep lookup as false, NOTE that ProtoExplorer will ensure functions are in the lookup when explored
void setfunctionfield(lua_State* L, lua_CFunction func, const char* method, bool lookup = false);

}; // namespace fakeroblox
