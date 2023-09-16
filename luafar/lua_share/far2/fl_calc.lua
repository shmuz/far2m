--

local help = {}
local lib = { help=help }

local function addHelp(signature, description)
  help[#help+1] = signature
  help[#help+1] = description
end

addHelp('fact(n)', 'Calculate factorial of n')

local floor=math.floor
local huge=math.huge
local limit1=200

local function factSafe(n, p)
    if n>0 then return factSafe(n-1, n*p)
    else return p end
end

function lib.fact(n)
    --
    -- return factorial of n
    --
    n=floor(n)
    if n>limit1 then return huge
    elseif n>0  then return factSafe(n-1, n)
    elseif n==0 then return 1
    end
    return 0
end

addHelp('fib(n)', 'Calculate n-th Fibonacci number')

local fibs
local limit2=1500
function lib.fib(n)
    --
    -- return n-th Fibonacci number (iterative)
    --
    if n>limit2 then return huge end
    n=floor(n)
    fibs={[0]=0, 1, 1}
    for i=3,n do
        if not fibs[i] then
            fibs[i]=fibs[i-1]+fibs[i-2]
        end
    end

    return fibs[n]
end

addHelp("sum(table)",        'Calculate a sum of the table items')
addHelp("sum(a1, a2, ...)",  'Calculate a sum of the arguments')
addHelp("mean(table)",       'Calculate a mean value of the table items')
addHelp("mean(a1, a2, ...)", 'Calculate a mean value of the arguments')
addHelp("logx(n, base)",     'Calculate logarithm of <n> by <base>')

local function count (t, ...)
    return t == nil and 0 or
           type(t) == 'table' and count(unpack(t)) + count(...) or
           1 + count(...)
end

local function sum (t, ...)
    return t == nil and 0 or
           type(t) == 'table' and sum(unpack(t)) + sum(...) or
           t + sum(...)
end
lib.sum = sum

function lib.mean (...)
    local n = count(...)
    return n > 0 and sum(...) / n or 0
end

function lib.logx (num,base)
  return math.log(num) / math.log(base or 10)
end

return lib
