
local a = loadCompiledResult("./case2/a.lua")
local b = loadCompiledResult("./case2/b.lua")

assert(a[1].opcode.hash == b[1].opcode.hash)

