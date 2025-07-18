#include "libraries/drawentrylib.hpp"
#include "classes/color3.hpp"
#include "classes/vector2.hpp"
#include "common.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

std::vector<DrawEntry*> DrawEntry::draw_list;

DrawEntry::DrawEntry(Type type, const char* class_name) : type(type), class_name(class_name) {}
DrawEntryLine::DrawEntryLine() : DrawEntry(DrawEntry::Line, "Line") {}
DrawEntryText::DrawEntryText() : DrawEntry(DrawEntry::Text, "Text") {}
DrawEntrySquare::DrawEntrySquare() : DrawEntry(DrawEntry::Square, "Square") {}
DrawEntryTriangle::DrawEntryTriangle() : DrawEntry(DrawEntry::Triangle, "Triangle") {}
DrawEntryQuad::DrawEntryQuad() : DrawEntry(DrawEntry::Quad, "Quad") {}

void DrawEntryText::updateTextBounds() {
    text_bounds = MeasureTextEx(font, text.c_str(), text_size, 0);
}
void DrawEntryText::updateFont() {
    switch (font_enum) {
        case Default:
            font = GetFontDefault();
            break;
        case Custom:
            assert(!"TODO: custom DrawEntryText font");
            break;
        default:
            assert(!"TODO: unsupported DrawEntryText::FontEnum type");
    }
    updateTextBounds();
}

// FIXME: text outline
// const float OUTLINE_OFFSET = 2;

void DrawEntryText::updateOutline() {
    // outline_position = Vector2{ position.x + OUTLINE_OFFSET / 2, position.y + OUTLINE_OFFSET / 2 };
    // outline_text_size = text_size + OUTLINE_OFFSET;

    // outline_position = Vector2{ position.x - 1, position.y };
    // outline_text_size = text_size + 1;
}

void DrawEntry::free() {
    switch (this->type) {
        case DrawEntry::Line:
            static_cast<DrawEntryLine*>(this)->~DrawEntryLine();
            break;
        case DrawEntry::Text:
            static_cast<DrawEntryText*>(this)->~DrawEntryText();
            break;
        case DrawEntry::Square:
            static_cast<DrawEntrySquare*>(this)->~DrawEntrySquare();
            break;
        case DrawEntry::Triangle:
            static_cast<DrawEntryTriangle*>(this)->~DrawEntryTriangle();
            break;
        case DrawEntry::Quad:
            static_cast<DrawEntryQuad*>(this)->~DrawEntryQuad();
            break;
        default:
            assert(!"TODO: all DrawEntry types in free");
            break;
    }
}

static int DrawEntry_new(lua_State* L) {
    const char* class_name = luaL_checkstring(L, 1);

    void* ud = nullptr;
    // FIXME: all DrawEntry types
    if (strequal(class_name, "Line")) {
        ud = lua_newuserdata(L, sizeof(DrawEntryLine));
        new(ud) DrawEntryLine();
    } else if (strequal(class_name, "Text")) {
        ud = lua_newuserdata(L, sizeof(DrawEntryText));
        new(ud) DrawEntryText();
    } else if (strequal(class_name, "Square")) {
        ud = lua_newuserdata(L, sizeof(DrawEntrySquare));
        new(ud) DrawEntrySquare();
    } else if (strequal(class_name, "Triangle")) {
        ud = lua_newuserdata(L, sizeof(DrawEntryTriangle));
        new(ud) DrawEntryTriangle();
    } else if (strequal(class_name, "Quad")) {
        ud = lua_newuserdata(L, sizeof(DrawEntryQuad));
        new(ud) DrawEntryQuad();
    } else
        luaL_error(L, "invalid DrawEntry class name '%s'", class_name);

    assert(ud);

    DrawEntry* entry = static_cast<DrawEntry*>(ud);
    entry->color.a = 255;
    // FIXME: use zindex (ideally we sort entire draw_list on any zindex change using a shared lock or something)
    // FIXME: synchronize
    DrawEntry::draw_list.push_back(entry);

    luaL_getmetatable(L, "DrawEntry");
    lua_setmetatable(L, -2);

    return 1;
}

namespace DrawEntry_methods {
    static int remove(lua_State* L) {
        DrawEntry* obj = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));

        if (!obj->alive)
            luaL_error(L, "INTERNAL TODO: determine remove behavior when DrawEntry is already removed");

        // TODO alive index and newindex behavior
        obj->alive = false;

        // FIXME: synchronize
        DrawEntry::draw_list.erase(std::find(DrawEntry::draw_list.begin(), DrawEntry::draw_list.end(), obj));

        obj->free();

        return 0;
    }
};

static int DrawEntry__tostring(lua_State* L) {
    DrawEntry* entry = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));

    lua_pushstring(L, entry->class_name);
    return 1;
}

lua_CFunction getDrawEntryMethod(DrawEntry* entry, const char* key) {
    if (strequal(key, "Remove") || strequal(key, "Destroy"))
        return DrawEntry_methods::remove;

    return nullptr;
}

static int DrawEntry__index(lua_State* L) {
    DrawEntry* entry = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Visible"))
        lua_pushboolean(L, entry->visible);
    else if (strequal(key, "ZIndex"))
        lua_pushinteger(L, entry->zindex);
    else if (strequal(key, "Transparency") || strequal(key, "Opacity"))
        lua_pushnumber(L, entry->color.a / 255.0);
    else if (strequal(key, "Color"))
        pushColor3(L, entry->color);
    else {
        lua_CFunction func = getDrawEntryMethod(entry, key);
        if (func)
            return pushFunctionFromLookup(L, func);

        switch (entry->type) {
            case DrawEntry::Line: {
                DrawEntryLine* entry_line = static_cast<DrawEntryLine*>(entry);
                if (strequal(key, "Thickness"))
                    lua_pushnumber(L, entry_line->thickness);
                else if (strequal(key, "From"))
                    pushVector2(L, entry_line->from);
                else if (strequal(key, "To"))
                    pushVector2(L, entry_line->to);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::Text: {
                DrawEntryText* entry_text = static_cast<DrawEntryText*>(entry);
                if (strequal(key, "Text")) {
                    std::string text = entry_text->text;
                    lua_pushlstring(L, text.c_str(), text.size());
                } else if (strequal(key, "TextBounds"))
                    pushVector2(L, entry_text->text_bounds);
                else if (strequal(key, "Size") || strequal(key, "TextSize") || strequal(key, "FontSize"))
                    lua_pushnumber(L, entry_text->text_size);
                else if (strequal(key, "Font"))
                    lua_pushunsigned(L, entry_text->font_enum);
                else if (strequal(key, "Centered"))
                    lua_pushboolean(L, entry_text->centered);
                else if (strequal(key, "Outlined"))
                    lua_pushboolean(L, entry_text->outlined);
                else if (strequal(key, "OutlineColor"))
                    pushColor3(L, entry_text->outline_color);
                else if (strequal(key, "Position"))
                    pushVector2(L, entry_text->position);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::Square: {
                DrawEntrySquare* entry_square = static_cast<DrawEntrySquare*>(entry);
                if (strequal(key, "Thickness"))
                    lua_pushnumber(L, entry_square->thickness);
                else if (strequal(key, "Size")) {
                    auto rect = entry_square->rect;
                    pushVector2(L, rect.width, rect.height);
                } else if (strequal(key, "Position")) {
                    auto rect = entry_square->rect;
                    pushVector2(L, rect.x, rect.y);
                } else if (strequal(key, "Filled"))
                    lua_pushboolean(L, entry_square->filled);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::Triangle: {
                DrawEntryTriangle* entry_triangle = static_cast<DrawEntryTriangle*>(entry);
                if (strequal(key, "Thickness"))
                    lua_pushnumber(L, entry_triangle->thickness);
                else if (strequal(key, "PointA"))
                    pushVector2(L, entry_triangle->pointa);
                else if (strequal(key, "PointB"))
                    pushVector2(L, entry_triangle->pointb);
                else if (strequal(key, "PointC"))
                    pushVector2(L, entry_triangle->pointc);
                else if (strequal(key, "Filled"))
                    lua_pushboolean(L, entry_triangle->filled);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::Quad: {
                DrawEntryQuad* entry_quad = static_cast<DrawEntryQuad*>(entry);
                if (strequal(key, "Thickness"))
                    lua_pushnumber(L, entry_quad->thickness);
                else if (strequal(key, "PointA"))
                    pushVector2(L, entry_quad->pointa);
                else if (strequal(key, "PointB"))
                    pushVector2(L, entry_quad->pointb);
                else if (strequal(key, "PointC"))
                    pushVector2(L, entry_quad->pointc);
                else if (strequal(key, "PointD"))
                    pushVector2(L, entry_quad->pointd);
                else if (strequal(key, "Filled"))
                    lua_pushboolean(L, entry_quad->filled);
                else
                    goto INVALID;

                break;
            }
            default:
                luaL_errorL(L, "INTERNAL TODO: all DrawEntry types (%s)", entry->class_name);
                break;
        }
    }

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of %s", key, entry->class_name);
}

static int DrawEntry__newindex(lua_State* L) {
    DrawEntry* entry = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Visible"))
        entry->visible = luaL_checkboolean(L, 3);
    else if (strequal(key, "ZIndex"))
        entry->zindex = luaL_checkinteger(L, 3);
    else if (strequal(key, "Transparency") || strequal(key, "Opacity")) {
        double alpha = luaL_checknumberrange(L, 3, 0, 1) * 255.0;
        entry->color.a = alpha;
        if (entry->type == DrawEntry::Text)
            static_cast<DrawEntryText*>(entry)->outline_color.a = alpha;
    } else if (strequal(key, "Color")) {
        auto alpha = entry->color.a;
        entry->color = *lua_checkcolor3(L, 3);
        entry->color.a = alpha;
    } else {
        switch (entry->type) {
            case DrawEntry::Line: {
                DrawEntryLine* entry_line = static_cast<DrawEntryLine*>(entry);
                if (strequal(key, "Thickness"))
                    entry_line->thickness = luaL_checknumber(L, 3);
                else if (strequal(key, "From"))
                    entry_line->from = *lua_checkvector2(L, 3);
                else if (strequal(key, "To"))
                    entry_line->to = *lua_checkvector2(L, 3);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::Text: {
                DrawEntryText* entry_text = static_cast<DrawEntryText*>(entry);
                if (strequal(key, "Text")) {
                    size_t l;
                    const char* str = luaL_checklstring(L, 3, &l);
                    entry_text->text = std::string(str, l);
                    entry_text->updateTextBounds();
                } else if (strequal(key, "TextBounds"))
                    goto READONLY;
                else if (strequal(key, "Size") || strequal(key, "TextSize") || strequal(key, "FontSize")) {
                    entry_text->text_size = luaL_checknumberrange(L, 3, 0, static_cast<unsigned>(-1));
                    entry_text->updateTextBounds();
                    entry_text->updateOutline();
                } else if (strequal(key, "Font")) {
                    entry_text->font_enum = static_cast<DrawEntryText::FontEnum>(luaL_checknumberrange(L, 3, 0, DrawEntryText::FONT__COUNT - 1));
                    entry_text->updateFont();
                } else if (strequal(key, "Centered"))
                    entry_text->centered = luaL_checkboolean(L, 3);
                else if (strequal(key, "Outlined"))
                    entry_text->outlined = luaL_checkboolean(L, 3);
                else if (strequal(key, "OutlineColor")) {
                    entry_text->outline_color = *lua_checkcolor3(L, 3);
                    entry_text->outline_color.a = entry->color.a;
                } else if (strequal(key, "Position")) {
                    entry_text->position = *lua_checkvector2(L, 3);
                    entry_text->updateOutline();
                } else
                    goto INVALID;

                break;
            }
            case DrawEntry::Square: {
                DrawEntrySquare* entry_square = static_cast<DrawEntrySquare*>(entry);
                if (strequal(key, "Thickness"))
                    entry_square->thickness = luaL_checknumber(L, 3);
                else if (strequal(key, "Size")) {
                    auto vector = lua_checkvector2(L, 3);
                    entry_square->rect.width = vector->x;
                    entry_square->rect.height = vector->y;
                } else if (strequal(key, "Position")) {
                    auto vector = lua_checkvector2(L, 3);
                    entry_square->rect.x = vector->x;
                    entry_square->rect.y = vector->y;
                } else if (strequal(key, "Filled"))
                    entry_square->filled = luaL_checkboolean(L, 3);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::Triangle: {
                DrawEntryTriangle* entry_triangle = static_cast<DrawEntryTriangle*>(entry);
                if (strequal(key, "Thickness"))
                    entry_triangle->thickness = luaL_checknumber(L, 3);
                else if (strequal(key, "PointA"))
                    entry_triangle->pointa = *lua_checkvector2(L, 3);
                else if (strequal(key, "PointB"))
                    entry_triangle->pointb = *lua_checkvector2(L, 3);
                else if (strequal(key, "PointC"))
                    entry_triangle->pointc = *lua_checkvector2(L, 3);
                else if (strequal(key, "Filled"))
                    entry_triangle->filled = luaL_checkboolean(L, 3);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::Quad: {
                DrawEntryQuad* entry_quad = static_cast<DrawEntryQuad*>(entry);
                if (strequal(key, "Thickness"))
                    entry_quad->thickness = luaL_checknumber(L, 3);
                else if (strequal(key, "PointA"))
                    entry_quad->pointa = *lua_checkvector2(L, 3);
                else if (strequal(key, "PointB"))
                    entry_quad->pointb = *lua_checkvector2(L, 3);
                else if (strequal(key, "PointC"))
                    entry_quad->pointc = *lua_checkvector2(L, 3);
                else if (strequal(key, "PointD"))
                    entry_quad->pointd = *lua_checkvector2(L, 3);
                else if (strequal(key, "Filled"))
                    entry_quad->filled = luaL_checkboolean(L, 3);
                else
                    goto INVALID;

                break;
            }
            default:
                luaL_errorL(L, "INTERNAL TODO: all DrawEntry types (%s)", entry->class_name);
                break;
        }
    }

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of %s", key, entry->class_name);

    READONLY:
    luaL_error(L, "Unable to assign property %s. Property is read only", key);
}
static int DrawEntry__namecall(lua_State* L) {
    DrawEntry* entry = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");

    lua_CFunction func = getDrawEntryMethod(entry, namecall);
    if (!func)
        luaL_error(L, "%s is not a valid member of %s", namecall, entry->class_name);

    return func(L);
}

void open_drawentrylib(lua_State *L) {
    // Drawing global
    lua_newtable(L);

    setfunctionfield(L, DrawEntry_new, "new");

    lua_pushvalue(L, -1);
    lua_setglobal(L, "Drawing");
    lua_setglobal(L, "DrawEntry");

    // metatable
    luaL_newmetatable(L, "DrawEntry");

    setfunctionfield(L, DrawEntry__tostring, "__tostring");
    setfunctionfield(L, DrawEntry__index, "__index");
    setfunctionfield(L, DrawEntry__newindex, "__newindex");
    setfunctionfield(L, DrawEntry__namecall, "__namecall");

    lua_pop(L, 1);
}

}; // namespace fakeroblox
