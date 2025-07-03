#pragma once

#include <functional>
#include <string>

#include "lua.h"

namespace fakeroblox {

using Feedback = std::function<void(std::string)>;

int fakeroblox_print(lua_State* thread);

}; // namespace fakeroblox
