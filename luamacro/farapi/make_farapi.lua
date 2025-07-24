-- This script converts farplug-wide.h into farapi.lua
-- BEWARE: quick-and-dirty processing

local rex = require "rex_pcre"

local srcdir = os.getenv("HOME").."/repos/far2m/far/far2sdk"
local srcfiles = { srcdir.."/farcommon.h", srcdir.."/farplug-wide.h" }
local trgfile = "farapi.lua"

local txt = ""
for _,fname in ipairs(srcfiles) do
  local f = assert(io.open(fname))
  txt = txt .. f:read("*all")
  f:close()
end

local trg_path = ... or trgfile
local f_out = assert(io.open(trg_path, "w"))

local function Remove_FAR_USE_INTERNALS(aText)
  local t={}
  local stage="outside"
  for line in aText:gmatch("([^\n]*)\n") do
    local insert=true
    if stage=="outside" then
      if line:find("#ifdef%s+FAR_USE_INTERNALS") then
        stage="if"; insert=false
      end
    elseif stage=="if" then
      insert=false
      if line:find("#endif") then
        stage="outside"
      elseif line:find("#else") then
        stage="else"
      end
    elseif stage=="else" then
      if line:find("#endif") then
        stage="outside"; insert=false
      end
    end
    if insert then table.insert(t,line) end
  end
  return table.concat(t, "\n")
end

local function RemoveFarInlineConstants(aText)
  aText = aText:gsub("\n%s*FAR_INLINE_CONSTANT.-\n(.-);", "\nenum {\n%1\n};")
  return aText
end

txt = Remove_FAR_USE_INTERNALS(txt) -- must go first
txt = RemoveFarInlineConstants(txt)
txt = rex.gsub(txt, "#ifdef\\s+__cplusplus\\b(.|\n)*?#endif\\b", "")
txt = rex.gsub(txt, "^\\s*#[^\n]+\n?", "", nil, "m")   -- delete all preprocessor lines
txt = rex.gsub(txt, "\\bWINAPI\\b", "__stdcall")       -- LuaJIT
txt = rex.gsub(txt, "\\bWINAPIV\\b", "__cdecl")        -- LuaJIT
txt = rex.gsub(txt, "\\b_export\\b", "")               -- LuaJIT doesn't accept _export
txt = rex.gsub(txt, "(?<=[0-9A-Fa-f])UL\\b", "U")      -- LuaJIT doesn't accept UL at the end
txt = rex.gsub(txt, "\\/\\*.*?\\*\\/", "\n", nil, "s") -- delete multiline comments
txt = rex.gsub(txt, "^\\s*\\/\\/.*\n?", "", nil, "m")  -- delete line comments

f_out:write("local ffi = require \"ffi\"\n")
f_out:write("require \"winapi\"\n\n")
f_out:write("ffi.cdef [=[\n")
f_out:write("#pragma pack(2)\n")
f_out:write(txt, "\n]=]\n")
f_out:close()
