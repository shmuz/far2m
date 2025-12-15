local asrt = require "far2.assert"
local F = far.Flags


local function test_XPanel(pan) -- (@pan: either APanel or PPanel)
  local pNum = pan==APanel and 1 or 0
  local panInfo = asrt.table(panel.GetPanelInfo(nil, pNum))
  local curItem = asrt.table(panel.GetCurrentPanelItem(nil, pNum))
  local colTypes = asrt.str(panel.GetColumnTypes(nil, pNum))
  local colCount = select(2, colTypes:gsub("%f[%w]N", "")) -- only "Name" columns
  local R = asrt.table(panInfo.PanelRect)

  asrt.bool (pan.Bof)
  asrt.eq   (pan.ColumnCount, colCount)
  asrt.eq   (pan.CurPos, asrt.num(panInfo.CurrentItem))
  asrt.eq   (pan.Current, asrt.str(curItem.FileName))
  asrt.num  (pan.DriveType)
  asrt.eq   (pan.Empty, panInfo.ItemsNumber==1) -- an empty panel has a ".." item
  asrt.bool (pan.Eof)
  asrt.eq   (pan.FilePanel, panInfo.PanelType==F.PTYPE_FILEPANEL)
  asrt.bool (pan.Filter)
  asrt.eq   (pan.Folder, curItem.FileAttributes:match("d")=="d")
  asrt.eq   (pan.Format, asrt.str(panel.GetPanelFormat(nil, pNum)))
  asrt.eq   (pan.Height, R.bottom - R.top + 1)
  asrt.eq   (pan.HostFile, asrt.str(panel.GetPanelHostFile(nil, pNum)))
  asrt.eq   (pan.ItemCount, panInfo.ItemsNumber)
  asrt.eq   (pan.Left, 0 ~= band(panInfo.Flags,F.PFLAGS_PANELLEFT))
  asrt.num  (pan.OPIFlags)
  asrt.eq   (pan.Path, asrt.table(panel.GetPanelDirectory(nil, pNum)).Name)
  asrt.str  (pan.Path0)
  asrt.eq   (pan.Plugin, 0 ~= band(panInfo.Flags,F.PFLAGS_PLUGIN))
  asrt.eq   (pan.Prefix, asrt.str(panel.GetPanelPrefix(nil, pNum)))
  asrt.bool (pan.Root)
  asrt.num  (pan.SelCount)
  asrt.bool (pan.Selected)
  asrt.eq   (pan.Type, panInfo.PanelType)
  asrt.str  (pan.UNCPath)
  asrt.eq   (pan.Visible, 0 ~= band(panInfo.Flags,F.PFLAGS_VISIBLE))
  asrt.eq   (pan.Width, R.right - R.left + 1)

  if pan == APanel then
    Keys "End"  asrt.istrue(pan.Eof); asrt.istrue(pan.Empty or not pan.Bof);
    Keys "Home" asrt.istrue(pan.Bof); asrt.istrue(pan.Empty or not pan.Eof);
  end
end

local test_APanel = function() test_XPanel(APanel) end
local test_PPanel = function() test_XPanel(PPanel) end


local function test_Panel_Item()
  local index = 0 -- 0 is the current element, otherwise element index
  for pan=0,1 do
    asrt.str    (Panel.Item(pan,index,0))  -- file name
    asrt.str    (Panel.Item(pan,index,1))  -- short file name
    asrt.num    (Panel.Item(pan,index,2))  -- file attributes
    asrt.str    (Panel.Item(pan,index,3))  -- creation time
    asrt.str    (Panel.Item(pan,index,4))  -- last access time
    asrt.str    (Panel.Item(pan,index,5))  -- modification time
    asrt.numint (Panel.Item(pan,index,6))  -- size
    asrt.numint (Panel.Item(pan,index,7))  -- packed size
    asrt.bool   (Panel.Item(pan,index,8))  -- selected
    asrt.num    (Panel.Item(pan,index,9))  -- number of links
    asrt.num    (Panel.Item(pan,index,10)) -- sort group
    asrt.str    (Panel.Item(pan,index,11)) -- diz text
    asrt.str    (Panel.Item(pan,index,12)) -- owner
    asrt.num    (Panel.Item(pan,index,13)) -- crc32
    asrt.num    (Panel.Item(pan,index,14)) -- position when read from the file system
    asrt.numint (Panel.Item(pan,index,15)) -- creation time
    asrt.numint (Panel.Item(pan,index,16)) -- last access time
    asrt.numint (Panel.Item(pan,index,17)) -- modification time
    asrt.num    (Panel.Item(pan,index,18)) -- number of streams
    asrt.numint (Panel.Item(pan,index,19)) -- size of streams
    asrt.str    (Panel.Item(pan,index,20)) -- change time
    asrt.numint (Panel.Item(pan,index,21)) -- change time
  end
end


local function test_Panel_SetPath()
  -- store
  local adir_old = panel.GetPanelDirectory(nil,1).Name
  local pdir_old = panel.GetPanelDirectory(nil,0).Name
  --test
  local pdir = "/bin"
  local adir = "/usr/bin"
  local afile = "ldd"
  asrt.istrue(Panel.SetPath(1, pdir))
  asrt.istrue(Panel.SetPath(0, adir, afile))
  asrt.eq (pdir, panel.GetPanelDirectory(nil,0).Name)
  asrt.eq (adir, panel.GetPanelDirectory(nil,1).Name)
  asrt.eq (panel.GetCurrentPanelItem(nil,1).FileName, afile)
  -- restore
  asrt.istrue(Panel.SetPath(1, pdir_old))
  asrt.istrue(Panel.SetPath(0, adir_old))
  actl.Commit()
end


-- N=Panel.Select(panelType,Action[,Mode[,Items]])
local function test_Panel_Select(pan)
  local adir_old = panel.GetPanelDirectory(nil,pan).Name -- store panel directory

  local PS = asrt.func(Panel.Select)
  local ACT_RM, ACT_ADD, ACT_INV, ACT_RST = 0,1,2,3       -- Action
  local MOD_ALL, MOD_INDEX, MOD_LIST, MOD_MASK = 0,1,2,3  -- Mode

  local dir = asrt.str(win.GetEnv("FARHOME"))
  asrt.istrue(panel.SetPanelDirectory(nil,pan,dir))
  local pi = asrt.table(panel.GetPanelInfo(nil,pan))
  local ItemsCount = asrt.num(pi.ItemsNumber)-1 -- don't count ".."
  assert(ItemsCount>=10, "not enough files to test")

  --------------------------------------------------------------
  asrt.eq(ItemsCount, PS(1-pan, ACT_ADD, MOD_ALL)) -- select all
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(ItemsCount, pi.SelectedItemsNumber)

  asrt.eq(ItemsCount, PS(1-pan, ACT_RM, MOD_ALL)) -- clear all
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  asrt.eq(ItemsCount, PS(1-pan, ACT_INV, MOD_ALL)) -- invert
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(ItemsCount, pi.SelectedItemsNumber)

  asrt.eq(0, PS(1-pan, ACT_INV, MOD_ALL)) -- invert again (return value is the selection count, contrary to docs)
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  asrt.eq(ItemsCount, PS(1-pan, ACT_RST, MOD_ALL)) -- restore (same as Ctrl+M)
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(ItemsCount, pi.SelectedItemsNumber)

  asrt.eq(ItemsCount, PS(1-pan, ACT_RM, MOD_ALL)) -- clear all
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  --------------------------------------------------------------
  asrt.eq(1, PS(1-pan, ACT_ADD, MOD_INDEX, 5))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(1, pi.SelectedItemsNumber)

  asrt.eq(1, PS(1-pan, ACT_RM, MOD_INDEX, 5))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  asrt.eq(1, PS(1-pan, ACT_INV, MOD_INDEX, 5))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(1, pi.SelectedItemsNumber)

  asrt.eq(1, PS(1-pan, ACT_INV, MOD_INDEX, 5))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  --------------------------------------------------------------
  local list = dir.."/FarEng.hlf\nFarEng.lng" -- the 1-st file with path, the 2-nd without
  asrt.eq(2, PS(1-pan, ACT_ADD, MOD_LIST, list))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(2, pi.SelectedItemsNumber)

  asrt.eq(2, PS(1-pan, ACT_RM, MOD_LIST, list))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  asrt.eq(2, PS(1-pan, ACT_INV, MOD_LIST, list))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(2, pi.SelectedItemsNumber)

  asrt.eq(2, PS(1-pan, ACT_INV, MOD_LIST, list))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  --------------------------------------------------------------
  local mask = "*.hlf;*.lng"
  local count = 0
  for i=1, pi.ItemsNumber do
    local item = asrt.table(panel.GetPanelItem(nil,pan,i))
    if far.CmpNameList(mask, item.FileName) then count=count+1 end
  end
  assert(count>1, "not enough files to test")

  asrt.eq(count, PS(1-pan, ACT_ADD, MOD_MASK, mask))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(count, pi.SelectedItemsNumber)

  asrt.eq(count, PS(1-pan, ACT_RM, MOD_MASK, mask))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  asrt.eq(count, PS(1-pan, ACT_INV, MOD_MASK, mask))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(count, pi.SelectedItemsNumber)

  asrt.eq(count, PS(1-pan, ACT_INV, MOD_MASK, mask))
  pi = asrt.table(panel.GetPanelInfo(nil, pan))
  asrt.eq(0, pi.SelectedItemsNumber)

  panel.SetPanelDirectory(nil, pan, adir_old) -- restore panel directory
end


local function test_panel_all()
  test_APanel()
  test_PPanel()
  test_Panel_Item()
  test_Panel_SetPath()
  test_Panel_Select(0)
  test_Panel_Select(1)

  asrt.eq (Panel.FAttr(0, ":"), -1)
  asrt.eq (Panel.FAttr(1, ":"), -1)

  asrt.eq (Panel.FExist(0, ":"), 0)
  asrt.eq (Panel.FExist(1, ":"), 0)

  asrt.func (Panel.SetPos)
  asrt.func (Panel.SetPosIdx)
end


return {
  test_panel_all = test_panel_all;
}