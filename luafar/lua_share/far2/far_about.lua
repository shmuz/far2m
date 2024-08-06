-- started: 2024-07-23

local function FarAbout()
  local Props = { Title="About far2m"; Bottom="CtrlC: Copy"; }
  local Items, Bkeys = {}, { {BreakKey="CtrlC"} }
  local Array = {}

  local Add = function(name, val, skipnil)
    if skipnil and val==nil then return end
    name = name or ""
    val = val == nil and "" or tostring(val)
    local text = ("%-30s│ %s"):format(name, val)
    table.insert(Items, { text=text; })
    table.insert(Array, text)
  end

  local Inf = Far.GetInfo()
  local un = win.uname()

  Add("FAR2M version",       Inf.Build)
  Add("  Platform",            Inf.Platform)
  Add("  Backend",             Inf.WinPortBackEnd)
  Add("  ConsoleColorPalette", Inf.ConsoleColorPalette)
  Add("  Admin",               Far.IsUserAdmin and "yes" or "no")
  Add("  PID",                 Far.PID)
  Add("  Main and Help languages", Inf.MainLang..", "..Inf.HelpLang)
  Add("  OEM and ANSI codepages", win.GetOEMCP()..", "..win.GetACP())
  Add("  FARHOME", os.getenv("FARHOME"), true)
  Add("  Config directory", far.InMyConfig())
  Add("  Cache directory",  far.InMyCache())
  Add("  Temp directory",   far.InMyTemp())

  Add()
  Add("uname",               "")
  Add("  sysname",           un.sysname)
  Add("  release",           un.release)
  Add("  version",           un.version)
  Add("  machine",           un.machine)

  Add()
  Add("Host",             win.GetHostName())
  Add("User",             os.getenv("USER"), true)
  Add("XDG_SESSION_TYPE", os.getenv("XDG_SESSION_TYPE"), true)
  Add("TERM",             os.getenv("TERM"), true)
  Add("COLORTERM",        os.getenv("COLORTERM"), true)
  Add("GDK_BACKEND",      os.getenv("GDK_BACKEND"), true)
  Add("DESKTOP_SESSION",  os.getenv("DESKTOP_SESSION"), true)

  local plugs = far.GetPlugins()
  Add()
  Add("-- Plugins (" ..#plugs.. ")")
  for i,v in ipairs(plugs) do
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
