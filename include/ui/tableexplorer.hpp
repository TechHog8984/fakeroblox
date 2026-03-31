#pragma once

#include "lobject.h"
#include "lua.h"

namespace frostbyte {

void UI_TableExplorer_setSelectedTable(lua_State* L, LuaTable* t);
void UI_TableExplorer_render(lua_State* L);

}; // namespace frostbyte
