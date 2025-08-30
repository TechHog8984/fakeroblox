local opmap = {}
local function getop(insn)
    local op = insn.op
    local result = opmap[op]
    if not result then
        for k,v in instructionlib do
            if type(v) ~= "number" then
                continue
            end
            if v ~= op then
                continue
            end
            result = k
            break
        end
        opmap[op] = result
    end
    return result
end

local function log(...)
    warn("[instruction spy]", ...)
end

instructionlib.stephook:Connect(function(insn, func)
    if func == getop or func == log then return end

    log(getop(insn), 'A', insn.a, 'B', insn.b, 'C', insn.c, 'D', insn.d, 'E', insn.e)

    if insn.op == instructionlib.LOP_GETIMPORT then
        log("  IMPORT: ", instructionlib.getimport(insn))
    end
end)
