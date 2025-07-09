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

int fakeroblox_print(lua_State* L);
int fakeroblox_warn(lua_State* L);

}; // namespace fakeroblox
