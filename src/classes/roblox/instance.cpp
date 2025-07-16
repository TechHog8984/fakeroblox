#include "classes/roblox/instance.hpp"
#include "classes/enum.hpp"
#include "classes/roblox/bindableevent.hpp"
#include "classes/roblox/datamodel.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"

#include "classes/roblox/serviceprovider.hpp"
#include "common.hpp"

#include "lobject.h"
#include "lua.h"
#include "lualib.h"
#include "lstate.h"

#include <cstdio>
#include <cstdlib>
#include <map>

namespace fakeroblox {

std::map<std::string, std::shared_ptr<rbxClass>> rbxClass::class_map;
std::vector<std::string> rbxClass::valid_class_names;
std::vector<std::string> rbxClass::valid_services;

rbxInstance::rbxInstance(std::shared_ptr<rbxClass> _class) : _class(_class) {}

rbxInstance::~rbxInstance() {
    // ScriptConsole::debugf("destroying instance... %p, %s\n", this, getValue<std::string>(PROP_INSTANCE_NAME).c_str());
}

template<class T>
T& rbxInstance::getValue(std::string name) {
    return std::get<T>(values.at(name).value);
}

template<class T>
void rbxInstance::setValue(std::string name, T value) {
    std::get<T>(values.at(name).value) = value;
}

int rbxInstance::pushEvent(lua_State* L, const char* name) {
    lua_getfield(L, LUA_REGISTRYINDEX, SIGNALLOOKUP);
    lua_pushlightuserdata(L, this);
    lua_rawget(L, -2);
    lua_pushstring(L, name);
    lua_rawget(L, -2);

    lua_remove(L, -2); // remove signallookup
    lua_remove(L, -2); // remove signallookup table
    return 1;
}

void rbxInstance::destroy(lua_State* L) {
    if (destroyed)
        return;

    // FIXME: full destroy behavior (consult docs)
    for (auto& child : children)
        child->destroy(L);
    for (auto& event : events) {
        pushEvent(L, event.c_str());
        rbxScriptSignal* signal = lua_checkrbxscriptsignal(L, -1);
        lua_pop(L, 1);
        signal->~rbxScriptSignal();
    }

    lua_pushnil(L);
    lua_setfield(L, 1, PROP_INSTANCE_PARENT);

    destroyed = true;

    lua_getfield(L, LUA_REGISTRYINDEX, INSTANCELOOKUP);
    lua_pushlightuserdata(L, this);
    lua_pushnil(L);
    lua_rawset(L, -3);
}
std::shared_ptr<rbxInstance> rbxInstance::findFirstChild(std::string name) {
    for (auto& child : children)
        if (child->getValue<std::string>(PROP_INSTANCE_NAME) == name)
            return child;
    return nullptr;
}

std::shared_ptr<rbxInstance>& lua_checkinstance(lua_State* L, int narg, const char* class_name) {
    void* ud = luaL_checkudata(L, narg, "Instance");
    auto instance = static_cast<std::shared_ptr<rbxInstance>*>(ud);

    if (class_name) {
        bool valid_class = false;
        rbxClass* c = (*instance)->_class.get();
        while (c) {
            if (strequal(c->name.c_str(), class_name)) {
                valid_class = true;
                break;
            }
            c = c->superclass.get();
        }

        Closure* cl = L->ci > L->base_ci ? curr_func(L) : NULL;
        assert(cl);
        assert(cl->isC);
        const char* debugname = cl->c.debugname + 0;

        if (!valid_class) {
            const char* fmt = "Expected ':' not '.' calling member function %s";
            int size = snprintf(NULL, 0, fmt, debugname);
            char* msg = static_cast<char*>(malloc(size));
            snprintf(msg, size + 1, fmt, debugname);

            lua_pushlstring(L, msg, size);
            free(msg);
            lua_error(L);
        }
    }

    // FIXME: this is ugly (duplicate code); I think the solution is moving to weak_ptr instead of shared_ptr reference
    return *static_cast<std::shared_ptr<rbxInstance>*>(ud);
}
std::shared_ptr<rbxInstance> lua_optinstance(lua_State* L, int narg, const char* class_name) {
    if (lua_gettop(L) < narg)
        return nullptr;

    if (lua_type(L, narg) == LUA_TNIL)
        return nullptr;

    return lua_checkinstance(L, narg, class_name);
}

namespace rbxInstance_methods {
    int destroy(lua_State *L) {
        // FIXME: synchronize
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
        instance->destroy(L);
        return 0;
    }
    int findFirstChild(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
        const char* name = luaL_checkstring(L, 2);

        lua_pushinstance(L, instance->findFirstChild(name));
        return 1;
    }
    int getChildren(lua_State* L) {
        // FIXME: synchronize
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
        auto& children = instance->children;

        lua_createtable(L, children.size(), 0);

        for (size_t i = 0; i < children.size(); i++) {
            auto& child = children[i];
            lua_pushnumber(L, i + 1);
            lua_pushinstance(L, child);
            lua_settable(L, -3);
        }

        return 1;
    }
    int getFullName(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);

        std::string result;
        rbxInstance* inst = instance.get();
        do {
            result.insert(result.begin(), '.');
            result.insert(0, inst->getValue<std::string>(PROP_INSTANCE_NAME));
            inst = inst->getValue<std::shared_ptr<rbxInstance>>(PROP_INSTANCE_PARENT).get();
        } while (inst);

        size_t last = result.size() - 1;
        if (!result.empty() && result.at(last) == '.')
            result.erase(last);

        lua_pushlstring(L, result.c_str(), result.size());
        return 1;
    }
}; // namespace rbxInstance_methods

int rbxInstance__tostring(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    auto name = instance->getValue<std::string>(PROP_INSTANCE_NAME);
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

    auto class_name = instance->getValue<std::string>(PROP_INSTANCE_CLASS_NAME);
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
        auto name = instance->getValue<std::string>(PROP_INSTANCE_NAME);
        luaL_error(L, "%s is not a valid member of %s \"%s\"", key, class_name.c_str(), name.c_str());
    }

    auto value = &instance->values[key];
    auto property = value->property;
    if (property->route)
        value = &instance->values[*property->route];

    if (value->property->tags & rbxProperty::WriteOnly)
        luaL_error(L, "'%s' is a write-only member of %s", key, class_name.c_str());

    if (value->is_nil)
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
                } else
                    assert(!"UNHANDLED ALTERNATIVE FOR PROPERTY VALUE");
                break;
            case DataType:
                luaL_error(L, "TODO: datatypes");
                break;
            case Instance:
                lua_pushinstance(L, std::get<std::shared_ptr<rbxInstance>>(value->value));
                break;
        }
    }

    return 1;
};

const char* getOptionalInstanceName(std::shared_ptr<rbxInstance> instance) {
    if (!instance || !instance.get())
        return "NULL";
    return instance->getValue<std::string>(PROP_INSTANCE_NAME).c_str();
}
int rbxInstance__newindex(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);
    luaL_checkany(L, 3);

    // TODO: methods
    auto class_name = instance->getValue<std::string>(PROP_INSTANCE_CLASS_NAME);
    if (instance->values.find(key) == instance->values.end()) {
        auto name = instance->getValue<std::string>(PROP_INSTANCE_NAME);
        luaL_error(L, "%s is not a valid member of %s \"%s\"", key, class_name.c_str(), name.c_str());
    }

    auto value = &instance->values[key];
    auto property = value->property;
    if (property->route)
        value = &instance->values[*property->route];

    if (value->property->tags & rbxProperty::ReadOnly)
        // luaL_error(L, "'%s' is a read-only member of %s", key, class_name.c_str());
        luaL_error(L, "Unable to assign property %s. Property is read only", key);

    switch (property->type_category) {
        case Primitive:
            if (lua_isnil(L, 3)) {
                value->is_nil = true;
                break;
            }

            if (std::holds_alternative<bool>(value->value)) {
                value->value = luaL_checkboolean(L, 3);
            } else if (std::holds_alternative<int32_t>(value->value))
                value->value = int32_t(luaL_checkinteger(L, 3));
            else if (std::holds_alternative<int64_t>(value->value))
                value->value = int64_t(luaL_checkinteger(L, 3));
            else if (std::holds_alternative<float>(value->value))
                value->value = float(luaL_checknumber(L, 3));
            else if (std::holds_alternative<double>(value->value))
                value->value = double(luaL_checknumber(L, 3));
            else if (std::holds_alternative<std::string>(value->value)) {
                size_t l;
                const char* str = luaL_checklstring(L, 3, &l);
                value->value = std::string(str, l);
            } else
                assert(!"UNHANDLED ALTERNATIVE FOR PROPERTY VALUE");
            break;
        case DataType:
            luaL_error(L, "TODO: datatypes");
            break;
        case Instance: {
            // TODO: synchronize
            std::shared_ptr<rbxInstance> new_value = lua_optinstance(L, 3);
            if (strequal(key, PROP_INSTANCE_PARENT)) {
                std::shared_ptr<rbxInstance> old_parent = instance->getValue<std::shared_ptr<rbxInstance>>(PROP_INSTANCE_PARENT);
                if ((!old_parent && !new_value) || (!old_parent.get() && !new_value.get()) || (old_parent && new_value && old_parent == new_value))
                    return 0;

                if (instance->destroyed)
                    luaL_error(L, "The Parent property of %s is locked, current parent: %s, new parent %s", getOptionalInstanceName(instance), getOptionalInstanceName(old_parent), getOptionalInstanceName(new_value));

                if (new_value == instance)
                    luaL_error(L, "Attempt to set %s as its own parent", getOptionalInstanceName(instance));

                if (old_parent)
                    old_parent->children.erase(std::find(old_parent->children.begin(), old_parent->children.end(), instance));

                if (new_value)
                    new_value->children.push_back(instance);
            }

            auto& ptr = std::get<std::shared_ptr<rbxInstance>>(value->value);
            if (!new_value)
                ptr.reset();
            else
                ptr = new_value;

            break;
        }
    }

    return 0;
};
int rbxInstance__namecall(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");
    std::string method_name = namecall;

    if (instance->methods.find(method_name) == instance->methods.end()) {
        auto name = instance->getValue<std::string>(PROP_INSTANCE_NAME);
        auto class_name = instance->getValue<std::string>(PROP_INSTANCE_CLASS_NAME);
        luaL_error(L, "%s is not a valid member of %s \"%s\"", namecall, class_name.c_str(), name.c_str());
    }

    lua_CFunction func = instance->methods[method_name].func;
    if (func)
        return instance->methods[method_name].func(L);
    else
        luaL_error(L, "INTERNAL ERROR: TODO implement '%s'", method_name.c_str());
}

void rbxInstance__dtor(lua_State* L, void* ud) {
    std::shared_ptr<rbxInstance>* instance_ptr = static_cast<std::shared_ptr<rbxInstance>*>(ud);

    rbxClass* c = (*instance_ptr)->_class.get();
    while (c) {
        if (c->destructor)
            c->destructor(*instance_ptr);
        c = c->superclass.get();
    }

    instance_ptr->reset();
}

std::shared_ptr<rbxInstance> newInstance(lua_State* L, const char* class_name) {
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
        for (auto& property : c->properties)
            instance->values[property.first] = property.second->default_value;
        instance->methods.insert(c->methods.begin(), c->methods.end());
        instance->events.insert(instance->events.end(), c->events.begin(), c->events.end());
        for (auto& event : c->events) {
            lua_pushstring(L, event.c_str());
            pushNewRbxScriptSignal(L, event);
            lua_rawset(L, -3);
        }
        if (c->constructor)
            c->constructor(instance);
        c = c->superclass.get();
    }
    lua_pop(L, 1); // signallookup table

    // FIXME: default values (things like archivable)
    // FIXME: unique_id
    instance->setValue<std::string>(PROP_INSTANCE_NAME, class_name);
    instance->setValue<std::string>(PROP_INSTANCE_CLASS_NAME, class_name);

    return instance;
}

int lua_pushinstance(lua_State* L, std::shared_ptr<rbxInstance> instance) {
    if (!instance || !instance.get()) {
        lua_pushnil(L);
        return 1;
    }

    return pushFromLookup(L, INSTANCELOOKUP, instance.get(), [&L, &instance](){
        void* ud = lua_newuserdatatagged(L, sizeof(std::shared_ptr<rbxInstance>), LUA_TAG_RBXINSTANCE);
        new(ud) std::shared_ptr<rbxInstance>(instance);

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

        std::shared_ptr<rbxInstance> instance = newInstance(L, class_name);
        lua_pushinstance(L, instance);

        if (parent) {
            lua_pushvalue(L, 2);
            lua_setfield(L, -2, PROP_INSTANCE_PARENT);
        }

        return 1;
    }
}; // namespace rbxInstance_datatype

void rbxInstanceSetup(lua_State* L, std::string api_dump) {
    // instancelookup
    newweaktable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, INSTANCELOOKUP);

    setup_rbxscriptsignal(L);

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
                    rbxClass::valid_services.push_back(class_name);
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
                            if (tag == "ReadOnly")
                                property->tags |= rbxProperty::ReadOnly;
                            else if (tag == "WriteOnly")
                                property->tags |= rbxProperty::WriteOnly;
                        } else
                            property->route = tag_json["PreferredDescriptorName"].template get<std::string>();
                    }
                }

                std::string category = member_json["ValueType"]["Category"].template get<std::string>();
                if (category == "Primitive") {
                    property->type_category = Primitive;
                    property->default_value = rbxValue();

                    std::string type = member_json["ValueType"]["Name"].template get<std::string>();
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
                } else if (category == "DataType") {
                    // FIXME: datatypes
                } else if (category == "Class") {
                    property->type_category = Instance;
                    property->default_value = rbxValue();
                    property->default_value.value = std::shared_ptr<rbxInstance>(nullptr);
                }

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

    rbxClass::class_map["Instance"]->methods.at("Destroy").func = rbxInstance_methods::destroy;
    rbxClass::class_map["Instance"]->methods.at("FindFirstChild").func = rbxInstance_methods::findFirstChild;
    rbxClass::class_map["Instance"]->methods.at("GetChildren").func = rbxInstance_methods::getChildren;
    rbxClass::class_map["Instance"]->methods.at("GetFullName").func = rbxInstance_methods::getFullName;

    rbxInstance_ServiceProvider_init(L);
    rbxInstance_DataModel_init(L);
    rbxInstance_BindableEvent_init();

    // metatable
    luaL_newmetatable(L, "Instance");

    setfunctionfield(L, rbxInstance__tostring, "__tostring", nullptr);
    setfunctionfield(L, rbxInstance__index, "__index", nullptr);
    setfunctionfield(L, rbxInstance__newindex, "__newindex", nullptr);
    setfunctionfield(L, rbxInstance__namecall, "__namecall", nullptr);

    lua_pop(L, 1);

    // datatype
    lua_newtable(L);

    lua_pushcfunction(L, rbxInstance_datatype::_new, "new");
    lua_setfield(L, -2, "new");

    lua_setglobal(L, "Instance");

    lua_setuserdatadtor(L, LUA_TAG_RBXINSTANCE, rbxInstance__dtor);

    auto datamodel = newInstance(L, "DataModel");
    datamodel->setValue<std::string>(PROP_INSTANCE_NAME, "FakeRoblox");

    lua_pushinstance(L, datamodel);
    lua_setglobal(L, "game");

    auto workspace = newInstance(L, "Workspace");
    lua_pushinstance(L, workspace);
    lua_pushinstance(L, datamodel);
    lua_setfield(L, -2, PROP_INSTANCE_PARENT);

    datamodel->setValue<std::shared_ptr<rbxInstance>>("Workspace", workspace);

    createService(L, datamodel, "UserInputService");

    lua_setglobal(L, "workspace");
}
void rbxInstanceCleanup(lua_State* L) {
}

}; // namespace fakeroblox
