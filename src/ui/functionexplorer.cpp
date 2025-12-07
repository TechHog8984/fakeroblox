#include "ui/functionexplorer.hpp"
#include "ui/tableexplorer.hpp"
#include "ui/ui.hpp"

#include "classes/roblox/serviceprovider.hpp"

#include "taskscheduler.hpp"

#include "imgui.h"

#include "lapi.h"
#include "lgc.h"
#include "lstate.h"
#include "lstring.h"
#include "lualib.h"
#include "lmem.h"

#include <memory>

namespace fakeroblox {

#define SELECTED_FUNCTION_KEY "functionexplorerselectedfunction"

// options
static bool filter_show_c = true;
static bool filter_show_lua = true;

void UI_FunctionExplorer_setSelectedFunction(lua_State* L, Closure* cl) {
    // TODO: I think this can cause a double free! verify this
    lua_pushnil(L);
    setclvalue(L, const_cast<TValue*>(luaA_toobject(L, -1)), cl);
    lua_rawsetfield(L, LUA_REGISTRYINDEX, SELECTED_FUNCTION_KEY);
}

namespace UI_FunctionExplorer_methods {
    static int selectFunction(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1, "FunctionExplorer");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        luaL_argcheck(L, lua_gettop(L) == 2, 3, "too many arguments");

        lua_rawsetfield(L, LUA_REGISTRYINDEX, SELECTED_FUNCTION_KEY);

        return 0;
    }
}

void UI_FunctionExplorer_init(lua_State* L, std::shared_ptr<rbxInstance> datamodel) {
    auto FunctionExplorer = std::make_shared<rbxClass>();
    FunctionExplorer->name.assign("FunctionExplorer");
    FunctionExplorer->tags |= rbxClass::NotCreatable;
    FunctionExplorer->superclass = rbxClass::class_map["Instance"];

    FunctionExplorer->methods["SelectFunction"] = {
      .name = "SelectFunction",
      .func = UI_FunctionExplorer_methods::selectFunction
    };

    rbxClass::class_map["FunctionExplorer"] = FunctionExplorer;
    ServiceProvider::registerService("FunctionExplorer");

    ServiceProvider::createService(L, datamodel, "FunctionExplorer");
}

#define getdebugname(closure) (closure->isC ? closure->c.debugname : (closure->l.p->debugname ? closure->l.p->debugname->data : nullptr))

std::vector<Closure*> closure_list;

void UI_FunctionExplorer_render(lua_State *L) {
    Closure* selected_function = nullptr;

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Show C Functions", nullptr, &filter_show_c);
            ImGui::MenuItem("Show Lua Functions", nullptr, &filter_show_lua);

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    lua_getfield(L, LUA_REGISTRYINDEX, SELECTED_FUNCTION_KEY);
    if (lua_isfunction(L, -1))
        selected_function = clvalue(luaA_toobject(L, -1));

    ImGui::BeginChild("Function List", ImVec2{230.f, ImGui::GetContentRegionAvail().y}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    static char list_search_buffer[128] = "";
    ImGui::InputText("Search", list_search_buffer, IM_ARRAYSIZE(list_search_buffer));

    ImGui::BeginChild("ScrollableRegion", ImGui::GetContentRegionAvail(), 0, ImGuiWindowFlags_HorizontalScrollbar);

    TaskScheduler::performGCWork(L, [&L, &selected_function] {
        struct VisitUserdata_t {
            Closure* current_selected;
            Closure* new_selected = nullptr;
            char msg[100];
        } VisitUserdata;
        VisitUserdata.current_selected = selected_function;

        luaM_visitgco(L, &VisitUserdata, [](void* ud, lua_Page* page, GCObject* object) {
            auto visit_ud = static_cast<VisitUserdata_t*>(ud);
            if (object->gch.tt == LUA_TFUNCTION) {
                auto cl = gco2cl(object);

                if (cl->isC) {
                    if (!filter_show_c)
                        goto RET;
                } else if (!filter_show_lua)
                    goto RET;

                const bool is_selected = visit_ud->current_selected && cl == visit_ud->current_selected;

                const char* debugname = getdebugname(cl);
                std::string namestr = "";
                if (debugname)
                    namestr.assign(" (").append(debugname) += ')';

                static const char* fmt = "%p%s";
                snprintf(visit_ud->msg, 100, fmt, object, namestr.c_str());

                if (strlen(list_search_buffer) == 0 || std::string(visit_ud->msg).find(list_search_buffer) != std::string::npos)
                    if (ImGui::Selectable(visit_ud->msg, is_selected))
                        visit_ud->new_selected = cl;
            }
            RET:
            return false;
        });

        if (VisitUserdata.new_selected) {
            UI_FunctionExplorer_setSelectedFunction(L, VisitUserdata.new_selected);
            selected_function = VisitUserdata.new_selected;
        }

        ImGui::EndChild();

        ImGui::EndChild();

        Closure* function_to_chose = nullptr;

        if (selected_function) {
            ImGui::SameLine();

            Closure* closure = selected_function;
            const bool is_c = closure->isC;

            ImGui::BeginChild("Function Display", ImVec2{0, 0}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

            ImGui::Text("Function %p", closure);
            ImGui::Text("Context: %s", is_c ? "C" : "Lua");

            const char* debugname = getdebugname(closure);
            ImGui::Text("Debug name: %s", debugname);

            if (is_c) {
                ImGui::Text("Pointer: %p", closure->c.f);
                ImGui::Text("Upvalue count: %u", closure->nupvalues);
                ImGui::Text("Stack size: %u", closure->stacksize);
            } else {
                Proto* p = closure->l.p;

                ImGui::Text("Upvalue count: %u", p->nups);
                ImGui::Text("Max stack size: %u", p->maxstacksize);
                ImGui::Text("Parameter count: %u", p->numparams);
                ImGui::Text("Vararg?: %s", p->is_vararg ? "YES" : "NO");

                ImGui::SeparatorText("Constants");

                size_t msg_size = 30 * sizeof(char);
                char* msg = static_cast<char*>(malloc(msg_size));
                TValue* k = p->k;
                for (int i = 0; i < p->sizek; i++) {
                    ImGui::PushID(i);

                    ImGui::Text("%d =", i + 1);

                    switch (ttype(k)) {
                        case LUA_TBOOLEAN: {
                            ImGui::SameLine();
                            bool value = bvalue(k);
                            ImGui::Checkbox("##value", &value);
                            if (value != bvalue(k))
                                bvalue(k) = value;
                            break;
                        } case LUA_TNUMBER:
                            ImGui::SameLine();
                            ImGui::DragScalar("##value", ImGuiDataType_Double, &nvalue(k));
                            break;
                        case LUA_TSTRING:  {
                            ImGui::SameLine();
                            std::string s(tsvalue(k)->data, tsvalue(k)->len);
                            if (ImGui_STDString("##value", s))
                                setsvalue(L, k, luaS_newlstr(L, s.c_str(), s.size()));
                            break;
                        }
                        default: {
                            ImGui::SameLine();
                            std::string str = rawtostringobj(L, k, true);
                            static const char* fmt = "%.*s";
                            size_t size = snprintf(NULL, 0, fmt, static_cast<int>(str.size()), str.c_str());
                            if (size > msg_size) {
                                msg_size = size;
                                msg = static_cast<char*>(realloc(msg, msg_size));
                            }
                            snprintf(msg, size + 1, fmt, static_cast<int>(str.size()), str.c_str());

                            switch (ttype(k)) {
                                case LUA_TTABLE: {
                                    if (ImGui::Button(msg)) {
                                        UI_TableExplorer_setSelectedTable(L, hvalue(k));
                                        menu_table_explorer_open = true;
                                    }
                                    break;
                                }
                                case LUA_TFUNCTION: {
                                    if (ImGui::Button(msg))
                                        function_to_chose = clvalue(k);
                                    break;
                                }
                                default:
                                    ImGui::TextUnformatted(msg, msg + size);
                                    break;
                            }
                            break;
                        }
                    }
                    k++;
                    ImGui::PopID();
                }
                free(msg);

                ImGui::SeparatorText("Upvalues");

                // TODO: upvalues; render as either [function's xth stack element] or [function's xth upvalue];
                // have a button that selects that function

                ImGui::Text("WIP");
            }

            ImGui::EndChild();
        }

        lua_pop(L, 1);

        if (function_to_chose)
            UI_FunctionExplorer_setSelectedFunction(L, function_to_chose);
    });
}

}; // namespace fakeroblox
