#include "classes/roblox/guibutton.hpp"

namespace fakeroblox {

std::map<rbxInstance*, bool> auto_button_color_map;

void rbxInstance_GuiButton_init() {
    rbxClass::class_map["GuiButton"]->constructor = [](lua_State* L, rbxInstance* instance) {
        std::get<bool>(instance->values["AutoButtonColor"].value) = true;
    };
}

bool checkAutoButtonColor(std::shared_ptr<rbxInstance> instance) {
    if (!instance->isA("GuiButton"))
        return false;
    if (!getInstanceValue<bool>(instance, "AutoButtonColor"))
        return false;

    // FIXME: when we make image buttons, we need to return false if we're doing a mouse hover and hoverimage is valid OR if we're doing a click and pressedimage is valid
    // (so we'll also need to intake a parameter differentiating hover vs click)

    // if (instance->isA("ImageButton") && isImageButtonImageValid(instance))
    //     return false;

    return true;
}
void handleGuiButtonMouseEnter(std::shared_ptr<rbxInstance> instance) {
    if (!checkAutoButtonColor(instance))
        return;

    auto_button_color_map[instance.get()] = true;
}
void handleGuiButtonMouseLeave(std::shared_ptr<rbxInstance> instance) {
    if (!checkAutoButtonColor(instance))
        return;

    auto_button_color_map[instance.get()] = false;
}

}; // namespace fakeroblox
