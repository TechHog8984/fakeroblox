#include "ui/imageexplorer.hpp"

#include "imageloader.hpp"
#include "imgui.h"

#include <GL/gl.h>
#include "raylib.h"

#include "lua.h"

namespace fakeroblox {

Image* chosen_image = nullptr;
bool texture_is_loaded = false;
Texture2D* loaded_texture = new Texture2D();
void UI_ImageExplorer_render(lua_State *L) {
    ImGui::BeginChild("Image List", ImVec2{ImGui::GetContentRegionAvail().x * 0.35f, ImGui::GetContentRegionAvail().y}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    const Image* old_chosen_image = chosen_image;
    // FIXME: synchronization
    for (auto& pair : ImageLoader::hash_image_map) {
        bool is_selected = chosen_image && pair.second == chosen_image;
        if (ImGui::Selectable(pair.first.c_str(), is_selected))
            chosen_image = pair.second;
    }

    if (old_chosen_image != chosen_image) {
        UnloadTexture(*loaded_texture);
        texture_is_loaded = false;
    }

    ImGui::EndChild();

    if (chosen_image) {
        ImGui::SameLine();

        ImGui::BeginChild("Image Display", ImVec2{0, 0}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        int my_image_width = 0;
        int my_image_height = 0;
        GLuint my_image_texture = 0;

        if (!texture_is_loaded) {
            *loaded_texture = LoadTextureFromImage(*chosen_image);
            texture_is_loaded = true;
        }
        my_image_texture = (*loaded_texture).id;
        my_image_width = (*loaded_texture).width;
        my_image_height = (*loaded_texture).height;

        ImGui::Image((ImTextureID)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));

        ImGui::EndChild();
    }
}

void UI_ImageExplorer_cleanup() {
    if (texture_is_loaded)
        UnloadTexture(*loaded_texture);
}

}; // namespace fakeroblox
