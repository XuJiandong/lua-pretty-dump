
function f(limit)
    local a = 0
    for i = 1, limit do
        a = a + i
    end
    return a
end


function g()
    local s = 0
    for i = 1, 10 do
        s = s + f(i)
    end
    return s
end

print(g())

