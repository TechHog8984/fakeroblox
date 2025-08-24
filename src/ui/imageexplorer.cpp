#include "ui/imageexplorer.hpp"

#include "imageloader.hpp"
#include "imgui.h"

// #define _CRT_SECURE_NO_WARNINGS
// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
#include <GL/gl.h>

#include "lua.h"

namespace fakeroblox {

// // Simple helper function to load an image into a OpenGL texture with common settings
// bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
// {
//     // Load from file
//     int image_width = 0;
//     int image_height = 0;
//     unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
//     if (image_data == NULL)
//         return false;

//     // Create a OpenGL texture identifier
//     GLuint image_texture;
//     glGenTextures(1, &image_texture);
//     glBindTexture(GL_TEXTURE_2D, image_texture);

//     // Setup filtering parameters for display
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

//     // Upload pixels into texture
//     glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
//     stbi_image_free(image_data);

//     *out_texture = image_texture;
//     *out_width = image_width;
//     *out_height = image_height;

//     return true;
// }

// // Open and read a file, then forward to LoadTextureFromMemory()
// bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
// {
//     FILE* f = fopen(file_name, "rb");
//     if (f == NULL)
//         return false;
//     fseek(f, 0, SEEK_END);
//     size_t file_size = (size_t)ftell(f);
//     if (file_size == -1)
//         return false;
//     fseek(f, 0, SEEK_SET);
//     void* file_data = IM_ALLOC(file_size);
//     fread(file_data, 1, file_size, f);
//     fclose(f);
//     bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
//     IM_FREE(file_data);
//     return ret;
// }

Image* chosen_image = nullptr;
void UI_ImageExplorer_render(lua_State *L) {
    ImGui::BeginChild("Image List", ImVec2{ImGui::GetContentRegionAvail().x * 0.35f, ImGui::GetContentRegionAvail().y}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    // FIXME: synchronization
    for (auto& pair : ImageLoader::hash_image_map) {
        bool is_selected = chosen_image && pair.second == chosen_image;
        if (ImGui::Selectable(pair.first.c_str(), is_selected))
            chosen_image = pair.second;
    }

    ImGui::EndChild();

    if (chosen_image) {
        ImGui::SameLine();

        ImGui::BeginChild("Image Display", ImVec2{0, 0}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        int my_image_width = 0;
        int my_image_height = 0;
        GLuint my_image_texture = 0;
        // bool ret = LoadTextureFromFile("../../MyImage01.jpg", &my_image_texture, &my_image_width, &my_image_height);
        // IM_ASSERT(ret);

        auto texture = LoadTextureFromImage(*chosen_image);
        my_image_texture = texture.id;
        my_image_width = texture.width;
        my_image_height = texture.height;
        ImGui::Image((ImTextureID)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));
        UnloadTexture(texture);

        ImGui::EndChild();
    }
}

}; // namespace fakeroblox
