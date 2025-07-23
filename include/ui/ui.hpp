#pragma once

#include "classes/udim.hpp"
#include "classes/udim2.hpp"

#include "imgui.h"
#include "raylib.h"

#include <string>

namespace fakeroblox {

int imgui_inputTextCallback(ImGuiInputTextCallbackData* data);

bool ImGui_STDString(const char* label, std::string& string);

void ImGui_Color3(const char* name, Color& color);
void ImGui_Color4(const char* name, Color& color);

void ImGui_DragVector2(const char* name, Vector2& vector2, float speed = 0.6f, float min = 0.f, float max = 3000.f);
void ImGui_DragVector3(const char* name, Vector3& vector1, float speed = 0.6f, float min = 0.f, float max = 3000.f);

void ImGui_DragUDim(const char* name, UDim& udim, float speed = 0.6f, float min = 0.f, float max = 3000.f);
void ImGui_DragUDim2(const char* name, UDim2& udim2, float speed = 0.6f, float min = 0.f, float max = 3000.f);

}; // namespace fakeroblox
