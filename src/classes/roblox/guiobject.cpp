#include "classes/roblox/guiobject.hpp"
#include "classes/roblox/guibutton.hpp"
#include "classes/roblox/instance.hpp"

namespace fakeroblox {

void rbxInstance_GuiObject_init() {
    auto& this_class = rbxClass::class_map["GuiObject"];

    this_class->constructor = [](lua_State* L, std::shared_ptr<rbxInstance> instance) {
        setInstanceValue(instance, L, "BackgroundColor3", Color { 163, 162, 165, 255 }, true);
        setInstanceValue(instance, L, "BorderColor3", Color { 27, 42, 53, 255 }, true);
        setInstanceValue(instance, L, "BorderSizePixel", 1, true);
        setInstanceValue(instance, L, "Visible", true, true);
    };

    rbxInstance_GuiButton_init();
}

}; // namespace fakeroblox
