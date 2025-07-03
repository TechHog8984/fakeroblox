#include "classes/roblox/instance.hpp"
#include "common.hpp"
#include "lua.h"
#include "lualib.h"
#include <algorithm>

namespace fakeroblox {

rbxInstance* lua_checkinstance(lua_State* L, int arg) {
    luaL_checkany(L, arg);
    void* ud = luaL_checkudata(L, arg, "Instance");

    return static_cast<rbxInstance*>(ud);
}
rbxInstance* lua_optinstance(lua_State* L, int arg) {
    if (lua_gettop(L) < arg)
        return nullptr;

    if (lua_type(L, arg) == LUA_TNIL)
        return nullptr;

    return lua_checkinstance(L, arg);
}

std::vector<rbxInstance*> all_instance_list;
namespace rbxInstance_methods {
    int destroy(lua_State *L) {
        // FIXME: synchronize
        rbxInstance* instance = lua_checkinstance(L, 1);
        if (instance->destroyed)
            return 0;

        auto position = std::find(all_instance_list.begin(), all_instance_list.end(), instance);
        if (position == all_instance_list.end())
            luaL_error(L, "failed to find instance in internal list");

        instance->destroyed = true;

        lua_pushnil(L);
        lua_setfield(L, -2, "Parent");

        instance->parent_locked = true;

        all_instance_list.erase(position);
        lua_unref(L, instance->ref);

        return 0;
    }
}; // namespace rbxInstance_methods

int rbxInstance__index(lua_State* L) {
    rbxInstance* instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // TODO: write-only properties, if they exist
    if (strequal(key, "Archivable"))
        lua_pushboolean(L, instance->archivable);
    else if (strequal(key, "Name"))
        lua_pushlstring(L, instance->name.data(), instance->name.size());
    else if (strequal(key, "ClassName"))
        lua_pushlstring(L, instance->class_name.data(), instance->class_name.size());
    else if (strequal(key, "Parent")) {
        rbxInstance* parent = instance->parent;
        if (parent) {
            lua_pushnumber(L, parent->ref);
            lua_gettable(L, LUA_REGISTRYINDEX);
        } else
            lua_pushnil(L);
    } else if (strequal(key, "Destroy"))
        lua_pushcfunction(L, rbxInstance_methods::destroy, "destroy");
    else
        luaL_error(L, "'%s' is not a valid member of %s '%s'", key, instance->class_name.c_str(), instance->name.c_str());

    return 1;
};

int rbxInstance__tostring(lua_State* L) {
    rbxInstance* instance = lua_checkinstance(L, 1);
    lua_pushlstring(L, instance->name.data(), instance->name.size());
    return 1;
}
const char* getOptionalInstanceName(rbxInstance* instance) {
    if (instance == nullptr)
        return "NULL";
    return instance->name.c_str();
}
int rbxInstance__newindex(lua_State* L) {
    rbxInstance* instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);
    luaL_checkany(L, 3);

    // TODO: read-only properties (like ClassName and functions)
    if (strequal(key, "Archivable"))
        instance->archivable = luaL_checkboolean(L, 2);
    else if (strequal(key, "Name"))
        // FIXME: verify null termination is fine
        instance->name = luaL_checkstring(L, 3);
    else if (strequal(key, "Parent")) {
        // TODO: synchronize
        rbxInstance* old_parent = instance->parent;
        rbxInstance* parent = lua_optinstance(L, 3);
        if ((!old_parent && !parent) || (old_parent && parent && old_parent == parent))
            return 0;

        if (instance->parent_locked)
            luaL_error(L, "The Parent property of %s is locked, current parent: %s, new parent %s", instance->name.c_str(), getOptionalInstanceName(old_parent), getOptionalInstanceName(parent));

        instance->parent = parent;

        if (old_parent)
            old_parent->children.erase(std::find(old_parent->children.begin(), old_parent->children.end(), instance));

        if (parent)
            parent->children.push_back(instance);
    } else
        luaL_error(L, "'%s' is not a valid member of %s '%s'", key, instance->class_name.c_str(), instance->name.c_str());

    return 0;
};


namespace rbxInstance_datatype {
    int _new(lua_State* L) {
        const char* class_name = luaL_checkstring(L, 1);
        rbxInstance* parent = lua_optinstance(L, 2);

        void* ud = lua_newuserdata(L, sizeof(rbxInstance));
        rbxInstance* instance = new (ud) rbxInstance;
        all_instance_list.push_back(instance);

        instance->ref = lua_ref(L, -1);

        // FIXME: default values
        instance->archivable = false;
        instance->name = class_name;
        instance->class_name = class_name;
        // FIXME: unique_id
        instance->unique_id = 0;

        luaL_getmetatable(L, "Instance");
        lua_setmetatable(L, -2);

        if (parent) {
            lua_pushvalue(L, 2);
            lua_setfield(L, -2, "Parent");
        }

        return 1;
    }
}; // namespace rbxInstance_datatype

void rbxInstanceSetup(lua_State* L) {
    // metatable
    luaL_newmetatable(L, "Instance");

    lua_pushcfunction(L, rbxInstance__tostring, "__tostring");
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, rbxInstance__index, "__index");
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, rbxInstance__newindex, "__newindex");
    lua_setfield(L, -2, "__newindex");

    lua_pop(L, 1);

    // datatype
    lua_newtable(L);

    lua_pushcfunction(L, rbxInstance_datatype::_new, "new");
    lua_setfield(L, -2, "new");

    lua_setglobal(L, "Instance");
}
void rbxInstanceCleanup(lua_State* L) {
    for (size_t i = 0; i < all_instance_list.size(); i++)
        lua_unref(L, all_instance_list[i]->ref);
    all_instance_list.clear();
}

}; // namespace fakeroblox
