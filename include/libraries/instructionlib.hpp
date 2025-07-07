#pragma once

#include "lcommon.h"
#include "lua.h"

namespace fakeroblox {

struct InstructionWrapper {
    Instruction insn;
};

void open_instructionlib(lua_State* L);

}; // namespace fakeroblox
