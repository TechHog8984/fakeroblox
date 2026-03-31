#pragma once

#include <cstddef>
#include "curl/curl.h"

namespace frostbyte {

struct MemoryStruct {
    char *memory;
    size_t size;
};
CURLcode newGetRequest(const char* url, struct MemoryStruct* chunk);

};
