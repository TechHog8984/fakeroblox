#pragma once

#include "raylib.h"
#include <string_view>

namespace fakeroblox {

extern Shader round_shader;

inline void drawingDrawLine(Vector2* from, Vector2* to, Color* color, float thickness) {
    DrawLineEx(*from, *to, thickness, *color);
}

inline void drawingDrawCircle(Vector2* center, float radius, Color* color, int num_sides, float thickness, bool filled) {
    if (num_sides) {
        DrawPolyLinesEx(*center, num_sides, radius, 0, thickness, *color);
        if (filled)
            DrawPoly(*center, num_sides, radius, 0, *color);
    } else {
        DrawRing(*center, radius - thickness, radius, 0.f, 360.f, 64, *color);
        if (filled)
            DrawCircleV(*center, radius, *color);
    }
}

inline void drawingDrawTriangle(Vector2* pointa, Vector2* pointb, Vector2* pointc, Color* color, float thickness, bool filled) {
    // DrawTriangle* doesn't support thickness, so we use DrawLineEx
    DrawLineEx(*pointa, *pointb, thickness, *color);
    DrawLineEx(*pointb, *pointc, thickness, *color);
    DrawLineEx(*pointc, *pointa, thickness, *color);
    if (filled)
        DrawTriangle(*pointc, *pointb, *pointa, *color);
}

inline void drawingDrawRectangle(Rectangle* rect, Color* color, float rounding, float thickness, bool filled) {
    DrawRectangleRoundedLinesEx(*rect, rounding, 4, thickness, *color);
    if (filled)
        DrawRectangleRounded(*rect, rounding, 4, *color);
}

inline void drawingDrawQuad(Vector2* pointa, Vector2* pointb, Vector2* pointc, Vector2* pointd, Color* color, float thickness, bool filled) {
    // DrawTriangle* doesn't support thickness, so we use DrawLineEx
    DrawLineEx(*pointa, *pointb, thickness, *color);
    DrawLineEx(*pointb, *pointc, thickness, *color);
    DrawLineEx(*pointc, *pointd, thickness, *color);
    DrawLineEx(*pointd, *pointa, thickness, *color);
    if (filled) {
        DrawTriangle(*pointc, *pointb, *pointa, *color);
        DrawTriangle(*pointd, *pointc, *pointa, *color);
    }
}

inline void drawingDrawText(Vector2* position, Font* font, float text_size, Color* color, bool outlined, Color* outline_color, std::string_view text) {
    if (outlined) {
        // top
        DrawTextEx(*font, text.data(), { .x = position->x, .y = position->y - 1 }, text_size, 0, *outline_color);
        // right
        DrawTextEx(*font, text.data(), { .x = position->x + 1, .y = position->y }, text_size, 0, *outline_color);
        // bottom
        DrawTextEx(*font, text.data(), { .x = position->x - 1, .y = position->y + 1 }, text_size, 0, *outline_color);
        // left
        DrawTextEx(*font, text.data(), { .x = position->x - 1, .y = position->y }, text_size, 0, *outline_color);
    }
    DrawTextEx(*font, text.data(), *position, text_size, 0, *color);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function" // FIXME: I believe this is a side effect of using mate...

inline void drawingDrawCenteredText(Vector2* position, Font* font, float text_size, Color* color, bool outlined, Color* outline_color, std::string_view text) {
      auto new_position = *position;
      auto text_bounds = MeasureTextEx(*font, text.data(), text_size, 0);
      new_position.x -= text_bounds.x / 2.f;
      new_position.y -= text_bounds.y / 2.f;

      drawingDrawText(&new_position, font, text_size, color, outlined, outline_color, text);
}

#pragma GCC diagnostic pop

}; // namespace fakeroblox
