#include "classes/roblox/players.hpp"
#include "classes/roblox/player.hpp"
#include "classes/roblox/serviceprovider.hpp"

namespace fakeroblox {

namespace rbxInstance_Players_methods {
    static int getPlayers(lua_State* L) {
        lua_checkinstance(L, 1, "Players");

        createweaktable(L, 1, 0);
        lua_pushinstance(L, rbxPlayer::localplayer);
        lua_rawseti(L, -2, 1);

        return 1;
    }
}; // namespace rbxInstance_Players_methods

void rbxInstance_Players_init(lua_State* L, std::shared_ptr<rbxInstance> datamodel) {
    rbxClass::class_map["Players"]->methods["GetPlayers"].func = rbxInstance_Players_methods::getPlayers;

    auto players_service = ServiceProvider::getService(L, datamodel, "Players");
    rbxInstance_Player_init(L, players_service);
}

}; // namespace fakeroblox
