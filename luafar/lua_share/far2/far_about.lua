-- started: 2024-07-23

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

local function FarAbout()
  local Props = { Title="About far2m"; Bottom="CtrlC, CtrlIns: Copy"; HelpTopic=":FarAbout"; }
  local Items, Bkeys = {}, "CtrlC CtrlIns"
  local Array = {}

  local AddEmptyLine = function()
    local text = ("%-30s│"):format("")
    table.insert(Items, { text=text; })
    table.insert(Array, text)
  end

  local Add = function(indent, name, val)
    if not val then return end
    name = name or ""
    if indent then
      name = (" "):rep(indent)..name
    end
    local text = ("%-30s│ %s"):format(name, tostring(val))
    table.insert(Items, { text=text; })
    table.insert(Array, text)
  end

  local AddEnv = function(name, indent)
    Add(indent, name, os.getenv(name))
  end

  local Inf = Far.GetInfo()
  local uname = win.uname()

  Add(0, "FAR2M version", Inf.Build)
  Add(0, "Compiler", Inf.Compiler)
  Add(0, "Platform", Inf.Platform)
  Add(0, "Backend", Inf.WinPortBackEnd[1])
  for k=2,#Inf.WinPortBackEnd do
    Add(2, "System component", Inf.WinPortBackEnd[k])
  end
  Add(0, "ConsoleColorPalette", Inf.ConsoleColorPalette)
  Add(0, "Admin",               Far.IsUserAdmin and "yes" or "no")
  Add(0, "PID",                 Far.PID)
  Add(0, "Main and Help languages", Inf.MainLang  ..", ".. Inf.HelpLang)
  Add(0, "OEM and ANSI codepages", win.GetOEMCP() ..", ".. win.GetACP())
  AddEnv("FARHOME", 0)
  AddEnv("FARSETTINGS", 0)
  AddEnv("FAR2M_ARGS", 0)
  Add(0, "Config directory", far.InMyConfig())
  Add(0, "Cache directory",  far.InMyCache())
  Add(0, "Temp directory",   far.InMyTemp())

  AddEmptyLine()
  Add(0, "os-release", os_release())
  Add(0, "uname",   "")
  Add(2, "sysname", uname.sysname)
  Add(2, "release", uname.release)
  Add(2, "version", uname.version)
  Add(2, "machine", uname.machine)

  AddEmptyLine()
  Add(0, "Host", win.GetHostName())
  AddEnv("COLORTERM")
  AddEnv("DESKTOP_SESSION")
  AddEnv("GDK_BACKEND")
  AddEnv("TERM")
  AddEnv("USER")
  AddEnv("WAYLAND_DISPLAY")
  AddEnv("WSL_DISTRO_NAME")
  AddEnv("XDG_SESSION_TYPE")

  local plugs = far.GetPlugins()
  AddEmptyLine()
  Add(0, "-- Plugins (" ..#plugs.. ")", "")
  for _, v in ipairs(plugs) do
   local dt = far.GetPluginInformation(v)
   --Add(0, dt.ModuleName:match("[^/]+$"), "")
   Add(0, dt.GInfo.Title, dt.GInfo.Description)
  end

  local item = far.Menu(Props, Items, Bkeys)
  if item and item.BreakKey then
    table.insert(Array, "")
    far.CopyToClipboard(table.concat(Array, "\n"))
  end
end

return FarAbout
