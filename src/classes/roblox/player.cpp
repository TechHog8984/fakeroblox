#include "classes/roblox/player.hpp"
#include "classes/roblox/instance.hpp"

namespace fakeroblox {

std::shared_ptr<rbxInstance> rbxPlayer::localplayer;
std::shared_ptr<rbxInstance> rbxPlayer::localmouse;

namespace rbxInstance_Player_methods {
    static int getMouse(lua_State* L) {
        lua_checkinstance(L, 1, "Player");
        return lua_pushinstance(L, rbxPlayer::localmouse);
    }
}; // namespace rbxInstance_Player_methods

void rbxInstance_Player_init(lua_State *L, std::shared_ptr<rbxInstance> players_service) {
    rbxClass::class_map["Player"]->methods["GetMouse"].func = rbxInstance_Player_methods::getMouse;

    rbxPlayer::localplayer = newInstance(L, "Player", players_service);
    rbxPlayer::localmouse = newInstance(L, "Mouse", rbxPlayer::localplayer);

    rbxPlayer::localplayer->values[PROP_INSTANCE_NAME].value = "LocalPlayer";
    rbxPlayer::localplayer->values["Mouse"].value = rbxPlayer::localmouse;

    players_service->values["LocalPlayer"].value = rbxPlayer::localplayer;
}

}; // namespace fakeroblox
