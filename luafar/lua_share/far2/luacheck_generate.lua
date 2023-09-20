-- Started: 2023-07-31
-- Goal: generate "luacheck_config.lua" for using with LuaCheck
-- Run Far w/out macros (far /m) to avoid contamination of globals then run this file from LuaMacro

assert(type(_G.mf) == "table", "Must be run by plugin LuaMacro")

local osWindows = string.sub(package.config, 1, 1) == "\\"

local exports = osWindows and [[
  "Analyse", "ClosePanel", "Compare", "Configure", "DeleteFiles", "ExitFAR",
  "GetContentData", "GetContentFields", "GetFiles", "GetFindData",
  "GetGlobalInfo", "GetOpenPanelInfo", "GetPluginInfo", "MakeDirectory", "OnError", "Open",
  "ProcessConsoleInput", "ProcessDialogEvent", "ProcessEditorEvent", "ProcessEditorInput",
  "ProcessHostFile", "ProcessPanelEvent", "ProcessPanelInput", "ProcessSynchroEvent",
  "ProcessViewerEvent", "PutFiles", "SetDirectory", "SetFindList", "SetStartupInfo",
]] or [[
  "ClosePanel", "Compare", "Configure", "DeleteFiles", "ExitFAR", "GetCustomData",
  "GetFiles", "GetFindData", "GetOpenPanelInfo", "GetPluginInfo", "GetVirtualFindData",
  "MakeDirectory", "MayExitFAR", "OnError", "Open", "OpenFilePlugin", "ProcessConsoleInput",
  "ProcessDialogEvent", "ProcessEditorEvent", "ProcessEditorInput", "ProcessPanelEvent",
  "ProcessHostFile", "ProcessKey", "ProcessViewerEvent", "PutFiles", "SetDirectory",
  "SetFindList",
]]

local globals_luafar = osWindows and
  { "bit64", "editor", "far", "_luaplug", "panel", "regex", "utf8", "viewer", "win", }
  or
  { "actl", "bit64", "editor", "far", "_luaplug", "panel", "regex", "utf8", "viewer", "win", }

local globals_luamacro = {
  "Keys", "akey", "band", "bnot", "bor", "bxor", "eval", "exit", "lshift", "mmode", "msgbox",
  "print", "prompt", "rshift",
  "Macro",  "Event",  "MenuItem",  "CommandLine",  "PanelModule",  "ContentColumns",
  "NoMacro","NoEvent","NoMenuItem","NoCommandLine","NoPanelModule","NoContentColumns",
  "_filename",
  "APanel", "Area", "BM", "CmdLine", "Dlg", "Drv", "Editor", "Far", "Help", "Menu", "Mouse",
  "Object", "PPanel", "Panel", "Plugin", "Viewer", "mf",
}

local function NoCaseCmp(a,b) return a:lower() < b:lower() end

local Obj = {}
local Obj_meta = { __index=Obj }

local function NewObj()
  return setmetatable( { Tt={} }, Obj_meta )
end

function Obj:Insert(s)
  table.insert(self.Tt, s)
end

function Obj:ProcessTable (Tbl, Name, Level)
  Level = (Level or 0) + 1
  self:Insert( ('\n[%-10s] = {'):format('"'..Name..'"') )
  self:Insert("fields = {")
  local arr = {} -- used for sorting (good for visual compare)
  for k in pairs(Tbl) do arr[#arr+1]=k end
  table.sort(arr, NoCaseCmp)
  for _,v in ipairs(arr) do
    if type(Tbl[v])=="table" then
      if not (Level==1 and v=="properties") then
        self:ProcessTable(Tbl[v],v,Level)
      end
    else
      self:Insert( ('"%s";'):format(v) )
    end
  end
  if Level==1 and type(Tbl.properties) == "table" then
    local props = Tbl.properties
    arr = {}
    for k in pairs(props) do arr[#arr+1]=k end
    table.sort(arr, NoCaseCmp)
    for _,v in ipairs(arr) do
      if type(v)=="table" then self:ProcessTable(props[v],v,Level)
      else self:Insert( ('"%s";'):format(v) )
      end
    end
  end
  self:Insert("};")
  self:Insert("};")
end

local function Generate (outname)
  assert(outname, "output file not specified")
  local fp = assert( io.open(outname, "w") )

  local self = NewObj()
  self:Insert("-- This file was automatically generated\n")
  self:Insert("local luafar = {")
  self:Insert("globals = {")
  self:Insert("export = {")
  self:Insert("fields = {")
  self:Insert(exports)
  self:Insert("};")
  self:Insert("};")
  self:Insert("};")

  self:Insert("read_globals = {")
  table.sort(globals_luafar, NoCaseCmp)
  for _,name in ipairs(globals_luafar) do
    if type(_G[name])=="table" then self:ProcessTable(_G[name], name)
    else self:Insert( ('"%s";'):format(name) )
    end
  end
  self:Insert("};")
  self:Insert("};")

  self:Insert("\n\n")
  self:Insert("local luamacro = {")
  self:Insert("read_globals = {")
  table.sort(globals_luamacro, NoCaseCmp)
  for _,name in ipairs(globals_luamacro) do
    if type(_G[name])=="table" then self:ProcessTable(_G[name], name)
    else self:Insert( ('"%s";'):format(name) )
    end
  end
  self:Insert("};")
  self:Insert("};")
  self:Insert("\n\n")
  self:Insert("return { luafar=luafar; luamacro=luamacro; }")
  local str = table.concat(self.Tt)
  fp:write(str, "\n")
  fp:close()
end

return Generate
