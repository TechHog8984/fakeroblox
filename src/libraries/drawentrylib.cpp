#include "libraries/drawentrylib.hpp"
#include "classes/color3.hpp"
#include "classes/vector2.hpp"
#include "common.hpp"

#include "fontloader.hpp"
#include "imageloader.hpp"

#include "lua.h"
#include "lualib.h"

#include <mutex>
#include <shared_mutex>

namespace fakeroblox {

std::vector<DrawEntry*> DrawEntry::draw_list;
std::shared_mutex DrawEntry::draw_list_mutex;

void sortDrawList() {
    std::lock_guard lock(DrawEntry::draw_list_mutex);

    std::sort(DrawEntry::draw_list.begin(), DrawEntry::draw_list.end(), [] (DrawEntry* a, DrawEntry* b) {
        return a->zindex < b->zindex;
    });
}

DrawEntry::DrawEntry(Type type, const char* class_name) : type(type), class_name(class_name) {}
DrawEntryLine::DrawEntryLine() : DrawEntry(DrawEntry::DrawTypeLine, "Line") {}
DrawEntryText::DrawEntryText() : DrawEntry(DrawEntry::DrawTypeText, "Text") {}
DrawEntryImage::DrawEntryImage() : DrawEntry(DrawEntry::DrawTypeImage, "Image") {}
DrawEntryCircle::DrawEntryCircle() : DrawEntry(DrawEntry::DrawTypeCircle, "Circle") {}
DrawEntrySquare::DrawEntrySquare() : DrawEntry(DrawEntry::DrawTypeSquare, "Square") {}
DrawEntryTriangle::DrawEntryTriangle() : DrawEntry(DrawEntry::DrawTypeTriangle, "Triangle") {}
DrawEntryQuad::DrawEntryQuad() : DrawEntry(DrawEntry::DrawTypeQuad, "Quad") {}

size_t DrawEntryText::default_font = FontDefault;
void DrawEntryText::updateTextBounds() {
    text_bounds = MeasureTextEx(*font, text.c_str(), text_size, 0);
}
void DrawEntryText::updateFont() {
    font = FontLoader::font_list[font_index];

    updateTextBounds();
}
void DrawEntryText::updateCustomFont() {
    unsigned char* data_ptr = reinterpret_cast<unsigned char*>(custom_font_data.data());
    int data_size = custom_font_data.size();
    font_index = FontLoader::getFont(data_ptr, data_size);

    updateFont();
}

// FIXME: text outline
// const float OUTLINE_OFFSET = 2;
void DrawEntryText::updateOutline() {
    // outline_position = Vector2{ position.x + OUTLINE_OFFSET / 2, position.y + OUTLINE_OFFSET / 2 };
    // outline_text_size = text_size + OUTLINE_OFFSET;

    // outline_position = Vector2{ position.x - 1, position.y };
    // outline_text_size = text_size + 1;
}

DrawEntryImage::~DrawEntryImage() {
    UnloadTexture(texture);
}
void DrawEntryImage::updateData() {
    unsigned char* data_ptr = reinterpret_cast<unsigned char*>(data.data());
    int data_size = data.size();

    image = ImageLoader::getImage(data_ptr, data_size);
    texture = LoadTextureFromImage(*image);
    // TODO: istexturevalid here?

    image_size.x = texture.width;
    image_size.y = texture.height;

    resizeImage();
}
void DrawEntryImage::resizeImage() {
    if (data.empty())
        return;

    UnloadTexture(texture);
    UnloadRenderTexture(mask);
    // FIXME: this affects the cached image, meaning two drawings with the same data will both be resized
    ImageResize(image, size.x, size.y);

    texture = LoadTextureFromImage(*image);
    mask = LoadRenderTexture(size.x, size.y);

    updateRounding();
}
void DrawEntryImage::updateRounding() {
    if (mask.id <= 0)
        return;

    BeginTextureMode(mask);
    ClearBackground(BLANK);

    DrawCircle(0, 0, rounding, WHITE);
    // DrawRectangleRounded((Rectangle){0, 0, size.x, size.y}, rounding, 16, WHITE);
    EndTextureMode();
}

void DrawEntry::onZIndexUpdate() {
    sortDrawList();
}

#define DrawEntry_free_case(type) case DrawEntry::DrawType##type:       \
    static_cast<DrawEntry##type*>(this)->~DrawEntry##type();  \
    break;                                                    \

void DrawEntry::free() {
    switch (this->type) {
        DrawEntry_free_case(Line)
        DrawEntry_free_case(Text)
        DrawEntry_free_case(Image)
        DrawEntry_free_case(Circle)
        DrawEntry_free_case(Square)
        DrawEntry_free_case(Triangle)
        DrawEntry_free_case(Quad)
        default:
            assert(!"TODO: all DrawEntry types in free");
            break;
    }
}

#undef DrawEntry_free_case

void DrawEntry::destroy(lua_State* L, bool dont_erase) {
    if (!alive)
        return;

    // TODO alive index and newindex behavior
    alive = false;

    if (!dont_erase) {
        std::lock_guard lock(DrawEntry::draw_list_mutex);
        DrawEntry::draw_list.erase(std::find(DrawEntry::draw_list.begin(), DrawEntry::draw_list.end(), this));
    }

    luaL_getmetatable(L, "DrawEntry");
    lua_getfield(L, -1, "objects");

    lua_pushnumber(L, lookup_index);
    lua_pushnil(L);
    lua_rawset(L, -3);

    lua_pop(L, 2);

    free();
}

#define DrawEntry_new_branch(type) if (strequal(class_name, #type)) { \
    ud = lua_newuserdata(L, sizeof(DrawEntry##type)); \
    new(ud) DrawEntry##type(); \


DrawEntry* pushNewDrawEntry(lua_State* L, const char* class_name) {
    void* ud = nullptr;
    // FIXME: all DrawEntry types
    DrawEntry_new_branch(Line)
    } else DrawEntry_new_branch(Text)
        DrawEntryText* entry_text = static_cast<DrawEntryText*>(ud);
        entry_text->font_index = DrawEntryText::default_font;
        entry_text->updateFont();
    } else DrawEntry_new_branch(Image)
    } else DrawEntry_new_branch(Circle)
    } else DrawEntry_new_branch(Square)
    } else DrawEntry_new_branch(Triangle)
    } else DrawEntry_new_branch(Quad)
    } else
        luaL_error(L, "invalid DrawEntry class name '%s'", class_name);

    assert(ud);

    const int original_top = lua_gettop(L);
    DrawEntry* entry = static_cast<DrawEntry*>(ud);
    entry->color.a = 255;

    std::shared_lock lock(DrawEntry::draw_list_mutex);
    DrawEntry::draw_list.push_back(entry);
    lock.unlock();

    sortDrawList();

    luaL_getmetatable(L, "DrawEntry");
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -3);

    lua_getfield(L, -1, "objects");
    entry->lookup_index = addToLookup(L, [&L, &original_top] () {
        lua_pushvalue(L, original_top);
    });

    lua_pop(L, 1);

    return entry;
}
static int DrawEntry_new(lua_State* L) {
    const char* class_name = luaL_checkstring(L, 1);
    pushNewDrawEntry(L, class_name);
    return 1;
}

DrawEntry* DrawEntry::clone(lua_State* L) {
    DrawEntry* entry = pushNewDrawEntry(L, class_name);

    entry->visible = visible;
    entry->zindex = zindex;
    entry->onZIndexUpdate();
    entry->color = color;

    switch (type) {
        case DrawEntry::DrawTypeLine: {
            DrawEntryLine* this_line = static_cast<DrawEntryLine*>(this);
            DrawEntryLine* entry_line = static_cast<DrawEntryLine*>(entry);
            entry_line->thickness = this_line->thickness;
            entry_line->from = this_line->from;
            entry_line->to = this_line->to;

            break;
        }
        case DrawEntry::DrawTypeText: {
            DrawEntryText* this_text = static_cast<DrawEntryText*>(this);
            DrawEntryText* entry_text = static_cast<DrawEntryText*>(entry);
            entry_text->text = this_text->text;
            entry_text->text_bounds = this_text->text_bounds;
            entry_text->text_size = this_text->text_size;
            entry_text->font_index = this_text->font_index;
            entry_text->updateFont();
            entry_text->centered = this_text->centered;
            entry_text->outlined = this_text->outlined;
            entry_text->outline_color = this_text->outline_color;
            entry_text->position = this_text->position;

            break;
        }
        case DrawEntry::DrawTypeImage: {
            DrawEntryImage* this_image = static_cast<DrawEntryImage*>(this);
            DrawEntryImage* entry_image = static_cast<DrawEntryImage*>(entry);
            entry_image->data = this_image->data;
            entry_image->size = this_image->size;
            entry_image->position = this_image->position;
            entry_image->rounding = this_image->rounding;
            entry_image->updateData();

            break;
        }
        case DrawEntry::DrawTypeCircle: {
            DrawEntryCircle* this_circle = static_cast<DrawEntryCircle*>(this);
            DrawEntryCircle* entry_circle = static_cast<DrawEntryCircle*>(entry);
            entry_circle->thickness = this_circle->thickness;
            entry_circle->num_sides = this_circle->num_sides;
            entry_circle->radius = this_circle->radius;
            entry_circle->filled = this_circle->filled;
            entry_circle->center = this_circle->center;

            break;
        }
        case DrawEntry::DrawTypeSquare: {
            DrawEntrySquare* this_square = static_cast<DrawEntrySquare*>(this);
            DrawEntrySquare* entry_square = static_cast<DrawEntrySquare*>(entry);
            entry_square->thickness = this_square->thickness;
            entry_square->rect = this_square->rect;
            entry_square->filled = this_square->filled;

            break;
        }
        case DrawEntry::DrawTypeTriangle: {
            DrawEntryTriangle* this_triangle = static_cast<DrawEntryTriangle*>(this);
            DrawEntryTriangle* entry_triangle = static_cast<DrawEntryTriangle*>(entry);
            entry_triangle->thickness = this_triangle->thickness;
            entry_triangle->pointa = this_triangle->pointa;
            entry_triangle->pointb = this_triangle->pointb;
            entry_triangle->pointc = this_triangle->pointc;
            entry_triangle->filled = this_triangle->filled;

            break;
        }
        case DrawEntry::DrawTypeQuad: {
            DrawEntryQuad* this_quad = static_cast<DrawEntryQuad*>(this);
            DrawEntryQuad* entry_quad = static_cast<DrawEntryQuad*>(entry);
            entry_quad->thickness = this_quad->thickness;
            entry_quad->pointa = this_quad->pointa;
            entry_quad->pointb = this_quad->pointb;
            entry_quad->pointc = this_quad->pointc;
            entry_quad->pointd = this_quad->pointd;
            entry_quad->filled = this_quad->filled;

            break;
        }
    }

    return entry;
}

static int DrawEntry_getObjects(lua_State* L) {
    luaL_getmetatable(L, "DrawEntry");
    lua_getfield(L, -1, "objects");

    return 1;
}
static int DrawEntry_clear(lua_State* L) {
    DrawEntry::clear(L);

    return 0;
}

#undef DrawEntry_new_branch

static int DrawEntryGlobal__index(lua_State* L) {
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "DefaultFont"))
        lua_pushunsigned(L, DrawEntryText::default_font);
    else
        lua_pushnil(L);

    return 1;
}

static int DrawEntryGlobal__newindex(lua_State* L) {
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "DefaultFont"))
        DrawEntryText::default_font = luaL_checknumberrange(L, 3, 0, FontLoader::font_count - 1);
    else
        lua_rawset(L, 1);

    return 0;
}

namespace DrawEntry_methods {
    static int remove(lua_State* L) {
        DrawEntry* obj = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));

        if (!obj->alive)
            luaL_error(L, "INTERNAL TODO: determine remove behavior when DrawEntry is already removed");

        obj->destroy(L);
        return 0;
    }
    static int clone(lua_State* L) {
        DrawEntry* obj = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));

        if (!obj->alive)
            luaL_error(L, "INTERNAL TODO: determine clone behavior when DrawEntry is already removed");

        obj->clone(L);
        return 1;
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
    else if (strequal(key, "Clone"))
        return DrawEntry_methods::clone;

    return nullptr;
}

static int DrawEntry__index(lua_State* L) {
    DrawEntry* entry = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));
    const char* key = luaL_checkstring(L, 2);

    // god i hate skids
    if (strequal(key, "__OBJECT_EXISTS")) {
        lua_pushboolean(L, entry->alive);
        return 1;
    }

    {
    std::lock_guard lock(entry->members_mutex);

    if (strequal(key, "Visible"))
        lua_pushboolean(L, entry->visible);
    else if (strequal(key, "ZIndex"))
        lua_pushinteger(L, entry->zindex);
    else if (strequal(key, "Transparency") || strequal(key, "Opacity"))
        lua_pushnumber(L, entry->color.a / 255.0);
    else if (strequal(key, "Color"))
        pushColor(L, entry->color);
    else {
        lua_CFunction func = getDrawEntryMethod(entry, key);
        if (func)
            return pushFunctionFromLookup(L, func);

        switch (entry->type) {
            case DrawEntry::DrawTypeLine: {
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
            case DrawEntry::DrawTypeText: {
                DrawEntryText* entry_text = static_cast<DrawEntryText*>(entry);
                if (strequal(key, "Text")) {
                    std::string text = entry_text->text;
                    lua_pushlstring(L, text.c_str(), text.size());
                } else if (strequal(key, "TextBounds"))
                    pushVector2(L, entry_text->text_bounds);
                else if (strequal(key, "Size") || strequal(key, "TextSize") || strequal(key, "FontSize"))
                    lua_pushnumber(L, entry_text->text_size);
                else if (strequal(key, "Font"))
                    lua_pushunsigned(L, entry_text->font_index);
                else if (strequal(key, "Centered"))
                    lua_pushboolean(L, entry_text->centered);
                else if (strequal(key, "Outlined"))
                    lua_pushboolean(L, entry_text->outlined);
                else if (strequal(key, "OutlineColor"))
                    pushColor(L, entry_text->outline_color);
                else if (strequal(key, "Position"))
                    pushVector2(L, entry_text->position);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::DrawTypeImage: {
                DrawEntryImage* entry_image = static_cast<DrawEntryImage*>(entry);
                if (strequal(key, "Data")) {
                    std::string data = entry_image->data;
                    lua_pushlstring(L, data.c_str(), data.size());
                } else if (strequal(key, "ImageSize"))
                    pushVector2(L, entry_image->image_size);
                else if (strequal(key, "Size"))
                    pushVector2(L, entry_image->size);
                else if (strequal(key, "Position"))
                    pushVector2(L, entry_image->position);
                else if (strequal(key, "Rounding"))
                    lua_pushnumber(L, entry_image->rounding);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::DrawTypeCircle: {
                DrawEntryCircle* entry_circle = static_cast<DrawEntryCircle*>(entry);
                if (strequal(key, "Thickness"))
                    lua_pushnumber(L, entry_circle->thickness);
                else if (strequal(key, "NumSides"))
                    lua_pushnumber(L, entry_circle->num_sides);
                else if (strequal(key, "Radius"))
                    lua_pushnumber(L, entry_circle->radius);
                else if (strequal(key, "Filled"))
                    lua_pushboolean(L, entry_circle->filled);
                else if (strequal(key, "Center") || strequal(key, "Position"))
                    pushVector2(L, entry_circle->center);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::DrawTypeSquare: {
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
            case DrawEntry::DrawTypeTriangle: {
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
            case DrawEntry::DrawTypeQuad: {
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
        }
    }
    }

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of %s", key, entry->class_name);
}

static int DrawEntry__newindex(lua_State* L) {
    DrawEntry* entry = static_cast<DrawEntry*>(luaL_checkudata(L, 1, "DrawEntry"));
    const char* key = luaL_checkstring(L, 2);

    // god i hate skids
    if (strequal(key, "__OBJECT_EXISTS")) {
        goto READONLY;
    }

    {
    std::lock_guard lock(entry->members_mutex);

    if (strequal(key, "Visible"))
        entry->visible = luaL_checkboolean(L, 3);
    else if (strequal(key, "ZIndex")) {
        entry->zindex = luaL_checkinteger(L, 3);
        entry->onZIndexUpdate();
    } else if (strequal(key, "Transparency") || strequal(key, "Opacity")) {
        double alpha = luaL_checknumberrange(L, 3, 0, 1) * 255.0;
        entry->color.a = alpha;
        if (entry->type == DrawEntry::DrawTypeText)
            static_cast<DrawEntryText*>(entry)->outline_color.a = alpha;
    } else if (strequal(key, "Color")) {
        auto alpha = entry->color.a;
        entry->color = *lua_checkcolor(L, 3);
        entry->color.a = alpha;
    } else {
        switch (entry->type) {
            case DrawEntry::DrawTypeLine: {
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
            case DrawEntry::DrawTypeText: {
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
                    if (lua_isstring(L, 3) && !lua_isnumber(L, 3)) {
                        size_t l;
                        const char* str = luaL_checklstring(L, 3, &l);
                        std::string data(str, l);
                        if (!FontLoader::getFontType(reinterpret_cast<unsigned char*>(data.data()), l))
                            luaL_error(L, "failed to detect valid type from font data; %s", FontLoader::SUPPORTED_FONT_TYPE_STRING);
                        entry_text->custom_font_data = data;
                        entry_text->updateCustomFont();
                    } else {
                        entry_text->font_index = luaL_checknumberrange(L, 3, 0, FontLoader::font_count - 1);
                        entry_text->updateFont();
                    }
                } else if (strequal(key, "Centered"))
                    entry_text->centered = luaL_checkboolean(L, 3);
                else if (strequal(key, "Outlined"))
                    entry_text->outlined = luaL_checkboolean(L, 3);
                else if (strequal(key, "OutlineColor")) {
                    entry_text->outline_color = *lua_checkcolor(L, 3);
                    entry_text->outline_color.a = entry->color.a;
                } else if (strequal(key, "Position")) {
                    entry_text->position = *lua_checkvector2(L, 3);
                    entry_text->updateOutline();
                } else
                    goto INVALID;

                break;
            }
            case DrawEntry::DrawTypeImage: {
                DrawEntryImage* entry_image = static_cast<DrawEntryImage*>(entry);
                if (strequal(key, "Data")) {
                    size_t l;
                    const char* str = luaL_checklstring(L, 3, &l);
                    std::string data(str, l);
                    if (!ImageLoader::getImageType(reinterpret_cast<unsigned char*>(data.data()), l))
                        luaL_error(L, "failed to detect valid type from image data; %s", ImageLoader::SUPPORTED_IMAGE_TYPE_STRING);
                    entry_image->data = data;
                    entry_image->updateData();
                } else if (strequal(key, "ImageSize"))
                    goto READONLY;
                else if (strequal(key, "Size")) {
                    entry_image->size = *lua_checkvector2(L, 3);
                    entry_image->resizeImage();
                } else if (strequal(key, "Position"))
                    entry_image->position = *lua_checkvector2(L, 3);
                else if (strequal(key, "Rounding")) {
                    entry_image->rounding = luaL_checknumber(L, 3);
                    entry_image->updateRounding();
                } else
                    goto INVALID;

                break;
            }
            case DrawEntry::DrawTypeCircle: {
                DrawEntryCircle* entry_circle = static_cast<DrawEntryCircle*>(entry);
                if (strequal(key, "Thickness"))
                    entry_circle->thickness = luaL_checknumber(L, 3);
                else if (strequal(key, "NumSides"))
                    entry_circle->num_sides = luaL_checknumber(L, 3);
                else if (strequal(key, "Radius"))
                    entry_circle->radius = luaL_checknumber(L, 3);
                else if (strequal(key, "Filled"))
                    entry_circle->filled = luaL_checkboolean(L, 3);
                else if (strequal(key, "Center") || strequal(key, "Position"))
                    entry_circle->center = *lua_checkvector2(L, 3);
                else
                    goto INVALID;

                break;
            }
            case DrawEntry::DrawTypeSquare: {
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
            case DrawEntry::DrawTypeTriangle: {
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
            case DrawEntry::DrawTypeQuad: {
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
        }
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
    setfunctionfield(L, DrawEntry_getObjects, "GetObjects");
    setfunctionfield(L, DrawEntry_clear, "Clear");

    lua_createtable(L, FontLoader::font_count, 0);

    for (size_t i = 0; i < FontLoader::font_count; i++) {
        lua_pushunsigned(L, i);
        lua_setfield(L, -2, FontLoader::font_name_list[i].c_str());
    }

    lua_setfield(L, -2, "Fonts");

    // Drawing global metatable
    lua_createtable(L, 2, 0);

    setfunctionfield(L, DrawEntryGlobal__index, "__index");
    setfunctionfield(L, DrawEntryGlobal__newindex, "__newindex");

    lua_setmetatable(L, -2);

    lua_pushvalue(L, -1);
    lua_setglobal(L, "Drawing");
    lua_setglobal(L, "DrawEntry");

    // metatable
    luaL_newmetatable(L, "DrawEntry");

    lua_newtable(L);
    lua_setfield(L, -2, "objects");

    setfunctionfield(L, DrawEntry__tostring, "__tostring");
    setfunctionfield(L, DrawEntry__index, "__index");
    setfunctionfield(L, DrawEntry__newindex, "__newindex");
    setfunctionfield(L, DrawEntry__namecall, "__namecall");

    lua_pop(L, 1);
}

void DrawEntry::clear(lua_State *L) {
    std::lock_guard draw_list_lock(DrawEntry::draw_list_mutex);

    while (!DrawEntry::draw_list.empty()) {
        auto& entry = DrawEntry::draw_list.back();
        entry->destroy(L, true);
        DrawEntry::draw_list.pop_back();
    }
}

void DrawEntry::render() {
    std::lock_guard draw_list_lock(DrawEntry::draw_list_mutex);

    for (size_t i = 0; i < DrawEntry::draw_list.size(); i++) {
        DrawEntry* entry = DrawEntry::draw_list[i];
        std::lock_guard members_lock(entry->members_mutex);

        if (!entry->visible)
            continue;

        auto& color = entry->color;
        switch (entry->type) {
            case DrawEntry::DrawTypeLine: {
                DrawEntryLine* entry_line = static_cast<DrawEntryLine*>(entry);
                DrawLineEx(entry_line->from, entry_line->to, entry_line->thickness, color);
                break;
            }
            case DrawEntry::DrawTypeText: {
                DrawEntryText* entry_text = static_cast<DrawEntryText*>(entry);
                auto position = entry_text->position;
                if (entry_text->centered) {
                    position.x -= entry_text->text_bounds.x / 2.f;
                    position.y -= entry_text->text_bounds.y / 2.f;
                }
                if (entry_text->outlined) {
                    // top
                    DrawTextEx(*entry_text->font, entry_text->text.c_str(), { .x = position.x, .y = position.y - 1 }, entry_text->text_size, 0, entry_text->outline_color);
                    // right
                    DrawTextEx(*entry_text->font, entry_text->text.c_str(), { .x = position.x + 1, .y = position.y }, entry_text->text_size, 0, entry_text->outline_color);
                    // bottom
                    DrawTextEx(*entry_text->font, entry_text->text.c_str(), { .x = position.x - 1, .y = position.y + 1 }, entry_text->text_size, 0, entry_text->outline_color);
                    // left
                    DrawTextEx(*entry_text->font, entry_text->text.c_str(), { .x = position.x - 1, .y = position.y }, entry_text->text_size, 0, entry_text->outline_color);
                }
                DrawTextEx(*entry_text->font, entry_text->text.c_str(), position, entry_text->text_size, 0, color);
                break;
            }
            case DrawEntry::DrawTypeImage: {
                DrawEntryImage* entry_image = static_cast<DrawEntryImage*>(entry);
                // BeginBlendMode(BLEND_SUBTRACT_COLORS);
                DrawTexture(entry_image->texture, entry_image->position.x, entry_image->position.y, color);
                // DrawTexture(entry_image->mask.texture, entry_image->position.x, entry_image->position.y, color);
                // EndBlendMode();

                break;
            }
            case DrawEntry::DrawTypeCircle: {
                DrawEntryCircle* entry_circle = static_cast<DrawEntryCircle*>(entry);
                if (entry_circle->num_sides) {
                    DrawPolyLines(entry_circle->center, entry_circle->num_sides, entry_circle->radius, 0, color);
                    if (entry_circle->filled)
                        DrawPoly(entry_circle->center, entry_circle->num_sides, entry_circle->radius, 0, color);
                } else {
                    DrawCircleLinesV(entry_circle->center, entry_circle->radius, color);
                    if (entry_circle->filled)
                        DrawCircleV(entry_circle->center, entry_circle->radius, color);
                }
                break;
            }
            case DrawEntry::DrawTypeSquare: {
                DrawEntrySquare* entry_square = static_cast<DrawEntrySquare*>(entry);
                DrawRectangleLinesEx(entry_square->rect, entry_square->thickness, color);
                if (entry_square->filled)
                    DrawRectangleRec(entry_square->rect, color);
                break;
            }
            case DrawEntry::DrawTypeTriangle: {
                DrawEntryTriangle* entry_triangle = static_cast<DrawEntryTriangle*>(entry);
                // DrawTriangle* doesn't support thickness, so we use DrawLineEx
                DrawLineEx(entry_triangle->pointa, entry_triangle->pointb, entry_triangle->thickness, color);
                DrawLineEx(entry_triangle->pointb, entry_triangle->pointc, entry_triangle->thickness, color);
                DrawLineEx(entry_triangle->pointc, entry_triangle->pointa, entry_triangle->thickness, color);
                if (entry_triangle->filled)
                    DrawTriangle(entry_triangle->pointc, entry_triangle->pointb, entry_triangle->pointa, color);
                break;
            }
            case DrawEntry::DrawTypeQuad: {
                DrawEntryQuad* entry_quad = static_cast<DrawEntryQuad*>(entry);
                // DrawTriangle* doesn't support thickness, so we use DrawLineEx
                DrawLineEx(entry_quad->pointa, entry_quad->pointb, entry_quad->thickness, color);
                DrawLineEx(entry_quad->pointb, entry_quad->pointc, entry_quad->thickness, color);
                DrawLineEx(entry_quad->pointc, entry_quad->pointd, entry_quad->thickness, color);
                DrawLineEx(entry_quad->pointd, entry_quad->pointa, entry_quad->thickness, color);
                if (entry_quad->filled) {
                    DrawTriangle(entry_quad->pointc, entry_quad->pointb, entry_quad->pointa, color);
                    DrawTriangle(entry_quad->pointd, entry_quad->pointc, entry_quad->pointa, color);
                }
                break;
            }
        }
    }
}

}; // namespace fakeroblox
