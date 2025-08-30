#include "classes/roblox/datatypes/enum.hpp"
#include "common.hpp"

#include "console.hpp"
#include "lua.h"
#include "lualib.h"

namespace fakeroblox {

std::map<std::string, Enum> Enum::enum_map;

int pushEnumTable(lua_State* L, std::string name) {
    return pushFromLookup(L, ENUMLOOKUP, [&L, name] { lua_pushstring(L, name.c_str()); }, [&L, name] {
        lua_createtable(L, 2, 0);

        Enum* enum_ptr = static_cast<Enum*>(lua_newuserdatatagged(L, sizeof(Enum), LUA_TAG_ENUM));
        new(enum_ptr) Enum;
        *enum_ptr = Enum::enum_map.at(name);

        luaL_getmetatable(L, "Enum");
        lua_setmetatable(L, -2);

        lua_rawseti(L, -2, 1);

        lua_newtable(L);

        lua_rawseti(L, -2, 2);
    });
}
int pushEnum(lua_State* L, std::string name) {
    assert(pushEnumTable(L, name) == 1);
    assert(lua_rawgeti(L, -1, 1) != LUA_TNIL);
    return 1;
}

int pushEnumItem(lua_State* L, std::string enum_name, std::string name) {
    assert(pushEnumTable(L, enum_name) == 1);
    assert(lua_rawgeti(L, -1, 2) != LUA_TNIL);
    lua_remove(L, -2);

    EnumItem enum_item = Enum::enum_map.at(enum_name).item_map.at(name);

    addToLookup(L, [&L, &enum_item] {
        EnumItem* enum_item_ptr = static_cast<EnumItem*>(lua_newuserdatatagged(L, sizeof(EnumItem), LUA_TAG_ENUMITEM));
        new(enum_item_ptr) EnumItem;
        *enum_item_ptr = enum_item;

        luaL_getmetatable(L, "EnumItem");
        lua_setmetatable(L, -2);
    }, true);
    return 1;
}
int pushEnumItem(lua_State* L, EnumItemWrapper& wrapper) {
    return pushEnumItem(L, wrapper.enum_name, wrapper.name);
}

static void Enum__dtor(lua_State* L, void* ud) {
    Enum* enum_ptr = static_cast<Enum*>(ud);
    enum_ptr->~Enum();
}
static void EnumItem__dtor(lua_State* L, void* ud) {
    EnumItem* enum_item_ptr = static_cast<EnumItem*>(ud);
    enum_item_ptr->~EnumItem();
}

Enum* lua_checkenum(lua_State* L, int narg) {
    return static_cast<Enum*>(luaL_checkudata(L, narg, "Enum"));
}

static int Enum__tostring(lua_State* L) {
    Enum* _enum = lua_checkenum(L, 1);

    lua_pushfstring(L, "%s", _enum->name.c_str());
    return 1;
}

static int Enum__index(lua_State* L) {
    Enum* _enum = lua_checkenum(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (_enum->item_map.find(key) == _enum->item_map.end())
        luaL_error(L, "%s is not a valid member of \"Enum.%s\"", key, _enum->name.c_str());

    return pushEnumItem(L, _enum->name, key);
}

EnumItem* lua_checkenumitem(lua_State* L, int narg) {
    return static_cast<EnumItem*>(luaL_checkudata(L, narg, "EnumItem"));
}

static int EnumItem__tostring(lua_State* L) {
    EnumItem* enum_item = lua_checkenumitem(L, 1);

    lua_pushfstring(L, "Enum.%s.%s", enum_item->enum_name.c_str(), enum_item->name.c_str());
    return 1;
}

static int EnumItem__index(lua_State* L) {
    EnumItem* enum_item = lua_checkenumitem(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strequal(key, "Name"))
        lua_pushstring(L, enum_item->name.c_str());
    else if (strequal(key, "Value"))
        lua_pushunsigned(L, enum_item->value);
    else if (strequal(key, "Enum"))
        pushEnum(L, enum_item->enum_name);
    else
        luaL_errorL(L, "%s is not a valid member of Enum.%s.%s", key, enum_item->enum_name.c_str(), enum_item->name.c_str());

    return 1;
}
static int EnumItem__eq(lua_State* L) {
    EnumItem* a = lua_checkenumitem(L, 1);
    EnumItem* b = lua_checkenumitem(L, 2);

    lua_pushboolean(L, a->enum_name == b->enum_name && a->value == b->value);
    return 1;
}

void lua_checkenums(lua_State* L, int narg) {
    luaL_checkudata(L, narg, "Enums");
}

static int Enums__tostring(lua_State* L) {
    lua_checkenums(L, 1);

    lua_pushstring(L, "Enums");
    return 1;
}

static int Enums__index(lua_State* L) {
    lua_checkenums(L, 1);
    const char* key = lua_tostring(L, 2);

    if (Enum::enum_map.find(key) == Enum::enum_map.end())
        luaL_error(L, "%s is not a valid member of \"Enum\"", key);

    return pushEnum(L, key);
}

void setup_enums(lua_State* L) {
    // enumlookup
    newweaktable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, ENUMLOOKUP);

    // Enum
    lua_newuserdata(L, 0);

    // Enums metatable
    luaL_newmetatable(L, "Enums");

    settypemetafield(L, "Enums");
    setfunctionfield(L, Enums__tostring, "__tostring", nullptr);
    setfunctionfield(L, Enums__index, "__index", nullptr);
    // TODO: Enums methods
    // setfunctionfield(L, Enums__namecall, "__namecall", nullptr);

    lua_setmetatable(L, -2);

    lua_setglobal(L, "Enum");

    // Enum metatable
    luaL_newmetatable(L, "Enum");

    settypemetafield(L, "Enum");
    setfunctionfield(L, Enum__tostring, "__tostring", nullptr);
    setfunctionfield(L, Enum__index, "__index", nullptr);
    // TODO: Enum methods
    // setfunctionfield(L, Enum__namecall, "__namecall", nullptr);

    lua_pop(L, 1);

    // EnumItem metatable
    luaL_newmetatable(L, "EnumItem");

    settypemetafield(L, "EnumItem");
    setfunctionfield(L, EnumItem__tostring, "__tostring", nullptr);
    setfunctionfield(L, EnumItem__index, "__index", nullptr);
    setfunctionfield(L, EnumItem__eq, "__eq", nullptr);
    // TODO: EnumItem methods
    // setfunctionfield(L, EnumItem__namecall, "__namecall", nullptr);

    lua_pop(L, 1);

    lua_setuserdatadtor(L, LUA_TAG_ENUM, Enum__dtor);
    lua_setuserdatadtor(L, LUA_TAG_ENUMITEM, EnumItem__dtor);
}

}; // namespace fakeroblox
