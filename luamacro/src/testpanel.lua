local F = far.Flags
local Title ="Luamacro test panel"
local mod = {}

mod.Info = {
  Guid = win.Uuid("121969E3-9565-49B4-8653-4A03D46CD7BE"); -- mandatory field
  Description = "A built-in panel for testing";
  StartDate = "2025-08-09";
}

function mod.GetFindData(object, handle, OpMode)
  return {
    {FileName="file1.txt"; FileSize=1000;},
    {FileName="file2.txt"; FileSize=3000;},
    {FileName="file3.txt"; FileSize=2000;},
    {FileName="file4.txt"; FileSize=4000;},
  }
end

function mod.GetOpenPanelInfo(object, handle)
  return {
    PanelTitle = Title;
    Flags = F.OPIF_ADDDOTS;
  }
end

function mod.SetDirectory()
  return true
end

return mod
