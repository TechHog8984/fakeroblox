#include "classes/roblox/baseplayergui.hpp"
#include "classes/roblox/camera.hpp"
#include "classes/roblox/instance.hpp"
#include "classes/udim2.hpp"

#include "raylib.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <mutex>

namespace fakeroblox {

std::vector<std::shared_ptr<rbxInstance>> gui_storage_list;

// modification of DrawRectanglePro that doesn't fill and allows thickness
void DrawRectangleLinesPro(Rectangle rec, Vector2 origin, float rotation, float thickness, Color color) {
    // FIXME: this leaves gaps in the corners

    Vector2 topLeft = { 0 };
    Vector2 topRight = { 0 };
    Vector2 bottomLeft = { 0 };
    Vector2 bottomRight = { 0 };

    // Only calculate rotation if needed
    if (rotation == 0.0f) {
        float x = rec.x - origin.x;
        float y = rec.y - origin.y;
        topLeft = (Vector2){ x, y };
        topRight = (Vector2){ x + rec.width, y };
        bottomLeft = (Vector2){ x, y + rec.height };
        bottomRight = (Vector2){ x + rec.width, y + rec.height };
    } else {
        float sinRotation = sinf(rotation*DEG2RAD);
        float cosRotation = cosf(rotation*DEG2RAD);
        float x = rec.x;
        float y = rec.y;
        float dx = -origin.x;
        float dy = -origin.y;

        topLeft.x = x + dx*cosRotation - dy*sinRotation;
        topLeft.y = y + dx*sinRotation + dy*cosRotation;

        topRight.x = x + (dx + rec.width)*cosRotation - dy*sinRotation;
        topRight.y = y + (dx + rec.width)*sinRotation + dy*cosRotation;

        bottomLeft.x = x + dx*cosRotation - (dy + rec.height)*sinRotation;
        bottomLeft.y = y + dx*sinRotation + (dy + rec.height)*cosRotation;

        bottomRight.x = x + (dx + rec.width)*cosRotation - (dy + rec.height)*sinRotation;
        bottomRight.y = y + (dx + rec.width)*sinRotation + (dy + rec.height)*cosRotation;
    }

    DrawLineEx(topLeft, topRight, thickness, color);
    DrawLineEx(topRight, bottomRight, thickness, color);
    DrawLineEx(bottomRight, bottomLeft, thickness, color);
    DrawLineEx(bottomLeft, topLeft, thickness, color);
}

void renderGuiObject(lua_State* L, std::shared_ptr<rbxInstance> object) {
    std::shared_ptr<rbxInstance> parent = object->getValue<std::shared_ptr<rbxInstance>>(PROP_INSTANCE_PARENT);
    const bool is_storage_child = std::find(gui_storage_list.begin(), gui_storage_list.end(), parent) != gui_storage_list.end();

    // TODO: separate function for pos&size calculations that only get called on parent changed?
    auto& parent_absolute_position = is_storage_child ? rbxCamera::screen_size : parent->getValue<Vector2>("AbsolutePosition");
    auto& parent_absolute_size = is_storage_child ? rbxCamera::screen_size : parent->getValue<Vector2>("AbsoluteSize");

    auto& position = object->getValue<UDim2>("Position");
    auto& size = object->getValue<UDim2>("Size");

    auto position_x_scale = position.x.scale;
    auto position_y_scale = position.y.scale;

    if (!is_storage_child) {
        position_x_scale++;
        position_y_scale++;
    }

    Vector2 absolute_position{
        parent_absolute_position.x * position_x_scale + position.x.offset,
        parent_absolute_position.y * position_y_scale + position.y.offset
    };
    Vector2 absolute_size{
        parent_absolute_size.x * size.x.scale + size.x.offset,
        parent_absolute_size.y * size.y.scale + size.y.offset
    };

    object->setValue<Vector2>(L, "AbsolutePosition", absolute_position);
    object->setValue<Vector2>(L, "AbsoluteSize", absolute_size);

    if (!object->getValue<bool>("Visible"))
        return;

    auto rotation = object->getValue<float>("Rotation");
    auto backgroundcolor = object->getValue<Color>("BackgroundColor3");
    backgroundcolor.a = (1 - object->getValue<float>("BackgroundTransparency")) * 255;

    Rectangle background_rect{
        .x = absolute_position.x * 2,
        .y = absolute_position.y * 2,
        .width = absolute_size.x,
        .height = absolute_size.y,
    };
    // TODO: rotation origin still seems a bit off
    DrawRectanglePro(background_rect, absolute_position, rotation, backgroundcolor);

    auto border_size = object->getValue<int>("BorderSizePixel");
    if (border_size) {
        auto border_color = object->getValue<Color>("BorderColor3");
        border_color.a = backgroundcolor.a;

        DrawRectangleLinesPro(background_rect, absolute_position, rotation, border_size, border_color);
    }
}

std::vector<std::shared_ptr<rbxInstance>> render_list;
void contributeToRenderList(std::shared_ptr<rbxInstance> instance, bool is_storage = false) {
    if (!is_storage) {
        if (instance->isA("LayerCollector")) {
            if (!instance->getValue<bool>("Enabled"))
                return;
            goto ADD_CHILDREN;
        }

        if (!instance->isA("GuiObject"))
            return;

        render_list.push_back(instance);
    }

    ADD_CHILDREN:
    std::lock_guard lock(instance->children_mutex);
    for (size_t i = 0; i < instance->children.size(); i++)
        contributeToRenderList(instance->children[i]);
}

void rbxInstance_BasePlayerGui_process(lua_State *L) {
    render_list.clear();

    for (unsigned int i = 0; i < gui_storage_list.size(); i++)
        contributeToRenderList(gui_storage_list[i], true);

    std::sort(render_list.begin(), render_list.end(), [] (std::shared_ptr<rbxInstance> a, std::shared_ptr<rbxInstance> b) {
        return a->getValue<int>("ZIndex") < b->getValue<int>("ZIndex");
    });

    for (unsigned int i = 0; i < render_list.size(); i++)
        renderGuiObject(L, render_list[i]);
}

void rbxInstance_BasePlayerGui_init(lua_State *L, std::initializer_list<std::shared_ptr<rbxInstance>> initial_gui_storage_list) {
    gui_storage_list.insert(gui_storage_list.end(), initial_gui_storage_list.begin(), initial_gui_storage_list.end());
}

};
