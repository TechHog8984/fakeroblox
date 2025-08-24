#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "raylib.h"

#include "classes/roblox/datatypes/enum.hpp"
#include "classes/udim.hpp"
#include "classes/udim2.hpp"

#include "type_registry.hpp"

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
    std::function<void(lua_State* L, rbxInstance* instance)> constructor = nullptr;
    std::function<void(rbxInstance*)> destructor = nullptr;

    void newMethod(const char* name, lua_CFunction func, lua_Continuation cont = nullptr) {
        rbxMethod method;
        method.name = name;
        method.func = func;
        method.cont = cont;
        methods.try_emplace(name, method);
    }
};

class rbxProperty;

struct rbxCallback {
    int index; // index in method lookup; -1 means empty (nil)
};

typedef std::variant<
    bool,
    int32_t,
    int64_t,
    float,
    double,
    std::string,

    rbxCallback,

    // TODO: this should most likely be a weak_ptr
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
    // TODO: setscriptable applies to all instances of a class because each value's property member will point to the same single property shared ptr.
    // We could get around this by making property an rbxProperty not a shared ptr, but this comes at the cost of memory.
    // Alternatively, we could just give rbxValue its own tags field that initially copies its property.
    std::shared_ptr<rbxProperty> property;

    // TODO: use monostate in the variant instead of is_nil?
    bool is_nil;
    rbxValueVariant value;
};

std::vector<std::weak_ptr<rbxInstance>> getNilInstances();

class rbxInstance {
public:
    REGISTER_TYPE(rbxInstance)

    static std::vector<std::weak_ptr<rbxInstance>> instance_list;
    static std::shared_mutex instance_list_mutex;

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

    bool isA(rbxClass* target_class);
    bool isA(const char* class_name);
    int pushEvent(lua_State* L, const char* name);
    void reportChanged(lua_State* L, const char* property);

    std::shared_ptr<rbxInstance> findFirstChild(std::string name);
};

#define PROP_INSTANCE_ARCHIVABLE "Archivable"
#define PROP_INSTANCE_NAME "Name"
#define PROP_INSTANCE_CLASS_NAME "ClassName"
#define PROP_INSTANCE_PARENT "Parent"

#define METHOD_INSTANCE_DESTROY "Destroy"

void destroyInstance(lua_State* L, std::shared_ptr<rbxInstance> instance, bool dont_remove_from_old_parent_children = false);
void setInstanceParent(lua_State* L, std::shared_ptr<rbxInstance> instance, std::shared_ptr<rbxInstance> new_parent, bool dont_remove_from_old_parent_children = false, bool dont_set_value = false);

bool isDescendantOf(std::shared_ptr<rbxInstance> other);

template<typename T>
T& getValue(std::shared_ptr<rbxInstance> instance, std::string name) {
    std::lock_guard lock(instance->values_mutex);
    return std::get<T>(instance->values.at(name).value);
}

template<typename T>
void setValue(std::shared_ptr<rbxInstance> instance, lua_State* L, std::string name, T value, bool dont_report_changed = false) {
    std::unique_lock lock(instance->values_mutex);

    auto& variant = instance->values.at(name).value;

    if (std::holds_alternative<EnumItemWrapper>(variant)) {
        if constexpr (std::is_same_v<T, std::string>) {
            auto& wrapper = std::get<EnumItemWrapper>(variant);
            if (value == wrapper.name)
                goto DUPLICATE;
            wrapper.name = value;

            goto AFTER_SET;
        }
        throw std::runtime_error("expected std::string when setting an EnumItem (pass the desired item's name)");
    } else if (std::holds_alternative<Color>(variant)) {
        if constexpr (std::is_same_v<T, Color>) {
            auto& v = std::get<Color>(variant);
            if (value.r == v.r && value.g == v.g && value.b == v.b)
                goto DUPLICATE;
            v = value;

            goto AFTER_SET;
        }
        throw std::runtime_error("unexpected type when setting Color value");
    } else if (std::holds_alternative<Vector2>(variant)) {
        if constexpr (std::is_same_v<T, Vector2>) {
            auto& v = std::get<Vector2>(variant);
            if (value.x == v.x && value.y == v.y)
                goto DUPLICATE;
            v = value;

            goto AFTER_SET;
        }
        throw std::runtime_error("unexpected type when setting Vector2 value");
    } else if (std::holds_alternative<Vector3>(variant)) {
        if constexpr (std::is_same_v<T, Vector3>) {
            auto& v = std::get<Vector3>(variant);
            if (value.x == v.x && value.y == v.y && value.z == v.z)
                goto DUPLICATE;
            v = value;

            goto AFTER_SET;
        }
        throw std::runtime_error("unexpected type when setting Vector3 value");
    } else if (std::holds_alternative<UDim>(variant)) {
        if constexpr (std::is_same_v<T, UDim>) {
            auto& v = std::get<UDim>(variant);
            if (value.scale == v.scale && value.offset == v.offset)
                goto DUPLICATE;
            v = value;

            goto AFTER_SET;
        }
        throw std::runtime_error("unexpected type when setting UDim value");
    } else if (std::holds_alternative<UDim2>(variant)) {
        if constexpr (std::is_same_v<T, UDim2>) {
            auto& v = std::get<UDim2>(variant);
            if (value.x.scale == v.x.scale && value.x.offset == v.x.offset && value.y.scale == v.y.scale && value.y.offset == v.y.offset)
                goto DUPLICATE;
            v = value;

            goto AFTER_SET;
        }
        throw std::runtime_error("unexpected type when setting UDim2 value");
    } else if (std::holds_alternative<std::shared_ptr<rbxInstance>>(variant)) {
        if constexpr (std::is_same_v<T, std::shared_ptr<rbxInstance>>) {
            auto& v = std::get<std::shared_ptr<rbxInstance>>(variant);

            if (value == v)
                goto DUPLICATE;

            if (name == PROP_INSTANCE_PARENT) {
                lock.unlock();
                setInstanceParent(L, instance, value, false, true);
                lock.lock();
            }

            v = value;
            goto AFTER_SET;
        }
        throw std::runtime_error("unexpected type when setting rbxInstance value");
    } else if (std::holds_alternative<rbxCallback>(variant))
        throw std::runtime_error("values of type rbxCallback cannot be set via setValue");

    #define handleType(type) {                                                             \
        if (std::holds_alternative<type>(variant)) {                                       \
            if constexpr (std::is_same_v<T, type>) {                                       \
                if (value == std::get<type>(variant))                                      \
                    goto DUPLICATE;                                                        \
            } else {                                                                       \
                throw std::runtime_error("unexpected type when setting " #type " value");  \
            }                                                                              \
        }                                                                                  \
    }

    handleType(bool)
    handleType(int32_t)
    handleType(int64_t)
    handleType(float)
    handleType(double)
    handleType(std::string)

    std::get<T>(variant) = value;

    goto AFTER_SET;
    AFTER_SET: ;

    lock.unlock();

    if (!dont_report_changed)
        instance->reportChanged(L, name.c_str());

    goto DUPLICATE;
    DUPLICATE: ;
}

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

std::shared_ptr<rbxInstance>& lua_checkinstance(lua_State* L, int narg, const char* class_name = nullptr);
std::shared_ptr<rbxInstance> lua_optinstance(lua_State* L, int narg, const char* class_name = nullptr);

void rbxInstanceSetup(lua_State* L, std::string api_jump);
void rbxInstanceCleanup(lua_State* L);

std::shared_ptr<rbxInstance> newInstance(lua_State* L, const char* class_name, std::shared_ptr<rbxInstance> parent = nullptr);
std::shared_ptr<rbxInstance> cloneInstance(lua_State* L, std::shared_ptr<rbxInstance> reference, bool is_deep = true, std::optional<std::map<std::shared_ptr<rbxInstance>, std::shared_ptr<rbxInstance>>*> cloned_map = std::nullopt);
int lua_pushinstance(lua_State* L, std::shared_ptr<rbxInstance> instance);

namespace rbxInstance_datatype {
    int _new(lua_State* L);
    int from_existing(lua_State* L);
}; // namespace rbxInstance_datatype

extern std::shared_ptr<rbxInstance> hiddenui;

}; // namespace fakeroblox
