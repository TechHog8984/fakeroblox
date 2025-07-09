#include "classes/roblox/instance.hpp"
#include "common.hpp"
#include "lua.h"
#include "lualib.h"
#include <map>

namespace fakeroblox {

#define LUA_TAG_RBXINSTANCE 1

static std::vector<std::string> valid_class_names;
static std::map<std::string, std::shared_ptr<rbxClass>> class_map;

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

void rbxInstance::destroy(lua_State* L) {
    if (destroyed)
        return;

    // FIXME: full destroy behavior (connections disconnected, etc)
    for (auto& child : children)
        child->destroy(L);

    lua_pushnil(L);
    lua_setfield(L, 1, PROP_INSTANCE_PARENT);

    destroyed = true;

    lua_getfield(L, LUA_REGISTRYINDEX, "instancelookup");
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

std::shared_ptr<rbxInstance>& lua_checkinstance(lua_State* L, int arg) {
    luaL_checkany(L, arg);
    void* ud = luaL_checkudata(L, arg, "Instance");

    return *static_cast<std::shared_ptr<rbxInstance>*>(ud);
}
std::shared_ptr<rbxInstance> lua_optinstance(lua_State* L, int arg) {
    if (lua_gettop(L) < arg)
        return nullptr;

    if (lua_type(L, arg) == LUA_TNIL)
        return nullptr;

    return lua_checkinstance(L, arg);
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
}; // namespace rbxInstance_methods

int rbxInstance__tostring(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    auto name = instance->getValue<std::string>(PROP_INSTANCE_NAME);
    lua_pushlstring(L, name.c_str(), name.size());
    return 1;
}

int pushMethod(lua_State* L, std::shared_ptr<rbxInstance>& instance, std::string method_name) {
    std::string original = method_name;
    rbxMethod& method = instance->methods[method_name];
    if (method.route)
        method_name = *method.route;

    if (method.func)
        lua_pushcfunction(L, method.func, original.c_str());
    else
        luaL_error(L, "INTERNAL ERROR: TODO implement '%s'", original.c_str());

    return 1;
}

int rbxInstance__index(lua_State* L) {
    std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1);
    const char* key = luaL_checkstring(L, 2);

    auto class_name = instance->getValue<std::string>(PROP_INSTANCE_CLASS_NAME);
    if (instance->values.find(key) == instance->values.end()) {
        if (instance->methods.find(key) != instance->methods.end())
            return pushMethod(L, instance, key);

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

    int top = lua_gettop(L);

    pushMethod(L, instance, method_name);
    for (int i = 0; i < top; i++) {
        lua_pushvalue(L, 1);
        lua_remove(L, 1);
    }
    lua_call(L, top, LUA_MULTRET);

    return lua_gettop(L);
}

void rbxInstance__dtor(lua_State* L, std::shared_ptr<rbxInstance> ptr) {
    ptr.reset();
}

std::shared_ptr<rbxInstance> newInstance(const char* class_name) {
    std::shared_ptr<rbxClass> _class = class_map[class_name];
    std::shared_ptr<rbxInstance> instance = std::make_shared<rbxInstance>(_class);

    rbxClass* c = _class.get();
    while (c) {
        for (auto& property : c->properties)
            instance->values[property.first] = property.second->default_value;
        instance->methods.insert(c->methods.begin(), c->methods.end());
        c = c->superclass.get();
    }

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
        std::shared_ptr<rbxInstance> parent = lua_optinstance(L, 2);

        if (class_map.find(class_name) == class_map.end()) {
            // std::string msg = "'";
            // msg.append(class_name) += '\'';
            // msg.append(" is not a valid class name");
            // luaL_error(L, "invalid arg #1 to new: '%.*s' is not a valid classname", static_cast<int>(msg.size()), msg.c_str());
            luaL_error(L, "Unable to create an Instance of type \"%s\"", class_name);
        }

        std::shared_ptr<rbxClass> _class = class_map[class_name];
        if (_class->tags & rbxClass::NotCreatable)
            luaL_error(L, "Unable to create an Instance of type \"%s\"", class_name);

        std::shared_ptr<rbxInstance> instance = newInstance(class_name);
        lua_pushinstance(L, instance);

        if (parent) {
            lua_pushvalue(L, 2);
            lua_setfield(L, -2, PROP_INSTANCE_PARENT);
        }

        return 1;
    }
}; // namespace rbxInstance_datatype

void rbxInstanceSetup(lua_State* L, std::string api_dump) {
    json api_json = json::parse(api_dump);
    valid_class_names.reserve(api_json["Classes"].size());

    std::map<std::string, std::string> superclass_map;

    for (auto& class_json : api_json["Classes"]) {
        std::string class_name = class_json["Name"].template get<std::string>();
        valid_class_names.push_back(class_name);

        std::shared_ptr<rbxClass> _class = std::make_shared<rbxClass>();
        auto& superclass_json = class_json["Superclass"];
        if (superclass_json.type() == json::value_t::string)
            superclass_map.try_emplace(class_name, superclass_json.template get<std::string>());

        for (auto& tag_json : class_json["Tags"]) {
            if (tag_json.type() == json::value_t::string) { // TODO: investigate when this isn't string (maybe just when Tags isn't not present?)
                std::string tag = tag_json.template get<std::string>();
                if (tag == "NotCreatable")
                    _class->tags |= rbxClass::NotCreatable;
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
            }
        }

        class_map[class_name] = _class;
    }

    for (auto& pair : superclass_map)
        class_map[pair.first]->superclass = class_map[pair.second];

    superclass_map.clear();

    class_map["Instance"]->methods["Destroy"].func = rbxInstance_methods::destroy;
    class_map["Instance"]->methods["FindFirstChild"].func = rbxInstance_methods::findFirstChild;

    // metatable
    luaL_newmetatable(L, "Instance");

    lua_pushcfunction(L, rbxInstance__tostring, "__tostring");
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, rbxInstance__index, "__index");
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, rbxInstance__newindex, "__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, rbxInstance__namecall, "__namecall");
    lua_setfield(L, -2, "__namecall");

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
    lua_pushstring(L, "kvs");
    lua_setfield(L, -2, "__mode");

    lua_setfield(L, LUA_REGISTRYINDEX, "instancelookup");

    lua_setuserdatadtor(L, LUA_TAG_RBXINSTANCE, (lua_Destructor) rbxInstance__dtor);

    auto datamodel = newInstance("DataModel");
    datamodel->setValue<std::string>(PROP_INSTANCE_NAME, "FakeRoblox");

    lua_pushinstance(L, datamodel);
    lua_setglobal(L, "game");

    auto workspace = newInstance("Workspace");
    lua_pushinstance(L, workspace);
    lua_pushinstance(L, datamodel);
    lua_setfield(L, -2, PROP_INSTANCE_PARENT);

    datamodel->setValue<std::shared_ptr<rbxInstance>>("Workspace", workspace);

    lua_setglobal(L, "workspace");
}
void rbxInstanceCleanup(lua_State* L) {
}

}; // namespace fakeroblox
