local sd = require"far2.simpledialog"
local F = far.Flags
local items = {
  guid=far.Guids.AdvancedConfigId;
  { tp="dbox"; text="Configuration editor"; },
  { tp="listbox"; y1=2; x1=4; list={}; listnoclose=1; listnobox=1; },
}
local posBox, posList = 1, 2

local r = actl.GetFarRect()
items.width = r.Right - r.Left - 5
items[posBox].width = r.Right - r.Left - 9
items[posBox].height = r.Bottom - r.Top - 4
items[posList].height = items[posBox].height - 2

local list = items[posList].list

local function MakeItem(idx)
  local key,name,tp,val0,val = Far.Cfg_Get(idx)
  if key then
    local txt = ("%-42s│%-8s│"):format(key.."."..name, tp)
    if tp == "integer" or tp == "string" or tp == "boolean" then
      if tp == "integer" then
        txt = ("%s%d = 0x%X"):format(txt, val, val)
      elseif tp == "string" then
        txt = txt .. val
      elseif tp == "boolean" then
        txt = txt .. (val == 0 and "false" or "true")
      end
      local item = { Text=txt; configIndex=idx; Flags=0; }
      if val ~= val0 then item.Flags = F.LIF_CHECKED + string.byte"*"; end
      return item
    else
      return "other"
    end
  end
end

for i=1,math.huge do
  local item = MakeItem(i)
  if not item then break end
  if type(item) == "table" then
    table.insert(list, item)
  end
end

table.sort(list, function(a,b) return a.Text < b.Text end)

items.proc = function(hDlg, msg, p1, p2)
  local Op
  if msg == F.DN_INITDIALOG then
    hDlg:ListSetMouseReaction(posList, F.LMRT_NEVER)
  elseif msg == F.DN_RESIZECONSOLE then
    --hDlg:MoveDialog(true, {X=-1, Y=-1}) -- crashes Far when pressing Esc
    hDlg:MoveDialog(1, {X=-1, Y=-1})
  elseif msg == F.DN_KEY and (p2 == F.KEY_ENTER or p2 == F.KEY_NUMENTER)  or
         msg == F.DN_MOUSECLICK and p2.EventFlags == F.DOUBLE_CLICK then
    Op = "edit"
  elseif msg == F.DN_KEY and p2 == F.KEY_DEL then
    Op = "reset"
  end
  if Op == "edit" or Op == "reset" then
    local data = hDlg:ListGetCurPos(posList)
    if data then
      local ok = false
      local pos = data.SelectPos
      local idx = list[pos].configIndex
      local key,name,tp,val0,val = Far.Cfg_Get(idx)
      if Op == "edit" then
        if tp == "boolean" then
          ok = Far.Cfg_Set(idx, val==0 and 1 or 0)
        else
          local str = far.InputBox (nil, "", ("%s.%s (%s)"):format(key,name,tp),
            nil, tostring(val), nil, "HelpTopic", nil)
          if str then
            if tp == "integer" and tonumber(str) then
              ok = Far.Cfg_Set(idx, tonumber(str))
            elseif tp == "string" then
              ok = Far.Cfg_Set(idx, str)
            end
          end
        end
      elseif Op == "reset" then
        ok = Far.Cfg_Set(idx, val0)
      end
      if ok then
        local item = MakeItem(idx)
        item.Index = pos
        hDlg:ListUpdate(posList, item)
        hDlg:ListSetCurPos(posList, data)
      end
    end
  end
end

--far.Show(#list) --> 272 items
sd.New(items):Run()
