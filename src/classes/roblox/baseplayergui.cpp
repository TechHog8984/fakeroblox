#include "classes/roblox/baseplayergui.hpp"
#include "classes/roblox/camera.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
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
    auto parent = instance->getValue<std::shared_ptr<rbxInstance>>(PROP_INSTANCE_PARENT);
    assert(parent);

    return std::find(gui_storage_list.begin(), gui_storage_list.end(), parent) != gui_storage_list.end();
}
void renderGuiObject(lua_State* L, std::shared_ptr<rbxInstance> instance, Vector2 mouse) {
    auto parent = instance->getValue<std::shared_ptr<rbxInstance>>(PROP_INSTANCE_PARENT);
    const bool is_storage_child = isStorageChild(parent);

    // TODO: separate function for pos&size calculations that only get called on parent changed?
    // above will be nice so a newly-created guiobject's Absolute* values aren't all zero until render
    auto& parent_absolute_position = is_storage_child ? rbxCamera::screen_size : parent->getValue<Vector2>("AbsolutePosition");
    auto& parent_absolute_size = is_storage_child ? rbxCamera::screen_size : parent->getValue<Vector2>("AbsoluteSize");
    auto parent_absolute_rotation = is_storage_child ? 0.0f : parent->getValue<float>("AbsoluteRotation");

    auto& position = instance->getValue<UDim2>("Position");
    auto& size = instance->getValue<UDim2>("Size");
    auto rotation = instance->getValue<float>("Rotation");

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

    float absolute_rotation = parent_absolute_rotation + rotation;

    instance->setValue<Vector2>(L, "AbsolutePosition", absolute_position);
    instance->setValue<Vector2>(L, "AbsoluteSize", absolute_size);
    instance->setValue<float>(L, "AbsoluteRotation", absolute_rotation);

    if (!instance->getValue<bool>("Visible"))
        return;

    auto background_color = instance->getValue<Color>("BackgroundColor3");
    background_color.a = (1 - instance->getValue<float>("BackgroundTransparency")) * 255;

    Rectangle shape_rect{
        .x = absolute_position.x + absolute_size.x / 2.f,
        .y = absolute_position.y + absolute_size.y / 2.f,
        .width = absolute_size.x,
        .height = absolute_size.y,
    };
    Vector2 shape_origin{absolute_size.x / 2.f, absolute_size.y / 2.f};
    DrawRectanglePro(shape_rect, shape_origin, absolute_rotation, background_color);

    auto border_size = instance->getValue<int>("BorderSizePixel");
    if (border_size) {
        auto border_color = instance->getValue<Color>("BorderColor3");
        border_color.a = background_color.a;

        DrawRotatedRectangleLines(absolute_position, absolute_size, rotation, border_size, border_color);
    }

    auto shape_lines = getRectangleLinesPro(shape_rect, shape_origin, absolute_rotation);
    const bool is_mouse_over = CheckCollisionPointPoly(mouse, shape_lines.data(), shape_lines.size());

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

void fireMouseMovementSignal(lua_State* L, Vector2& mouse, std::shared_ptr<rbxInstance> instance, const char* event) {
    pushFunctionFromLookup(L, fireRBXScriptSignal);
    instance->pushEvent(L, event);

    lua_pushnumber(L, mouse.x);
    lua_pushnumber(L, mouse.y);

    lua_call(L, 3, 0);
}

void rbxInstance_BasePlayerGui_process(lua_State *L) {
    next_clickable_instance.reset();
    next_gui_objects_hovered.clear();

    mouse_enter_list.clear();
    mouse_leave_list.clear();

    // generate render list

    render_list.clear();

    for (size_t i = 0; i < gui_storage_list.size(); i++)
        contributeToRenderList(gui_storage_list[i], true);

    std::sort(render_list.begin(), render_list.end(), [] (std::shared_ptr<rbxInstance> a, std::shared_ptr<rbxInstance> b) {
        return a->getValue<int>("ZIndex") < b->getValue<int>("ZIndex");
    });

    auto mouse = GetMousePosition();

    // render objects
    for (size_t i = 0; i < render_list.size(); i++)
        renderGuiObject(L, render_list[i], mouse);

    clickable_instance = next_clickable_instance;
    gui_objects_hovered = next_gui_objects_hovered;

    // handle some signals (the rest are handled in UserInputService)

    for (size_t i = 0; i < mouse_enter_list.size(); i++) {
        auto& instance = mouse_enter_list[i];
        fireMouseMovementSignal(L, mouse, instance, "MouseEnter");
        UserInputService::signalMouseMovement(instance, InputBegan);
    }

    for (size_t i = 0; i < mouse_leave_list.size(); i++) {
        auto& instance = mouse_leave_list[i];
        fireMouseMovementSignal(L, mouse, instance, "MouseLeave");
        UserInputService::signalMouseMovement(instance, InputEnded);
    }
}

void rbxInstance_BasePlayerGui_init(lua_State *L, std::initializer_list<std::shared_ptr<rbxInstance>> initial_gui_storage_list) {
    gui_storage_list.insert(gui_storage_list.end(), initial_gui_storage_list.begin(), initial_gui_storage_list.end());
}

};
