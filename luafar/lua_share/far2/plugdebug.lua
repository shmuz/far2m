-- plugdebug module
-- luacheck: new_globals Old_export

local F = far.Flags

local PanelEvents = { [0]="FE_CHANGEVIEWMODE","FE_REDRAW","FE_IDLE","FE_CLOSE",
  "FE_BREAK","FE_COMMAND","FE_GOTFOCUS","FE_KILLFOCUS","FE_CHANGESORTPARAMS" }

local OpenFrom = {	[0]="OPEN_LEFTDISKMENU","OPEN_PLUGINSMENU","OPEN_FINDLIST","OPEN_SHORTCUT",
  "OPEN_COMMANDLINE","OPEN_EDITOR","OPEN_VIEWER","OPEN_FILEPANEL","OPEN_DIALOG","OPEN_ANALYSE",
  "OPEN_RIGHTDISKMENU","OPEN_FROMMACRO",[100]="OPEN_LUAMACRO" }

-- local InputEvents = {
--   [F.KEY_EVENT               ] = "KEY_EVENT";
--   [F.MOUSE_EVENT             ] = "MOUSE_EVENT";
--   [F.WINDOW_BUFFER_SIZE_EVENT] = "WINDOW_BUFFER_SIZE_EVENT";
--   [F.MENU_EVENT              ] = "MENU_EVENT";
--   [F.FOCUS_EVENT             ] = "FOCUS_EVENT";
-- }

local OpModes = {
  [F.OPM_SILENT   ] = "OPM_SILENT";
  [F.OPM_FIND     ] = "OPM_FIND";
  [F.OPM_VIEW     ] = "OPM_VIEW";
  [F.OPM_QUICKVIEW] = "OPM_QUICKVIEW";
  [F.OPM_EDIT     ] = "OPM_EDIT";
  [F.OPM_DESCR    ] = "OPM_DESCR";
  [F.OPM_TOPLEVEL ] = "OPM_TOPLEVEL";
  [F.OPM_PGDN     ] = "OPM_PGDN";
  [F.OPM_COMMANDS ] = "OPM_COMMANDS";
  [0x00           ] = "OPM_NONE";
}


local function quote(var)
  return type(var)=="string" and "'"..var.."'" or tostring(var)
end

local function Inject(_, name)
  local func = rawget(Old_export, name) -- here rawget is important, otherwise any Lua error would crash Far
  if not func then return end
  return function(...)
    local txt = name
    --------------------------------------------------------------------------------
    if name=="Analyse" then
      local Info = ...
      txt = ("%s (%s)"):format(name, quote(Info.FileName))
    --------------------------------------------------------------------------------
    elseif name=="GetFindData" then
      local OpMode = select(3, ...)
      txt = ("%s (%s)"):format(name, OpModes[OpMode] or OpMode)
    --------------------------------------------------------------------------------
    elseif name=="Open" then
      local from, _, item = ...
      from = OpenFrom[from]
      ------------------------------------------------------------------------------
      if from == "OPEN_FROMMACRO" then
        local t = { from }
        for k=1, item.n do t[k+1]=quote(item[k]) end
        txt = ("%s (%s)"):format(name, table.concat(t,", "))
      ------------------------------------------------------------------------------
      elseif from == "OPEN_COMMANDLINE" then
        txt = ("%s (%s, '%s')"):format(name, from, item)
      ------------------------------------------------------------------------------
      elseif from == "OPEN_SHORTCUT" then
        local sFlags = item.Flags==0 and "FOSF_NONE" or item.Flags==1 and "FOSF_ACTIVE" or item.Flags
        txt = ("%s (%s, '%s', %s, %s)"):format(name,from,item.HostFile,quote(item.ShortcutData),sFlags)
      ------------------------------------------------------------------------------
      else
        txt = ("%s (%s)"):format(name, from)
      ------------------------------------------------------------------------------
      end
    --------------------------------------------------------------------------------
    elseif name=="ProcessEvent" then
      local Event, Param = select(3, ...)
      if Event==F.FE_IDLE then -- don't show this event
        txt = nil
      else
        txt = ("%s (%s, %s)"):format(name, PanelEvents[Event], quote(Param))
      end
    --------------------------------------------------------------------------------
    elseif name=="ProcessKey" then
      local key, ctrlstate = select(3, ...)
      txt = ("%s (key=0x%X, ctrlstate=0x%X)"):format(name, key, ctrlstate)
    --------------------------------------------------------------------------------
    elseif name=="SetDirectory" then
      local Dir, OpMode = select(3, ...)
      txt = ("%s ('%s', %s)"):format(name, Dir, OpModes[OpMode] or OpMode)
    --------------------------------------------------------------------------------
    end
    if txt then
      far.Log(txt)
    end
    return func(...)
  end
end

local function Running()
  return Old_export and true
end

local function Start()
  -- If table 'export' contains real elements then replace it and add injections.
  -- Else do nothing.
  if next(export) then
    -- Keep Old_export as a global variable to withstand reloading this module.
    Old_export, export = export, {}
    setmetatable(Old_export, nil)
    setmetatable(export, { __index=Inject; })
  end
end

local function Stop()
  if not next(export) then
    export, Old_export = Old_export, nil
    setmetatable(export, nil)
  end
end

return {
  Running = Running;
  Start = Start;
  Stop = Stop;
}
