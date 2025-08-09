-- coding: utf-8

local Shared = ...
local mc = Shared.Constants
local Msg, ErrMsg, pack = Shared.Msg, Shared.ErrMsg, Shared.pack
local MacroDirs = Shared.MacroDirs

local F = far.Flags
local type = type
local string_find, string_sub = string.find, string.sub
local band, bor = bit64.band, bit64.bor
local JoinPath = win.JoinPath
local MacroCallFar = Shared.MacroCallFar
local gmeta = { __index=_G }
local LastMessage = {}
local LoadCounter = 0
--------------------------------------------------------------------------------
local TrueAreaNames = {
  [F.MACROAREA_OTHER]                = "Other",
  [F.MACROAREA_SHELL]                = "Shell",
  [F.MACROAREA_VIEWER]               = "Viewer",
  [F.MACROAREA_EDITOR]               = "Editor",
  [F.MACROAREA_DIALOG]               = "Dialog",
  [F.MACROAREA_SEARCH]               = "Search",
  [F.MACROAREA_DISKS]                = "Disks",
  [F.MACROAREA_MAINMENU]             = "MainMenu",
  [F.MACROAREA_MENU]                 = "Menu",
  [F.MACROAREA_HELP]                 = "Help",
  [F.MACROAREA_INFOPANEL]            = "Info",
  [F.MACROAREA_QVIEWPANEL]           = "QView",
  [F.MACROAREA_TREEPANEL]            = "Tree",
  [F.MACROAREA_FINDFOLDER]           = "FindFolder",
  [F.MACROAREA_USERMENU]             = "UserMenu",
  [F.MACROAREA_SHELLAUTOCOMPLETION]  = "ShellAutoCompletion",
  [F.MACROAREA_DIALOGAUTOCOMPLETION] = "DialogAutoCompletion",
  [F.MACROAREA_GRABBER]              = "Grabber",
--~   [F.MACROAREA_DESKTOP]              = "Desktop",
  [F.MACROAREA_COMMON]               = "Common",
}

local AllAreaNames = {}
for k,v in pairs(TrueAreaNames) do
  local str = v:lower()
  AllAreaNames[k] = str
  AllAreaNames[str] = k
end

local SomeAreaNames = {
  "other", "viewer", "editor", "dialog", "menu", "help", "dialogautocompletion",
  "grabber", "desktop", "common" -- "common" должен идти последним
}

local function GetTrueAreaName(Mode) return TrueAreaNames[Mode] end
local function GetAreaName(Mode)     return AllAreaNames[Mode] end
local function GetAreaCode(Area)     return AllAreaNames[Area:lower()] end
--------------------------------------------------------------------------------

local MCODE_F_CHECKALL     = mc.MCODE_F_CHECKALL
local MCODE_F_MACROSETTINGS = mc.MCODE_F_MACROSETTINGS

local ReadOnlyConfig = false

local Areas
local LoadedMacros
local LoadMacrosDone
local LoadingInProgress
local EnumState = {}
local Events
local EventGroups = {
  "consoleinput", "dialogevent", "editorevent", "editorinput", "exitfar", "mayexitfar",
  "synchroevent", "viewerevent",
}
local AddedMenuItems
local AddedPrefixes
local IdSet
local LoadedPanelModules
local ContentColumns
local KeyReplaceTable

package.nounload = {lpeg=true}
local initial_modules = {}

local function FixInitialModules()
  for k in pairs(package.loaded) do initial_modules[k]=true end
end

local function CheckFileName (mask, name)
  return far.ProcessName(F.PN_CMPNAMELIST, mask, name, F.PN_SKIPPATH)
end

local StringToFlags, FlagsToString do
  local MacroFlagsByInt = {
    [ mc.MFLAGS_ENABLEOUTPUT        ] = "EnableOutput",
    [ mc.MFLAGS_NOSENDKEYSTOPLUGINS ] = "NoSendKeysToPlugins",
    [ mc.MFLAGS_RUNAFTERFARSTART    ] = "RunAfterFARStart",
    [ mc.MFLAGS_EMPTYCOMMANDLINE    ] = "EmptyCommandLine",
    [ mc.MFLAGS_NOTEMPTYCOMMANDLINE ] = "NotEmptyCommandLine",
    [ mc.MFLAGS_EDITSELECTION       ] = "EVSelection",
    [ mc.MFLAGS_EDITNOSELECTION     ] = "NoEVSelection",
    [ mc.MFLAGS_SELECTION           ] = "Selection",
    [ mc.MFLAGS_PSELECTION          ] = "PSelection",
    [ mc.MFLAGS_NOSELECTION         ] = "NoSelection",
    [ mc.MFLAGS_PNOSELECTION        ] = "NoPSelection",
    [ mc.MFLAGS_NOFILEPANELS        ] = "NoFilePanels",
    [ mc.MFLAGS_PNOFILEPANELS       ] = "NoFilePPanels",
    [ mc.MFLAGS_NOPLUGINPANELS      ] = "NoPluginPanels",
    [ mc.MFLAGS_PNOPLUGINPANELS     ] = "NoPluginPPanels",
    [ mc.MFLAGS_NOFOLDERS           ] = "NoFolders",
    [ mc.MFLAGS_PNOFOLDERS          ] = "NoPFolders",
    [ mc.MFLAGS_NOFILES             ] = "NoFiles",
    [ mc.MFLAGS_PNOFILES            ] = "NoPFiles",
  }
  local MacroFlagsByStr={}
  for k,v in pairs(MacroFlagsByInt) do MacroFlagsByStr[v:lower()]=k end

  function StringToFlags (str, filename)
    local flags = 0
    if type(str) == "string" then
      for word in str:gmatch("[^ |]+") do
        local f = MacroFlagsByStr[word:lower()]
        if f then
          flags = bor(flags, f)
        else
          local line1 = filename or "<not a file>"
          local btn = filename and "OK;Edit" or "OK"
          if 2 == ErrMsg(line1.."\nInvalid macro flag: "..word, nil, btn) then
            editor.Editor(filename,nil,nil,nil,nil,nil,nil,1,nil,65001)
          end
        end
      end
    end
    return flags
  end

  -- assume 52 bits at most
  function FlagsToString (flags)
    local str, bit = "", 1
    while flags >= bit do
      if band(flags,bit) ~= 0 then
        local s = MacroFlagsByInt[bit]
        if s then
          if str ~= "" then str = str.." " end
          str = str..s
        end
      end
      bit = bit * 2
    end
    return str
  end
end

local function AddId (trg, src)
  trg.id = "<no id>"
  if type(src.id)=="string" then
    local lstr = string.lower(src.id)
    if not IdSet[lstr] then
      IdSet[lstr] = true
      trg.id = src.id
    end
  end
end

local function EV_Handler (macros, filename, ...)
  -- Get current priorities.
  local indexes,priorities = {},{}
  for i,m in ipairs(macros) do
    indexes[i],priorities[i] = i, -1
    if (not m.filemask) or (filename and CheckFileName(m.filemask, filename)) then
      if m.condition then
        local pr = m.condition(...)
        if pr then
          if type(pr)=="number" then priorities[i] = pr<0 and 0 or pr>100 and 100 or pr
          else priorities[i] = m.priority
          end
        end
      else
        priorities[i] = m.priority
      end
    end
  end

  -- Sort by current priorities (stable sort).
  table.sort(indexes, function(i,j)
      return priorities[i]>priorities[j] or priorities[i]==priorities[j] and i<j
    end)

  -- Execute.
  for _,i in ipairs(indexes) do
    if priorities[i] < 0 then break end
    local ret = macros[i].action(...)
    if ret then
      if macros==Events.dialogevent or macros==Events.editorinput or macros==Events.synchroevent then
        return ret
      elseif macros==Events.consoleinput then
        if ret ~= 0 then return ret end
      end
    else
      if macros==Events.mayexitfar then return false end
    end
  end
  if macros==Events.mayexitfar then return true end
end

local function export_ProcessEditorEvent (EditorID, Event, Param)
  return EV_Handler(Events.editorevent, editor.GetFileName(nil), EditorID, Event, Param)
end

local function export_ProcessViewerEvent (ViewerID, Event, Param)
  return EV_Handler(Events.viewerevent, viewer.GetFileName(nil), ViewerID, Event, Param)
end

local function export_ExitFAR (unload)
  return EV_Handler(Events.exitfar, nil, not not unload)
end

local function export_MayExitFAR ()
  return EV_Handler(Events.mayexitfar)
end

local function export_ProcessDialogEvent (Event, Param)
  return EV_Handler(Events.dialogevent, nil, Event, Param)
end

local function export_ProcessEditorInput (Rec)
  return EV_Handler(Events.editorinput, editor.GetFileName(nil), Rec)
end

local function export_ProcessConsoleInput (Rec)
  return EV_Handler(Events.consoleinput, nil, Rec)
end

local function export_ProcessSynchroEvent (Event, Param)
  return EV_Handler(Events.synchroevent, nil, Event, Param)
end

local function export_GetContentFields (colnames)
  for _,m in ipairs(ContentColumns) do
    if m.GetContentFields(colnames) then return true end
  end
end

local function export_GetContentData (filename, colnames)
  local tOut = {}
  for _,m in ipairs(ContentColumns) do
    if m.filemask==nil or CheckFileName(m.filemask, filename) then
      local data = m.GetContentData(filename, colnames)
      if type(data) == "table" then
        for i in ipairs(colnames) do
          tOut[i] = tOut[i] or (type(data[i])=="string" and data[i])
        end
      end
    end
  end
  return tOut
end

local ExpandKey do -- измеренное время исполнения на ключе "CtrlAltShiftF12" = ??? (Lua); 2.3uS (LuaJIT);
  local t={}

  ExpandKey = function (key)
    local ctrl,alt,rest
    key = key:lower()
    local start = 1
    for _=1,3 do
      local from,to,word = string_find(key, "^([lr]?ctrl)", start)
      if from then ctrl = ctrl or word
      else
        from,to,word = string_find(key, "^([lr]?alt)", start)
        if from then alt = alt or word
        else
          from,to,word = string_find(key, "^(shift)", start)
          if from then rest = rest or word
          else break
          end
        end
      end
      start = to+1
    end
    rest = (rest or "")..string_sub(key, start)
    ctrl, alt = ctrl or "", alt or ""

    if ctrl=="ctrl" then
      if alt=="alt" then
        t[1] = "lctrllalt"..rest; t[2] = "lctrlralt"..rest
        t[3] = "rctrllalt"..rest; t[4] = "rctrlralt"..rest
        return t,4
      else
        t[1] = "lctrl"..alt..rest
        t[2] = "rctrl"..alt..rest
        return t,2
      end
    else
      if alt=="alt" then
        t[1] = ctrl.."lalt"..rest
        t[2] = ctrl.."ralt"..rest
        return t,2
      else
        t[1] = ctrl..alt..rest
        return t,1
      end
    end
  end
end

local function AddRegularMacro (srctable, FileName)
  if not (type(srctable)=="table" and type(srctable.area)=="string") then
    return
  end

  local macro = {}
  macro.area = srctable.area

  local key,id = nil,srctable.id
  if id then
    id = win.Uuid(id)
    key = id and KeyReplaceTable[id]
  end
  key = key or srctable.key
  macro.key = type(key)=="string" and key:find("%S") and key or "none"

  local keyregex = macro.key:match("^/(.+)/$")
  if keyregex then
    local ok
    ok, macro.keyregex = pcall(regex.new, "^("..keyregex..")$", "i")
    if not ok then
      ErrMsg("Invalid regex: "..macro.key)
      return
    end
  end

  if type(srctable.action)=="function" then
    macro.action = srctable.action
  elseif type(srctable.code)=="string" then
    local isMoonScript = srctable.language=="moonscript"
    if srctable.code:sub(1,1) == "@" then
      macro.code = srctable.code
      macro.language = isMoonScript and "moonscript" or "lua"
    else
      local f, msg = (isMoonScript and require("moonscript").loadstring or loadstring)(srctable.code)
      if f then
        macro.action = f
      else
        if FileName then ErrMsg(FileName..":\n"..msg, isMoonScript and "MoonScript"); end
        return
      end
    end
  else
    macro.action = function() end -- intended use: do all the things in condition()
  end

  local arFound = {} -- prevent multiple inclusions, i.e. area="Editor Editor"
  for a in srctable.area:lower():gmatch("%S+") do
    local arTable = Areas[a]
    if arTable and not arFound[a] then
      if macro.keyregex then
        arTable[1] = arTable[1] or {}
        table.insert(arTable[1], macro)
      else
        local keyFound = {} -- prevent multiple inclusions
        for k in macro.key:lower():gmatch("%S+") do
          local t,n = ExpandKey(k)
          for i=1,n do
            local normkey = t[i]
            if not keyFound[normkey] then
              arTable[normkey] = arTable[normkey] or {}
              table.insert(arTable[normkey], macro)
              keyFound[normkey] = true
            end
          end
        end
      end
      arFound[a] = true
    end
  end

  if next(arFound) then
    macro.flags = StringToFlags(srctable.flags, FileName)

    if type(srctable.description)=="string" then macro.description=srctable.description end
    if type(srctable.condition)=="function" then macro.condition=srctable.condition end
    if type(srctable.filemask)=="string" then macro.filemask=srctable.filemask end

    local priority = srctable.priority
    if type(priority)=="number" then
      macro.priority = priority>100 and 100 or priority<0 and 0 or priority
    end
    priority = srctable.sortpriority
    if type(priority)=="number" then
      macro.sortpriority = priority>100 and 100 or priority<0 and 0 or priority
    end
    macro.selected = srctable.selected and true
    AddId(macro, srctable)

    if FileName then
      macro.FileName = FileName
    else
      macro.guid = srctable.guid
      macro.callback = srctable.callback
      macro.callbackId = srctable.callbackId
      macro.language = srctable.language
    end

    macro.data = {}
    for k,v in pairs(srctable) do macro.data[k]=v; end
    macro.index = #LoadedMacros+1
    LoadedMacros[macro.index] = macro
    return macro
  end
end

local CharNames = { ["."]="Dot", ["<"]="Less", [">"]="More", ["|"]="Pipe", ["/"]="Slash",
                    [":"]="Colon", ["?"]="Question", ["*"]="Asterisk", ['"']="Quote" }

local function AddRecordedMacro (srctable, filename)
  local area = srctable.area:lower()
  if not (area and Areas[area]) then return end
  local arTable = Areas[area]

  local key = srctable.key
  -- check correspondence between (a) filename and (b) area_key
  if ("%s_%s"):format(area, (key:gsub(".",CharNames))):lower() ~=
     filename:gsub("^.*/",""):sub(1,-5):lower() then return
  end

  if srctable.code and srctable.code:sub(1,1) ~= "@" then
    local f, msg = loadstring(srctable.code)
    if not f then ErrMsg(msg) return end
  end

  local macro = { FileName=filename }
  local t,n = ExpandKey(key)
  for i=1,n do
    local normkey = t[i]
    arTable[normkey] = arTable[normkey] or {}
    arTable[normkey].recorded = macro
  end

  for _,v in ipairs{"area","key","action","code","description"} do macro[v]=srctable[v] end

  macro.flags = StringToFlags(srctable.flags, filename)
  if type(macro.description)~="string" then macro.description=nil end

  macro.index = #LoadedMacros+1
  LoadedMacros[macro.index] = macro
end

local AddEvent_fields = {"group","action","description","priority","condition","filemask"}
local function AddEvent (srctable, FileName)
  local group = type(srctable)=="table" and type(srctable.group)=="string" and srctable.group:lower()
  if not (group and Events[group]) then return end

  if type(srctable.action)~="function" then return end

  local macro={}
  table.insert(Events[group], macro)

  for _,v in ipairs(AddEvent_fields) do macro[v]=srctable[v] end
  macro.FileName = FileName

  if type(macro.description)~="string" then macro.description=nil end
  if type(macro.condition)~="function" then macro.condition=nil end
  if type(macro.filemask)~="string" then macro.filemask=nil end

  if type(macro.priority)~="number" then macro.priority=50
  elseif macro.priority>100 then macro.priority=100 elseif macro.priority<0 then macro.priority=0
  end
  AddId(macro, srctable)

  macro.index = #LoadedMacros+1
  LoadedMacros[macro.index] = macro
  return macro
end

local function AddMenuItem (srctable, FileName)
  if type(srctable)=="table" and
     type(srctable.menu)=="string" and
     type(srctable.guid)=="string" and
     (type(srctable.text)=="function" or type(srctable.text)=="string") and
     type(srctable.action)=="function"
  then
    local item = {}
    item.guid = win.Uuid(srctable.guid)
    if item.guid and #item.guid==16 and not AddedMenuItems[item.guid] then
      item.flags = {}
      for w in srctable.menu:lower():gmatch("%S+") do
        if w=="plugins" or w=="disks" or w=="config" then item.flags[w]=true end
      end
      if type(srctable.area)=="string" then
        for w in srctable.area:lower():gmatch("%S+") do
          if w == "common" then
            item.flags[w]=true
          else
            local code = GetAreaCode(w)
            if code then item.flags[code]=true end
          end
        end
      end
      local text = srctable.text
      item.text = type(text)=="function" and text or function() return text end
      item.action = srctable.action
      item.description = type(srctable.description)=="string" and srctable.description or ""
      item.FileName = FileName
      item.index = #AddedMenuItems + 1
      AddedMenuItems[item.index] = item
      AddedMenuItems[item.guid] = item
      return true
    end
  end
  return false
end

local function AddPrefixes (srctable, FileName)
  local result = 0
  if type(srctable)=="table" and
     type(srctable.prefixes)=="string" and
     type(srctable.action)=="function"
  then
    for prefix in srctable.prefixes:lower():gmatch("[^:]+") do
      if prefix:match("^%S+$") and not AddedPrefixes[prefix] then
        local item = {
          prefix = prefix,
          action = srctable.action,
          description = type(srctable.description)=="string" and srctable.description or "",
          FileName = FileName
        }
        AddedPrefixes[prefix] = item
        AddedPrefixes[1] = AddedPrefixes[1]..":"..prefix
        result = result + 1
      end
    end
  end
  return result
end

local function AddPanelModule (srctable, FileName)
  if  type(srctable) == "table" and type(srctable.Info) == "table" then
    local guid = srctable.Info.Guid
    if type(guid) == "string" and #guid == 16 then
      if not LoadedPanelModules[guid] then
        if FileName then srctable.FileName=FileName; end
        LoadedPanelModules[guid] = srctable
        table.insert(LoadedPanelModules, srctable)
      end
    end
  end
end

local function AddTestPanel()
  local path = win.JoinPath(Shared.ShareDir, "testpanel.lua")
  local func = loadfile(path)
  if func then
    local mod = func()
    AddPanelModule(mod, path)
    Shared.TestPanel = mod
  end
end

local function AddContentColumns (srctable, FileName)
  if    type(srctable) == "table"
    and type(srctable.GetContentFields) == "function"
    and type(srctable.GetContentData) == "function"
  then
     if type(srctable.filemask)~="string" then srctable.filemask=nil; end
     if type(srctable.description)~="string" then srctable.description=nil; end
     if FileName then srctable.FileName=FileName; end
     table.insert(ContentColumns, srctable)
  end
end

local function EnumMacros (strArea, resetEnum)
  local area = strArea:lower()
  if Areas[area] then
    if EnumState.area ~= area or resetEnum then
      EnumState.area, EnumState.index = area, 0
    end
    while true do
      EnumState.index = EnumState.index + 1
      local macro = LoadedMacros[EnumState.index]
      if macro then
        if not macro.disabled and macro.area and macro.area:lower():find(area) then
          LastMessage = pack(macro.key, macro.description or "")
          return LastMessage
        end
      else
        EnumState.index = 0
        break
      end
    end
  end
end

local function GetMoonscriptLineNumber (filename, line)
  local line_tables = require("moonscript.line_tables")
  local errors = require("moonscript.errors")
  local cache = {}
  local table = line_tables["@"..filename]
  if table then
    return errors.reverse_line_number(filename, table, line, cache)
  end
end

-- ...nager\unicode_far\CommonProfile\Macros\scripts\test1.lua:9:
-- attempt to perform arithmetic on a nil value

local function ErrMsgLoad (msg, filename, isMoonScript, mode)
  local title = isMoonScript and mode=="compile" and "MoonScript" or "LuaMacro"

  if type(msg)~="string" and type(msg)~="number" then
    ErrMsg(filename..":\n<non-string error message>", title, nil, "w")
    return
  end

  if mode=="run" then
    local fname, line, found
    fname = msg:match("^error loading module .- from file '([^\n]+)':")
    if fname then
      found = true
      line = tonumber(msg:match("^.-\n.-:(%d+):"))
    else
      fname,line = msg:match("^(.-):(%d+):")
      if fname then
        line = tonumber(line)
        if string_sub(fname,1,3) ~= "..." then
          found = true
        else
          fname = string_sub(fname,4)
          -- for k=1,5 do
          --   if fname:utf8valid() then break end
          --   fname = string_sub(fname,2)
          -- end
          local middle = fname:match("^[^/]*/[^/]+/")
          if middle then
            local from = string_find(filename:lower(), middle:lower(), 1, true)
            if from then
              fname = string_sub(filename,1,from-1) .. fname
              local attr = win.GetFileAttr(fname)
              found = attr and not attr:find("d")
            end
          end
        end
      end
    end
    if found then
      if 2 == ErrMsg(msg, title, "OK;Edit", "wl") then
        if isMoonScript then line = GetMoonscriptLineNumber(fname,line) end
        editor.Editor(fname,nil,nil,nil,nil,nil,nil,line or 1,nil,65001)
      end
    else
      ErrMsg(msg, title, "OK", "wl")
    end
  else
    if 2 == far.Message(msg, title, "OK;Edit", "wl") then
      local pattern = isMoonScript and "%[(%d+)%] >>" or "^[^\n]-:(%d+):"
      local line = tonumber(msg:match(pattern))
      if line and isMoonScript then line = GetMoonscriptLineNumber(filename,line) end
      editor.Editor(filename,nil,nil,nil,nil,nil,nil,line or 1,nil,65001)
    end
  end
end

-- Example: 908710AB-E08C-43A0-8B63-09F788E55BAC = 'F5 F8'
local function AddReplacingKeys(filename)
  local fp = io.open(filename)
  if fp then
    for ln in fp:lines() do
      local guid,key = ln:match("^%s*([^%s=]+)%s*=%s*(%S.-)%s*$")
      if guid then
        guid = win.Uuid( guid:match('^"(.+)"$') or guid:match("^'(.+)'$") or guid )
        if guid then
          KeyReplaceTable[guid] = key:match('^"(.+)"$') or key:match("^'(.+)'$") or key
        end
      end
    end
    fp:close()
  end
end

local function LoadMacros (unload, paths)
  if LoadingInProgress then return end
  LoadingInProgress = true

  if LoadMacrosDone then
    local ok, msg = xpcall(function() return export_ExitFAR(true) end,
                           function(msg) return debug.traceback(msg,2) end)
    if not ok then
      msg = string.gsub(msg, "\t", "   ")
      ErrMsg(msg)
    end
    LoadMacrosDone = false
  end

  export.ExitFAR = nil
  export.MayExitFAR = nil
  export.ProcessDialogEvent = nil
  export.ProcessEditorEvent = nil
  export.ProcessEditorInput = nil
  export.ProcessViewerEvent = nil
  export.ProcessConsoleInput = nil
  export.ProcessSynchroEvent = nil
  export.GetContentFields = nil
  export.GetContentData = nil

  local numerrors=0
  local newAreas = {}
  Events = {}
  EnumState = {}
  LoadedMacros = {}
  AddedMenuItems = {}
  AddedPrefixes = { [1]="" }
  IdSet = {}
  LoadedPanelModules = {}
  ContentColumns = {}
  KeyReplaceTable = {}
  if Shared.panelsort then Shared.panelsort.DeleteSortModes() end

  local AreaNames = panel.CheckPanelsExist() and AllAreaNames or SomeAreaNames
  for _,name in pairs(AreaNames) do newAreas[name]={} end
  for _,name in ipairs(EventGroups) do Events[name]={} end
  for k in pairs(package.loaded) do
    if initial_modules[k]==nil and not package.nounload[k] then
      package.loaded[k]=nil
    end
  end

  -- Copy macros loaded by MCTL_ADDMACRO to save them from destruction.
  if Areas then
    local IdUpdated = {}
    for a,areatable in pairs(Areas) do
      for k,macroarray in pairs(areatable) do
        for _,m in ipairs(macroarray) do
          if m.guid and not m.disabled then
            newAreas[a][k] = newAreas[a][k] or {}
            table.insert(newAreas[a][k], m)
            if not IdUpdated[m] then
              IdUpdated[m] = true
              m.index = #LoadedMacros+1
              LoadedMacros[m.index] = m
            end
          end
        end
      end
    end
  end
  Areas = newAreas

  AddTestPanel() -- add always as it is needed for testing

  if not unload then
    LoadCounter = LoadCounter + 1
    local DummyFunc = function() end
    if not ReadOnlyConfig then
      for _,v in ipairs {"scripts", "modules", "lib32", "lib64"} do
        win.CreateDir(JoinPath(MacroDirs.MainPath, v))
      end
      win.CreateDir(far.InMyConfig("Menus"))
    end

    local ok1, moonscript = pcall(require, "moonscript")
    if not ok1 then moonscript=nil; end

    local FuncList1 = {"Macro",  "Event",  "MenuItem",  "CommandLine",  "PanelModule",  "ContentColumns"}
    local FuncList2 = {"NoMacro","NoEvent","NoMenuItem","NoCommandLine","NoPanelModule","NoContentColumns"}

    local function LoadRegularFile (FindData, FullPath, macroinit)
      if FindData.FileAttributes:find("d") then return end
      if macroinit and #FullPath==#macroinit and far.LStricmp(FullPath,macroinit)==0 then
        return
      end
      local isMoonScript = string_find(FullPath, "[nN]", -1)
      local f, msg = (isMoonScript and moonscript.loadfile or loadfile)(FullPath)
      if not f then
        numerrors=numerrors+1
        ErrMsgLoad(msg,FullPath,isMoonScript,"compile")
        return
      end
      local env = {
        Macro          = function(t) return not not AddRegularMacro(t,FullPath) end;
        Event          = function(t) return not not AddEvent(t,FullPath) end;
        MenuItem       = function(t) return AddMenuItem(t,FullPath) end;
        CommandLine    = function(t) return AddPrefixes(t,FullPath) end;
        PanelModule    = function(t) return AddPanelModule(t,FullPath) end;
        ContentColumns = function(t) return AddContentColumns(t,FullPath) end;
      }
      for _,name in ipairs(FuncList2) do env[name]=DummyFunc; end
      setmetatable(env,gmeta)
      setfenv(f, env)
      local ok, msg = xpcall(function() return f(FullPath, LoadCounter) end, debug.traceback)
      if ok then
        for _,name in ipairs(FuncList1) do env[name]=nil; end
        for _,name in ipairs(FuncList2) do env[name]=nil; end
      else
        numerrors=numerrors+1
        msg = string.gsub(msg,"\n\t","\n   ")
        ErrMsgLoad(msg,FullPath,isMoonScript,"run")
      end
    end

    local tempRecordedMacro
    local function ReadRecordedMacro (m)
      if tempRecordedMacro == nil then
        if type(m) == "table" then
          local t_action, t_code = type(m.action), type(m.code)
          tempRecordedMacro = type(m.area)=="string" and type(m.key)=="string" and
            (t_action=="function" and t_code=="nil" or t_action=="nil" and t_code=="string") and m
        else
          tempRecordedMacro = false
        end
      end
    end

    local function LoadRecordedFile (FindData, FullPath)
      if FindData.FileAttributes:find("d") then return end
      local f, msg = loadfile(FullPath)
      if not f then
        numerrors=numerrors+1; ErrMsg(msg); return
      end
      local env = setmetatable({ Macro=ReadRecordedMacro }, gmeta)
      setfenv(f, env)
      tempRecordedMacro = nil
      local ok, msg = xpcall(f, debug.traceback)
      if ok then
        if tempRecordedMacro then
          env.Macro = nil
          AddRecordedMacro(tempRecordedMacro, FullPath)
        end
      else
        msg = msg:gsub("\n\t","\n   ")
        numerrors=numerrors+1; ErrMsg(msg)
      end
    end

    paths = paths and win.ExpandEnv(paths) or JoinPath(MacroDirs.MainPath,"scripts;")..MacroDirs.LoadPathList

    local filemask = moonscript and "*.lua,*.moon" or "*.lua"
    for p in paths:gmatch("[^;]+") do
      p = far.ConvertPath(p, F.CPM_FULL) -- needed for relative paths
      AddReplacingKeys(JoinPath(p, "_keyreplace.cfg"))
      local macroinit = JoinPath(p, "_macroinit.lua")
      local info = win.GetFileInfo(macroinit)
      if info and not info.FileAttributes:find("d") then
        LoadRegularFile(info, macroinit, nil)
      else
        macroinit = nil
      end
      far.RecursiveSearch (p, filemask, LoadRegularFile, bor(F.FRS_RECUR,F.FRS_SCANSYMLINK), macroinit)
    end

    far.RecursiveSearch (JoinPath(MacroDirs.MainPath, "internal"), "*.lua", LoadRecordedFile, 0)

    export.ExitFAR             = Events.exitfar[1]      and export_ExitFAR
    export.MayExitFAR          = Events.mayexitfar[1]   and export_MayExitFAR
    export.ProcessDialogEvent  = Events.dialogevent[1]  and export_ProcessDialogEvent
    export.ProcessEditorEvent  = Events.editorevent[1]  and export_ProcessEditorEvent
    export.ProcessEditorInput  = Events.editorinput[1]  and export_ProcessEditorInput
    export.ProcessViewerEvent  = Events.viewerevent[1]  and export_ProcessViewerEvent
    export.ProcessConsoleInput = Events.consoleinput[1] and export_ProcessConsoleInput
    export.ProcessSynchroEvent = Events.synchroevent[1] and export_ProcessSynchroEvent
    if ContentColumns[1] then
      export.GetContentFields = export_GetContentFields
      export.GetContentData   = export_GetContentData
    end

    LoadMacrosDone = true
  end

  LoadingInProgress = nil
  return numerrors==0
end

local function InitMacroSystem()
  LoadMacros(true)
end

local function WriteOneMacro (dir, macro, keyname, delete)
  local fname = ("%s/%s_%s.lua"):format(dir, macro.area, (keyname:gsub(".", CharNames)))
  local attr = win.GetFileAttr(fname)
  if attr then
    win.SetFileAttr(fname, "") --> it is a no-op on Linux/far2m
    win.DeleteFile(fname)
  end

  if delete then return end

  -- operation "write"
  local fp = io.open(fname, "w")
  if fp then
    fp:write(([[
Macro {
  description=%q;
  area=%q; key=%q;
  flags=%q;
  code=%q;
}
]]):format(macro.description, macro.area, macro.key, FlagsToString(macro.flags), macro.code))
    fp:close()
    macro.FileName = fname
  end
end

local function WriteMacros()
  if ReadOnlyConfig then return end

  local dir = JoinPath(MacroDirs.MainPath, "internal")
  if not win.CreateDir(dir, true) then return end

  for areaname,area in pairs(Areas) do
    for keyname,macroarray in pairs(area) do
      local macro = macroarray.recorded
      if macro and macro.needsave then
        WriteOneMacro(dir, macro, macro.key, macro.disabled)
        macro.needsave = nil
        if macro.disabled then
          macroarray.recorded = nil
        end
      end
    end
  end
  return true
end

local function GetFromMenu (menuitems, area, key)
  for _,item in ipairs(menuitems) do
    local descr = item.macro.description
    if not descr or descr=="" then
      descr = Msg.UtNoDescription_Index:format(item.macro.index)
    end
    item.text = descr
    item.selected = item.macro.selected
  end

  table.sort(menuitems,
    function(item1,item2)
      local p1,p2 = item1.priority, item2.priority
      local s1,s2 = item1.macro.sortpriority or 50, item2.macro.sortpriority or 50
      return p1>p2 or p1==p2 and (s1>s2 or s1==s2 and far.LStricmp(item1.text, item2.text) < 0)
    end)

  local pos_sep
  for i,item in ipairs(menuitems) do
    if item.priority < menuitems[1].priority then
      pos_sep = i; break
    end
  end

  local bkeys = { {BreakKey="F4"; action="edit"; } }
  for i,item in ipairs(menuitems) do
    local ch = i<10 and tostring(i) or i<36 and string.char(i+55)
    if ch then
      local pos = pos_sep and i>=pos_sep and i+1 or i
      item.text = ch..". "..item.text
      table.insert(bkeys, {BreakKey=ch, pos=pos})
      if i>=10 then table.insert(bkeys, {BreakKey=ch:lower(), pos=pos}) end
    end
  end
  if pos_sep then
    table.insert(menuitems, pos_sep, {separator=true, text=Msg.UtLowPriority})
  end

  local props = {
      Title = ("%s: %s | %s"):format(Msg.UtExecuteMacroTitle, area, key),
      Bottom = Msg.UtExecuteMacroBottom,
      Flags = { FMENU_WRAPMODE=1, FMENU_CHANGECONSOLETITLE=1 },
      Id = win.Uuid("165AA6E3-C89B-4F82-A0C5-C309243FD21B") }
  while true do
    local item, pos = far.Menu(props, menuitems, bkeys)
    if not item then
      return
    elseif item.macro then
      return item.macro
    elseif item.BreakKey and item.action == "edit" then
      props.SelectIndex = pos
      local m = menuitems[pos].macro
      if m.FileName then
        local startline = m.action and debug.getinfo(m.action,"S").linedefined
        editor.Editor(m.FileName,nil,nil,nil,nil,nil,nil,startline,nil,65001)
      end
    elseif item.pos then
      return menuitems[item.pos].macro
    end
  end
end

local function GetMacro (argMode, argKey, argUseCommon, argCheckOnly)
  if LoadingInProgress then return end

  local area = GetAreaName(argMode)
  if not area then return end -- трюк используется в CheckForEscSilent() в Фаре

  local key = argKey:lower()
  do
    local alt
    local from,to,ctrl = string_find(key, "^(r?ctrl)")
    if ctrl=="ctrl" then ctrl="lctrl" end
    local start = to and to+1 or 1
    from,to,alt = string_find(key, "^(r?alt)", start)
    if alt=="alt" then alt="lalt" end
    start = to and to+1 or start
    key = (ctrl or "") .. (alt or "") .. string_sub(key, start)
  end

  local Names = { area, argUseCommon and area~="common" and "common" or nil }

  -- First, check "keyboard-recorded" macros, they have the highest priority.
  for _,areaname in ipairs(Names) do
    local areatable = Areas[areaname]
    if areatable and areatable[key] then
      local m = areatable[key].recorded
      if m and not m.disabled and (argCheckOnly or MacroCallFar(MCODE_F_CHECKALL,argMode,m.flags,nil,nil)) then
        return m, areaname, true
      end
    end
  end

  -- Create collector table: keys are macros, values are indexes into CInfo.
  -- For each macro, CInfo stores 2 consecutive values: dynamic priority and found area.
  local Collector, CInfo = {}, {}

  -- Filter macros by filemask and flags. Put the "successful" ones in the collector.
  local filename = area=="editor" and editor.GetFileName() or area=="viewer" and viewer.GetFileName()

  local function ExamineMacro (m, areaname)
    local check = not (filename and m.filemask) or CheckFileName(m.filemask, filename)
    if check and MacroCallFar(MCODE_F_CHECKALL, GetAreaCode(area), m.flags, m.callback, m.callbackId) then
      if not Collector[m] then
        local n = #CInfo + 1
        Collector[m] = n
        CInfo[n] = m.priority or 50
        CInfo[n+1] = areaname
      end
    end
  end

  for _,areaname in ipairs(Names) do
    local areatable = Areas[areaname]
    if areatable then
      local macros = areatable[key]
      if macros then
        for _,m in ipairs(macros) do
          if not m.disabled then
            if argCheckOnly then return m, areaname end
            ExamineMacro(m, areaname)
          end
        end
      end
      local macros_regex = areatable[1]
      if macros_regex then
        for _,m in ipairs(macros_regex) do
          if not m.disabled and m.keyregex:match(key) then
            if argCheckOnly then return m, areaname end
            ExamineMacro(m, areaname)
          end
        end
      end
    end
  end
  if not next(Collector) then return end

  -- Filter macros by condition() where available; update dynamic priorities.
  -- Calculate maximal priority and number of macros left in the container.
  local max_priority = -1
  local nummacros = 0
  for m,p in pairs(Collector) do
    if m.condition then
      local pr = m.condition(argKey, m.data) -- unprotected call
      if pr then
        if type(pr)=="number" then
          CInfo[p] = pr>100 and 100 or pr<0 and 0 or pr
        end
      else
        Collector[m] = nil
      end
    end
    if Collector[m] then
      nummacros = nummacros + 1
      if max_priority < CInfo[p] then max_priority = CInfo[p] end
    end
  end
  if nummacros == 0 then return end

  -- If only 1 macro is left, do return it.
  if nummacros == 1 then
    local m = next(Collector)
    return m, CInfo[Collector[m]+1]
  end

  -- Make an array with highest priority macros.
  local macrolist = {}
  local nindex = nil
  for m,p in pairs(Collector) do
    macrolist[#macrolist+1] = { macro=m; priority=CInfo[p] }
    if CInfo[p] == max_priority then
      nindex = nindex and -1 or #macrolist
    end
  end
  if nindex > 0 then
    local m = macrolist[nindex].macro
    return m, CInfo[Collector[m]+1]
  end

  -- Make order of macros in the menu consistent
  table.sort(macrolist, function(m1,m2) return Collector[m1.macro] < Collector[m2.macro] end)

  local m = GetFromMenu(macrolist, GetTrueAreaName(argMode), argKey)
  if m then return m, CInfo[Collector[m]+1] end
  return {}, nil

end

local function GetMacroWrapper (argMode, argKey, argUseCommon)
  local macro,area,kb_macro = GetMacro(argMode, argKey, argUseCommon, true)
  if macro then
    kb_macro = not not kb_macro -- convert to boolean
    LastMessage = pack(GetAreaCode(area), macro.code or "", macro.description or "", macro.flags, kb_macro)
    return LastMessage
  end
end

local function ProcessRecordedMacro (Mode, Key, code, flags, description)
  local Area = GetTrueAreaName(Mode)
  local area, key = Area:lower(), Key:lower()

  local keys,numkeys = ExpandKey(Key)

  if code == "" then -- удаление
    for i=1,numkeys do
      local k = keys[i]
      local m = Areas[area][k] and Areas[area][k].recorded or
                Areas["common"][k] and Areas["common"][k].recorded
      if m then
        m.disabled,m.needsave = true,true
        break
      end
    end
    return
  end

  local macro = {
    area=Area, key=Key, code=code, flags=flags, description=description,
    needsave=true
  }
  local existing = Areas[area][keys[1]] and Areas[area][keys[1]].recorded
  macro.index = existing and existing.index or #LoadedMacros+1
  LoadedMacros[macro.index] = macro

  for i=1,numkeys do
    local k = keys[i]
    Areas[area][k] = Areas[area][k] or {}
    Areas[area][k].recorded = macro
  end
end

local function AddMacroFromFAR (mode, key, lang, code, flags, description, guid, callback, callbackId, priority)
  local area = GetTrueAreaName(mode)
  local m = AddRegularMacro { area=area, key=key, code=code, flags=flags, description=description,
                              guid=guid, callback=callback, callbackId=callbackId, language=lang, priority=priority }
  local action = m and m.action
  if action then
    local env = setmetatable({}, gmeta)
    setfenv(action, env)
  end
  return not not m
end

local function DelMacro (guid, callbackId) -- MCTL_DELMACRO
  for _,areatable in pairs(Areas) do
    for _,macroarray in pairs(areatable) do
      for _,m in ipairs(macroarray) do
        if m.guid and m.guid==guid and m.callbackId==callbackId and not m.disabled then
          m.disabled = true
          return true
        end
      end
    end
  end
end

local function RunStartMacro()
  if not LoadMacrosDone then return end

  local mode = far.MacroGetArea()
  local mtable = (mode==F.MACROAREA_EDITOR and Areas.editor)
    or (mode==F.MACROAREA_VIEWER and Areas.viewer) or Areas.shell

  if mtable == nil then return end

  for k=1,2 do
    if k==2 then mtable = Areas.common end
    for _,macros in pairs(mtable) do
      local m = macros.recorded
      if m and not m.disabled and m.flags and band(m.flags,mc.MFLAGS_RUNAFTERFARSTART)~=0 and not m.autostartdone then
        m.autostartdone=true
        if MacroCallFar(MCODE_F_CHECKALL, mode, m.flags) then
          Shared.keymacro.PostNewMacro(m, m.flags, nil, true)
        end
      end
      for _,m in ipairs(macros) do
        if not m.disabled and m.flags and band(m.flags,mc.MFLAGS_RUNAFTERFARSTART)~=0 and not m.autostartdone then
          m.autostartdone=true
          if MacroCallFar(MCODE_F_CHECKALL, mode, m.flags) then
            if not m.condition or m.condition(nil, m.data) then
              Shared.keymacro.PostNewMacro(m, m.flags, nil, true)
            end
          end
        end
      end
    end
  end
  return true
end

local function GetMacroCopy (index)
  if LoadedMacros[index] then
    local t={}
    for k,v in pairs(LoadedMacros[index]) do t[k]=v end
    return t
  end
  return nil
end


local function EnumScripts (ScriptType)
  local ScriptOrigin = {
    CustomSortModes = Shared.panelsort and Shared.panelsort.GetCustomSortModes(),
    Event = LoadedMacros,
    Macro = LoadedMacros,
    MenuItem = AddedMenuItems,
    CommandLine = AddedPrefixes,
    PanelModule = LoadedPanelModules,
    ContentColumns = ContentColumns,
  }

  local ScriptFilter = {
    Event = function (index) return LoadedMacros[index].group end,
    Macro = function (index) return LoadedMacros[index].area end,
    MenuItem = function (index) return type(index) == "number" end,
    CommandLine =  function (index) return index ~= 1 end,
    PanelModule =  function (index) return type(index) == "number" end,
  }

  local function copy(source)
    local t={}
    for k,v in pairs(source) do
      if type(v) == "table" then
        v = k=="data" and v or copy(v)
      end
      t[k]=v
    end
    return t
  end

  local origin = ScriptOrigin[ScriptType]
  if not origin then
    error("Wrong argument: " .. tostring(ScriptType))
  end
  local index
  return function()
    while true do
      index = next(origin, index)
      if not index then return nil end
      local filter = ScriptFilter[ScriptType]
      if not filter or filter(index) then
        return copy(origin[index]), index
      end
    end
  end
end

local function EditUnsavedMacro (index)
  local m = LoadedMacros[index]
  if m and m.code then
    local flags, code, descr = MacroCallFar(MCODE_F_MACROSETTINGS, m.key, m.flags, m.code, m.description or "")
    if flags then
      m.flags, m.code, m.description = flags, code, descr
    end
  end
end

return {
  AddMacroFromFAR = AddMacroFromFAR,
  CheckFileName = CheckFileName,
  DelMacro = DelMacro,
  EditUnsavedMacro = EditUnsavedMacro,
  EnumMacros = EnumMacros,
  EnumScripts = EnumScripts,
  FixInitialModules = FixInitialModules,
  FlagsToString = FlagsToString,
  GetAreaCode = GetAreaCode,
  GetMacro = GetMacro,
  GetMacroCopy = GetMacroCopy,
  GetMacroWrapper = GetMacroWrapper,
  GetMenuItems = function() return AddedMenuItems end,
  GetMoonscriptLineNumber = GetMoonscriptLineNumber,
  GetPrefixes = function() return AddedPrefixes end,
  GetTrueAreaName = GetTrueAreaName,
  InitMacroSystem = InitMacroSystem,
  LoadingInProgress = function() return LoadingInProgress end,
  LoadMacros = LoadMacros,
  ProcessRecordedMacro = ProcessRecordedMacro,
  RunStartMacro = RunStartMacro,
  UnloadMacros = InitMacroSystem,
  WriteMacros = WriteMacros,
  GetPanelModules = function() return LoadedPanelModules end
}
