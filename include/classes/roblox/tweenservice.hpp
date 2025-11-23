#pragma once

#include "classes/roblox/instance.hpp"
#include "tween_functions.hpp"

namespace fakeroblox {

struct Tween {
    bool active;
    std::string property;
    rbxValueVariant original;
    rbxValueVariant target;
};
struct TweenObject {
    bool is_empty = false;
    TweenFunction tween_func; // for optimization purposes so we don't compute this constantly

    bool has_delay = false;
    double delay_timer = 0;

    double start_time = 0;
    double end_time = 0;
    double elapsed = 0;

    int repeat_count = 0;
    bool reset_properties = false;

    enum ReverseState {
        // NOTE: activateTween relies on NA as 0 and WAITING as 1
        NA,
        WAITING,
        REVERSING
    } reverse_state = NA;

    std::shared_ptr<rbxInstance> instance;
    std::shared_ptr<rbxInstance> tween_instance;
    std::vector<Tween> tween_list;
};

class TweenService {
    static std::vector<std::shared_ptr<rbxInstance>> active_tween_list;
    static std::shared_mutex active_tween_list_mutex;
public:
    static void activateTween(lua_State* L, std::shared_ptr<rbxInstance> tween_instance);
    static void cancelTween(lua_State* L, std::shared_ptr<rbxInstance> tween_instance);
    static void pauseTween(lua_State* L, std::shared_ptr<rbxInstance> tween_instance);

    static void process(lua_State* L);
};

void rbxInstance_TweenService_init();

}; // namespace fakeroblox
