#include "http.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <curl/curl.h>

namespace fakeroblox {

/* credits START: https://stackoverflow.com/questions/27007379/how-do-i-get-response-value-using-curl-in-c/27007490#27007490 */

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = static_cast<char*>(realloc(mem->memory, mem->size + realsize + 1));
    if(mem->memory == NULL) {
        fprintf(stderr, "not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

CURLcode performRequest(CURL* curl, struct MemoryStruct* chunk, const char* method = "GET") {
    CURLcode res;

    chunk->memory = static_cast<char*>(malloc(1));
    chunk->size = 0;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) chunk);

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return res;
}
/* credits END: https://stackoverflow.com/questions/27007379/how-do-i-get-response-value-using-curl-in-c/27007490#27007490 */

int newGetRequest(const char* url, struct MemoryStruct* chunk) {
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        return performRequest(curl, chunk);
    }
    return 1;
}

};
