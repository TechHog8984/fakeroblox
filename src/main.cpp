#include <cfloat>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <shared_mutex>
#include <stdexcept>

#include "common.hpp"
#include "curl/curl.h"
#include "libraries/cryptlib.hpp"
#include "libraries/drawingimmediate.hpp"
#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

#include "ImGuiFileDialog.h"

#include "taskscheduler.hpp"
#include "cli.hpp"
#include "environment.hpp"
#include "console.hpp"
#include "tests.hpp"
#include "fontloader.hpp"
#include "imageloader.hpp"

#include "ui/ui.hpp"
#include "ui/drawentrylist.hpp"
#include "ui/instanceexplorer.hpp"
#include "ui/functionexplorer.hpp"
#include "ui/imageexplorer.hpp"
#include "ui/tableexplorer.hpp"

#include "libraries/cachelib.hpp"
#include "libraries/drawentrylib.hpp"
#include "libraries/instructionlib.hpp"

#include "classes/color3.hpp"
#include "classes/vector2.hpp"
#include "classes/vector3.hpp"
#include "classes/udim.hpp"
#include "classes/udim2.hpp"
#include "classes/roblox/baseplayergui.hpp"
#include "classes/roblox/camera.hpp"
#include "classes/roblox/datamodel.hpp"
#include "classes/roblox/runservice.hpp"
#include "classes/roblox/userinputservice.hpp"
#include "classes/roblox/workspace.hpp"
#include "classes/roblox/custom/imguiservice.hpp"

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

std::string readFileToString(const char* file_path) {
    std::ifstream file(file_path);
    if (!file)
        throw std::runtime_error("failed to open file");

    std::string result;
    std::string buffer;
    while (std::getline(file, buffer))
        result.append(buffer) += '\n';
    if (result.size() > 0)
        result.erase(result.size() - 1);

    file.close();

    return result;
}

void writeStringToFile(const char* file_path, std::string_view contents) {
    std::ofstream file(file_path);

    file << contents;
}

size_t next_script_editor_tab_index = 0;
struct ScriptEditorTab {
    bool exists;
    bool newly_created;
    std::string name;
    std::string code;
};

std::vector<ScriptEditorTab> script_editor_tab_list;
void pushNewScriptEditorTab(std::string contents) {
    next_script_editor_tab_index++;
    std::string name = "script";
    name.append(std::to_string(next_script_editor_tab_index));
    script_editor_tab_list.push_back({true, true, name, contents});
}
void pushNewScriptEditorTab() {
    pushNewScriptEditorTab("print'fakeroblox on top'");
}

std::string script_editor_save_path;
std::string script_editor_save_contents;
std::string_view script_editor_current_contents;

void tryRunCode(lua_State* L, const char* name, const char* code, size_t code_length) {
    try {
        TaskScheduler::startCodeOnNewThread(L, name, code, code_length, [] (std::string error) {
            Console::ScriptConsole.error(error);
        });
    } catch(std::exception& e) {
        Console::ScriptConsole.error(e.what());
    }
}

int main(int argc, char** argv) {
    if (argc < 1) {
        displayHelp();
        return 1;
    }

    for (unsigned i = 1; i < (unsigned) argc; i++) {
        const char* arg = argv[i];
        if (strequal(arg, "-h") || strequal(arg, "--help")) {
            displayHelp();
            return 0;
        } else if (strequal(arg, "--nosandbox"))
            TaskScheduler::sandboxing = false;
        else {
            fprintf(stderr, "ERROR: unrecognized option '%s'\n", arg);
            return 1;
        }
    }

    std::string api_dump;
    try {
        api_dump.assign(readFileToString("assets/Full-API-Dump.json"));
    } catch (std::exception& e) {
        fprintf(stderr, "ERROR: failed to read assets/Full-API-Dump.json: %s", e.what());
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    TaskScheduler::setup(L);

    Console::ScriptConsole.debugf("main state: %p", L);

    open_fakeroblox_environment(L);

    open_tasklib(L);
    open_vector2lib(L);
    open_vector3lib(L);
    open_color3lib(L);
    open_udimlib(L);
    open_udim2lib(L);

    initializeSharedPtrDestructorList();

    lua_setuserdatadtor(L, LUA_TAG_SHAREDPTR_OBJECT, [](lua_State* L, void* ud) {
        SharedPtrObject* object = static_cast<SharedPtrObject*>(ud);

        sharedptr_destructor_list[object->class_index](object->object);
        free(object->object);
    });

    rbxInstanceSetup(L, api_dump);

    // the following need to happen after rbxInstance setup
    open_instructionlib(L);
    open_cachelib(L);
    open_cryptlib(L);
    UI_FunctionExplorer_init(L, DataModel::instance);
    ImGuiService_init(L, DataModel::instance);

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "fakeroblox");
    SetExitKey(KEY_NULL);
    SetTargetFPS(TaskScheduler::target_fps);

    // load fonts before opening drawentry lib and after InitWindow. This means that we have to push some lua state stuff after window creation
    FontLoader::load();
    open_drawentrylib(L);
    open_drawingimmediate(L);

    lua_newtable(L);
    lua_setglobal(L, "shared");

    if (TaskScheduler::sandboxing)
        luaL_sandbox(L);

    lua_getglobal(L, "shared");
    lua_setreadonly(L, -1, false);

    lua_State* appL = TaskScheduler::newThread(L, [] (std::string error) { Console::ScriptConsole.error(error); });
    lua_pop(L, 1);
    Console::ScriptConsole.debugf("app state: %p", appL);

    lua_State* userL = TaskScheduler::newThread(L, [] (std::string error) { Console::ScriptConsole.error(error); });
    lua_pop(L, 1);
    Console::ScriptConsole.debugf("user state: %p", userL);

    lua_State* testL = TaskScheduler::newThread(L, [] (std::string error) { Console::TestsConsole.error(error); });
    lua_pop(L, 1);
    Console::ScriptConsole.debugf("test state: %p", testL);

    lua_State* fontL = TaskScheduler::newThread(L, [] (std::string error) { Console::ScriptConsole.error(error); });
    lua_pop(L, 1);
    FontLoader::L = fontL;
    Console::ScriptConsole.debugf("font state: %p", fontL);

    {
        char buf[100];
        snprintf(buf, 100, "App State (%p)", appL);
        getTask(appL)->identifier.assign(buf);
        snprintf(buf, 100, "User State (%p)", userL);
        getTask(userL)->identifier.assign(buf);
        snprintf(buf, 100, "Test State (%p)", testL);
        getTask(testL)->identifier.assign(buf);
        snprintf(buf, 100, "Font State (%p)", fontL);
        getTask(fontL)->identifier.assign(buf);
    }

    static lua_State* dont_kill_thread_list[4] = { appL, userL, testL, fontL };
    static lua_State** dont_kill_thread_list_end = dont_kill_thread_list + IM_ARRAYSIZE(dont_kill_thread_list);

    // window items
    bool show_fps = false;
    bool menu_editor_open = true;
    bool menu_console_open = true;
    bool menu_tests_open = false;
    bool menu_thread_list_open = false;
    bool menu_drawentry_list_open = false;
    bool menu_instance_explorer_open = false;
    bool menu_function_explorer_open = false;

    // debugging items
    bool enable_user_input_service = true;
    bool enable_run_service = true;
    bool menu_image_explorer_open = false;
    bool menu_table_explorer_open = false;

    bool all_tests_succeeded = false;
    bool has_tested = false;
    bool is_running_tests = false;
    bool should_run_tests = false;

    setupTests(&is_running_tests, &all_tests_succeeded);

    pushNewScriptEditorTab();

    rlImGuiSetup(true);
    const double initial_game_time = lua_clock();
    while (!WindowShouldClose() && !DataModel::shutdown) {
        if (enable_user_input_service)
            UserInputService::process(appL);
        if (enable_run_service)
            RunService::process(appL);

        TaskScheduler::run();

        int screen_width = GetScreenWidth();
        int screen_height = GetScreenHeight();
        rbxCamera::screen_size.x = screen_width;
        rbxCamera::screen_size.y = screen_height;

        // camera
        rbxInstance_Camera_updateViewport(appL);

        BeginDrawing();
        ClearBackground(DARKGRAY);

        // gui object
        rbxInstance_BasePlayerGui_process(appL);

        // lua drawings
        DrawEntry::render();
        render_drawingimmediate(appL);

        // ui
        rlImGuiBegin();
        const float imgui_frame_height = ImGui::GetFrameHeightWithSpacing();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Options")) {
                ImGui::BeginDisabled();
                ImGui::Text("sandboxing - %s", TaskScheduler::sandboxing ? "enabled" : "disabled");
                ImGui::EndDisabled();

                ImGui::MenuItem("print routes to stdout", nullptr, &print_stdout);
                if (ImGui::MenuItem("enable stephook", nullptr, &enable_stephook))
                    onEnableStephookChange(appL);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Show FPS", nullptr, &show_fps);
                ImGui::Separator();
                ImGui::MenuItem("Script Editor", nullptr, &menu_editor_open);
                ImGui::MenuItem("Script Console", nullptr, &menu_console_open);
                ImGui::Separator();
                ImGui::MenuItem("Tests", nullptr, &menu_tests_open);
                ImGui::Separator();
                ImGui::MenuItem("Thread List", nullptr, &menu_thread_list_open);
                ImGui::MenuItem("DrawEntry List", nullptr, &menu_drawentry_list_open);
                ImGui::Separator();
                ImGui::MenuItem("Instance Explorer", nullptr, &menu_instance_explorer_open);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debugging")) {
                ImGui::MenuItem("Enable UserInputService", nullptr, &enable_user_input_service);
                ImGui::MenuItem("Enable RunService", nullptr, &enable_run_service);
                ImGui::MenuItem("Function Explorer", nullptr, &menu_function_explorer_open);
                ImGui::MenuItem("Table Explorer", nullptr, &menu_table_explorer_open);
                ImGui::MenuItem("Image Explorer", nullptr, &menu_image_explorer_open);

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (menu_editor_open) {
            if (ImGui::Begin("Script Editor", &menu_editor_open)) {
                if (ImGui::Button("Open File(s)")) {
                    IGFD::FileDialogConfig config;
                    config.countSelectionMax = 0;
                    config.path = ".";
                    ImGuiFileDialog::Instance()->OpenDialog("scripteditoropen", "Open File(s)", ".luau,.lua,.*", config); 
                }
                ImGui::SameLine();
                if (ImGui::Button("Execute File(s)")) {
                    IGFD::FileDialogConfig config;
                    config.countSelectionMax = 0;
                    config.path = ".";
                    ImGuiFileDialog::Instance()->OpenDialog("scripteditorexecute", "Execute File(s)", ".luau,.lua,.*", config); 
                }
                ImGui::SameLine();
                if (ImGui::Button("Save To File")) {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    ImGuiFileDialog::Instance()->OpenDialog("scripteditorsave", "Save File", ".*", config); 
                }

                if (ImGuiFileDialog::Instance()->Display("scripteditoropen", ImGuiWindowFlags_NoCollapse, ImVec2{0, 250})) {
                    if (ImGuiFileDialog::Instance()->IsOk())
                        for (auto& pair : ImGuiFileDialog::Instance()->GetSelection())
                            pushNewScriptEditorTab(readFileToString(pair.second.c_str()));

                    ImGuiFileDialog::Instance()->Close();
                }
                if (ImGuiFileDialog::Instance()->Display("scripteditorexecute", ImGuiWindowFlags_NoCollapse, ImVec2{0, 250})) {
                    if (ImGuiFileDialog::Instance()->IsOk())
                        for (auto& pair : ImGuiFileDialog::Instance()->GetSelection()) {
                            std::string contents = readFileToString(pair.second.c_str());
                            tryRunCode(userL, pair.first.c_str(), contents.c_str(), contents.length());
                        }

                    ImGuiFileDialog::Instance()->Close();
                }
                if (ImGuiFileDialog::Instance()->Display("scripteditorsave", ImGuiWindowFlags_NoCollapse, ImVec2{0, 250})) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();

                        if (ImGuiFileDialog::Instance()->GetSelection().empty())
                            // TODO: error handling & feedback
                            writeStringToFile(file_path.c_str(), script_editor_current_contents);
                        else {
                            script_editor_save_path = file_path;
                            script_editor_save_contents = script_editor_current_contents;
                        }

                        Console::ScriptConsole.debugf("file path: %s", file_path.c_str());
                    }

                    ImGuiFileDialog::Instance()->Close();
                }

                if (!script_editor_save_path.empty()) {
                    if (ImGui::Begin("Save File Confirmation")) {
                        ImGui::Text("Are you sure you want to overwrite '%s'?", script_editor_save_path.c_str());
                        const bool yes = ImGui::Button("YES");
                        const bool no = ImGui::Button("NO");

                        // TODO: feedback
                        if (yes)
                            // TODO: error handling
                            writeStringToFile(script_editor_save_path.c_str(), script_editor_save_contents);

                        if (yes || no)
                            script_editor_save_path.clear();

                        ImGui::End();
                    }
                }

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
                            script_editor_current_contents = tab.code;

                            if (ImGui::Button("Execute"))
                                tryRunCode(userL, tab.name.c_str(), tab.code.c_str(), tab.code.length());

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
                lua_State* thread_to_kill = nullptr;

                std::shared_lock lock(TaskScheduler::thread_list_mutex);
                for (size_t i = 0; i < TaskScheduler::thread_list.size();i ++) {
                    lua_State* thread = TaskScheduler::thread_list[i];
                    Task* task = getTask(thread);
                    std::string identifier = task->identifier;

                    if (ImGui::Button(identifier.c_str()))
                        task->view.open = true;
                    if (task->view.open) {
                        std::string win_id = std::string("Thread ");
                        win_id.append(identifier);
                        if (ImGui::Begin(win_id.c_str(), &task->view.open)) {
                            static const char* status_item_list[] = { "Idle", "Running", "Yielding", "Waiting", "Deferring", "Delaying" };
                            ImGui::Combo("Status", reinterpret_cast<int*>(&task->status), status_item_list, IM_ARRAYSIZE(status_item_list));

                            static const char* capability_item_list[] = {
                                "None",
                                "PluginSecurity",
                                "INVALID_CAPABILITY",
                                "LocalUserSecurity",
                                "WritePlayerSecurity",
                                "RobloxScriptSecurity",
                                "RobloxSecurity",
                                "NotAccesibleSecurity"
                            };
                            ImGui::Combo("Capability", reinterpret_cast<int*>(&task->capability), capability_item_list, IM_ARRAYSIZE(capability_item_list));

                            const bool dont_kill = std::find(dont_kill_thread_list, dont_kill_thread_list_end, thread) != dont_kill_thread_list_end;
                            if (dont_kill)
                                ImGui::BeginDisabled();
                            if (ImGui::Button("Kill"))
                                thread_to_kill = thread;
                            if (dont_kill) {
                                ImGui::SetItemTooltip("protected thread");
                                ImGui::EndDisabled();
                            }

                            ImGui::End();
                        }
                    }
                }
                lock.unlock();

                if (thread_to_kill)
                    TaskScheduler::killThread(thread_to_kill);
            }
            ImGui::End();
        }
        if (menu_drawentry_list_open) {
            if (ImGui::Begin("DrawEntry List", &menu_drawentry_list_open))
                UI_DrawEntryList_render(appL);
            ImGui::End();
        }
        if (menu_instance_explorer_open) {
            if (ImGui::Begin("Instance Explorer", &menu_instance_explorer_open, ImGuiWindowFlags_MenuBar))
                UI_InstanceExplorer_render(appL);
            ImGui::End();
        }

        if (menu_function_explorer_open) {
            if (ImGui::Begin("Function Explorer", &menu_function_explorer_open, ImGuiWindowFlags_MenuBar))
                UI_FunctionExplorer_render(appL);
            ImGui::End();
        }
        if (menu_table_explorer_open) {
            if (ImGui::Begin("Table Explorer", &menu_table_explorer_open, ImGuiWindowFlags_MenuBar))
                UI_TableExplorer_render(appL);
            ImGui::End();
        }
        if (menu_image_explorer_open) {
            if (ImGui::Begin("Image Explorer", &menu_image_explorer_open))
                UI_ImageExplorer_render(appL);
            ImGui::End();
        }

        ImGuiService_render(appL);

        if (show_fps)
            DrawFPS(30, 30);

        rlImGuiEnd();

        EndDrawing();

        if (should_run_tests) {
            should_run_tests = false;
            startAllTests(testL);
        }

        setInstanceValue<double>(Workspace::instance, appL, "DistributedGameTime", lua_clock() - initial_game_time);
    }
    DataModel::onShutdown(appL);

    for (auto& entry : DrawEntry::draw_list)
        entry->free();

    rlImGuiShutdown();
    FontLoader::unload();
    UI_ImageExplorer_cleanup();
    ImageLoader::unload();
    CloseWindow();

    TaskScheduler::killThread(appL);
    TaskScheduler::killThread(userL);
    TaskScheduler::killThread(testL);
    TaskScheduler::killThread(fontL);

    TaskScheduler::cleanup();
    rbxInstanceCleanup(appL);

    lua_close(L);

    curl_global_cleanup();

    return 0;
}
