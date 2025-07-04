#pragma once

#include <functional>
#include <string>
#include <cstring> // for strcmp for strequal

#include "lua.h"

namespace fakeroblox {

#define strequal(str1, str2) (strcmp(str1, str2) == 0)

using Feedback = std::function<void(std::string)>;

int fakeroblox_print(lua_State* L);
int fakeroblox_warn(lua_State* L);

}; // namespace fakeroblox
