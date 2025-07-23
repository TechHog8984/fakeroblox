#include "ui/ui.hpp"

#include <string>

namespace fakeroblox {

int imgui_inputTextCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        std::string* str = (std::string*) data->UserData;
        str->resize(data->BufTextLen);
        data->Buf = (char*) str->c_str();
    }
    return 0;
}

bool ImGui_STDString(const char* label, std::string& string) {
    return ImGui::InputText(
        label,
        &string[0],
        string.capacity() + 1,
        ImGuiInputTextFlags_CallbackResize,
        imgui_inputTextCallback,
        (void*) &string
    );
}

void ImGui_Color3(const char* name, Color& color) {
    float v[3] = {
        color.r / 255.f,
        color.g / 255.f,
        color.b / 255.f
    };
    ImGui::ColorEdit3(name, v);
    color.r = v[0] * 255.f;
    color.g = v[1] * 255.f;
    color.b = v[2] * 255.f;
}
void ImGui_Color4(const char* name, Color& color) {
    float v[4] = {
        color.r / 255.f,
        color.g / 255.f,
        color.b / 255.f,
        color.a / 255.f
    };
    ImGui::ColorEdit4(name, v);
    color.r = v[0] * 255.f;
    color.g = v[1] * 255.f;
    color.b = v[2] * 255.f;
    color.a = v[3] * 255.f;
}

void ImGui_DragVector2(const char* name, Vector2& vector2, float speed, float min, float max) {
    float v[2] = { vector2.x, vector2.y };
    ImGui::DragScalarN(name, ImGuiDataType_Float, v, 2, speed, &min, &max);
    vector2.x = v[0];
    vector2.y = v[1];
}
void ImGui_DragVector3(const char* name, Vector3& vector3, float speed, float min, float max) {
    float v[3] = { vector3.x, vector3.y, vector3.z };
    ImGui::DragScalarN(name, ImGuiDataType_Float, v, 3, speed, &min, &max);
    vector3.x = v[0];
    vector3.y = v[1];
    vector3.z = v[2];
}

void ImGui_DragUDim(const char* name, UDim& udim, float speed, float min, float max) {
    float v[2] = { udim.scale, udim.offset };
    ImGui::DragScalarN(name, ImGuiDataType_Float, v, 2, speed, &min, &max);
    udim.scale = v[0];
    udim.offset = v[1];
}
void ImGui_DragUDim2(const char* name, UDim2& udim2, float speed, float min, float max) {
    float v[4] = { udim2.x.scale, udim2.x.offset, udim2.y.scale, udim2.y.offset };
    ImGui::DragScalarN(name, ImGuiDataType_Float, v, 4, speed, &min, &max);
    udim2.x.scale = v[0];
    udim2.x.offset = v[1];
    udim2.y.scale = v[2];
    udim2.y.offset = v[3];
}

}; // namespace fakeroblox
