#include "common.hpp"
#include "console.hpp"
#include "taskscheduler.hpp"

#include "classes/roblox/instance.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

#include "lua.h"
#include "lualib.h"
#include "lapi.h"
#include "lnumutils.h"
#include "lstate.h"

namespace fakeroblox {

bool print_stdout = false;

std::map<size_t, Destructor> sharedptr_destructor_list;
std::map<void*, size_t> object_destructor_map;

void initializeSharedPtrDestructorList() {
    #define addConstructor(Class) {                                                  \
        sharedptr_destructor_list[Class::class_index()] = [] (void* ud) {            \
            std::shared_ptr<Class>* ptr = static_cast<std::shared_ptr<Class>*>(ud);  \
            ptr->reset();                                                            \
        };                                                                           \
    }

    // i created all this because I thought Tasks would do the same thing, but then I realized I could just use states and the userthread callback, so now it's just rbxInstance
    addConstructor(rbxInstance)

    #undef addConstructor
}

std::string fixString(std::string_view original) {
    std::string result = "\"";

    for (auto& ch : original) {
        if (ch > 31 && ch < 127 && ch != '"' && ch != '\\')
            result.append(std::string{ch});
        else {
            switch (ch) {
                case 7:
                    result.append("\\a");
                    break;
                case 8:
                    result.append("\\b");
                    break;
                case 9:
                    result.append("\\t");
                    break;
                case 10:
                    result.append("\\n");
                    break;
                case 11:
                    result.append("\\v");
                    break;
                case 12:
                    result.append("\\f");
                    break;
                case 13:
                    result.append("\\r");
                    break;

                case '"':
                    result.append("\\\"");
                    break;
                case '\\':
                    result.append("\\\\");
                    break;

                default:
                    result.append("\\");

                    unsigned char n = (unsigned char) ch;
                    if (n < 10)
                        result.append("00");
                    else if (n < 100)
                        result.append("0");

                    result.append(std::to_string((unsigned char)ch));
            };
        };
    };

    result.append("\"");

    return result;
};

// from Luau/VM/src/laux.cpp
std::string safetostringobj(lua_State* L, const TValue* obj, bool use_fixstring) {
    std::string str;
    switch (obj->tt) {
        case LUA_TNIL:
            str.assign("nil");
            break;
        case LUA_TBOOLEAN:
            str.assign((bvalue(obj) ? "true" : "false"));
            break;
        case LUA_TNUMBER: {
            double n = nvalue(obj);
            char s[LUAI_MAXNUM2STR];
            char* e = luai_num2str(s, n);
            str = std::string(s, e - s);
            break;
        }
        case LUA_TVECTOR: {
            const float* v = vvalue(obj);

            char s[LUAI_MAXNUM2STR * LUA_VECTOR_SIZE];
            char* e = s;
            for (int i = 0; i < LUA_VECTOR_SIZE; ++i) {
                if (i != 0) {
                    *e++ = ',';
                    *e++ = ' ';
                }
                e = luai_num2str(e, v[i]);
            }
            str = std::string(s, e - s);
            break;
        }
        case LUA_TSTRING:
            if (use_fixstring)
                str = fixString(std::string_view(tsvalue(obj)->data, tsvalue(obj)->len));
            else
                str = std::string(tsvalue(obj)->data, tsvalue(obj)->len);
            break;
        default: {
            const void* ptr = ttype(obj) == LUA_TUSERDATA ? uvalue(obj)->data : ttype(obj) == LUA_TLIGHTUSERDATA ? pvalue(obj) : iscollectable(obj) ? gcvalue(obj) : nullptr;
            unsigned long long enc = lua_encodepointer(L, uintptr_t(ptr));
            static const char* fmt = "%s: 0x%016llx";
            int size = snprintf(NULL, 0, fmt, lua_typename(L, obj->tt), enc);
            str.resize(size);
            snprintf(str.data(), size + 1, fmt, lua_typename(L, obj->tt), enc);
            break;
        }
    }
    return str;
}
std::string safetostring(lua_State* L, int index) {
    return safetostringobj(L, luaA_toobject(L, index));
}

double luaL_checknumberrange(lua_State* L, int narg, double min, double max) {
    double n = luaL_checknumber(L, narg);
    luaL_argcheck(L, n >= min && n <= max, narg, "invalid range");
    return n;
}
double luaL_optnumberrange(lua_State* L, int narg, double min, double max, double def) {
    double n = def;
    if (lua_isnumber(L, narg)) {
        n = lua_tonumber(L, narg);
        luaL_argcheck(L, n >= min && n <= max, narg, "invalid range");
    }
    return n;
}

int newweaktable(lua_State* L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    lua_pushstring(L, "kvs");
    lua_setfield(L, -2, "__mode");

    return 1;
}
// TODO: only call pushKey once (use pushvalue)
int pushFromLookup(lua_State* L, const char* lookup, std::function<void()> pushKey, std::function<void()> pushValue) {
    lua_getfield(L, LUA_REGISTRYINDEX, lookup);
    pushKey();
    lua_rawget(L, -2);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        pushValue();

        pushKey();
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
    }

    lua_remove(L, -2);

    return 1;
}
int pushFromLookup(lua_State* L, const char* lookup, void* ptr, std::function<void()> pushValue) {
    return pushFromLookup(L, lookup, [&L, &ptr] { lua_pushlightuserdata(L, ptr); }, pushValue);
}

int pushFunctionFromLookup(lua_State* L, lua_CFunction func, const char* name, lua_Continuation cont) {
    return pushFromLookup(L, METHODLOOKUP, reinterpret_cast<void*>(func), [&L, func, name, cont] {
        const char* namecstr = name == nullptr ? nullptr : getFromStringLookup(L, addToStringLookup(L, name));

        if (cont)
            lua_pushcclosurek(L, func, namecstr, 0, cont);
        else
            lua_pushcfunction(L, func, namecstr);
    });
}

int addToLookup(lua_State *L, std::function<void()> pushValue, bool keep_value) {
    pushValue();

    lua_getglobal(L, "table");
    lua_getfield(L, -1, "find");
    lua_remove(L, -2); // table

    lua_pushvalue(L, -3); // lookup
    lua_pushvalue(L, -3); // value

    lua_call(L, 2, 1);

    bool cached = lua_isnumber(L, -1);
    int index = cached ? lua_tonumber(L, -1) : lua_objlen(L, -3) + 1;
    lua_pop(L, 1); // table.find result

    if (cached) {
        if (!keep_value)
            lua_pop(L, 1);
    } else {
        if (keep_value)
            lua_pushvalue(L, -1);
        lua_rawseti(L, -2 - keep_value, index); // lookup[index] = value
    }

    lua_remove(L, -1 - keep_value); // pop lookup

    return index;
}

int addToStringLookup(lua_State *L, std::string string) {
    lua_getfield(L, LUA_REGISTRYINDEX, STRINGLOOKUP);
    return addToLookup(L, [&L, &string]{
        lua_pushlstring(L, string.c_str(), string.size());
    });
}
const char* getFromStringLookup(lua_State *L, int index) {
    lua_getfield(L, LUA_REGISTRYINDEX, STRINGLOOKUP);
    lua_rawgeti(L, -1, index);

    const char* str = lua_tostring(L, -1);

    lua_pop(L, 2);
    return str;
}

std::string getStackMessage(lua_State* L) {
    std::string message;
    int n = lua_gettop(L);
    for (int i = 0; i < n; i++) {
        size_t l;
        const char* s = luaL_tolstring(L, i + 1, &l);
        if (i > 0)
            message += '\t';
        message.append(s, l);
        lua_pop(L, 1);
    }
    return message;
}

int log(lua_State* L, Console::Message::Type type) {
    std::string msg = getStackMessage(L);
    auto& console = getTask(L)->console;
    if (print_stdout && console->id != Tests)
        printf("%s %.*s\n", Console::getMessageTypeString(type), static_cast<int>(msg.size()), msg.c_str());
    else
        console->log(msg, type);
    return 0;
}
// these functions are exposed in environment.cpp
int fr_print(lua_State* L) {
    return log(L, Console::Message::INFO);
}
int fr_warn(lua_State* L) {
    return log(L, Console::Message::WARNING);
}

void setfunctionfield(lua_State *L, lua_CFunction func, const char *method, const char* debugname, bool lookup) {
    assert(lua_istable(L, -1));

    if (lookup)
        pushFunctionFromLookup(L, func, debugname);
    else
        lua_pushcfunction(L, func, debugname);
    lua_setfield(L, -2, method);
}
void setfunctionfield(lua_State *L, lua_CFunction func, const char *method, bool lookup) {
    setfunctionfield(L, func, method, nullptr, lookup);
}

std::string sha1ToString(unsigned int *hashed) {
    char result[51] = "";
    char buf[11];

    for (int i = 0; i < 5; i++) {
        snprintf(buf, sizeof(buf), "%010u", hashed[i]);

        strncat(result, buf, sizeof(result) - strlen(result) - 1);
    }

    return result;
}

}; // namespace fakeroblox
