#include "libraries/instructionlib.hpp"

#include "common.hpp"
#include "console.hpp"

#include <cassert>

#include "lua.h"
#include "lualib.h"
#include "lcommon.h"
#include "lstate.h"
#include "lobject.h"
#include "lapi.cpp"
#include "Luau/Bytecode.h"

namespace fakeroblox {

#define LUAU_SET_OP(insn, op) ((insn) = ((insn) & ~0xff) | ((op) & 0xff))

#define LUAU_SET_A(insn, a) ((insn) = ((insn) & ~(0xff << 8)) | (((a) & 0xff) << 8))

#define LUAU_SET_B(insn, b) ((insn) = ((insn) & ~(0xff << 16)) | (((b) & 0xff) << 16))

#define LUAU_SET_C(insn, c) ((insn) = ((insn) & ~(0xff << 24)) | (((c) & 0xff) << 24))

#define LUAU_SET_D(insn, d) ((insn) = ((insn) & 0xffff) | (((d) & 0xffff) << 16))

#define LUAU_SET_E(insn, e) ((insn) = ((insn) & 0xff) | (((e) & 0xffffff) << 8))

void stephook(lua_State* L, lua_Debug* ar);

InstructionWrapper* lua_checkinstruction(lua_State* L, int arg) {
    luaL_checkany(L, arg);
    void* ud = luaL_checkudata(L, arg, "InstructionWrapper");

    return static_cast<InstructionWrapper*>(ud);
}

int instruction__index(lua_State* L) {
    InstructionWrapper* insn = lua_checkinstruction(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strlen(key) == 1) {
        switch (*key) {
            case 'a':
            case 'A':
                lua_pushnumber(L, LUAU_INSN_A(insn->insn));
                break;
            case 'b':
            case 'B':
                lua_pushnumber(L, LUAU_INSN_B(insn->insn));
                break;
            case 'c':
            case 'C':
                lua_pushnumber(L, LUAU_INSN_C(insn->insn));
                break;
            case 'd':
            case 'D':
                lua_pushnumber(L, LUAU_INSN_D(insn->insn));
                break;
            case 'e':
            case 'E':
                lua_pushnumber(L, LUAU_INSN_E(insn->insn));
                break;
            default:
                goto INVALID;
        }
    } else if (strequal(key, "op")) {
        lua_pushnumber(L, LUAU_INSN_OP(insn->insn));
    } else
        goto INVALID;

    return 1;

    INVALID:
    luaL_error(L, "%s is not a valid member of instruction", key);
}
int instruction__newindex(lua_State* L) {
    InstructionWrapper* insn = lua_checkinstruction(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strlen(key) == 1) {
        switch (*key) {
            case 'a':
            case 'A':
                LUAU_SET_A(insn->insn, luaL_checkunsigned(L, 3));
                break;
            case 'b':
            case 'B':
                LUAU_SET_B(insn->insn, luaL_checkunsigned(L, 3));
                break;
            case 'c':
            case 'C':
                LUAU_SET_C(insn->insn, luaL_checkunsigned(L, 3));
                break;
            case 'd':
            case 'D':
                LUAU_SET_D(insn->insn, luaL_checkinteger(L, 3));
                break;
            case 'e':
            case 'E':
                LUAU_SET_E(insn->insn, luaL_checkinteger(L, 3));
                break;
            default:
                goto INVALID;
        }
    } else if (strequal(key, "op")) {
        LUAU_SET_OP(insn->insn, luaL_checkunsigned(L, 3));
    } else
        goto INVALID;

    return 0;

    INVALID:
    luaL_error(L, "'%s' is not a valid member of instruction", key);
}
int instruction__namecall(lua_State* L) {
    InstructionWrapper* insn = lua_checkinstruction(L, 1);
    const char* namecall = lua_namecallatom(L, nullptr);
    if (!namecall)
        luaL_error(L, "no namecall method!");

    if (strequal(namecall, "set"))
        insn->insn = luaL_checkunsigned(L, 2);

    return 0;
}

void open_instructionlib(lua_State *L) {
    // instruction global
    lua_newtable(L);

    lua_pushnumber(L, LOP_NOP);
    lua_setfield(L, -2, "LOP_NOP");

    lua_pushnumber(L, LOP_LOADNIL);
    lua_setfield(L, -2, "LOP_LOADNIL");

    lua_pushnumber(L, LOP_LOADB);
    lua_setfield(L, -2, "LOP_LOADB");

    lua_pushnumber(L, LOP_LOADN);
    lua_setfield(L, -2, "LOP_LOADN");

    lua_pushnumber(L, LOP_LOADK);
    lua_setfield(L, -2, "LOP_LOADK");

    lua_pushnumber(L, LOP_MOVE);
    lua_setfield(L, -2, "LOP_MOVE");

    lua_pushnumber(L, LOP_GETGLOBAL);
    lua_setfield(L, -2, "LOP_GETGLOBAL");

    lua_pushnumber(L, LOP_SETGLOBAL);
    lua_setfield(L, -2, "LOP_SETGLOBAL");

    lua_pushnumber(L, LOP_GETUPVAL);
    lua_setfield(L, -2, "LOP_GETUPVAL");

    lua_pushnumber(L, LOP_SETUPVAL);
    lua_setfield(L, -2, "LOP_SETUPVAL");

    lua_pushnumber(L, LOP_CLOSEUPVALS);
    lua_setfield(L, -2, "LOP_CLOSEUPVALS");

    lua_pushnumber(L, LOP_GETIMPORT);
    lua_setfield(L, -2, "LOP_GETIMPORT");

    lua_pushnumber(L, LOP_GETTABLE);
    lua_setfield(L, -2, "LOP_GETTABLE");

    lua_pushnumber(L, LOP_SETTABLE);
    lua_setfield(L, -2, "LOP_SETTABLE");

    lua_pushnumber(L, LOP_GETTABLEKS);
    lua_setfield(L, -2, "LOP_GETTABLEKS");

    lua_pushnumber(L, LOP_SETTABLEKS);
    lua_setfield(L, -2, "LOP_SETTABLEKS");

    lua_pushnumber(L, LOP_GETTABLEN);
    lua_setfield(L, -2, "LOP_GETTABLEN");

    lua_pushnumber(L, LOP_SETTABLEN);
    lua_setfield(L, -2, "LOP_SETTABLEN");

    lua_pushnumber(L, LOP_NEWCLOSURE);
    lua_setfield(L, -2, "LOP_NEWCLOSURE");

    lua_pushnumber(L, LOP_NAMECALL);
    lua_setfield(L, -2, "LOP_NAMECALL");

    lua_pushnumber(L, LOP_CALL);
    lua_setfield(L, -2, "LOP_CALL");

    lua_pushnumber(L, LOP_RETURN);
    lua_setfield(L, -2, "LOP_RETURN");

    lua_pushnumber(L, LOP_JUMP);
    lua_setfield(L, -2, "LOP_JUMP");

    lua_pushnumber(L, LOP_JUMPBACK);
    lua_setfield(L, -2, "LOP_JUMPBACK");

    lua_pushnumber(L, LOP_JUMPIF);
    lua_setfield(L, -2, "LOP_JUMPIF");

    lua_pushnumber(L, LOP_JUMPIFNOT);
    lua_setfield(L, -2, "LOP_JUMPIFNOT");

    lua_pushnumber(L, LOP_JUMPIFEQ);
    lua_setfield(L, -2, "LOP_JUMPIFEQ");

    lua_pushnumber(L, LOP_JUMPIFLE);
    lua_setfield(L, -2, "LOP_JUMPIFLE");
    lua_pushnumber(L, LOP_JUMPIFLT);
    lua_setfield(L, -2, "LOP_JUMPIFLT");
    lua_pushnumber(L, LOP_JUMPIFNOTEQ);
    lua_setfield(L, -2, "LOP_JUMPIFNOTEQ");
    lua_pushnumber(L, LOP_JUMPIFNOTLE);
    lua_setfield(L, -2, "LOP_JUMPIFNOTLE");
    lua_pushnumber(L, LOP_JUMPIFNOTLT);
    lua_setfield(L, -2, "LOP_JUMPIFNOTLT");

    lua_pushnumber(L, LOP_ADD);
    lua_setfield(L, -2, "LOP_ADD");
    lua_pushnumber(L, LOP_SUB);
    lua_setfield(L, -2, "LOP_SUB");
    lua_pushnumber(L, LOP_MUL);
    lua_setfield(L, -2, "LOP_MUL");
    lua_pushnumber(L, LOP_DIV);
    lua_setfield(L, -2, "LOP_DIV");
    lua_pushnumber(L, LOP_MOD);
    lua_setfield(L, -2, "LOP_MOD");
    lua_pushnumber(L, LOP_POW);
    lua_setfield(L, -2, "LOP_POW");

    lua_pushnumber(L, LOP_ADDK);
    lua_setfield(L, -2, "LOP_ADDK");
    lua_pushnumber(L, LOP_SUBK);
    lua_setfield(L, -2, "LOP_SUBK");
    lua_pushnumber(L, LOP_MULK);
    lua_setfield(L, -2, "LOP_MULK");
    lua_pushnumber(L, LOP_DIVK);
    lua_setfield(L, -2, "LOP_DIVK");
    lua_pushnumber(L, LOP_MODK);
    lua_setfield(L, -2, "LOP_MODK");
    lua_pushnumber(L, LOP_POWK);
    lua_setfield(L, -2, "LOP_POWK");

    lua_pushnumber(L, LOP_AND);
    lua_setfield(L, -2, "LOP_AND");
    lua_pushnumber(L, LOP_OR);
    lua_setfield(L, -2, "LOP_OR");

    lua_pushnumber(L, LOP_ANDK);
    lua_setfield(L, -2, "LOP_ANDK");
    lua_pushnumber(L, LOP_ORK);
    lua_setfield(L, -2, "LOP_ORK");

    lua_pushnumber(L, LOP_CONCAT);
    lua_setfield(L, -2, "LOP_CONCAT");

    lua_pushnumber(L, LOP_NOT);
    lua_setfield(L, -2, "LOP_NOT");
    lua_pushnumber(L, LOP_MINUS);
    lua_setfield(L, -2, "LOP_MINUS");
    lua_pushnumber(L, LOP_LENGTH);
    lua_setfield(L, -2, "LOP_LENGTH");

    lua_pushnumber(L, LOP_NEWTABLE);
    lua_setfield(L, -2, "LOP_NEWTABLE");

    lua_pushnumber(L, LOP_DUPTABLE);
    lua_setfield(L, -2, "LOP_DUPTABLE");

    lua_pushnumber(L, LOP_SETLIST);
    lua_setfield(L, -2, "LOP_SETLIST");

    lua_pushnumber(L, LOP_FORNPREP);
    lua_setfield(L, -2, "LOP_FORNPREP");

    lua_pushnumber(L, LOP_FORNLOOP);
    lua_setfield(L, -2, "LOP_FORNLOOP");

    lua_pushnumber(L, LOP_FORGLOOP);
    lua_setfield(L, -2, "LOP_FORGLOOP");

    lua_pushnumber(L, LOP_FORGPREP_INEXT);
    lua_setfield(L, -2, "LOP_FORGPREP_INEXT");

    lua_pushnumber(L, LOP_FASTCALL3);
    lua_setfield(L, -2, "LOP_FASTCALL3");

    lua_pushnumber(L, LOP_FORGPREP_NEXT);
    lua_setfield(L, -2, "LOP_FORGPREP_NEXT");

    lua_pushnumber(L, LOP_NATIVECALL);
    lua_setfield(L, -2, "LOP_NATIVECALL");

    lua_pushnumber(L, LOP_GETVARARGS);
    lua_setfield(L, -2, "LOP_GETVARARGS");

    lua_pushnumber(L, LOP_DUPCLOSURE);
    lua_setfield(L, -2, "LOP_DUPCLOSURE");

    lua_pushnumber(L, LOP_PREPVARARGS);
    lua_setfield(L, -2, "LOP_PREPVARARGS");

    lua_pushnumber(L, LOP_LOADKX);
    lua_setfield(L, -2, "LOP_LOADKX");

    lua_pushnumber(L, LOP_JUMPX);
    lua_setfield(L, -2, "LOP_JUMPX");

    lua_pushnumber(L, LOP_FASTCALL);
    lua_setfield(L, -2, "LOP_FASTCALL");

    lua_pushnumber(L, LOP_COVERAGE);
    lua_setfield(L, -2, "LOP_COVERAGE");

    lua_pushnumber(L, LOP_CAPTURE);
    lua_setfield(L, -2, "LOP_CAPTURE");

    lua_pushnumber(L, LOP_SUBRK);
    lua_setfield(L, -2, "LOP_SUBRK");
    lua_pushnumber(L, LOP_DIVRK);
    lua_setfield(L, -2, "LOP_DIVRK");

    lua_pushnumber(L, LOP_FASTCALL1);
    lua_setfield(L, -2, "LOP_FASTCALL1");

    lua_pushnumber(L, LOP_FASTCALL2);
    lua_setfield(L, -2, "LOP_FASTCALL2");

    lua_pushnumber(L, LOP_FASTCALL2K);
    lua_setfield(L, -2, "LOP_FASTCALL2K");

    lua_pushnumber(L, LOP_FORGPREP);
    lua_setfield(L, -2, "LOP_FORGPREP");

    lua_pushnumber(L, LOP_JUMPXEQKNIL);
    lua_setfield(L, -2, "LOP_JUMPXEQKNIL");
    lua_pushnumber(L, LOP_JUMPXEQKB);
    lua_setfield(L, -2, "LOP_JUMPXEQKB");

    lua_pushnumber(L, LOP_JUMPXEQKN);
    lua_setfield(L, -2, "LOP_JUMPXEQKN");
    lua_pushnumber(L, LOP_JUMPXEQKS);
    lua_setfield(L, -2, "LOP_JUMPXEQKS");

    lua_pushnumber(L, LOP_IDIV);
    lua_setfield(L, -2, "LOP_IDIV");

    lua_pushnumber(L, LOP_IDIVK);
    lua_setfield(L, -2, "LOP_IDIVK");

    lua_pushvalue(L, -1);
    lua_setglobal(L, "instruction");
    lua_setfield(L, LUA_REGISTRYINDEX, "instructionlib");

    luaL_newmetatable(L, "InstructionWrapper");

    lua_pushcfunction(L, instruction__index, "__index");
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, instruction__newindex, "__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, instruction__namecall, "__namecall");
    lua_setfield(L, -2, "__namecall");

    lua_pop(L, 1);

    lua_singlestep(L, true);
    lua_callbacks(L)->debugstep = stephook;
}

void stephook(lua_State* L, lua_Debug* ar) {
    const Instruction* pc = L->ci->savedpc;

    Closure* this_cl = clvalue(L->ci->func);
    if (pc <= this_cl->l.p->code) {
        Console::ScriptConsole.error("[step]: pc is out of range; it should be past the first instruction");
        return;
    }
    const Instruction insn = *--pc;

    lua_getglobal(L, "debugstephook");

    if (!lua_isnil(L, -1)) {
        luaL_checktype(L, -1, LUA_TFUNCTION);

        // stop recursion
        if (this_cl == clvalue(L->top - 1))
            return;

        InstructionWrapper* wrapper = static_cast<InstructionWrapper*>(lua_newuserdata(L, sizeof(InstructionWrapper)));
        wrapper->insn = insn;

        luaL_getmetatable(L, "InstructionWrapper");
        lua_setmetatable(L, -2);

        setclvalue(L, L->top, this_cl);
        api_incr_top(L);
        lua_call(L, 2, 0);

        *const_cast<Instruction*>(pc) = wrapper->insn;
    }
}

}; // namespace fakeroblox
