

LUAC = "../lua-5.1.5/src/luac.exe"

-- return a flat table of functions
-- ogirinal functions are in tree structure
-- also, sort them by opcode size for fast searching
local function normalize(func, funcTable)
    -- linedefined == 0, means it's chunk function
    if func.header and func.header.linedefined > 0 then
        table.insert(funcTable, func)
    end
    for _, f in ipairs(func) do
        normalize(f, funcTable)
    end
    table.sort(funcTable, function (a, b) return #a.opcode < #b.opcode end)
    return funcTable
end 

function loadCompiledResult(file)
    local output = io.popen(LUAC .. " -l -p " .. file, "r")
    local result = {}
    for line in output:lines() do
        table.insert(result, line)
    end
    local func = loadstring(table.concat(result, "\n"))()
    return normalize(func, {})
end

dofile("./case1/test.lua")
dofile("./case2/test.lua")
dofile("./case3/test.lua")
print("all test cases passed")

