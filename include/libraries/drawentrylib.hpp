#pragma once

#include <shared_mutex>
#include <string>
#include <vector>

#include "raylib.h"

#include "lua.h"

namespace fakeroblox {

class DrawEntry {
public:
    static std::vector<DrawEntry*> draw_list;
    static std::shared_mutex draw_list_mutex;

    static void clear(lua_State* L);
    static void render();

    enum Type {
        DrawTypeLine,
        DrawTypeText,
        DrawTypeImage,
        DrawTypeCircle,
        DrawTypeSquare,
        DrawTypeTriangle,
        DrawTypeQuad
    } type;
    const char* class_name = "[DrawEntry]";

    int lookup_index;
    bool alive = true;

    std::shared_mutex members_mutex;

    bool visible = false;
    int zindex = 0; // TODO: verify default value
    Color color{255, 255, 255};

    void onZIndexUpdate();
    void free();
    void destroy(lua_State* L, bool dont_erase = false);
    DrawEntry* clone(lua_State* L);

    DrawEntry(Type type, const char* class_name);
};

class DrawEntryLine : public DrawEntry {
public:
    double thickness = 1;
    Vector2 from{0, 0};
    Vector2 to{0, 0};

    DrawEntryLine();
};

enum FontEnum {
    FontDefault,     // use whatever raylib's default font is

    FontUI,          // Segoe UI 
    FontSystem,      // ProggyClean
    FontPlex,        // IBMPlexSans
    FontMonospace,   // SometypeMono

    FONT__COUNT      // do not use!
};

class DrawEntryText : public DrawEntry {
public:
    static size_t default_font;

    std::string text = "";
    Vector2 text_bounds{0, 0};
    double text_size = 20;

    int font_index = FontDefault;
    std::string custom_font_data = "";
    Font* font = nullptr;

    bool centered = false;
    bool outlined = false;
    Color outline_color{0, 0, 0, 255};
    Vector2 position{0, 0};

    DrawEntryText();

    void updateTextBounds();
    void updateFont();
    void updateCustomFont();
    void updateOutline();
};

class DrawEntryImage : public DrawEntry {
public:
    Image* image;
    Texture2D texture;
    RenderTexture2D mask = {};

    std::string data = "";
    Vector2 image_size{0, 0};

    Vector2 size{0, 0};
    Vector2 position{0, 0};
    double rounding = 0;

    DrawEntryImage();
    ~DrawEntryImage();

    void updateData();
    void resizeImage();
    void updateRounding();
};

class DrawEntryCircle : public DrawEntry {
public:
    double thickness = 1;
    int num_sides = 0;
    double radius = 0;
    bool filled = true;
    Vector2 center{0, 0};

    DrawEntryCircle();
};

class DrawEntrySquare : public DrawEntry {
public:
    double thickness = 1;
    Rectangle rect{0, 0, 0, 0};
    bool filled = true;

    DrawEntrySquare();
};

class DrawEntryTriangle : public DrawEntry {
public:
    double thickness = 1;
    Vector2 pointa{0, 0};
    Vector2 pointb{0, 0};
    Vector2 pointc{0, 0};
    bool filled = true;

    DrawEntryTriangle();
};
class DrawEntryQuad : public DrawEntry {
public:
    double thickness = 1;
    Vector2 pointa{0, 0};
    Vector2 pointb{0, 0};
    Vector2 pointc{0, 0};
    Vector2 pointd{0, 0};
    bool filled = true;

    DrawEntryQuad();
};

DrawEntry* pushNewDrawEntry(lua_State* L, const char* class_name);

int DrawEntry__index(lua_State* L);
int DrawEntry__newindex(lua_State* L);
void open_drawentrylib(lua_State* L);

}; // namespace fakeroblox
