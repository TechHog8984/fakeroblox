#pragma once

#include "lua.h"
#include <string>

namespace fakeroblox {

class FileSystem {
public:
    static std::string home_path;
    static std::string workspace_path;
};

void open_filesystemlib(lua_State* L);

}; // namespace fakeroblox
