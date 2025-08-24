#pragma once

#include "raylib.h"

#include <map>
#include <string>
#include <vector>

#include "lua.h"

namespace fakeroblox {

class FontLoader {
public:
    static constexpr const char* SUPPORTED_FONT_TYPE_STRING = "expected ttf or otf";
    static lua_State* L;

    static size_t font_count;
    static std::vector<Font*> font_list;
    static std::map<std::string, size_t> hash_font_map;
    static std::vector<std::string> font_name_list;

    static void load();
    static void unload();

    static const char* getFontType(unsigned char* data, int data_size);
    static size_t getFont(unsigned char* data, int data_size);
};

}; // namespace fakeroblox
