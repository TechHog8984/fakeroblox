#include "classes/roblox/tweenbase.hpp"
#include "classes/roblox/instance.hpp"
#include "classes/roblox/tweenservice.hpp"

namespace fakeroblox {

namespace rbxInstance_TweenBase_methods {
    static int cancel(lua_State* L) {
        auto instance = lua_checkinstance(L, 1, "TweenBase");

        TweenService::cancelTween(L, instance);
        return 0;
    }
    static int play(lua_State* L) {
        auto instance = lua_checkinstance(L, 1, "TweenBase");

        TweenService::activateTween(L, instance);
        return 0;
    }
    static int pause(lua_State* L) {
        auto instance = lua_checkinstance(L, 1, "TweenBase");

        TweenService::pauseTween(L, instance);
        return 0;
    }
}; // rbxInstance_TweenBase_methods

void rbxInstance_TweenBase_init() {
    rbxClass::class_map["TweenBase"]->constructor = [](lua_State* L, std::shared_ptr<rbxInstance> instance) {
        getInstanceValue<EnumItemWrapper>(instance, "PlaybackState").name = "Begin";
    };

    rbxClass::class_map["TweenBase"]->methods["Cancel"].func = rbxInstance_TweenBase_methods::cancel;
    rbxClass::class_map["TweenBase"]->methods["Play"].func = rbxInstance_TweenBase_methods::play;
    rbxClass::class_map["TweenBase"]->methods["Pause"].func = rbxInstance_TweenBase_methods::pause;
}

}; // namespace fakeroblox
