-- started: 2022-11-03

local M    = require "hv_message"
local Sett = require "far2.settings"
local sd   = require "far2.simpledialog"
local SETTINGS_KEY  = nil
local SETTINGS_NAME = "settings"

local F = far.Flags

local DefOpt = {
  ProcessEditorInput = true;
  CheckFileMask      = true;
  FileMask           = "*.hlf";
  AssignKeyName      = "F1";
  Style              = 0;
}
local Opt

local function Trim(s)
  return s:match("^%s*(.-)%s*$")
end

local function LoadConfig()
  Opt = {}
  local loaded = Sett.mload(SETTINGS_KEY, SETTINGS_NAME) or {}
  for k in pairs(DefOpt) do
    if loaded[k] == nil then Opt[k] = DefOpt[k]
    else Opt[k] = loaded[k]
    end
  end
end

local function FileExists(Name)
  return win.GetFileAttr(Name) ~= nil
end

local function CheckExtension(ptrName)
  if Opt.CheckFileMask and Opt.FileMask ~= "" then
    return far.ProcessName("PN_CMPNAMELIST", Opt.FileMask, ptrName, "PN_SKIPPATH")
  end
  return true
end

local function ShowHelp(fullfilename, topic, CmdLine, ShowError)
  if fullfilename and (CmdLine or CheckExtension(fullfilename)) then
    topic = topic or M.MDefaultTopic
    return far.ShowHelp(fullfilename, topic, F.FHELP_CUSTOMFILE + (ShowError and 0 or F.FHELP_NOSHOWERROR))
  end
end

local function RestorePosition(ei)
  local esp = {}
  esp.CurLine = ei.CurLine
  esp.CurPos  = ei.CurPos
  esp.TopScreenLine = ei.TopScreenLine
  esp.LeftPos = ei.LeftPos
  editor.SetPosition(nil,esp)
end

-- для "этой темы" ищем её имя (от позиции курсора вверх/вниз по файлу)
local function FindTopic (ForwardDirect, RestorePos)
  local ret = nil
  local ei = editor.GetInfo()

  local Direct = ForwardDirect and 1 or -1

  local esp = { CurLine=ei.CurLine; }
  while true do
    if ForwardDirect then
      if esp.CurLine > ei.TotalLines then break end
    else
      if esp.CurLine < 1 then break end
    end
    editor.SetPosition(nil,esp)
    local egs = editor.GetString()
    local tmp = egs.StringText

    -- "Тема": начинается '@', дальше букво-цифры, не содержит '='
    if tmp:match("^@[^%-+=][^=]*$") then
      ret = tmp:sub(2)
      break
    end

    esp.CurLine = esp.CurLine + Direct
  end

  if RestorePos then
    RestorePosition(ei)
  end

  return ret
end

-- это HLF-файл?
-- первая строка hlf всегда начинается с ".Language="
local function IsHlf()
  if far.MacroGetArea() ~= F.MACROAREA_EDITOR then
    return false
  end

  local ret=false
  local ei = editor.GetInfo()
  local CheckedHlf=true

  if Opt.CheckFileMask then
    local FileName = editor.GetFileName();
    if FileName then
      CheckedHlf = CheckExtension(FileName)
    end
  end

  if CheckedHlf and ei.TotalLines >= 3 then
    local esp = {}
    for i=1,3 do
      esp.CurLine = i
      editor.SetPosition(nil,esp)
      local egs = editor.GetString()

      if 0 == far.LStrnicmp(".Language=",egs.StringText,10) then
        -- доп.проверка
        if FindTopic(true,false) then
          ret = true
        end
        break
      end
    end

    RestorePosition(ei)
  end

  return ret
end

local function ShowHelpFromTempFile()
  local fname = far.MkTemp("HLF")
  if fname then
    fname = fname..".hlf"

    local Handle = io.open(fname, "w")

    if Handle then
      local ei = editor.GetInfo()

      for i=1, ei.TotalLines do
        local egs = editor.GetString(nil,i)
        Handle:write(egs.StringText, "\n")
      end

      Handle:close()
      ShowHelp(fname, FindTopic(false,true))
      win.DeleteFile(fname)
    end
  end
end

local function ShowCurrentHelpTopic()
  local Result = true
  local FileName = editor.GetFileName()
  local ei = editor.GetInfo()

  if Opt.Style == 1 then -- "show modified file"
    if 0 == bit64.band(ei.CurState, F.ECSTATE_SAVED) then
      ShowHelpFromTempFile()
    else
      local Topic = FindTopic(false,true) or FindTopic(true,true)
      if Topic and Topic ~= "" then
        ShowHelp(FileName, Topic, false)
      else
        Result = false
      end
    end
  else
    if Opt.Style == 2 then -- "save and show modified file"
      if 0 == bit64.band(ei.CurState, F.ECSTATE_SAVED) then
        editor.SaveFile()
      end
    end
    ShowHelp(FileName, FindTopic(false,true), false)
  end

  return Result
end

local function FindPluginHelp(Name)
  local hPlugins = far.GetPlugins()
  if hPlugins then
    if not Name:find("%.") then
      Name = Name..".hlf"
    end
    for _,hPlug in ipairs(hPlugins) do
      local Info = far.GetPluginInformation(hPlug)
      if Info then
        local file = Info.ModuleName:match(".*/")..Name
        if FileExists(file) then
          return file
        elseif file:find("^/usr/") then -- is far2m installed?
          file = file:gsub("^(.-)/lib/", "%1/share/")
          if FileExists(file) then
            return file
          end
        end
      end
    end
  end
end

local function OpenCommandLine(cmdbuf)
  -- разбор параметров ком.строки
  local ModuleName = far.PluginStartupInfo().ModuleName

  if cmdbuf:find("%S") then
    local ptrName, ptrTopic = far.SplitCmdLine(cmdbuf)

    ptrTopic = ptrTopic and ptrTopic:match("%S.*")
    if ptrTopic then
      ptrTopic = ptrTopic:gsub("^@", "")
      ptrTopic = ptrTopic:find("%S") and Trim(ptrTopic)
    end

    if not ptrTopic and ptrName:find('^@') then
      ptrTopic = Trim(ptrName:sub(2))
      ptrName = nil
    end

    -- Здесь: ptrName - имя файла/GUID, ptrTopic - имя темы

    -- по GUID`у не найдено, пробуем имя файла
    if not ptrName then
      far.ShowHelp(ModuleName, ptrTopic, F.FHELP_FARHELP)
    else
      local TempFileName = ptrName

      -- Если имя файла без пути...
      if not ptrName:find("/") then
        -- ...смотрим в текущем каталоге
        local ptrCurDir = far.GetCurrentDirectory()

        if ptrCurDir then
          ptrCurDir = win.JoinPath(ptrCurDir, ptrName)
          if FileExists(ptrCurDir) then
            ptrName = ptrCurDir
          end
        end

        -- ...в текущем нет...
        if not ptrName:find("/") then
          -- ...смотрим в %FARHOME%
          local ExpFileName = win.JoinPath(win.GetEnv("FARHOME"), ptrName)
          if not FileExists(ExpFileName) then
            -- ...в %FARHOME% нет, поищем по путям плагинов.
            ExpFileName = FindPluginHelp(ptrName)
            if ExpFileName then ptrName=ExpFileName end
          else
            ptrName=ExpFileName
          end
        end
      else
        -- ptrName указан с путём.
        ptrName = win.ExpandEnv(ptrName)
      end

      local FileName = far.ConvertPath(ptrName)
      if not ShowHelp(FileName, ptrTopic, true, ptrTopic and ptrTopic ~= "") then
        -- синтаксис hlf:topic_из_ФАР_хелпа ==> TempFileName
        far.ShowHelp(ModuleName, TempFileName, F.FHELP_FARHELP)
      end
    end
  else
    -- параметры не указаны, выводим подсказку по использованию плагина.
    far.ShowHelp(ModuleName, "cmd", F.FHELP_SELFHELP)
  end

end

local function OpenFromMacro(Item)
  if IsHlf() then -- проверяем файл на принадлежность к системе помощи Far Manager
    if ShowCurrentHelpTopic() then
      return true
    end
  end
end

function export.Open(OpenFrom, _Id, Item)
  if OpenFrom == F.OPEN_COMMANDLINE then
    return OpenCommandLine(Item)
  elseif OpenFrom == F.OPEN_FROMMACRO then
    return OpenFromMacro(Item)
  elseif OpenFrom == F.OPEN_EDITOR then
    if IsHlf() then -- проверяем файл на принадлежность к системе помощи Far Manager
      ShowCurrentHelpTopic()
    else
      far.Message(M.MNotAnHLF, M.MTitle, M.MOk)
    end
  end
end

local MenuGuid = win.Uuid("5D01066A-9019-46E9-B821-B40DA9119E16")

function export.GetPluginInfo()
  return {
    CommandPrefix = "HLF";
    Flags = F.PF_EDITOR + F.PF_DISABLEPANELS;
    PluginConfigStrings = { M.MTitle };
    PluginMenuStrings = not Opt.ProcessEditorInput and { M.MShowHelpTopic } or nil;
    PluginConfigGuids = MenuGuid;
    PluginMenuGuids = MenuGuid;
  }
end

function export.ProcessEditorInput (Rec)
  if Opt.ProcessEditorInput
      and Rec.EventType == F.KEY_EVENT
      and Rec.KeyDown
      and Opt.AssignKeyName
      and Opt.AssignKeyName == far.InputRecordToName(Rec)
  then
    if IsHlf() or (Opt.CheckFileMask and CheckExtension(editor.GetFileName())) then
      return ShowCurrentHelpTopic()
    end
  end
end

function export.Configure()
  local X1 = math.max(M.MProcessEditorInput:len(), M.MCheckMaskFile:len()) + 10
  local Items = {
    guid="7A3A74E8-505E-482B-A7F3-2ECE6AC41650";
    help="Config";
    width=0;
    { tp="dbox";  text=M.MConfig; },
    { tp="chbox"; text=M.MProcessEditorInput; val=Opt.ProcessEditorInput; name="ProcessEditorInput"; },
    { tp="chbox"; text=M.MCheckMaskFile;      val=Opt.CheckFileMask;      name="CheckFileMask"; },
    { tp="edit";  x1=X1; ystep=-1; width=28;  val=Opt.AssignKeyName;      name="AssignKeyName"; },
    { tp="edit";  x1=X1;           width=28;  val=Opt.FileMask;           name="FileMask"; },
    { tp="sep"; },
    { tp="text";  text=M.MStyle; },
    { tp="rbutt"; text=M.MStyle1;             val=Opt.Style==0;           name="Style0"; },
    { tp="rbutt"; text=M.MStyle2;             val=Opt.Style==1;           name="Style1"; },
    { tp="rbutt"; text=M.MStyle3;             val=Opt.Style==2;           name="Style2"; },
    { tp="sep"; },
    { tp="butt"; text=M.MOk;     centergroup=1; default=1; },
    { tp="butt"; text=M.MCancel; centergroup=1; cancel=1; },
  }

  local out = sd.New(Items):Run()
  if out then
    Opt.ProcessEditorInput = out.ProcessEditorInput
    Opt.CheckFileMask      = out.CheckFileMask
    Opt.AssignKeyName      = out.AssignKeyName
    Opt.FileMask           = out.FileMask
    Opt.Style = out.Style2 and 2 or out.Style1 and 1 or 0
    Sett.msave(SETTINGS_KEY, SETTINGS_NAME, Opt)
    return true
  end
end

do
  LoadConfig()
  --far.ReloadDefaultScript = true
end
