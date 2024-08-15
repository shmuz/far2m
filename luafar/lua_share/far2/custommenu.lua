-- Name    : Custom menu
-- Author  : Shmuel Zeigerman
-- Started : 2010-03-25
-- Portable Far3/far2m : 2023-06-29

-- luacheck: no unused args

local osWindows = package.config:sub(1,1) == "\\"
local XLat = osWindows and far.XLat or require "far2.xlat"
local F = far.Flags
local min, max, floor, ceil = math.min, math.max, math.floor, math.ceil
local band, bor = bit64.band, bit64.bor
local DlgSend = far.SendDlgMessage

local function GetColor (index)
  local tbl = osWindows and far.Colors or far.Flags
  return far.AdvControl("ACTL_GETCOLOR", tbl[index])
end

local function SendRedrawMessage (hDlg)
  DlgSend(hDlg, "DM_REDRAW")
end

local function FlagsToInt (input)
  local tp, ret = type(input), 0
  if tp == "table" then
    for k,v in pairs(input) do
      if F[k] and v then ret = bor(ret, F[k]) end
    end
  elseif tp == "string" then ret = F[input] or 0
  elseif tp == "number" then ret = input
  end
  return ret
end

local function limit (v, lo, hi)
  if lo>hi then lo,hi=hi,lo end
  return v<lo and lo or v>hi and hi or v
end

local function btest (bitset, bitname)
  return band(bitset, F[bitname]) ~= 0
end

local List = {}
local ListMeta = { __index=List }

local ListMetaDebug = {
  __index=function(self,k)
    if type(List[k])=="function" then
      self.Log(("  "):rep(self.depth)..k)
      return function(...)
        self.depth = self.depth + 1
        local a,b,c,d = List[k](...)
        self.depth = self.depth - 1
        return a,b,c,d
      end
    else
      return List[k]
    end
  end;
}

local function SetParam(trg, src, key, default)
  if default == nil then
    trg[key] = src[key]
  else
    trg[key] = src[key] or default
  end
end

-- new custom list
local function NewList (props, items, bkeys, startId)
  assert (type(props) == "table")
  assert (type(items) == "table")
  assert (not bkeys or type(bkeys)=="table")
  local P = props
  local self
  if props.debug then
    self = setmetatable ({}, ListMetaDebug)
    self.debug = true
    self.depth = 1
    self.Log = osWindows and win.OutputDebugString or far.Log --luacheck: no global
  else
    self = setmetatable ({}, ListMeta)
    self.Log = function() end
  end

  -- Constants
  self.bkeys     = bkeys
  self.flags     = FlagsToInt(P.flags)
  self.items     = items
  self.startId   = startId or 1
  self.title     = P.title or ""
  self:SetIndexData()

  -- Variables
  self.bottom    = P.bottom or ""
  self.drawitems = items
  self.fulltitle = self.title
  self.sel       = P.sel or 1
  -- fields initialized to nil:
  --   w, h, upper, clickX, clickY

  -- Custom list properties and their defaults
  if P.resizeScreen then
    local sb = win.GetConsoleScreenBufferInfo()
    self.wmax = max(4, sb.WindowRight - sb.WindowLeft + 1 - 8)
    self.hmax = max(1, sb.WindowBottom - sb.WindowTop + 1 - 8)
  else
    self.wmax = max(4, P.wmax or 74)
    self.hmax = max(1, P.hmax or 19)
  end

  SetParam(self, P, "autocenter")
  self.col_highlight         = P.col_highlight         or GetColor("COL_MENUHIGHLIGHT")
  self.col_selectedhighlight = P.col_selectedhighlight or GetColor("COL_MENUSELECTEDHIGHLIGHT")
  self.col_selectedtext      = P.col_selectedtext      or GetColor("COL_MENUSELECTEDTEXT")
  self.col_text              = P.col_text              or GetColor("COL_MENUTEXT")
  self.ellipsis  = (P.ellipsis or 1) % 4
  SetParam(self, P, "filterlines")
  SetParam(self, P, "margin", "  ")
  SetParam(self, P, "pattern", "")
  SetParam(self, P, "resizeH")
  SetParam(self, P, "resizeScreen")
  SetParam(self, P, "resizeW")
  self.rmargin = 1 -- right margin length
  SetParam(self, P, "searchmethod", "regex")
  SetParam(self, P, "searchstart", 1)
  SetParam(self, P, "selalign", "center") -- top/center/bottom/
  SetParam(self, P, "selignore")
  SetParam(self, P, "xlat")
  SetParam(self, P, "showdates")
  SetParam(self, P, "dateformat", "%Y-%m-%d")

  SetParam(self, P, "keys_searchmethod",      "F5")
  SetParam(self, P, "keys_ellipsis",          "F6")
  SetParam(self, P, "keys_showitem",          "F7")
  SetParam(self, P, "keys_xlatonoff",         "F8")
  SetParam(self, P, "keys_clearpattern",      {"Del","NumDel"})
  SetParam(self, P, "keys_insertpattern",     {"CtrlV","RCtrlV","ShiftIns","ShiftNum0"})
  SetParam(self, P, "keys_checkitem",         {"Ins","Num0"})
  SetParam(self, P, "keys_copyitem",          {"CtrlC","CtrlIns","CtrlNum0","RCtrlC","RCtrlIns","RCtrlNum0"})
  SetParam(self, P, "keys_deleteitem",        {"ShiftDel","ShiftNumDel"})
  SetParam(self, P, "keys_copyfiltereditems", {"CtrlShiftIns","RCtrlShiftIns","CtrlShiftNum0","RCtrlShiftNum0"})
  SetParam(self, P, "keys_delfiltereditems",  {"CtrlDel","RCtrlDel","CtrlNumDel","RCtrlNumDel"})
  SetParam(self, P, "keys_applyxlat",         {"CtrlAltX","RCtrlAltX","CtrlRAltX","RCtrlRAltX"})

  self:SetSize()
  return self
end

function List:CreateDialogItems (x, y)
  self.x, self.y = x, y
  return { "DI_USERCONTROL", x, y, x+self.w+1, y+self.h+1, 0,0,0,0, "" }
end

function List:SetSize()
  if self.resizeW then
    local wd = 0
    for _,v in ipairs(self.drawitems) do
      local len = v.text:len() + (v.separator and 1 or 0)
      wd = max(wd, len)
      if wd > self.wmax then break end -- this check is needed for performance (with big lists && long lines)
    end
    self.w = max(self.margin:len() + wd + self.rmargin, self.fulltitle:len()+2,
                 self.bottom:len() + 2)
    self.w = min(self.w, self.wmax)
  else
    self.w = self.wmax
  end
  self.h = self.resizeH and min (#self.drawitems, self.hmax) or self.hmax
end

function List:OnResizeConsole (hDlg, consoleSize)
  if self.resizeScreen then
    self.wmax, self.hmax = max(4, consoleSize.X - 8), max(1, consoleSize.Y - 6)
    self:SetSize()
    self:SetUpperItem()
    DlgSend(hDlg, "DM_RESIZEDIALOG", 0, {X=self.w + 6, Y=self.h + 4})
  end
end

function List:AdjustEnds()
  if self.selalign == "top" then self:KeyHome()
  elseif self.selalign == "bottom" then self:KeyEnd()
  end
end

function List:FindUpperItem()
  for i,v in ipairs(self.drawitems) do
    if not v.separator then return i end
  end
  return 1
end

function List:FindLowerItem()
  for i=#self.drawitems,1,-1 do
    if not self.drawitems[i].separator then return i end
  end
  return #self.drawitems
end

-- calculate index of the upper element shown
function List:SetUpperItem ()
  if self.selignore and self.selalign ~= "center" then
    local item = self.sel and self.drawitems[self.sel]
    if not item or item.separator then
      self:AdjustEnds()
    end
  end
  if self.selalign == "top" then
    if self.selignore or not self.sel then
      self.sel = self:FindUpperItem()
    end
    self.upper = max(1, self.sel-self.h+1)
  elseif self.selalign == "bottom" then
    if self.selignore or not self.sel then
      self.sel = self:FindLowerItem()
    end
    self.upper = max(1, self.sel - self.h + 1)
  else -- "center"
    if self.selignore or not self.sel then
      self.sel = ceil(#self.drawitems / 2)
    end
    self.upper = self.sel - floor(self.h / 2)
    if self.upper < 1 then
      self.upper = 1
    else
      self.upper = max(1, min(self.upper, #self.drawitems-self.h+1))
    end
  end
end

function List:OnInitDialog (hDlg)
  if self.filterlines then
    self:ChangePattern(hDlg, self.pattern)
    self:AdjustEnds()
  else
    self:PrepareToDisplay(hDlg)
  end
end

function List:OnDrawDlgItem (x, y)
  self:Draw (x+self.x, y+self.y)
end

function List:OnGotFocus ()
  self.focused = true
end

function List:OnKillFocus ()
  self.focused = false
end

function List:OnDialogClose ()
  if self.timer then
    self.timer:Close()
    self.timer = nil
  end
end

do
  local ch = ("").char
  local sng = {c1=9484,c2=9488,c3=9492,c4=9496,hor=9472,ver=9474,sep1=9500,sep2=9508}
  local dbl = {c1=9556,c2=9559,c3=9562,c4=9565,hor=9552,ver=9553,sep1=9567,sep2=9570}
  for k,v in pairs(sng) do
    sng[k], dbl[k] = ch(v), ch(dbl[k])
  end
  local up, dn = ch(9650), ch(9660)
  local scr1, scr2 = ch(9617), ch(9619)

  function List:DrawBox (x, y)
    local T = self.focused and dbl or sng
    local color = self.col_text
    if self.mousestate ~= "drag_slider" then
      local len_ratio = #self.drawitems==0 and 0 or self.h/#self.drawitems
      local range = self.h - 2
      self.slider_len = max(floor(range * len_ratio + 0.5), 1)
      local hidden = #self.drawitems - self.h
      local offs_ratio = hidden==0 and 0 or (range-self.slider_len) / hidden
      self.slider_start = floor((self.upper-1) * offs_ratio + 0.5) -- 0-based
    end

    local function Horisontal (cleft, cright, text, yy)
      local mlen = text=="" and 0 or 1
      far.Text(x, yy, color, cleft) -- left corner
      local tlen = text:len()
      local len = max(ceil((self.w - 2*mlen - tlen)/2-0.5), 0)
      far.Text(x+1, yy, color, T.hor:rep(len))
      far.Text(x+1+len+mlen, yy, color, text:sub(1, self.w-1))
      local offs = len + tlen + 2*mlen
      far.Text(x+1+offs, yy, color, T.hor:rep(self.w - offs))
      far.Text(x+self.w+1, yy, color, cright) -- right corner
    end

    Horisontal(T.c1, T.c2, self.fulltitle, y)
    for k=1,self.h do
      local item = self.drawitems[self.upper+k-1]
      local sep = item and item.separator
      far.Text(x, y+k, color, sep and T.sep1 or T.ver)
      local c
      if self.h < 3 or self.h >= #self.drawitems then c = sep and T.sep2 or T.ver
      elseif k == 1 then c = up
      elseif k == self.h then c = dn
      else
        local m = k-2
        c = m >= self.slider_start and m < self.slider_start+self.slider_len and scr2 or scr1
      end
      far.Text(x+self.w+1, y+k, color, c)
    end
    Horisontal(T.c3, T.c4, self.bottom, y+self.h+1)
  end
end

function List:Draw (x, y)
  self:DrawBox(x, y)
  x, y = x+1, y+1-self.upper
  local mlen = self.margin:len()
  local char = ("").char
  local check, hor = char(8730), char(9472)
  for i=self.upper, self.upper+self.h-1 do
    local v = self.drawitems[i]
    if not v then break end
    local vdata = self.idata[v]
    local text = v.text or ""
    if v.separator then
      local color = self.col_text
      if text ~= "" then text = " " .. text:sub(1, self.w-4) .. " " end
      local tlen = text:len()
      local len1 = floor((self.w - tlen) / 2)
      far.Text(x, y+i, color, hor:rep(len1))
      far.Text(x+len1, y+i, color, text)
      far.Text(x+len1+tlen, y+i, color, hor:rep(self.w-len1-tlen))
    else
      local color = i==self.sel and self.col_selectedtext or self.col_text
      local color2 = i==self.sel and self.col_selectedhighlight or self.col_highlight
      local tlen = text:len()
      local maxlen = self.w - mlen - self.rmargin
      local text2, fr, to = text, vdata.fr, vdata.to
      if tlen > maxlen then
        maxlen = maxlen - 3
        local ss = self.searchstart - 1
        local br1 = ss + floor((maxlen - ss) * self.ellipsis/3 + 0.5)
        local br2 = tlen - (maxlen - br1) + 1
        text2 = text:sub(1, br1) .. "..." .. v.text:sub(br2)
        if fr and to >= fr then
          local offs = br2 - 1 - br1 - 3
          if fr >= br2-1 then fr = fr - offs
          elseif fr > br1+1 then fr = br1 + 2
          end
          if to >= br2-1 then to = to - offs
          elseif to > br1+1 then to = br1 + 2
          end
        end
      end
      far.Text(x, y+i, color, self.margin)
      if v.checked then far.Text(x, y+i, color, check) end
      if fr and to >= fr then
        far.Text(x+mlen,      y+i, color,  text2:sub(1, fr-1))
        far.Text(x+mlen+fr-1, y+i, color2, text2:sub(fr, to))
        far.Text(x+mlen+to,   y+i, color,  text2:sub(to+1))
      else
        far.Text(x+mlen,      y+i, color,  text2)
      end
      if i == self.sel then
        local start = mlen + text2:len()
        far.Text(x+start, y+i, color, (" "):rep(self.w - start))
      end
    end
  end
end

function List:MouseEvent (hDlg, Ev, x, y)
  if osWindows and Ev.EventFlags == F.MOUSE_WHEELED then
    self:KeyMsWheel(Ev.ButtonState < 0x80000000 and "up" or "down")
    SendRedrawMessage(hDlg)
    return
  end

  local X, Y = Ev.MousePositionX, Ev.MousePositionY
  local MOVED = (Ev.EventFlags == F.MOUSE_MOVED)

  -- A workaround: sometimes there frequently come parasite MOUSE_MOVED events
  --               having the same PositionX and PositionY ...
  if MOVED then
    if not self.MouseX then self.MouseX, self.MouseY = X, Y; return end
    if X==self.MouseX and Y==self.MouseY then return end
    self.MouseX, self.MouseY = X, Y
  end

  local LEFT = btest(Ev.ButtonState, "FROM_LEFT_1ST_BUTTON_PRESSED")
  x, y = x+self.x, y+self.y -- screen coordinates of component's top-left corner

  local function MakeScrollFunction (method)
    method(self, false)
    SendRedrawMessage(hDlg)
    local first = true
    return function(tmr)
      if first then
        first = false
        if osWindows then tmr.Interval=30 end -- setting timer.Interval is currently not supported in luafar2m
      end
      if not ( (method==self.KeyPageUp or method==self.KeyPageDown) and
               (Y >= y + 2 + self.slider_start) and
               (Y  < y + 2 + self.slider_start + self.slider_len) ) then
        method(self, false)
        SendRedrawMessage(hDlg)
      end
    end
  end

  if self.timer then
    if LEFT then return 1 end
    self.timer:Close()
    self.timer = nil
  end
  local inside = Y>y and Y<=y+self.h and X>x and X<=x+self.w
  ------------------------------------------------------------------------------
  if not self.mousestate then
    if LEFT then
      if not MOVED and
          ( (Y==y or Y==y+self.h+1) and (X >= x) and (X <= x+self.w+1)  or
            (X==x or X==x+self.w+1) and (Y >= y) and (Y <= y+self.h+1) )
      then
        -- click on border
        if #self.drawitems>self.h and X==x+self.w+1 and Y>y and Y<=y+self.h then
          -- click on scrollbar
          local period = osWindows and 300 or 50
          if Y == y + 1 then                           -- click on "up" arrow
            self.timer = far.Timer(period, MakeScrollFunction(self.KeyUp))
          elseif Y == y + self.h then                  -- click on "down" arrow
            self.timer = far.Timer(period, MakeScrollFunction(self.KeyDown))
          elseif Y < y + 2 + self.slider_start then    -- click above the slider
            self.timer = far.Timer(period, MakeScrollFunction(self.KeyPageUp))
          elseif Y >= y + 2 + self.slider_start + self.slider_len then -- click below the slider
            self.timer = far.Timer(period, MakeScrollFunction(self.KeyPageDown))
          else                                         -- click on slider
            -- start dragging slider
            self.clickY = Y
            self.mousestate = "drag_slider"
          end
        else
          -- start dragging dialog
          self.clickX, self.clickY = X, Y
          self.mousestate = "drag_dialog"
        end
        return 0
      end
    end

    if inside then
      if LEFT then
        self.mousestate = "inside"
      end
      if MOVED then
        local index = self.upper + (Y - y) - 1
        local item = self.drawitems[index]
        if item and not item.separator then self.sel = index end
        SendRedrawMessage(hDlg)
      end
    end
  ------------------------------------------------------------------------------
  elseif self.mousestate == "drag_dialog" then
    if LEFT then
      if MOVED then
        if self.clickX then
          DlgSend(hDlg, "DM_MOVEDIALOG", 0, { X = X - self.clickX, Y = Y - self.clickY })
          self.clickX, self.clickY = X, Y
        end
      end
      return 0
    else
      self.mousestate = nil
    end
  ------------------------------------------------------------------------------
  elseif self.mousestate == "drag_slider" then
    if LEFT then
      if MOVED and (Y ~= self.clickY) then
        local n = (self.h - 2) - self.slider_len
        self.slider_start = limit(self.slider_start + (Y - self.clickY), 0, n)
        self.upper = floor(1 + self.slider_start * (#self.drawitems - self.h) / n)
        self.sel = self.upper + self.slider_start
        self.clickY = Y
        SendRedrawMessage(hDlg)
      end
      return 0
    else
      self.mousestate = nil
    end
  ------------------------------------------------------------------------------
  elseif self.mousestate == "inside" then
    if LEFT then
      if inside then
        if MOVED then
          local index = self.upper + (Y - y) - 1
          local item = self.drawitems[index]
          if item and not item.separator then self.sel = index end
          SendRedrawMessage(hDlg)
        end
      end
    else
      self.mousestate = nil
      if inside then
        local index = self.upper + (Y - y) - 1
        local item = self.drawitems[index]
        if item and not item.separator then
          self.sel = index
          return 1, item, item and self.idata[item].index
        end
      end
    end
  ------------------------------------------------------------------------------
  else
    self.mousestate = nil
  end
  return 1
end

local function ProcessSearchMethod (method, pattern)
  local sNeedEscape = "[~!@#$%%^&*()%-+[%]{}\\|:;'\",<.>/?]"
  ----------------------------------------------------------
  if method == "dos" then
    pattern = pattern:gsub("%*+", "*"):gsub(sNeedEscape, "\\%1")
                     :gsub("\\[?*]", {["\\?"]=".", ["\\*"]=".*?"})
  elseif method == "plain" then
    pattern = pattern:gsub(sNeedEscape, "\\%1")
  elseif method == "lua" then
    local map = { l="%a", u="%a", L="%A", U="%A" }
    if ("").charpattern then -- luautf8 library (since 30-Aug-2019)
      return function(s, p, init)
        p = p:gsub("(%%?)(.)", function(a,b) return a=="" and b:lower() or map[b] end)
        local ok, fr, to = pcall(("").find, s:lower(), p, init)
        if ok then return fr, to end
        return nil
      end
    else -- Selene Unicode library (prior to 30-Aug-2019)
      return function(s, p, init)
        p = p:gsub("(%%?)(.)", function(a,b) return a=="" and b:lower() or map[b] end)
        local ok, fr, to = pcall(("").find, s:lower(), p, #(s:sub(1, init-1)) + 1)
        if not (ok and fr) then return nil end
        return string.sub(s,1,fr-1):len()+1, string.sub(s,1,to):len()
      end
    end
  elseif method ~= "regex" then
    error("invalid search method")
  end
  ----------------------------------------------------------
  local ok, cregex = pcall(regex.new, pattern, "i")
  if ok then
    return function (text, patt, start)
      return cregex:find(text, start)
    end
  end
end

function List:UpdateSizePos (hDlg)
  self:SetSize()

  local dim
  local resize = self.resizeW or self.resizeH

  if resize then
    dim = DlgSend(hDlg, "DM_RESIZEDIALOG", 0, { X=self.w+6, Y=self.h+4 })
    self.w = min (dim.X-6, self.w)
    self.h = min (dim.Y-4, self.h)
  end

  self:SetUpperItem()

  if self.autocenter then
    DlgSend(hDlg, "DM_MOVEDIALOG", 1, { X=-1, Y=-1 })
  end

  if resize then
    DlgSend(hDlg, "DM_SETITEMPOSITION", self.startId, { Left=2, Top=1, Right=dim.X-3, Bottom=dim.Y-2 })
  end

  if not (resize or self.autocenter) then
    DlgSend(hDlg, "DM_REDRAW") -- ensure this function always causes a redraw
  end
end

local function GetDate (filetime)
  if filetime then
    local ft = win.FileTimeToLocalFileTime(filetime)
    return ft and win.FileTimeToSystemTime(ft)
  end
end

function List:ChangePattern (hDlg, pattern)
  if pattern then self.pattern = pattern end
  local oldsel = self.drawitems[self.sel]
  local numItems = 0
  self.drawitems = {}
  self.sel = nil

  local find, find2
  local pat2 = self.xlat and XLat(pattern)
  if type(self.searchmethod)=="function" then
    find, find2 = self.searchmethod, self.searchmethod
  else
    find = ProcessSearchMethod(self.searchmethod, pattern)
    find2 = pat2 and ProcessSearchMethod(self.searchmethod, pat2)
  end

  if find or find2 then
    local groupdate
    for _,v in ipairs(self.items) do
      local fr, to
      if find then
        fr, to = find(v.text, pattern, self.searchstart)
      end
      if fr==nil and find2 then
        fr, to = find2(v.text, pat2, self.searchstart)
      end
      local vdata = self.idata[v]
      vdata.fr, vdata.to = fr, to
      if fr then
        if self.showdates then
          local ft = GetDate(v.time)
          if ft then
            local date = (ft.wYear*12 + ft.wMonth)*31 + ft.wDay
            if not groupdate or (self.selalign=="bottom" and groupdate < date)
                             or (self.selalign=="top" and groupdate > date) then
              groupdate = date
              ft.year,ft.month,ft.day = ft.wYear,ft.wMonth,ft.wDay
              local text = os.date(self.dateformat, os.time(ft))
              self.drawitems[#self.drawitems+1] = { separator=true; text=text; }
            end
          end
        end
        numItems = numItems + 1
        self.drawitems[#self.drawitems+1] = v
        if oldsel == v then
          self.sel = #self.drawitems
        end
      end
    end
  end
  self.fulltitle = (self.pattern == "") and self.title or self.title.." ["..self.pattern.."]"
  self.bottom = ("%d of %d items [%s:%s, %s:xlat=%s]"):format(
    numItems,
    #self.items,
    self.keys_searchmethod or "",
    self.searchmethod,
    self.keys_xlatonoff or "",
    self.xlat and "on" or "off")
  self:UpdateSizePos(hDlg)
end

function List:PrepareToDisplay (hDlg)
  self.drawitems = {}
  for _,v in ipairs(self.items) do
    local vdata = self.idata[v]
    vdata.fr, vdata.to = v.fr, v.to
    self.drawitems[#self.drawitems+1] = v
  end
  self.fulltitle = self.title
  self:UpdateSizePos(hDlg)
end

local ConvertTable = {
  Space=" ", BackSlash="\\", Divide="/", Multiply="*", Subtract="-", Add="+",
}

function List:KeyDown (wrap)
  local oldsel = self.sel
  for i = self.sel+1, #self.drawitems do
    local v = self.drawitems[i]
    if not v.separator then
      self.sel = i
      self.upper = max(self.upper, self.sel - self.h + 1)
      break
    end
  end
  if wrap and self.sel==oldsel and btest(self.flags, "FMENU_WRAPMODE") then
    for i=1, self.sel-1 do
      local v = self.drawitems[i]
      if not v.separator then
        self.sel = i
        self.upper = max(1, self.sel - self.h + 1)
        break
      end
    end
  end
end

function List:KeyUp (wrap)
  local oldsel = self.sel
  for i = self.sel-1, 1, -1 do
    local v = self.drawitems[i]
    if not v.separator then
      self.sel = i
      self.upper = min(self.upper, self.sel)
      break
    end
  end
  if wrap and self.sel==oldsel and btest(self.flags, "FMENU_WRAPMODE") then
    for i = #self.drawitems, self.sel+1, -1 do
      local v = self.drawitems[i]
      if not v.separator then
        self.sel = i
        self.upper = max(1, self.sel - self.h + 1)
        break
      end
    end
  end
end

function List:KeyPageDown()
  local oldsel = self.sel
  for i = self.sel+self.h, #self.drawitems do
    local v = self.drawitems[i]
    if not v.separator then
      self.sel = i
      self.upper = self.upper + self.sel - oldsel
      self.upper = max(1, min(self.upper, #self.drawitems - self.h + 1))
      break
    end
  end
  if self.sel == oldsel then
    for i = self.sel+self.h-1, self.sel+1, -1 do
      local v = self.drawitems[i]
      if v and not v.separator then
        self.sel = i
        self.upper = max(self.upper, self.sel - self.h + 1)
        break
      end
    end
  end
end

function List:KeyPageUp()
  local oldsel = self.sel
  for i = self.sel-self.h, 1, -1 do
    local v = self.drawitems[i]
    if not v.separator then
      self.sel = i
      self.upper = max(1, self.upper + self.sel - oldsel)
      break
    end
  end
  if self.sel == oldsel then
    for i = self.sel-self.h+1, self.sel-1 do
      local v = self.drawitems[i]
      if v and not v.separator then
        self.sel = i
        self.upper = max(1, self.upper - self.h)
        break
      end
    end
  end
end

function List:KeyHome()
  self.upper = 1
  for i,v in ipairs(self.drawitems) do
    if not v.separator then
      self.sel = i
      self.upper = max(1, self.sel - self.h + 1)
      break
    end
  end
end

function List:KeyEnd()
  self.upper = max(1, #self.drawitems - self.h + 1)
  for i = #self.drawitems, 1, -1 do
    local v = self.drawitems[i]
    if not v.separator then
      self.sel = i
      break
    end
  end
end

function List:KeyMsWheel (dir)
  local N = #self.drawitems
  if N > 0 then
    for k=1,2 do -- loop to skip a separator
      if dir == "up" then
        if self.sel==1 or self.sel==2 and self.drawitems[1].separator then
          break
        end
        self.upper = max(1, self.upper-1)
        self.sel = self.sel-1
      else
        if self.sel==N then
          break
        end
        self.upper = min(max(1, N-self.h+1), self.upper+1)
        self.sel = self.sel+1
      end
      if not self.drawitems[self.sel].separator then
        break
      end
    end
  end
end

function List:DeleteCurrentItem (hDlg)
  local Item = self.drawitems[self.sel]
  local index = Item and self.idata[Item].index
  if index then
    local items, idata = self.items, self.idata
    for k=index+1,#items do
      local t = idata[items[k]]
      t.index = t.index - 1
    end
    table.remove(items, index)
    idata[Item] = nil

    local old_sel, old_upper = self.sel, self.upper
    local old_frame = min(#self.drawitems, self.h)
    self:ChangePattern(hDlg, self.pattern)

    local num = #self.drawitems
    if num > 1 then
      local frame = min(num, self.h)
      if frame == old_frame then
        if old_upper + old_frame - 1 < num + 1 then
          self.upper, self.sel = old_upper, old_sel
        else
          self.upper, self.sel = old_upper-1, old_sel
          if self.sel > num then self.sel = num end
        end
      else
        if old_sel < num + 1 then self.sel = old_sel
        else self.sel = old_sel - 1
        end
      end

      local item = self.drawitems[self.sel]
      if item and item.separator then
        self:KeyDown()
      elseif self.sel == #self.drawitems then
        self:KeyEnd()
      end
    end
  end
end

-- Delete all currently displayed items
function List:DeleteFilteredItems (hDlg, bConfirm)
  if self.drawitems[1] then
    if not bConfirm or far.Message(("Are you sure to delete %d items?"):format(#self.drawitems),
                                    "Delete items", ";YesNo", "w") == 1 then
      local items, idata = self.items, self.idata
      for _,v in ipairs(self.drawitems) do
        items[idata[v].index] = false
        idata[v] = nil
      end
      local shift = 0
      for i,v in ipairs(items) do
        if v == false then
          shift = shift + 1
        elseif shift > 0 then
          items[i-shift] = v
          idata[v].index = i-shift
        end
      end
      for i=#items,#items-shift+1,-1 do items[i] = nil end
      self:ChangePattern(hDlg, self.pattern)
    end
  end
end

-- Delete some items from the currently displayed items
function List:DeleteNonexistentItems (hDlg, fExist, fConfirm)
  if self.drawitems[1] then
    local n = 0
    local items, idata = self.items, self.idata
    -- mark nonexistent items
    for _,v in ipairs(self.drawitems) do
      if not (v.separator or fExist(v)) then
        idata[v].marked = true
        n = n + 1
      end
    end
    if n > 0 then
      if not fConfirm or fConfirm(n) then
        for k,v in pairs(idata) do
          if v.marked then
            items[v.index] = false
            idata[k] = nil
          end
        end
        local shift = 0
        for i,v in ipairs(items) do
          if v == false then
            shift = shift + 1
          elseif shift > 0 then
            items[i-shift] = v
            idata[v].index = i-shift
          end
        end
        for i=#items,#items-shift+1,-1 do items[i] = nil end
        self:ChangePattern(hDlg, self.pattern)
      else
        for _,v in pairs(idata) do v.marked = nil end
      end
    end
  end
end

function List:CopyItemToClipboard()
  local Item = self.drawitems[self.sel]
  if Item then far.CopyToClipboard(Item.text:sub(self.searchstart)) end
end

function List:CopyFilteredItemsToClipboard()
  local t = {}
  for k,v in ipairs(self.drawitems) do
    if not v.separator then
      t[#t+1] = v.text
    end
  end
  t[#t+1] = ""
  far.CopyToClipboard(table.concat(t, "\n"))
end

function List:ToggleSearchMethod (hDlg)
  if     self.searchmethod == "dos"   then self.searchmethod = "lua"
  elseif self.searchmethod == "lua"   then self.searchmethod = "regex"
  elseif self.searchmethod == "regex" then self.searchmethod = "plain"
  else self.searchmethod = "dos"
  end
  self:ChangePattern(hDlg, self.pattern)
end

local function FindKey (t, key)
  if t == key then return key end
  if type(t) == "table" then
    for _,v in ipairs(t) do
      if v == key then return key end
    end
  end
end

function List:Key (hDlg, key)
  if self.debug then
    self.Log(("%s%s"):format(("  "):rep(self.depth), key))
  end
  local Item = self.drawitems[self.sel]
  if self.keyfunction then
    local ret = self:keyfunction(hDlg, key, Item)
    if ret == "break" then return { BreakKey=key }, Item and self.idata[Item].index end
    if ret == "done" then return "done" end
  end

  if key == "Home" or key == "Num7" then
    self:KeyHome()
  elseif key == "End" or key == "Num1" then
    self:KeyEnd()
  elseif key == "Down" or key == "Right" or key == "Num2" or key == "Num6" then
    self:KeyDown(true)
  elseif key == "Up" or key == "Left" or key == "Num8" or key == "Num4" then
    self:KeyUp(true)
  elseif key == "PgDn" or key == "Num3" then
    self:KeyPageDown()
  elseif key == "PgUp" or key == "Num9" then
    self:KeyPageUp()
  elseif key == "MsWheelDown" then
    self:KeyMsWheel("down")
  elseif key == "MsWheelUp" then
    self:KeyMsWheel("up")

  elseif key == "Enter" or key == "NumEnter" then
    return Item, Item and self.idata[Item].index

  elseif FindKey(self.keys_copyitem, key) then
    self:CopyItemToClipboard()

  elseif FindKey(self.keys_copyfiltereditems, key) then
    self:CopyFilteredItemsToClipboard()

  elseif FindKey(self.keys_checkitem, key) then
    if Item then Item.checked = not Item.checked or nil end

  elseif FindKey(self.keys_ellipsis, key) then
    self.ellipsis = (self.ellipsis + 1) % 4

  elseif FindKey(self.keys_showitem, key) then
    if Item then far.Message(Item.text:sub(self.searchstart), "Full Item Text", ";Ok") end

  elseif self.filterlines then

    if FindKey(self.keys_clearpattern, key) and self.pattern ~= "" then
      self:ChangePattern(hDlg, "")

    elseif FindKey(self.keys_insertpattern, key) then
      local str = far.PasteFromClipboard()
      self:ChangePattern(hDlg, str and str:match("^[^\r\n]*") or "")

    elseif key == "BS" and self.pattern ~= "" then
      self:ChangePattern(hDlg, self.pattern:sub(1,-2))

    elseif FindKey(self.keys_searchmethod, key) then
      self:ToggleSearchMethod(hDlg)

    elseif FindKey(self.keys_deleteitem, key) then
      self:DeleteCurrentItem(hDlg)

    elseif FindKey(self.keys_delfiltereditems, key) then
      self:DeleteFilteredItems(hDlg, true)

    elseif FindKey(self.keys_xlatonoff, key) then
      self.xlat = not self.xlat
      self:ChangePattern(hDlg, self.pattern)

    elseif FindKey(self.keys_applyxlat, key) then
      local result = XLat(self.pattern, nil, nil, "XLAT_SWITCHKEYBLAYOUT")
      if result then self:ChangePattern(hDlg, result) end

    elseif key:len() == 1 then
      self:ChangePattern(hDlg, self.pattern..key)

    elseif ConvertTable[key] then
      self:ChangePattern(hDlg, self.pattern..ConvertTable[key])

    end
  end

  if self.onlistchange then
    local CurItem = self.drawitems[self.sel]
    if CurItem ~= Item then self:onlistchange(hDlg, key, CurItem) end
  end
end

function List:SetIndexData()
  self.idata = {}
  for i,v in ipairs(self.items) do self.idata[v] = { index=i } end
end

function List:Sort (fCompare)
  table.sort(self.items, fCompare)
  self:SetIndexData()
end

local function Menu (props, list)
  assert(type(props) == "table")
  assert(type(list) == "table")

  list.title  = props.Title or list.title
  list.bottom = props.Bottom or list.bottom
  list.sel    = props.SelectIndex or list.sel
  list.flags  = FlagsToInt(props.Flags)

  list.autocenter = (list.autocenter ~= false)
  list.resizeW    = (list.resizeW ~= false)
  list.resizeH    = (list.resizeH ~= false)

  local ret, ret_item, ret_pos
  local Rect
  local Items = {  -- a hidden element for setting console title
    { F.DI_TEXT,  1,1,8,1,  0,0,0,F.DIF_HIDDEN, "" },
    list:CreateDialogItems(2, 1),
  }
  local pos_title, pos_usercontrol = 1, 2
  list.startId = pos_usercontrol
  ------------------------------------------------------------------------------
  local function DlgProc (hDlg, msg, param1, param2)
    if msg == F.DN_INITDIALOG then
      list.Log("DN_INITDIALOG")
      DlgSend(hDlg, F.DM_SETINPUTNOTIFY or F.DM_SETMOUSEEVENTNOTIFY, 1) -- keep flag backward compatibility
      list:OnInitDialog (hDlg)

    elseif osWindows and msg == F.DN_GETVALUE then
      if param1 == pos_usercontrol then
        local tp = param2.GetType
        if tp == 7 then                             -- get CurPos
          return { ValType=F.FMVT_INTEGER, Value=list.sel }
        elseif tp == 0 or tp == 10 then             -- get item text
          local item = list.drawitems[list.sel]
          return item and { ValType=F.FMVT_STRING, Value=item.text or "" }
        elseif tp == 11 then                        -- get ItemCount
          return { ValType=F.FMVT_INTEGER, Value=#list.drawitems }
        end
      end

    elseif msg == F.DN_DRAWDIALOG then
      list.Log("DN_DRAWDIALOG")
      Rect = DlgSend(hDlg, "DM_GETDLGRECT")

    elseif msg == F.DN_CTLCOLORDIALOG then
      list.Log("DN_CTLCOLORDIALOG")
      return list.col_text

    elseif msg == F.DN_DRAWDLGITEM then
      list.Log("DN_DRAWDLGITEM")
      if param1 == pos_usercontrol then
        list:OnDrawDlgItem (Rect.Left, Rect.Top)
        DlgSend(hDlg, "DM_SETTEXT", pos_title, list.fulltitle)
      end

    elseif not osWindows and msg == F.DN_KEY then
      list.Log("DN_KEY")
      if param1 == pos_usercontrol then
        local key = far.KeyToName(param2)
        if not key then return end
        ret_item, ret_pos = list:Key(hDlg, key)
        if ret_item == "done" then
          return true
        elseif ret_item then
          if key~="Enter" and key~="NumEnter" then -- prevent DN_CLOSE from coming twice
            DlgSend(hDlg, "DM_CLOSE", -1, 0)
          end
        else
          SendRedrawMessage(hDlg)
        end
      end

    elseif not osWindows and msg == F.DN_MOUSEEVENT then
      list.Log("DN_MOUSEEVENT")
      ret, ret_item, ret_pos = list:MouseEvent(hDlg, param2, Rect.Left, Rect.Top)
      if ret_item then DlgSend(hDlg, "DM_CLOSE", -1, 0) end
      return ret

    elseif osWindows and (msg == F.DN_CONTROLINPUT or msg == F.DN_INPUT) then
      if param2.EventType == F.KEY_EVENT then
        if param1 == pos_usercontrol then
          local key = far.InputRecordToName(param2)
          -- far.Show(
          --   "far.InputRecordToName(param2): "..tostring(key),
          --   param2.EventType,
          --   param2.KeyDown,
          --   param2.RepeatCount,
          --   param2.VirtualKeyCode,
          --   param2.VirtualScanCode,
          --   param2.UnicodeChar,
          --   param2.ControlKeyState
          -- )
          if not key then return end
          ret_item, ret_pos = list:Key(hDlg, key)
          if ret_item == "done" then
            return true
          elseif ret_item then
            if key~="Enter" and key~="NumEnter" then -- prevent DN_CLOSE from coming twice
              DlgSend(hDlg, "DM_CLOSE")
            end
          else
            SendRedrawMessage(hDlg)
          end
        end
      elseif param2.EventType == F.MOUSE_EVENT then
        if param1 ~= -1 then --> -1 = click outside the dialog
          ret, ret_item, ret_pos = list:MouseEvent(hDlg, param2, Rect.Left, Rect.Top)
          if ret_item then DlgSend(hDlg, "DM_CLOSE") end
          return ret
        end
      end

    elseif msg == F.DN_GOTFOCUS then
      list.Log("DN_GOTFOCUS")
      list:OnGotFocus()

    elseif msg == F.DN_KILLFOCUS then
      list.Log("F.DN_KILLFOCUS")
      list:OnKillFocus()

    elseif msg == F.DN_RESIZECONSOLE then
      list.Log("DN_RESIZECONSOLE")
      list:OnResizeConsole(hDlg, param2)
      return 1

    elseif msg == F.DN_CLOSE then
      list.Log("DN_CLOSE")
      local canclose = true
      if param1 == pos_usercontrol and type(list.CanClose) == "function" and ret_item and ret_pos then
        canclose = list:CanClose(list.items[ret_pos], ret_item.BreakKey)
      end
      if canclose then
        list:OnDialogClose(); return 1
      end
      return 0

    end
  end
  ----------------------------------------------------------------------------
  local X1, Y1, X2, Y2 = -1, -1, list.w+6, list.h+4
  if props.X and props.Y then
    X1, Y1 = props.X, props.Y
    X2, Y2 = X1+X2-1, Y1+Y2-1
  end

  ret = far.Dialog(props.DialogId, X1, Y1, X2, Y2, props.HelpTopic, Items, 0, DlgProc)
  if ret == pos_usercontrol then
    return ret_item, ret_pos
  end
  return nil
end

return {
  Menu = Menu;
  NewList = NewList;
}
