#include "classes/roblox/userinputservice.hpp"
#include "classes/roblox/datatypes/enum.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/instance.hpp"
#include "classes/roblox/serviceprovider.hpp"
#include "libraries/tasklib.hpp"

#include "common.hpp"

#include "raylib.h"

#include <map>
#include <queue>
#include <stdexcept>

namespace fakeroblox {

void userInputServiceFire(lua_State* L, const char* event, std::shared_ptr<rbxInstance> input_object, bool game_processed) {
    std::shared_ptr<rbxInstance> uis = ServiceProvider::service_map.at("UserInputService");

    pushFunctionFromLookup(L, fireRbxScriptSignal);
    uis->pushEvent(L, event);

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

const char* getKeyCodeName(lua_State* L, bool shift, unsigned int key) {
    const char* value = nullptr;
    try {
        if (shift && raylib_key_to_keycode_map.find(keyShifted(key)) != raylib_key_to_keycode_map.end())
            value = raylib_key_to_keycode_map.at(keyShifted(key));
        else
            value = raylib_key_to_keycode_map.at(key);
    } catch(std::out_of_range& e) {
        TaskScheduler::getTaskFromThread(L)->console->errorf("[UserInputService::process] unhandled key code %ud", key);
    }
    return value;
}

const char* MOUSE_BUTTON_MAP[] = {
    "MouseButton1",
    "MouseButton2",
    "MouseButton3"
};

struct InputEventMouseClick {
    unsigned int mouse;
    bool down;
};
struct InputEventKeyboard {
    const char* keycode;
    bool down;
};
struct InputEvent {
    enum {
        MouseClick,
        MouseMovement,
        MouseWheel,
        Keyboard
    } type;

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

int global_mouse_wheel = 0;
void UserInputService::process(lua_State *L) {
    const Vector2 mouse_delta = GetMouseDelta();
    const Vector2 mouse_position = GetMousePosition();
    const Vector2 mouse_wheel_vector = GetMouseWheelMoveV();

    const int mouse_wheel_y = mouse_wheel_vector.y;
    const int mouse_wheel = mouse_wheel_y ? (mouse_wheel_y > 0 ? 1 : -1) : 0;

    for (unsigned int mouse = 0; mouse < 3; mouse++) {
        const bool pressed = IsMouseButtonPressed(mouse);
        const bool released = IsMouseButtonReleased(mouse);

        if (pressed || released) {
            InputEvent event = {
                .type = InputEvent::MouseClick,
                .mouse_click = {
                    .mouse = mouse,
                    .down = pressed
                }
            };
            pushInputEvent(event);
        }
    }
    if (mouse_delta.x || mouse_delta.y) {
        InputEvent event = {
            .type = InputEvent::MouseMovement
        };
        pushInputEvent(event);
    }
    if (mouse_wheel && mouse_wheel != global_mouse_wheel) {
        global_mouse_wheel = mouse_wheel;
        InputEvent event = {
            .type = InputEvent::MouseWheel
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
                .keyboard = {
                    .keycode = keycode,
                    .down = pressed
                }
            };
            pushInputEvent(event);
        }
    }

    while (!input_event_queue.empty()) {
        auto& event = input_event_queue.front();
        auto input_object = newInstance(L, "InputObject");

        auto& key_code = input_object->getValue<EnumItemWrapper>("KeyCode");
        auto& user_input_type = input_object->getValue<EnumItemWrapper>("UserInputType");
        auto& position = input_object->getValue<Vector3>("Position");
        auto& delta = input_object->getValue<Vector3>("Delta");

        key_code.name.assign("Unknown");
        user_input_type.name.assign("None");

        const char* signal = nullptr;
        switch (event.type) {
            case InputEvent::MouseClick:
                global_mouse_wheel = 0;
                signal = event.mouse_click.down ? "InputBegan" : "InputEnded";
                user_input_type.name.assign(MOUSE_BUTTON_MAP[event.mouse_click.mouse]);
                break;
            case InputEvent::MouseMovement:
                signal = "InputChanged";
                user_input_type.name.assign("MouseMovement");
                break;
            case InputEvent::MouseWheel:
                signal = "InputChanged";
                user_input_type.name.assign("MouseWheel");
                break;
            case InputEvent::Keyboard:
                signal = event.keyboard.down ? "InputBegan" : "InputEnded";
                key_code.name.assign(event.keyboard.keycode);
                user_input_type.name.assign("Keyboard");
                break;
        }

        position.x = mouse_position.x;
        position.y = mouse_position.y;
        position.z = global_mouse_wheel;

        delta.x = mouse_delta.x;
        delta.y = mouse_delta.y;
        delta.z = 0;

        assert(signal != nullptr);
        userInputServiceFire(L, signal, input_object, false);
        input_event_queue.pop();
    }
}

#undef keyShifted

void rbxInstance_UserInputService_init(lua_State *L) {
    
}

}; // namespace fakeroblox
