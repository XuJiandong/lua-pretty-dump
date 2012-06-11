local a = loadCompiledResult("./case3/a.lua")
local b = loadCompiledResult("./case3/b.lua")

local result = {}
for _, f in ipairs(a) do
    local found = false
    for _, g in ipairs(b) do
        if f.opcode.hash == g.opcode.hash then
            found = true
        end
    end
    if not found then
        table.insert(result, f.header.linedefined)
    end
end

assert(#result == 1)
assert(result[1] == 15)

