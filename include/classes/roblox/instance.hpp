#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <raylib.h>
#include <shared_mutex>
#include <string>
#include <variant>
#include <vector>

#include "classes/roblox/datatypes/enum.hpp"
#include "classes/udim.hpp"
#include "classes/udim2.hpp"
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

class rbxInstance;

class rbxClass {
public:
    static std::map<std::string, std::shared_ptr<rbxClass>> class_map;
    static std::vector<std::string> valid_class_names;
    static std::vector<std::string> valid_services;

    std::string name;

    enum Tags : uint8_t {
        NotCreatable = 1 << 0,
    };
    uint8_t tags = 0;

    std::shared_ptr<rbxClass> superclass;
    std::map<std::string, std::shared_ptr<rbxProperty>> properties;
    std::map<std::string, rbxMethod> methods;
    std::vector<std::string> events;
    std::function<void(lua_State* L, std::shared_ptr<rbxInstance> instance)> constructor = nullptr;
    std::function<void(std::shared_ptr<rbxInstance>)> destructor = nullptr;

    void newMethod(const char* name, lua_CFunction func, lua_Continuation cont = nullptr) {
        rbxMethod method;
        method.name = name;
        method.func = func;
        method.cont = cont;
        methods.try_emplace(name, method);
    }
};

class rbxProperty;

typedef std::variant<
    bool,
    int32_t,
    int64_t,
    float,
    double,
    std::string,

    std::shared_ptr<rbxInstance>,
    EnumItemWrapper,

    Color,
    Vector2,
    Vector3,
    UDim,
    UDim2
> rbxValueVariant;

class rbxValue {
public:
    std::shared_ptr<rbxProperty> property;

    bool is_nil;
    rbxValueVariant value;
};

class rbxInstance {
public:
    std::shared_ptr<rbxClass> _class;
    std::map<std::string, rbxValue> values;
    std::map<std::string, rbxMethod> methods;
    std::vector<std::string> events;
    std::vector<std::shared_ptr<rbxInstance>> children;

    std::shared_mutex values_mutex;
    std::shared_mutex children_mutex;

    bool destroyed = false;
    bool parent_locked = false;
    void* userdata = nullptr;

    std::shared_mutex destroyed_mutex;
    std::shared_mutex parent_locked_mutex;

    rbxInstance(std::shared_ptr<rbxClass> _class);
    ~rbxInstance();

    template<typename T>
    T& getValue(std::string name) {
        std::lock_guard lock(values_mutex);
        return std::get<T>(values.at(name).value);
    }

    template<typename T>
    void setValue(lua_State* L, std::string name, T value, bool dont_report_changed = false) {
        std::lock_guard lock(values_mutex);
        std::get<T>(values.at(name).value) = value;
        if (!dont_report_changed)
            reportChanged(L, name.c_str());
    }

    bool isA(rbxClass* target_class);
    bool isA(const char* class_name);
    int pushEvent(lua_State* L, const char* name);
    void reportChanged(lua_State* L, const char* property);

    std::shared_ptr<rbxInstance> findFirstChild(std::string name);
};

void destroyInstance(lua_State* L, std::shared_ptr<rbxInstance> instance);
void setInstanceParent(lua_State* L, std::shared_ptr<rbxInstance> instance, std::shared_ptr<rbxInstance> new_parent);

class rbxProperty {
public:
    enum Tags : uint8_t {
        Hidden = 1 << 0,
        Deprecated = 1 << 1,
        ReadOnly = 1 << 2,
        WriteOnly = 1 << 3,
        NotScriptable = 1 << 4
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
