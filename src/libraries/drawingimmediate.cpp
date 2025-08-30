#include "libraries/drawingimmediate.hpp"
#include "basedrawing.hpp"
#include "classes/color3.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"

#include "classes/vector2.hpp"
#include "common.hpp"

#include "fontloader.hpp"
#include "lua.h"
#include "lualib.h"

#include <set>
#include <unordered_set>

namespace fakeroblox {

struct PaintEvent {
    int zindex;
    int ref;
};

struct PaintEventCompare {
    bool operator()(const PaintEvent& a, const PaintEvent& b) const {
        return a.zindex < b.zindex;
    }
};

std::set<PaintEvent, PaintEventCompare> paint_event_set;

namespace drawingimmediate_methods {
    // TODO: determine if it's safe to just modify alpha directly (instead of making a copy)
    static int line(lua_State* L) {
        auto from = lua_checkvector2(L, 1);
        auto to = lua_checkvector2(L, 2);
        auto color = *lua_checkcolor(L, 3);
        double opacity = luaL_checknumber(L, 4);
        double thickness = luaL_checknumber(L, 5);

        color.a = opacity * 255.f;

        drawingDrawLine(from, to, &color, thickness);

        return 0;
    }

    static void _circle(lua_State* L, bool filled) {
        auto center = lua_checkvector2(L, 1);
        double radius = luaL_checknumber(L, 2);
        auto color = *lua_checkcolor(L, 3);
        double opacity = luaL_checknumber(L, 4);
        int num_sides = luaL_checkinteger(L, 5);
        double thickness = luaL_checknumber(L, 6);

        color.a = opacity * 255.f;

        drawingDrawCircle(center, radius, &color, num_sides, thickness, filled);
    }
    static int circle(lua_State* L) {
        _circle(L, false);

        return 0;
    }
    static int filledCircle(lua_State* L) {
        _circle(L, true);

        return 0;
    }

    static void _triangle(lua_State* L, bool filled) {
        auto pointa = lua_checkvector2(L, 1);
        auto pointb = lua_checkvector2(L, 2);
        auto pointc = lua_checkvector2(L, 3);
        auto color = *lua_checkcolor(L, 4);
        double opacity = luaL_checknumber(L, 5);
        double thickness = luaL_checknumber(L, 6);

        color.a = opacity * 255.f;

        drawingDrawTriangle(pointa, pointb, pointc, &color, thickness, filled);
    }
    static int triangle(lua_State* L) {
        _triangle(L, false);

        return 0;
    }
    static int filledTriangle(lua_State* L) {
        _triangle(L, true);

        return 0;
    }

    static void _rectangle(lua_State* L, bool filled) {
        auto top_left = lua_checkvector2(L, 1);
        auto size = lua_checkvector2(L, 2);
        auto color = *lua_checkcolor(L, 3);
        double opacity = luaL_checknumber(L, 4);
        double rounding = luaL_checknumber(L, 5);
        double thickness = luaL_checknumber(L, 6);

        color.a = opacity * 255.f;

        Rectangle rect { .x = top_left->x, .y = top_left->y, .width = size->x, .height = size->y };
        rect.x = top_left->x;
        rect.y = top_left->y;
        rect.width = top_left->x;
        rect.height = top_left->y;

        drawingDrawRectangle(&rect, &color, rounding, thickness, filled);
    }
    static int rectangle(lua_State* L) {
        _rectangle(L, false);

        return 0;
    }
    static int filledRectangle(lua_State* L) {
        _rectangle(L, true);

        return 0;
    }

    static void _quad(lua_State* L, bool filled) {
        auto pointa = lua_checkvector2(L, 1);
        auto pointb = lua_checkvector2(L, 2);
        auto pointc = lua_checkvector2(L, 3);
        auto pointd = lua_checkvector2(L, 4);
        auto color = *lua_checkcolor(L, 5);
        double opacity = luaL_checknumber(L, 6);
        double thickness = luaL_checknumber(L, 7);

        color.a = opacity * 255.f;

        drawingDrawQuad(pointa, pointb, pointc, pointd, &color, thickness, filled);
    }
    static int quad(lua_State* L) {
        _quad(L, false);

        return 0;
    }
    static int filledQuad(lua_State* L) {
        _quad(L, true);

        return 0;
    }

    static void _text(lua_State* L, bool outlined) {
        const int arg_offset = outlined * 2;

        auto position = lua_checkvector2(L, 1);
        int font = luaL_checkinteger(L, 2);
        auto text_size = luaL_checknumber(L, 3);
        auto color = *lua_checkcolor(L, 4);
        double opacity = luaL_checknumber(L, 5);

        Color outline_color;
        if (outlined) {
            outline_color = *lua_checkcolor(L, 6);
            double outline_opacity = luaL_checknumber(L, 7);

            outline_color.a = outline_opacity * 255.f;
        }

        size_t strl;
        const char* str = luaL_checklstring(L, 6 + arg_offset, &strl);
        bool centered = luaL_checkboolean(L, 7 + arg_offset);

        color.a = opacity * 255.f;

        if (centered)
            drawingDrawCenteredText(position, FontLoader::font_list[font], text_size, &color, outlined, &outline_color, std::string_view(str, strl));
        else
            drawingDrawText(position, FontLoader::font_list[font], text_size, &color, outlined, &outline_color, std::string_view(str, strl));
    }
    static int text(lua_State* L) {
        _text(L, false);

        return 0;
    }
    static int outlinedText(lua_State* L) {
        _text(L, true);

        return 0;
    }

    static int getPaint(lua_State* L) {
        const int zindex = luaL_checkinteger(L, 1);

        auto it = paint_event_set.find(PaintEvent{zindex});
        if (it == paint_event_set.end()) {
            pushNewRBXScriptSignal(L, "paint");
            int ref = lua_ref(L, -1);

            paint_event_set.insert({ zindex, ref });
        } else
            lua_rawgeti(L, LUA_REGISTRYINDEX, it->ref);

        return 1;
    }
}; // namespace drawingimmediate_methods

void open_drawingimmediate(lua_State* L) {
    // DrawingImmediate global
    lua_newtable(L);

    setfunctionfield(L, drawingimmediate_methods::line, "Line");

    setfunctionfield(L, drawingimmediate_methods::circle, "Circle");
    setfunctionfield(L, drawingimmediate_methods::filledCircle, "FilledCircle");

    setfunctionfield(L, drawingimmediate_methods::triangle, "Triangle");
    setfunctionfield(L, drawingimmediate_methods::filledTriangle, "FilledTriangle");

    setfunctionfield(L, drawingimmediate_methods::rectangle, "Rectangle");
    setfunctionfield(L, drawingimmediate_methods::filledRectangle, "FilledRectangle");

    setfunctionfield(L, drawingimmediate_methods::quad, "Quad");
    setfunctionfield(L, drawingimmediate_methods::filledQuad, "FilledQuad");

    setfunctionfield(L, drawingimmediate_methods::text, "Text");
    setfunctionfield(L, drawingimmediate_methods::outlinedText, "OutlinedText");

    setfunctionfield(L, drawingimmediate_methods::getPaint, "GetPaint");

    lua_setglobal(L, "DrawingImmediate");
}

void render_drawingimmediate(lua_State* L) {
    for (const auto& event : paint_event_set) {
        pushFunctionFromLookup(L, fireRBXScriptSignal);
        lua_rawgeti(L, LUA_REGISTRYINDEX, event.ref);
        lua_call(L, 1, 0);
    }
}

}; // namespace fakeroblox
