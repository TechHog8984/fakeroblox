#pragma once

#include <functional>
#include <string>
#include <cstring> // for strcmp for strequal

#include "lua.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace fakeroblox {

#define strequal(str1, str2) (strcmp(str1, str2) == 0)

using Feedback = std::function<void(std::string)>;
using OnKill = std::function<void()>;

double luaL_checknumberrange(lua_State* L, int narg, double min, double max);
double luaL_optnumberrange(lua_State* L, int narg, double min, double max, double def = 0);

int newweaktable(lua_State* L);
int pushFromLookup(lua_State* L, const char* lookup, void* ptr, std::function<void()> pushValue);
int pushFunctionFromLookup(lua_State* L, lua_CFunction func, std::string name = nullptr, lua_Continuation cont = nullptr);

#define INSTANCELOOKUP "instancelookup"
#define METHODLOOKUP "methodlookup"
#define STRINGLOOKUP "stringlookup"

int addToStringLookup(lua_State* L, std::string string);
const char* getFromStringLookup(lua_State* L, int index);

int fr_print(lua_State* L);
int fr_warn(lua_State* L);

void setfunctionfield(lua_State* L, lua_CFunction func, const char* method, bool lookup = false);

}; // namespace fakeroblox
