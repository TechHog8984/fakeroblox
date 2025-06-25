#include <cstdio>

#include "cli.hpp"

namespace fakeroblox {

void displayHelp(const char* filename) {
    printf("fakeroblox by techhog\n"
        "usage: %s [options]\n\n"
        "options:\n"
        "  -h  -  displays this page\n"
    , filename);
}

}; // namespace fakeroblox
