-- started: 2024-07-23

local function FarAbout()
  local Props = { Title="About far2m"; Bottom="CtrlC: Copy"; HelpTopic=":FarAbout"; }
  local Items, Bkeys = {}, { {BreakKey="CtrlC"} }
  local Array = {}

  local Add = function(indent, name, val)
    name = name or ""
    if indent then
      name = (" "):rep(indent)..name
    end
    val = val == nil and "" or tostring(val)
    local text = ("%-30s│ %s"):format(name, val)
    table.insert(Items, { text=text; })
    table.insert(Array, text)
  end

  local AddEnv = function(name, indent)
    local val = os.getenv(name)
    if val ~= nil then
      Add(indent, name, val)
    end
  end

  local Inf = Far.GetInfo()
  local uname = win.uname()

  Add(0, "FAR2M version", Inf.Build)
  if Inf.Compiler then
    Add(2, "Compiler", Inf.Compiler)
  end
  Add(2, "Platform", Inf.Platform)

  if Inf.WinPortBackEnd == "GUI" then
    local build = os.getenv("FAR2M_WX_BUILD")
    local use   = os.getenv("FAR2M_WX_USE")
    if build and use then
      Add(2, "Backend", ("%s, WX_BUILD: %s, WX_USE: %s"):format(Inf.WinPortBackEnd, build, use))
    else
      Add(2, "Backend", Inf.WinPortBackEnd)
    end
  else
    Add(2, "Backend", Inf.WinPortBackEnd)
  end

  Add(2, "ConsoleColorPalette", Inf.ConsoleColorPalette)
  Add(2, "Admin",               Far.IsUserAdmin and "yes" or "no")
  Add(2, "PID",                 Far.PID)
  Add(2, "Main and Help languages", Inf.MainLang  ..", ".. Inf.HelpLang)
  Add(2, "OEM and ANSI codepages", win.GetOEMCP() ..", ".. win.GetACP())
  AddEnv("FARHOME", 2)
  AddEnv("FARSETTINGS", 2)
  AddEnv("FAR_ARGS", 2)
  Add(2, "Config directory", far.InMyConfig())
  Add(2, "Cache directory",  far.InMyCache())
  Add(2, "Temp directory",   far.InMyTemp())

  Add()
  Add(0, "uname",     "")
  Add(2, "sysname", uname.sysname)
  Add(2, "release", uname.release)
  Add(2, "version", uname.version)
  Add(2, "machine", uname.machine)

  Add()
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
  Add()
  Add(0, "-- Plugins (" ..#plugs.. ")")
  for _, v in ipairs(plugs) do
   local dt = far.GetPluginInformation(v)
   --Add(0, dt.ModuleName:match("[^/]+$"), "")
   Add(0, dt.GInfo.Title, dt.GInfo.Description)
  end

  local item = far.Menu(Props, Items, Bkeys)
  if item and item.BreakKey == "CtrlC" then
    table.insert(Array, "")
    far.CopyToClipboard(table.concat(Array, "\n"))
  end
end

return FarAbout
