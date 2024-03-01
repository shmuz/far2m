-- coding: utf-8
-- started: 2012-04-20

-- This plugin does not support reloading the default script on the fly.
if not (...) then return end

local GlobalInfo = far.GetPluginGlobalInfo()

local F, Msg = far.Flags, nil
local bor = bit64.bor
local co_yield, co_resume, co_status = coroutine.yield, coroutine.resume, coroutine.status

local PROPAGATE={} -- a unique value, inaccessible to scripts.
local gmeta = { __index=_G }
local LastMessage
local strParseError = ""
local Shared
local TablePanelSort -- must be separate from LastMessage, otherwise Far crashes after a macro is called from CtrlF12.
local TableExecString -- must be separate from LastMessage, otherwise Far crashes
local utils, macrobrowser, panelsort, keymacro, farcmds

local ExpandEnv = win.ExpandEnv

local RegexExpandEnv = regex.new( [[ \$ \( (\w+) \) | \$ (\w+) ]], "x")
local HomeDir = far.GetMyHome()

local function FullExpand(text)
  text = text:gsub("^%~", HomeDir)
  text = RegexExpandEnv:gsub(text, function(a,b) return win.GetEnv(a or b) end)
  return text
end

local function Unquote(text)
  text = text:gsub('^"(.+)"$', "%1") -- remove double quotes
  return text
end

local function pack (...)
  return { n=select("#",...), ... }
end

local function yield_resume (co, ...)
  local t1, t2 = ...
  if t1==true and t2==PROPAGATE then
    return co_resume(co, co_yield(select(2, ...)))
  end
  return ...
end

-- Override coroutine.resume for scripts, making it possible to call Keys(),
-- print(), Plugin.Call(), exit(), etc. from nested coroutines.
function coroutine.resume(co, ...) return yield_resume(co, co_resume(co, ...)) end

local ErrMsg = function(msg, title, buttons, flags)
  if type(msg)=="string" and not msg:utf8valid() and string.sub(msg,1,3)~="..." then
    local wstr = win.MultiByteToWideChar(msg, win.GetACP(), "e")
    msg = wstr and win.Utf32ToUtf8(wstr) or msg
  end
  return far.Message(msg, title or "LuaMacro", buttons, flags or "wl")
end

local function checkarg (arg, argnum, reftype)
  if type(arg) ~= reftype then
    error(("arg. #%d: %s expected, got %s"):format(argnum, reftype, type(arg)), 3)
  end
end

-------------------------------------------------------------------------------
-- Functions implemented via "returning a key" to Far
-------------------------------------------------------------------------------

function _G.Keys (...)
  for n=1,select("#",...) do
    local str=select(n,...)
    if type(str)=="string" then
      for key in str:gmatch("%S+") do
        local cnt,name = key:match("^(%d+)%*(.+)")
        if cnt then cnt = tonumber(cnt)
        else        cnt,name = 1,key
        end
        local lname = name:lower()
        if     lname == "disout" then keymacro.mmode(1,1)
        elseif lname == "enout"  then keymacro.mmode(1,0)
        else
          local R1,R2 = keymacro.TransformKey(name)
          for _=1,cnt do co_yield(PROPAGATE, F.MPRT_KEYS, R1, R2) end
        end
      end
    end
  end
end

function _G.print (...)
  local param = ""
  if select("#", ...)>0 then param = (...) end
  co_yield(PROPAGATE, F.MPRT_PRINT, tostring(param))
end

function _G.exit ()
  co_yield(PROPAGATE, "exit")
end

local function yieldcall (...)
  return co_yield(PROPAGATE, ...)
end

-------------------------------------------------------------------------------
-- END: Functions implemented via "returning a key" to Far
-------------------------------------------------------------------------------

local PluginInfo

function export.GetPluginInfo()
  local out = {
    Flags = bor(F.PF_PRELOAD,F.PF_FULLCMDLINE,F.PF_EDITOR,F.PF_VIEWER,F.PF_DIALOG),
    CommandPrefix = "lm:macro:lua:moon:luas:moons:edit:view:load:unload:goto"..utils.GetPrefixes()[1],
    PluginMenuStrings = { "Macro Browser" },
    PluginMenuGuids = { win.Uuid("EF6D67A2-59F7-4DF3-952E-F9049877B492") },
    PluginConfigGuids = {},
    DiskMenuGuids = {},
  }

  local mode = far.MacroGetArea()
  local area = utils.GetTrueAreaName(mode)
  local IsDiskMenuPossible = area=="Shell" or area=="Tree" or area=="QView" or area=="Info"
  for _,item in ipairs(utils.GetMenuItems()) do
    local flags = item.flags
    if flags.config then
      local ok, text = pcall(item.text, "Config", area)
      if ok then
        if type(text) == "string" then
          out.PluginConfigStrings = out.PluginConfigStrings or {}
          table.insert(out.PluginConfigStrings, text)
          table.insert(out.PluginConfigGuids, item.guid)
        end
      else
        ErrMsg(text)
      end
    end
    if IsDiskMenuPossible and flags.disks then
      local ok, text = pcall(item.text, "Disks", area)
      if ok then
        if type(text) == "string" then
          out.DiskMenuStrings = out.DiskMenuStrings or {}
          table.insert(out.DiskMenuStrings, text)
          table.insert(out.DiskMenuGuids, item.guid)
        end
      else
        ErrMsg(text)
      end
    end
    if flags.plugins and (flags[mode] or flags.common) then
      local ok, text = pcall(item.text, "Plugins", area)
      if ok then
        if type(text) == "string" then
          out.PluginMenuStrings = out.PluginMenuStrings or {}
          table.insert(out.PluginMenuStrings, text)
          table.insert(out.PluginMenuGuids, item.guid)
        end
      else
        ErrMsg(text)
      end
    end
  end
  PluginInfo = out
  return out
end

local function GetFileParams (Text)
  local from,to = Text:find("^%s*@%s*")
  if from then
    local fname
    if Text:sub(to+1,to+1) == "\"" then -- quoted file name
      from, to, fname = regex.find(Text, [["((?:\\.|[^"])+)"]], to+1)
    else -- unquoted file name
      from, to, fname = regex.find(Text, [[((?:\\.|\S)+)]], to+1)
    end
    if from then
      local space,params = Text:match("^(%s*)(.*)", to+1)
      if space~="" or params=="" then
        fname = far.SplitCmdLine(fname)
        return FullExpand(fname), params
      end
    end
    error("Invalid macrosequence specification")
  end
end

local function loadmacro (Lang, Text, Env, ConvertPath)
  local _loadstring, _loadfile = loadstring, loadfile
  if Lang == "moonscript" then
    local ms = require "moonscript"
    _loadstring, _loadfile = ms.loadstring, ms.loadfile
  end

  local f1,f2,msg
  local fname,params = GetFileParams(Text)
  if fname then
    fname = ConvertPath and far.ConvertPath(fname, F.CPM_NATIVE) or fname
    f2,msg = _loadstring("return "..params)
    if f2 then
      f1,msg = _loadfile(fname)
    end
  else
    f1,msg = _loadstring(Text)
  end

  if f1 then
    strParseError = ""
    Env = Env or setmetatable({}, gmeta)
    Env._filename = fname
    setfenv(f1, Env)
    if f2 then setfenv(f2, Env) end
    return f1,f2
  else
    strParseError = msg
    return nil,msg
  end
end

local function postmacro (f, ...)
  if type(f) == "function" then
    keymacro.PostNewMacro(pack(f, ...), 0, nil, true)
    return true
  end
  return false
end

local function FixReturn (handle, ok, ...)
  local ret1, ret_type = ...
  if ok then
    local status = co_status(handle.coro)
    if status == "suspended" and ret1 == PROPAGATE and ret_type ~= "exit" then
      handle._store = pack(select(3, ...))
      return ret_type, handle._store
    else
      return F.MPRT_NORMALFINISH, pack(true, ...)
    end
  else
    local msg = type(ret1)=="string" and ret1 or "(error object is not a string)"
    msg = string.gsub(debug.traceback(handle.coro, msg), "\n\t", "\n   ")
    ErrMsg(msg)
    return F.MPRT_ERRORFINISH
  end
end

local function MacroStep (handle, ...)
  if handle then
    local status = co_status(handle.coro)
    if status == "suspended" then
      if handle.params then
        local params = handle.params
        handle.params = nil
        local tp = type(params)
        if tp == "function" then
          local tt = pack(xpcall(params, debug.traceback))
          if tt[1] then
            return FixReturn(handle, co_resume(handle.coro, unpack(tt,2,tt.n)))
          else
            ErrMsg(tt[2])
            return F.MPRT_ERRORFINISH
          end
        elseif tp == "table" then
          return FixReturn(handle, co_resume(handle.coro, params))
        end
      else
        return FixReturn(handle, co_resume(handle.coro, ...))
      end
    else
      ErrMsg("Step: called on macro in "..status.." status") -- debug only: should not be here
    end
  else
    ErrMsg("Step: handle does not exist") -- debug only: should not be here
  end
end

local function MacroParse (Lang, Text, onlyCheck, skipFile)
  local _loadstring, _loadfile = loadstring, loadfile
  if Lang == "moonscript" then
    local ms = require "moonscript"
    _loadstring, _loadfile = ms.loadstring, ms.loadfile
  end

  local ok,msg
  local fname,params = GetFileParams(Text)
  if fname then
    ok,msg = _loadstring("return "..params)
    if ok and not skipFile then
      ok,msg = _loadfile(fname)
    end
  else
    ok,msg = _loadstring(Text)
  end

  if ok then
    strParseError = ""
    return F.MPRT_NORMALFINISH
  else
    strParseError = msg
    if not onlyCheck then
      far.Message(msg, Msg.MMacroParseErrorTitle, Msg.MOk, "lw")
    end
    return F.MPRT_ERRORPARSE
  end
end

local function GetLastParseError()
  LastMessage = pack(strParseError, tonumber(strParseError:match(":(%d+): ")) or 0, 0)
  return LastMessage
end

local function ExecString (lang, text, params, onlyCheck)
  if type(text)=="string" then
    local chunk, msg = loadmacro(lang, text)
    if chunk then
      TableExecString = pack(chunk(unpack(params,1,params.n)))
      return F.MPRT_NORMALFINISH, TableExecString
    else
      if not onlyCheck then ErrMsg(msg) end
      TableExecString = { msg }
      return F.MPRT_ERRORPARSE, TableExecString
    end
  end
end

local function About()
  -- LuaMacro
  local text = ("%s %d.%d.%d build %d"):format(GlobalInfo.Title, unpack(GlobalInfo.Version))

  -- Lua/LuaJIT
  text = text.."\n"..(jit and jit.version or _VERSION)

  -- MoonScript and LPeg
  local ok,lib = pcall(require, "moonscript.version")
  if ok then
    text = text.."\nMoonScript "..lib.version
    if lpeg then
      local v = lpeg.version
      text = text.."\n"..(type(v)=="function" and "LPeg "..v() or v) -- depends on LPeg version
    end
  end

  -- All together
  far.Message(text, "About", nil, "l")
end

local function ShowAndPass(...) far.Show(...) return ... end

local function Open_CommandLine (strCmdLine)
  local prefix, text = strCmdLine:match("^%s*([^:%s]+):%s*(.-)%s*$")
  if not prefix then return end -- this can occur with Plugin.Command()
  prefix = prefix:lower()
  ----------------------------------------------------------------------------
  if prefix == "lm" or prefix == "macro" then
    if text=="" then return end
    local cmd = text:match("%S*"):lower()
    if cmd == "load" then
      local paths = text:match("%S.*",5)
      paths = paths and paths:gsub([[^"(.+)"$]], "%1")
      far.MacroLoadAll(paths)
    elseif cmd == "save" then utils.WriteMacros()
    elseif cmd == "unload" then utils.UnloadMacros()
    elseif cmd == "about" then About()
    elseif cmd == "farconfig" then require("far2.far_config")()
    elseif cmd == "test" then
      far.MacroPost( [[
        local function Quit(n) actl.Quit(n) Keys("Esc") end
        local OK, R
        R = far.PluginStartupInfo().ShareDir.."/macrotest.lua"
        R = loadfile(R) or Quit(1)
        OK, R = pcall(R)
        OK = OK or Quit(2)
        R.test_all = R.test_all or Quit(3)
        OK = pcall(R.test_all)
        Quit(OK and 0 or 4)
      ]], 0, "CtrlShiftF12")
    elseif cmd ~= "" then
      ErrMsg(Msg.CL_UnsupportedCommand .. cmd)
    end
  ----------------------------------------------------------------------------
  elseif prefix == "lua" or prefix == "moon" or prefix == "luas" or prefix == "moons" then
    if text=="" then return end
    local show = false
    if text:find("^=") then
      show, text = true, text:sub(2)
    end
    local fname = GetFileParams(text)
    if show and not fname then
      text = "return "..text
    end
    local lang = (prefix=="lua" or prefix=="luas") and "lua" or "moonscript"
    local f1,f2 = loadmacro(lang, text, nil, true)
    if f1 then
      local ff1 = show and function(...) return ShowAndPass(f1(...)) end or f1
      if prefix=="lua" or prefix=="moon" then
        keymacro.PostNewMacro({ ff1,f2,HasFunction=true }, 0, nil, true)
      else
        f2 = f2 or function() end
        Shared.CmdLineResult = nil
        Shared.CmdLineResult = pack(ff1(f2()))
      end
    else
      ErrMsg(f2)
    end
  ----------------------------------------------------------------------------
  elseif prefix=="edit" or prefix=="view" or prefix=="load" or prefix=="unload" or prefix=="goto" then
    farcmds.Command(prefix, text)
  ----------------------------------------------------------------------------
  else
    local item = utils.GetPrefixes()[prefix]
    if item then return item.action(prefix, text) end
  end
end

local function PanelModuleExist(mod)
  for _,module in ipairs(utils.GetPanelModules()) do
    if mod == module then return true; end
  end
end

local function OpenLuaMacro (calltype, ...)
  if     calltype==F.MCT_KEYMACRO       then return keymacro.Dispatch(...)
  elseif calltype==F.MCT_MACROPARSE     then return MacroParse(...)
  elseif calltype==F.MCT_DELMACRO       then return utils.DelMacro(...)
  elseif calltype==F.MCT_ENUMMACROS     then return utils.EnumMacros(...)
  elseif calltype==F.MCT_GETMACRO       then return utils.GetMacroWrapper(...)
  elseif calltype==F.MCT_LOADMACROS     then
    local InitedRAM,Paths = ...
    keymacro.InitInternalVars(InitedRAM)
    return utils.LoadMacros(false,Paths)
  elseif calltype==F.MCT_RECORDEDMACRO  then return utils.ProcessRecordedMacro(...)
  elseif calltype==F.MCT_RUNSTARTMACRO  then return utils.RunStartMacro()
  elseif calltype==F.MCT_WRITEMACROS    then return utils.WriteMacros()
  elseif calltype==F.MCT_EXECSTRING     then return ExecString(...)
  elseif calltype==F.MCT_ADDMACRO       then return utils.AddMacroFromFAR(...)
  elseif calltype==F.MCT_PANELSORT      then
    if panelsort then
      TablePanelSort = { panelsort.SortPanelItems(...) }
      if TablePanelSort[1] then return TablePanelSort end
    end
  elseif calltype==F.MCT_GETCUSTOMSORTMODES then
    if panelsort then
      TablePanelSort = panelsort.GetSortModes()
      return TablePanelSort
    end
  elseif calltype==F.MCT_CANPANELSORT then
    return panelsort and panelsort.CanDoPanelSort(...)
  end
end

local CanCreatePanel = {
  [F.OPEN_DISKMENU]      = true;
  [F.OPEN_FINDLIST]      = true;
  [F.OPEN_SHORTCUT]      = true;
--[F.OPEN_FILEPANEL]     = true; -- does it needed?
  [F.OPEN_PLUGINSMENU]   = true;
}

local function OpenCommandLine (CmdLine)
  local mod, obj = Open_CommandLine(CmdLine)
  return mod and obj and PanelModuleExist(mod) and { module=mod; object=obj }
end

local function OpenShortcut (Item)
  if Item then
    local mod_guid, data = Item:match(
      "^(%x%x%x%x%x%x%x%x%-%x%x%x%x%-%x%x%x%x%-%x%x%x%x%-%x%x%x%x%x%x%x%x%x%x%x%x)/(.*)")
    if mod_guid then
      local mod = utils.GetPanelModules()[win.Uuid(mod_guid)]
      if mod and type(mod.OpenShortcut) == "function" then
        local obj = mod.OpenShortcut(Item)
        return obj and { module=mod; object=obj }
      end
    end
  end
end

local function OpenFromMacro (argtable)
  if argtable[1]=="argtest" then -- argtest: return received arguments
    return unpack(argtable, 2, argtable.n)
  elseif argtable[1]=="macropost" then -- test Mantis # 2222
    return far.MacroPost([[far.Message"macropost"]])
  elseif argtable[1]=="browser" then
    macrobrowser()
  end
end

function export.Open (OpenFrom, Id, ...)
  if OpenFrom == F.OPEN_LUAMACRO then
    return OpenLuaMacro(Id, ...)

  elseif OpenFrom == F.OPEN_FROMMACRO then
    return OpenFromMacro(...)

  elseif OpenFrom == F.OPEN_SHORTCUT then
    return OpenShortcut(...)

  elseif OpenFrom == F.OPEN_COMMANDLINE then
    return OpenCommandLine(...)

  elseif OpenFrom == F.OPEN_FINDLIST then
    for _,mod in ipairs(utils.GetPanelModules()) do
      if type(mod.Open) == "function" then
        local obj = mod.Open(OpenFrom, Id, ...)
        if obj then return { module=mod; object=obj } end
      end
    end

  else
    local items = utils.GetMenuItems()
    if Id == 0 then
      macrobrowser()
    elseif type(Id) == "number" then
      local guid = PluginInfo[OpenFrom==F.OPEN_DISKMENU and "DiskMenuGuids" or "PluginMenuGuids"][Id+1]
      if guid and items[guid] then
        local mod, obj = items[guid].action(OpenFrom, ...)
        if CanCreatePanel[OpenFrom] and mod and obj and PanelModuleExist(mod) then
          return { module=mod; object=obj }
        end
      end
    else
      macrobrowser()
    end

  end
end

-- TODO: when called from a module's panel, call that module's Configure()
function export.Configure (Item)
  local items = utils.GetMenuItems()
  local guid = PluginInfo.PluginConfigGuids[Item+1]
  if guid and items[guid] then
    return items[guid].action()
  end
end

local function ReadIniFile (filename)
  local fp = io.open(filename)
  if not fp then return nil end

  local currsect = 1
  local t = { [currsect]={} }
  local numline = 0

  if fp:read(3) ~= "\239\187\191" then fp:seek("set",0) end -- skip UTF-8 BOM
  for line in fp:lines() do
    numline = numline + 1
    local sect = line:match("^%s*%[([^%]]+)%]%s*$")
    if sect then
      t[sect] = t[sect] or {}
      currsect = sect
    else
      local id,val = line:match("^%s*(%w+)%s*=%s*(.-)%s*$")
      if id then
        t[currsect][id] = val
      elseif not (line:match("^%s*;") or line:match("^%s*$")) then
        fp:close()
        return nil, (("%s:%d: invalid line in ini-file"):format(filename,numline))
      end
    end
  end
  fp:close()
  return t
end

local function GetMacroDirs()
  local mainpath, loadpathlist
  local cfg, msg = ReadIniFile(far.PluginStartupInfo().ShareDir.."/luamacro.ini")
  if cfg then
    local sect = cfg["General"]
    if sect then
      mainpath = sect["MainPath"]
      loadpathlist = sect["LoadPathList"]
    end
  else
    if msg then ErrMsg(msg) end
  end

  local dirs = {}
  dirs.MainPath = mainpath and ExpandEnv(mainpath) or far.InMyConfig("Macros")
  dirs.LoadPathList = loadpathlist and ExpandEnv(loadpathlist) or ""
  return dirs
end

local function Init()
  Shared = {
    checkarg          = checkarg,
    ErrMsg            = ErrMsg,
    FullExpand        = FullExpand,
    GetLastParseError = GetLastParseError,
    loadmacro         = loadmacro,
    MacroDirs         = GetMacroDirs(),
    MacroStep         = MacroStep,
    pack              = pack,
    Unquote           = Unquote,
    yieldcall         = yieldcall,
  }
  Shared.MacroCallFar, far.MacroCallFar = far.MacroCallFar, nil
  Shared.MacroCallToLua, far.MacroCallToLua = far.MacroCallToLua, nil

  local ShareDirSlash = far.PluginStartupInfo().ShareDir .. "/"
  local function RunPluginFile (fname, param)
    local func = assert(loadfile(ShareDirSlash..fname))
    return func(param)
  end

  Msg = RunPluginFile("lang.lua");
  Shared.Msg = Msg

  Shared.Settings = RunPluginFile("settings.lua")
  Shared.OpCodes = RunPluginFile("opcodes.lua")

  utils = RunPluginFile("utils.lua", Shared)
  Shared.utils = utils

  RunPluginFile("api.lua", Shared)
  mf.postmacro = postmacro
  mf.acall = function(f, ...)
    checkarg(f, 1, "function")
    return yieldcall("acall", f, ...)
  end

  keymacro = RunPluginFile("keymacro.lua", Shared)
  Shared.keymacro = keymacro
  mf.mmode, _G.mmode = keymacro.mmode, keymacro.mmode
  mf.akey, _G.akey = keymacro.akey, keymacro.akey
  mf.AddExitHandler = keymacro.AddExitHandler

  farcmds = RunPluginFile("farcmds.lua", Shared)
  macrobrowser = RunPluginFile("mbrowser.lua", Shared)

  do
    pcall(require, "moonscript")
  end

  if bit and jit then
    RunPluginFile("winapi.lua")
    RunPluginFile("farapi.lua")

    panelsort = RunPluginFile("panelsort.lua", Shared)
    Shared.panelsort = panelsort
    Panel.LoadCustomSortMode = panelsort.LoadCustomSortMode
    Panel.SetCustomSortMode = panelsort.SetCustomSortMode
    Panel.CustomSortMenu = panelsort.CustomSortMenu
  end

  utils.FixInitialModules()
  utils.InitMacroSystem()
  local mainpath = Shared.MacroDirs.MainPath
  local modules = mainpath .. "/modules"
  package.path = ("%s/?.lua;%s/?/init.lua;%s"):format(modules, modules, package.path)
  if package.moonpath then
    package.moonpath = ("%s/?.moon;%s/?/init.moon;%s"):format(modules, modules, package.moonpath)
  end
  package.cpath = mainpath..(win.IsProcess64bit() and "/lib64" or "/lib32").."/?.so;"..package.cpath
end

function export.OpenFilePlugin (Name, Data, OpMode)
  for _,module in ipairs(utils.GetPanelModules()) do
    if type(module.OpenFilePlugin) == "function" then
      local obj = module.OpenFilePlugin(Name, Data, OpMode)
      if obj then
        return { module=module; object=obj }
      end
    end
  end
end

function export.GetOpenPanelInfo (wrapped_obj, handle, ...)
  local mod, obj = wrapped_obj.module, wrapped_obj.object
  if type(mod.GetOpenPanelInfo) == "function" then
    local op_info = mod.GetOpenPanelInfo(obj, handle, ...)
    if type(op_info) == "table" then
      if type(op_info.ShortcutData) == "string"
         and type(mod.Info) == "table"
         and type(mod.Info.Guid) == "string"
      then
        op_info._ModuleShortcutData = win.Uuid(mod.Info.Guid) .. "/" .. op_info.ShortcutData
      end
      return op_info
    end
  end
end

function export.MakeDirectory (wrapped_obj, ...)
  local func = wrapped_obj.module.MakeDirectory
  if type(func) == "function" then return func(wrapped_obj.object, ...)
  else return 1, "" -- suppress Far error message
  end
end

for _,name in ipairs {"ClosePanel","Compare","DeleteFiles","GetFiles","GetFindData","GetLinkTarget",
      "ProcessHostFile","ProcessPanelEvent","ProcessKey","PutFiles","SetDirectory",
      "SetFindList"} do
  export[name] =
    function(wrapped_obj, ...)
      local func = wrapped_obj.module[name]
      if type(func) == "function" then
        return func(wrapped_obj.object, ...)
      end
    end
end

local ok, msg = pcall(Init) -- pcall is used to handle RunPluginFile() failure in one place only
if not ok then
  export=nil; ErrMsg(msg)
end
