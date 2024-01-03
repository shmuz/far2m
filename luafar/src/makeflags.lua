-- This script is intended to generate the "farflags.c" file

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

local function add_defines (src, trg_int, trg_ptr)
  local skip = NewSkip()
  for line in src:gmatch("[^\r\n]+") do
    if not skip:Process(line) then
      local c, v = line:match("#define%s+([A-Z][A-Z0-9_]*)%s+(.+)")
      if c then
        if v:find("%(%s*void%s*%*%s*%)") or v:find("%(%s*HANDLE%s*%)") then
          table.insert(trg_ptr, c)
        else
          table.insert(trg_int, c)
        end
      end
    end
  end
end

local function add_enums (src, trg)
  local skip = NewSkip()
  local enum = false
  for line in src:gmatch("[^\r\n]+") do
    if not skip:Process(line) then
      if line:find("^%s*enum%s*[%w_]*%s*$") then
        enum = true
      elseif enum then
        if line:find("^%s*};") then
          enum = false
        else
          local c = line:match("^%s*([%w_]+)")
          if c then table.insert(trg, c) end
        end
      end
    end
  end
end

local function write_target (trg_int, trg_ptr)
  io.write [[
static const flag_pair flags[] = {
]]
  table.sort(trg_int)
  table.sort(trg_ptr)
  for _,v in ipairs(trg_int) do
    local len = math.max(1, 40 - #v)
    local space = (" "):rep(len)
    io.write(string.format('  {"%s",%s%s },\n', v, space, v))
  end
  for _,v in ipairs(trg_ptr) do
    local len = math.max(1, 40 - #v)
    local space = (" "):rep(len)
    io.write(string.format('  {"%s",%s(intptr_t) %s },\n', v, space, v))
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
// farflags.c
// DON'T EDIT: THIS FILE IS AUTO-GENERATED.

#ifdef __cplusplus
extern "C" {
#endif
#include "lua.h"
#ifdef __cplusplus
}
#endif

#define LUAFAR_INTERNALS

#include "farplug-wide.h"
#include "farcolor.h"
#include "farkeys.h"

typedef struct {
  const char* key;
  int64_t val;
} flag_pair;

]]


local file_bottom = [[
void add_flags (lua_State *L)
{
  int i;
  int nelem = sizeof(flags) / sizeof(flags[0]);
  for (i=0; i<nelem; ++i) {
    lua_pushnumber(L, flags[i].val);
    lua_setfield(L, -2, flags[i].key);
  }
}

]]

do
  local dir = ...
  assert (dir, "input directory not specified")

  local trg_int, trg_ptr = {}, {}
  for _,v in ipairs(t_winapi) do table.insert(trg_int, v) end

  for _,fname in ipairs { "farplug-wide.h", "farcolor.h", "farkeys.h" } do
    local fp = assert (io.open (dir.."/"..fname))
    local src = fp:read ("*all")
    fp:close()
    if fname == "farplug-wide.h" then
      add_defines(src, trg_int, trg_ptr)
    end
    add_enums(src, trg_int)
  end

  io.write(file_top)
  write_target(trg_int, trg_ptr)
  io.write(file_bottom)
end

