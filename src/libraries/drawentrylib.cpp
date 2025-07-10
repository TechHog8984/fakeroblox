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

static int DrawEntry_new(lua_State* L) {
    const char* class_name = luaL_checkstring(L, 1);

    void* ud = nullptr;
    // FIXME: all DrawEntry types
    if (strequal(class_name, "Line")) {
        ud = lua_newuserdata(L, sizeof(DrawEntryLine));
        new(ud) DrawEntryLine();
    // } else if (strequal(class_name, "Text")) {
    //     ud = lua_newuserdata(L, sizeof(DrawEntryText));
    //     new(ud) DrawEntryText();
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
        // FIXME: verify alpha math
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
    else if (strequal(key, "Transparency") || strequal(key, "Opacity"))
        // FIXME: verify alpha math
        entry->color.a = luaL_checknumberrange(L, 3, 0, 1) * 255.0;
    else if (strequal(key, "Color")) {
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
            default:
                luaL_errorL(L, "INTERNAL TODO: all DrawEntry types (%s)", entry->class_name);
                break;
        }
    }

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of %s", key, entry->class_name);
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
