local F = far.Flags
local Title ="Luamacro test panel"
local mod = {}

mod.Info = {
  Guid = win.Uuid("121969E3-9565-49B4-8653-4A03D46CD7BE"); -- mandatory field
  Description = "A built-in panel for testing";
  StartDate = "2025-08-09";
}

function mod.GetFindData(obj, handle, OpMode)
  if obj.files == nil then
    obj.files = {}
    local num = tonumber(obj.args:match("^[%w_]+%s+(%d+)")) or 0
    for k=1,num do
      local name = ("file-%d.txt"):format(k)
      local size = math.random(1000, 9999)
      obj.files[k] = { FileName=name; FileSize=size; }
    end
  end
  return obj.files
end

function mod.GetOpenPanelInfo(obj, handle)
  return {
    PanelTitle = Title;
    Flags = F.OPIF_ADDDOTS;
  }
end

function mod.SetDirectory()
  return true
end

function mod.ProcessPanelEvent(obj, handle, Event, Param)
  if Event == F.FE_STARTSORT then
    return true -- tell Far to not call Compare on this panel
  end
end

return mod
