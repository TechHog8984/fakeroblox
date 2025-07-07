#include "classes/roblox/instance.hpp"
#include "common.hpp"
#include "lua.h"
#include "lualib.h"
#include <algorithm>

namespace fakeroblox {

#define LUA_TAG_RBXINSTANCE 1

rbxInstance* lua_checkinstance(lua_State* L, int arg) {
    luaL_checkany(L, arg);
    void* ud = luaL_checkudata(L, arg, "Instance");

    return *static_cast<rbxInstance**>(ud);
}
rbxInstance* lua_optinstance(lua_State* L, int arg) {
    if (lua_gettop(L) < arg)
        return nullptr;

    if (lua_type(L, arg) == LUA_TNIL)
        return nullptr;

    return lua_checkinstance(L, arg);
}

namespace rbxInstance_methods {
    int destroy(lua_State *L) {
        // FIXME: synchronize
        rbxInstance* instance = lua_checkinstance(L, 1);
        if (instance->destroyed)
            return 0;

        instance->destroyed = true;

        lua_pushnil(L);
        lua_setfield(L, -2, PROP_INSTANCE_PARENT);

        instance->parent_locked = true;

        return 0;
    }
}; // namespace rbxInstance_methods

int rbxInstance__index(lua_State* L) {
    rbxInstance* instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // TODO: write-only properties, if they exist
    if (strequal(key, PROP_INSTANCE_ARCHIVABLE))
        lua_pushboolean(L, instance->archivable);
    else if (strequal(key, PROP_INSTANCE_NAME))
        lua_pushlstring(L, instance->name.data(), instance->name.size());
    else if (strequal(key, PROP_INSTANCE_CLASS_NAME))
        lua_pushlstring(L, instance->class_name.data(), instance->class_name.size());
    else if (strequal(key, PROP_INSTANCE_PARENT)) {
        rbxInstance* parent = instance->parent;
        if (parent) {
            pushInstance(L, std::make_shared<rbxInstance>(*parent));
        } else
            lua_pushnil(L);
    } else if (strequal(key, METHOD_INSTANCE_DESTROY))
        lua_pushcfunction(L, rbxInstance_methods::destroy, METHOD_INSTANCE_DESTROY);
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
    if (strequal(key, PROP_INSTANCE_ARCHIVABLE))
        instance->archivable = luaL_checkboolean(L, 2);
    else if (strequal(key, PROP_INSTANCE_NAME)) {
        size_t size;
        auto str = luaL_checklstring(L, 3, &size);
        instance->name = std::string(str, size);
    } else if (strequal(key, PROP_INSTANCE_PARENT)) {
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

void rbxInstance__dtor(lua_State* L, std::shared_ptr<rbxInstance> ptr) {
    ptr.reset();
}

std::shared_ptr<rbxInstance> newInstance(const char* class_name) {
    std::shared_ptr<rbxInstance> instance = std::make_shared<rbxInstance>();

    // FIXME: default values
    instance->archivable = false;
    instance->name = class_name;
    instance->class_name = class_name;
    // FIXME: unique_id
    instance->unique_id = 0;

    return instance;
}

int pushInstance(lua_State* L, std::shared_ptr<rbxInstance> instance) {
    lua_getfield(L, LUA_REGISTRYINDEX, "instancelookup");
    lua_pushlightuserdata(L, instance.get());
    lua_rawget(L, -2);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        void* ud = lua_newuserdatatagged(L, sizeof(std::shared_ptr<rbxInstance>), LUA_TAG_RBXINSTANCE);
        new(ud) std::shared_ptr<rbxInstance>(instance);

        luaL_getmetatable(L, "Instance");
        lua_setmetatable(L, -2);

        lua_pushlightuserdata(L, instance.get());
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
    }

    lua_remove(L, -2);

    return 1;
}
namespace rbxInstance_datatype {
    int _new(lua_State* L) {
        const char* class_name = luaL_checkstring(L, 1);
        rbxInstance* parent = lua_optinstance(L, 2);

        std::shared_ptr<rbxInstance> instance = newInstance(class_name);
        pushInstance(L, instance);

        if (parent) {
            lua_pushvalue(L, 2);
            lua_setfield(L, -2, PROP_INSTANCE_PARENT);
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

    // instancelookup
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");

    lua_setfield(L, LUA_REGISTRYINDEX, "instancelookup");

    lua_setuserdatadtor(L, LUA_TAG_RBXINSTANCE, (lua_Destructor) rbxInstance__dtor);

    auto instance = newInstance("DataModel");
    instance->name.assign("FakeRoblox");

    pushInstance(L, instance);
    lua_setglobal(L, "game");
}
void rbxInstanceCleanup(lua_State* L) {
}

}; // namespace fakeroblox
