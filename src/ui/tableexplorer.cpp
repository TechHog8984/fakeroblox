#include "ui/tableexplorer.hpp"

#include "taskscheduler.hpp"

#include <cstddef>

#include "imgui.h"

#include "lua.h"
#include "lgc.h"
#include "lobject.h"
#include "ltable.h"
#include "lapi.h"
#include "lmem.h"
#include "lstate.h"

#define SELECTED_TABLE_KEY "tablestuffselectedtable"

namespace fakeroblox {

static constexpr float INDENT = 20;

// options
static bool filter_has_array = false;
static bool filter_has_node = false;

void setSelectedTable(lua_State* L, LuaTable* t) {
    // TODO: I think this can cause a double free! verify this
    lua_pushnil(L);
    sethvalue(L, const_cast<TValue*>(luaA_toobject(L, -1)), t);
    lua_rawsetfield(L, LUA_REGISTRYINDEX, SELECTED_TABLE_KEY);
}

void UI_TableExplorer_render(lua_State *L) {
    LuaTable* selected_table = nullptr;

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Filter Has Array", nullptr, &filter_has_array);
            ImGui::MenuItem("Filter Has Node", nullptr, &filter_has_node);

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    lua_getfield(L, LUA_REGISTRYINDEX, SELECTED_TABLE_KEY);
    if (lua_istable(L, -1))
        selected_table = hvalue(luaA_toobject(L, -1));

    ImGui::BeginChild("Table List", ImVec2{200.f, ImGui::GetContentRegionAvail().y});

    static char list_search_buffer[128] = "";
    ImGui::InputText("Search", list_search_buffer, IM_ARRAYSIZE(list_search_buffer));

    ImGui::BeginChild("ScrollableRegion", ImGui::GetContentRegionAvail(), 0, ImGuiWindowFlags_HorizontalScrollbar);

    const bool resume_gc = TaskScheduler::pauseGarbageCollection(L);

    struct VisitUserdata_t {
        LuaTable* current_selected;
        LuaTable* new_selected = nullptr;
        char msg[30];
    } VisitUserdata;
    VisitUserdata.current_selected = selected_table;

    luaM_visitgco(L, &VisitUserdata, [](void* ud, lua_Page* page, GCObject* object) {
        auto visit_ud = static_cast<VisitUserdata_t*>(ud);
        if (object->gch.tt == LUA_TTABLE) {
            auto t = gco2h(object);

            if ((filter_has_array && !t->sizearray) || (filter_has_node && sizenode(t) == 0))
                goto RET;
            const bool is_selected = visit_ud->current_selected && t == visit_ud->current_selected;

            static const char* fmt = "%p";
            int size = snprintf(NULL, 0, fmt, object);
            snprintf(visit_ud->msg, size + 1, fmt, object);

            if (strlen(list_search_buffer) == 0 || std::string(visit_ud->msg).find(list_search_buffer) != std::string::npos)
                if (ImGui::Selectable(visit_ud->msg, is_selected))
                    visit_ud->new_selected = t;
        }
        RET:
        return false;
    });

    if (VisitUserdata.new_selected) {
        setSelectedTable(L, VisitUserdata.new_selected);
        selected_table = VisitUserdata.new_selected;
    }

    ImGui::EndChild();

    ImGui::EndChild();

    LuaTable* table_to_chose = nullptr;

    if (selected_table) {
        ImGui::SameLine();

        ImGui::BeginChild("Table Explorer", ImVec2{0, 0}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Text("table %p", selected_table);

        ImGui::SeparatorText("array");
        ImGui::Text("size array: %d", selected_table->sizearray);

        // TODO: this section is in dire need of a code cleanup

        ImGui::Text("{");
        ImGui::Indent(INDENT);

        size_t msg_size = 50 * sizeof(char);
        char* msg = static_cast<char*>(malloc(msg_size));

        for (int i = 0; i < selected_table->sizearray; i++) {
            auto obj = &selected_table->array[i];
            if (!ttisnil(obj)) {
                std::string str = safetostringobj(L, obj, true);

                ImGui::Text("%d =", i + 1);

                static const char* fmt = "%.*s";
                size_t size = snprintf(NULL, 0, fmt, static_cast<int>(str.size()), str.c_str());
                if (size > msg_size) {
                    msg_size = size;
                    msg = static_cast<char*>(realloc(msg, msg_size));
                }
                snprintf(msg, size + 1, fmt, static_cast<int>(str.size()), str.c_str());

                ImGui::SameLine();
                if (ttistable(obj)) {
                    if (ImGui::Button(msg))
                        table_to_chose = hvalue(obj);
                } else
                    ImGui::TextUnformatted(msg, msg + size);

            }
        }

        ImGui::Indent(-INDENT);
        ImGui::Text("}");

        ImGui::SeparatorText("node");
        ImGui::Text("size node: %d", sizenode(selected_table));

        ImGui::Text("{");
        ImGui::Indent(INDENT);

        lua_pushnil(L);
        StkId key = const_cast<StkId>(luaA_toobject(L, -1));

        for (int i = 0; i < sizenode(selected_table); i++) {
            auto obj = gval(gnode(selected_table, i));
            if (!ttisnil(obj)) {
                getnodekey(L, key, gnode(selected_table, i));
                std::string key_str = safetostringobj(L, key, true);
                std::string value_str = safetostringobj(L, obj, true);

                static const char* fmt_left = "[%.*s] =";
                size_t size_left = snprintf(NULL, 0, fmt_left, static_cast<int>(key_str.size()), key_str.c_str());
                if (size_left > msg_size) {
                    msg_size = size_left;
                    msg = static_cast<char*>(realloc(msg, msg_size));
                }
                snprintf(msg, msg_size + 1, fmt_left, static_cast<int>(key_str.size()), key_str.c_str());

                ImGui::TextUnformatted(msg, msg + size_left);

                static const char* fmt_right = "%.*s";
                size_t size_right = snprintf(NULL, 0, fmt_right, static_cast<int>(value_str.size()), value_str.c_str());
                if (size_right > msg_size) {
                    msg_size = size_right;
                    msg = static_cast<char*>(realloc(msg, msg_size));
                }
                snprintf(msg, msg_size + 1, fmt_right, static_cast<int>(value_str.size()), value_str.c_str());

                ImGui::SameLine();
                if (ttistable(obj)) {
                    if (ImGui::Button(msg))
                        table_to_chose = hvalue(obj);
                } else
                    ImGui::TextUnformatted(msg, msg + size_right);
            }
        }
        free(msg);

        lua_pop(L, 1);

        ImGui::Indent(-INDENT);
        ImGui::Text("}");

        ImGui::EndChild();
    }

    lua_pop(L, 1);

    if (table_to_chose)
        setSelectedTable(L, table_to_chose);

    if (resume_gc)
        TaskScheduler::resumeGarbageCollection(L);
}

}; // namespace fakeroblox
