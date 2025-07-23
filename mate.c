#define MATE_IMPLEMENTATION
#include "mate.h"

int main() {
    StartBuild();

    ExecutableOptions executable_options = {
        .output = "fakeroblox",
        .flags = "-std=c++17 -Wall -Werror -g"
    };
    Executable executable = CreateExecutable(executable_options);

    AddIncludePaths(executable, "./include");

    AddFile(executable, "./src/*.cpp");
    AddFile(executable, "./src/classes/*.cpp");
    AddFile(executable, "./src/classes/roblox/*.cpp");
    AddFile(executable, "./src/classes/roblox/datatypes/*.cpp");
    AddFile(executable, "./src/libraries/*.cpp");
    AddFile(executable, "./src/ui/*.cpp");

    AddIncludePaths(executable, "./dependencies/Luau/Analysis/include");
    AddIncludePaths(executable, "./dependencies/Luau/Ast/include");
    AddIncludePaths(executable, "./dependencies/Luau/Common/include");
    AddIncludePaths(executable, "./dependencies/Luau/Compiler/include");
    AddIncludePaths(executable, "./dependencies/Luau/Config/include");
    AddIncludePaths(executable, "./dependencies/Luau/VM/include");
    AddIncludePaths(executable, "./dependencies/Luau/VM/src");

    AddIncludePaths(executable, "./dependencies/rlImGui");
    AddIncludePaths(executable, "./dependencies/rlImGui/imgui-master");

    LinkSystemLibraries(executable, "m", "stdc++", "curl", "raylib");

    AddLibraryPaths(executable, "./dependencies/Luau/cmake");
    LinkSystemLibraries(executable, "Luau");

    AddLibraryPaths(executable, "./dependencies/rlImGui/bin/Release");
    LinkSystemLibraries(executable, "rlImGui");

    InstallExecutable(executable);

    EndBuild();
    return 0;
}
