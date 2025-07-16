#pragma once

#include <map>
#include <string>

#include "lua.h"

namespace fakeroblox {

class EnumItem {
public:
    std::string name;
    std::string enum_name;
    unsigned int value = 0;
};

class Enum {
public:
    static std::map<std::string, Enum> enum_map;

    std::string name;
    std::map<std::string, EnumItem> item_map;
};

void setup_enums(lua_State* L);

}; // namespace fakeroblox
