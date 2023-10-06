-- started: 2010-05-30
-- author: Shmuel Zeigerman

--[[---------------------------------------------------------------------------
Purpose:
  Makes  creation and  maintenance of  LuaFAR plugins'  language files
  easy. Only one file ("template file") needs to be  maintained, while
  the  "language  files"  and  the  "Lua  module  file"  are generated
  automatically. The order of message blocks in the template file does
  not matter.
-------------------------------------------------------------------------------
Input:
  @aFileName:
    Name  of a "template" file.

-------------------------------------------------------------------------------
Files written:
  A) Lua module file.
  B) Language  files  (file per  language). These  files begin  with a
     UTF-8 BOM.
-------------------------------------------------------------------------------
Returns:
  Nothing. (If something goes wrong, errors are raised).
-------------------------------------------------------------------------------
--]]

local function get_quoted (s)
  if s:sub(1,1) ~= '"' then
    error("no opening quote in line: " .. s)
  end
  local len, q = 1, nil
  for c in s:sub(2):gmatch("\\?.") do
    len = len + c:len()
    if c == '"' then q=c break end
  end
  if not q then
    error("no closing quote in line: " .. s)
  end
  return s:sub(1, len)
end

local function join(dir, file)
  return dir:gsub("/?$", "/", 1) .. file
end

local function MakeLang (aFileName, aOutDir)
  assert ( type(aFileName) == "string" )

  local bom_utf8 = "\239\187\191"
  local exist = {}
  local t_out = {}
  local languages = {}
  local msgfile, lngfile

  do
    local fp = assert(io.open(aFileName))
    local stOut, stLang, stSett, stMsg = 1,2,3,4
    local state = stOut
    local n = 0
    local dflt
    if fp:read(3) ~= bom_utf8 then -- skip UTF8 BOM
      fp:seek("set", 0)
    end

    for line in fp:lines() do
      if state~=stMsg and (line:match("^%s*$") or line:match("^%s*//")) then
        line = line -- luacheck: no unused
      elseif state==stOut then
        if line:lower():match("%[languages%]") then
          state=stLang
        else
          error "[Languages] expected"
        end
      elseif state==stLang then
        if line:lower():match("%[settings%]") then
          assert(languages[1], "no language found in [Languages]")
          for k=1, 1 + #languages do t_out[k] = {} end
          state=stSett
        else
          local s = assert(line:match("^%s*(%.Language=%w%w%w.*)"))
          table.insert(languages, s)
        end
      elseif state==stSett then
        if line:lower():match("%[messages%]") then
          assert(msgfile, "no msgfile found in [Settings]")
          assert(lngfile, "no lngfile found in [Settings]")
          state = stMsg
        else
          local k,v = line:match("%s*([^%s=]+)%s*=%s*(%S+)")
          assert(k, "bad line in [Settings]")
          if k:lower()=="msgfile"     then msgfile=v
          elseif k:lower()=="lngfile" then lngfile=v
          end
        end
      elseif state==stMsg then
        if not line:match("^%s*//") then -- comment lines are always skipped
          if line:match("%S") then
            n = n + 1
            if n > #t_out then
              error("extra line in block: " .. line)
            elseif n == 1 then
              local ident = line:match("^([%a_][%w_]*)%s*$")
              if ident then
                if exist[ident] then
                  error("duplicate identifier: "..line)
                end
                exist[ident] = true
                table.insert(t_out[n], ident)
              else
                error("bad message name: `" .. line .. "'")
              end
            elseif n == 2 then
              dflt = get_quoted(line)
              table.insert(t_out[n], dflt)
            else
              table.insert(t_out[n],
                           line:match("^upd:") and "// need translation:\n"..dflt or get_quoted(line))
            end
          else -- empty line: serves as a delimiter between blocks
            if n > 0 then
              if n < #t_out then
                local t = t_out[1]
                error("too few lines in block `" .. t[#t] .. "'")
              end
              n = 0
            end
          end
        end
      end
    end
    fp:close()
  end
  ----------------------------------------------------------------------------
  -- check for duplicates
  local map = {}
  for _,name in ipairs(t_out[1]) do
    if map[name] then error("duplicate name: " .. name) end
    map[name] = true
  end
  ----------------------------------------------------------------------------
  local fp = assert(io.open(join(aOutDir, msgfile), "w"))
  fp:write("-- This file is auto-generated. Don't edit.\n\n")
  fp:write("local indexes = {\n")
  for k,name in ipairs(t_out[1]) do
    fp:write("  ", name, " = ", k-1, ",\n")
  end
  fp:write([[
}
local GetMsg = far.GetMsg
return setmetatable( {},
  { __index = function(t,s) return GetMsg(indexes[s]) end } )
]])
  fp:close()
  ----------------------------------------------------------------------------
  for k,v in ipairs(languages) do
    local lang = assert( v:match("%.Language=(%w+)") )
    local suffix = lang:lower()=="slovak" and "sky" or lang:sub(1,3)
    local fname = lngfile.."_"..suffix:lower()..".lng"
    fp = assert(io.open(join(aOutDir, fname), "w"))
    fp:write(bom_utf8, v, "\n\n")
    fp:write(table.concat(t_out[k+1], "\n"), "\n")
    fp:close()
  end
end

--return MakeLang
MakeLang(...)

