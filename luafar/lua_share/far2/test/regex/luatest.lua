-- See Copyright Notice in the file LICENSE

local P = {}

-- arrays: deep comparison
function P.eq (t1, t2, lut)
  if t1 == t2 then return true end
  if type(t1) ~= "table" or type(t2) ~= "table" or #t1 ~= #t2 then
    return false
  end

  lut = lut or {} -- look-up table: are these 2 arrays already compared?
  lut[t1] = lut[t1] or {}
  if lut[t1][t2] then return true end
  lut[t2] = lut[t2] or {}
  lut[t1][t2], lut[t2][t1] = true, true

  for k,v in ipairs (t1) do
    if not P.eq (t2[k], v, lut) then return false end -- recursion
  end
  return true
end

-- a "nil GUID", to be used instead of nils in datasets
P.NT = "b5f74fe5-46f4-483a-8321-e58ba2fa0e17"

-- pack vararg in table, replacing nils with "NT" items
local function packNT (...)
  local t = {}
  for i=1, select ("#", ...) do
    local v = select (i, ...)
    t[i] = (v == nil) and P.NT or v
  end
  return t
end

-- unpack table into vararg, replacing "NT" items with nils
local function unpackNT (t)
  local len = #t
  local function unpack_from (i)
    local v = t[i]
    if v == P.NT then v = nil end
    if i == len then return v end
    return v, unpack_from (i+1)
  end
  if len > 0 then return unpack_from (1) end
end

-- print results (deep into arrays)
function P.print_results (val, printfunc, indent, lut)
  indent = indent or ""
  lut = lut or {} -- look-up table
  local str = tostring (val)
  if type (val) == "table" then
    if val == P.NT then
      printfunc (indent .. "nil")
    elseif lut[val] then
      printfunc (indent .. str)
    else
      lut[val] = true
      printfunc (indent .. str)
      for _,v in ipairs (val) do
        P.print_results (v, printfunc, "  " .. indent, lut) -- recursion
      end
    end
  else
    printfunc (indent .. str)
  end
end

-- returns:
--  1) true, if success; false, if failure
--  2) test results table or error_message
function P.test_function (test, func)
  local res
  local t = packNT (pcall (func, unpackNT (test[1])))
  if t[1] then
    table.remove (t, 1)
    res = t
  else
    res = t[2] --> error_message
  end
  local how = (type (res) == type (test[2])) and
    (type (res) == "string" or P.eq (res, test[2])) -- allow error messages to differ
  return how, res
end

-- returns:
--  1) true, if success; false, if failure
--  2) test results table or error_message
--  3) test results table or error_message
function P.test_method (test, constructor, name)
  local res1, res2
  local ok, r = pcall (constructor, unpackNT (test[1]))
  if ok then
    local t = packNT (pcall (r[name], r, unpackNT (test[2])))
    if t[1] then
      table.remove (t, 1)
      res1, res2 = t, nil
    else
      res1, res2 = 2, t[2] --> 2, error_message
    end
  else
    res1, res2 = 1, r  --> 1, error_message
  end
  return P.eq (res1, test[3]), res1, res2
end

-- returns: a list of failed tests
function P.test_set (set, lib)
  local list = {}

  if type (set.Func) == "function" then
    local func = set.Func
    for i,test in ipairs (set) do
      local ok, res = P.test_function (test, func)
      if not ok then
        table.insert (list, {i=i, res})
      end
    end

  elseif type (set.Method) == "string" then
    for i,test in ipairs (set) do
      local ok, res1, res2 = P.test_method (test, lib.new, set.Method)
      if not ok then
        table.insert (list, {i=i, res1, res2})
      end
    end

  else
    error ("neither set.Func nor set.Method is valid")
  end

  return list
end

return P
