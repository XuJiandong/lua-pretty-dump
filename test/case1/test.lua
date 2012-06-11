
local a = loadCompiledResult("./case1/a.lua")
local b = loadCompiledResult("./case1/b.lua")

assert(a[1].opcode.hash == b[1].opcode.hash)
assert(a[2].opcode.hash ~= b[2].opcode.hash)

