-- started: 2011-10-30

-- Message box implementation (as opposed to Far's `Message' function binding).

-- TODO
-- 1. Multiple button lines (the initial reason for writing this module). ==> Done 2011-10-31.
-- 2. (Optional, flag-driven) line wrapping (word-wise).                  ==> Done 2011-11-01.
-- 3. (Optional, flag-driven) colorizing individual text lines.           ==> Done 2011-11-02.
-- 4. Document this module.                                               ==> Done 2011-11-03.
-- 5. Implement drawing separator lines (single and double).              ==> Done 2011-11-03.
-- 6. Buttons argument can contain '\n' to force a new button line.       ==> Done 2011-11-05.
-- 7. Lifted restriction of "every new element is placed on a new line".  ==> Done 2011-11-07.


local F = far.Flags
local bnot, band, bor = bit64.bnot, bit64.band, bit64.bor
local min, max = math.min, math.max
local STARTX, STARTY = 5, 2

-- Dialog API constants
local IDX_X1, IDX_Y1, IDX_X2, IDX_Y2, IDX_FLAGS, IDX_DATA = 2,3,4,5,9,10

local function Label (x1, y1, text, color)
  return {"DI_TEXT", x1,y1,0,y1, 0,0,0,F.DIF_SHOWAMPERSAND, text, color=color}
end

local function Separator (y1, kind, text, color)
  local flags = bor(F.DIF_SHOWAMPERSAND, kind==2 and F.DIF_SEPARATOR2 or F.DIF_SEPARATOR)
  return {"DI_TEXT", -1,y1,-1,y1, 0,0,0,flags, text or "", color=color}
end

local function WrapText (aText, aMaxLen, aMaxLen1, aMaxItems)
  aMaxItems = aMaxItems or math.huge
  local function sub(from,to) return aText:sub(from,to) end
  local items = {}
  local lastSpace, lastDelim
  local start, pos, maxlen = 1, 1, (aMaxLen1 or aMaxLen)

  while #items < aMaxItems do
    local char = sub(pos, pos)
    if char == "" then -- end of the entire message
      items[#items+1] = sub(start)
      break
    elseif char == '\n' or char == '\r' then  -- end of the line
      items[#items+1] = sub(start, pos-1)
      start = pos + 1
      if char=='\r' and sub(start,start)=='\n' then start = start+1 end
      maxlen, pos, lastSpace, lastDelim = aMaxLen, start, nil, nil
    elseif pos-start < maxlen then            -- characters inside the line
      if char==' ' or char=='\t' then lastSpace = pos
      elseif char:find('[^%w_]') then lastDelim = pos
      end
      pos = pos + 1
    else                                      -- 1-st character beyond the line
      pos = lastSpace or lastDelim or pos-1
      items[#items+1] = sub(start, pos)
      start = pos + 1
      maxlen, pos, lastSpace, lastDelim = aMaxLen, start, nil, nil
    end
  end
  return items
end

local function AddElement (self, aElement, aTarget, aMaxLines)
  local start_y = self.y
  aMaxLines = aMaxLines or math.huge
  local text, color, separator
  if type(aElement) == "table" then
    text = aElement.text==nil and "" or tostring(aElement.text)
    color, separator = aElement.color, aElement.separator
  else
    text = tostring(aElement)
  end
  if separator then
    if aMaxLines > 0 then
      text = text:sub(1, self.maxchars):match("[^\r\n]*")
      if self.x ~= STARTX then self.y = self.y + 1 end
      aTarget[#aTarget+1] = Separator(self.y, separator, text, color)
      self.x, self.y = STARTX, self.y + 1
    end
  elseif self.bWrapLines then
    local items = WrapText(text, self.maxchars, self.maxchars+STARTX-self.x, aMaxLines)
    local w = 0
    for i,v in ipairs(items) do
      if i == 2 then self.x = STARTX end
      w = v:len()
      if v ~= "" then
        aTarget[#aTarget+1] = Label(self.x, self.y+i-1, v, color)
      end
    end
    self.y = self.y + #items - 1
    self.x = self.x + w
  else
    local added_lines = 0
    for line, eol in text:gmatch("([^\r\n]*)(\r?\n?)") do
      if added_lines >= aMaxLines then break end
      if line ~= "" then
        local w = min(line:len(), self.maxchars - self.x + STARTX)
        aTarget[#aTarget+1] = Label(self.x, self.y, line:sub(1,w), color)
        self.x = self.x + w
      end
      if eol ~= "" then
        self.x, self.y = STARTX, self.y + 1
        added_lines = added_lines + 1
      end
    end
  end
  return self.y - start_y
end

--[[
@aText      : Text elements to display inside the dialog frame.
              Either a string or a table, depending on flag 'c'.
              Sequences '\n', '\r\n' and '\r' are treated as line separators.
@aTitle     : Title string; optional.
@aButtons   : Buttons string; ';' and '\n' serve as button separators; optional.
              Buttons automatically wrap on multiple lines if don't fit on one
              line. Separator '\n' forces new line for the next button.
@aFlags     : Concatenation of 0 or more character flags; optional.
              'l' - left-align text lines (default: center lines on the dialog).
              'w' - use "warning" color set for the dialog and its elements.
              'R' - don't wrap long text lines (default: wrap).
              'c' - "color"-mode, that changes treating the aText argument. With
                 this flag set, aText should be an array of individual elements,
                 each of which is either a string or a table.
                 A table elements may have the following fields:
                    "text" (string)
                    "color" (number; optional)
                    "separator" (1 = single line, 2 = double line; optional)
                 Each element's begins at the position next to the previous
                 element's end. Separators are always put on separate lines.
@aHelpTopic : Help topic string; optional.
@aId        : Dialog Id; binary GUID string; optional.

@returns    : negative number when dialog was canceled, button number otherwise
              (1 is the first button).
--]]

local function Message (aText, aTitle, aButtons, aFlags, aHelpTopic, aId)
  if aFlags and aFlags:find("c") then
    assert(type(aText)=="table", "argument #1 must be table when flag 'c' is specified")
  else
    aText = { tostring(aText) }
  end
  aTitle = aTitle or "Message"
  aButtons = aButtons or "OK"
  aId = aId or win.Uuid("bee50a78-be62-418d-95f0-a84982ac268e")

  local data do
    local sb = win.GetConsoleScreenBufferInfo()
    local MAXWIDTH = max(8, sb.WindowRight - sb.WindowLeft - 3)
    local MAXHEIGHT = max(7, sb.WindowBottom - sb.WindowTop - 1)
    data = {
      bWrapLines = not (aFlags and aFlags:find("R")),
      maxlines = MAXHEIGHT - STARTY - 2,
      maxchars = MAXWIDTH - STARTX - 4,
      y = STARTY,
      x = STARTX,
    }
  end

  -- Calculate buttons.
  local tb_buttons = {}
  local btnlen, maxbtnlen, btnlines, numbuttons = 0, 0, 0, 0

  local function putfirstbutton (btn)
    btnlines = btnlines + 1
    btnlen = min(btn:gsub("&",""):len() + 4, data.maxchars)
    maxbtnlen = max(maxbtnlen, btnlen)
    if btnlen == data.maxchars then btnlen = 0 end
  end

  for btn, delim in aButtons:gmatch("([^;\n]+)([;\n]?)") do
    if btnlines >= data.maxlines then break end
    numbuttons = numbuttons + 1
    if btnlen == 0 then -- new line
      putfirstbutton (btn)
    else
      btnlen = btnlen + btn:gsub("&",""):len() + 5
      if btnlen == data.maxchars then
        maxbtnlen = data.maxchars
        btnlen = 0
      elseif btnlen > data.maxchars then
        putfirstbutton (btn)
      else
        maxbtnlen = max(maxbtnlen, btnlen)
        if delim == "\n" then btnlen = 0 end
      end
    end
    tb_buttons[numbuttons] = {"DI_BUTTON",  0,btnlines,0,0,  0,0,0,F.DIF_CENTERGROUP, btn}
  end

  -- Calculate text lines.
  local nseparator = btnlines==0 and 0 or 1
  local numlines = data.maxlines - btnlines - nseparator
  local heights, tb_labels = {}, {}
  for i, elem in ipairs(aText) do
    heights[i] = AddElement(data, elem, tb_labels, 500)
  end
  if aText.callback and aText.callback(heights, numlines, data.y-STARTY+1) then
    data.y, data.x = STARTY, STARTX
    tb_labels = {}
    for i, elem in ipairs(aText) do
      AddElement(data, elem, tb_labels, heights[i] + 1)
    end
  end

  -- Add labels.
  numlines = min(numlines, data.y-STARTY+1)
  local numchars = 0
  local D = {{"DI_DOUBLEBOX",  3,1,0,0,  0,0,0,0,  aTitle},}
  for _,v in ipairs(tb_labels) do
    if v[IDX_Y1] > numlines + STARTY - 1 then break end
    D[#D+1] = v
    numchars = max(numchars, v[IDX_X1] - STARTX + v[IDX_DATA]:len())
  end

  -- Add a separator and buttons.
  if numbuttons > 0 then
    D[#D+1] = Separator(numlines + STARTY)
    numchars = max(numchars, maxbtnlen)
    for k=1,numbuttons do
      local btn = tb_buttons[k]
      btn[IDX_Y1] = btn[IDX_Y1] + numlines + STARTY
      D[#D+1] = btn
    end
    local btn1 = tb_buttons[1]
    btn1[IDX_FLAGS] = bor(btn1[IDX_FLAGS], F.DIF_DEFAULTBUTTON)
  end

  numchars = min(data.maxchars, max(numchars, aTitle:len()+2))
  D[1][IDX_Y2] = numlines + btnlines + nseparator + STARTY
  D[1][IDX_X2] = numchars + STARTX + 1

  if not (aFlags and aFlags:find("l")) then -- centered text
    local idx1, y1
    for k=1,#tb_labels+1 do -- upper limit is incremented on purpose
      local label = tb_labels[k]
      if idx1 then
        if not label or label[IDX_Y1] > y1 then
          local w = tb_labels[k-1][IDX_X1] - tb_labels[idx1][IDX_X1] - 1 + tb_labels[k-1][IDX_DATA]:len()
          local delta = math.floor((numchars-w)/2)
          for m=idx1,k-1 do tb_labels[m][IDX_X1] = tb_labels[m][IDX_X1] + delta end
          if label then idx1, y1 = k, label[IDX_Y1] end
        end
      else
        if label then idx1, y1 = 1, label[IDX_Y1] end
      end
    end
  end

  local function DlgProc (hDlg, msg, param1, param2)
    if msg == F.DN_CTLCOLORDLGITEM then
      if param1 > 1 and param1 <= #tb_labels+1 then
        local color = D[param1].color
        if color then
          for i=1,#param2 do param2[i] = color end
          return param2
        end
      end
    end
  end

  local dflags = aFlags and aFlags:find("w") and F.FDLG_WARNING or 0
  local ret = far.Dialog(aId, -1, -1, D[1][IDX_X2]+4, D[1][IDX_Y2]+2, aHelpTopic, D, dflags, DlgProc)
  return ret < 0 and ret or (ret - (#D - numbuttons))
end

local function GetMaxChars()
  local sb = win.GetConsoleScreenBufferInfo()
  local MAXWIDTH = max(8, sb.WindowRight - sb.WindowLeft - 3)
  return MAXWIDTH - 9
end

--[[
Display a two-column table.
@items: an array of rows (tables); row[1],row[2] = left and right column texts.
        A row can also be a single or double separator line if row.separator is
        1 or 2; a separator can have optional field row.text.
@flags: 'T' - righT-align the right column.
--]]
local function TableBox (items, title, buttons, flags, helptopic, id)
  local LEN1, LEN2 = 0, 0
  local out = {}
  flags = (flags or "") .. "lc"
  local rightAlign = flags:find("T")
  for _,v in ipairs(items) do
    if not v.separator then
      local v1,v2 = tostring(v[1]),tostring(v[2])
      local n1,n2 = v1:len(),v2:len()
      if LEN1 < n1 then LEN1 = n1 end
      if LEN2 < n2 then LEN2 = n2 end
    end
  end
  local nl = false
  for _,v in ipairs(items) do
    if v.separator then
      out[#out+1], nl = v, false
    else
      local v1,v2 = tostring(v[1]),tostring(v[2])
      if nl then out[#out+1] = "\n" end
      out[#out+1] = v1
      if rightAlign then
        out[#out+1] = (" "):rep(LEN1+2-v1:len()+LEN2-v2:len())
      else
        out[#out+1] = (" "):rep(LEN1+2-v1:len())
      end
      out[#out+1] = v2
      nl = true
    end
  end
  return Message(out, title, buttons, flags, helptopic, id)
end

local function GetInvertedColor (element)
  local color = actl.GetColor(F[element])
  return band(bnot(color), 0xFF)
end

return {
  GetInvertedColor = GetInvertedColor,
  GetMaxChars = GetMaxChars,
  Message = Message,
  TableBox = TableBox,
}
