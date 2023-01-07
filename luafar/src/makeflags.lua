-- This script is intended to generate the "flags.cpp" file

local function add_defines (src, trg)
  local skip = false
  for line in src:gmatch("[^\r\n]+") do
    if line:find("^%s*//") then
      skip = skip
    elseif line:find("#ifdef%s+FAR_USE_INTERNALS") then
      skip = true
    elseif skip then
      if line:find("#else") or line:find("#endif") then skip = false end
    else
      local c = line:match("#define%s+([A-Z][A-Z0-9_]*)%s")
      if c then table.insert(trg, c) end
    end
  end
end

local function add_enums (src, trg)
  local enum, skip = false, false
  for line in src:gmatch("[^\r\n]+") do
    if line:find("^%s*//") then
      skip = skip
    elseif line:find("#ifdef%s+FAR_USE_INTERNALS") then
      skip = true
    elseif skip then
      if line:find("#else") or line:find("#endif") then skip = false end
    else
      if line:find("^%s*enum%s*[%w_]*%s*$") then
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

-- Windows API constants
local t_winapi = {
  "FOREGROUND_BLUE", "FOREGROUND_GREEN", "FOREGROUND_RED",
  "FOREGROUND_INTENSITY", "BACKGROUND_BLUE", "BACKGROUND_GREEN",
  "BACKGROUND_RED", "BACKGROUND_INTENSITY", "CTRL_C_EVENT", "CTRL_BREAK_EVENT",
  "CTRL_CLOSE_EVENT", "CTRL_LOGOFF_EVENT", "CTRL_SHUTDOWN_EVENT",
  "ENABLE_LINE_INPUT", "ENABLE_ECHO_INPUT", "ENABLE_PROCESSED_INPUT",
  "ENABLE_WINDOW_INPUT", "ENABLE_MOUSE_INPUT", --[["ENABLE_INSERT_MODE",
  "ENABLE_QUICK_EDIT_MODE", "ENABLE_EXTENDED_FLAGS", "ENABLE_AUTO_POSITION",]]
  "ENABLE_PROCESSED_OUTPUT", "ENABLE_WRAP_AT_EOL_OUTPUT", "KEY_EVENT",
  "MOUSE_EVENT", "WINDOW_BUFFER_SIZE_EVENT", "MENU_EVENT", "FOCUS_EVENT",
  "CAPSLOCK_ON", "ENHANCED_KEY", "RIGHT_ALT_PRESSED", "LEFT_ALT_PRESSED",
  "RIGHT_CTRL_PRESSED", "LEFT_CTRL_PRESSED", "SHIFT_PRESSED", "NUMLOCK_ON",
  "SCROLLLOCK_ON", "FROM_LEFT_1ST_BUTTON_PRESSED", "RIGHTMOST_BUTTON_PRESSED",
  "FROM_LEFT_2ND_BUTTON_PRESSED", "FROM_LEFT_3RD_BUTTON_PRESSED",
  "FROM_LEFT_4TH_BUTTON_PRESSED", "MOUSE_MOVED", "DOUBLE_CLICK", "MOUSE_WHEELED"
}


local file_top = [[
// flags.c
// DON'T EDIT: THIS FILE IS AUTO-GENERATED.

#ifdef __cplusplus
extern "C" {
#endif
#include "lua.h"
#ifdef __cplusplus
}
#endif

#include "farplug-wide.h"

typedef struct {
  const char* key;
  INT_PTR val;
} flag_pair;

]]


local file_bottom = [[
void add_flags (lua_State *L)
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
  add_defines(src, collector)
  add_enums(src, collector)
  for _,v in ipairs(t_winapi) do table.insert(collector, v) end

  io.write(file_top)
  write_target(collector)
  io.write(file_bottom)
end

