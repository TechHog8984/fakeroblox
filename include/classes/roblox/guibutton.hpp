#pragma once

#include "classes/roblox/instance.hpp"

namespace fakeroblox {

static const float AUTO_BUTTON_COLOR_V = 1.425;
extern std::map<rbxInstance*, bool> auto_button_color_map;

void rbxInstance_GuiButton_init();

void handleGuiButtonMouseEnter(std::shared_ptr<rbxInstance> instance);
void handleGuiButtonMouseLeave(std::shared_ptr<rbxInstance> instance);

}; // namespace fakeroblox
