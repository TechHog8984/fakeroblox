#pragma once

#include <cstddef>

namespace fakeroblox {

struct MemoryStruct {
    char *memory;
    size_t size;
};
int newGetRequest(const char* url, struct MemoryStruct* chunk);

};
