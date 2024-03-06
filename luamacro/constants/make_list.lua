--[[--------------------------------------------------------------------------------
1. This utility takes macroopcode.hpp as its input and generates
   the file opcodes.cpp
--]]--------------------------------------------------------------------------------

local Skip = {}
local SkipMt = { __index=Skip; }

local function NewSkip()
  return setmetatable( { mLevel=0 }, SkipMt)
end

function Skip:Process(line)
  if line:find("^%s*//") then
    return true
  elseif self.mLevel == 0 then
    if line:find("^%s*#ifdef%s+FAR_USE_INTERNALS")
    or line:find("^%s*#if.-_WIN32_WINNT")
    or line:find("^%s*#if%s+0") then
      self.mLevel = 1
    end
    return self.mLevel > 0
  else
    if line:find("^%s*#if") then
      self.mLevel = self.mLevel + 1
    elseif line:find("^%s*#endif") then
      self.mLevel = self.mLevel - 1
    end
    return true
  end
end

local function add_enums (src, trg)
  local skip = NewSkip()
  local inside = false
  for line in src:gmatch("[^\r\n]+") do
    line = line:gsub("//.*", "")
    if not skip:Process(line) then
      if line:find("^%s*enum%s*[%w_]*%s*{?%s*$") then
        inside = true
      elseif inside then
        if line:find("^%s*};") then
          inside = false
        else
          local c = line:match("^%s*([%w_]+)")
          if c then table.insert(trg, c) end
        end
      end
    end
  end
end

local function add_static_const (src, trg)
  local skip = NewSkip()
  local inside = false
  for line in src:gmatch("[^\r\n]+") do
    line = line:gsub("//.*", "")
    if not skip:Process(line) then
      if line:find("^%s*static%s+const.*$") then
        inside = true
      elseif inside then
        local c = line:match("^%s*([%w_]+)")
        if c then table.insert(trg, c) end
        if line:find(";") then
          inside = false
        end
      end
    end
  end
end

local OutputFile = select(1, ...)
local InputFiles = { select(2, ...) }
assert(#InputFiles == 2)

local Header = [[
#include <stdio.h>
#include "macroopcode.hpp"
#include "macrovalues.hpp"

int main(int argc, char **argv)
{
    if (argc < 2) return 1;

	FILE* fp = fopen(argv[1], "w");
	if (!fp) return 2;

	fprintf(fp, "return {\n");
]]

local Footer = [[
	fprintf(fp, "}\n");

	fclose(fp);
	return 0;
}
]]

local out = assert(io.open(OutputFile, "w"))
out:write(Header)

local trg = {}
for _,file in ipairs(InputFiles) do
  local fp = assert(io.open(file))
  local src = fp:read ("*all")
  add_enums(src, trg)
  add_static_const(src, trg)
  fp:close()
end

for _,name in ipairs(trg) do
  local s = ('\tfprintf(fp, "  %s = 0x%%X;\\n", %s);'):format(name, name)
  out:write(s, "\n")
end

out:write(Footer)
out:close()
