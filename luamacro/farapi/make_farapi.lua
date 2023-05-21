-- This script converts farplug-wide.h into farapi.lua
-- BEWARE: quick-and-dirty processing

local rex = require "rex_pcre"

local srcfile = os.getenv("HOME").."/far2m/far/far2sdk/farplug-wide.h"
local trgfile = "farapi.lua"
local f_in = assert(io.open(srcfile))
local txt = f_in:read("*all")
f_in:close()

local f_out = assert(io.open(trgfile, "w"))

local function Remove_FAR_USE_INTERNALS()
  local t={}
  local stage="outside"
  for line in txt:gmatch("([^\n]*)\n") do
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

txt = Remove_FAR_USE_INTERNALS() -- must go first
txt = rex.gsub(txt, "#ifdef\\s+__cplusplus\\b(.|\n)*?#endif\\b", "")
txt = rex.gsub(txt, "^\\s*#[^\n]+\n?", "", nil, "m")   -- delete all preprocessor lines
txt = rex.gsub(txt, "\\bWINAPI\\b", "__stdcall")       -- LuaJIT
txt = rex.gsub(txt, "\\bWINAPIV\\b", "__cdecl")        -- LuaJIT
txt = rex.gsub(txt, "\\b_export\\b", "")               -- LuaJIT doesn't accept _export
txt = rex.gsub(txt, "(?<=[0-9A-Fa-f])UL\\b", "U")      -- LuaJIT doesn't accept UL at the end
txt = rex.gsub(txt, "\\/\\*.*?\\*\\/", "\n", nil, "s") -- delete multiline comments
txt = rex.gsub(txt, "^\\s*\\/\\/.*\n?", "", nil, "m")  -- delete line comments

f_out:write("local ffi = require \"ffi\"\n")
f_out:write("ffi.cdef [=[\n")
f_out:write("#pragma pack(2)\n")
f_out:write(txt, "\n]=]\n")
f_out:close()
