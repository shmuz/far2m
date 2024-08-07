-- coding: utf-8
-- luacheck: no global

local Shared = ...
local utils, yieldcall = Shared.utils, Shared.yieldcall
local Sett = Shared.Settings
local mc = Shared.Constants

local MCODE_F_USERMENU = mc.MCODE_F_USERMENU
local MCODE_V_MACRO_AREA = mc.MCODE_V_MACRO_AREA
local F=far.Flags
local band = bit64.band
local MacroCallFar = Shared.MacroCallFar

local function SetProperties (namespace, proptable)
  local meta = {}
  meta.__index = function(tb,nm)
    local f = proptable[nm]
    if f then return f() end
    if nm == "properties" then return proptable end -- to allow introspection
    error("property not supported: "..tostring(nm), 2)
  end
  setmetatable(namespace, meta)
  return namespace
end
--------------------------------------------------------------------------------

-- "mf" ("macrofunctions") namespace
mf = {
  abs             = function(...) return MacroCallFar( mc.MCODE_F_ABS       , ...) end,
--akey            -- implemented in keymacro.lua
  asc             = function(...) return MacroCallFar( mc.MCODE_F_ASC       , ...) end,
  atoi            = function(...) return MacroCallFar( mc.MCODE_F_ATOI      , ...) end,
  chr             = utf8.char,
  clip            = function(...) return MacroCallFar( mc.MCODE_F_CLIP      , ...) end,
  date            = function(...) return MacroCallFar( mc.MCODE_F_DATE      , ...) end,
--env             -- implemented in this file
  fattr           = function(...) return MacroCallFar( mc.MCODE_F_FATTR     , ...) end,
  fexist          = function(...) return MacroCallFar( mc.MCODE_F_FEXIST    , ...) end,
  float           = function(...) return MacroCallFar( mc.MCODE_F_FLOAT     , ...) end,
  flock           = function(...) return MacroCallFar( mc.MCODE_F_FLOCK     , ...) end,
  fmatch          = function(...) return MacroCallFar( mc.MCODE_F_FMATCH    , ...) end,
  fsplit          = function(...) return MacroCallFar( mc.MCODE_F_FSPLIT    , ...) end,
  index           = function(...) return MacroCallFar( mc.MCODE_F_INDEX     , ...) end,
  int             = function(...) return MacroCallFar( mc.MCODE_F_INT       , ...) end,
  itoa            = function(...) return MacroCallFar( mc.MCODE_F_ITOA      , ...) end,
  key             = function(...) return MacroCallFar( mc.MCODE_F_KEY       , ...) end,
  lcase           = function(...) return MacroCallFar( mc.MCODE_F_LCASE     , ...) end,
  len             = function(...) return MacroCallFar( mc.MCODE_F_LEN       , ...) end,
  max             = function(...) return MacroCallFar( mc.MCODE_F_MAX       , ...) end,
  min             = function(...) return MacroCallFar( mc.MCODE_F_MIN       , ...) end,
  mod             = function(...) return MacroCallFar( mc.MCODE_F_MOD       , ...) end,
  msgbox          = function(...) return MacroCallFar( mc.MCODE_F_MSGBOX    , ...) end,
  prompt          = function(...) return MacroCallFar( mc.MCODE_F_PROMPT    , ...) end,
  replace         = function(...) return MacroCallFar( mc.MCODE_F_REPLACE   , ...) end,
  rindex          = function(...) return MacroCallFar( mc.MCODE_F_RINDEX    , ...) end,
--size2str        -- implemented in this file
  sleep           = function(...) return MacroCallFar( mc.MCODE_F_SLEEP     , ...) end,
  string          = function(...) return MacroCallFar( mc.MCODE_F_STRING    , ...) end,
  strwrap         = function(...) return MacroCallFar( mc.MCODE_F_STRWRAP   , ...) end,
  substr          = function(...) return MacroCallFar( mc.MCODE_F_SUBSTR    , ...) end,
  testfolder      = function(...) return MacroCallFar( mc.MCODE_F_TESTFOLDER, ...) end,
  trim            = function(...) return MacroCallFar( mc.MCODE_F_TRIM      , ...) end,
  ucase           = function(...) return MacroCallFar( mc.MCODE_F_UCASE     , ...) end,
  udlsplit        = function(...) return MacroCallFar( mc.MCODE_UDLIST_SPLIT, ...) end,
  waitkey         = function(...) return MacroCallFar( mc.MCODE_F_WAITKEY   , ...) end,
  xlat            = function(...) return MacroCallFar( mc.MCODE_F_XLAT      , ...) end,
}

mf.mainmenu = function(param)
  local mprt =
    param == "fileassociations" and F.MPRT_FILEASSOCIATIONS or
    param == "filehighlight"    and F.MPRT_FILEHIGHLIGHT    or
    param == "filemaskgroups"   and F.MPRT_FILEMASKGROUPS   or
    param == "filepanelmodes"   and F.MPRT_FILEPANELMODES   or
    param == "foldershortcuts"  and F.MPRT_FOLDERSHORTCUTS
  if mprt then
    yieldcall(mprt)
  else
    error("parameter not supported: "..tostring(param), 2)
  end
end

mf.env = function(Name, Mode, Value)
  local oldvalue = win.GetEnv(Name)
  if Mode and Mode ~= 0 then
    win.SetEnv(Name, Value)
  end
  return oldvalue or ""
end

mf.iif = function(Expr, res1, res2)
  if Expr and Expr~=0 and Expr~="" then return res1 else return res2 end
end

-- S=strpad(V,Size[,Fill[,Op]])
mf.strpad = function(V, Size, Fill, Op)
  local tp = type(V)
  if tp == "number" then V=tostring(V)
  elseif tp ~= "string" then V=""
  end

  Size = math.floor(tonumber(Size) or 0)
  if Size < 0 then Size = 0 end

  tp = type(Fill)
  if tp == "number" then Fill=tostring(Fill)
  elseif tp ~= "string" then Fill=" "
  end

  Op = tonumber(Op)
  if not (Op==0 or Op==1 or Op==2) then Op=0 end

  local strDest=V
  local LengthFill = Fill:len()
  if Size > 0 and LengthFill > 0 then
    local LengthSrc = strDest:len()
    local FineLength = Size-LengthSrc

    if FineLength > 0 then
      local NewFill = {}

      for I=1, FineLength do
        local pos = (I-1) % LengthFill + 1
        NewFill[I] = Fill:sub(pos,pos)
      end
      NewFill = table.concat(NewFill)

      local CntL, CntR = 0, 0
      if Op == 0 then     -- right
        CntR = FineLength
      elseif Op == 1 then -- left
        CntL = FineLength
      elseif Op == 2 then -- center
        if LengthSrc > 0 then
          CntL = math.floor(FineLength / 2)
          CntR = FineLength-CntL
        else
          CntL = FineLength
        end
      end

      strDest = NewFill:sub(1,CntL)..strDest..NewFill:sub(1,CntR)
    end
  end

  return strDest
end

-- S = Size2Str(Size, Flags [,Width])
-- @param Flags: FARFORMATFILESIZEFLAGS values are used
mf.size2str = function(Size, Flags, Width)
  Flags = type(Flags)=="number" and Flags or 0
  Width = type(Width)=="number" and Width or 0
  return far.FormatFileSize(Size, Width, Flags, Flags)
end

mf.usermenu = function(mode, filename)
  if not panel.CheckPanelsExist() then return end -- mantis #2986 (crash)
  if mode and type(mode)~="number" then return end
  mode = mode or 0
  local sync_call = band(mode,0x100) ~= 0
  mode = band(mode,0xFF)
  if mode==0 or mode==1 then
    if sync_call then MacroCallFar(MCODE_F_USERMENU, mode==1)
    else yieldcall(F.MPRT_USERMENU, mode==1)
    end
  elseif (mode==2 or mode==3) and type(filename)=="string" then
    if mode==3 then
      if not filename:find("^/") then
        filename = far.InMyConfig(win.JoinPath("Menus",filename))
      end
    end
    if sync_call then MacroCallFar(MCODE_F_USERMENU, filename)
    else yieldcall(F.MPRT_USERMENU, filename)
    end
  end
end

mf.GetMacroCopy = utils.GetMacroCopy
mf.EnumScripts = utils.EnumScripts
--------------------------------------------------------------------------------

Object = {
  CheckHotkey = function(...) return MacroCallFar(mc.MCODE_F_MENU_CHECKHOTKEY, ...) end,
  GetHotkey   = function(...) return MacroCallFar(mc.MCODE_F_MENU_GETHOTKEY, ...) end,
}

SetProperties(Object, {
  Bof        = function() return MacroCallFar(mc.MCODE_C_BOF       ) end,
  CurPos     = function() return MacroCallFar(mc.MCODE_V_CURPOS    ) end,
  Empty      = function() return MacroCallFar(mc.MCODE_C_EMPTY     ) end,
  Eof        = function() return MacroCallFar(mc.MCODE_C_EOF       ) end,
  Height     = function() return MacroCallFar(mc.MCODE_V_HEIGHT    ) end,
  ItemCount  = function() return MacroCallFar(mc.MCODE_V_ITEMCOUNT ) end,
  Selected   = function() return MacroCallFar(mc.MCODE_C_SELECTED  ) end,
  Title      = function() return MacroCallFar(mc.MCODE_V_TITLE     ) end,
  Width      = function() return MacroCallFar(mc.MCODE_V_WIDTH     ) end,
})
--------------------------------------------------------------------------------

local prop_Area = {
  Current    = function() return utils.GetTrueAreaName(MacroCallFar(MCODE_V_MACRO_AREA)) end,
  Other      = function() return MacroCallFar(MCODE_V_MACRO_AREA)==0  end,
  Shell      = function() return MacroCallFar(MCODE_V_MACRO_AREA)==1  end,
  Viewer     = function() return MacroCallFar(MCODE_V_MACRO_AREA)==2  end,
  Editor     = function() return MacroCallFar(MCODE_V_MACRO_AREA)==3  end,
  Dialog     = function() return MacroCallFar(MCODE_V_MACRO_AREA)==4  end,
  Search     = function() return MacroCallFar(MCODE_V_MACRO_AREA)==5  end,
  Disks      = function() return MacroCallFar(MCODE_V_MACRO_AREA)==6  end,
  MainMenu   = function() return MacroCallFar(MCODE_V_MACRO_AREA)==7  end,
  Menu       = function() return MacroCallFar(MCODE_V_MACRO_AREA)==8  end,
  Help       = function() return MacroCallFar(MCODE_V_MACRO_AREA)==9  end,
  Info       = function() return MacroCallFar(MCODE_V_MACRO_AREA)==10 end,
  QView      = function() return MacroCallFar(MCODE_V_MACRO_AREA)==11 end,
  Tree       = function() return MacroCallFar(MCODE_V_MACRO_AREA)==12 end,
  FindFolder = function() return MacroCallFar(MCODE_V_MACRO_AREA)==13 end,
  UserMenu   = function() return MacroCallFar(MCODE_V_MACRO_AREA)==14 end,
  ShellAutoCompletion  = function() return MacroCallFar(MCODE_V_MACRO_AREA)==15 end,
  DialogAutoCompletion = function() return MacroCallFar(MCODE_V_MACRO_AREA)==16 end,
  Grabber    = function() return MacroCallFar(MCODE_V_MACRO_AREA)==17 end,
}

local prop_APanel = {
  Bof         = function() return MacroCallFar(mc.MCODE_C_APANEL_BOF         ) end,
  ColumnCount = function() return MacroCallFar(mc.MCODE_V_APANEL_COLUMNCOUNT ) end,
  CurPos      = function() return MacroCallFar(mc.MCODE_V_APANEL_CURPOS      ) end,
  Current     = function() return MacroCallFar(mc.MCODE_V_APANEL_CURRENT     ) end,
  DriveType   = function() return MacroCallFar(mc.MCODE_V_APANEL_DRIVETYPE   ) end,
  Empty       = function() return MacroCallFar(mc.MCODE_C_APANEL_ISEMPTY     ) end,
  Eof         = function() return MacroCallFar(mc.MCODE_C_APANEL_EOF         ) end,
  FilePanel   = function() return MacroCallFar(mc.MCODE_C_APANEL_FILEPANEL   ) end,
  Filter      = function() return MacroCallFar(mc.MCODE_C_APANEL_FILTER      ) end,
  Folder      = function() return MacroCallFar(mc.MCODE_C_APANEL_FOLDER      ) end,
  Format      = function() return MacroCallFar(mc.MCODE_V_APANEL_FORMAT      ) end,
  Height      = function() return MacroCallFar(mc.MCODE_V_APANEL_HEIGHT      ) end,
  HostFile    = function() return MacroCallFar(mc.MCODE_V_APANEL_HOSTFILE    ) end,
  ItemCount   = function() return MacroCallFar(mc.MCODE_V_APANEL_ITEMCOUNT   ) end,
  Left        = function() return MacroCallFar(mc.MCODE_C_APANEL_LEFT        ) end,
  OPIFlags    = function() return MacroCallFar(mc.MCODE_V_APANEL_OPIFLAGS    ) end,
  Path        = function() return MacroCallFar(mc.MCODE_V_APANEL_PATH        ) end,
  Path0       = function() return MacroCallFar(mc.MCODE_V_APANEL_PATH0       ) end,
  Plugin      = function() return MacroCallFar(mc.MCODE_C_APANEL_PLUGIN      ) end,
  Prefix      = function() return MacroCallFar(mc.MCODE_V_APANEL_PREFIX      ) end,
  Root        = function() return MacroCallFar(mc.MCODE_C_APANEL_ROOT        ) end,
  SelCount    = function() return MacroCallFar(mc.MCODE_V_APANEL_SELCOUNT    ) end,
  Selected    = function() return MacroCallFar(mc.MCODE_C_APANEL_SELECTED    ) end,
  Type        = function() return MacroCallFar(mc.MCODE_V_APANEL_TYPE        ) end,
  UNCPath     = function() return MacroCallFar(mc.MCODE_V_APANEL_UNCPATH     ) end,
  Visible     = function() return MacroCallFar(mc.MCODE_C_APANEL_VISIBLE     ) end,
  Width       = function() return MacroCallFar(mc.MCODE_V_APANEL_WIDTH       ) end,
}

local prop_PPanel = {
  Bof         = function() return MacroCallFar(mc.MCODE_C_PPANEL_BOF         ) end,
  ColumnCount = function() return MacroCallFar(mc.MCODE_V_PPANEL_COLUMNCOUNT ) end,
  CurPos      = function() return MacroCallFar(mc.MCODE_V_PPANEL_CURPOS      ) end,
  Current     = function() return MacroCallFar(mc.MCODE_V_PPANEL_CURRENT     ) end,
  DriveType   = function() return MacroCallFar(mc.MCODE_V_PPANEL_DRIVETYPE   ) end,
  Empty       = function() return MacroCallFar(mc.MCODE_C_PPANEL_ISEMPTY     ) end,
  Eof         = function() return MacroCallFar(mc.MCODE_C_PPANEL_EOF         ) end,
  FilePanel   = function() return MacroCallFar(mc.MCODE_C_PPANEL_FILEPANEL   ) end,
  Filter      = function() return MacroCallFar(mc.MCODE_C_PPANEL_FILTER      ) end,
  Folder      = function() return MacroCallFar(mc.MCODE_C_PPANEL_FOLDER      ) end,
  Format      = function() return MacroCallFar(mc.MCODE_V_PPANEL_FORMAT      ) end,
  Height      = function() return MacroCallFar(mc.MCODE_V_PPANEL_HEIGHT      ) end,
  HostFile    = function() return MacroCallFar(mc.MCODE_V_PPANEL_HOSTFILE    ) end,
  ItemCount   = function() return MacroCallFar(mc.MCODE_V_PPANEL_ITEMCOUNT   ) end,
  Left        = function() return MacroCallFar(mc.MCODE_C_PPANEL_LEFT        ) end,
  OPIFlags    = function() return MacroCallFar(mc.MCODE_V_PPANEL_OPIFLAGS    ) end,
  Path        = function() return MacroCallFar(mc.MCODE_V_PPANEL_PATH        ) end,
  Path0       = function() return MacroCallFar(mc.MCODE_V_PPANEL_PATH0       ) end,
  Plugin      = function() return MacroCallFar(mc.MCODE_C_PPANEL_PLUGIN      ) end,
  Prefix      = function() return MacroCallFar(mc.MCODE_V_PPANEL_PREFIX      ) end,
  Root        = function() return MacroCallFar(mc.MCODE_C_PPANEL_ROOT        ) end,
  SelCount    = function() return MacroCallFar(mc.MCODE_V_PPANEL_SELCOUNT    ) end,
  Selected    = function() return MacroCallFar(mc.MCODE_C_PPANEL_SELECTED    ) end,
  Type        = function() return MacroCallFar(mc.MCODE_V_PPANEL_TYPE        ) end,
  UNCPath     = function() return MacroCallFar(mc.MCODE_V_PPANEL_UNCPATH     ) end,
  Visible     = function() return MacroCallFar(mc.MCODE_C_PPANEL_VISIBLE     ) end,
  Width       = function() return MacroCallFar(mc.MCODE_V_PPANEL_WIDTH       ) end,
}

local prop_CmdLine = {
  Bof       = function() return MacroCallFar(mc.MCODE_C_CMDLINE_BOF       ) end,
  Empty     = function() return MacroCallFar(mc.MCODE_C_CMDLINE_EMPTY     ) end,
  Eof       = function() return MacroCallFar(mc.MCODE_C_CMDLINE_EOF       ) end,
  Selected  = function() return MacroCallFar(mc.MCODE_C_CMDLINE_SELECTED  ) end,
  CurPos    = function() return MacroCallFar(mc.MCODE_V_CMDLINE_CURPOS    ) end,
  ItemCount = function() return MacroCallFar(mc.MCODE_V_CMDLINE_ITEMCOUNT ) end,
  Value     = function() return MacroCallFar(mc.MCODE_V_CMDLINE_VALUE     ) end,
  Result    = function() return Shared.CmdLineResult end,
}

local prop_Drv = {
  ShowMode = function() return MacroCallFar(mc.MCODE_V_DRVSHOWMODE ) end,
  ShowPos  = function() return MacroCallFar(mc.MCODE_V_DRVSHOWPOS  ) end,
}

local prop_Help = {
  FileName = function() return MacroCallFar(mc.MCODE_V_HELPFILENAME ) end,
  SelTopic = function() return MacroCallFar(mc.MCODE_V_HELPSELTOPIC ) end,
  Topic    = function() return MacroCallFar(mc.MCODE_V_HELPTOPIC    ) end,
}

local prop_Mouse = {
  X             = function() return MacroCallFar(mc.MCODE_C_MSX             ) end,
  Y             = function() return MacroCallFar(mc.MCODE_C_MSY             ) end,
  Button        = function() return MacroCallFar(mc.MCODE_C_MSBUTTON        ) end,
  CtrlState     = function() return MacroCallFar(mc.MCODE_C_MSCTRLSTATE     ) end,
  EventFlags    = function() return MacroCallFar(mc.MCODE_C_MSEVENTFLAGS    ) end,
  LastCtrlState = function() return MacroCallFar(mc.MCODE_C_MSLASTCTRLSTATE ) end,
}

local prop_Viewer = {
  FileName = function() return MacroCallFar(mc.MCODE_V_VIEWERFILENAME) end,
  State    = function() return MacroCallFar(mc.MCODE_V_VIEWERSTATE)    end,
}
--------------------------------------------------------------------------------

Dlg = {
  GetValue = function(...) return MacroCallFar(mc.MCODE_F_DLG_GETVALUE, ...) end,
  SetFocus = function(...) return MacroCallFar(mc.MCODE_F_DLG_SETFOCUS, ...) end,
}

SetProperties(Dlg, {
  CurPos     = function() return MacroCallFar(mc.MCODE_V_DLGCURPOS)    end,
  Id         = function() return MacroCallFar(mc.MCODE_V_DLGINFOID)    end,
  Owner      = function() return MacroCallFar(mc.MCODE_V_DLGINFOOWNER) end,
  ItemCount  = function() return MacroCallFar(mc.MCODE_V_DLGITEMCOUNT) end,
  ItemType   = function() return MacroCallFar(mc.MCODE_V_DLGITEMTYPE)  end,
  PrevPos    = function() return MacroCallFar(mc.MCODE_V_DLGPREVPOS)   end,
})
--------------------------------------------------------------------------------

Editor = {
--DelLine  -- implemented below in this file
  GetStr   = function(n)   return editor.GetString(nil,n,2) or "" end,
--InsStr   -- implemented below in this file
  Pos      = function(...) return MacroCallFar(mc.MCODE_F_EDITOR_POS, ...) end,
  Sel      = function(...) return MacroCallFar(mc.MCODE_F_EDITOR_SEL, ...) end,
  Set      = function(...) return MacroCallFar(mc.MCODE_F_EDITOR_SET, ...) end,
--SetStr   -- implemented below in this file
  SetTitle = function(...) return MacroCallFar(mc.MCODE_F_EDITOR_SETTITLE, ...) end,
  Undo     = function(...) return MacroCallFar(mc.MCODE_F_EDITOR_UNDO, ...) end,
}

SetProperties(Editor, {
  CurLine  = function() return MacroCallFar(mc.MCODE_V_EDITORCURLINE) end,
  CurPos   = function() return MacroCallFar(mc.MCODE_V_EDITORCURPOS) end,
  FileName = function() return MacroCallFar(mc.MCODE_V_EDITORFILENAME) end,
  Lines    = function() return MacroCallFar(mc.MCODE_V_EDITORLINES) end,
  RealPos  = function() return MacroCallFar(mc.MCODE_V_EDITORREALPOS) end,
  SelValue = function() return MacroCallFar(mc.MCODE_V_EDITORSELVALUE) end,
  State    = function() return MacroCallFar(mc.MCODE_V_EDITORSTATE) end,
  Value    = function() return editor.GetString(nil,nil,2) or "" end,
})

Editor.DelLine = function(Line)
  local ok
  if far.MacroGetArea() == F.MACROAREA_EDITOR then
    local info = editor.GetInfo()
    if band(info.CurState, F.ECSTATE_LOCKED) == 0 then
      Line = tonumber(Line)
      if Line and Line < 1 then Line = nil end
      if Line then
        Line = math.floor(Line)
        if Line <= info.TotalLines then
          editor.SetPosition(nil,Line)
          ok = editor.DeleteString()
          if info.CurLine > Line then info.CurLine = info.CurLine-1 end
          editor.SetPosition(nil,info) -- restore the position
        end
      else
        ok = editor.DeleteString()
      end
    end
  end
  return ok and 1 or 0
end

Editor.InsStr = function(S, Line)
  local ok
  if far.MacroGetArea() == F.MACROAREA_EDITOR then
    local info = editor.GetInfo()
    if band(info.CurState, F.ECSTATE_LOCKED) == 0 then
      Line = tonumber(Line)
      if not Line or Line < 1 then Line = info.CurLine end
      Line = math.floor(Line)
      if Line <= info.TotalLines then
        if type(S)=="number" then S = tostring(S) end
        if type(S)~="string" then S = "" end
        editor.SetPosition(nil, Line, 1)
        ok = editor.InsertString()
        if S ~= "" then
          editor.SetPosition(nil, Line, 1)
          editor.SetString(nil, nil, S)
        end
        if info.CurLine > Line then info.CurLine = info.CurLine+1 end
        editor.SetPosition(nil,info) -- restore the position
      end
    end
  end
  return ok and 1 or 0
end

Editor.SetStr = function(S, Line)
  local ok
  if far.MacroGetArea() == F.MACROAREA_EDITOR then
    local info = editor.GetInfo()
    if band(info.CurState, F.ECSTATE_LOCKED) == 0 then
      Line = tonumber(Line)
      if not Line or Line < 1 then Line = info.CurLine end
      Line = math.floor(Line)
      if Line <= info.TotalLines then
        if type(S)=="number" then S = tostring(S) end
        if type(S)~="string" then S = "" end
        ok = editor.SetString(nil, Line, S)
      end
    end
  end
  return ok and 1 or 0
end
--------------------------------------------------------------------------------

Menu = {
  Filter     = function(...) return MacroCallFar(mc.MCODE_F_MENU_FILTER, ...) end,
  FilterStr  = function(...) return MacroCallFar(mc.MCODE_F_MENU_FILTERSTR, ...) end,
  GetValue   = function(...) return MacroCallFar(mc.MCODE_F_MENU_GETVALUE, ...) end,
  ItemStatus = function(...) return MacroCallFar(mc.MCODE_F_MENU_ITEMSTATUS, ...) end,
  Select     = function(...) return MacroCallFar(mc.MCODE_F_MENU_SELECT, ...) end,
--Show       = function(...) return MacroCallFar(mc.MCODE_F_MENU_SHOW, ...) end,
}

Menu.Show = function (Items, TitleAndFooter, Flags, SelectOrFilter, X, Y)
  Flags = tonumber(Flags) or 0

  local bResultAsIndex     = band(Flags, 0x008) ~= 0
  local bMultiSelect       = band(Flags, 0x010) ~= 0
  local bSorting           = band(Flags, 0x020) ~= 0
  local bPacking           = band(Flags, 0x040) ~= 0
  local bAutohighlight     = band(Flags, 0x080) ~= 0
--local bSetMenuFilter     = band(Flags, 0x100) ~= 0
  local bAutoNumbering     = band(Flags, 0x200) ~= 0
--local bExitAfterNavigate = band(Flags, 0x400) ~= 0

  local props, rows = {}, {}
  props.Flags = bAutohighlight and F.FMENU_AUTOHIGHLIGHT or 0
  props.X, props.Y = tonumber(X), tonumber(Y)

  local remdups = bPacking and {}
  local separators = {}

  if Items then
    for v,eol in tostring(Items):gmatch("([^\r\n]*)(\r?\n?)") do
      if v=="" and eol=="" then break end
      local txt = v:gsub("[\1\2\3\4]%s?", "")
      local sep = v:find("\1")
      local a2  = v:find("\2")
      local a3  = v:find("\3")
      local a4  = v:find("\4")
      if sep then
        table.insert(separators, {text=txt; separator=true; checked=a2; disable=a3; grayed=a4;
                     Pos=#rows+#separators+1; })
      else
        if not (remdups and remdups[txt]) then
          table.insert(rows, {text=txt; checked=a2; disable=a3; grayed=a4; })
        end
        if remdups then remdups[txt] = true end
      end
    end
  end

  if bSorting then
    table.sort(rows, function(a,b)
        local n1,n2 = tonumber(a.text,10), tonumber(b.text,10)
        if n1 and n2 then return n1 < n2 end
        return a.text < b.text
      end)
  end

  if bAutoNumbering then
    local cur = 0
    for _,v in ipairs(rows) do
      cur = cur + 1
      v.text = cur..". "..v.text
    end
  end

  -- insert separators (they neither participate in sort nor in removing duplicates)
  for _, sep in ipairs(separators) do
    table.insert(rows, sep.Pos, sep); sep.Pos=nil
  end

  if TitleAndFooter then
    props.Title, props.Bottom = tostring(TitleAndFooter):match("([^\r\n]*)[\r\n]*([^\r\n]*)")
  end

  if type(SelectOrFilter) == "number" then
    props.SelectIndex = SelectOrFilter
  elseif type(SelectOrFilter) == "string" then
    local pat = SelectOrFilter:lower()
    for i,v in ipairs(rows) do
      if not v.separator and far.CmpName(pat,v.text:lower()) then
        props.SelectIndex = i; break;
      end
    end
  end

  local bkeys = bMultiSelect and {{BreakKey="INSERT"}}

  while true do
    local item,pos = far.Menu(props,rows,bkeys)
    if not item then
      return bResultAsIndex and 0 or ""
    elseif item.BreakKey == "INSERT" then
      rows[pos].checked = not rows[pos].checked
      props.SelectIndex = pos
    elseif bMultiSelect then
      local t = {}
      for i,r in ipairs(rows) do
        if r.checked then
          table.insert(t, bResultAsIndex and tostring(i) or r.text)
        end
      end
      return t[1] and table.concat(t,"\n") or bResultAsIndex and pos or item.text
    else
      return bResultAsIndex and pos or item.text
    end
  end
end

SetProperties(Menu, {
  Id         = function() return MacroCallFar(mc.MCODE_V_MENUINFOID) end,
  Value      = function() return MacroCallFar(mc.MCODE_V_MENU_VALUE) end,
})
--------------------------------------------------------------------------------

Far = {
  GetConfig      = function(...) return MacroCallFar(mc.MCODE_F_FAR_GETCONFIG, ...) end,
  DisableHistory = function(...) return Shared.keymacro.DisableHistory(...) end,
  KbdLayout      = function(...) return MacroCallFar(mc.MCODE_F_KBDLAYOUT, ...) end,
  KeyBar_Show    = function(...) return MacroCallFar(mc.MCODE_F_KEYBAR_SHOW, ...) end,
  SetConfig      = function(...) return MacroCallFar(mc.MCODE_F_FAR_SETCONFIG, ...) end,
  Window_Scroll  = function(...) return MacroCallFar(mc.MCODE_F_WINDOW_SCROLL, ...) end,
}

SetProperties(Far, {
  FullScreen     = function() return MacroCallFar(mc.MCODE_C_FULLSCREENMODE) end,
  Height         = function() return MacroCallFar(mc.MCODE_V_FAR_HEIGHT) end,
  IsUserAdmin    = function() return MacroCallFar(mc.MCODE_C_ISUSERADMIN) end,
  PID            = function() return MacroCallFar(mc.MCODE_V_FAR_PID) end,
  Title          = function() return MacroCallFar(mc.MCODE_V_FAR_TITLE) end,
  UpTime         = function() return MacroCallFar(mc.MCODE_V_FAR_UPTIME) end,
  Width          = function() return MacroCallFar(mc.MCODE_V_FAR_WIDTH) end,
})

Far.GetInfo = function()
  local a1,a2,a3,a4,a5,a6,a7 = MacroCallFar(mc.MCODE_FAR_GETINFO)
  return {
    Build               = a1;
    Platform            = a2;
    MainLang            = a3;
    HelpLang            = a4;
    ConsoleColorPalette = a5;
    WinPortBackEnd      = a6;
    Compiler            = a7;
  }
end
--------------------------------------------------------------------------------

BM = {
  Add   = function(...) return MacroCallFar(mc.MCODE_F_BM_ADD,   ...) end,
  Back  = function(...) return MacroCallFar(mc.MCODE_F_BM_BACK,  ...) end,
  Clear = function(...) return MacroCallFar(mc.MCODE_F_BM_CLEAR, ...) end,
  Del   = function(...) return MacroCallFar(mc.MCODE_F_BM_DEL,   ...) end,
  Get   = function(...) return MacroCallFar(mc.MCODE_F_BM_GET,   ...) end,
  Goto  = function(...) return MacroCallFar(mc.MCODE_F_BM_GOTO,  ...) end,
  Next  = function(...) return MacroCallFar(mc.MCODE_F_BM_NEXT,  ...) end,
  Pop   = function(...) return MacroCallFar(mc.MCODE_F_BM_POP,   ...) end,
  Prev  = function(...) return MacroCallFar(mc.MCODE_F_BM_PREV,  ...) end,
  Push  = function(...) return MacroCallFar(mc.MCODE_F_BM_PUSH,  ...) end,
  Stat  = function(...) return MacroCallFar(mc.MCODE_F_BM_STAT,  ...) end,
}
--------------------------------------------------------------------------------

Plugin = {
  Call    = function(...) return yieldcall(F.MPRT_PLUGINCALL,    ...) end,
  Command = function(...) return yieldcall(F.MPRT_PLUGINCOMMAND, ...) end,
  Config  = function(...) return yieldcall(F.MPRT_PLUGINCONFIG,  ...) end,
  Menu    = function(...) return yieldcall(F.MPRT_PLUGINMENU,    ...) end,

  Exist = function(...) return MacroCallFar(mc.MCODE_F_PLUGIN_EXIST, ...) end,

  Load = function(DllPath, Force) -- 2-nd param and return are booleans
    return (Force and far.ForcedLoadPlugin or far.LoadPlugin)(F.PLT_PATH, DllPath)
  end,

  Unload = function(DllPath) return far.UnloadPlugin(F.PLT_PATH, DllPath); end, -- returns a boolean

  SyncCall = function(...)
    local v = Shared.keymacro.CallPlugin(Shared.pack(...), false)
    if type(v)=="userdata" then return Shared.MacroCallToLua(v) else return v end
  end,
}
--------------------------------------------------------------------------------

Panel = {
  FAttr     = function(...) return MacroCallFar(mc.MCODE_F_PANEL_FATTR, ...) end,
  FExist    = function(...) return MacroCallFar(mc.MCODE_F_PANEL_FEXIST, ...) end,
  Item      = function(a,b,c)
    local r = MacroCallFar(mc.MCODE_F_PANELITEM,a,b,c)
    if c==8 and r==0 then r=false end -- 8:Selected; boolean property
    return r
  end,
  Select    = function(...) return MacroCallFar(mc.MCODE_F_PANEL_SELECT, ...) end,
  SetPath   = function(...) return MacroCallFar(mc.MCODE_F_PANEL_SETPATH, ...) end,
  SetPluginPath
            = function(...) return MacroCallFar(mc.MCODE_F_PANEL_SETPLUGINPATH, ...) end,
  SetPos    = function(...) return MacroCallFar(mc.MCODE_F_PANEL_SETPOS, ...) end,
  SetPosIdx = function(...) return MacroCallFar(mc.MCODE_F_PANEL_SETPOSIDX, ...) end,
}
--------------------------------------------------------------------------------

Area    = SetProperties({}, prop_Area)
APanel  = SetProperties({}, prop_APanel)
PPanel  = SetProperties({}, prop_PPanel)
CmdLine = SetProperties({}, prop_CmdLine)
Drv     = SetProperties({}, prop_Drv)
Help    = SetProperties({}, prop_Help)
Mouse   = SetProperties({}, prop_Mouse)
Viewer  = SetProperties({}, prop_Viewer)
--------------------------------------------------------------------------------

local EVAL_SUCCESS       =  0
local EVAL_SYNTAXERROR   = 11
local EVAL_BADARGS       = -1
local EVAL_MACRONOTFOUND = -2  -- макрос не найден среди загруженных макросов
local EVAL_MACROCANCELED = -3  -- было выведено меню выбора макроса, и пользователь его отменил
local EVAL_RUNTIMEERROR  = -4  -- макрос был прерван в результате ошибки времени исполнения

local function Eval_GetData (str) -- Получение данных макроса для Eval(S,2).
  local Mode = far.MacroGetArea()
  local UseCommon = false
  str = str:match("^%s*(.-)%s*$")

  local slash, strArea, strKey = str:match("^(/?)(.-)/(.+)$")
  if slash == '/' then
    strKey = str:sub(2)
    UseCommon = true
  elseif strArea then
    if strArea ~= "." then -- вариант "./Key" не подразумевает поиск в макрообласти Common
      local SpecifiedMode = utils.GetAreaCode(strArea)
      if SpecifiedMode then
        Mode = SpecifiedMode
      else
        strKey = str
        UseCommon = true
      end
    end
  else
    strKey = str
    UseCommon = true
  end

  return Mode, strKey, UseCommon
end

local function Eval_FixReturn (ok, ...)
  return ok and EVAL_SUCCESS or EVAL_RUNTIMEERROR, ...
end

-- @param mode:
--   0=Выполнить макропоследовательность str
--   1=Проверить макропоследовательность str и вернуть код ошибки компиляции
--   2=Выполнить макрос, назначенный на сочетание клавиш str
--   3=Проверить макропоследовательность str и вернуть строку-сообщение с ошибкой компиляции
function mf.eval (str, mode, lang)
  if type(str) ~= "string" then return EVAL_BADARGS end
  mode = mode or 0
  if not (mode==0 or mode==1 or mode==2 or mode==3) then return EVAL_BADARGS end
  lang = lang or "lua"
  if not (lang=="lua" or lang=="moonscript") then return EVAL_BADARGS end

  if mode == 2 then
    local area,key,usecommon = Eval_GetData(str)
    if not area then return EVAL_MACRONOTFOUND end

    local macro = utils.GetMacro(area,key,usecommon,false)
    if not macro then return EVAL_MACRONOTFOUND end
    if macro=="cancel" then return EVAL_MACROCANCELED end

    return Eval_FixReturn(yieldcall("eval", macro, key))
  end

  local ok, env = pcall(getfenv, 3)
  local chunk, params = Shared.loadmacro(lang, str, ok and env)
  if chunk then
    if mode==1 then return EVAL_SUCCESS end
    if mode==3 then return "" end
    if params then chunk(params())
    else chunk()
    end
    return EVAL_SUCCESS
  else
    local msg = params
    if mode==0 then Shared.ErrMsg(msg) end
    return mode==3 and msg or EVAL_SYNTAXERROR
  end
end
--------------------------------------------------------------------------------

mf.serialize = Sett.serialize
mf.deserialize = Sett.deserialize
mf.mdelete = Sett.mdelete
mf.msave = Sett.msave
mf.mload = Sett.mload

function mf.printconsole(...)
  local narg = select("#", ...)
  for i=1,narg do
    local text = select(i, ...)
    if i > 1 then far.WriteConsole("\t") end
    far.WriteConsole(text)
  end
end
--------------------------------------------------------------------------------

_G.band, _G.bnot, _G.bor, _G.bxor, _G.lshift, _G.rshift =
  bit64.band, bit64.bnot, bit64.bor, bit64.bxor, bit64.lshift, bit64.rshift

_G.eval, _G.msgbox, _G.prompt = mf.eval, mf.msgbox, mf.prompt

mf.Keys, mf.exit, mf.print = _G.Keys, _G.exit, _G.print
--------------------------------------------------------------------------------
