#include "classes/roblox/guiobject.hpp"
#include "classes/roblox/instance.hpp"

namespace fakeroblox {

void rbxInstance_GuiObject_init() {
    rbxClass::class_map["GuiObject"]->constructor = [](lua_State* L, rbxInstance* instance) {
        std::get<Color>(instance->values["BackgroundColor3"].value) = {163, 162, 165, 255};
        std::get<Color>(instance->values["BorderColor3"].value) = {27, 42, 53, 255};
        std::get<int>(instance->values["BorderSizePixel"].value) = 1;
        std::get<bool>(instance->values["Visible"].value) = true;
    };
}

};
