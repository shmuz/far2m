-- Encoding: UTF-8
-- tmppanel.lua

local sd = require "far2.simpledialog"

-- The default message table
local M = {
  MOk                         = "Ok";
  MCancel                     = "Cancel";
  MError                      = "Error";
  MWarning                    = "Warning";
  MTempPanel                  = "LuaFAR Temp. Panel";
  MTempPanelTitleNum          = " %sLuaFAR Temp. Panel [%d] ";
  MDiskMenuString             = "temporary (LuaFAR)";
  MF7                         = "Remove";
  MAltShiftF12                = "Switch";
  MAltShiftF2                 = "SavLst";
  MAltShiftF3                 = "Goto";
  MTempUpdate                 = "Updating temporary panel contents";
  MTempSendFiles              = "Sending files to temporary panel";
  MSwitchMenuTxt              = "Total files:";
  MSwitchMenuTitle            = "Available temporary panels";
  MConfigTitle                = "LuaFAR Temporary Panel";
  MConfigAddToDisksMenu       = "Add to &Disks menu";
  MConfigAddToPluginsMenu     = "Add to &Plugins menu";
  MConfigCommonPanel          = "Use &common panel";
  MSafeModePanel              = "&Safe panel mode";
  MReplaceInFilelist          = "&Replace files with file list";
  MMenuForFilelist            = "&Menu from file list";
  MCopyContents               = "Copy folder c&ontents";
  MFullScreenPanel            = "F&ull screen mode";
  MColumnTypes                = "Column &types";
  MColumnWidths               = "Column &widths";
  MStatusColumnTypes          = "Status line column t&ypes";
  MStatusColumnWidths         = "Status l&ine column widths";
  MMask                       = "File masks for the file &lists:";
  MPrefix                     = "Command line pre&fix:";
  MConfigNewOption            = "New settings will become active after FAR restart";
  MNewPanelForSearchResults   = "&New panel for search results";
  MListFilePath               = "Save file list as";
  MCopyContentsMsg            = "Copy folder contents?";
  MSavePanelsOnFarExit        = "Sa&ve panels on FAR exit";
}

-- This function should be called if message localization support is needed
local function Mod_SetMessageTable(msg_tbl) M = msg_tbl; end

local F  = far.Flags
local band, bor = bit64.band, bit64.bor

-- constants
local COMMONPANELSNUMBER = 10
local BOM_UTF32LE = "\255\254\0\0"
local BOM_UTF16LE = "\255\254"
local BOM_UTF8 = "\239\187\191"

local Opt = {
  AddToDisksMenu            = true,
  AddToPluginsMenu          = true,
  CommonPanel               = true,
  SafeModePanel             = false,
  CopyContents              = 2,
  ReplaceMode               = true,
  MenuForFilelist           = false,
  NewPanelForSearchResults  = true,
  FullScreenPanel           = false,
  ColumnTypes               = "N,S",
  ColumnWidths              = "0,8",
  StatusColumnTypes         = "NR,SC,D,T",
  StatusColumnWidths        = "0,8,0,5",
  Mask                      = "*.tmp2",
  Prefix                    = "tmp2",
  SavePanels                = false,
}

local Env, Panel = {}, {}
local EnvMeta = { __index = Env }

local function LTrim(s) return s:match "^%s*(.*)" end
local function Trim(s) return s:match "^%s*(.-)%s*$" end
local function Unquote(s) return (s:gsub("\"", "")) end
local function ExtractFileName(s) return s:match "[^/]*$" end
local function ExtractFileDir(s) return s:match ".*/" or "" end
local function AddEndSlash(s) return (s:gsub("/?$", "/", 1)) end
local function TruncStr(s, maxlen)
  local len = s:len()
  return len <= maxlen and s or s:sub(1,6) .. "..." .. s:sub (len - maxlen + 10)
end

local ExpandEnvironmentStr = win.ExpandEnv

local function IsDirectory (PanelItem)
  return PanelItem.FileAttributes:find"d" and true
end


-- File lists are supported in the following formats:
-- (a) UTF-8/16LE/32LE with BOM and (b) UTF-8 w/o BOM.
local function ListFromFile (aFileName)
  local list = {}
  local hFile = io.open (aFileName, "rb")
  if hFile then
    local text = hFile:read("*a")
    hFile:close()
    if text then
      local strsub = string.sub
      if strsub(text, 1, 3) == BOM_UTF8 then
        text = strsub(text, 4)
      elseif strsub(text, 1, 4) == BOM_UTF32LE then
        text = win.Utf32ToUtf8(strsub(text, 5))
      elseif strsub(text, 1, 2) == BOM_UTF16LE then
        text = win.MultiByteToWideChar(strsub(text, 3), 1200)
        text = win.Utf32ToUtf8(text)
      -- else -- default is UTF-8
        -- do nothing
      end
      for line in text:gmatch("[^\n\r]+") do
        table.insert(list, line)
      end
    end
  end
  return list
end


local function IsOwnersDisplayed (ColumnTypes)
  for word in ColumnTypes:gmatch "[^,]+" do
    if word == "O" then return true end
  end
end


local function IsLinksDisplayed (ColumnTypes)
  for word in ColumnTypes:gmatch "[^,]+" do
    if word == "LN" then return true end
  end
end


local function IsGroupsDisplayed (ColumnTypes)
  for word in ColumnTypes:gmatch "[^,]+" do
    if word == "U" then return true end
  end
end


local function ParseParam (str)
  local p1, p2 = str:match "^%|(.*)%|(.*)"
  if p1 then
    return p1, LTrim(p2)
  end
  return nil, str
end


local function isDevice (FileName, dev_begin)
  local len = dev_begin:len()
  return FileName:sub(1, len):upper() == dev_begin:upper() and
         FileName:sub(len+1):match("%d+$") and true
end


local function CheckForCorrect (Name)
  Name = ExpandEnvironmentStr(Name)
  local _, p = ParseParam (Name)
  if p:match [[^\\%.\%a%:$]]
      or isDevice(p, [[\\.\PhysicalDrive]])
      or isDevice(p, [[\\.\cdrom]]) then
    return { FileName = p, FileAttributes = "a" }
  end

  if p:find "%S" and not p:find "[?*]" and p ~= "/" and p ~= ".." then
    local q = p:gsub("/$", "")
    local Item = win.GetFileInfo(q)
    if Item then
      Item.FileName = p
      Item.PackSize = Item.FileSize
      return Item
    end
  end
end


local function IsCurrentFileCorrect (Handle)
  local fname = panel.GetCurrentPanelItem(Handle, 1).FileName
  return (fname == ".." or CheckForCorrect(fname)) and fname
end


local function GoToFile (Target, PanelNumber)
  local Dir  = Unquote (Trim (ExtractFileDir (Target)))
  if Dir ~= "" then
    panel.SetPanelDirectory (nil, PanelNumber, Dir)
  end

  local PInfo = assert(panel.GetPanelInfo (nil, PanelNumber))
  local Name = Unquote (Trim (ExtractFileName (Target))):upper()
  for i=1, PInfo.ItemsNumber do
    local item = panel.GetPanelItem (nil, PanelNumber, i)
    if Name == ExtractFileName (item.FileName):upper() then
      panel.RedrawPanel (nil, PanelNumber, { CurrentItem=i, TopPanelItem=i })
      return
    end
  end
end


local function ShowMenuFromFile (FileName)
  local list = ListFromFile(FileName,false)
  local menuitems = {}
  for i, line in ipairs(list) do
    line = ExpandEnvironmentStr(line)
    local part1, part2 = ParseParam(line)
    if part1 == "-" then
      menuitems[i] = { separator=true }
    else
      local menuline = TruncStr(part1 or part2, 67)
      menuitems[i] = { text=menuline, action=part2 }
    end
  end

  local Title = ExtractFileName(FileName):gsub("%.[^.]+$", "")
  Title = TruncStr(Title, 64)
  local Item = far.Menu(
    { Flags="FMENU_WRAPMODE", Title=Title, HelpTopic="Contents", Bottom=#menuitems.." lines" },
    menuitems)
  if not Item then return end

  local panelitem = CheckForCorrect (Item.action)
  if panelitem then
    if IsDirectory (panelitem) then
      panel.SetPanelDirectory (nil, 1, Item.action)
    end
  else
    panel.SetCmdLine (nil, Item.action)
  end
end


-- Создать новое окружение, или воссоздать из истории /?/
local function Mod_NewEnv (aEnv)
  local self = aEnv or {}

  -- создать или воссоздать опции для окружения
  self.Opt = self.Opt or {}
  for k,v in pairs(Opt) do -- скопировать отсутствующие опции
    if self.Opt[k]==nil then self.Opt[k]=v end
  end
  self.OptMeta = { __index = self.Opt } -- метатаблица для будущего наследования

  -- инициализировать некоторые переменные
  self.LastSearchResultsPanel = self.LastSearchResultsPanel or 1
  self.StartupOptCommonPanel = self.Opt.CommonPanel
  self.StartupOptFullScreenPanel = self.Opt.FullScreenPanel

  -- если нет "общих" панелей - создать их
  if not self.CommonPanels then
    self.CommonPanels = {}
    for i=1,COMMONPANELSNUMBER do self.CommonPanels[i] = {} end
    self.CurrentCommonPanel = 1
  end

  -- установить наследование функций от базового окружения
  return setmetatable (self, EnvMeta)
end
--------------------------------------------------------------------------------


function Env:OpenPanelFromOutput (command, options)
  local h = io.popen (command, "r")
  if h then
    local list = {}
    for line in h:lines() do
      table.insert (list, line)
    end
    h:close()
    local newpanel = self:NewPanel(options)
    newpanel:ProcessList (list, newpanel.Opt.ReplaceMode)
    return newpanel
  end
end


function Env:GetPluginInfo()
  local opt = self.Opt
  local Info = {
    Flags = 0;
    CommandPrefix = opt.Prefix;
  }
  if opt.AddToPluginsMenu then
    Info.PluginMenuStrings = { M.MTempPanel }
  end
  if opt.AddToDisksMenu then
    Info.DiskMenuStrings = { M.MDiskMenuString }
  end
  Info.PluginConfigStrings = { M.MTempPanel }
  return Info
end


function Env:SelectPanelFromMenu()
  local txt = M.MSwitchMenuTxt
  local fmt1 = "&%s. %s %d"
  local menuitems = {}
  for i = 1, COMMONPANELSNUMBER do
    local menuline
    if i <= 10 then
      menuline = fmt1:format(i-1, txt, #self.CommonPanels[i])
    elseif i <= 36 then
      menuline = fmt1:format(string.char(("A"):byte()+i-11), txt, #self.CommonPanels[i])
    else
      menuline = ("   %s %d"):format(txt, #self.CommonPanels[i])
    end
    menuitems[i] = { text=menuline }
  end

  local Item, Position = far.Menu( {
    Flags = {FMENU_AUTOHIGHLIGHT=1, FMENU_WRAPMODE=1},
    Title = M.MSwitchMenuTitle, HelpTopic = "Contents",
    SelectIndex = self.CurrentCommonPanel,
  }, menuitems)
  return Item and Position
end


function Env:FindSearchResultsPanel()
  for i,v in ipairs(self.CommonPanels) do
    if #v == 0 then return i end
  end
  -- no panel is empty - use least recently used index
  local index = self.LastSearchResultsPanel
  self.LastSearchResultsPanel = self.LastSearchResultsPanel + 1
  if self.LastSearchResultsPanel > #self.CommonPanels then
    self.LastSearchResultsPanel = 1
  end
  return index
end


function Env:NewPanel (aOptions)
  local pan = {
    Env = self,
    LastOwnersRead = false,
    LastLinksRead = false,
    LastGroupsRead = false,
  }

  pan.Opt = setmetatable({}, self.OptMeta)
  if aOptions then
    for k,v in pairs(aOptions) do pan.Opt[k] = v end
  end

  if self.StartupOptCommonPanel then
    pan.Index = self.CurrentCommonPanel
    pan.GetItems = Panel.GetRefItems
    pan.ReplaceFiles = Panel.ReplaceRefFiles
  else
    pan.Files = {}
    pan.GetItems = Panel.GetOwnItems
    pan.ReplaceFiles = Panel.ReplaceOwnFiles
  end
  return setmetatable (pan, { __index = Panel })
end


function Env:OpenFilePlugin (Name, Data)
  if Name then
    for mask in self.Opt.Mask:gmatch "[^,]+" do
      if far.CmpName(mask, Name, "PN_SKIPPATH") then
        if self.Opt.MenuForFilelist then
          ShowMenuFromFile(Name)
          break
        else
          local pan = self:NewPanel()
          pan:ProcessList (ListFromFile(Name), self.Opt.ReplaceMode)
          pan.HostFile = Name
          return pan
        end
      end
    end
  end
end


function Env:Open (OpenFrom, Item)
  self.StartupOpenFrom = OpenFrom
  if OpenFrom == F.OPEN_COMMANDLINE then
    local newOpt = setmetatable({}, {__index=self.Opt})
    local ParamsTable = {
      full    = "FullScreenPanel";
      menu    = "MenuForFilelist";
      replace = "ReplaceMode";
      safe    = "SafeModePanel";
    }

    -- handle options
    local argv = Item
    while argv ~= "" do
      local switch, param, rest = argv:match "^%s*([+%-])(%S*)(.*)"
      if not switch then
        break
      end
      param = param:lower()
      if ParamsTable[param] then
        newOpt[ParamsTable[param]] = (switch == "+")
      elseif param:match("^%d$") then
        self.CurrentCommonPanel = tonumber(param) + 1
      end
      argv = rest
    end

    -- handle next arguments
    argv = Trim(argv)
    if argv == "" then
      return self:NewPanel(newOpt)
    elseif argv:sub(1,1) == "<" then
      return self:OpenPanelFromOutput(argv:sub(2), newOpt)
    else
      argv = far.SplitCmdLine(argv)
      local PathName = ExpandEnvironmentStr(argv)
      local attr = win.GetFileAttr(PathName)
      if attr and not attr:find("d") then
        if newOpt.MenuForFilelist then
          ShowMenuFromFile (PathName)
        else
          local pan = self:NewPanel(newOpt)
          pan:ProcessList (ListFromFile(PathName), newOpt.ReplaceMode)
          pan.HostFile = PathName
          return pan
        end
      end
    end
  else
    return self:NewPanel()
  end
end


function Env:ExitFAR()
  if not self.Opt.SavePanels then
    self.CommonPanels = nil
    self.CurrentCommonPanel = nil
  end
end


function Env:Configure()
  local width = 78
  local DC = math.floor(width/2-1)

  local Items = {
    guid = "dd4492cf-d7a3-431d-b464-3fe4ee63de57";
    width = width;
    help = "Config";
    {tp="dbox"; text=M.MConfigTitle; },
    {tp="chbox"; name="AddToDisksMenu"; text=M.MConfigAddToDisksMenu; },
    {tp="chbox"; name="AddToPluginsMenu"; y1=""; x1=DC; text=M.MConfigAddToPluginsMenu; },
    {tp="sep"; ystep=2; },
    ------------------------------------------------------------------------------------------------
    {tp="chbox"; name="CommonPanel";    text=M.MConfigCommonPanel; },
    {tp="chbox"; name="SafeModePanel";  text=M.MSafeModePanel; },
--  {tp="chbox"; name="AnyInPanel"; text=M.MAnyInPanel; },
    {tp="chbox"; name="CopyContents"; tristate=1; text=M.MCopyContents; };
    {tp="chbox"; name="ReplaceMode";              x1=DC; ystep=-2; text=M.MReplaceInFilelist; },
    {tp="chbox"; name="MenuForFilelist";          x1=DC; text=M.MMenuForFilelist; },
    {tp="chbox"; name="NewPanelForSearchResults"; x1=DC; text=M.MNewPanelForSearchResults; },
    {tp="chbox"; name="SavePanels";               x1=DC; text=M.MSavePanelsOnFarExit; },
    {tp="sep"; },
    ------------------------------------------------------------------------------------------------
    {tp="text"; text=M.MColumnTypes;          },
    {tp="edit"; name="ColumnTypes";  x2=DC-2; },
    {tp="text"; text=M.MColumnWidths;         },
    {tp="edit"; name="ColumnWidths"; x2=DC-2; },
    {tp="text"; x1=DC; ystep=-3; text=M.MStatusColumnTypes; },
    {tp="edit"; x1=DC; name="StatusColumnTypes";   },
    {tp="text"; x1=DC; text=M.MStatusColumnWidths; },
    {tp="edit"; x1=DC; name="StatusColumnWidths";  },
    {tp="chbox"; name="FullScreenPanel"; text=M.MFullScreenPanel; },
    {tp="sep"; },
    ------------------------------------------------------------------------------------------------
    {tp="text"; text=M.MMask;         },
    {tp="edit"; name="Mask"; x2=DC-2; },
    {tp="text"; x1=DC; ystep=-1; text=M.MPrefix; },
    {tp="edit"; x1=DC; name="Prefix"; },
    {tp="sep"; },
    ------------------------------------------------------------------------------------------------
    {tp="butt"; text=M.MOk;     centergroup=1; default=1; },
    {tp="butt"; text=M.MCancel; centergroup=1; cancel=1;  },
  }
  local dlg = sd.New(Items)
  dlg:LoadData(self.Opt)

  local out = dlg:Run()
  if out then
    dlg:SaveData(out, self.Opt)

    if self.StartupOptCommonPanel ~= self.Opt.CommonPanel then
      far.Message (M.MConfigNewOption, M.MTempPanel, M.MOk)
    end
    return true
  end
end


function Panel:GetOwnItems()
  return self.Files
end


function Panel:GetRefItems()
  return self.Env.CommonPanels[self.Index]
end


function Panel:ReplaceOwnFiles (Table)
  self.Files = Table
end


function Panel:ReplaceRefFiles(Table)
  self.Env.CommonPanels[self.Index] = Table
end


function Panel:ClosePanel (Handle)
  collectgarbage "collect"
end


function Panel:ProcessList (aList, aReplaceMode)
  if aReplaceMode then self:ReplaceFiles {} end
  local items = self:GetItems()
  for _,v in ipairs(aList) do
    if win.GetFileAttr(v) then
      items[#items+1] = v
    end
  end
end


function Panel:UpdateItems (ShowOwners, ShowLinks, ShowGroups)
  local hScreen = #self:GetItems() >= 1000 and far.SaveScreen()
  if hScreen then far.Message(M.MTempUpdate, M.MTempPanel, "") end

  self.LastOwnersRead = ShowOwners
  self.LastLinksRead = ShowLinks
  self.LastGroupsRead = ShowGroups
  local RemoveTable = {}
  local PanelItems = {}
  for i,v in ipairs(self:GetItems()) do
    local panelitem = CheckForCorrect (v)
    if panelitem then
      table.insert (PanelItems, panelitem)
    else
      RemoveTable[i] = true
    end
  end
  self:RemoveMarkedItems(RemoveTable)

  if ShowOwners or ShowLinks or ShowGroups then
    for _,v in ipairs(PanelItems) do
      if ShowOwners then
        v.Owner = far.GetFileOwner(nil, v.FileName)
      end
      if ShowLinks then
        v.NumberOfLinks = far.GetNumberOfLinks(v.FileName)
      end
      if ShowGroups then
        v.Group = far.GetFileGroup(nil, v.FileName)
      end
    end
  end
  if hScreen then far.RestoreScreen(hScreen) end
  return PanelItems
end


function Panel:ProcessRemoveKey (Handle)
  local PInfo = assert(panel.GetPanelInfo (Handle, 1))
  if PInfo.SelectedItemsNumber == 0 then
    return
  end
  local tb_out, tb_dict = {}, {}
  for i=1, PInfo.SelectedItemsNumber do
    local item = panel.GetSelectedPanelItem (Handle, 1, i)
    tb_dict[item.FileName] = true
  end
  for _,v in ipairs(self:GetItems()) do
    if not tb_dict[v] then
      table.insert (tb_out, v)
    end
  end
  self:ReplaceFiles (tb_out)

  panel.UpdatePanel (Handle, 1, true)
  panel.RedrawPanel (Handle, 1)

  PInfo = assert(panel.GetPanelInfo (nil, 0))
  if PInfo.PanelType == F.PTYPE_QVIEWPANEL then
    panel.UpdatePanel (nil, 0, true)
    panel.RedrawPanel (nil, 0)
  end
end


function Panel:SaveListFile (Path)
  local hFile = io.open (Path, "wb")
  if hFile then
    for _,v in ipairs(self:GetItems()) do
      hFile:write (v, "\n")
    end
    hFile:close()
  else
    far.Message ("", M.MError, nil, "we")
  end
end


function Panel:ProcessSaveListKey (Handle)
  if #self:GetItems() == 0 then return end

  -- default path: opposite panel directory\panel<index>.<mask extension>
  local CurDir = panel.GetPanelDirectory(nil, 0).Name
  local ListPath = AddEndSlash (CurDir) .. "panel"
  if self.Index then
    ListPath = ListPath .. (self.Index - 1)
  end

  local ExtBuf = self.Opt.Mask:gsub(",.*", "")
  local ext = ExtBuf:match "%..-$"
  if ext and not ext:match "[*?]" then
    ListPath = ListPath .. ext
  end

  ListPath = far.InputBox (nil, M.MTempPanel, M.MListFilePath,
      "TmpPanel.SaveList", ListPath, nil, nil, F.FIB_BUTTONS)
  if ListPath then
    self:SaveListFile (ListPath)
    panel.UpdatePanel (nil, 0, true)
    panel.RedrawPanel (nil, 0)
  end
end


do
  local VK = win.GetVirtualKeys()
  local C, A, S = F.PKF_CONTROL, F.PKF_ALT, F.PKF_SHIFT
  local PREPROCESS = F.PKF_PREPROCESS

  function Panel:ProcessKey (Handle, Key, ControlState)
    if band(Key, PREPROCESS) ~= 0 then
      return false
    end

    if ControlState == 0 and Key == VK.F1 then
      far.ShowHelp (far.PluginStartupInfo().ModuleName, nil,
        bor (F.FHELP_USECONTENTS, F.FHELP_NOSHOWERROR))
      return true
    end

    if ControlState == bor(A,S) and Key == VK.F9 then -- AltShiftF9
       if self.ConfigFunction then self.ConfigFunction() end
       return true
    end

    if ControlState == bor(A,S) and Key == VK.F3 then -- AltShiftF3
      local CurFileName = IsCurrentFileCorrect (Handle)
      if CurFileName and CurFileName ~= ".." then
        local currItem = assert(panel.GetCurrentPanelItem (Handle))
        if IsDirectory (currItem) then
          panel.SetPanelDirectory (nil, 0, CurFileName)
        else
          GoToFile(CurFileName, 0)
        end
        panel.RedrawPanel (nil, 0)
        return true
      end
    end

    if ControlState ~= C and (Key==VK.F3 or Key==VK.F4 or Key==VK.F5 or
                              Key==VK.F6 or Key==VK.F8) then
      if not IsCurrentFileCorrect (Handle) then
        return true
      end
    end

    if self.Opt.SafeModePanel and ControlState == C and Key == VK.PRIOR then
      local CurFileName = IsCurrentFileCorrect(Handle)
      if CurFileName then
        if CurFileName ~= ".." then
          GoToFile(CurFileName, 1)
        else
          panel.ClosePanel(Handle, ".")
        end
        return true
      end
    end

    if ControlState == 0 and Key == VK.F7 then
      self:ProcessRemoveKey (Handle)
      collectgarbage "collect"
      return true
    elseif ControlState == bor(A,S) and Key == VK.F2 then
      self:ProcessSaveListKey()
      return true
    else
      if self.Env.StartupOptCommonPanel and ControlState == bor(A,S) then
        if Key == VK.F12 then
          local index = self.Env:SelectPanelFromMenu()
          if index then
            self:SwitchToPanel (Handle, index)
          end
          return true
        elseif Key >= VK["0"] and Key <= VK["9"] then
          self:SwitchToPanel (Handle, Key - VK["0"] + 1)
          return true
        end
      end
    end
    return false
  end
end


function Panel:RemoveDuplicates ()
  local RemoveTable, map = {}, {}
  for i,v in ipairs(self:GetItems()) do
    if map[v] then RemoveTable[i] = true
    else map[v] = true
    end
  end
  self:RemoveMarkedItems(RemoveTable)
end


function Panel:CommitPutFiles (hRestoreScreen)
  far.RestoreScreen (hRestoreScreen)
end


function Panel:PutFiles (Handle, PanelItems, Move, SrcPath, OpMode)
  local was_error
  local hScreen = self:BeginPutFiles()
  for _,v in ipairs (PanelItems) do
    if not self:PutOneFile(SrcPath, v) then
      was_error = true
    end
  end
  collectgarbage "collect"
  self:CommitPutFiles (hScreen)
  return not was_error
end


function Panel:BeginPutFiles()
  self.SelectedCopyContents = self.Opt.CopyContents
  local hScreen = far.SaveScreen()
  far.Message (M.MTempSendFiles, M.MTempPanel, "")
  return hScreen
end


function Panel:PutOneFile (_SrcPath, PanelItem)
  local CurName = PanelItem.FileName
  PanelItem = CheckForCorrect(CurName)
  if not PanelItem then return false end

  local NameOnly = not CurName:match("/")
  CurName = PanelItem.FileName
  if NameOnly then
    CurName = AddEndSlash (far.GetCurrentDirectory()) .. CurName
  end
  local items = self:GetItems()
  items[#items+1] = CurName

  if self.SelectedCopyContents ~= 0 and NameOnly and IsDirectory(PanelItem) then
    if self.SelectedCopyContents == 2 then
      local res = far.Message (M.MCopyContentsMsg, M.MWarning, ";YesNo", "", "Config")
      self.SelectedCopyContents = (res == 1) and 1 or 0
    end
    if self.SelectedCopyContents ~= 0 then
      local DirPanelItems = far.GetDirList (CurName)
      if DirPanelItems then
        for _, v in ipairs (DirPanelItems) do
          items[#items+1] = v.FileName
        end
      else
        self:ReplaceFiles {}
        return false
      end
    end
  end
  return true
end


function Panel:GetFindData (Handle, OpMode)
  local OK, Items = far.SudoCRCall(
    function()
      self:RemoveDuplicates()
      local types = panel.GetColumnTypes (Handle)
      return self:UpdateItems (IsOwnersDisplayed (types), IsLinksDisplayed (types), IsGroupsDisplayed (types))
    end)
  if OK then return Items end
  far.Message(Items, M.MError, nil, "wl")
end


function Panel:RemoveMarkedItems (RemoveTable)
  if next(RemoveTable) then
    local tb = {}
    local items = self:GetItems()
    for i,v in ipairs(items) do
      if not RemoveTable[i] then table.insert(tb, v) end
    end
    self:ReplaceFiles(tb)
  end
end


function Panel:ProcessPanelEvent (Handle, Event, Param)
  if Event == F.FE_CHANGEVIEWMODE then
    local types = panel.GetColumnTypes (Handle)
    local UpdateOwners = IsOwnersDisplayed (types) and not self.LastOwnersRead
    local UpdateLinks = IsLinksDisplayed (types) and not self.LastLinksRead
    local UpdateGroups = IsGroupsDisplayed (types) and not self.LastGroupsRead
    if UpdateOwners or UpdateLinks or UpdateGroups then
      self:UpdateItems (UpdateOwners, UpdateLinks, UpdateGroups)
      panel.UpdatePanel (Handle, 1, true)
      panel.RedrawPanel (Handle, 1)
    end
  end
  return false
end


local OPIF_SAFE_FLAGS, OPIF_COMMON_FLAGS do
  local f = F
  OPIF_SAFE_FLAGS = bor (f.OPIF_USEFILTER, f.OPIF_USESORTGROUPS,
                  f.OPIF_USEHIGHLIGHTING, f.OPIF_ADDDOTS, f.OPIF_SHOWNAMESONLY)
  OPIF_COMMON_FLAGS = bor (OPIF_SAFE_FLAGS, f.OPIF_REALNAMES,
                  f.OPIF_EXTERNALGET, f.OPIF_EXTERNALDELETE)
end

function Panel:GetOpenPanelInfo (Handle)
  -----------------------------------------------------------------------------
  --far.Message"GetOpenPanelInfo"  --> this crashes FAR if enter then exit viewer/editor
                                   --  on a file in the emulated file system
  -----------------------------------------------------------------------------
  local Info = {
    Flags = self.Opt.SafeModePanel and OPIF_SAFE_FLAGS or OPIF_COMMON_FLAGS,
    Format = M.MTempPanel,
    CurDir = "",
  }
  if self.HostFile then
    local cur = panel.GetCurrentPanelItem(nil, 1)
    if cur and cur.FileName==".." then Info.HostFile=self.HostFile; end
  end
  -----------------------------------------------------------------------------
  local TitleMode = self.Opt.SafeModePanel and "(R) " or ""
  if self.Index then
    Info.PanelTitle = M.MTempPanelTitleNum : format(TitleMode, self.Index-1)
  else
    Info.PanelTitle = (" %s%s ") : format(TitleMode, M.MTempPanel)
  end
  -----------------------------------------------------------------------------
  local mode = {
    ColumnTypes = self.Opt.ColumnTypes,
    ColumnWidths = self.Opt.ColumnWidths,
    StatusColumnTypes = self.Opt.StatusColumnTypes,
    StatusColumnWidths = self.Opt.StatusColumnWidths,
    CaseConversion = true,
  }
  if self.Env.StartupOpenFrom == F.OPEN_COMMANDLINE then
    mode.FullScreen = self.Opt.FullScreenPanel
  else
    mode.FullScreen = self.Env.StartupOptFullScreenPanel
  end
  Info.PanelModesArray = { [5] = mode }
  Info.PanelModesNumber = 10
  Info.StartPanelMode = ("4"):byte()
  -----------------------------------------------------------------------------
  Info.KeyBar = {
    Titles = { [7] = M.MF7 },
    AltShiftTitles = { [2] = M.MAltShiftF2, [3] = M.MAltShiftF3,
      [12] = self.Env.StartupOptCommonPanel and M.MAltShiftF12 }
  }
  -----------------------------------------------------------------------------
  return Info
end


function Panel:SetDirectory (Handle, Dir, OpMode)
  if 0 == band(OpMode, F.OPM_FIND) then
    panel.ClosePanel (Handle, (Dir ~= "/" and Dir or nil))
    return true
  end
end


function Panel:SetFindList (Handle, PanelItems)
  local hScreen = self:BeginPutFiles()
  if self.Index and self.Opt.NewPanelForSearchResults then
    self.Index = self.Env:FindSearchResultsPanel()
    self.Env.CurrentCommonPanel = self.Index
  end
  local newfiles = {}
  for i,v in ipairs(PanelItems) do
    newfiles[i] = v.FileName
  end
  self:ReplaceFiles (newfiles)
  self:CommitPutFiles (hScreen)
  return true
end


function Panel:SwitchToPanel (Handle, Index)
  if Index and Index ~= self.Index then
    self.Index = Index
    self.Env.CurrentCommonPanel = Index
    panel.UpdatePanel(Handle, 1, true)
    panel.RedrawPanel(Handle, 1)
  end
end


return {
  SetMessageTable = Mod_SetMessageTable;
  NewEnv          = Mod_NewEnv;
  Panel           = Panel;
}
