#include "classes/roblox/layercollector.hpp"
#include "classes/roblox/instance.hpp"

namespace fakeroblox {

void rbxInstance_LayerCollector_init() {
    rbxClass::class_map["LayerCollector"]->constructor = [](lua_State* L, std::shared_ptr<rbxInstance> instance) {
        setInstanceValue(instance, L, "Enabled", true, true);
    };
}

}; // namespace fakeroblox
