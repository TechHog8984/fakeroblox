#include "imageloader.hpp"
#include "common.hpp"

namespace fakeroblox {

std::map<std::string, Image*> ImageLoader::hash_image_map;

Image* cloneImage(Image* original) {
    return new Image(ImageCopy(*original));
}

static bool checkSignaturePNG(const unsigned char *data, int data_size) {
    if (data_size < 8)
        return false;
    const unsigned char png_sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    return memcmp(data, png_sig, 8) == 0;
}

const char* ImageLoader::getImageType(unsigned char* data, int data_size) {
    if (checkSignaturePNG(data, data_size))
        return ".png";
    return nullptr;
}

Image* ImageLoader::getImage(unsigned char* data, int data_size) {
    const char* file_extension = getImageType(data, data_size);
    assert(file_extension);

    std::string hashed = sha1ToString(ComputeSHA1(data, data_size));

    auto cached = hash_image_map.find(hashed);
    if (cached != hash_image_map.end())
        return cached->second;

    Image* image = new Image(LoadImageFromMemory(file_extension, data, data_size));

    hash_image_map[hashed] = image;

    return image;
}

void ImageLoader::unload() {
    for (auto& pair : hash_image_map) {
        UnloadImage(*pair.second);
        delete pair.second;
    }
}

}; // namespace fakerobox
