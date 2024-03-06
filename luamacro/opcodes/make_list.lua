--[[--------------------------------------------------------------------------------
1. This utility takes macroopcode.hpp as its input and generates
   the file opcodes.cpp
--]]--------------------------------------------------------------------------------

local InputFile, OutputFile = ...
assert(OutputFile)

local Header = [[
#include <stdio.h>
#include "macroopcode.hpp"

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

local fp = assert(io.open(InputFile))
local out = assert(io.open(OutputFile, "w"))

out:write(Header)
for line in fp:lines() do
  local name, comment = line:match( "^%s+(MCODE_[%w_]+).-(//.-%S.*)" )
  name = name or line:match( "^%s+(MCODE_[%w_]+)" )
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
