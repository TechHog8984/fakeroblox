#pragma once

#include "lua.h"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Instance is the base class of all objects. We may adapt to Roblox's choice of an Object abstraction in the future.

namespace fakeroblox {

class rbxProperty;

enum TypeCategory {
    Primitive,
    DataType,
    Instance
};
enum Type_PrimitiveType {
    Boolean,
    Integer32,
    Integer64,
    Float,
    Double,
    String,
    Nil,
};
enum Type_DataType {
    // FIXME: datatypes
};

struct rbxMethod {
    std::string name;
    std::optional<std::string> route = std::nullopt;
};

class rbxClass {
public:
    enum Tags : uint8_t {
        NotCreatable = 1 << 0,
    };
    uint8_t tags = 0;

    std::shared_ptr<rbxClass> superclass;
    std::map<std::string, std::shared_ptr<rbxProperty>> properties;
    std::map<std::string, rbxMethod> methods;
};

class rbxValue;

class rbxInstance {
public:
    std::shared_ptr<rbxClass> _class;
    std::map<std::string, rbxValue> values;
    std::map<std::string, rbxMethod> methods;
    std::vector<std::shared_ptr<rbxInstance>> children;

    bool destroyed = false;

    rbxInstance(std::shared_ptr<rbxClass> _class);
    // ~rbxInstance() {
    //     printf("destroying instance... %p\n", this);
    // }

    template<class T>
    T& getValue(std::string name);

    template<class T>
    void setValue(std::string name, T value);

    std::shared_ptr<rbxInstance> findFirstChild(std::string name);
};

class rbxProperty;

class rbxValue {
public:
    std::shared_ptr<rbxProperty> property;

    bool is_nil;
    std::variant<bool, int32_t, int64_t, float, double, std::string, std::shared_ptr<rbxInstance>> value;
};

class rbxProperty {
public:
    enum Tags : uint8_t {
        ReadOnly = 1 << 0,
        WriteOnly = 1 << 1,
    };
    uint8_t tags = 0;

    TypeCategory type_category;
    rbxValue default_value;

    std::optional<std::string> route = std::nullopt;
};

#define PROP_INSTANCE_ARCHIVABLE "Archivable"
#define PROP_INSTANCE_NAME "Name"
#define PROP_INSTANCE_CLASS_NAME "ClassName"
#define PROP_INSTANCE_PARENT "Parent"

#define METHOD_INSTANCE_DESTROY "Destroy"

std::shared_ptr<rbxInstance>& lua_checkinstance(lua_State* L, int arg);
std::shared_ptr<rbxInstance> lua_optinstance(lua_State* L, int arg);

void rbxInstanceSetup(lua_State* L, std::string api_jump);
void rbxInstanceCleanup(lua_State* L);

std::shared_ptr<rbxInstance> newInstance(const char* class_name);
int lua_pushinstance(lua_State* L, std::shared_ptr<rbxInstance> instance);

namespace rbxInstance_datatype {
    int _new(lua_State* L);
    int from_existing(lua_State* L);
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
