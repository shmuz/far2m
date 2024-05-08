-- Started: 2024-01-20

local TopTitle = "Configuration editor"
local BottomTitle = "F4 Shift+F4 Del Ctrl+H"
local CheckChar = string.byte "*"

local sd = require"far2.simpledialog"
local F = far.Flags
local band, bor, bnot = bit64.band, bit64.bor, bit64.bnot

local function MakeItem(idx)
  local val,tp,val0,key,name,saved = Far.GetConfig(idx)
  if val then
    local txt = ("%-42s│%-8s│%s │"):format(key.."."..name, tp, saved and "s" or "-")
    if tp == "integer" or tp == "string" or tp == "boolean" or tp == "3-state" then
      if tp == "integer" then
        txt = ("%s%d = 0x%X"):format(txt, val, val)
      elseif tp == "string" then
        txt = txt .. val
      elseif tp == "boolean" then
        txt = txt .. (val == 0 and "false" or "true")
      elseif tp == "3-state" then
        local num = val % 3
        txt = txt .. (num==0 and "false" or num==1 and "true" or "other")
      end
      local item = { Text=txt; configIndex=idx; Flags=0; }
      if val ~= val0 then item.Flags = F.LIF_CHECKED + CheckChar; end
      return item
    else
      return "none"
    end
  end
end

local function EditValue(asHex, key, name, tp, val0, val)
  local items = {
    width = tp=="integer" and 40 or 76;
    {tp="dbox"; },                                           -- 1
    {tp="text"; text= ("%s.%s (%s)"):format(key,name,tp); }, -- 2
    {tp="edit"; val=tostring(val); },                        -- 3
    {tp="sep"; },                                            -- 4
    {tp="butt"; centergroup=1; default=1; text="OK"; },      -- 5
    {tp="butt"; centergroup=1;            text="Reset"; },   -- 6
    {tp="butt"; centergroup=1; cancel=1;  text="Cancel"; },  -- 7
  }
  local posEdit, posOK = 3, 5

  if tp == "integer" then
    if asHex then
      items[posEdit] = {tp="fixedit"; mask="0xHHHHHHHH"; val=("%X"):format(val); }
    else
      items[posEdit] = {tp="fixedit"; mask="9999999999"; val=tostring(val); }
    end
  end
  items[posEdit].name = "result"

  local closeaction = function(hDlg, Par1, tOut)
    if Par1 == posOK and tp == "integer" then
      local v = tonumber(tOut.result)
      if v == nil then
        far.Message("No valid integer entered", "Error", nil, "w");
        return 0
      elseif v > 0xFFFFFFFF then
        far.Message("The value must be < 2^32", "Error", nil, "w")
        return 0
      end
    end
  end

  function items.proc (hDlg, Msg, Par1, Par2)
    if Msg == F.DN_CLOSE then
      return closeaction(hDlg, Par1, Par2)
    end
  end

  local out, pos = sd.New(items):Run()
  if out then
    if pos == posOK then
      return "ok", tp=="string" and out.result or tonumber(out.result)
    else
      return "reset"
    end
  end
end

local function FarConfig()
  local Hidden = false -- the options having default values are hidden

  local items = {
    guid=far.Guids.AdvancedConfigId;
    help=":FarConfig";
    { tp="listbox"; list={}; listnoclose=1; },
  }
  local posList = 1
  local list = items[posList].list

  local r = actl.GetFarRect()
  items.width = r.Right - r.Left - 4
  items[posList].height = r.Bottom - r.Top - 4

  for i=1,math.huge do
    local item = MakeItem(i)
    if not item then
      break
    elseif type(item) == "table" then
      table.insert(list, item)
    end
  end

  table.sort(list, function(a,b) return far.LStricmp(a.Text, b.Text) < 0; end)

  items.proc = function(hDlg, msg, p1, p2)
    local Op, AsHex

    if msg == F.DN_INITDIALOG then
      hDlg:ListSetMouseReaction(posList, F.LMRT_NEVER)
      hDlg:ListSetTitles(posList, { Title=TopTitle; Bottom=BottomTitle; })

    elseif msg == F.DN_RESIZECONSOLE then
      local r = actl.GetFarRect()
      local W = r.Right - r.Left - 4
      local H = r.Bottom - r.Top - 2

      hDlg:ResizeDialog(1, {X = W, Y = H})
      local item = hDlg:GetDlgItem(posList)
      item[4] = W - 4 -- x2
      item[5] = H - 2 -- y2
      hDlg:SetDlgItem(posList, item)

      hDlg:MoveDialog(1, {X = -1, Y = -1})
      hDlg:ListSetMouseReaction(posList, F.LMRT_NEVER)

    elseif msg == "EVENT_KEY" then
      if p2=="Enter" or p2=="NumEnter" or p2=="F4" or p2=="ShiftF4" then
        Op = "edit"
        AsHex = (p2 == "ShiftF4")
      else
        Op = p2=="Del" and "reset" or p2=="CtrlH" and "hide"
      end

    elseif msg == "EVENT_MOUSE" then
      if p2.EventFlags == F.DOUBLE_CLICK then
        Op = "edit"
      end
    end

    if Op == "edit" or Op == "reset" then
      local data = hDlg:ListGetCurPos(posList)
      if data then
        local ok = false
        local pos = data.SelectPos
        local idx = list[pos].configIndex
        local val,tp,val0,key,name = Far.GetConfig(idx)

        if Op == "edit" then
          if tp == "boolean" then
            ok = Far.SetConfig(idx, val==0 and 1 or 0)
          elseif tp == "3-state" then
            ok = Far.SetConfig(idx, (val + 1) % 3)
          else
            local what, ret = EditValue(AsHex,key,name,tp,val0,val)
            if what then
              ok = Far.SetConfig(idx, what=="ok" and ret or what=="reset" and val0)
            end
          end
        elseif Op == "reset" then
          ok = Far.SetConfig(idx, val0)
        end

        if ok then
          local item = MakeItem(idx)
          item.Index = pos
          hDlg:ListUpdate(posList, item)
          hDlg:ListSetCurPos(posList, data)
        end
      end
    elseif Op == "hide" then
      Hidden = not Hidden
      hDlg:EnableRedraw(false)
      for i=1,math.huge do
        local item = hDlg:ListGetItem(posList, i)
        if not item then break end
        local updating = false
        if Hidden and 0==band(item.Flags, F.LIF_CHECKED) then
          updating = true
          item.Flags = bor(item.Flags, F.LIF_HIDDEN)
        elseif not Hidden and 0~=band(item.Flags, F.LIF_HIDDEN) then
          updating = true
          item.Flags = band(item.Flags, bnot(F.LIF_HIDDEN))
        end
        if updating then
          item.Index = i
          hDlg:ListUpdate(posList, item)
        end
      end
      ----------------------------------------------------------------------------------
      -- a workaround for case when the listbox has enough height to show all the itens
      -- but shows only part of them due to trying to preserve the old TopPos
      local data = hDlg:ListGetCurPos(posList)
      data.TopPos = 1
      hDlg:ListSetCurPos(posList, data)
      ----------------------------------------------------------------------------------
      hDlg:EnableRedraw(true)
    end
  end

  --far.Show(#list) --> 272 items
  sd.New(items):Run()
end

return FarConfig
