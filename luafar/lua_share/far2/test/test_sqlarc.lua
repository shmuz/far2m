-- started: 2020-01-11
-- coding: UTF-8

assert(Far and Panel and Editor and Viewer and mf, "This script must be run by LuaMacro plugin")

local PluginGuids = {
  Plugin        = 0xF309DDDB;
  ConfirmDelete = "E9D1D88D-E8F7-4548-AD4B-4FA61DBD0AFF";
  MakeFolder    = "E5A63040-CE75-4A43-8637-1D8CB233E05C";
  ExtractFiles  = "7804AF5A-0F67-467D-8879-670E2005B955";
}
local join = win.JoinPath
local TempDir = far.InMyTemp("sqlarc_test")
local HostFile = join(TempDir, "test1.sqlarc")
local F = far.Flags

-- Ensure (partial) restore of panels state
local function RestorePanelsOnExit()
  mf.AddExitHandler(panel.SetPanelDirectory, nil, 1, panel.GetPanelDirectory(nil,1))
  mf.AddExitHandler(panel.SetPanelDirectory, nil, 0, panel.GetPanelDirectory(nil,0))
  mf.AddExitHandler(
    function(arg)
      if APanel.Left~=arg then panel.SetActivePanel(nil,0); end
    end, APanel.Left)
end

local function create_file(dir, fname, attr)
  win.Sleep(1) -- to avoid the same timestamp in different files
  fname = join(dir, fname)
  local fp = assert(io.open(fname, "wb"))
  local size = math.random(0,1000)
  if size > 0 then
    local t = {}
    for k=1,size do t[k]=string.char(math.random(0,255)) end
    fp:write(table.concat(t))
  end
  fp:close()
  if attr then
    assert(win.SetFileAttr(fname, attr))
  end
end

local function is_our_panel (activePanel)
  assert(far.MacroGetArea() == F.MACROAREA_SHELL)
  local nPanel = activePanel and 1 or 0
  local OwnerId = assert(panel.GetPanelInfo(nil, nPanel)).OwnerID
  local File = panel.GetPanelHostFile(nil, nPanel)
  return OwnerId == PluginGuids.Plugin and File == HostFile
end

local function test_jump_outside()
  if not is_our_panel(true) then
    Keys("Tab"); assert(is_our_panel(true))
  end
  if not APanel.Root then
    Keys("CtrlBackslash"); assert(APanel.Root)
  end
  Keys("CtrlBackslash")
  assert(not APanel.Plugin)
end

local function test_delete_content()
  assert(is_our_panel(true))
  if not APanel.Root then
    Keys("CtrlBackslash"); assert(APanel.Root)
  end
  if not APanel.Empty then
    Keys("Home ShiftEnd F8") -- select all in the root directory
    assert(Area.Dialog and Dlg.Id == PluginGuids.ConfirmDelete)
    Keys("Enter")
    assert(APanel.Empty)
  end
end

local function test_create_archive()
  assert(Plugin.Exist(PluginGuids.Plugin))
  assert(Area.Shell)
  assert(win.CreateDir(TempDir, "t"))
  assert(not win.GetFileAttr(HostFile) or win.DeleteFile(HostFile)) -- assure the old file isn't here
  assert(Plugin.Command(PluginGuids.Plugin, HostFile)) -- create a DB file
  assert(is_our_panel(true))
  assert(APanel.Empty)
end

local function test_create_archive_dirs()
  assert(is_our_panel(true))
  test_delete_content()

  Keys("F7"); assert(Area.Dialog and Dlg.Id==PluginGuids.MakeFolder)
  print("Папка 1"); Keys("Enter");
  assert(APanel.Folder)
  assert(APanel.Current == "Папка 1")
  Keys("Enter")
  assert(APanel.Path == "Папка 1")

  Keys("F7"); assert(Area.Dialog and Dlg.Id==PluginGuids.MakeFolder)
  print("Папка 2"); Keys("Enter");
  assert(APanel.Folder)
  assert(APanel.Current == "Папка 2")
  Keys("Enter")
  assert(APanel.Path == join("Папка 1", "Папка 2"))
end

local function test_remove_tree(aDir, aInnerCall)
  if not aInnerCall then
    local dir = win.GetEnv("FARHOME")
    panel.SetPanelDirectory(nil, 0, dir)
    panel.SetPanelDirectory(nil, 1, dir)
  end
  if win.GetFileAttr(aDir) then
    far.RecursiveSearch(aDir, "*",
      function(aItem, aFullPath)
        if aItem.FileAttributes:find("d") then
          test_remove_tree(aFullPath, true) -- recurse
        else
          assert(win.DeleteFile(aFullPath))
        end
      end)
    assert(win.RemoveDir(aDir))
  end
end

local function test_get_filesystem_dir_data(aDir)
  local t = {}
  far.RecursiveSearch(aDir, "*",
    function(aItem, aFullPath)
      assert(aItem.FileName ~= "..")
      if aItem.FileAttributes:find("d") then
        aItem.FileSize = 0 -- not 4096 as got from FAR
        aItem.Items = test_get_filesystem_dir_data(aFullPath) -- recurse
      end
      table.insert(t, aItem)
    end)
  table.sort(t, function(a,b) return a.FileName < b.FileName; end)
  return t
end

local function test_get_archive_dir_data (activePanel, aDir)
  -- Set directory inside the archive
  local nPanel = activePanel and 1 or 0
  assert(Panel.SetPluginPath(1-nPanel, join("/", aDir)))
  -- Collect directory items
  local t = {}
  for k=1,assert(panel.GetPanelInfo(nil, nPanel)).ItemsNumber do
    local item = assert(panel.GetPanelItem(nil, nPanel, k))
    if item.FileName ~= ".." then table.insert(t, item) end
  end
  -- Collect items from subdirectories
  for _,item in ipairs(t) do
    if item.FileAttributes:find("d") then
      item.Items = test_get_archive_dir_data(activePanel, join(aDir, item.FileName)) -- recurse
    end
  end
  -- Sort and return
  table.sort(t, function(a,b) return a.FileName < b.FileName; end)
  return t
end

local function test_compare_archive_filesystem(array_arc, array_fs)
  assert(#array_arc == #array_fs)
  for i=1,#array_arc do -- both arrays are sorted by file names
    local t_arc = array_arc[i]
    local t_fs = array_fs[i]
    assert(t_arc.FileName       == t_fs.FileName)
    assert(t_arc.FileSize       == t_fs.FileSize)
    assert(t_arc.FileAttributes == t_fs.FileAttributes)
--  assert(t_arc.CreationTime   == t_fs.CreationTime)
    assert(t_arc.LastWriteTime  == t_fs.LastWriteTime)
    if t_arc.FileAttributes:find("d") then
      test_compare_archive_filesystem(t_arc.Items, t_fs.Items) -- recurse
    end
  end
end

local function test_create_filesystem_tree()
  local rootdir = join(TempDir, "rootdir")

  local dir1   = join(rootdir, "Наши продукты")
  local dir1_1 = join(dir1,    "Фрукты")
  local dir1_2 = join(dir1,    "Овощи")

  local dir2   = join(rootdir, "Our docs")
  local dir2_1 = join(dir2,    "Inbox")
  local dir2_2 = join(dir2,    "Sent")

  for _,dname in ipairs {dir1_1, dir1_2, dir2_1, dir2_2} do
    assert(win.CreateDir(dname, "t"))
  end

  create_file (rootdir, "notes-1.txt")
  create_file (rootdir, "notes-2.txt", "")

  create_file (dir1,   "заметки-1.текст", "a")
  create_file (dir1,   "заметки-2.текст", "ah")
  create_file (dir1_1, "яблоки.текст",    "hs")
  create_file (dir1_1, "груши.текст",     "as")
  create_file (dir1_2, "картошка.текст",  "ahs")
  create_file (dir1_2, "морковка.текст")

  create_file (dir2,   "notes-1.txt")
  create_file (dir2,   "notes-2.txt")
  create_file (dir2_1, "letter from friend-1.txt")
  create_file (dir2_1, "letter from friend-2.txt")
  create_file (dir2_2, "letter to friend-1.txt")
  create_file (dir2_2, "letter to friend-2.txt")

  -- This call is needed to eliminate the effect of cached directories' timestamps
  -- as the filesystem cache is updated at unpredictable moments.
  -- Without this call LastWriteTime comparisons will fail on some directories.
  far.RecursiveSearch(rootdir, "*",
    function(item,fullpath)
      assert(item.FileName ~= "..")
      if item.FileAttributes:find("d") then
        local wTime = item.LastWriteTime + math.random(-1e8, 1e8) -- math.random part isn't strictly necessary
        assert(win.SetFileTimes(fullpath, {LastWriteTime=wTime}))
      end
    end, "FRS_RECUR")

  return rootdir
end

local function test_put_files(srcdir)
  assert(is_our_panel(true))
  test_delete_content()
  assert(panel.SetPanelDirectory(nil, 0, srcdir))
  assert(not PPanel.Plugin)
  assert(is_our_panel(true))
  Keys("Tab Home ShiftEnd F5")
  assert(Area.Dialog and Dlg.Id==far.Guids.CopyFilesId)
  Keys("Enter")
  assert(Area.Shell)
  local fs_data = test_get_filesystem_dir_data(srcdir)
  local arc_data = test_get_archive_dir_data(false, "")
  test_compare_archive_filesystem(arc_data, fs_data)
end

local function test_get_files(trgdir)
  test_remove_tree(trgdir)
  assert(win.CreateDir(trgdir))
  assert(panel.SetPanelDirectory(nil, 0, trgdir))
  assert(not APanel.Plugin)
  assert(Plugin.Command(PluginGuids.Plugin, HostFile))
  assert(is_our_panel(true))
  Keys("Home ShiftEnd F5")
  assert(Area.Dialog and Dlg.Id==PluginGuids.ExtractFiles)
  Keys("Enter")
  local fs_data = test_get_filesystem_dir_data(trgdir)
  local arc_data = test_get_archive_dir_data(true, "")
  test_compare_archive_filesystem(arc_data, fs_data)
end

local function test_all()
  RestorePanelsOnExit()

  -- randomize
  math.randomseed(os.time())

  test_remove_tree(TempDir)
  test_create_archive()
  test_create_archive_dirs()

  local rootdir = test_create_filesystem_tree()
  test_put_files(rootdir)
  test_get_files(rootdir)

  test_jump_outside()
  test_remove_tree(TempDir)
end

return {
  test_all = test_all;
}
