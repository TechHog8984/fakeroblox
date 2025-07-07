#include <cfloat>
#include <cstdio>
#include <cstring>

#include "classes/roblox/instance.hpp"
#include "libraries/instructionlib.hpp"
#include "libraries/signallib.hpp"
#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

#include "lua.h"
#include "lualib.h"

#include "libraries/tasklib.hpp"
#include "cli.hpp"
#include "script_console.hpp"

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

int fakeroblox_log(lua_State* L, ScriptConsole::Message::Type type) {
    std::string message;
    int n = lua_gettop(L);
    for (int i = 0; i < n; i++) {
        size_t l;
        const char* s = luaL_tolstring(L, i + 1, &l);
        if (i > 0)
            message += '\t';
        message.append(s, l);
        lua_pop(L, 1);
    }
    ScriptConsole::log(message, type);
    return 0;
}
int fakeroblox_print(lua_State* L) {
    return fakeroblox_log(L, ScriptConsole::Message::INFO);
}
int fakeroblox_warn(lua_State* L) {
    return fakeroblox_log(L, ScriptConsole::Message::WARNING);
}
int fakeroblox_getreg(lua_State* L) {
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
}

size_t next_script_editor_tab_index = 0;
struct ScriptEditorTab {
    bool exists;
    std::string name;
    std::string code;
};

std::vector<ScriptEditorTab> script_editor_tab_list;
void pushNewScriptEditorTab() {
    next_script_editor_tab_index++;
    std::string name = "script";
    name.append(std::to_string(next_script_editor_tab_index));
    script_editor_tab_list.push_back({true, name, "print'fakeroblox on top'"});
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
    lua_pushcfunction(L, fakeroblox_warn, "warn");
    lua_setglobal(L, "warn");
    lua_pushcfunction(L, fakeroblox_getreg, "getreg");
    lua_setglobal(L, "getreg");

    open_tasklib(L);
    open_instructionlib(L);
    setup_signallib(L);

    rbxInstanceSetup(L);

    luaL_sandbox(L);

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "fakeroblox");
    SetExitKey(KEY_NULL);
    rlImGuiSetup(true);

    bool menu_editor_open = true;
    bool menu_console_open = true;
    bool menu_thread_list_open = false;

    pushNewScriptEditorTab();

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
                ImGui::MenuItem("Thread List", nullptr, &menu_thread_list_open);

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (menu_editor_open) {
            if (ImGui::Begin("Script Editor", &menu_editor_open)) {
                if (ImGui::Button("+##scripteditornewtab"))
                    pushNewScriptEditorTab();

                ImGui::SameLine();

                if (ImGui::BeginTabBar("scripteditortabs", ImGuiTabBarFlags_Reorderable )) {
                    for (size_t i = 0; i < script_editor_tab_list.size(); i++) {
                        auto& tab = script_editor_tab_list[i];

                        if (ImGui::BeginTabItem(tab.name.c_str(), &tab.exists)) {
                            ImGui::InputTextMultiline(
                                "##scriptbox",
                                &tab.code[0],
                                tab.code.capacity() + 1,
                                ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - imgui_frame_height),
                                ImGuiInputTextFlags_CallbackResize,
                                inputTextCallback,
                                (void*) &tab.code
                            );
                            if (ImGui::Button("Execute")) {
                                auto error = tryRunCode(L, tab.name.c_str(), tab.code.c_str(), ScriptConsole::error);
                                if (error.has_value())
                                    ScriptConsole::error(error.value());
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Clear"))
                                tab.code.clear();
                            ImGui::EndTabItem();
                        }
                    }
                    // remove tabs that were closed
                    for (auto tab = script_editor_tab_list.begin(); tab != script_editor_tab_list.end();) {
                        if (!tab->exists)
                            tab = script_editor_tab_list.erase(tab);
                        else
                            ++tab;
                    }
                    if (script_editor_tab_list.empty())
                        pushNewScriptEditorTab();
                    ImGui::EndTabBar();
                }

                ImGui::End();
            }
        }
        if (menu_console_open) {
            if (ImGui::Begin("Script Console", &menu_console_open)) {
                if (ImGui::Button("Clear")) {
                    ScriptConsole::clear();
                } else {
                    static const char* goto_text = "go to";
                    static const char* top_button_text = "top";
                    static const char* bottom_button_text = "bottom";
                    float text_width = ImGui::CalcTextSize(goto_text).x + ImGui::GetStyle().FramePadding.x * 2.f;
                    float top_button_width = ImGui::CalcTextSize(top_button_text).x + ImGui::GetStyle().FramePadding.x * 2.f;
                    float bottom_button_width = ImGui::CalcTextSize(bottom_button_text).x + ImGui::GetStyle().FramePadding.x * 2.f;

                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - (text_width + ImGui::GetStyle().ItemSpacing.x + top_button_width + bottom_button_width));
                    ImGui::Text("%s", goto_text);
                    ImGui::SameLine();
                    bool go_to_top = ImGui::Button(top_button_text);
                    ImGui::SameLine();
                    bool go_to_bottom = ImGui::Button(bottom_button_text);

                    ImGui::Checkbox("INFO", &ScriptConsole::show_info);
                    ImGui::SameLine();
                    ImGui::Checkbox("WARNING", &ScriptConsole::show_warning);
                    ImGui::SameLine();
                    ImGui::Checkbox("ERROR", &ScriptConsole::show_error);
                    ImGui::SameLine();
                    ImGui::Checkbox("DEBUG", &ScriptConsole::show_debug);

                    ImGui::Separator();

                    ImGui::BeginChild("ScrollableRegion", ImGui::GetContentRegionAvail(), 0,  ImGuiWindowFlags_HorizontalScrollbar);
                    if (!go_to_bottom) go_to_bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

                    for (size_t i = 0; i < ScriptConsole::message_count; i++) {
                        bool skip = false;
                        auto& message = ScriptConsole::messages[i];
                        ImVec4 color;
                        switch (message.type) {
                            case ScriptConsole::Message::INFO:
                                skip = !ScriptConsole::show_info;
                                color = ImVec4{1,1,1,1};
                                break;
                            case ScriptConsole::Message::WARNING:
                                skip = !ScriptConsole::show_warning;
                                color = ImVec4{1,1,0,1};
                                break;
                            case ScriptConsole::Message::ERROR:
                                skip = !ScriptConsole::show_error;
                                color = ImVec4{1,0,0,1};
                                break;
                            case ScriptConsole::Message::DEBUG:
                                skip = !ScriptConsole::show_debug;
                                color = ImVec4{1,0,1,1};
                                break;
                        }
                        if (skip) continue;
                        ImGui::TextColored(color, "%.*s", static_cast<int>(message.content.size()), message.content.c_str());
                    }

                    if (go_to_top)
                        ImGui::SetScrollY(0.f);
                    else if (go_to_bottom)
                        ImGui::SetScrollHereY(1.f);

                    ImGui::EndChild();
                }
                ImGui::End();
            }
        }
        if (menu_thread_list_open) {
            if (ImGui::Begin("Thread List", &menu_thread_list_open)) {
                std::vector<Task*> tasks_to_kill;
                for (auto& pair : TaskScheduler::task_map) {
                    Task* task = pair.second;
                    std::string identifier = task->identifier;
                    if (ImGui::Button(identifier.c_str()))
                        task->view.open = true;
                    if (task->view.open) {
                        std::string win_id = std::string("Thread ");
                        win_id.append(identifier);
                        if (ImGui::Begin(win_id.c_str(), &task->view.open)) {
                            ImGui::Text("status: %s", taskStatusTostring(task->status));
                            if (ImGui::Button("Kill"))
                                tasks_to_kill.push_back(task);
                            ImGui::End();
                        }
                    }
                }
                for (size_t i = 0; i < tasks_to_kill.size(); i++)
                    TaskScheduler::killTask(L, tasks_to_kill[i]);
                ImGui::End();
            }
        }

        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    TaskScheduler::cleanup(L);
    rbxInstanceCleanup(L);
    lua_close(L);

    return 0;
}
