#pragma once

#include <functional>
#include <shared_mutex>
#include <string>
#include <cstring> // for strcmp for strequal

#include "console.hpp"
#include "nlohmann/json.hpp"

#include "lobject.h"
#include "lua.h"
#include "type_registry.hpp"

using json = nlohmann::json;

namespace fakeroblox {

// NOTE: this is hard-coded sorry
#define MAX_KEYBOARD_KEYS 512

#define strequal(str1, str2) (strcmp(str1, str2) == 0)

#define luaL_optnumberloose(L, narg, def) (luaL_opt(L, lua_tonumber, narg, def))

enum LuaTag {
    LUA_TAG_SHAREDPTR_OBJECT = 1,
    LUA_TAG_ENUM,
    LUA_TAG_ENUMITEM,
    LUA_TAG_TWEENINFO,
    LUA_TAG_NUMBER_SEQUENCE,
    LUA_TAG_COLOR_SEQUENCE,
};

extern bool print_stdout;

int countDecimals(double value);
#define decimalFmt(value) countDecimals(value), value

double getSeconds(lua_State* L, int arg = 1);

#define SHAREDPTR_TAG_COUNT 2
using Destructor = std::function<void(void*)>;

extern std::map<size_t, Destructor> sharedptr_destructor_list;

struct SharedPtrObject {
    size_t class_index;
    void* object;
};
void initializeSharedPtrDestructorList();

using Feedback = std::function<void(std::string)>;
using OnKill = std::function<void()>;

std::string fixString(std::string_view original);

std::string rawtostringobj(lua_State* L, const TValue* obj, bool use_fixstring = false);
std::string rawtostring(lua_State* L, int index);

double luaL_checknumberrange(lua_State* L, int narg, double min, double max, const char* context);
double luaL_optnumberrange(lua_State* L, int narg, double min, double max, const char* context, double def = 0);

bool luaL_isudatareal(lua_State* L, int ud, const char* tname);
void* luaL_checkudatareal(lua_State* L, int ud, const char* tname);

int createweaktable(lua_State* L, int narr, int nrec);
int newweaktable(lua_State* L);

int pushFromLookup(lua_State* L, const char* lookup, std::function<void()> pushKey, std::function<void()> pushValue);
int pushFromLookup(lua_State* L, const char* lookup, void* ptr, std::function<void()> pushValue);

int pushFunctionFromLookup(lua_State* L, lua_CFunction func, const char* name = nullptr, lua_Continuation cont = nullptr);
// push lookup, call addToLookup, lookup is popped by addToLookup
int addToLookup(lua_State *L, std::function<void()> pushValue, bool keep_value = false);

template<class T>
void pushNewSharedPtrObject(lua_State* L, std::shared_ptr<T>& ptr) {
    SharedPtrObject* object = static_cast<SharedPtrObject*>(lua_newuserdatatagged(L, sizeof(SharedPtrObject), LUA_TAG_SHAREDPTR_OBJECT));
    object->class_index = T::class_index();
    object->object = malloc(sizeof(std::shared_ptr<T>));
    new(object->object) std::shared_ptr<T>(ptr);
}

template<class T>
int pushFromSharedPtrLookup(lua_State* L, const char* lookup, std::shared_ptr<T>& ptr, std::function<void(void)> initialize) {
    return pushFromLookup(L, lookup, ptr.get(), [&L, &ptr, &initialize](){
        pushNewSharedPtrObject(L, ptr);

        initialize();
    });
}

#define INSTANCELOOKUP "instancelookup"
#define METHODLOOKUP "methodlookup"
#define RBXSCRIPTCONNECTION_METHODLOOKUP "rbxscriptconnectionmethodlookup"
// TODO: I don't think we use the string lookup for any non-static strings, so it should be removed which will become easily when we get rid of redundant std::string uses
#define STRINGLOOKUP "stringlookup"
#define SIGNALLOOKUP "signallookup"
#define SIGNALCONNECTIONLISTLOOKUP "signalconnectionlistlookup"
#define ENUMLOOKUP "enumlookup"

// TODO: replace as many std::string functon parameters as are feasible
int addToStringLookup(lua_State* L, std::string string);
const char* getFromStringLookup(lua_State* L, int index);

std::string getStackMessage(lua_State* L);

int fr_print(lua_State* L);
int fr_warn(lua_State* L);

// if you keep lookup as false, NOTE that FunctionExplorer will ensure functions are in the lookup when explored
void setfunctionfield(lua_State* L, lua_CFunction func, const char* method, const char* debugname, bool lookup = false);
// if you keep lookup as false, NOTE that FunctionExplorer will ensure functions are in the lookup when explored
void setfunctionfield(lua_State* L, lua_CFunction func, const char* method, bool lookup = false);

void settypemetafield(lua_State* L, const char* type);

std::string sha1ToString(unsigned int* hashed);

}; // namespace fakeroblox
