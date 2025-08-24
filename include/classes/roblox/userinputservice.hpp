#pragma once

#include "classes/roblox/instance.hpp"

#include "lua.h"

namespace fakeroblox {

enum InputState {
    InputBegan,
    InputChanged,
    InputEnded
};

class UserInputService {
public:
    static bool is_window_focused;
    static Vector2 mouse_position;

    static void signalMouseMovement(std::shared_ptr<rbxInstance> instance, InputState type);
    static void process(lua_State* L);
};

void rbxInstance_UserInputService_init();

}; // namespace fakeroblox
