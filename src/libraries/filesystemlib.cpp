#include "libraries/filesystemlib.hpp"
#include "environment.hpp"

#include "lua.h"
#include "lualib.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

namespace fakeroblox {

std::string FileSystem::home_path;
std::string FileSystem::workspace_path;

static void checkPath(lua_State* L, const char* path) {
    if (!*path)
        luaL_error(L, "path cannot be empty");

    if (strstr(path, "/.") != NULL || strstr(path, "\\.") != NULL)
        luaL_error(L, "path contains invalid characters (slash followed by a dot)");
}

static int fr_appendfile(lua_State* L) {
    const char* relative_path = luaL_checkstring(L, 1);
    size_t datal;
    const char* data = luaL_checklstring(L, 2, &datal);

    checkPath(L, relative_path);

    std::string path = FileSystem::workspace_path;
    path.append(relative_path);
    std::ofstream file;

    file.open(path, std::ios::app);

    if (!file.is_open())
        luaL_error(L, "failed to open file '%s'", relative_path);

    file << std::string(data, datal);
    file.close();

    return 0;
}
static int fr_readfile(lua_State* L) {
    const char* relative_path = luaL_checkstring(L, 1);

    checkPath(L, relative_path);

    std::string path = FileSystem::workspace_path;
    path.append(relative_path);
    if (!std::filesystem::exists(path))
        luaL_error(L, "failed to open file '%s'", relative_path);

    std::ifstream file(path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    lua_pushlstring(L, content.c_str(), content.size());
    return 1;
}
static int fr_writefile(lua_State* L) {
    const char* relative_path = luaL_checkstring(L, 1);
    size_t datal;
    const char* data = luaL_checklstring(L, 2, &datal);

    checkPath(L, relative_path);

    std::string path = FileSystem::workspace_path;
    path.append(relative_path);
    std::ofstream file;

    file.open(path, std::ios::out);

    if (!file.is_open())
        luaL_error(L, "failed to open file '%s'", relative_path);

    file << std::string(data, datal);
    file.close();

    return 0;
}

static int fr_makefolder(lua_State* L) {
    const char* relative_path = luaL_checkstring(L, 1);

    checkPath(L, relative_path);

    std::string path = FileSystem::workspace_path;
    path.append(relative_path);

    if (std::filesystem::is_directory(path))
        return 0;

    if (!std::filesystem::create_directory(path))
        luaL_error(L, "failed to create directory '%s'", relative_path);

    return 0;
}

static bool isFile(std::string_view path) {
    // TODO: symlinks too?
    const bool result = std::filesystem::is_regular_file(path);

    return result;
}
static int fr_isfile(lua_State* L) {
    const char* relative_path = luaL_checkstring(L, 1);

    checkPath(L, relative_path);

    std::string path = FileSystem::workspace_path;
    path.append(relative_path);

    lua_pushboolean(L, isFile(path));
    return 1;
}
static int fr_isfolder(lua_State* L) {
    const char* relative_path = luaL_checkstring(L, 1);

    checkPath(L, relative_path);

    std::string path = FileSystem::workspace_path;
    path.append(relative_path);

    lua_pushboolean(L, std::filesystem::is_directory(path));
    return 1;
}


static int fr_delfile(lua_State* L) {
    const char* relative_path = luaL_checkstring(L, 1);

    checkPath(L, relative_path);

    std::string path = FileSystem::workspace_path;
    path.append(relative_path);

    if (!std::filesystem::exists(path))
        luaL_error(L, "no file or directory exists at path '%s'", relative_path);

    if (!isFile(path))
        luaL_error(L, "given path is not a file '%s'", relative_path);

    if (!std::filesystem::remove(path))
        luaL_error(L, "failed to delete file '%s'", relative_path);

    return 0;
}
static int fr_delfolder(lua_State* L) {
    const char* relative_path = luaL_checkstring(L, 1);

    checkPath(L, relative_path);

    std::string path = FileSystem::workspace_path;
    path.append(relative_path);

    if (!std::filesystem::exists(path))
        luaL_error(L, "no file or directory exists at path '%s'", relative_path);

    if (!std::filesystem::is_directory(path))
        luaL_error(L, "given path is not a folder '%s'", relative_path);

    if (!std::filesystem::remove(path))
        luaL_error(L, "failed to delete file '%s'", relative_path);

    return 0;
}

void open_filesystemlib(lua_State* L) {
    env_expose(appendfile)
    env_expose(readfile)
    env_expose(writefile)
    env_expose(makefolder)
    env_expose(isfile)
    env_expose(isfolder)
    env_expose(delfile)
    env_expose(delfolder)
}

}; // namespace fakeroblox
