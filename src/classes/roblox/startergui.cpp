#include "classes/roblox/startergui.hpp"
#include "classes/roblox/coregui.hpp"
#include "classes/roblox/datatypes/enum.hpp"
#include "classes/roblox/instance.hpp"

#include "common.hpp"
#include "lualib.h"

namespace fakeroblox {

std::shared_ptr<rbxInstance> notification_frame_title_template;
std::shared_ptr<rbxInstance> notification_frame_text_template;

namespace rbxInstance_StarterGui_methods {
    static int setCore(lua_State* L) {
        lua_checkinstance(L, 1, "StarterGui");
        const char* parameter = luaL_checkstring(L, 2);

        if (strequal(parameter, "SendNotification")) {
            if (lua_istable(L, 3)) {
                lua_rawgetfield(L, 3, "Title");
                if (!lua_isstring(L, -1))
                    goto RET;
                const char* title = lua_tostring(L, -1);
                lua_pop(L, 1);

                lua_rawgetfield(L, 3, "Text");
                if (!lua_isstring(L, -1))
                    goto RET;
                const char* text = lua_tostring(L, -1);
                lua_pop(L, 1);

                auto notification = newInstance(L, "Frame");
                setInstanceValue<std::string>(notification, L, PROP_INSTANCE_NAME, "Notification", true);
                setInstanceValue(notification, L, "Size", UDim2{1, 0, 0, 45}, true);
                // static float notification_transparency = 0.6 * l_GameSettings_0.PreferredTransparency;
                static float notification_transparency = 0.6;
                setInstanceValue(notification, L, "BackgroundTransparency", notification_transparency, true);
                setInstanceValue(notification, L, "BackgroundColor3", Color{0, 0, 0, 255}, true);
                setInstanceValue(notification, L, "BorderSizePixel", 0, true);

                auto notification_title = cloneInstance(L, notification_frame_title_template);
                setInstanceValue<std::string>(notification_title, L, "Text", title);

                auto notification_text = cloneInstance(L, notification_frame_text_template);
                setInstanceValue<std::string>(notification_text, L, "Text", text);

                setInstanceParent(L, notification_title, notification);
                setInstanceParent(L, notification_text, notification);

                setInstanceParent(L, notification, CoreGui::notification_frame);
            }
        } else
             luaL_error(L, "SetCore: %s has not been registered by the CoreScripts", parameter);

        RET:
        return 0;
    }
}; // rbxInstance_StarterGui_methods

void rbxInstance_StarterGui_init(lua_State* L) {
    rbxClass::class_map["StarterGui"]->methods["SetCore"].func = rbxInstance_StarterGui_methods::setCore;

    notification_frame_title_template = newInstance(L, "TextLabel");
    setInstanceValue<std::string>(notification_frame_title_template, L, PROP_INSTANCE_NAME, "NotificationTitle", true);
    setInstanceValue(notification_frame_title_template, L, "Size", UDim2{1, -100 * 2, 0, 18}, true);
    setInstanceValue(notification_frame_title_template, L, "Position", UDim2{0, 100, 0.5, -18}, true);
    setInstanceValue(notification_frame_title_template, L, "BackgroundTransparency", 1.f, true);
    getInstanceValue<EnumItemWrapper>(notification_frame_title_template, "Font").name = "SourceSansBold";
    getInstanceValue<EnumItemWrapper>(notification_frame_title_template, "FontSize").name = "Size18";
    setInstanceValue(notification_frame_title_template, L, "TextColor3", Color{247, 247, 247, 255}, true);

    notification_frame_text_template = newInstance(L, "TextLabel");
    setInstanceValue<std::string>(notification_frame_text_template, L, PROP_INSTANCE_NAME, "NotificationText", true);
    setInstanceValue(notification_frame_text_template, L, "Size", UDim2{1, -100 * 2, 0, 28}, true);
    setInstanceValue(notification_frame_text_template, L, "Position", UDim2{0, 100, 0.5, 1}, true);
    setInstanceValue(notification_frame_text_template, L, "BackgroundTransparency", 1.f, true);
    getInstanceValue<EnumItemWrapper>(notification_frame_text_template, "Font").name = "SourceSans";
    getInstanceValue<EnumItemWrapper>(notification_frame_text_template, "FontSize").name = "Size14";
    setInstanceValue(notification_frame_text_template, L, "TextColor3", Color{235, 235, 235, 255}, true);
    setInstanceValue(notification_frame_text_template, L, "TextWrap", true, true);
    getInstanceValue<EnumItemWrapper>(notification_frame_text_template, "TextYAlignment").name = "Top";
}

}; // namespace fakeroblox
