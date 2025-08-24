#include "fontloader.hpp"
#include "common.hpp"

namespace fakeroblox {

lua_State* FontLoader::L = nullptr;
size_t FontLoader::font_count = 5;
std::vector<Font*> FontLoader::font_list = {};
std::map<std::string, size_t> FontLoader::hash_font_map;
std::vector<std::string> FontLoader::font_name_list;

void FontLoader::load() {
    // NOTE: L will not be initialized yet

    font_list.reserve(font_count);
    font_name_list.reserve(font_count);

    Font font_default = GetFontDefault();
    Font font_ui = LoadFont("assets/Segoe UI.ttf");
    Font font_proggy = LoadFont("assets/ProggyClean.ttf");
    Font font_plex = LoadFont("assets/IBMPlexSans.ttf");
    Font font_monospace = LoadFont("assets/SometypeMono.ttf");

    font_list.push_back(new Font(font_default));
    font_list.push_back(new Font(font_ui));
    font_list.push_back(new Font(font_proggy));
    font_list.push_back(new Font(font_plex));
    font_list.push_back(new Font(font_monospace));

    font_name_list.push_back("Default");
    font_name_list.push_back("UI");
    font_name_list.push_back("System");
    font_name_list.push_back("Plex");
    font_name_list.push_back("Monospace");
}
void FontLoader::unload() {
    font_name_list.clear();

    for (unsigned int i = 0; i < font_list.size(); i++) {
        UnloadFont(*font_list[i]);
        delete font_list[i];
    }
    font_list.clear();

    hash_font_map.clear();
}


static bool checkSignatureTTF(const unsigned char* data, int data_size) {
    if (data_size < 4)
        return false;

    return data[0] == 0x00 && data[1] == 0x01 &&
        data[2] == 0x00 && data[3] == 0x00;
}

static bool checkSignatureOTF(const unsigned char* data, int data_size) {
    if (data_size < 4)
        return false;

    return data[0] == 'O' && data[1] == 'T' &&
        data[2] == 'T' && data[3] == 'O';
}

const char* FontLoader::getFontType(unsigned char* data, int data_size) {
    if (checkSignatureTTF(data, data_size))
        return ".ttf";
    if (checkSignatureOTF(data, data_size))
        return ".otf";
    return nullptr;
}

size_t FontLoader::getFont(unsigned char *data, int data_size) {
    const char* file_extension = getFontType(data, data_size);
    assert(file_extension);

    std::string hashed = sha1ToString(ComputeSHA1(data, data_size));

    auto cached = hash_font_map.find(hashed);
    if (cached != hash_font_map.end())
        return cached->second;

    size_t index = font_count++;

    Font* font = new Font(LoadFontFromMemory(file_extension, data, data_size, 256, nullptr, 0));

    font_list.push_back(font);
    hash_font_map[hashed] = index;
    font_name_list.push_back(hashed);

    lua_getglobal(L, "Drawing");
    lua_getfield(L, -1, "Fonts");
    lua_pushunsigned(L, index);
    lua_setfield(L, -2, hashed.c_str());

    return index;
}

}; // namespace fakerobox
