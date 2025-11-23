#include "classes/roblox/coregui.hpp"
#include "classes/udim2.hpp"

namespace fakeroblox {

std::shared_ptr<rbxInstance> CoreGui::notification_frame;

namespace rbxInstance_CoreGui_methods {
}; // rbxInstance_CoreGui_methods

void rbxInstance_CoreGui_setup(lua_State* L, std::shared_ptr<rbxInstance> instance) {
    auto roblox_gui = newInstance(L, "ScreenGui", instance);
    setInstanceValue<std::string>(roblox_gui, L, PROP_INSTANCE_NAME, "RobloxGui", true);

    auto notification_frame = newInstance(L, "Frame", roblox_gui);
    setInstanceValue<std::string>(notification_frame, L, PROP_INSTANCE_NAME, "NotificationFrame", true);
    setInstanceValue(notification_frame, L, "Size", UDim2{0, 200, 0.42, 0}, true);
    setInstanceValue(notification_frame, L, "Position", UDim2{1, -200 - 4, 0.5, 0}, true);
    setInstanceValue<float>(notification_frame, L, "BackgroundTransparency", 1.0, true);

    CoreGui::notification_frame = notification_frame;
}

}; // namespace fakeroblox
