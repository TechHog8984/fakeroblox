#include <cfloat>
#include <cstdio>
#include <cstring>
#include <shared_mutex>

#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

#include "lua.h"
#include "lualib.h"

#include "libraries/tasklib.hpp"
#include "cli.hpp"

using namespace fakeroblox;

int handleRecordOption(const char* option, const char*& arg, bool can_be_empty = false) {
    size_t option_length = strlen(option);

    if (strncmp(arg, option, option_length) != 0)
        return 1;

    if (strlen(arg) == option_length || arg[option_length] != '=') {
        fprintf(stderr, "ERROR: %s expects an equals sign\n", option);
        return 1;
    } else if (!can_be_empty && strlen(arg) < option_length + 2) {
        fprintf(stderr, "ERROR: %s expects a value after the equals sign\n", option);
        return 1;
    }

    arg += option_length + 1;
    return 0;
}

int inputTextCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        std::string* str = (std::string*) data->UserData;
        str->resize(data->BufTextLen);
        data->Buf = (char*) str->c_str();
    }
    return 0;
}

class ScriptConsole {
public:
    class Message {
    public:
        enum {
            INFO,
            WARNING,
            ERROR
        } type;
        std::string content;
    };
    static std::shared_mutex mutex;
    static std::vector<Message> messages;
    static size_t message_count;

    static void clear() {
        std::shared_lock lock(mutex);
        message_count = 0;
        messages.clear();
    }

    static void info(std::string content) {
        std::shared_lock lock(mutex);
        message_count++;
        messages.push_back({ .type = Message::INFO, .content = content });
    }
    static void warning(std::string content) {
        std::shared_lock lock(mutex);
        message_count++;
        messages.push_back({ .type = Message::WARNING, .content = content });
    }
    static void error(std::string content) {
        std::shared_lock lock(mutex);
        message_count++;
        messages.push_back({ .type = Message::ERROR, .content = content });
    }
};

std::shared_mutex ScriptConsole::mutex;
std::vector<ScriptConsole::Message> ScriptConsole::messages;
size_t ScriptConsole::message_count = 0;

int fakeroblox::fakeroblox_print(lua_State* thread) {
    std::string message;
    int n = lua_gettop(thread);
    for (int i = 0; i < n; i++) {
        size_t l;
        const char* s = luaL_tolstring(thread, i + 1, &l);
        if (i > 0)
            message += '\t';
        message.append(s, l);
        lua_pop(thread, 1);
    }
    ScriptConsole::info(message);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 1) {
        displayHelp();
        return 1;
    }

    for (unsigned i = 1; i < (unsigned) argc; i++) {
        const char* arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            displayHelp();
            return 0;
        } else {
            fprintf(stderr, "ERROR: unrecognized option '%s'\n", arg);
            return 1;
        }
    }

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, fakeroblox_print, "print");
    lua_setglobal(L, "print");

    open_tasklib(L);

    luaL_sandbox(L);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "fakeroblox");
    SetExitKey(KEY_NULL);
    rlImGuiSetup(true);

    bool menu_editor_open = true;
    bool menu_console_open = true;

    while (!WindowShouldClose()) {
        TaskScheduler::run(L);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        rlImGuiBegin();
        const float imgui_frame_height = ImGui::GetFrameHeightWithSpacing();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Script Editor", nullptr, &menu_editor_open);
                ImGui::MenuItem("Script Console", nullptr, &menu_console_open);

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (menu_editor_open) {
            if (ImGui::Begin("Script Editor", &menu_editor_open)) {
                static std::string script = "print(\"hello, roblox\")";

                ImGui::InputTextMultiline(
                    "##scriptbox",
                    &script[0],
                    script.capacity() + 1,
                    ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - imgui_frame_height),
                    ImGuiInputTextFlags_CallbackResize,
                    inputTextCallback,
                    (void*) &script
                );
                if (ImGui::Button("Execute")) {
                    auto error = tryRunCode(L, script.c_str(), ScriptConsole::error);
                    if (error.has_value())
                        ScriptConsole::error(error.value());
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear"))
                    script.clear();
                
                ImGui::End();
            }
        }
        if (menu_console_open) {
            if (ImGui::Begin("Script Console", &menu_console_open)) {
                if (ImGui::Button("Clear")) {
                    ScriptConsole::clear();
                } else {
                    ImGui::BeginChild("ScrollableRegion", ImGui::GetContentRegionAvail());

                    for (size_t i = 0; i < ScriptConsole::message_count; i++) {
                        auto& message = ScriptConsole::messages[i];
                        switch (message.type) {
                            case ScriptConsole::Message::INFO:
                                ImGui::TextColored(ImVec4{1,1,1,1}, "%s", message.content.c_str());
                                break;
                            case ScriptConsole::Message::WARNING:
                                ImGui::TextColored(ImVec4{1,1,0,1}, "%s", message.content.c_str());
                                break;
                            case ScriptConsole::Message::ERROR:
                                ImGui::TextColored(ImVec4{1,0,0,1}, "%s", message.content.c_str());
                                break;
                        }
                    }

                    ImGui::EndChild();
                }
                ImGui::End();
            }
        }
        
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    TaskScheduler::cleanup(L);
    lua_close(L);

    return 0;
}
