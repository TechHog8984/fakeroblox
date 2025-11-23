#include "classes/roblox/tweenservice.hpp"
#include "classes/roblox/datatypes/enum.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/instance.hpp"
#include "classes/tweeninfo.hpp"

#include "common.hpp"
#include "tween_functions.hpp"

#include "lua.h"
#include "lualib.h"

#include <variant>
#include <vector>

namespace fakeroblox {

std::vector<std::shared_ptr<rbxInstance>> TweenService::active_tween_list;
std::shared_mutex TweenService::active_tween_list_mutex;

std::map<std::shared_ptr<rbxInstance>, TweenObject> tween_instance_to_object_map;

void TweenService::activateTween(lua_State* L, std::shared_ptr<rbxInstance> tween_instance) {
    auto& playback_state = getInstanceValue<EnumItemWrapper>(tween_instance, "PlaybackState");
    if (playback_state.name == "Delayed" || playback_state.name == "Playing")
        return;

    std::lock_guard lock(TweenService::active_tween_list_mutex);

    auto& tween_info = getInstanceValue<TweenInfo>(tween_instance, "TweenInfo");
    auto& tween_object = tween_instance_to_object_map.at(tween_instance);

    const bool was_paused = playback_state.name == "Paused";

    const double clock = lua_clock();

    tween_object.tween_func = linear;
    switch (getEnumItemFromWrapper(tween_info.easing_direction).value) {
        case 0: // In
            switch (getEnumItemFromWrapper(tween_info.easing_style).value) {
                case 0: // Linear
                    break;
                case 1: // Sine
                    tween_object.tween_func = easeInSine;
                    break;
                case 2: // Back
                    tween_object.tween_func = easeInBack;
                    break;
                case 3: // Quad
                    tween_object.tween_func = easeInQuad;
                    break;
                case 4: // Quart
                    tween_object.tween_func = easeInQuart;
                    break;
                case 5: // Quint
                    tween_object.tween_func = easeInQuint;
                    break;
                case 6: // Bounce
                    tween_object.tween_func = easeInBounce;
                    break;
                case 7: // Elastic
                    tween_object.tween_func = easeInElastic;
                    break;
                case 8: // Exponential
                    tween_object.tween_func = easeInExponential;
                    break;
                case 9: // Circular
                    tween_object.tween_func = easeInCircular;
                    break;
                case 10: // Cubic
                    tween_object.tween_func = easeInCubic;
                    break;
            }
            break;
        case 1: // Out
            switch (getEnumItemFromWrapper(tween_info.easing_style).value) {
                case 0: // Linear
                    break;
                case 1: // Sine
                    tween_object.tween_func = easeOutSine;
                    break;
                case 2: // Back
                    tween_object.tween_func = easeOutBack;
                    break;
                case 3: // Quad
                    tween_object.tween_func = easeOutQuad;
                    break;
                case 4: // Quart
                    tween_object.tween_func = easeOutQuad;
                    break;
                case 5: // Quint
                    tween_object.tween_func = easeOutQuint;
                    break;
                case 6: // Bounce
                    tween_object.tween_func = easeOutBounce;
                    break;
                case 7: // Elastic
                    tween_object.tween_func = easeOutElastic;
                    break;
                case 8: // Exponential
                    tween_object.tween_func = easeOutExponential;
                    break;
                case 9: // Circular
                    tween_object.tween_func = easeOutCircular;
                    break;
                case 10: // Cubic
                    tween_object.tween_func = easeOutCubic;
                    break;
            }
            break;
        case 2: // InOut
            switch (getEnumItemFromWrapper(tween_info.easing_style).value) {
                case 0: // Linear
                    break;
                case 1: // Sine
                    tween_object.tween_func = easeInOutSine;
                    break;
                case 2: // Back
                    tween_object.tween_func = easeInOutBack;
                    break;
                case 3: // Quad
                    tween_object.tween_func = easeInOutQuad;
                    break;
                case 4: // Quart
                    tween_object.tween_func = easeInOutQuart;
                    break;
                case 5: // Quint
                    tween_object.tween_func = easeInOutQuint;
                    break;
                case 6: // Bounce
                    tween_object.tween_func = easeInOutBounce;
                    break;
                case 7: // Elastic
                    tween_object.tween_func = easeInOutElastic;
                    break;
                case 8: // Exponential
                    tween_object.tween_func = easeInOutExponential;
                    break;
                case 9: // Circular
                    tween_object.tween_func = easeInOutCircular;
                    break;
                case 10: // Cubic
                    tween_object.tween_func = easeInOutCubic;
                    break;
            }
            break;
    }

    if (!was_paused) {
        // TODO: verify these two lines
        tween_object.repeat_count = tween_info.repeat_count;
        tween_object.reverse_state = static_cast<TweenObject::ReverseState>(tween_info.reverses);

        for (size_t i = 0; i < tween_object.tween_list.size(); i++) {
            auto& tween = tween_object.tween_list[i];
            tween.original = getInstanceValueVariant(tween_object.instance, tween.property.c_str());
        }
    }

    // interrupt conflicting tweens
    {

    std::vector<const char*> this_properties;
    this_properties.reserve(tween_object.tween_list.size());

    std::transform(
        tween_object.tween_list.begin(), tween_object.tween_list.end(),
        std::back_inserter(this_properties),
        [](const Tween& tween) { return tween.property.c_str(); }
    );

    for (size_t i = 0; i < TweenService::active_tween_list.size(); i++) {
        auto& other_tween_instance = TweenService::active_tween_list[i];
        auto& other_tween_object = tween_instance_to_object_map.at(other_tween_instance);

        if (other_tween_object.instance != tween_object.instance)
            continue;

        for (size_t i2 = 0; i2 < other_tween_object.tween_list.size(); i2++) {
            auto& other_tween = other_tween_object.tween_list[i2];
            bool found = false;
            for (size_t i3 = 0; i3 < this_properties.size(); i3++) {
                auto& property = this_properties[i3];
                if (strequal(property, other_tween.property.c_str())) {
                    found = true;
                    break;
                }
            }

            other_tween.active = !found;
        }
    }
    }

    const bool has_delay = tween_info.delay_time;
    tween_object.has_delay = has_delay;

    if (has_delay && !was_paused) {
        tween_object.delay_timer = clock + tween_info.delay_time;
        playback_state.name = "Delayed";
    } else {
        tween_object.delay_timer = 0;

        tween_object.start_time = clock;
        if (was_paused)
            tween_object.start_time -= tween_object.elapsed;
        tween_object.end_time = clock + tween_info.time - (tween_object.elapsed * was_paused);
        playback_state.name = "Playing";
    }

    tween_instance->reportChanged(L, "PlaybackState");

    TweenService::active_tween_list.push_back(tween_instance);
}
void TweenService::cancelTween(lua_State* L, std::shared_ptr<rbxInstance> tween_instance) {
    auto& playback_state = getInstanceValue<EnumItemWrapper>(tween_instance, "PlaybackState");
    if (playback_state.name == "Completed" || playback_state.name == "Cancelled")
        return;
    playback_state.name = "Cancelled";
    tween_instance->reportChanged(L, "PlaybackState");
}
void TweenService::pauseTween(lua_State* L, std::shared_ptr<rbxInstance> tween_instance) {
    auto& playback_state = getInstanceValue<EnumItemWrapper>(tween_instance, "PlaybackState");
    if (playback_state.name != "Playing")
        return;
    playback_state.name = "Paused";
    tween_instance->reportChanged(L, "PlaybackState");
}

void TweenService::process(lua_State *L) {
    std::shared_lock lock(TweenService::active_tween_list_mutex);

    static std::vector<std::shared_ptr<rbxInstance>> completed_tween_list;
    completed_tween_list.clear();

    const double clock = lua_clock();
    for (size_t i = 0; i < TweenService::active_tween_list.size(); i++) {
        auto& tween_instance = TweenService::active_tween_list[i];
        auto& tween_object = tween_instance_to_object_map.at(tween_instance);

        auto& playback_state = getInstanceValue<EnumItemWrapper>(tween_instance, "PlaybackState");
        auto& tween_info = getInstanceValue<TweenInfo>(tween_instance, "TweenInfo");

        if (playback_state.name == "Cancelled")
            goto COMPLETE;

        if (tween_object.delay_timer) {
            if (clock >= tween_object.delay_timer) {
                playback_state.name = "Playing";
                tween_instance->reportChanged(L, "PlaybackState");

                tween_object.delay_timer = 0;
                goto RESET_TIMING;
            }
            continue;
        }

        {
        if (playback_state.name != "Playing")
            continue;

        if (tween_object.reset_properties) {
            tween_object.reset_properties = false;
            for (size_t i = 0; i < tween_object.tween_list.size(); i++) {
                auto& tween = tween_object.tween_list[i];
                setInstanceValueVariant(tween_object.instance, L, tween.property.c_str(), tween.original, true);
            }
        }

        tween_object.elapsed = clock - tween_object.start_time;
        double elapsed = tween_object.elapsed;
        if (tween_object.reverse_state == TweenObject::REVERSING)
            elapsed = tween_info.time - elapsed;

        bool has_active_tween = false;
        for (size_t i = 0; i < tween_object.tween_list.size(); i++) {
            auto& tween = tween_object.tween_list[i];

            if (!tween.active)
                continue;

            has_active_tween = true;

            const char* property = tween.property.c_str();
            auto& value = getInstanceValueVariant(tween_object.instance, property);

            if (std::holds_alternative<bool>(value))
                setInstanceValue<bool>(tween_object.instance, L, property, static_cast<bool>(tween_object.tween_func(elapsed, std::get<bool>(tween.original), std::get<bool>(tween.target) - std::get<bool>(tween.original), tween_info.time)));
            else if (std::holds_alternative<int32_t>(value))
                setInstanceValue(tween_object.instance, L, property, static_cast<int32_t>(tween_object.tween_func(elapsed, std::get<int32_t>(tween.original), std::get<int32_t>(tween.target) - std::get<int32_t>(tween.original), tween_info.time)));
            else if (std::holds_alternative<int64_t>(value))
                setInstanceValue(tween_object.instance, L, property, static_cast<int64_t>(tween_object.tween_func(elapsed, std::get<int64_t>(tween.original), std::get<int64_t>(tween.target) - std::get<int64_t>(tween.original), tween_info.time)));
            else if (std::holds_alternative<float>(value))
                setInstanceValue(tween_object.instance, L, property, static_cast<float>(tween_object.tween_func(elapsed, std::get<float>(tween.original), std::get<float>(tween.target) - std::get<float>(tween.original), tween_info.time)));
            else if (std::holds_alternative<double>(value))
                setInstanceValue(tween_object.instance, L, property, static_cast<double>(tween_object.tween_func(elapsed, std::get<double>(tween.original), std::get<double>(tween.target) - std::get<double>(tween.original), tween_info.time)));
            else if (std::holds_alternative<EnumItemWrapper>(value)) {
                // NOTE: untested; not going to test rn because I can't find a property that doesn't throw the 'is not a datatype that can be tweened' error
                // const unsigned int original_value = getEnumItemFromWrapper(std::get<EnumItemWrapper>(tween.original)).value;
                // const unsigned int target_value = getEnumItemFromWrapper(std::get<EnumItemWrapper>(tween.target)).value;

                // NOTE: we need to update to new parameters
                // unsigned int id = tween_object.tween_func(elapsed, original_value - target_value, tween_info.time);

                // auto& wrapper = std::get<EnumItemWrapper>(value);
                // wrapper.name = getEnumItemFromValue(wrapper.enum_name.c_str(), id).name;
            } else if (std::holds_alternative<Color>(value)) {
                auto& original = std::get<Color>(tween.original);
                auto& target = std::get<Color>(tween.target);

                Color color{0, 0, 0, original.a};
                color.r = tween_object.tween_func(elapsed, original.r, target.r - original.r, tween_info.time);
                color.g = tween_object.tween_func(elapsed, original.g, target.g - original.g, tween_info.time);
                color.b = tween_object.tween_func(elapsed, original.b, target.b - original.b, tween_info.time);

                setInstanceValue(tween_object.instance, L, property, color);
            } else if (std::holds_alternative<UDim>(value)) {
                auto& original = std::get<UDim>(tween.original);
                auto& target = std::get<UDim>(tween.target);

                UDim udim{0, 0};
                udim.scale = tween_object.tween_func(elapsed, original.scale, target.scale - original.scale, tween_info.time);
                udim.offset = tween_object.tween_func(elapsed, original.offset, target.offset - original.offset, tween_info.time);

                setInstanceValue(tween_object.instance, L, property, udim);
            } else if (std::holds_alternative<UDim2>(value)) {
                auto& original = std::get<UDim2>(tween.original);
                auto& target = std::get<UDim2>(tween.target);

                UDim2 udim2{{0, 0}, {0, 0}};
                udim2.x.scale = tween_object.tween_func(elapsed, original.x.scale, target.x.scale - original.x.scale, tween_info.time);
                udim2.x.offset = tween_object.tween_func(elapsed, original.x.offset, target.x.offset - original.x.offset, tween_info.time);
                udim2.y.scale = tween_object.tween_func(elapsed, original.y.scale, target.y.scale - original.y.scale, tween_info.time);
                udim2.y.offset = tween_object.tween_func(elapsed, original.y.offset, target.y.offset - original.y.offset, tween_info.time);

                setInstanceValue(tween_object.instance, L, property, udim2);
            } else if (std::holds_alternative<Vector2>(value)) {
                auto& original = std::get<Vector2>(tween.original);
                auto& target = std::get<Vector2>(tween.target);

                Vector2 vector2{0, 0};
                vector2.x = tween_object.tween_func(elapsed, original.x, target.x - original.x, tween_info.time);
                vector2.y = tween_object.tween_func(elapsed, original.y, target.y - original.y, tween_info.time);

                setInstanceValue(tween_object.instance, L, property, vector2);
            } else if (std::holds_alternative<Vector3>(value)) {
                auto& original = std::get<Vector3>(tween.original);
                auto& target = std::get<Vector3>(tween.target);

                Vector3 vector3{0, 0};
                vector3.x = tween_object.tween_func(elapsed, original.x, target.x - original.x, tween_info.time);
                vector3.y = tween_object.tween_func(elapsed, original.y, target.y - original.y, tween_info.time);
                vector3.z = tween_object.tween_func(elapsed, original.z, target.z - original.z, tween_info.time);

                setInstanceValue(tween_object.instance, L, property, vector3);
            } else
                assert(!"UNHANDLED TYPE FOR TWEEN");
        }

        // cancel if every tween has been interrupted
        if (!has_active_tween && !tween_object.is_empty) {
            playback_state.name = "Cancelled";
            tween_instance->reportChanged(L, "PlaybackState");
            goto COMPLETE;
        }

        if (clock >= tween_object.end_time) {
            switch (tween_object.reverse_state) {
                case TweenObject::REVERSING:
                    tween_object.reverse_state = TweenObject::WAITING;
                    break;
                case TweenObject::WAITING:
                    tween_object.reverse_state = TweenObject::REVERSING;
                    goto RESET_TIMING;
                    break;
                default:
                    break;
            }

            if (tween_object.repeat_count) {
                if (tween_object.repeat_count > 0)
                    tween_object.repeat_count--;

                tween_object.reset_properties = true;
                if (tween_object.has_delay) {
                    tween_object.delay_timer = clock + tween_info.delay_time;

                    playback_state.name = "Delayed";
                    tween_instance->reportChanged(L, "PlaybackState");
                    continue;
                }

                goto RESET_TIMING;
            }

            playback_state.name = "Completed";
            tween_instance->reportChanged(L, "PlaybackState");
            goto COMPLETE;
        }
        }

        continue;

        COMPLETE:
        completed_tween_list.push_back(tween_instance);
        continue;

        RESET_TIMING:
        tween_object.start_time = clock;
        tween_object.end_time = clock + tween_info.time;
        continue;
    }

    for (size_t i = 0; i < completed_tween_list.size(); i++) {
        auto& tween_instance = completed_tween_list[i];
        auto& playback_state = getInstanceValue<EnumItemWrapper>(tween_instance, "PlaybackState");

        TweenService::active_tween_list.erase(std::find(TweenService::active_tween_list.begin(), TweenService::active_tween_list.end(), tween_instance));

        pushFunctionFromLookup(L, fireRBXScriptSignal);

        tween_instance->pushEvent(L, "Completed");
        pushEnumItem(L, playback_state);

        lua_call(L, 2, 0);
    }
}

namespace rbxInstance_TweenService_methods {
    static int create(lua_State* L) {
        lua_checkinstance(L, 1, "TweenService");
        auto instance = lua_checkinstance(L, 2);
        auto tweeninfo = lua_checktweeninfo(L, 3);
        luaL_checktype(L, 4, LUA_TTABLE);

        auto tween_instance = newInstance(L, "Tween");

        // FIXME: this should happen in constructors
        setInstanceValue(tween_instance, L, "Instance", instance, true);
        setInstanceValue(tween_instance, L, "TweenInfo", *tweeninfo, true);

        TweenObject tween_object;
        tween_object.instance = instance;
        tween_object.tween_instance = tween_instance;

        bool is_empty = true;
        lua_pushnil(L);
        while (lua_next(L, 4)) {
            is_empty = false;
            // FIXME: we need a function for checking if a property exists and if not erroring; it will also be good for when we implement capabilties; call it here (actually, getinstancevaluevariant should call it)
            const char* property = luaL_checkstring(L, -2);
            auto& original = getInstanceValueVariant(instance, property);

            // TODO: Vector2int16
            // TODO: there is another err, "TweenService:Create property named 'PROPERTY' on object 'NAME' is not a data type that can be tweened";
            // we need to figure out what types are of DescribedBase
            if (!(std::holds_alternative<bool>(original) || std::holds_alternative<int32_t>(original) || std::holds_alternative<int64_t>(original) || std::holds_alternative<float>(original) || std::holds_alternative<double>(original) || std::holds_alternative<EnumItemWrapper>(original) || std::holds_alternative<Color>(original) || std::holds_alternative<UDim>(original) || std::holds_alternative<UDim2>(original) || std::holds_alternative<Vector2>(original) || std::holds_alternative<Vector3>(original)))
                luaL_error(L, "property named '%s' cannot be tweened due to type mismatch (property is a 'DescribedBase', but given type is '%s')", property, luaL_typename(L, -1));

            tween_object.tween_list.push_back({
                .active = true,
                .property = property,
                .target = luaValueToValueVariant(L, -1, original) 
            });

            lua_pop(L, 1);
        }

        tween_object.is_empty = is_empty;

        tween_instance_to_object_map[tween_instance] = tween_object;

        return lua_pushinstance(L, tween_instance);
    }
}; // namespace rbxInstance_TweenService_methods

namespace rbxInstance_TweenService_static_methods {
    static int getValue(lua_State* L) {
        TweenFunctionAlpha tween_func = linearAlpha;

        const double alpha = luaL_checknumber(L, 1);
        auto easing_style = lua_checkenumitem(L, 2, "EasingStyle");
        auto easing_direction = lua_checkenumitem(L, 3, "EasingDirection");

        switch (easing_direction->value) {
            case 0: // In
                switch (easing_style->value) {
                    case 0: // Linear
                        break;
                    case 1: // Sine
                        tween_func = easeInSineAlpha;
                        break;
                    case 2: // Back
                        tween_func = easeInBackAlpha;
                        break;
                    case 3: // Quad
                        tween_func = easeInQuadAlpha;
                        break;
                    case 4: // Quart
                        tween_func = easeInQuartAlpha;
                        break;
                    case 5: // Quint
                        tween_func = easeInQuintAlpha;
                        break;
                    case 6: // Bounce
                        tween_func = easeInBounceAlpha;
                        break;
                    case 7: // Elastic
                        tween_func = easeInElasticAlpha;
                        break;
                    case 8: // Exponential
                        tween_func = easeInExponentialAlpha;
                        break;
                    case 9: // Circular
                        tween_func = easeInCircularAlpha;
                        break;
                    case 10: // Cubic
                        tween_func = easeInCubicAlpha;
                        break;
                }
                break;
            case 1: // Out
                switch (easing_style->value) {
                    case 0: // Linear
                        break;
                    case 1: // Sine
                        tween_func = easeOutSineAlpha;
                        break;
                    case 2: // Back
                        tween_func = easeOutBackAlpha;
                        break;
                    case 3: // Quad
                        tween_func = easeOutQuadAlpha;
                        break;
                    case 4: // Quart
                        tween_func = easeOutQuadAlpha;
                        break;
                    case 5: // Quint
                        tween_func = easeOutQuintAlpha;
                        break;
                    case 6: // Bounce
                        tween_func = easeOutBounceAlpha;
                        break;
                    case 7: // Elastic
                        tween_func = easeOutElasticAlpha;
                        break;
                    case 8: // Exponential
                        tween_func = easeOutExponentialAlpha;
                        break;
                    case 9: // Circular
                        tween_func = easeOutCircularAlpha;
                        break;
                    case 10: // Cubic
                        tween_func = easeOutCubicAlpha;
                        break;
                }
                break;
            case 2: // InOut
                switch (easing_style->value) {
                    case 0: // Linear
                        break;
                    case 1: // Sine
                        tween_func = easeInOutSineAlpha;
                        break;
                    case 2: // Back
                        tween_func = easeInOutBackAlpha;
                        break;
                    case 3: // Quad
                        tween_func = easeInOutQuadAlpha;
                        break;
                    case 4: // Quart
                        tween_func = easeInOutQuartAlpha;
                        break;
                    case 5: // Quint
                        tween_func = easeInOutQuintAlpha;
                        break;
                    case 6: // Bounce
                        tween_func = easeInOutBounceAlpha;
                        break;
                    case 7: // Elastic
                        tween_func = easeInOutElasticAlpha;
                        break;
                    case 8: // Exponential
                        tween_func = easeInOutExponentialAlpha;
                        break;
                    case 9: // Circular
                        tween_func = easeInOutCircularAlpha;
                        break;
                    case 10: // Cubic
                        tween_func = easeInOutCubicAlpha;
                        break;
                }
                break;
        }

        double result = tween_func(alpha);
        result = result < 0.0 ? 0.0 : (result > 1.0 ? 1.0 : result);

        lua_pushnumber(L, result);
        return 1;
    }
}; // namespace rbxInstance_TweenService_methods

void rbxInstance_TweenService_init() {
    rbxClass::class_map["TweenService"]->methods["Create"].func = rbxInstance_TweenService_methods::create;

    rbxClass::class_map["TweenService"]->methods["GetValue"].func = rbxInstance_TweenService_static_methods::getValue;
}

}; // namespace fakeroblox
