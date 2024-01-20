local sd = require"far2.simpledialog"
local F = far.Flags
local items = {
  guid=far.Guids.AdvancedConfigId;
  { tp="listbox"; list={}; text="Configuration editor"; },
}

local r = actl.GetFarRect()
items.width = r.Right - r.Left - 5
items[1].width = r.Right - r.Left - 9
items[1].height = r.Bottom - r.Top - 4

local list = items[1].list
for i=1,math.huge do
  local key,name,tp,val0,val = Far.Cfg_Get(i)
  if not key then break end
  local txt = ("%-42s│%-7s│"):format(key.."."..name, tp)
  if tp == "integer" or tp == "string" then
    if tp == "integer" then
      txt = ("%s%d = 0x%X"):format(txt, val, val)
    else
      txt = txt .. val
    end
    local t = {Text=txt}
    if val ~= val0 then t.Flags = F.LIF_CHECKED + string.byte"*"; end
    table.insert(list, t)
  end
end

table.sort(list, function(a,b) return a.Text < b.Text end)

items.proc = function(hDlg, msg, p1, p2)
  if msg == F.DN_INITDIALOG then
  elseif msg == F.DN_RESIZECONSOLE then
    --hDlg:MoveDialog(true, {X=-1, Y=-1}) -- crashes Far when pressing Esc
    hDlg:MoveDialog(1, {X=-1, Y=-1})
  end
end

--far.Show(#list) --> 272 items
sd.New(items):Run()
