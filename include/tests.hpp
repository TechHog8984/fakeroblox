#pragma once

#include <string>
#include <variant>

#include "lua.h"

namespace fakeroblox {
    struct FakeRobloxTest {
        const char* name;
        std::variant<lua_CFunction, std::string> value;
    };

    void setupTests(bool* is_running_tests, bool* all_tests_succeeded);
    void startAllTests(lua_State* L);
}; // namespace fakeroblox
