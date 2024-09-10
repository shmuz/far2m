-- coding: UTF-8
-- This script requires to be run by LuaMacro plugin.

local sqlite3 = require "lsqlite3"

-- known GUIDs of the plugin's dialogs
local Guid_ConfirmClose  = "27224BE2-EEF4-4240-808F-38095BCEF7B2"
local Guid_ConfirmDelete = "4472C7D8-E2B2-46A0-A005-B10B4141EBBD"
local Guid_EditRow       = "866927E1-60F1-4C87-A09D-D481D4189534"
local Guid_Export        = "E9F91B4F-82B2-4B36-9C4B-240D7EE7BF59"
local Guid_Plugin        = "D4BC5EA7-8229-4FFE-AAC1-5A4F51A0986A"
local Guid_Pragmas       = "FF769EE0-2643-48F1-A8A2-239CD3C6691F"
local SysId_Plugin       = 0xD4BC5EA7

-- known mechanisms for loading user modules
-- local USERMOD_DIR = win.GetEnv("farprofile").."\\PluginsData\\Polygon"
local USERMOD_TABLE = "modules-"..Guid_Plugin

-- known plugin's keys
local Key_Edit_Row              = "F4"
local Key_Export_Dialog         = "F5"
local Key_Insert_Row            = "ShiftF4"
local Key_Query_From_Editor     = "F6"
local Key_View_Content          = "F3"
local Key_View_Create_Statement = "F4"
local Key_View_Pragmas          = "ShiftF4"

-- Ensure (partial) restore of panels state
local function RestorePanelsOnExit()
  mf.AddExitHandler(panel.SetPanelDirectory, nil, 1, panel.GetPanelDirectory(nil,1))
  mf.AddExitHandler(panel.SetPanelDirectory, nil, 0, panel.GetPanelDirectory(nil,0))
  mf.AddExitHandler(
    function(arg)
      if APanel.Left~=arg then panel.SetActivePanel(nil,0); end
    end, APanel.Left)
end

local function assert_farpanel()
  assert(not APanel.Plugin, "active panel must not be plugin panel")
end

local function assert_plugpanel()
  assert(APanel.Plugin, "active panel must be plugin panel")
end

local function assert_close(keys)
  Keys(keys)
  if Dlg.Id == Guid_ConfirmClose then
    Keys("Enter") -- option "Confirm closing the panel" was ON
  end
  assert_farpanel()
end

local function assert_tmpdir()
  return far.InMyTemp()
end

local function ExecCmdLine(command)
  assert(Area.Shell)
  assert(panel.SetCmdLine(nil, command))
  Keys("Enter")
end

local function PluginOpenInMemory()
  assert(Area.Shell)
  assert_farpanel()
  assert(Plugin.SyncCall(SysId_Plugin, "open", ":memory:", ""), "open :memory: failed")
  assert_plugpanel()
  assert(APanel.Current=="..")
  assert(APanel.ItemCount==2)
end

local function PluginOpenFile(filename, flags)
  assert(Area.Shell)
  assert_farpanel()
  assert(Plugin.SyncCall(SysId_Plugin, "open", filename, flags), "open file failed")
  assert_plugpanel()
  assert(APanel.Current=="..")
end

local function CreateNewDB (subdir)
  local tmp = assert_tmpdir()
  if subdir then
    tmp = win.JoinPath(tmp, subdir)
    assert(win.CreateDir(tmp, "t"))
  end
  local DbFileName = win.JoinPath(tmp, "polygon-test.sqlite3")
  if win.GetFileAttr(DbFileName) and not win.DeleteFile(DbFileName) then
    error("could not delete "..DbFileName)
  end
  local DB, _, errmsg = sqlite3.open(DbFileName)
  assert(DB, errmsg)
  return DB, DbFileName
end

local function CreateTableAndView()
  ExecCmdLine [[
    BEGIN TRANSACTION;
    CREATE TABLE IF NOT EXISTS MyTable (a,b,c);
    CREATE VIEW  IF NOT EXISTS MyView (x,y) AS SELECT c,a FROM MyTable;
    INSERT INTO MyTable VALUES (1,2,3);
    INSERT INTO MyTable VALUES (1,2,3);
    END TRANSACTION; ]]
end

-- Bug (regression) introduced in SVN revision 2837.
-- HISTORY:
--   2018-04-07, v.0.4.1
--      1. Fix: incorrect selection after selected items were deleted (regression bug from v0.2.0).
--   2018-03-08, v.0.1
--      4. [fix] Normal work with selection when the panel displays a table contents.
local function test_r2837()
  local DB, DbFileName = CreateNewDB()

  local nrows, nselrows = 10, 3 -- these variables were arguments to this function previously
  assert(nrows >= nselrows, "arguments constraint failed")

  local query = "CREATE TABLE test (a,b,c)"
  if sqlite3.OK ~= DB:exec(query) then error(DB:errmsg()); end

  query = "INSERT INTO test VALUES (5,7,9)"
  DB:exec("BEGIN TRANSACTION;")
  for _ = 1,nrows do DB:exec(query); end
  DB:exec("END TRANSACTION;")

  assert(DB:close()==sqlite3.OK, "could not close DB")

  assert_farpanel()
  assert(Plugin.Call(SysId_Plugin, "open", DbFileName, ""), "Plugin.Call() failed")
  assert_plugpanel()
  assert(Plugin.Command(SysId_Plugin, DbFileName))
  assert(Panel.SetPath(0, "test"))

  local sel = {}
  for k=1,nselrows do sel[k]=k+1; end -- add 1 to skip ".."
  panel.SetSelection(nil,1,sel,true)

  for k=1,3 do -- test selection
    local info = panel.GetPanelInfo(nil,1)
    assert(info.ItemsNumber==nrows+1, "bad items number")
    assert(info.SelectedItemsNumber==nselrows, "bad selected items number")
    if k==1 then Keys("CtrlR") end
    if k==2 then
      panel.UpdatePanel(nil,1,true) -- true: keep selection
      panel.RedrawPanel(nil,1)
    end
  end

  Keys("F8");    assert(Dlg.Id==Guid_ConfirmDelete, "bad area, or wrong dialog")
  Keys("Enter"); assert(Area.Shell)
  Keys("Home") -- for getting correct SelectedItemsNumber

  for k=1,3 do -- test selection
    local info = panel.GetPanelInfo(nil,1)
    assert(info.ItemsNumber==nrows+1-nselrows, "bad items number")
    assert(info.SelectedItemsNumber==0, "bad selected items number")
    if k==1 then Keys("CtrlR") end
    if k==2 then
      panel.UpdatePanel(nil,1,true) -- true: keep selection
      panel.RedrawPanel(nil,1)
    end
  end

  --assert(panel.ClosePanel(nil,1), "could not close panel")
  assert_close("CtrlBackSlash Enter")
end

local function test_command_line()
  PluginOpenInMemory()

  ExecCmdLine("BEGIN TRANSACTION")
  ExecCmdLine("CREATE TABLE test1 (a,b,c)")
  local N = 17
  local cmd = ("INSERT INTO test1 VALUES (2, 'foo', x'123456');"):rep(N)
  ExecCmdLine(cmd)
  ExecCmdLine("END TRANSACTION")

  assert(Panel.SetPath(0, "test1"))
  assert(APanel.Path=="/main/test1")
  assert(APanel.ItemCount==N+1);
  Keys "Enter";
  assert(APanel.ItemCount==3)

  ExecCmdLine("lua far.Message'test'")
  assert(Area.Dialog); Keys "Esc";
  assert(Area.Shell)
  ExecCmdLine("lua assert(select('#', ...)==2)") -- arg#1: table tInfo; arg#2: panel handle
  assert(Area.Shell)

  assert_close("CtrlBackslash")
end

local function test_query_from_editor()
  local function EnterEditor()
    Keys(Key_Query_From_Editor)
    assert(Area.Menu or Area.Editor)
    if Area.Menu then Keys("F4") end
    assert(Area.Editor)
  end

  PluginOpenInMemory()
  local N = 4

  -- test from the database folder
  EnterEditor()
  Keys("CtrlA Del")
  print("BEGIN TRANSACTION;\n")
  print("CREATE TABLE IF NOT EXISTS MyTable (a,b,c);\n")
  print("CREATE VIEW  IF NOT EXISTS MyView (x,y) AS SELECT c,a FROM MyTable;\n")
  for _=1,N do print("INSERT INTO MyTable VALUES (1,2,3);\n") end
  print("END TRANSACTION;\n")
  Keys("F2 Esc")
  assert(Area.Shell)
  assert(APanel.ItemCount == 4)

  -- test from a table
  assert(Panel.SetPath(0, "MyTable"))
  assert(APanel.Path=="/main/MyTable")
  assert(APanel.ItemCount == N+1)
  EnterEditor()
  Keys("Space BS F2 Esc") -- ensure return of EEC_MODIFIED
  assert(APanel.Path=="/main/MyTable")
  assert(APanel.ItemCount == 2*N+1)

  -- test from a view
  assert(Panel.SetPluginPath(0, "/main/MyView"))
  assert(APanel.Path=="/main/MyView")
  assert(APanel.ItemCount == 2*N+1)
  EnterEditor()
  Keys("Space BS F2 Esc")
  assert(APanel.Path=="/main/MyView")
  assert(APanel.ItemCount == 3*N+1)

  -- test without save in the editor: nothing should change
  EnterEditor()
  Keys("Esc")
  assert(Area.Menu)
  Keys("Esc")
  assert(APanel.Path=="/main/MyView")
  assert(APanel.ItemCount == 3*N+1)

  assert_close("Enter CtrlBackslash")
end

local function test_panel_modes()
  PluginOpenInMemory()
  assert(APanel.Width < Far.Width)
  for n=0,9 do
    Keys("Ctrl"..n)
    if n%2 == 0 then assert(APanel.Width == Far.Width)
    else assert(APanel.Width < Far.Width)
    end
  end
  assert_close("CtrlBackslash")
end

local function test_dbitem_view()
  PluginOpenInMemory()
  CreateTableAndView()
  ---------------------------------------------
  assert(Panel.SetPos(0, "MyTable") ~= 0)
  Keys(Key_View_Content)
  assert(Area.Viewer)
  Keys("Esc")
  assert(Area.Shell)

  Keys(Key_View_Create_Statement)
  assert(Area.Viewer)
  Keys("Esc")
  assert(Area.Shell)
  ---------------------------------------------
  assert(Panel.SetPos(0, "MyView") ~= 0)
  Keys(Key_View_Content)
  assert(Area.Viewer)
  Keys("Esc")
  assert(Area.Shell)

  Keys(Key_View_Create_Statement)
  assert(Area.Viewer)
  Keys("Esc")
  assert(Area.Shell)
  ---------------------------------------------
  assert(Panel.SetPos(0, "sqlite_master") ~= 0)
  Keys(Key_View_Content)
  assert(Area.Viewer)
  Keys("Esc")
  assert(Area.Shell)

  Keys(Key_View_Create_Statement) -- nothing should happen as 'sqlite_master' has no create statement
  assert(Area.Shell)
  ---------------------------------------------
  assert_close("CtrlBackslash")
end

local function test_pragmas_view()
  PluginOpenInMemory()
  Keys(Key_View_Pragmas)
  assert(Area.Dialog and Dlg.Id == Guid_Pragmas)
  Keys("Esc")
  assert(Area.Shell)
  assert_close("CtrlBackslash")
end

local function test_table_export()
  PluginOpenInMemory()
  CreateTableAndView()
  ---------------------------------------------
  for _,name in ipairs {"MyTable", "MyView", "sqlite_master"} do
    assert(Panel.SetPos(0, name) ~= 0)
    Keys(Key_Export_Dialog)
    assert(Area.Dialog and Dlg.Id == Guid_Export)
    Keys("Esc")
    assert(Area.Shell)
  end
  ---------------------------------------------
  assert_close("CtrlBackslash")
end

-- Test that pressing Enter on a table without ROWID enters that table.
local function test_without_rowid()
  assert_farpanel()
  PluginOpenInMemory()
  ExecCmdLine("CREATE TABLE tbl (a,b,PRIMARY KEY(a,b)) WITHOUT ROWID")
  assert(APanel.ItemCount==3)
  ExecCmdLine("INSERT INTO tbl VALUES(0,1)")
  ExecCmdLine("INSERT INTO tbl VALUES(0,2)")
  ExecCmdLine("INSERT INTO tbl VALUES(0,3)")
  assert(Panel.SetPos(0, "tbl") ~= 0)
  Keys("Enter")
  assert(APanel.ItemCount==4)
  Keys("Enter")
  assert(APanel.ItemCount==3)
  assert_close("Home Enter")
end

local function str2value(str)
   -- NULL
   if str:upper() == "NULL" then return nil end
   -- number
   local num = tonumber(str)
   if num then return num end
   -- string
   local s = str:match("^'(.*)'$")
   if s then return (s:gsub("''", "'")) end
   -- blob
   s = str:match("^[xX]'(%x*)'$")
   assert(s and #s%2==0, "invalid SQL value string")
   local t = {}
   for p in s:gmatch("..") do
     table.insert(t, string.char(tonumber(p,16)))
   end
   return table.concat(t)
end

local function test_edit_row()
  assert_farpanel()
  local DB, DbFileName = CreateNewDB()
  assert(Plugin.Call(SysId_Plugin, "open", DbFileName, ""), "Plugin.Call() failed")
  assert_plugpanel()

  ExecCmdLine("CREATE TABLE test1(a,b,c)")
  assert(Panel.SetPath(0, "test1"))
  assert(APanel.Path=="/main/test1")
  assert(APanel.ItemCount==1)
  local data = {
    {"1234",  "'123'",  "x'1234'" },
    {"4.75",  "'Кот'",  "x'ABCD'" },
    {"NULL",  "'Foo'",  "x'773c3024cf95de9e155f29714e388be5'" },
  }
  -- Test 1: create new panel items and fill them with data
  for i,row in ipairs(data) do
    Keys(Key_Insert_Row)
    assert(Dlg.Id==Guid_EditRow, "bad area, or wrong dialog")
    for _,val in ipairs(row) do
      print(val);  Keys("Tab")
    end
    Keys("Enter")
    assert(APanel.ItemCount == i+1)
    assert(Area.Shell)
  end

  -- read data from database and validate it has expected values
  local cnt = 0
  for row in DB:nrows("SELECT * FROM test1 ORDER BY RowId") do
    cnt = cnt+1
    local ref = data[cnt]
    assert(row.a == str2value(ref[1]))
    assert(row.b == str2value(ref[2]))
    assert(row.c == str2value(ref[3]))
  end

  -- Test 2: edit panel item with RowId==2: change all its fields
  local newdata = { "2021", "-7.16", "'ЛЕПЁШКА'" }
  Keys("Home Down")
  for i=1,#data do -- order of items is not known: it depends on the current sorting mode
    local item = panel.GetCurrentPanelItem(nil,1,i)
    if 2==tonumber(item.FileName) then -- item.FileName contains RowId, e.g. "0000000002"
      Keys(Key_Edit_Row)
      assert(Dlg.Id==Guid_EditRow, "bad area, or wrong dialog")
      for _,val in ipairs(newdata) do
        print(val);  Keys("Tab")
      end
      Keys("Enter")
      break
    end
    Keys("Down")
  end
  assert(APanel.ItemCount == #data+1)
  assert(Area.Shell)

  -- read data from database and validate it has expected values
  cnt = 0
  for row in DB:nrows("SELECT * FROM test1 ORDER BY RowId") do
    cnt = cnt+1
    local ref = cnt==2 and newdata or data[cnt]
    assert(row.a == str2value(ref[1]))
    assert(row.b == str2value(ref[2]))
    assert(row.c == str2value(ref[3]))
  end

  assert(DB:close()==sqlite3.OK, "could not close DB")
  win.DeleteFile(DbFileName)
  assert_close("CtrlBackslash Enter")
end

-- The initial problem (fixed in commit 3779):
--   If there are many files in the folder, and the file opened by Polygon is shown somewhere
--   in the middle of the visible part of the panel, then after closing it moves to the bottom
--   of the visible part.
--   The plugins Arclite, Sqlarc, TmpPanel, LF TmpPanel do not have this "feature".
--   The reason is that when Far calls GetOpenPanelInfoW the CurDir field of the returned
--   table is not an empty string, but "\main".
local function test_panel_position_on_exit()
  RestorePanelsOnExit()
  local sqlite = require "lsqlite3"
  local F = far.Flags

  local tmp = assert_tmpdir()
  local workdir = win.JoinPath(tmp, win.Uuid("L"))
  assert(win.CreateDir(workdir))
  assert(panel.SetPanelDirectory(nil,1,workdir))

  local old_panel_info = panel.GetPanelInfo(nil,1) -- store sort mode and order
  local height = old_panel_info.PanelRect.bottom - old_panel_info.PanelRect.top + 1
  assert(height >= 12)
  -- Maximal number of "Name" columns (3 in "Brief" panel mode).
  -- If the actual panel mode contains more than 3 "Name" columns then this test fails.
  local MAX_NAME_COLS = 3
  local N = MAX_NAME_COLS*height + 10

  panel.SetSortMode(nil,1,"SM_NAME")
  panel.SetSortOrder(nil,1,false)
  for k=1,N do io.open(("%s/abc-%03d"):format(workdir, k), "w"):close() end
  for k=1,N do io.open(("%s/xyz-%03d"):format(workdir, k), "w"):close() end
  local db = sqlite.open(workdir.."/foo.db")
  db:exec("CREATE TABLE mytable(x,y,z);")
  db:close()

  panel.UpdatePanel(nil,1)
  panel.RedrawPanel(nil,1,
    { CurrentItem=N+2; --> 1*".." + N*"abc*" --> "foo.db"
      TopPanelItem=N-4; })
  assert(panel.GetPanelInfo(nil,1).TopPanelItem == N-4)

  for k=1,5 do
    local curr = panel.GetCurrentPanelItem(nil,1)
    assert(curr.FileName == "foo.db")
    curr = panel.GetPanelItem(nil,1,N+2)
    assert(curr.FileName == "foo.db")
    if k < 5 then
      Keys("Enter") -- open plugin panel and set mainDB mode if not set already
      if panel.GetPanelItem(nil,1,2).FileName == "main" then
        Keys("Down Enter AltShiftF6") -- option "Multi-database mode" was ON
      end
      if     k==1 then assert_close("Enter")                          -- mainDB mode
      elseif k==2 then assert_close("CtrlBackSlash")                  -- *
      elseif k==3 then assert_close("AltShiftF6 Enter Up Enter")      -- multiDB mode
      elseif k==4 then assert_close("AltShiftF6 Enter CtrlBackSlash") -- *
      end
    end
  end

  -- remove temporary files
  far.RecursiveSearch(workdir, "abc-*;xyz-*;foo.db",
    function(_,fullname) win.DeleteFile(fullname) end)
  assert(panel.SetPanelDirectory(nil, 1, win.GetEnv("FARHOME")))
  assert(win.RemoveDir(workdir))

  -- restore sort mode and order
  local sortmode = old_panel_info.SortMode
  local reverse = bit64.band(old_panel_info.Flags,F.PFLAGS_REVERSESORTORDER) ~= 0
  if sortmode < 100 then
    panel.SetSortMode (nil,1,old_panel_info.SortMode)
    panel.SetSortOrder(nil,1,reverse)
  else
    Panel.SetCustomSortMode(sortmode, 0, reverse and "reverse" or "direct")
  end
end

local function test_user_modules()
  local umod1, umod2 = 'usermod1.lua', 'usermod2.lua' -- user module files

  -- Create a database file in a separate directory.
  -- The database contains the usermodule table listing 2 modules.
  local db, DbFileName = CreateNewDB("far-polygon")
  local query = ([[
    CREATE TABLE '%s' (script, load_priority, enabled);
    INSERT INTO '%s' VALUES('%s', 40, 1);
    INSERT INTO '%s' VALUES('%s', 50, 1);
    ]]):format(USERMOD_TABLE, USERMOD_TABLE, umod1, USERMOD_TABLE, umod2)
  db:exec(query)
  db:close()

  local umod_content = [=[
    -- filename w/o path will be used as an SQL table name
    local fname = ...
    assert(type(fname) == "string")
    fname = assert(fname:match("[^/]+$")) -- remove path

    local function Check_tInfo (t)
      assert(type(t.db)          == "userdata")
      assert(type(t.file_name)   == "string")
    --assert(type(t.multi_db)    == "boolean")
      assert(type(t.schema)      == "string")
      assert(type(t.panel_mode)  == "string")
      assert(type(t.curr_object) == "string")
    --assert(type(t.rowid_name)  == "string")
      assert(type(t.get_rowid)   == "function")
    end

    local mod = {}
    mod.OnOpenConnection = function(info)
      Check_tInfo(info)
      info.db:exec( ([[
        BEGIN TRANSACTION;
        CREATE TABLE '%s' (key, value);
        INSERT INTO '%s' VALUES('ProcessPanelEvent', 0);
        INSERT INTO '%s' VALUES('ProcessPanelInput', 0);
        INSERT INTO '%s' VALUES('ClosePanel', 0);
        END TRANSACTION;
      ]]):format(fname, fname, fname, fname) )
    end

    local numEvent = 0
    mod.ProcessPanelEvent = function(info, handle, event, param)
      if numEvent < 2 then -- to not slow down the test
        numEvent = numEvent + 1
        Check_tInfo(info)
        info.db:exec("UPDATE '" ..fname.. "' SET value=value+1 WHERE key='ProcessPanelEvent';")
      end
    end

    mod.ProcessPanelInput = function(info, handle, rec)
      Check_tInfo(info)
      info.db:exec("UPDATE '" ..fname.. "' SET value=value+1 WHERE key='ProcessPanelInput';")
    end

    mod.ClosePanel = function(info, handle)
      Check_tInfo(info)
      info.db:exec("UPDATE '" ..fname.. "' SET value=value+1 WHERE key='ClosePanel';")
    end

    UserModule(mod)
  ]=]

  -- Create 2 usermodule files
  local DbDir = DbFileName:match(".*/")
  local fp
  fp = io.open(DbDir..umod1, "w"); fp:write(umod_content); fp:close()
  fp = io.open(DbDir..umod2, "w"); fp:write(umod_content); fp:close()

  -- Open the database file with the switch "i" which enables loading of individual user modules.
  -- Then close the file. This should trigger all types of events in each user module.
  PluginOpenFile(DbFileName, "i")
  assert_close("Enter")

  -- Check the database file content and compare it against the expected values.
  db = assert(sqlite3.open(DbFileName))
  for k=1,2 do
    local query = ("SELECT * FROM '%s';"):format(k==1 and umod1 or umod2)
    local a,b,c = 0,0,0
    for row in db:nrows(query) do
      if     row.key == "ProcessPanelEvent" then assert(row.value==2); a=a+1
      elseif row.key == "ProcessPanelInput" then assert(row.value==1); b=b+1
      elseif row.key == "ClosePanel"        then assert(row.value==1); c=c+1
      else error("unknown key")
      end
    end
    assert(a==1 and b==1 and c==1)
  end
  db:close()

  -- Clean up
  win.DeleteFile(DbDir..umod1)
  win.DeleteFile(DbDir..umod2)
  win.DeleteFile(DbFileName)
end

local AllTests = {
  test_command_line           = test_command_line;
  test_dbitem_view            = test_dbitem_view;
  test_edit_row               = test_edit_row;
  test_panel_modes            = test_panel_modes;
  test_panel_position_on_exit = test_panel_position_on_exit;
  test_pragmas_view           = test_pragmas_view;
  test_query_from_editor      = test_query_from_editor;
  test_r2837                  = test_r2837;
  test_table_export           = test_table_export;
  test_user_modules           = test_user_modules;
  test_without_rowid          = test_without_rowid;
}

AllTests.test_all = function()
  local t = {}
  for name in pairs(AllTests) do
    if name ~= "test_all" then t[#t+1]=name end
  end
  table.sort(t)
  for _,name in ipairs(t) do AllTests[name]() end
end

return AllTests
