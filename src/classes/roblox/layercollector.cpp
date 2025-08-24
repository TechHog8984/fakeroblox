#include "classes/roblox/layercollector.hpp"
#include "classes/roblox/instance.hpp"

namespace fakeroblox {

void rbxInstance_LayerCollector_init() {
    rbxClass::class_map["LayerCollector"]->constructor = [](lua_State* L, rbxInstance* instance) {
        std::get<bool>(instance->values["Enabled"].value) = true;
    };
}

}; // namespace fakeroblox
