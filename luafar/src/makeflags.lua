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
  local STR_INLINE = "FAR_INLINE_CONSTANT"
  local STATE_OUT, STATE_INLINE = 1, 2
  local state = STATE_OUT
  local skip = NewSkip()

  for line in src:gmatch("[^\r\n]+") do
    if not skip:Process(line) then
      if state == STATE_OUT then
        local c, v = line:match("#define%s+([A-Z][A-Z0-9_]*)%s+(.+)")
        if c and c ~= STR_INLINE then
          if v:find("%(%s*void%s*%*%s*%)") or v:find("%(%s*HANDLE%s*%)") then
            table.insert(trg_ptr, c)
          else
            table.insert(trg_int, c)
          end
        elseif c == nil and line:find(STR_INLINE) then
          state = STATE_INLINE
        end
      elseif state == STATE_INLINE then
        local c, delim = line:match("([A-Z][A-Z0-9_]*)%s*=.-([,;])")
        if c then
          table.insert(trg_int, c)
          if delim == ";" then state = STATE_OUT end
        end
      else
        error("must not get here")
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
  "FOREGROUND_BLUE", "FOREGROUND_GREEN", "FOREGROUND_RED", "FOREGROUND_INTENSITY", "FOREGROUND_TRUECOLOR",
  "BACKGROUND_BLUE", "BACKGROUND_GREEN", "BACKGROUND_RED", "BACKGROUND_INTENSITY", "BACKGROUND_TRUECOLOR",
  "COMMON_LVB_REVERSE_VIDEO", "COMMON_LVB_UNDERSCORE", "COMMON_LVB_STRIKEOUT",
  "CTRL_C_EVENT", "CTRL_BREAK_EVENT",
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
#include "lauxlib.h"
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
void push_far_flags (lua_State *L)
{
  int nelem = sizeof(flags) / sizeof(flags[0]);
  lua_createtable(L, 0, nelem);
  for (int i=0; i<nelem; ++i) {
    lua_pushnumber(L, flags[i].val);
    lua_setfield(L, -2, flags[i].key);
  }
  (void)luaL_dostring(L, far_Guids);
}

]]

local function write_guids(fname)
  local fp = assert(io.open(fname))
  io.write("const char far_Guids[] = \"far.Guids = {\"", "\n")
  for line in fp:lines() do
    local name,rest = line:match("^%s*DEFINE_GUID%s*%(%s*(%w+)(.+)")
    if name then
      local t = {}
      rest = rest:gsub("0[xX]", "")
      for v in rest:gmatch("%x%x?") do t[#t+1] = ("%02X"):format(tonumber(v,16)) end
      assert(#t == 16, name)
      io.write("  \"", name, "='",
        table.concat(t, nil,  1,  4), "-",
        table.concat(t, nil,  5,  6), "-",
        table.concat(t, nil,  7,  8), "-",
        table.concat(t, nil,  9, 10), "-",
        table.concat(t, nil, 11, 16), "';\"", "\n")
    end
  end
  io.write("\"}\";", "\n\n")
  fp:close()
end

do
  local dir = ...
  assert (dir, "input directory not specified")

  local trg_int, trg_ptr = {}, {}
  for _,v in ipairs(t_winapi) do table.insert(trg_int, v) end

  for _,fname in ipairs { "farcommon.h", "farplug-wide.h", "farcolor.h", "farkeys.h" } do
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
  write_guids(dir .. "/../src/DlgGuid.hpp")
  io.write(file_bottom)
end

