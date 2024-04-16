-- This module is created from source code of LuaMacro plugin.
-- Reason: make this functionality available to _any_ LuaFAR plugin.
-- The module's exported functions (serialize, deserialize, mdelete, mload, msave)
-- are described in macroapi_manual.<lang>.chm.

local JoinPath = win.JoinPath

local work_dir

local function checkarg (arg, argnum, reftype)
  if type(arg) ~= reftype then
    error(("arg. #%d: %s expected, got %s"):format(argnum, reftype, type(arg)), 3)
  end
end

local function basicSerialize (o)
  local tp = type(o)
  if tp == "nil" or tp == "boolean" then
    return tostring(o)
  elseif tp == "number" then
    if o == math.modf(o) then return tostring(o) end
    return string.format("(%.17f * 2^%d)", math.frexp(o)) -- preserve accuracy
  elseif tp == "string" then
    return string.format("%q", o)
  end
end

local function int64Serialize (o)
  if bit64.type(o) then
    return "bit64.new(\"" .. tostring(o) .. "\")"
  end
end

local function AddToIndex (idx, t)
  local n = idx[t]
  if not n then
    n = #idx + 1
    idx[n], idx[t] = t, n
    for k,v in pairs(t) do
      if type(k)=="table" then AddToIndex(idx, k) end
      if type(v)=="table" then AddToIndex(idx, v) end
    end
    if debug.getmetatable(t) then AddToIndex(idx,debug.getmetatable(t)) end
  end
end

local function tableSerialize (tbl)
  if type(tbl) == "table" then
    local idx = {}
    AddToIndex(idx, tbl)
    local lines = { "local t; local idx={}; for i=1,"..#idx.." do idx[i]={} end" }
    for i,t in ipairs(idx) do
      local found
      lines[#lines+1] = "do t=idx["..i.."]"
      for k,v in pairs(t) do
        local k2 = basicSerialize(k) or type(k)=="table" and "idx["..idx[k].."]"
        if k2 then
          local v2 = basicSerialize(v) or int64Serialize(v) or type(v)=="table" and "idx["..idx[v].."]"
          if v2 then
            found = true
            lines[#lines+1] = "  t["..k2.."] = "..v2
          end
        end
      end
      if found then lines[#lines+1]="end" else lines[#lines]=nil end
    end
    for i,t in ipairs(idx) do
      local mt = debug.getmetatable(t)
      if mt then
        lines[#lines+1] = "setmetatable(idx["..i.."], idx["..idx[mt].."])"
      end
    end
    lines[#lines+1] = "return idx[1]\n"
    return table.concat(lines, "\n")
  end
  return nil
end

local function serialize (o)
  local s = basicSerialize(o) or int64Serialize(o)
  return s and "return "..s or tableSerialize(o)
end

local function deserialize (str, isfile)
  checkarg(str, 1, "string")
  local chunk, err
  if isfile then
    chunk, err = loadfile(str)
  else
    chunk, err = loadstring(str)
  end
  if chunk==nil then return nil,err end

  setfenv(chunk, { bit64={new=bit64.new}; setmetatable=setmetatable; })
  local ok, result = pcall(chunk)
  if not ok then return nil,result end

  return result,nil
end

local function get_work_dir(key)
  work_dir = work_dir or far.InMyConfig("plugins/luafar")
  return JoinPath(work_dir, key)
end

local function mdelete (key, name)
  checkarg(key, 1, "string")
  checkarg(name, 2, "string")
  local dir = get_work_dir(key)
  if name ~= "*" then
    return win.DeleteFile(JoinPath(dir, name))
  else
    far.RecursiveSearch(dir, "*", function(item, fullname) win.DeleteFile(fullname) end)
    return win.RemoveDir(dir)
  end
end

local function msave (key, name, value)
  checkarg(key, 1, "string")
  checkarg(name, 2, "string")
  local str = serialize(value)
  if str then
    local dir = get_work_dir(key)
    if win.CreateDir(dir, true) then
      local fp = io.open(JoinPath(dir,name), "w")
      if fp then
        fp:write(str)
        fp:close()
        return true
      end
    end
  end
  return false
end

local function mload (key, name)
  checkarg(key, 1, "string")
  checkarg(name, 2, "string")
  return deserialize(JoinPath(get_work_dir(key),name), true)
end

local function field (t, seq)
  checkarg(t, 1, "table")
  checkarg(seq, 2, "string")
  for key in seq:gmatch("[^.]+") do
    t[key] = t[key] or {}
    t = t[key]
  end
  return t
end

return {
  deserialize = deserialize;
  field = field;
  mdelete = mdelete;
  mload = mload;
  msave = msave;
  serialize = serialize;
}
