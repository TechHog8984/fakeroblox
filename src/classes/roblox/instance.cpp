#include "classes/roblox/instance.hpp"
#include "classes/color3.hpp"
#include "classes/roblox/baseplayergui.hpp"
#include "classes/roblox/camera.hpp"
#include "classes/roblox/coregui.hpp"
#include "classes/roblox/datatypes/enum.hpp"
#include "classes/roblox/bindableevent.hpp"
#include "classes/roblox/datamodel.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/guiobject.hpp"
#include "classes/roblox/httpservice.hpp"
#include "classes/roblox/layercollector.hpp"
#include "classes/roblox/players.hpp"
#include "classes/roblox/runservice.hpp"
#include "classes/roblox/serviceprovider.hpp"
#include "classes/roblox/startergui.hpp"
#include "classes/roblox/tweenbase.hpp"
#include "classes/roblox/tweenservice.hpp"
#include "classes/roblox/userinputservice.hpp"
#include "classes/roblox/workspace.hpp"
#include "classes/tweeninfo.hpp"
#include "classes/vector2.hpp"
#include "classes/vector3.hpp"

#include "common.hpp"

#include "console.hpp"
#include "taskscheduler.hpp"
#include "ui/instanceexplorer.hpp"

#include "lobject.h"
#include "lua.h"
#include "lualib.h"
#include "lstate.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <variant>

namespace fakeroblox {

std::map<std::string, std::shared_ptr<rbxClass>> rbxClass::class_map;
std::vector<std::string> rbxClass::valid_class_names;
std::vector<std::string> rbxClass::valid_services;


std::shared_ptr<rbxProperty> rbxClass::newInternalProperty(const char* name, TypeCategory type_category, rbxValue default_value) {
    std::shared_ptr<rbxProperty> property = std::make_shared<rbxProperty>();

    property->internal = true;
    property->type_category = type_category;
    property->default_value = default_value;

    properties[name] = property;

    property->default_value.property = property;

    return property;
}

std::vector<std::weak_ptr<rbxInstance>> rbxInstance::instance_list;
std::shared_mutex rbxInstance::instance_list_mutex;

rbxInstance::rbxInstance(std::shared_ptr<rbxClass> _class) : _class(_class) {}

// lua_State* rbxInstance::destructorL = nullptr;

rbxInstance::~rbxInstance() {
    Console::ScriptConsole.debug("destroying instance...");

    /*
    // FIXME: we need to free events at some point, but the below code doesn't work because L is not guaranteed to allowstack manipulation

    // #define killEvent(event) {                               \
    //     pushEvent(destructorL, event);                                 \
    //     lua_checkrbxscriptsignal(destructorL, -1)->~rbxScriptSignal(); \
    //     lua_pop(destructorL, 1);                                       \
    // }

    // if (destructorL)
    //     for (auto& event : events)
    //         killEvent(event.c_str())
    */

    rbxClass* c = _class.get();
    while (c) {
        // if (destructorL)
        //     for (auto& property : c->properties)
        //         killEvent(property.first.c_str());

        if (c->destructor)
            c->destructor(this);
        c = c->superclass.get();
    }

    // #undef killEvent
}

bool rbxInstance::isA(rbxClass* target_class) {
    rbxClass* c = _class.get();
    while (c) {
        if (c == target_class)
            return true;
        c = c->superclass.get();
    }
    return false;
}
bool rbxInstance::isA(const char* class_name) {
    return isA(rbxClass::class_map.at(class_name).get());
}

int rbxInstance::pushEvent(lua_State* L, const char* name) {
    lua_getfield(L, LUA_REGISTRYINDEX, SIGNALLOOKUP);
    lua_pushlightuserdata(L, this);
    lua_rawget(L, -2);
    lua_pushstring(L, name);
    lua_rawget(L, -2);

    assert(!lua_isnil(L, -1));

    lua_remove(L, -2); // remove signallookup
    lua_remove(L, -2); // remove signallookup table
    return 1;
}
void rbxInstance::reportChanged(lua_State* L, const char* property) {
    pushFunctionFromLookup(L, fireRBXScriptSignal);
    pushEvent(L, "Changed");
    lua_pushstring(L, property);
    lua_call(L, 2, 0);

    pushFunctionFromLookup(L, fireRBXScriptSignal);
    pushEvent(L, property);
    lua_call(L, 1, 0);
}

void clearAllInstanceChildren(lua_State* L, std::shared_ptr<rbxInstance> instance) {
    std::lock_guard children_lock(instance->children_mutex);
    auto& children = instance->children;

    for (size_t i = 0; i < children.size(); i++)
        destroyInstance(L, children[i], true);

    children.clear();
}
void destroyInstance(lua_State* L, std::shared_ptr<rbxInstance> instance, bool dont_remove_from_old_parent_children) {
    std::lock_guard destroyed_lock(instance->destroyed_mutex);
    if (instance->destroyed)
        return;

    clearAllInstanceChildren(L, instance);

    for (auto& event : instance->events) {
        pushFunctionFromLookup(L, disconnectAllRBXScriptSignal);
        instance->pushEvent(L, event.c_str());
        lua_call(L, 1, 0);
    }
    rbxClass* c = instance->_class.get();
    while (c) {
        for (auto& property : c->properties) {
            pushFunctionFromLookup(L, disconnectAllRBXScriptSignal);
            instance->pushEvent(L, property.first.c_str());
            lua_call(L, 1, 0);
        }
        c = c->superclass.get();
    }

    setInstanceParent(L, instance, nullptr, dont_remove_from_old_parent_children);
    std::lock_guard parent_locked_lock(instance->parent_locked_mutex);

    instance->destroyed = true;
    instance->parent_locked = true;

    lua_getfield(L, LUA_REGISTRYINDEX, INSTANCELOOKUP);
    lua_pushlightuserdata(L, instance.get());
    lua_pushnil(L);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}
std::shared_ptr<rbxInstance> rbxInstance::findFirstChild(std::string name) {
    std::lock_guard children_lock(children_mutex);

    for (size_t i = 0; i < children.size(); i++)
        if (getInstanceValue<std::string>(children[i], PROP_INSTANCE_NAME) == name)
            return children[i];
    return nullptr;
}

void addInstanceToDescendantsList(std::vector<std::shared_ptr<rbxInstance>>& descendants, std::shared_ptr<rbxInstance> instance, bool skip_instance = false) {
    std::lock_guard children_lock(instance->children_mutex);

    descendants.reserve(descendants.capacity() + !skip_instance + instance->children.size());
    if (!skip_instance)
        descendants.push_back(instance);

    for (size_t i = 0; i < instance->children.size(); i++)
        addInstanceToDescendantsList(descendants, instance->children[i]);
}

std::vector<std::shared_ptr<rbxInstance>>getDescendants(std::shared_ptr<rbxInstance> instance) {
    std::lock_guard children_lock(instance->children_mutex);

    std::vector<std::shared_ptr<rbxInstance>> descendants;
    addInstanceToDescendantsList(descendants, instance, true);

    return descendants;
}

bool isDescendantOf(std::shared_ptr<rbxInstance> instance, std::shared_ptr<rbxInstance> other) {
    assert(other);

    auto parent = getInstanceValue<std::shared_ptr<rbxInstance>>(instance, PROP_INSTANCE_PARENT);
    if (!parent)
        return false;

    do {
        if (parent == other)
            return true;

        parent = getInstanceValue<std::shared_ptr<rbxInstance>>(parent, PROP_INSTANCE_PARENT);
    } while (parent);

    return false;
}

rbxValueVariant& getInstanceValueVariant(std::shared_ptr<rbxInstance> instance, const char* name) {
    std::lock_guard lock(instance->values_mutex);
    return instance->values.at(name).value;
}

// TODO: we should probably use this in Instance __index if we can
rbxValueVariant luaValueToValueVariant(lua_State* L, int idx, rbxValueVariant& reference) {
    assert(!lua_isnil(L, idx));

    if (std::holds_alternative<bool>(reference))
        return luaL_checkboolean(L, idx);
    else if (std::holds_alternative<int32_t>(reference)) {
        return static_cast<int32_t>(luaL_checkinteger(L, idx));
    } else if (std::holds_alternative<int64_t>(reference)) {
        return static_cast<int64_t>(luaL_checkinteger(L, idx));
    } else if (std::holds_alternative<float>(reference)) {
        return static_cast<float>(luaL_checknumber(L, idx));
    } else if (std::holds_alternative<double>(reference)) {
        return static_cast<double>(luaL_checknumber(L, idx));
    } else if (std::holds_alternative<std::string>(reference)) {
        size_t l;
        const char* str = luaL_checklstring(L, idx, &l);
        return std::string(str, l);
    } else if (std::holds_alternative<rbxCallback>(reference)) {
        luaL_checktype(L, idx, LUA_TFUNCTION);

        idx = lua_absindex(L, idx);
        lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
        const int index = addToLookup(L, [&L, &idx] {
             lua_pushvalue(L, idx);
        }, false);

        return rbxCallback{ .index = index };

    } else if (std::holds_alternative<EnumItemWrapper>(reference)) {
        const char* expected_enum = std::get<EnumItemWrapper>(reference).enum_name.c_str();
        const auto value = lua_checkenumitem(L, idx, expected_enum);
        return EnumItemWrapper{ .name = value->name, .enum_name = value->enum_name };

    } else if (std::holds_alternative<Color>(reference))
        return *lua_checkcolor(L, idx);
    else if (std::holds_alternative<ColorSequenceKeypoint>(reference))
        return *lua_checkcolorsequencekeypoint(L, idx);
    else if (std::holds_alternative<ColorSequence>(reference))
        return *lua_checkcolorsequence(L, idx);
    else if (std::holds_alternative<NumberRange>(reference))
        return *lua_checknumberrange(L, idx);
    else if (std::holds_alternative<NumberSequenceKeypoint>(reference))
        return *lua_checknumbersequencekeypoint(L, idx);
    else if (std::holds_alternative<NumberSequence>(reference))
        return *lua_checknumbersequence(L, idx);
    else if (std::holds_alternative<Rect>(reference))
        return *lua_checkrect(L, idx);
    else if (std::holds_alternative<TweenInfo>(reference))
        return *lua_checktweeninfo(L, idx);
    else if (std::holds_alternative<UDim>(reference))
        return *lua_checkudim(L, idx);
    else if (std::holds_alternative<UDim2>(reference))
        return *lua_checkudim2(L, idx);
    else if (std::holds_alternative<Vector2>(reference))
        return *lua_checkvector2(L, idx);
    else if (std::holds_alternative<Vector3>(reference))
        return *lua_checkvector3(L, idx);

    else if (std::holds_alternative<std::shared_ptr<rbxInstance>>(reference))
        return lua_checkinstance(L, idx);
    else
        assert(!"UNHANDLED ALTERNATIVE FOR DATATYPE VALUE");
}

void setInstanceValueVariant(std::shared_ptr<rbxInstance> instance, lua_State* L, const char* name, rbxValueVariant value, bool dont_report_changed) {
    #define handleType(type) if (std::holds_alternative<type>(value))                           \
        setInstanceValue<type>(instance, L, name, std::get<type>(value), dont_report_changed);  \

    handleType(bool)
    else handleType(int32_t)
    else handleType(int64_t)
    else handleType(float)
    else handleType(double)
    else handleType(std::string)

    else handleType(rbxCallback)
    else handleType(std::shared_ptr<rbxInstance>)

    else handleType(EnumItemWrapper)
    else handleType(Color)
    else handleType(TweenInfo)
    else handleType(ColorSequenceKeypoint)
    else handleType(ColorSequence)
    else handleType(NumberRange)
    else handleType(NumberSequenceKeypoint)
    else handleType(NumberSequence)
    else handleType(Rect)
    else handleType(UDim)
    else handleType(UDim)
    else handleType(Vector2)
    else handleType(Vector3)

    #undef handleType
}

std::shared_ptr<rbxInstance>& lua_checkinstance(lua_State* L, int narg, const char* class_name) {
    void* ud = luaL_checkudatareal(L, narg, "Instance");
    SharedPtrObject* object = static_cast<SharedPtrObject*>(ud);
    auto instance = static_cast<std::shared_ptr<rbxInstance>*>(object->object);

    if (class_name && !(*instance)->isA(class_name)) {
        Closure* cl = L->ci > L->base_ci ? curr_func(L) : NULL;
        assert(cl);
        assert(cl->isC);
        const char* debugname = cl->c.debugname + 0;

        const char* fmt = "Expected ':' not '.' calling member function %s";
        int size = snprintf(NULL, 0, fmt, debugname);
        char* msg = static_cast<char*>(malloc(size * sizeof(char)));
        snprintf(msg, size + 1, fmt, debugname);

        lua_pushlstring(L, msg, size);
        free(msg);
        lua_error(L);
    }

    return *instance;
}
std::shared_ptr<rbxInstance> lua_optinstance(lua_State* L, int narg, const char* class_name) {
    if (lua_isnoneornil(L, narg))
        return nullptr;

    luaL_argexpected(L, lua_isuserdata(L, narg), narg, "userdata or nil");

    return lua_checkinstance(L, narg, class_name);
}

namespace rbxInstance_methods {
    static int clearAllChildren(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);

        clearAllInstanceChildren(L, instance);
        return 0;
    }
    static int clone(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);

        lua_pushinstance(L, cloneInstance(L, instance));
        return 1;
    }
    static int destroy(lua_State *L) {
        auto instance = lua_checkinstance(L, 1);

        destroyInstance(L, instance);
        return 0;
    }
    static int findFirstChild(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);
        const char* name = luaL_checkstring(L, 2);

        lua_pushinstance(L, instance->findFirstChild(name));
        return 1;
    }
    static int getChildren(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);

        std::lock_guard children_lock(instance->children_mutex);
        auto& children = instance->children;

        lua_createtable(L, children.size(), 0);

        for (size_t i = 0; i < children.size(); i++) {
            auto& child = children[i];
            lua_pushinstance(L, child);
            lua_rawseti(L, -2, i + 1);
        }

        return 1;
    }
    static int getDescendants(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);

        auto descendants = getDescendants(instance);

        lua_createtable(L, descendants.size(), 0);

        for (size_t i = 0; i < descendants.size(); i++) {
            auto& child = descendants[i];
            lua_pushinstance(L, child);
            lua_rawseti(L, -2, i + 1);
        }

        return 1;
    }
    static int getFullName(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);

        std::string result;
        std::shared_ptr<rbxInstance> inst = instance;
        do {
            result.insert(result.begin(), '.');
            result.insert(0, getInstanceValue<std::string>(inst, PROP_INSTANCE_NAME));
            inst = getInstanceValue<std::shared_ptr<rbxInstance>>(inst, PROP_INSTANCE_PARENT);
        } while (inst);

        size_t last = result.size() - 1;
        if (!result.empty() && result.at(last) == '.')
            result.erase(last);

        lua_pushlstring(L, result.c_str(), result.size());
        return 1;
    }
    static int getPropertyChangedSignal(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);
        const char* key = luaL_checkstring(L, 2);

        if (instance->values.find(key) == instance->values.end())
            luaL_error(L, "%s is not a valid property name.", key);

        auto value = &instance->values[key];
        auto& property = value->property;
        if (property->route)
            value = &instance->values[*property->route];

        if (property->tags & rbxProperty::NotScriptable)
            luaL_error(L, "%s is not a scriptable property.", key);

        return instance->pushEvent(L, key);
    }
    static int isA(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);
        const char* class_name = luaL_checkstring(L, 2);

        lua_pushboolean(L, instance->isA(class_name));
        return 1;
    }
    static int isDescendantOf(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);
        auto other = lua_checkinstance(L, 2);

        lua_pushboolean(L, isDescendantOf(instance, other));
        return 1;
    }

    static int waitForChild(lua_State* L) {
        auto instance = lua_checkinstance(L, 1);
        std::string name = luaL_checkstring(L, 2);
        const double timeout = luaL_optnumber(L, 3, 5);

        auto child = instance->findFirstChild(name);
        if (child) {
            lua_pushinstance(L, child);
            return 1;
        }

        return TaskScheduler::yieldForWork(L, [instance, name, timeout] (lua_State* thread) {
            auto start = std::chrono::system_clock::now();

            auto child = instance->findFirstChild(name);
            while (!child) {
                if (std::chrono::duration<double>(std::chrono::system_clock::now() - start).count() >= timeout) {
                    getTask(thread)->console->warningf("TODO this message lol; infinite yield possible while waiting for child \"%.*s\"", static_cast<int>(name.size()), name.c_str());
                    return 0;
                }
                child = instance->findFirstChild(name);
            }

            lua_pushinstance(thread, child);
            return 1;
        });
    }
}; // namespace rbxInstance_methods

int rbxInstance__tostring(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    auto name = getInstanceValue<std::string>(instance, PROP_INSTANCE_NAME);
    lua_pushlstring(L, name.c_str(), name.size());
    return 1;
}

int pushMethod(lua_State* L, std::shared_ptr<rbxInstance>& instance, std::string method_name) {
    rbxMethod method = instance->methods[method_name];
    if (method.route)
        method = instance->methods[*method.route];

    assert(!method.route);

    if (method.func)
        return pushFunctionFromLookup(L, method.func, method.name.c_str(), method.cont);
    else
        luaL_error(L, "INTERNAL ERROR: TODO implement '%s'", method.name.c_str());

    return 0;
}

int rbxInstance__index(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);

    auto class_name = getInstanceValue<std::string>(instance, PROP_INSTANCE_CLASS_NAME);
    if (instance->values.find(key) == instance->values.end()) {
        if (instance->methods.find(key) != instance->methods.end())
            return pushMethod(L, instance, key);
        if (std::find(instance->events.begin(), instance->events.end(), key) != instance->events.end())
            return instance->pushEvent(L, key);

        auto child = instance->findFirstChild(key);
        if (child) {
            lua_pushinstance(L, child);
            return 1;
        }
        goto INVALID_MEMBER;
    }

    {
    auto value = &instance->values[key];
    auto property = value->property;
    if (property->route)
        value = &instance->values[*property->route];

    if (property->internal)
        goto INVALID_MEMBER;

    if (property->tags & rbxProperty::NotScriptable)
        goto INVALID_MEMBER;

    if (property->tags & rbxProperty::WriteOnly)
        luaL_error(L, "'%s' is a write-only member of %s", key, class_name.c_str());

    std::lock_guard values_lock(instance->values_mutex);

    if (std::holds_alternative<std::monostate>(value->value))
        lua_pushnil(L);
    else {
        switch (property->type_category) {
            case Primitive:
                if (std::holds_alternative<bool>(value->value))
                    lua_pushboolean(L, std::get<bool>(value->value));
                else if (std::holds_alternative<int32_t>(value->value))
                    lua_pushinteger(L, std::get<int32_t>(value->value));
                else if (std::holds_alternative<int64_t>(value->value))
                    lua_pushinteger(L, std::get<int64_t>(value->value));
                else if (std::holds_alternative<float>(value->value))
                    lua_pushnumber(L, std::get<float>(value->value));
                else if (std::holds_alternative<double>(value->value))
                    lua_pushnumber(L, std::get<double>(value->value));
                else if (std::holds_alternative<std::string>(value->value)) {
                    std::string str = std::get<std::string>(value->value);
                    lua_pushlstring(L, str.c_str(), str.size());
                } else if (std::holds_alternative<rbxCallback>(value->value)) {
                    // auto& wrapper = std::get<LuaFunctionWrapper>(value->value);
                    // if (wrapper.index == -1) {
                    //     lua_pushnil(L);
                    // } else {
                    //     lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
                    //     lua_rawgeti(L, -1, wrapper.index);
                    //     lua_remove(L, -2);
                    // }

                    luaL_error(L, "%s is a callback member of %s; you can only set the callback value, get is not available", key, class_name.c_str());
                } else
                    assert(!"UNHANDLED ALTERNATIVE FOR PROPERTY VALUE");
                break;
            case DataType: {
                if (std::holds_alternative<EnumItemWrapper>(value->value))
                    assert(pushEnumItem(L, std::get<EnumItemWrapper>(value->value)) == 1);

                else if (std::holds_alternative<Color>(value->value))
                    assert(pushColor(L, std::get<Color>(value->value)) == 1);
                else if (std::holds_alternative<TweenInfo>(value->value))
                    assert(pushTweenInfo(L, std::get<TweenInfo>(value->value)) == 1);
                else if (std::holds_alternative<ColorSequenceKeypoint>(value->value))
                    assert(pushColorSequenceKeypoint(L, std::get<ColorSequenceKeypoint>(value->value)) == 1);
                else if (std::holds_alternative<ColorSequence>(value->value))
                    assert(pushColorSequence(L, std::get<ColorSequence>(value->value)) == 1);
                else if (std::holds_alternative<NumberRange>(value->value))
                    assert(pushNumberRange(L, std::get<NumberRange>(value->value)) == 1);
                else if (std::holds_alternative<NumberSequenceKeypoint>(value->value))
                    assert(pushNumberSequenceKeypoint(L, std::get<NumberSequenceKeypoint>(value->value)) == 1);
                else if (std::holds_alternative<NumberSequence>(value->value))
                    assert(pushNumberSequence(L, std::get<NumberSequence>(value->value)) == 1);
                else if (std::holds_alternative<Rect>(value->value))
                    assert(pushRect(L, std::get<Rect>(value->value)) == 1);
                else if (std::holds_alternative<UDim>(value->value))
                    assert(pushUDim(L, std::get<UDim>(value->value)) == 1);
                else if (std::holds_alternative<UDim2>(value->value))
                    assert(pushUDim2(L, std::get<UDim2>(value->value)) == 1);
                else if (std::holds_alternative<Vector2>(value->value))
                    assert(pushVector2(L, std::get<Vector2>(value->value)) == 1);
                else if (std::holds_alternative<Vector3>(value->value))
                    assert(pushVector3(L, std::get<Vector3>(value->value)) == 1);
                else
                    assert("!UNHANDLED ALTERNATIVE FOR DATATYPE VALUE");

                break;
            } case Instance:
                lua_pushinstance(L, std::get<std::shared_ptr<rbxInstance>>(value->value));
                break;
        }
    }

    return 1;
    }

    INVALID_MEMBER:
    auto name = getInstanceValue<std::string>(instance, PROP_INSTANCE_NAME);
    luaL_error(L, "%s is not a valid member of %s \"%s\"", key, class_name.c_str(), name.c_str());
};

const char* getOptionalInstanceName(std::shared_ptr<rbxInstance> instance) {
    if (!instance)
        return "NULL";
    return getInstanceValue<std::string>(instance, PROP_INSTANCE_NAME).c_str();
}

void setInstanceParent(lua_State* L, std::shared_ptr<rbxInstance> instance, std::shared_ptr<rbxInstance> new_parent, bool dont_remove_from_old_parent_children, bool dont_set_value) {
    std::shared_ptr<rbxInstance> old_parent = getInstanceValue<std::shared_ptr<rbxInstance>>(instance, PROP_INSTANCE_PARENT);

    std::shared_lock parent_locked_lock(instance->parent_locked_mutex);
    if (instance->parent_locked)
        luaL_error(L, "The Parent property of %s is locked, current parent: %s, new parent %s", getOptionalInstanceName(instance), getOptionalInstanceName(old_parent), getOptionalInstanceName(new_parent));

    if (new_parent == instance)
        luaL_error(L, "Attempt to set %s as its own parent", getOptionalInstanceName(instance));

    if (old_parent && !dont_remove_from_old_parent_children) {
        std::lock_guard old_parent_children_lock(old_parent->children_mutex);

        old_parent->children.erase(std::find(old_parent->children.begin(), old_parent->children.end(), instance));
    }

    if (new_parent) {
        std::lock_guard parent_children_lock(new_parent->children_mutex);

        new_parent->children.push_back(instance);
    }

    if (!dont_set_value)
        std::get<std::shared_ptr<rbxInstance>>(instance->values[PROP_INSTANCE_PARENT].value) = new_parent;
}

static int fr_getinstances(lua_State* L) {
    std::lock_guard lock(rbxInstance::instance_list_mutex);

    lua_newtable(L);

    for (size_t i = 0; i < rbxInstance::instance_list.size(); i++) {
        auto child = rbxInstance::instance_list[i].lock();
        if (!child)
            continue;
        lua_pushnumber(L, i + 1);
        lua_pushinstance(L, child);
        lua_settable(L, -3);
    }

    return 1;
}
std::vector<std::weak_ptr<rbxInstance>> getNilInstances() {
    std::lock_guard lock(rbxInstance::instance_list_mutex);

    std::vector<std::weak_ptr<rbxInstance>> nil_instances;

    for (size_t i = 0; i < rbxInstance::instance_list.size(); i++) {
        auto child = rbxInstance::instance_list[i].lock();
        if (!child)
            continue;
        if (getInstanceValue<std::shared_ptr<rbxInstance>>(child, PROP_INSTANCE_PARENT))
            continue;

        nil_instances.push_back(child);
    }

    return nil_instances;
}
static int fr_getnilinstances(lua_State* L) {
    auto nil_instances = getNilInstances();

    lua_newtable(L);

    for (size_t i = 0; i < nil_instances.size(); i++) {
        auto child = nil_instances[i].lock();
        if (!child)
            continue;

        lua_pushnumber(L, i + 1);
        lua_pushinstance(L, child);
        lua_settable(L, -3);
    }

    return 1;
}

int rbxInstance__newindex(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);
    luaL_checkany(L, 3);

    auto class_name = getInstanceValue<std::string>(instance, PROP_INSTANCE_CLASS_NAME);
    if (instance->values.find(key) == instance->values.end())
        goto INVALID_MEMBER;

    {
    auto value = &instance->values[key];
    auto property = value->property;
    if (property->route)
        value = &instance->values[*property->route];

    if (property->internal)
        goto INVALID_MEMBER;

    if (property->tags & rbxProperty::NotScriptable)
        goto INVALID_MEMBER;

    if (property->tags & rbxProperty::ReadOnly)
        // luaL_error(L, "'%s' is a read-only member of %s", key, class_name.c_str());
        luaL_error(L, "Unable to assign property %s. Property is read only", key);

    switch (property->type_category) {
        case Primitive:
            if (std::holds_alternative<bool>(value->value)) {
                const bool new_value = lua_toboolean(L, 3);
                setInstanceValue(instance, L, key, new_value);
            } else if (std::holds_alternative<int32_t>(value->value)) {
                int isnum;
                const int32_t new_value = lua_tointegerx(L, 3, &isnum);
                if (!isnum)
                    getTask(L)->console->warningf("value of type %s cannot be converted to a number", luaL_typename(L, 3));
                setInstanceValue(instance, L, key, new_value);
            } else if (std::holds_alternative<int64_t>(value->value)) {
                int isnum;
                const int64_t new_value = lua_tointegerx(L, 3, &isnum);
                if (!isnum)
                    getTask(L)->console->warningf("value of type %s cannot be converted to a number", luaL_typename(L, 3));
                setInstanceValue(instance, L, key, new_value);
            } else if (std::holds_alternative<float>(value->value)) {
                int isnum;
                const float new_value = lua_tonumberx(L, 3, &isnum);
                if (!isnum)
                    getTask(L)->console->warningf("value of type %s cannot be converted to a number", luaL_typename(L, 3));
                setInstanceValue(instance, L, key, new_value);
            } else if (std::holds_alternative<double>(value->value)) {
                int isnum;
                const double new_value = lua_tonumberx(L, 3, &isnum);
                if (!isnum)
                    getTask(L)->console->warningf("value of type %s cannot be converted to a number", luaL_typename(L, 3));
                setInstanceValue(instance, L, key, new_value);
            } else if (std::holds_alternative<std::string>(value->value)) {
                size_t l;
                const char* str = luaL_tolstring(L, 3, &l);
                if (str == NULL)
                    luaL_error(L, "Unable to assign property %s. string expected, got %s", key, luaL_typename(L, 3));

                const std::string new_value = std::string(str, l);
                setInstanceValue(instance, L, key, new_value);
            } else if (std::holds_alternative<rbxCallback>(value->value)) {
                if (lua_isnil(L, 3))
                    goto SKIP;

                // NOTE: Roblox does not do this! the value will be set to whatever type
                luaL_checktype(L, 3, LUA_TFUNCTION);
                auto& current = std::get<rbxCallback>(value->value);

                lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
                const int new_value = addToLookup(L, [&L] {
                     lua_pushvalue(L, 3);
                }, false);
                if (new_value == current.index)
                    goto SKIP;

                current.index = new_value;
            } else
                assert(!"UNHANDLED ALTERNATIVE FOR PROPERTY VALUE");
            break;
        case DataType:
            if (std::holds_alternative<std::monostate>(value->value))
                // TODO: why did i create this branch lol....
                ;
            if (std::holds_alternative<EnumItemWrapper>(value->value)) {
                const char* expected_enum = std::get<EnumItemWrapper>(value->value).enum_name.c_str();
                const std::string new_value = lua_checkenumitem(L, 3, expected_enum)->name;
                setInstanceValue(instance, L, key, new_value);

            } else if (std::holds_alternative<Color>(value->value)) {
            // TODO: (for all of these types) use to* not check* and error "Unable to assign property %skey. %stype expected, got %stypename3"
                const auto new_value = lua_checkcolor(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<TweenInfo>(value->value)) {
                const auto new_value = lua_checktweeninfo(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<ColorSequenceKeypoint>(value->value)) {
                const auto new_value = lua_checkcolorsequencekeypoint(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<ColorSequence>(value->value)) {
                const auto new_value = lua_checkcolorsequence(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<NumberRange>(value->value)) {
                const auto new_value = lua_checknumberrange(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<NumberSequenceKeypoint>(value->value)) {
                const auto new_value = lua_checknumbersequencekeypoint(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<NumberSequence>(value->value)) {
                const auto new_value = lua_checknumbersequence(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<Rect>(value->value)) {
                const auto new_value = lua_checkrect(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<UDim>(value->value)) {
                const auto new_value = lua_checkudim(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<UDim2>(value->value)) {
                const auto new_value = lua_checkudim2(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<Vector2>(value->value)) {
                const auto new_value = lua_checkvector2(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else if (std::holds_alternative<Vector3>(value->value)) {
                const auto new_value = lua_checkvector3(L, 3);
                setInstanceValue(instance, L, key, *new_value);
            } else
                assert(!"UNHANDLED ALTERNATIVE FOR DATATYPE VALUE");

            break;
        case Instance: {
            std::shared_ptr<rbxInstance> new_value = lua_optinstance(L, 3);
            setInstanceValue(instance, L, key, new_value);
            break;
        }

    }
    }

    SKIP:

    return 0;

    INVALID_MEMBER:
    auto name = getInstanceValue<std::string>(instance, PROP_INSTANCE_NAME);
    luaL_error(L, "%s is not a valid member of %s \"%s\"", key, class_name.c_str(), name.c_str());
};
int rbxInstance__namecall(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");
    std::string method_name = namecall;

    if (instance->methods.find(method_name) == instance->methods.end()) {
        auto name = getInstanceValue<std::string>(instance, PROP_INSTANCE_NAME);
        auto class_name = getInstanceValue<std::string>(instance, PROP_INSTANCE_CLASS_NAME);
        luaL_error(L, "%s is not a valid member of %s \"%s\"", namecall, class_name.c_str(), name.c_str());
    }

    lua_CFunction func = instance->methods[method_name].func;
    if (func)
        return instance->methods[method_name].func(L);
    else
        luaL_error(L, "INTERNAL ERROR: TODO implement '%s'", method_name.c_str());
}

std::shared_ptr<rbxInstance> newInstance(lua_State* L, const char* class_name, std::shared_ptr<rbxInstance> parent) {
    std::shared_ptr<rbxClass> _class = rbxClass::class_map[class_name];
    std::shared_ptr<rbxInstance> instance = std::make_shared<rbxInstance>(_class);

    lua_newtable(L);

    lua_getfield(L, LUA_REGISTRYINDEX, SIGNALLOOKUP);
    lua_pushlightuserdata(L, instance.get());
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);

    lua_pop(L, 1);

    rbxClass* c = _class.get();
    while (c) {
        for (auto& property : c->properties) {
            instance->values[property.first] = property.second->default_value;
            // for GetPropertyChangedSignal
            pushNewRBXScriptSignal(L, property.first);
            lua_rawsetfield(L, -2, property.first.c_str());
        }
        instance->methods.insert(c->methods.begin(), c->methods.end());
        instance->events.insert(instance->events.end(), c->events.begin(), c->events.end());
        for (auto& event : c->events) {
            pushNewRBXScriptSignal(L, event);
            lua_rawsetfield(L, -2, event.c_str());
        }
        if (c->constructor)
            c->constructor(L, instance);
        c = c->superclass.get();
    }
    lua_pop(L, 1); // signallookup table

    // FIXME: more default values
    // FIXME: unique_id
    setInstanceValue<bool>(instance, L, PROP_INSTANCE_ARCHIVABLE, true, true);
    setInstanceValue<std::string>(instance, L, PROP_INSTANCE_NAME, class_name, true);
    setInstanceValue<std::string>(instance, L, PROP_INSTANCE_CLASS_NAME, class_name, true);
    setInstanceParent(L, instance, parent);

    std::lock_guard instance_list_lock(rbxInstance::instance_list_mutex);
    rbxInstance::instance_list.push_back(instance);

    return instance;
}
std::shared_ptr<rbxInstance> cloneInstance(lua_State* L, std::shared_ptr<rbxInstance> reference, bool is_deep, std::optional<std::map<std::shared_ptr<rbxInstance>, std::shared_ptr<rbxInstance>>*> cloned_map) {
    auto instance = newInstance(L, getInstanceValue<std::string>(reference, PROP_INSTANCE_CLASS_NAME).c_str());
    if (is_deep && !getInstanceValue<bool>(reference, PROP_INSTANCE_ARCHIVABLE))
        return nullptr;

    rbxClass* c = instance->_class.get();
    while (c) {
        for (auto& property : c->properties)
            instance->values[property.first].value = reference->values[property.first].value;
        c = c->superclass.get();
    }
    instance->values[PROP_INSTANCE_PARENT].value = std::shared_ptr<rbxInstance>(nullptr);

    if (is_deep) {
        // FIXME: I think the fact that this function spends time with the lock unlocked is inheritly flawed.
        // Unlocking then locking again leaves time in the middle for actions to be performed from other threads.
        // ^ not specific to this function; happens in other places too

        std::shared_lock children_lock(reference->children_mutex);
        auto& children = reference->children;

        // kinda hacky
        std::map<std::shared_ptr<rbxInstance>, std::shared_ptr<rbxInstance>> this_cloned_map;
        if (!cloned_map)
            cloned_map = &this_cloned_map;

        (**cloned_map)[reference] = instance;

        // store cloned children in a vector to get around mutex issues
        std::vector<std::shared_ptr<rbxInstance>> cloned_children;
        cloned_children.reserve(children.size());

        for (size_t i = 0; i < children.size(); i++) {
            auto cloned = cloneInstance(L, children[i], true, cloned_map);
            if (!cloned) continue;
            cloned_children.push_back(cloned);
        }

        children_lock.unlock();

        for (size_t i = 0; i < cloned_children.size(); i++)
            setInstanceParent(L, cloned_children[i], instance, true);

        children_lock.lock();

        for (auto& pair : **cloned_map) {
            // TODO: does it help or hurt performance for this `c` to be the same as the one above?
            c = pair.second->_class.get();
            while (c) {
                for (auto& property : c->properties)
                    if (property.second->type_category == Instance && property.first != PROP_INSTANCE_PARENT) {
                        auto value = getInstanceValue<std::shared_ptr<rbxInstance>>(pair.second, property.first.c_str());
                        auto it = (*cloned_map)->find(value);
                        if (it != (*cloned_map)->end())
                            setInstanceValue<std::shared_ptr<rbxInstance>>(pair.second, L, property.first.c_str(), it->second, true);
                    }
                c = c->superclass.get();
            }
        }
    }

    return instance;
}

int lua_pushinstance(lua_State* L, std::shared_ptr<rbxInstance> instance) {
    if (!instance) {
        lua_pushnil(L);
        return 1;
    }

    return pushFromSharedPtrLookup(L, INSTANCELOOKUP, instance, [&L] {
        luaL_getmetatable(L, "Instance");
        lua_setmetatable(L, -2);
    });
}
namespace rbxInstance_datatype {
    int _new(lua_State* L) {
        const char* class_name = luaL_checkstring(L, 1);
        std::shared_ptr<rbxInstance> parent = lua_optinstance(L, 2);

        if (rbxClass::class_map.find(class_name) == rbxClass::class_map.end()) {
            // std::string msg = "'";
            // msg.append(class_name) += '\'';
            // msg.append(" is not a valid class name");
            // luaL_error(L, "invalid arg #1 to new: '%.*s' is not a valid classname", static_cast<int>(msg.size()), msg.c_str());
            luaL_error(L, "Unable to create an Instance of type \"%s\"", class_name);
        }

        std::shared_ptr<rbxClass> _class = rbxClass::class_map[class_name];
        if (_class->tags & rbxClass::NotCreatable)
            luaL_error(L, "Unable to create an Instance of type \"%s\"", class_name);

        std::shared_ptr<rbxInstance> instance = newInstance(L, class_name, parent);
        return lua_pushinstance(L, instance);
    }
    int fromExisting(lua_State* L) {
        std::shared_ptr<rbxInstance> reference = lua_checkinstance(L, 1);

        return lua_pushinstance(L, cloneInstance(L, reference, false));
    }
}; // namespace rbxInstance_datatype

static int fr_getcallbackvalue(lua_State* L) {
    auto instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);

    auto class_name = getInstanceValue<std::string>(instance, PROP_INSTANCE_CLASS_NAME);
    if (instance->values.find(key) == instance->values.end()) {
        auto name = getInstanceValue<std::string>(instance, PROP_INSTANCE_NAME);
        luaL_error(L, "%s is not a valid member of %s \"%s\"", key, class_name.c_str(), name.c_str());
    }
    auto value = &instance->values[key];

    auto& wrapper = std::get<rbxCallback>(value->value);
    if (wrapper.index == -1)
        lua_pushnil(L);
    else {
        lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
        lua_rawgeti(L, -1, wrapper.index);
        lua_remove(L, -2);
    }

    return 1;
}

static int fr_isscriptable(lua_State* L) {
    auto instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);

    auto class_name = getInstanceValue<std::string>(instance, PROP_INSTANCE_CLASS_NAME);
    if (instance->values.find(key) == instance->values.end()) {
        auto name = getInstanceValue<std::string>(instance, PROP_INSTANCE_NAME);
        luaL_error(L, "%s is not a valid member of %s \"%s\"", key, class_name.c_str(), name.c_str());
    }
    auto value = &instance->values[key];
    auto property = value->property;

    lua_pushboolean(L, !(property->tags & rbxProperty::NotScriptable));
    return 1;
}
static int fr_setscriptable(lua_State* L) {
    auto instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);
    const bool new_value = luaL_checkboolean(L, 3);

    auto class_name = getInstanceValue<std::string>(instance, PROP_INSTANCE_CLASS_NAME);
    if (instance->values.find(key) == instance->values.end()) {
        auto name = getInstanceValue<std::string>(instance, PROP_INSTANCE_NAME);
        luaL_error(L, "%s is not a valid member of %s \"%s\"", key, class_name.c_str(), name.c_str());
    }
    auto value = &instance->values[key];
    auto& property = value->property;

    const bool old = !(property->tags & rbxProperty::NotScriptable);

    if (new_value)
        property->tags &= ~rbxProperty::NotScriptable;
    else
        property->tags |= rbxProperty::NotScriptable;

    lua_pushboolean(L, old);
    return 1;
}

static int fr_gethui(lua_State* L) {
    return lua_pushinstance(L, hiddenui);
}

void rbxInstanceSetup(lua_State* L, std::string api_dump) {
    // rbxInstance::destructorL = TaskScheduler::newThread(L, [] (std::string error) { Console::ScriptConsole.error(error); });

    // instancelookup
    newweaktable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, INSTANCELOOKUP);

    setup_rbxScriptSignal(L);

    // TODO: something in imgui for json stuff (all `rbxClass`s and properties, enums, etc)
    json api_json = json::parse(api_dump);
    rbxClass::valid_class_names.reserve(api_json["Classes"].size());

    std::map<std::string, std::string> superclass_map;

    for (auto& class_json : api_json["Classes"]) {
        std::string class_name = class_json["Name"].template get<std::string>();
        rbxClass::valid_class_names.push_back(class_name);

        std::shared_ptr<rbxClass> _class = std::make_shared<rbxClass>();
        _class->name.assign(class_name);

        auto& superclass_json = class_json["Superclass"];
        if (superclass_json.type() == json::value_t::string)
            superclass_map.try_emplace(class_name, superclass_json.template get<std::string>());

        for (auto& tag_json : class_json["Tags"]) {
            if (tag_json.type() == json::value_t::string) { // TODO: investigate when this isn't string (maybe just when Tags isn't not present?)
                std::string tag = tag_json.template get<std::string>();
                if (tag == "NotCreatable")
                    _class->tags |= rbxClass::NotCreatable;
                else if (tag == "Service")
                    ServiceProvider::registerService(class_name.c_str());
            }
        }

        for (auto& member_json : class_json["Members"]) {
            std::string member_name = member_json["Name"].template get<std::string>();
            std::string member_type = member_json["MemberType"].template get<std::string>();

            auto& tags = member_json["Tags"];
            if (member_type == "Property") {
                std::shared_ptr<rbxProperty> property = std::make_shared<rbxProperty>();
                if (tags.type() == json::value_t::array) {
                    for (auto& tag_json : tags) {
                        if (tag_json.type() == json::value_t::string) {
                            std::string tag = tag_json.template get<std::string>();
                            if (tag == "Hidden")
                                property->tags |= rbxProperty::Hidden;
                            else if (tag == "Deprecated")
                                property->tags |= rbxProperty::Deprecated;
                            else if (tag == "ReadOnly")
                                property->tags |= rbxProperty::ReadOnly;
                            else if (tag == "WriteOnly")
                                property->tags |= rbxProperty::WriteOnly;
                            else if (tag == "NotScriptable")
                                property->tags |= rbxProperty::NotScriptable;
                        } else
                            property->route = tag_json["PreferredDescriptorName"].template get<std::string>();
                    }
                }

                std::string category = member_json["ValueType"]["Category"].template get<std::string>();
                std::string type = member_json["ValueType"]["Name"].template get<std::string>();
                if (category == "Primitive") {
                    property->type_category = Primitive;
                    property->default_value = rbxValue();

                    if (type == "bool") {
                        property->default_value.value = false;
                    } else if (type == "int") {
                        property->default_value.value = int32_t(0);
                    } else if (type == "int64") {
                        property->default_value.value = int64_t(0);
                    } else if (type == "float") {
                        property->default_value.value = float(0.0);
                    } else if (type == "double") {
                        property->default_value.value = double(0.0);
                    } else if (type == "string") {
                        property->default_value.value = "";
                    }
                } else if (category == "Enum") {
                    property->type_category = DataType;
                    property->default_value = rbxValue();

                    property->default_value.value = EnumItemWrapper { .name = "", .enum_name = member_name };
                } else if (category == "DataType") {
                    property->type_category = DataType;
                    property->default_value = rbxValue();

                    // FIXME: all datatypes
                    if (type == "Color3")
                        property->default_value.value = Color{255, 255, 255, 255};
                    else if (type == "TweenInfo")
                        property->default_value.value = TweenInfo();
                    else if (type == "ColorSequenceKeypoint")
                        property->default_value.value = ColorSequenceKeypoint{0, 0, 0};
                    else if (type == "ColorSequence")
                        property->default_value.value = ColorSequence();
                    else if (type == "NumberRange")
                        property->default_value.value = NumberRange();
                    else if (type == "NumberSequenceKeypoint")
                        property->default_value.value = NumberSequenceKeypoint{0, 0, 0};
                    else if (type == "NumberSequence")
                        property->default_value.value = NumberSequence();
                    else if (type == "Rect")
                        property->default_value.value = Rect{0, 0, 0, 0};
                    else if (type == "UDim")
                        property->default_value.value = UDim{0, 0};
                    else if (type == "UDim2")
                        property->default_value.value = UDim2{{0, 0}, {0, 0}};
                    else if (type == "Vector2")
                        property->default_value.value = Vector2{0, 0};
                    else if (type == "Vector3")
                        property->default_value.value = Vector3{0, 0, 0};
                } else if (category == "Class") {
                    property->type_category = Instance;
                    property->default_value = rbxValue();
                    property->default_value.value = std::shared_ptr<rbxInstance>(nullptr);
                }

                _class->properties[member_name] = property;
                property->default_value.property = property;
            } else if (member_type == "Callback") {
                std::shared_ptr<rbxProperty> property = std::make_shared<rbxProperty>();
                property->type_category = Primitive;
                property->default_value = rbxValue();
                property->default_value.value = rbxCallback { .index = -1 };

                _class->properties[member_name] = property;
                property->default_value.property = property;
            } else if (member_type == "Function") {
                rbxMethod method;
                method.name = member_name;

                if (tags.type() == json::value_t::array) {
                    for (auto& tag_json : tags) {
                        if (tag_json.type() == json::value_t::string) {
                        } else
                            method.route = tag_json["PreferredDescriptorName"].template get<std::string>();
                    }
                }
                _class->methods.try_emplace(member_name, method);
            } else if (member_type == "Event") {
                _class->events.push_back(member_name);
            }
        }

        rbxClass::class_map[class_name] = _class;
    }

    for (auto& pair : superclass_map)
        rbxClass::class_map[pair.first]->superclass = rbxClass::class_map[pair.second];

    superclass_map.clear();

    for (auto& enum_json : api_json["Enums"]) {
        std::string enum_name = enum_json["Name"].template get<std::string>();

        Enum enums;
        enums.name = enum_name;

        for (auto& item_json : enum_json["Items"]) {
            std::string item_name = item_json["Name"].template get<std::string>();
            unsigned int item_value = item_json["Value"].template get<int>();

            EnumItem item;
            item.name = item_name;
            item.enum_name = enum_name;
            item.value = item_value;

            enums.item_map[item_name] = item;
        }

        Enum::enum_map[enum_name] = enums;
    }

    setup_enums(L);

    rbxClass::class_map["Instance"]->methods.at("ClearAllChildren").func = rbxInstance_methods::clearAllChildren;
    rbxClass::class_map["Instance"]->methods.at("Clone").func = rbxInstance_methods::clone;
    rbxClass::class_map["Instance"]->methods.at("Destroy").func = rbxInstance_methods::destroy;
    rbxClass::class_map["Instance"]->methods.at("FindFirstChild").func = rbxInstance_methods::findFirstChild;
    rbxClass::class_map["Instance"]->methods.at("GetChildren").func = rbxInstance_methods::getChildren;
    rbxClass::class_map["Instance"]->methods.at("GetDescendants").func = rbxInstance_methods::getDescendants;
    rbxClass::class_map["Instance"]->methods.at("GetFullName").func = rbxInstance_methods::getFullName;
    rbxClass::class_map["Object"]->methods.at("GetPropertyChangedSignal").func = rbxInstance_methods::getPropertyChangedSignal;
    rbxClass::class_map["Object"]->methods.at("IsA").func = rbxInstance_methods::isA;
    rbxClass::class_map["Instance"]->methods.at("IsDescendantOf").func = rbxInstance_methods::isDescendantOf;
    rbxClass::class_map["Instance"]->methods.at("WaitForChild").func = rbxInstance_methods::waitForChild;

    // metatable
    luaL_newmetatable(L, "Instance");

    settypemetafield(L, "Instance");
    setfunctionfield(L, rbxInstance__tostring, "__tostring", nullptr);
    setfunctionfield(L, rbxInstance__index, "__index", nullptr);
    setfunctionfield(L, rbxInstance__newindex, "__newindex", nullptr);
    setfunctionfield(L, rbxInstance__namecall, "__namecall", nullptr);

    lua_pop(L, 1);

    // datatype
    lua_newtable(L);

    setfunctionfield(L, rbxInstance_datatype::_new, "new", "new");
    setfunctionfield(L, rbxInstance_datatype::fromExisting, "fromExisting", "fromExisting");

    lua_setglobal(L, "Instance");

    rbxInstance_BindableEvent_init();

    rbxInstance_ServiceProvider_init(L);

    rbxInstance_DataModel_init(L);
    rbxInstance_HttpService_init();

    auto datamodel = newInstance(L, "DataModel");
    setInstanceValue<std::string>(datamodel, L, PROP_INSTANCE_NAME, "FakeRoblox");

    DataModel::instance = datamodel;

    lua_pushinstance(L, datamodel);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "game");
    lua_setglobal(L, "Game");

    auto workspace = ServiceProvider::getService(L, datamodel, "Workspace");

    Workspace::instance = workspace;

    lua_pushinstance(L, workspace);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "workspace");
    lua_setglobal(L, "Workspace");

    datamodel->values["Workspace"].value = workspace;

    rbxInstance_Players_init(L, datamodel);

    rbxInstance_LayerCollector_init();
    rbxInstance_GuiObject_init();

    auto coregui = ServiceProvider::getService(L, datamodel, "CoreGui");
    hiddenui = cloneInstance(L, coregui);
    // TODO: we could easily just create a new class called HiddenUi
    hiddenui->values[PROP_INSTANCE_NAME].value = "HiddenUi";

    rbxInstance_CoreGui_setup(L, coregui);

    rbxInstance_BasePlayerGui_init(L, { coregui, hiddenui });
    rbxInstance_StarterGui_init(L);

    rbxInstance_UserInputService_init();
    rbxInstance_RunService_init(L);
    RunService::instance = ServiceProvider::getService(L, datamodel, "RunService");

    rbxInstance_Camera_init(L, workspace);
    rbxInstance_TweenService_init();
    rbxInstance_TweenBase_init();

    ServiceProvider::createService(L, datamodel, "UserInputService");

    UI_InstanceExplorer_init(datamodel);

    pushFunctionFromLookup(L, fr_getinstances, "getinstances");
    lua_setglobal(L, "getinstances");
    pushFunctionFromLookup(L, fr_getnilinstances, "getnilinstances");
    lua_setglobal(L, "getnilinstances");

    pushFunctionFromLookup(L, fr_getcallbackvalue, "getcallbackvalue");
    lua_setglobal(L, "getcallbackvalue");

    pushFunctionFromLookup(L, fr_isscriptable, "isscriptable");
    lua_setglobal(L, "isscriptable");
    pushFunctionFromLookup(L, fr_setscriptable, "setscriptable");
    lua_setglobal(L, "setscriptable");

    pushFunctionFromLookup(L, fr_gethui, "gethui");
    lua_setglobal(L, "gethui");
}
void rbxInstanceCleanup(lua_State* L) {
}

std::shared_ptr<rbxInstance> hiddenui = nullptr;

}; // namespace fakeroblox
