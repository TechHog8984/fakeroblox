#pragma once

#include <map>
#include <string>

#include "raylib.h"

namespace fakeroblox {

Image* cloneImage(Image* original);

class ImageLoader {
public:
    static std::map<std::string, Image*> hash_image_map;

    static constexpr const char* SUPPORTED_IMAGE_TYPE_STRING = "expected PNG";

    static const char* getImageType(unsigned char* data, int data_size);
    static Image* getImage(unsigned char *data, int data_size);
    static void unload();
};

}; // namespace fakeroblox
