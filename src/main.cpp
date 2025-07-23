#include <cfloat>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <fstream>
#include <queue>
#include <shared_mutex>
#include <thread>

#include "classes/roblox/instance.hpp"
#include "common.hpp"
#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

#include "cli.hpp"
#include "environment.hpp"
#include "console.hpp"
#include "tests.hpp"

#include "ui/ui.hpp"
#include "ui/drawentrylist.hpp"
#include "ui/instanceexplorer.hpp"
#include "ui/functionexplorer.hpp"

#include "libraries/tasklib.hpp"
#include "libraries/instructionlib.hpp"
#include "libraries/drawentrylib.hpp"

#include "classes/color3.hpp"
#include "classes/vector2.hpp"
#include "classes/vector3.hpp"
#include "classes/udim.hpp"
#include "classes/udim2.hpp"
#include "classes/roblox/camera.hpp"
#include "classes/roblox/userinputservice.hpp"
#include "classes/roblox/baseplayergui.hpp"

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

    Console::ScriptConsole.debugf("main state: %p", L);

    open_fakeroblox_environment(L);

    open_tasklib(L);
    open_vector2lib(L);
    open_vector3lib(L);
    open_color3lib(L);
    open_udimlib(L);
    open_udim2lib(L);
    open_drawentrylib(L);

    rbxInstanceSetup(L, api_dump);

    lua_getglobal(L, "workspace");
    auto& workspace = lua_checkinstance(L, -1);
    lua_pop(L, 1);

    // the following need to happen after rbxInstance setup
    open_instructionlib(L);
    UI_FunctionExplorer_init(L);

    lua_newtable(L);
    lua_setglobal(L, "shared");

    luaL_sandbox(L);

    lua_getglobal(L, "shared");
    lua_setreadonly(L, -1, false);

    auto appL_pair = createThread(L, [] (std::string error) { Console::ScriptConsole.error(error); });
    lua_State* appL = appL_pair.first;
    lua_pop(L, 1);
    Console::ScriptConsole.debugf("app state: %p", appL);

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "fakeroblox");
    SetExitKey(KEY_NULL);
    rlImGuiSetup(true);

    bool show_fps = false;
    bool menu_editor_open = true;
    bool menu_console_open = true;
    bool menu_tests_open = false;
    bool menu_thread_list_open = false;
    bool menu_drawentry_list_open = false;
    bool menu_instance_explorer_open = false;
    bool menu_function_explorer_open = false;

    bool all_tests_succeeded = false;
    bool has_tested = false;
    bool is_running_tests = false;
    bool should_run_tests = false;

    pushNewScriptEditorTab();

    const double initial_game_time = lua_clock();
    while (!WindowShouldClose()) {
        UserInputService::process(appL);

        TaskScheduler::run();

        int screen_width = GetScreenWidth();
        int screen_height = GetScreenHeight();
        rbxCamera::screen_size.x = screen_width;
        rbxCamera::screen_size.y = screen_height;

        // camera
        rbxInstance_Camera_updateViewport(appL);

        BeginDrawing();
        ClearBackground(DARKGRAY);

        if (show_fps)
            DrawFPS(30, 30);

        // gui object
        rbxInstance_BasePlayerGui_process(appL);

        // lua drawings
        std::shared_lock draw_entry_list_lock(DrawEntry::draw_list_mutex);
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
                    if (entry_text->outlined) {
                        // top
                        DrawTextEx(entry_text->font, entry_text->text.c_str(), { .x = entry_text->position.x, .y = entry_text->position.y - 1 }, entry_text->text_size, 0, entry_text->outline_color);
                        // right
                        DrawTextEx(entry_text->font, entry_text->text.c_str(), { .x = entry_text->position.x + 1, .y = entry_text->position.y }, entry_text->text_size, 0, entry_text->outline_color);
                        // bottom
                        DrawTextEx(entry_text->font, entry_text->text.c_str(), { .x = entry_text->position.x - 1, .y = entry_text->position.y + 1 }, entry_text->text_size, 0, entry_text->outline_color);
                        // left
                        DrawTextEx(entry_text->font, entry_text->text.c_str(), { .x = entry_text->position.x - 1, .y = entry_text->position.y }, entry_text->text_size, 0, entry_text->outline_color);
                    }
                    DrawTextEx(entry_text->font, entry_text->text.c_str(), entry_text->position, entry_text->text_size, 0, color);
                    break;
                }
                case DrawEntry::Circle: {
                    DrawEntryCircle* entry_circle = static_cast<DrawEntryCircle*>(entry);
                    if (entry_circle->num_sides) {
                        DrawPolyLines(entry_circle->center, entry_circle->num_sides, entry_circle->radius, 0, color);
                        if (entry_circle->filled)
                            DrawPoly(entry_circle->center, entry_circle->num_sides, entry_circle->radius, 0, color);
                    } else {
                        DrawCircleLinesV(entry_circle->center, entry_circle->radius, color);
                        if (entry_circle->filled)
                            DrawCircleV(entry_circle->center, entry_circle->radius, color);
                    }
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
        draw_entry_list_lock.unlock();

        // ui
        rlImGuiBegin();
        const float imgui_frame_height = ImGui::GetFrameHeightWithSpacing();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Show FPS", nullptr, &show_fps);
                ImGui::MenuItem("Script Editor", nullptr, &menu_editor_open);
                ImGui::MenuItem("Script Console", nullptr, &menu_console_open);
                ImGui::MenuItem("Tests", nullptr, &menu_tests_open);
                ImGui::MenuItem("Thread List", nullptr, &menu_thread_list_open);
                ImGui::MenuItem("DrawEntry List", nullptr, &menu_drawentry_list_open);
                ImGui::MenuItem("Instance Explorer", nullptr, &menu_instance_explorer_open);
                ImGui::MenuItem("Function Explorer", nullptr, &menu_function_explorer_open);

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
                                auto error = tryRunCode(appL, tab.name.c_str(), tab.code.c_str(), [] (std::string error){
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

                    ImGui::Checkbox("##info", &Console::ScriptConsole.show_info);
                    ImGui::SameLine();
                    ImGui::TextColored(Console::ColorINFO, "INFO");
                    ImGui::SameLine();

                    ImGui::Checkbox("##warning", &Console::ScriptConsole.show_warning);
                    ImGui::SameLine();
                    ImGui::TextColored(Console::ColorWARNING, "WARNING");
                    ImGui::SameLine();

                    ImGui::Checkbox("##error", &Console::ScriptConsole.show_error);
                    ImGui::SameLine();
                    ImGui::TextColored(Console::ColorERROR, "ERROR");
                    ImGui::SameLine();

                    ImGui::Checkbox("##debug", &Console::ScriptConsole.show_debug);
                    ImGui::SameLine();
                    ImGui::TextColored(Console::ColorDEBUG, "DEBUG");

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
                std::queue<Task*> tasks_to_kill;

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
                                tasks_to_kill.push(task);
                            ImGui::End();
                        }
                    }
                }

                while (!tasks_to_kill.empty()) {
                    TaskScheduler::killTask(tasks_to_kill.front());
                    tasks_to_kill.pop();
                }
            }
            ImGui::End();
        }
        if (menu_drawentry_list_open) {
            if (ImGui::Begin("DrawEntry List", &menu_drawentry_list_open))
                UI_DrawEntryList_render(appL);
            ImGui::End();
        }
        if (menu_instance_explorer_open) {
            if (ImGui::Begin("Instance Explorer", &menu_instance_explorer_open))
                UI_InstanceExplorer_render(appL);
            ImGui::End();
        }
        if (menu_function_explorer_open) {
            if (ImGui::Begin("Function Explorer", &menu_function_explorer_open))
                UI_FunctionExplorer_render(appL);
            ImGui::End();
        }

        rlImGuiEnd();

        EndDrawing();

        if (should_run_tests) {
            should_run_tests = false;
            std::thread([&appL, &is_running_tests, &all_tests_succeeded]{
                runAllTests(appL, is_running_tests, all_tests_succeeded);
            }).detach();
        }

        workspace->setValue<double>(appL, "DistributedGameTime", lua_clock() - initial_game_time);
    }

    rlImGuiShutdown();
    CloseWindow();

    TaskScheduler::killTask(appL_pair.second);

    for (auto& entry : DrawEntry::draw_list)
        entry->free();

    TaskScheduler::cleanup();
    rbxInstanceCleanup(appL);

    lua_close(L);

    curl_global_cleanup();

    return 0;
}
