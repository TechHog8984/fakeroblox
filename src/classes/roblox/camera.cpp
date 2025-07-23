#include "classes/roblox/camera.hpp"

namespace fakeroblox {

Vector2 rbxCamera::screen_size{0, 0};
std::shared_ptr<rbxInstance> camera;

int global_width = 0;
int global_height = 0;
void rbxInstance_Camera_updateViewport(lua_State* L) {
    auto width = rbxCamera::screen_size.x;
    auto height = rbxCamera::screen_size.y;

    if (width != global_width || height != global_height) {
        camera->setValue(L, "ViewportSize", rbxCamera::screen_size);
    }
    global_width = width;
    global_height = height;
}

void rbxInstance_Camera_init(lua_State *L, rbxInstance* workspace) {
    camera = newInstance(L, "Camera");
    workspace->setValue(L, "CurrentCamera", camera);
}

}; // namespace fakeroblox
