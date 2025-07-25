#pragma once

#include "classes/roblox/instance.hpp"

#include "lua.h"

namespace fakeroblox {

enum InputType {
    InputBegan,
    InputChanged,
    InputEnded
};

class UserInputService {
public:
    static bool is_window_focused;

    static void signalMouseMovement(std::shared_ptr<rbxInstance> instance, InputType type);
    static void process(lua_State* L);
};

void rbxInstance_UserInputService_init(lua_State* L);

}; // namespace fakeroblox
