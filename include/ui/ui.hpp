#pragma once

#include "classes/udim.hpp"
#include "classes/udim2.hpp"

#include "imgui.h"
#include "raylib.h"

#include <string>

namespace fakeroblox {

// window items
extern bool show_fps;
extern bool menu_editor_open;
extern bool menu_console_open;
extern bool menu_tests_open;
extern bool menu_thread_list_open;
extern bool menu_drawentry_list_open;
extern bool menu_instance_explorer_open;
extern bool menu_function_explorer_open;

extern bool enable_user_input_service;
extern bool enable_run_service;
extern bool enable_tween_service;
extern bool menu_image_explorer_open;
extern bool menu_table_explorer_open;

int imgui_inputTextCallback(ImGuiInputTextCallbackData* data);

bool ImGui_STDString(const char* label, std::string& string);

void ImGui_Color3(const char* name, Color& color);
void ImGui_Color4(const char* name, Color& color);

void ImGui_DragVector2(const char* name, Vector2& vector2, float speed = 0.6f, float min = 0.f, float max = 3000.f);
void ImGui_DragVector3(const char* name, Vector3& vector1, float speed = 0.6f, float min = 0.f, float max = 3000.f);

void ImGui_DragUDim(const char* name, UDim& udim, float speed = 0.6f, float min = 0.f, float max = 3000.f);
void ImGui_DragUDim2(const char* name, UDim2& udim2, float speed = 0.6f, float min = 0.f, float max = 3000.f);

}; // namespace fakeroblox
