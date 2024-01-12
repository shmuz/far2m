--[[--------------------------------------------------------------------------------
1. This utility takes macroopcode.hpp as its input and generates
   the file opcodes.cpp
--]]--------------------------------------------------------------------------------

local FarSource   = os.getenv("HOME") .. "/far2m"
local InputFile   = FarSource .. "/far/src/macro/macroopcode.hpp"
local OutputFile  = "opcodes.cpp"

local Header = [[
#include <stdio.h>
#include "macroopcode.hpp"

int main()
{
	FILE* fp=fopen("opcodes.lua", "w");
	if (!fp) return 1;

	fprintf(fp, "return {\n");
]]

local Footer = [[
	fprintf(fp, "}\n");

	fclose(fp);
	return 0;
}
]]

local rex = require "rex_pcre"

local fp = io.open(InputFile)
local out = io.open(OutputFile, "w")

out:write(Header)
for line in fp:lines() do
  local name, comment = rex.match(line, [[^\s+(MCODE_\w+)(?:\s*=\w+)?\s*,\s*(//.*\S)?]])
  if name then
    if comment then
      comment = '"' .. comment:gsub("^//%s*", " -- "):gsub('"', '\\"') .. '"'
    else
      comment = '""'
    end
    local s=('\tfprintf(fp, "  %s=0x%%X;%%s\\n", %s, %s);'):format(name,name,comment)
    out:write(s, "\n")
  end
end
out:write(Footer)

out:close()
fp:close()
