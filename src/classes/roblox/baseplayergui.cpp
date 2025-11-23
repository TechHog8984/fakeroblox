#include "classes/roblox/baseplayergui.hpp"
#include "classes/roblox/camera.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/guibutton.hpp"
#include "classes/roblox/instance.hpp"
#include "classes/roblox/userinputservice.hpp"
#include "classes/udim2.hpp"

#include "common.hpp"

#include "raylib.h"
#include "rlgl.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <mutex>

namespace fakeroblox {

std::vector<std::shared_ptr<rbxInstance>> gui_storage_list;

// modification of DrawRectanglePro that doesn't fill and allows thickness
std::array<Vector2, 4> getRectangleLinesPro(Rectangle rec, Vector2 origin, float rotation) {
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


    return {topLeft, topRight, bottomRight, bottomLeft};
}
void DrawRotatedRectangleLines(Vector2 position, Vector2 size, float rotation, float thickness, Color color) {
    Vector2 center = { position.x + size.x / 2.0f, position.y + size.y / 2.0f };

    rlPushMatrix();
    rlTranslatef(center.x, center.y, 0.0f);
    rlRotatef(rotation, 0, 0, 1);
    rlTranslatef(-center.x, -center.y, 0.0f);

    Rectangle rect = { position.x, position.y, size.x, size.y };
    DrawRectangleLinesEx(rect, thickness, color);

    rlPopMatrix();
}

std::weak_ptr<rbxInstance> clickable_instance;
std::weak_ptr<rbxInstance> next_clickable_instance;

std::weak_ptr<rbxInstance> getClickableGuiObject() {
    return clickable_instance;
}

std::vector<std::weak_ptr<rbxInstance>> gui_objects_hovered;
std::vector<std::weak_ptr<rbxInstance>> next_gui_objects_hovered;

std::vector<std::weak_ptr<rbxInstance>> getGuiObjectsHovered() {
    return gui_objects_hovered;
}

std::vector<std::shared_ptr<rbxInstance>> mouse_enter_list;
std::vector<std::shared_ptr<rbxInstance>> mouse_leave_list;
std::map<rbxInstance*, bool> mouse_over_map;

// NOTE: expects Parent
static bool isStorageChild(std::shared_ptr<rbxInstance> instance) {
    auto parent = getInstanceValue<std::shared_ptr<rbxInstance>>(instance, PROP_INSTANCE_PARENT);
    assert(parent);

    return std::find(gui_storage_list.begin(), gui_storage_list.end(), parent) != gui_storage_list.end();
}
Vector2 Vector2Zero{0, 0};

void renderGuiObject(lua_State* L, std::shared_ptr<rbxInstance> instance, Vector2 mouse, bool anyImGui) {
    auto parent = getInstanceValue<std::shared_ptr<rbxInstance>>(instance, PROP_INSTANCE_PARENT);
    const bool is_storage_child = isStorageChild(parent);

    // FIXME: separate function for pos&size calculations that only get called on parent changed?
    // necessary so a newly-created guiobject's Absolute* values are accurate before render
    auto parent_absolute_position = is_storage_child ? Vector2Zero : getInstanceValue<Vector2>(parent, "AbsolutePosition");
    auto parent_absolute_size = is_storage_child ? rbxCamera::screen_size : getInstanceValue<Vector2>(parent, "AbsoluteSize");
    auto parent_absolute_rotation = is_storage_child ? 0.0f : getInstanceValue<float>(parent, "AbsoluteRotation");

    auto& position = getInstanceValue<UDim2>(instance, "Position");
    auto& size = getInstanceValue<UDim2>(instance, "Size");
    auto rotation = getInstanceValue<float>(instance, "Rotation");

    auto position_x_scale = position.x.scale;
    auto position_y_scale = position.y.scale;

    Vector2 absolute_position{
        parent_absolute_position.x + parent_absolute_size.x * position_x_scale + position.x.offset,
        parent_absolute_position.y + parent_absolute_size.y * position_y_scale + position.y.offset
    };
    Vector2 absolute_size{
        parent_absolute_size.x * size.x.scale + size.x.offset,
        parent_absolute_size.y * size.y.scale + size.y.offset
    };

    float absolute_rotation = parent_absolute_rotation + rotation;

    setInstanceValue<Vector2>(instance, L, "AbsolutePosition", absolute_position);
    setInstanceValue<Vector2>(instance, L, "AbsoluteSize", absolute_size);
    setInstanceValue<float>(instance, L, "AbsoluteRotation", absolute_rotation);

    if (!getInstanceValue<bool>(instance, "Visible"))
        return;

    auto background_color = getInstanceValue<Color>(instance, "BackgroundColor3");
    {
        auto position = auto_button_color_map.find(instance.get());
        if (position != auto_button_color_map.end() && position->second) {
            // TODO: hopefully optimize this
            const auto& hsv = ColorToHSV(background_color);
            background_color = ColorFromHSV(hsv.x, hsv.y, hsv.z / AUTO_BUTTON_COLOR_V);
        }
    }

    background_color.a = (1 - getInstanceValue<float>(instance, "BackgroundTransparency")) * 255;

    Rectangle shape_rect{
        .x = absolute_position.x + absolute_size.x / 2.f,
        .y = absolute_position.y + absolute_size.y / 2.f,
        .width = absolute_size.x,
        .height = absolute_size.y,
    };
    Vector2 shape_origin{absolute_size.x / 2.f, absolute_size.y / 2.f};
    DrawRectanglePro(shape_rect, shape_origin, absolute_rotation, background_color);

    auto border_size = getInstanceValue<int>(instance, "BorderSizePixel");
    if (border_size) {
        auto border_color = getInstanceValue<Color>(instance, "BorderColor3");
        border_color.a = background_color.a;

        DrawRotatedRectangleLines(absolute_position, absolute_size, rotation, border_size, border_color);
    }

    auto shape_lines = getRectangleLinesPro(shape_rect, shape_origin, absolute_rotation);
    const bool is_mouse_over = !anyImGui && CheckCollisionPointPoly(mouse, shape_lines.data(), shape_lines.size());

    auto& mouse_over_state = mouse_over_map[instance.get()];

    if (is_mouse_over) {
        next_gui_objects_hovered.push_back(instance);
        if (instance->isA("GuiButton"))
            next_clickable_instance = instance;
    }

    if (is_mouse_over != mouse_over_state)
        (is_mouse_over ? mouse_enter_list : mouse_leave_list).push_back(instance);

    mouse_over_state = is_mouse_over;
}

std::vector<std::shared_ptr<rbxInstance>> render_list;
void contributeToRenderList(std::shared_ptr<rbxInstance> instance, bool is_storage = false) {
    if (!is_storage) {
        if (instance->isA("LayerCollector")) {
            if (!getInstanceValue<bool>(instance, "Enabled"))
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

void fireMouseMovementSignal(lua_State* L, Vector2& mouse, std::shared_ptr<rbxInstance> instance, const char* event) {
    pushFunctionFromLookup(L, fireRBXScriptSignal);
    instance->pushEvent(L, event);

    lua_pushnumber(L, mouse.x);
    lua_pushnumber(L, mouse.y);

    lua_call(L, 3, 0);
}

void rbxInstance_BasePlayerGui_process(lua_State *L, bool anyImGui) {
    next_clickable_instance.reset();
    next_gui_objects_hovered.clear();

    mouse_enter_list.clear();
    mouse_leave_list.clear();

    // generate render list

    render_list.clear();

    for (size_t i = 0; i < gui_storage_list.size(); i++)
        contributeToRenderList(gui_storage_list[i], true);

    // TODO: use a std::set if stable sort is still possible (see drawingimmediate)
    std::stable_sort(render_list.begin(), render_list.end(), [] (std::shared_ptr<rbxInstance> a, std::shared_ptr<rbxInstance> b) {
        return getInstanceValue<int>(a, "ZIndex") < getInstanceValue<int>(b, "ZIndex");
    });

    auto mouse = GetMousePosition();

    // render objects
    for (size_t i = 0; i < render_list.size(); i++)
        renderGuiObject(L, render_list[i], mouse, anyImGui);

    clickable_instance = next_clickable_instance;
    gui_objects_hovered = next_gui_objects_hovered;

    // handle some signals (the rest are handled in UserInputService)

    for (size_t i = 0; i < mouse_enter_list.size(); i++) {
        auto& instance = mouse_enter_list[i];
        fireMouseMovementSignal(L, mouse, instance, "MouseEnter");
        UserInputService::signalMouseMovement(instance, InputBegan);
        handleGuiButtonMouseEnter(instance);
    }

    for (size_t i = 0; i < mouse_leave_list.size(); i++) {
        auto& instance = mouse_leave_list[i];
        fireMouseMovementSignal(L, mouse, instance, "MouseLeave");
        UserInputService::signalMouseMovement(instance, InputEnded);
        handleGuiButtonMouseLeave(instance);
    }
}

void rbxInstance_BasePlayerGui_init(lua_State *L, std::initializer_list<std::shared_ptr<rbxInstance>> initial_gui_storage_list) {
    gui_storage_list.insert(gui_storage_list.end(), initial_gui_storage_list.begin(), initial_gui_storage_list.end());
}

};
