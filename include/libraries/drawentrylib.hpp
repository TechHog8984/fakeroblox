#pragma once

#include <raylib.h>
#include <string>
#include <vector>

#include "lua.h"

namespace fakeroblox {

class DrawEntry {
public:
    static std::vector<DrawEntry*> draw_list;

    enum Type {
      Line,
      Text,
      Image,
      Circle,
      Square,
      Triangle,
      Quad
    } type;
    const char* class_name = "[DrawEntry]";

    bool alive = true;

    bool visible = false;
    int zindex = 0; // TODO: verify default value
    Color color{255, 255, 255};

    void free();

    DrawEntry(Type type, const char* class_name);
};

class DrawEntryLine : public DrawEntry {
public:
    double thickness = 1;
    Vector2 from{0, 0};
    Vector2 to{0, 0};

    DrawEntryLine();
};

class DrawEntryText : public DrawEntry {
public:
    std::string text = "";
    Vector2 text_bounds{0, 0};
    double text_size = 20;

    enum FontEnum {
        Default,     // use whatever raylib's default font is
        Custom,      // supply ttf via custom_font_data
        FONT__COUNT  // do not use!
    } font_enum;
    std::string custom_font_data = "";
    Font font = GetFontDefault();

    bool centered = false;
    bool outlined = false;
    Color outline_color{0, 0, 0};
    Vector2 position{0, 0};

    Vector2 outline_position{1, 1};
    double outline_text_size = 22;

    DrawEntryText();

    void updateTextBounds();
    void updateFont();
    void updateOutline();
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

void open_drawentrylib(lua_State* L);

}; // namespace fakeroblox
