#pragma once

#include "lua.h"
#include <string>
#include <vector>

// Instance is the base class of all objects. We may adapt to Roblox's choice of an Object abstraction in the future.

namespace fakeroblox {

class rbxInstance {
public:
    int ref = 0;
    std::vector<rbxInstance*> children;
    bool destroyed = false;

    bool archivable;
    // TODO: capabilities
    std::string name;
    std::string class_name;
    rbxInstance* parent = nullptr; bool parent_locked = false;
    // TODO: roblox_locked
    // TODO: sandboxed
    size_t unique_id;
};

rbxInstance* lua_checkinstance(lua_State* L, int arg);
rbxInstance* lua_optinstance(lua_State* L, int arg);

void rbxInstanceSetup(lua_State* L);
void rbxInstanceCleanup(lua_State* L);

namespace rbxInstance_datatype {
    int _new(lua_State* L);
    // TODO: fromExisting
}; // namespace rbxInstance_datatype

namespace rbxInstance_methods {
    // TODO: addTag

    int clearAllChildren(lua_State* L);
    // TODO: clone
    int destroy(lua_State* L);

    // TODO: findFirstAncestor*
    // TODO: findFirstChild*
    // TODO: findFirstDescendant

    // TODO: getActor

    // TODO: getAttribute*

    int getChildren(lua_State* L);

    // TODO: getDebugId

    // TODO: getDescendants

    // TODO: getFullName

    // TODO: getStyled*

    // TODO: getTags & hasTag

    // TODO: is[Ancestor/Descendant]Of

    // TODO: isPropertyModified

    // TODO: removeTag

    // TODO: resetPropertyToDefault

    // TODO: setAttribute

    // TODO: waitForChild
}; // namespace rbxInstance_methods

}; // namespace fakeroblox
