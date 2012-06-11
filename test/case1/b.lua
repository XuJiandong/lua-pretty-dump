
function f(limit)
    local a = 0
    for i = 1, limit do
        a = a + i
    end
    return a
end


function g()
    local s = 0
    for i = 1, 20 do
        s = s + f(i)
    end
    return s
end

print(g())

