-- This script is intended to generate the "flags.cpp" file

local function add_enums (src, trg)
  local enum, skip = false, false
  for line in src:gmatch("[^\r\n]+") do
    if line:find("#ifdef%s+FAR_USE_INTERNALS") or line:find("#if.-_WIN32_WINNT") then
      skip = true
    elseif skip then
      if line:find("#else") or line:find("#endif") then skip = false end
    else
      if line:find("^%s*enum%s*[%w_]*") then
        enum = true
      elseif enum then
        if line:find("^%s*};") then
          enum = false
        else
          local c = line:match("[%w_]+")
          if c then table.insert(trg, c) end
        end
      end
    end
  end
end

local function write_target (trg)
  io.write [[
static const flag_pair flags[] = {
]]
  table.sort(trg) -- sort the table: this will allow for binary search
  for k,v in ipairs(trg) do
    local len = math.max(1, 32 - #v)
    local space = (" "):rep(len)
    io.write(string.format('  {"%s",%s(INT_PTR) %s },\n', v, space, v))
  end
  io.write("};\n\n")
end

local file_top = [[
// flags.cpp
// DON'T EDIT: THIS FILE IS AUTO-GENERATED.

#ifdef __cplusplus
extern "C" {
#endif
#include "lua.h"
#ifdef __cplusplus
}
#endif

#include "farplug-wide.h"
#include "farcolor.h"
#include "farkeys.h"

typedef struct {
  const char* key;
  INT_PTR val;
} flag_pair;

]]


local file_bottom = [[
void add_@@@ (lua_State *L)
{
  int i;
  const int nelem = sizeof(flags) / sizeof(flags[0]);
  for (i=0; i<nelem; ++i) {
    lua_pushinteger(L, flags[i].val);
    lua_setfield(L, -2, flags[i].key);
  }
}

]]

do
  local fname = ...
  assert (fname, "input file not specified")
  local fp = assert (io.open (fname))
  local src = fp:read ("*all")
  fp:close()

  local collector = {}
  add_enums(src, collector)

  io.write(file_top)
  write_target(collector)
  local shortname = fname:match("[^\\/]+$")
  local rep = shortname=="farcolor.h" and "colors" or
              shortname=="farkeys.h" and "keys"    or
              error("this input file is not supported")
  file_bottom = file_bottom:gsub("@@@", rep)
  io.write(file_bottom)
end
