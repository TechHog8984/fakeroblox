#include "classes/roblox/userinputservice.hpp"
#include "classes/roblox/baseplayergui.hpp"
#include "classes/roblox/datatypes/enum.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/instance.hpp"
#include "classes/roblox/serviceprovider.hpp"
#include "classes/vector2.hpp"

#include "console.hpp"
#include "taskscheduler.hpp"

#include "raylib.h"

#include "lualib.h"

#include <map>
#include <queue>
#include <stdexcept>

namespace fakeroblox {

void genericFire(lua_State* L, std::shared_ptr<rbxInstance> instance, const char* event) {
    pushFunctionFromLookup(L, fireRBXScriptSignal);
    instance->pushEvent(L, event);

    lua_call(L, 1, 0);
}

void genericFireInputObject(lua_State* L, std::shared_ptr<rbxInstance> instance, const char* event, std::shared_ptr<rbxInstance> input_object, bool game_processed) {
    pushFunctionFromLookup(L, fireRBXScriptSignal);
    instance->pushEvent(L, event);

    lua_pushinstance(L, input_object);
    lua_pushboolean(L, game_processed);

    lua_call(L, 3, 0);
}

#define keyShifted(key) (key + MAX_KEYBOARD_KEYS)

static std::map<unsigned int, const char*> raylib_key_to_keycode_map = {
    { KEY_NULL, "Unknown"},

    { KEY_APOSTROPHE, "Quote"},
    { keyShifted(KEY_APOSTROPHE), "QuotedDouble"},
    { KEY_COMMA, "Comma"},
    { keyShifted(KEY_COMMA), "LessThan"},
    { KEY_MINUS, "Minus"},
    { keyShifted(KEY_MINUS), "Underscore"},
    { KEY_PERIOD, "Period"},
    { keyShifted(KEY_PERIOD), "GreaterThan"},
    { KEY_SLASH, "Slash"},
    { keyShifted(KEY_SLASH), "Question"},
    { KEY_ZERO, "Zero"},
    { keyShifted(KEY_ZERO), "RightParenthesis"},
    { KEY_ONE, "One"},
    // NOTE: Where's the exclamation point? Wtf Roblox...
    { KEY_TWO, "Two"},
    { keyShifted(KEY_TWO), "At"},
    { KEY_THREE, "Three"},
    { keyShifted(KEY_THREE), "Hash"},
    { KEY_FOUR, "Four"},
    { keyShifted(KEY_FOUR), "Dollar"},
    { KEY_FIVE, "Five"},
    { keyShifted(KEY_FIVE), "Percent"},
    { KEY_SIX, "Six"},
    { keyShifted(KEY_SIX), "Caret"},
    { KEY_SEVEN, "Seven"},
    { keyShifted(KEY_SEVEN), "Ampersand"},
    { KEY_EIGHT, "Eight"},
    { keyShifted(KEY_EIGHT), "Asterisk"},
    { KEY_NINE, "Nine"},
    { keyShifted(KEY_NINE), "LeftParenthesis"},
    { KEY_SEMICOLON, "Semicolon"},
    { keyShifted(KEY_SEMICOLON), "Colon"},
    { KEY_EQUAL, "Equals"},
    { keyShifted(KEY_EQUAL), "Plus"},

    { KEY_A, "A"},
    { KEY_B, "B"},
    { KEY_C, "C"},
    { KEY_D, "D"},
    { KEY_E, "E"},
    { KEY_F, "F"},
    { KEY_G, "G"},
    { KEY_H, "H"},
    { KEY_I, "I"},
    { KEY_J, "J"},
    { KEY_K, "K"},
    { KEY_L, "L"},
    { KEY_M, "M"},
    { KEY_N, "N"},
    { KEY_O, "O"},
    { KEY_P, "P"},
    { KEY_Q, "Q"},
    { KEY_R, "R"},
    { KEY_S, "S"},
    { KEY_T, "T"},
    { KEY_U, "U"},
    { KEY_V, "V"},
    { KEY_W, "W"},
    { KEY_X, "X"},
    { KEY_Y, "Y"},
    { KEY_Z, "Z"},

    { KEY_LEFT_BRACKET, "LeftBracket"},
    { keyShifted(KEY_LEFT_BRACKET), "LeftCurly"},
    { KEY_BACKSLASH, "BackSlash"},
    { keyShifted(KEY_BACKSLASH), "Pipe"},
    { KEY_RIGHT_BRACKET, "RightBracket"},
    { keyShifted(KEY_RIGHT_BRACKET), "RightCurly"},
    { KEY_GRAVE, "Backquote"},
    { keyShifted(KEY_GRAVE), "Tilde"},

    { KEY_SPACE, "Space"},
    { KEY_ESCAPE, "Escape"},
    { KEY_ENTER, "Return"},
    { KEY_TAB, "Tab"},
    { KEY_BACKSPACE, "Backspace"},
    { KEY_INSERT, "Insert"},
    { KEY_DELETE, "Delete"},
    { KEY_RIGHT, "Right"},
    { KEY_LEFT, "Left"},
    { KEY_DOWN, "Down"},
    { KEY_UP, "Up"},
    { KEY_PAGE_UP, "PageUp"},
    { KEY_PAGE_DOWN, "PageDown"},
    { KEY_HOME, "Home"},
    { KEY_END, "End"},
    { KEY_CAPS_LOCK, "CapsLock"},
    { KEY_SCROLL_LOCK, "ScrollLock"},
    { KEY_NUM_LOCK, "NumLock"},
    { KEY_PRINT_SCREEN, "Print"},
    { KEY_PAUSE, "Pause"},
    { KEY_F1, "F1"},
    { KEY_F2, "F2"},
    { KEY_F3, "F3"},
    { KEY_F4, "F4"},
    { KEY_F5, "F5"},
    { KEY_F6, "F6"},
    { KEY_F7, "F7"},
    { KEY_F8, "F8"},
    { KEY_F9, "F9"},
    { KEY_F10, "F10"},
    { KEY_F11, "F11"},
    { KEY_F12, "F12"},
    { KEY_LEFT_SHIFT, "LeftShift"},
    { KEY_LEFT_CONTROL, "LeftControl"},
    { KEY_LEFT_ALT, "LeftAlt"},
    { KEY_LEFT_SUPER, "LeftSuper"},
    { KEY_RIGHT_SHIFT, "RightShift"},
    { KEY_RIGHT_CONTROL, "RightControl"},
    { KEY_RIGHT_ALT, "RightAlt"},
    { KEY_RIGHT_SUPER, "RightSuper"},
    { KEY_KB_MENU, "Menu"},

    { KEY_KP_0, "KeypadZero"},
    { KEY_KP_1, "KeypadOne"},
    { KEY_KP_2, "KeypadTwo"},
    { KEY_KP_3, "KeypadThree"},
    { KEY_KP_4, "KeypadFour"},
    { KEY_KP_5, "KeypadFive"},
    { KEY_KP_6, "KeypadSix"},
    { KEY_KP_7, "KeypadSeven"},
    { KEY_KP_8, "KeypadEight"},
    { KEY_KP_9, "KeypadNine"},
    { KEY_KP_DECIMAL, "KeypadPeriod"},
    { KEY_KP_DIVIDE, "KeypadDivide"},
    { KEY_KP_MULTIPLY, "KeypadMultiply"},
    { KEY_KP_SUBTRACT, "KeypadMinus"},
    { KEY_KP_ADD, "KeypadPlus"},
    { KEY_KP_ENTER, "KeypadEnter"},
    { KEY_KP_EQUAL, "KeypadEquals"},
};
const int MAX_KEY = KEY_KB_MENU;

std::array<std::shared_ptr<rbxInstance>, MAX_KEY + 6> input_object_array; 

const char* getKeyCodeName(lua_State* L, bool shift, unsigned int key) {
    const char* value = nullptr;
    try {
        if (shift && raylib_key_to_keycode_map.find(keyShifted(key)) != raylib_key_to_keycode_map.end())
            value = raylib_key_to_keycode_map.at(keyShifted(key));
        else
            value = raylib_key_to_keycode_map.at(key);
    } catch(std::out_of_range& e) {
        getTask(L)->console->errorf("[UserInputService::process] unhandled key code %ud", key);
    }
    return value;
}

const char* MOUSE_BUTTON_MAP[] = {
    "MouseButton1",
    "MouseButton2",
    "MouseButton3"
};

const char* INPUT_SIGNAL_MAP[] = {
    "InputBegan",
    "InputChanged",
    "InputEnded"
};
const char* INPUT_STATE_MAP[] = {
    "Begin",
    "Change",
    "End"
};

struct InputEventMouseClick {
    unsigned int mouse;
};
struct InputEventKeyboard {
    unsigned int key;
    const char* keycode;
};
struct InputEvent {
    enum {
        MouseClick,
        MouseMovement,
        MouseWheel,
        Keyboard
    } type;

    InputState state;
    std::weak_ptr<rbxInstance> instance;

    union {
        InputEventMouseClick mouse_click;
        InputEventKeyboard keyboard;
    };
};

std::queue<InputEvent> input_event_queue;
std::mutex input_event_mutex;

void pushInputEvent(InputEvent& event) {
    std::lock_guard<std::mutex> lock(input_event_mutex);

    input_event_queue.push(event);
}

void UserInputService::signalMouseMovement(std::shared_ptr<rbxInstance> instance, InputState type) {
    InputEvent event = {
        .type = InputEvent::MouseMovement,
        .state = type,
        .instance = instance
    };
    pushInputEvent(event);
}

int global_mouse_wheel = 0;
bool UserInputService::is_window_focused = false;
Vector2 UserInputService::mouse_position = GetMousePosition();

void UserInputService::process(lua_State *L) {
    const Vector2 mouse_delta = GetMouseDelta();
    UserInputService::mouse_position = GetMousePosition();
    const Vector2 mouse_wheel_vector = GetMouseWheelMoveV();

    const int mouse_wheel_y = mouse_wheel_vector.y;
    const int mouse_wheel = mouse_wheel_y ? (mouse_wheel_y > 0 ? 1 : -1) : 0;

    const bool is_window_focused = IsWindowFocused();
    if (is_window_focused != UserInputService::is_window_focused)
        signalMouseMovement(nullptr, is_window_focused ? InputBegan : InputEnded);
    UserInputService::is_window_focused = is_window_focused;

    for (unsigned int mouse = 0; mouse < 3; mouse++) {
        const bool pressed = IsMouseButtonPressed(mouse);
        const bool released = IsMouseButtonReleased(mouse);

        if (pressed || released) {
            InputEvent event = {
                .type = InputEvent::MouseClick,
                .state = pressed ? InputBegan : InputEnded,
                .mouse_click = {
                    .mouse = mouse
                }
            };
            pushInputEvent(event);
        }
    }

    if (mouse_delta.x || mouse_delta.y) {
        InputEvent event = {
            .type = InputEvent::MouseMovement,
            .state = InputChanged
        };
        pushInputEvent(event);
    }

    if (mouse_wheel && mouse_wheel != global_mouse_wheel) {
        global_mouse_wheel = mouse_wheel;
        InputEvent event = {
            .type = InputEvent::MouseWheel,
            .state = InputChanged
        };
        pushInputEvent(event);
    }

    // TODO: cache this?
    const bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    for (unsigned int key = 0; key < MAX_KEYBOARD_KEYS; key++)  {
        const bool pressed = IsKeyPressed(key);
        const bool released = IsKeyReleased(key);

        if (pressed || released) {
            const char* keycode = getKeyCodeName(L, shift, key);
            if (!keycode) continue;

            InputEvent event = {
                .type = InputEvent::Keyboard,
                .state = pressed ? InputBegan : InputEnded,
                .keyboard = {
                    .key = key,
                    .keycode = keycode,
                }
            };
            pushInputEvent(event);
        }
    }

    while (!input_event_queue.empty()) {
        auto& event = input_event_queue.front();

        size_t array_index;
        if (event.type == InputEvent::MouseClick)
            array_index = MAX_KEY + 1 + event.mouse_click.mouse;
        else if (event.type == InputEvent::MouseMovement)
            array_index = MAX_KEY + 4;
        else if (event.type == InputEvent::MouseWheel)
            array_index = MAX_KEY + 5;
        else {
            assert(event.type == InputEvent::Keyboard);
            array_index = event.keyboard.key;
        }

        const char* input_state = INPUT_STATE_MAP[event.state];
        const char* input_signal = INPUT_SIGNAL_MAP[event.state];

        std::string key_code_name = "Unknown";
        std::string user_input_type_name = "None";

        std::shared_ptr<rbxInstance> input_object = input_object_array[array_index];
        if (!input_object) {
            input_object = newInstance(L, "InputObject");
            input_object_array[array_index] = input_object;

            auto& key_code = getInstanceValue<EnumItemWrapper>(input_object, "KeyCode");
            auto& user_input_type = getInstanceValue<EnumItemWrapper>(input_object, "UserInputType");

            switch (event.type) {
                case InputEvent::MouseClick:
                    global_mouse_wheel = 0;
                    user_input_type_name.assign(MOUSE_BUTTON_MAP[event.mouse_click.mouse]);
                    break;
                case InputEvent::MouseMovement:
                    user_input_type_name.assign("MouseMovement");
                    break;
                case InputEvent::MouseWheel:
                    user_input_type_name.assign("MouseWheel");
                    break;
                case InputEvent::Keyboard:
                    key_code_name.assign(event.keyboard.keycode);
                    user_input_type_name.assign("Keyboard");
                    break;
            }

            key_code.name.assign(key_code_name);
            user_input_type.name.assign(user_input_type_name);
        }

        if (event.state == InputEnded)
            input_object_array[array_index].reset();

        Vector3 position = {
            mouse_position.x,
            mouse_position.y,
            static_cast<float>(global_mouse_wheel)
        };
        Vector3 delta = {
            mouse_delta.x,
            mouse_delta.y,
            0
        };

        getInstanceValue<EnumItemWrapper>(input_object, "UserInputState").name.assign(input_state);
        setInstanceValue(input_object, L, "Position", position);
        setInstanceValue(input_object, L, "Delta", delta);

        // TODO: gameProcessedEvent
        const static bool game_processed = false;

        std::shared_ptr<rbxInstance> event_instance;
        if (auto ptr = event.instance.lock())
            event_instance = ptr;
        else
            event_instance = ServiceProvider::service_map.at("UserInputService");

        genericFireInputObject(L, event_instance, input_signal, input_object, game_processed);

        auto hovered_gui_objects = getGuiObjectsHovered();

        if (event.type != InputEvent::MouseMovement)
            for (size_t i = 0; i < hovered_gui_objects.size(); i++) {
                auto instance = hovered_gui_objects[i].lock();
                if (!instance)
                    continue;
                genericFireInputObject(L, instance, input_signal, input_object, game_processed);
            }

        auto clickable = getClickableGuiObject().lock();
        if (clickable) {
            if (event.type == InputEvent::MouseClick) {
                // TODO: MouseButton*Click & Activated signals
                std::string button_signal = "MouseButton";
                button_signal += ('1' + event.mouse_click.mouse);
                button_signal.append(event.state == InputBegan ? "Down" : "Up");
                genericFire(L, clickable, button_signal.c_str());
            }
        }

        input_event_queue.pop();
    }
}

#undef keyShifted

namespace rbxInstance_UserInputService_methods {
    static int getMouseLocation(lua_State* L) {
        lua_checkinstance(L, 1, "UserInputService");

        return pushVector2(L, UserInputService::mouse_position);
    }
    static int isMouseButtonPressed(lua_State* L) {
        lua_checkinstance(L, 1, "UserInputService");

        luaL_argcheck(L, lua_isnumber(L, 2) || lua_isuserdata(L, 2), 2, "expected number or userdata");

        int mouse = -1;
        if (lua_isnumber(L, 2))
            mouse = luaL_checkinteger(L, 2);
        else 
            mouse = lua_checkenumitem(L, 2)->value;

        if (mouse > 2) {
            getTask(L)->console->warning("UserInputService.IsMouseButtonPressed - UserInputType provided is not a mouse button.");
            lua_pushboolean(L, false);
        } else
            lua_pushboolean(L, IsMouseButtonPressed(mouse));

        return 1;
    }
}; // namespace rbxInstance_UserInputService_methods

void rbxInstance_UserInputService_init() {
    UserInputService::signalMouseMovement(nullptr, InputBegan);

    rbxClass::class_map["UserInputService"]->methods["GetMouseLocation"].func = rbxInstance_UserInputService_methods::getMouseLocation;
    rbxClass::class_map["UserInputService"]->methods["IsMouseButtonPressed"].func = rbxInstance_UserInputService_methods::isMouseButtonPressed;
}

}; // namespace fakeroblox
