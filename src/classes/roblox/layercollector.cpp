#include "classes/roblox/layercollector.hpp"
#include "classes/roblox/instance.hpp"

namespace fakeroblox {

void rbxInstance_LayerCollector_init() {
    rbxClass::class_map["LayerCollector"]->constructor = [](lua_State* L, std::shared_ptr<rbxInstance> instance) {
        instance->setValue<bool>(L, "Enabled", true, true);
    };
}

}; // namespace fakeroblox
