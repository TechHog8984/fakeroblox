#include "classes/color3.hpp"
#include "classes/roblox/datatypes/rbxscriptsignal.hpp"
#include "classes/roblox/instance.hpp"
#include "common.hpp"
#include "ltable.h"
#include "ui/functionexplorer.hpp"
#include "ui/ui.hpp"

#include "classes/roblox/serviceprovider.hpp"

#include "taskscheduler.hpp"

#include "imgui.h"

#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "lua.h"
#include "lualib.h"
#include "lmem.h"

#include <memory>

namespace fakeroblox {

namespace ImGuiService_methods {
    static int begin(lua_State* L) {
        const char* str = luaL_checkstring(L, 1);
        std::shared_ptr<rbxInstance> boolvalue = lua_optinstance(L, 2, "BoolValue");

        bool* p_open = NULL;
        if (boolvalue)
            p_open = &getInstanceValue<bool>(boolvalue, "Value");

        lua_pushboolean(L, ImGui::Begin(str, p_open));
        return 1;
    }
    static int end(lua_State* L) {
        ImGui::End();

        return 0;
    }

    static int text(lua_State* L) {
        size_t textl;
        const char* text = luaL_checklstring(L, 1, &textl);

        ImGui::Text("%.*s", static_cast<int>(textl), text);

        return 0;
    }

    static int button(lua_State* L) {
        const char* label = luaL_checkstring(L, 1);

        lua_pushboolean(L, ImGui::Button(label));
        return 1;
    }
    static int checkbox(lua_State* L) {
        const char* label = luaL_checkstring(L, 1);
        std::shared_ptr<rbxInstance> boolvalue = lua_checkinstance(L, 2, "BoolValue");

        lua_pushboolean(L, ImGui::Checkbox(label, &getInstanceValue<bool>(boolvalue, "Value")));
        return 1;
    }
    static int bullet(lua_State* L) {
        ImGui::Bullet();
        return 0;
    }

    static int beginCombo(lua_State* L) {
        const char* label = luaL_checkstring(L, 1);

        lua_pushboolean(L, ImGui::BeginCombo(label, "testing123"));
        return 1;
    }
    static int endCombo(lua_State* L) {
        ImGui::EndCombo();
        return 0;
    }
    static int combo(lua_State* L) {
        const char* label = luaL_checkstring(L, 1);
        std::shared_ptr<rbxInstance> intvalue = lua_checkinstance(L, 2, "IntValue");
        luaL_checktype(L, 3, LUA_TTABLE);

        LuaTable* table = hvalue(luaA_toobject(L, 3));
        const int item_count = luaH_getn(table);

        const char** items = static_cast<const char**>(malloc(sizeof(const char*) * item_count));
        for (int i = 0; i < item_count; i++) {
            auto value = &table->array[i];
            if (value->tt != LUA_TSTRING)
                luaL_error(L, "invalid value at index %d! expected string, got %s", i + 1, lua_typename(L, value->tt));
            items[i] = svalue(value);
        }

        int64_t* value_ptr = &getInstanceValue<int64_t>(intvalue, "Value");
        int32_t value = *value_ptr;

        const bool changed = ImGui::Combo(label, &value, items, item_count);
        free(items);

        if (changed)
            *value_ptr = value;

        lua_pushboolean(L, changed);
        return 1;
    }

    static int inputText(lua_State* L) {
        const char* label = luaL_checkstring(L, 1);
        std::shared_ptr<rbxInstance> stringvalue = lua_checkinstance(L, 2, "StringValue");

        lua_pushboolean(L, ImGui_STDString(label, getInstanceValue<std::string>(stringvalue, "Value")));
        return 1;
    }

    static int colorEdit(lua_State* L) {
        const char* label = luaL_checkstring(L, 1);
        std::shared_ptr<rbxInstance> color3value = lua_checkinstance(L, 2, "Color3Value");

        Color* color = &getInstanceValue<Color>(color3value, "Value");

        float col[3] = { color->r / 255.f, color->g / 255.f, color->b / 255.f };

        const bool changed = ImGui::ColorEdit3(label, col);

        if (changed) {
            color->r = col[0] * 255.f;
            color->g = col[1] * 255.f;
            color->b = col[2] * 255.f;
            setInstanceValue(color3value, L, "Value", *color);
        }

        lua_pushboolean(L, changed);
        return 1;
    }
}

std::shared_ptr<rbxInstance> ImGuiService;

void ImGuiService_init(lua_State* L, std::shared_ptr<rbxInstance> datamodel) {
    auto _class = std::make_shared<rbxClass>();
    _class->name.assign("ImGuiService");
    _class->tags |= rbxClass::NotCreatable;
    _class->superclass = rbxClass::class_map["Instance"];

    _class->methods["Begin"] = {
      .name = "Begin",
      .func = ImGuiService_methods::begin
    };
    _class->methods["End"] = {
      .name = "End",
      .func = ImGuiService_methods::end
    };

    _class->methods["Text"] = {
      .name = "Text",
      .func = ImGuiService_methods::text
    };

    _class->methods["Button"] = {
      .name = "Button",
      .func = ImGuiService_methods::button
    };
    _class->methods["Checkbox"] = {
      .name = "Checkbox",
      .func = ImGuiService_methods::checkbox
    };
    _class->methods["Bullet"] = {
      .name = "Bullet",
      .func = ImGuiService_methods::bullet
    };

    _class->methods["BeginCombo"] = {
      .name = "BeginCombo",
      .func = ImGuiService_methods::beginCombo
    };
    _class->methods["EndCombo"] = {
      .name = "EndCombo",
      .func = ImGuiService_methods::endCombo
    };
    _class->methods["Combo"] = {
      .name = "Combo",
      .func = ImGuiService_methods::combo
    };

    _class->methods["InputText"] = {
      .name = "InputText",
      .func = ImGuiService_methods::inputText
    };

    _class->methods["ColorEdit"] = {
      .name = "ColorEdit",
      .func = ImGuiService_methods::colorEdit
    };

    _class->events.push_back("Render");

    rbxClass::class_map["ImGuiService"] = _class;
    ServiceProvider::registerService("ImGuiService");

    ImGuiService = ServiceProvider::getService(L, datamodel, "ImGuiService");
}

void ImGuiService_render(lua_State *L) {
    pushFunctionFromLookup(L, fireRBXScriptSignal);
    ImGuiService->pushEvent(L, "Render");

    lua_call(L, 1, 0);
}

}; // namespace fakeroblox
