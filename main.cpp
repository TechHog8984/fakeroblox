#include <cfloat>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <fstream>
#include <thread>

#include "classes/color3.hpp"
#include "environment.hpp"
#include "libraries/drawentrylib.hpp"
#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

#include "cli.hpp"
#include "console.hpp"
#include "tests.hpp"

#include "classes/roblox/instance.hpp"
#include "classes/vector2.hpp"
#include "libraries/tasklib.hpp"
#include "libraries/instructionlib.hpp"

#include "lua.h"
#include "lualib.h"

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

int imgui_inputTextCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        std::string* str = (std::string*) data->UserData;
        str->resize(data->BufTextLen);
        data->Buf = (char*) str->c_str();
    }
    return 0;
}

size_t next_script_editor_tab_index = 0;
struct ScriptEditorTab {
    bool exists;
    bool newly_created;
    std::string name;
    std::string code;
};

std::vector<ScriptEditorTab> script_editor_tab_list;
void pushNewScriptEditorTab() {
    next_script_editor_tab_index++;
    std::string name = "script";
    name.append(std::to_string(next_script_editor_tab_index));
    script_editor_tab_list.push_back({true, true, name, "print'fakeroblox on top'"});
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

    std::ifstream api_dump_file("Full-API-Dump.json");
    if (!api_dump_file) {
        fprintf(stderr, "ERROR: failed to open Full-API-Dump.json\n");
        return 1;
    }

    std::string api_dump;
    std::string buffer;
    while (std::getline(api_dump_file, buffer))
        api_dump.append(buffer) += '\n';
    assert(api_dump.size() > 0);
    api_dump.erase(api_dump.size() - 1);

    api_dump_file.close();

    curl_global_init(CURL_GLOBAL_DEFAULT);

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    open_fakeroblox_environment(L);

    open_tasklib(L);
    open_vector2lib(L);
    open_color3lib(L);
    open_drawentrylib(L);

    rbxInstanceSetup(L, api_dump);

    open_instructionlib(L); // needs to happen after rbxInstance setup

    lua_newtable(L);
    lua_setglobal(L, "shared");

    luaL_sandbox(L);

    lua_getglobal(L, "shared");
    lua_setreadonly(L, -1, false);

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "fakeroblox");
    SetExitKey(KEY_NULL);
    rlImGuiSetup(true);

    bool menu_editor_open = true;
    bool menu_console_open = true;
    bool menu_tests_open = false;
    bool menu_thread_list_open = false;

    bool all_tests_succeeded = false;
    bool has_tested = false;
    bool is_running_tests = false;
    bool should_run_tests = false;

    pushNewScriptEditorTab();

    while (!WindowShouldClose()) {
        TaskScheduler::run(L);

        BeginDrawing();
        ClearBackground(DARKGRAY);

        // lua drawings
        // FIXME: entry could get freed by luau garbage collector, so these should be shared pointers maybe
        for (auto& entry : DrawEntry::draw_list) {
            if (!entry->visible)
                continue;

            auto& color = entry->color;
            switch (entry->type) {
                case DrawEntry::Line: {
                    DrawEntryLine* entry_line = static_cast<DrawEntryLine*>(entry);
                    DrawLineEx(entry_line->from, entry_line->to, entry_line->thickness, color);
                    break;
                }
                case DrawEntry::Text: {
                    DrawEntryText* entry_text = static_cast<DrawEntryText*>(entry);
                    // FIXME: text outline
                    // if (entry_text->outlined)
                    //     DrawTextEx(entry_text->font, entry_text->text.c_str(), entry_text->outline_position, entry_text->outline_text_size, 0, entry_text->outline_color);
                    DrawTextEx(entry_text->font, entry_text->text.c_str(), entry_text->position, entry_text->text_size, 0, color);
                    break;
                }
                case DrawEntry::Square: {
                    DrawEntrySquare* entry_square = static_cast<DrawEntrySquare*>(entry);
                    DrawRectangleLinesEx(entry_square->rect, entry_square->thickness, color);
                    if (entry_square->filled)
                        DrawRectangleRec(entry_square->rect, color);
                    break;
                }
                case DrawEntry::Triangle: {
                    DrawEntryTriangle* entry_triangle = static_cast<DrawEntryTriangle*>(entry);
                    // DrawTriangle* doesn't support thickness, so we use DrawLineEx
                    DrawLineEx(entry_triangle->pointa, entry_triangle->pointb, entry_triangle->thickness, color);
                    DrawLineEx(entry_triangle->pointb, entry_triangle->pointc, entry_triangle->thickness, color);
                    DrawLineEx(entry_triangle->pointc, entry_triangle->pointa, entry_triangle->thickness, color);
                    if (entry_triangle->filled)
                        DrawTriangle(entry_triangle->pointc, entry_triangle->pointb, entry_triangle->pointa, color);
                    break;
                }
                case DrawEntry::Quad: {
                    DrawEntryQuad* entry_quad = static_cast<DrawEntryQuad*>(entry);
                    // DrawTriangle* doesn't support thickness, so we use DrawLineEx
                    DrawLineEx(entry_quad->pointa, entry_quad->pointb, entry_quad->thickness, color);
                    DrawLineEx(entry_quad->pointb, entry_quad->pointc, entry_quad->thickness, color);
                    DrawLineEx(entry_quad->pointc, entry_quad->pointd, entry_quad->thickness, color);
                    DrawLineEx(entry_quad->pointd, entry_quad->pointa, entry_quad->thickness, color);
                    if (entry_quad->filled) {
                        DrawTriangle(entry_quad->pointc, entry_quad->pointb, entry_quad->pointa, color);
                        DrawTriangle(entry_quad->pointd, entry_quad->pointc, entry_quad->pointa, color);
                    }
                    break;
                }
                default:
                    Console::ScriptConsole.debugf("INTERNAL TODO: all DrawEntry types (%s)", entry->class_name);
                    break;
            }
        }

        // ui
        rlImGuiBegin();
        const float imgui_frame_height = ImGui::GetFrameHeightWithSpacing();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Script Editor", nullptr, &menu_editor_open);
                ImGui::MenuItem("Script Console", nullptr, &menu_console_open);
                ImGui::MenuItem("Tests", nullptr, &menu_tests_open);
                ImGui::MenuItem("Thread List", nullptr, &menu_thread_list_open);

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // TODO: open_file, execute_file, save_file buttons
        if (menu_editor_open) {
            if (ImGui::Begin("Script Editor", &menu_editor_open)) {
                if (ImGui::Button("+##scripteditornewtab"))
                    pushNewScriptEditorTab();

                ImGui::SameLine();

                if (ImGui::BeginTabBar("scripteditortabs", ImGuiTabBarFlags_Reorderable )) {
                    for (size_t i = 0; i < script_editor_tab_list.size(); i++) {
                        auto& tab = script_editor_tab_list[i];

                        ImGuiTabItemFlags flags = 0;
                        if (tab.newly_created) {
                            flags |= ImGuiTabItemFlags_SetSelected;
                            tab.newly_created = false;
                        }
                        if (ImGui::BeginTabItem(tab.name.c_str(), &tab.exists, flags)) {
                            ImGui::InputTextMultiline(
                                "##scriptbox",
                                &tab.code[0],
                                tab.code.capacity() + 1,
                                ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - imgui_frame_height),
                                ImGuiInputTextFlags_CallbackResize,
                                imgui_inputTextCallback,
                                (void*) &tab.code
                            );
                            if (ImGui::Button("Execute")) {
                                auto error = tryRunCode(L, tab.name.c_str(), tab.code.c_str(), [] (std::string error){
                                    Console::ScriptConsole.error(error);
                                });
                                if (error.has_value())
                                    Console::ScriptConsole.error(error.value());
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

            }
            ImGui::End();
        }
        if (menu_console_open) {
            if (ImGui::Begin("Script Console", &menu_console_open)) {
                if (ImGui::Button("Clear")) {
                    Console::ScriptConsole.clear();
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

                    ImGui::Checkbox("INFO", &Console::ScriptConsole.show_info);
                    ImGui::SameLine();
                    ImGui::Checkbox("WARNING", &Console::ScriptConsole.show_warning);
                    ImGui::SameLine();
                    ImGui::Checkbox("ERROR", &Console::ScriptConsole.show_error);
                    ImGui::SameLine();
                    ImGui::Checkbox("DEBUG", &Console::ScriptConsole.show_debug);

                    ImGui::Separator();

                    ImGui::BeginChild("ScrollableRegion", ImGui::GetContentRegionAvail(), 0,  ImGuiWindowFlags_HorizontalScrollbar);
                    if (!go_to_bottom) go_to_bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

                    Console::ScriptConsole.renderMessages();

                    if (go_to_top)
                        ImGui::SetScrollY(0.f);
                    else if (go_to_bottom)
                        ImGui::SetScrollHereY(1.f);

                    ImGui::EndChild();
                }
            }
            ImGui::End();
        }
        if (menu_tests_open) {
            if (ImGui::Begin("Tests", &menu_tests_open)) {
                if (is_running_tests)
                    ImGui::Text("Running tests...");
                else {
                    if (ImGui::Button("Run all tests")) {
                        has_tested = true;
                        is_running_tests = true;
                        should_run_tests = true;
                        Console::TestsConsole.clear();
                    } else if (has_tested) {
                        if (all_tests_succeeded)
                            ImGui::TextColored({0.4, 1, 0.4, 1}, "All tests succeeded!");
                        else
                            ImGui::TextColored({1, 0.4, 0.4, 1}, "Some tests failed!");
                    }
                }

                ImGui::BeginChild("ScrollableRegion", ImGui::GetContentRegionAvail(), 0,  ImGuiWindowFlags_HorizontalScrollbar);
                Console::TestsConsole.renderMessages();
                ImGui::EndChild();

            }
            ImGui::End();
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
            }
            ImGui::End();
        }

        rlImGuiEnd();

        EndDrawing();

        if (should_run_tests) {
            should_run_tests = false;
            std::thread([&L, &is_running_tests, &all_tests_succeeded]{
                runAllTests(L, is_running_tests, all_tests_succeeded);
            }).detach();
        }
    }

    rlImGuiShutdown();
    CloseWindow();

    TaskScheduler::cleanup(L);
    rbxInstanceCleanup(L);
    lua_close(L);

    for (auto& entry : DrawEntry::draw_list)
        entry->free();

    curl_global_cleanup();

    return 0;
}
