#include "classes/roblox/guiobject.hpp"
#include "classes/roblox/instance.hpp"

namespace fakeroblox {

void rbxInstance_GuiObject_init() {
    rbxClass::class_map["GuiObject"]->constructor = [](lua_State* L, std::shared_ptr<rbxInstance> instance) {
        instance->setValue<Color>(L, "BackgroundColor3", {163, 162, 165, 255});
        instance->setValue<Color>(L, "BorderColor3", {27, 42, 53, 255});
        instance->setValue<int>(L, "BorderSizePixel", 1);
        instance->setValue<bool>(L, "Visible", true);
    };
}

};
