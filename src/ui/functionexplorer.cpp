#include "ui/functionexplorer.hpp"

#include "classes/roblox/instance.hpp"
#include "classes/roblox/serviceprovider.hpp"

#include "common.hpp"
#include "imgui.h"

#include "lobject.h"
#include "lstate.h" // for L->top
#include "lua.h"
#include "lualib.h"
#include "lmem.h"

#include <memory>
#include <shared_mutex>

namespace fakeroblox {

static int selected_function_index = -1;
static std::shared_mutex select_function_mutex;

namespace UI_FunctionExplorer_methods {
    static int selectFunction(lua_State* L) {
        std::shared_ptr<rbxInstance> instance = lua_checkinstance(L, 1, "FunctionExplorer");
        luaL_checktype(L, 2, LUA_TFUNCTION);

        std::lock_guard lock(select_function_mutex);

        lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);
        selected_function_index = addToLookup(L, [&L] {
            lua_pushvalue(L, 2);
        });

        return 0;
    }
}

void UI_FunctionExplorer_init(lua_State* L) {
    auto FunctionExplorer = std::make_shared<rbxClass>();
    FunctionExplorer->name.assign("FunctionExplorer");
    FunctionExplorer->superclass = rbxClass::class_map["Instance"];

    FunctionExplorer->methods["SelectFunction"] = {
      .name = "SelectFunction",
      .func = UI_FunctionExplorer_methods::selectFunction
    };

    rbxClass::class_map["FunctionExplorer"] = FunctionExplorer;
    ServiceProvider::registerService("FunctionExplorer");

    lua_getglobal(L, "game");
    std::shared_ptr<rbxInstance> game = lua_checkinstance(L, -1, "DataModel");
    lua_pop(L, 1);

    ServiceProvider::getService(L, game, "FunctionExplorer");
}

std::vector<Closure*> closure_list;

void UI_FunctionExplorer_render(lua_State *L) {
    ImGui::BeginChild("Function List", ImVec2{ImGui::GetContentRegionAvail().x * 0.35f, ImGui::GetContentRegionAvail().y}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::Text("FUNCTION LIST WIP");

    // const bool was_gc_running = lua_gc(L, LUA_GCISRUNNING, 0);
    // if (was_gc_running)
    //     lua_gc(L, LUA_GCSTOP, 0);

    // int context;

    // luaM_visitgco(L, &context, [] (void* ud, lua_Page* page, GCObject* gc_object) {
    //     // auto context = static_cast<GetGcContext*>(ud);

    //     auto type = gc_object->gch.tt;
    //     if (type != LUA_TFUNCTION)
    //         return false;

    //     return false;
    // });

    ImGui::EndChild();

    if (selected_function_index > -1) {
        lua_getfield(L, LUA_REGISTRYINDEX, METHODLOOKUP);

        ImGui::SameLine();

        std::lock_guard lock(select_function_mutex);

        lua_rawgeti(L, -1, selected_function_index);
        TValue* func_value = L->top - 1;
        assert(func_value->tt == LUA_TFUNCTION);

        Closure* closure = clvalue(func_value);
        const bool is_c = closure->isC;

        ImGui::BeginChild("Function Display", ImVec2{0, 0}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Text("Function @ %d", selected_function_index);
        ImGui::Text("Context: %s", is_c ? "C" : "Lua");

        const char* debugname = is_c ? closure->c.debugname : (closure->l.p->debugname ? closure->l.p->debugname->data : nullptr);
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

            TValue* k = p->k;
            for (int i = 0; i < p->sizek; i++) {
                ImGui::Text("%u", i);

                ImGui::SameLine();
                switch (ttype(k)) {
                    case LUA_TNIL:
                        ImGui::Text("nil");
                        break;
                    case LUA_TBOOLEAN:
                        ImGui::Text("bool (%s)", bvalue(k) ? "true" : "false");
                        break;
                    case LUA_TNUMBER:
                        ImGui::Text("number (%.f)", nvalue(k));
                        break;
                    case LUA_TSTRING: 
                        ImGui::TextUnformatted(tsvalue(k)->data, tsvalue(k)->data + tsvalue(k)->len);
                        break;
                    default:
                        ImGui::Text("%s (WIP)", lua_typename(L, ttype(k)));
                        break;
                }
                k++;
            }

            ImGui::SeparatorText("Upvalues");

            // TODO: upvalues; render as either [function's xth stack element] or [function's xth upvalue];
            // have a button that selects that function

            ImGui::Text("WIP");
        }

        ImGui::EndChild();

        lua_pop(L, 2);
    }

    // if (was_gc_running)
    //     lua_gc(L, LUA_GCRESTART, 0);
}

}; // namespace fakeroblox
