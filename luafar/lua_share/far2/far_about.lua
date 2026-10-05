-- started: 2024-07-23
-- far2m diagnostic script: far:about

local function os_release()
  local name
  local fp = io.open("/etc/os-release")
  if fp then
    local txt = fp:read("*all")
    fp:close()
    name = txt:match("PRETTY_NAME%s*=%s*([^\n]+)")
    if name then
      name = name:gsub("^\"(.+)\"$", "%1")
    end
  end
  return name
end

local function GetLuaEngineInfo()
  if jit then
    local str = jit.version:match(("%d"):rep(10))
    str = str and os.date(" (%Y-%m-%d)", str) or ""
    return jit.version .. str
  else
    return _VERSION
  end
end

local function FarAbout()
  local Props = {
    Title = "About far2m",
    Bottom = "CtrlC, CtrlIns: Copy",
    HelpTopic = ":FarAbout",
    Id = win.Uuid("01EB28A5-0A1A-4383-8536-0E4C24CC279B"),
    Flags = "FMENU_SHOWAMPERSAND FMENU_WRAPMODE",
  }
  local Items, Bkeys = {}, "CtrlC CtrlIns"
  local Array = {}

  local AddHeader = function(title)
    table.insert(Items, { text = title, separator = true })
    if Array[1] then
      table.insert(Array, "")
    end
    table.insert(Array, "=== " .. title .. " ===")
  end

  local Add = function(indent, name, val)
    if val == nil or val == "" then return end
    name = name or ""
    if indent and indent > 0 then
      name = (" "):rep(indent) .. name
    end
    local text = ("%-30s│ %s"):format(name, tostring(val))
    table.insert(Items, { text = text })
    table.insert(Array, text)
  end

  local AddEnv = function(name, indent)
    Add(indent or 0, name, os.getenv(name))
  end

  local Inf = Far.GetInfo()
  local uname = win.uname()

  -- Section 1: Core Engine
  AddHeader("1. Far2m Core & Engine")
  Add(0, "FAR2M version", Inf.Build)
  Add(0, "Compiler", Inf.Compiler)
  Add(0, "Platform", Inf.Platform)
  Add(0, "Lua Engine", GetLuaEngineInfo())
  if Inf.WinPortBackEnd then
    Add(0, "Backend", Inf.WinPortBackEnd[1])
    for k = 2, #Inf.WinPortBackEnd do
      Add(2, "System component", Inf.WinPortBackEnd[k])
    end
  end
  Add(0, "ConsoleColorPalette", Inf.ConsoleColorPalette)
  Add(0, "Admin / Root", Far.IsUserAdmin and "yes" or "no")
  Add(0, "PID", Far.PID)
  Add(0, "Main & Help languages", Inf.MainLang .. ", " .. Inf.HelpLang)
  Add(0, "OEM & ANSI codepages", win.GetOEMCP() .. ", " .. win.GetACP())

  -- Section 2: Storage & Configuration
  AddHeader("2. Directories & Config")
  Add(0, "Config directory", far.InMyConfig())
  Add(0, "Cache directory", far.InMyCache())
  Add(0, "Temp directory", far.InMyTemp())
  AddEnv("FARHOME")
  AddEnv("FAR2M_SETTINGS")
  AddEnv("FAR2M_ARGS")

  -- Section 3: Host Specifications
  AddHeader("3. Host & Operating System")
  Add(0, "os-release", os_release())
  Add(0, "Host Name", win.GetHostName())
  if uname then
    Add(0, "sysname", uname.sysname)
    Add(0, "release", uname.release)
    Add(0, "version", uname.version)
    Add(0, "machine", uname.machine)
  end
  AddEnv("WSL_DISTRO_NAME")

  -- Section 4: Terminal & Session
  AddHeader("4. Terminal, Session & Locale")
  AddEnv("USER")
  AddEnv("SUDO_USER")
  AddEnv("SHELL")
  AddEnv("TERM")
  AddEnv("TERM_PROGRAM")
  AddEnv("TERM_PROGRAM_VERSION")
  AddEnv("COLORTERM")
  AddEnv("DESKTOP_SESSION")
  AddEnv("XDG_SESSION_TYPE")
  AddEnv("DISPLAY")
  AddEnv("WAYLAND_DISPLAY")
  AddEnv("GDK_BACKEND")
  AddEnv("TMUX")
  AddEnv("STY")
  AddEnv("SSH_CLIENT")
  AddEnv("SSH_CONNECTION")
  AddEnv("LANG")
  AddEnv("LC_ALL")
  AddEnv("LC_CTYPE")

  -- Section 5: Plugins
  local plugs = far.GetPlugins()
  local info = {}
  for i, v in ipairs(plugs) do
    info[i] = far.GetPluginInformation(v)
  end
  table.sort(info, function(a, b)
    return (a.GInfo.Title or "") < (b.GInfo.Title or "")
  end)

  AddHeader("5. Active Plugins (" .. #plugs .. ")")
  if #info == 0 then
    Add(0, "Status", "(none loaded)")
  else
    for _, dt in ipairs(info) do
      Add(0, dt.GInfo.Title, dt.GInfo.Description)
    end
  end

  local item = far.Menu(Props, Items, Bkeys)
  if item and item.BreakKey then
    table.insert(Array, "")
    far.CopyToClipboard(table.concat(Array, "\n"))
  end
end

return FarAbout
