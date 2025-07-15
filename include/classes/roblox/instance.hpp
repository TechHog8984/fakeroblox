#pragma once

#include "classes/roblox/datatypes/rbxscriptsignal.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lua.h"

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
    lua_CFunction func = nullptr;
    lua_Continuation cont = nullptr;
};

class rbxClass {
public:
    static std::map<std::string, std::shared_ptr<rbxClass>> class_map;

    std::string name;

    enum Tags : uint8_t {
        NotCreatable = 1 << 0,
    };
    uint8_t tags = 0;

    std::shared_ptr<rbxClass> superclass;
    std::map<std::string, std::shared_ptr<rbxProperty>> properties;
    std::map<std::string, rbxMethod> methods;
    std::vector<std::string> events;

    void newMethod(const char* name, lua_CFunction func, lua_Continuation cont = nullptr) {
        rbxMethod method;
        method.name = name;
        method.func = func;
        method.cont = cont;
        methods.try_emplace(name, method);
    }
};

class rbxValue;

class rbxInstance {
public:
    std::shared_ptr<rbxClass> _class;
    std::map<std::string, rbxValue> values;
    std::map<std::string, rbxMethod> methods;
    std::vector<std::string> events;
    std::vector<std::shared_ptr<rbxInstance>> children;

    bool destroyed = false;

    rbxInstance(std::shared_ptr<rbxClass> _class);
    ~rbxInstance();

    template<class T>
    T& getValue(std::string name);

    template<class T>
    void setValue(std::string name, T value);

    int pushEvent(lua_State* L, const char* name);

    void destroy(lua_State* L);
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

std::shared_ptr<rbxInstance>& lua_checkinstance(lua_State* L, int narg, const char* class_name = nullptr);
std::shared_ptr<rbxInstance> lua_optinstance(lua_State* L, int narg, const char* class_name = nullptr);

void rbxInstanceSetup(lua_State* L, std::string api_jump);
void rbxInstanceCleanup(lua_State* L);

std::shared_ptr<rbxInstance> newInstance(lua_State* L, const char* class_name);
int lua_pushinstance(lua_State* L, std::shared_ptr<rbxInstance> instance);

namespace rbxInstance_datatype {
    int _new(lua_State* L);
    int from_existing(lua_State* L);
}; // namespace rbxInstance_datatype

}; // namespace fakeroblox
