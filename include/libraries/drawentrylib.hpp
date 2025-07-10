#pragma once

#include <raylib.h>
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
    Color color;

    DrawEntry(Type type, const char* class_name);
};

class DrawEntryLine : public DrawEntry {
public:
    float thickness = 1; // TODO: verify default value
    Vector2 from;
    Vector2 to;

    DrawEntryLine();
};

void open_drawentrylib(lua_State* L);

}; // namespace fakeroblox
