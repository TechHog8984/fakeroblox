#include <cstdio>

#include "cli.hpp"

namespace fakeroblox {

void displayHelp(const char* filename) {
    printf("fakeroblox by techhog\n"
        "usage: %s [options]\n\n"
        "options:\n"
        "  -h           -  displays this page\n"
        "  --nosandbox  -  disables sandboxing (Luau will perform less optimizations, but functions like getgenv need this flag to work)\n"
    , filename);
}

}; // namespace fakeroblox
