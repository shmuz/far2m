-- started: 2024-07-23

local function FarAbout()
  local Props = { Title="About far2m"; Bottom="CtrlC: Copy"; HelpTopic=":FarAbout"; }
  local Items, Bkeys = {}, { {BreakKey="CtrlC"} }
  local Array = {}

  local Add = function(name, val)
    name = name or ""
    val = val == nil and "" or tostring(val)
    local text = ("%-30s│ %s"):format(name, val)
    table.insert(Items, { text=text; })
    table.insert(Array, text)
  end

  local AddEnv = function(name, indent)
    local val = os.getenv(name)
    if val ~= nil then
      if indent then name = (" "):rep(indent)..name end
      Add(name, val)
    end
  end

  local Inf = Far.GetInfo()
  local un = win.uname()

  Add("FAR2M version",         Inf.Build)
  if Inf.Compiler then
    Add("  Compiler", Inf.Compiler)
  end
  Add("  Platform",            Inf.Platform)
  Add("  Backend",             Inf.WinPortBackEnd)
  Add("  ConsoleColorPalette", Inf.ConsoleColorPalette)
  Add("  Admin",               Far.IsUserAdmin and "yes" or "no")
  Add("  PID",                 Far.PID)
  Add("  Main and Help languages", Inf.MainLang  ..", ".. Inf.HelpLang)
  Add("  OEM and ANSI codepages", win.GetOEMCP() ..", ".. win.GetACP())
  AddEnv("FARHOME", 2)
  Add("  Config directory", far.InMyConfig())
  Add("  Cache directory",  far.InMyCache())
  Add("  Temp directory",   far.InMyTemp())

  Add()
  Add("uname",     "")
  Add("  sysname", un.sysname)
  Add("  release", un.release)
  Add("  version", un.version)
  Add("  machine", un.machine)

  Add()
  Add("Host", win.GetHostName())
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
  Add("-- Plugins (" ..#plugs.. ")")
  for _, v in ipairs(plugs) do
   local dt = far.GetPluginInformation(v)
   --Add(dt.ModuleName:match("[^/]+$"), "")
   Add(dt.GInfo.Title, dt.GInfo.Description)
  end

  local item = far.Menu(Props, Items, Bkeys)
  if item and item.BreakKey == "CtrlC" then
    table.insert(Array, "")
    far.CopyToClipboard(table.concat(Array, "\n"))
  end
end

return FarAbout
