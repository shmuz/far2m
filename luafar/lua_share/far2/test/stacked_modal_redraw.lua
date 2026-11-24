---- started: 2026-11-22
-- Regression test for stacked modal frame redraw (Manager::DeleteCommit):
-- When a full-screen modal editor is opened on top of a smaller modal dialog
-- and then closed via editor.Quit(), both the underlying base frame (Panels)
-- and the top modal dialog itself must be redrawn immediately—including all
-- screen rows and 64-bit color attributes—without touching disk.

local asrt = require "far2.assert"
local F = far.Flags
local dlg_title = "Test/Dialog"
local ed_text = "We are in the editor / test"
local tx1, tx2, tx3, at1, at2, at3
local stage

local function GetScreen()
  return far.GetScreenRect(1, 2, Far.Width, Far.Height)
end

local function check_stage(val)
  asrt.eq(stage, val)
  stage = stage + 1
end

local function has_text(rows, substr)
  for _, row in ipairs(rows) do
    if row:find(substr, 1, true) then
      return true
    end
  end
  return false
end

local function Test()
  asrt.table(tx1)
  asrt.table(tx2)
  asrt.table(tx3)
  asrt.eq(#tx1, #tx3)
  asrt.eq(#at1, #at2)
  asrt.eq(#at1, #at3)

  for y = 1, #tx1 do
    asrt.eq(tx1[y], tx3[y]) -- every row's text must be restored
  end
  for i = 1, #at1 do
    asrt.eq(at1[i], at3[i]) -- every cell's 64-bit attributes must be restored
  end

  asrt.istrue(has_text(tx1, dlg_title))  -- dialog was visible before editor
  asrt.isfalse(has_text(tx1, ed_text))   -- editor text was not on screen yet
  asrt.eq(tx2[1]:find(ed_text, 1, true), 1) -- row 2 (line 1 in editor) has ed_text
  asrt.isfalse(has_text(tx2, dlg_title)) -- editor covered the dialog
end

local function in_editor(hDlg)
  check_stage(3)
  asrt.istrue(Area.Editor)
  editor.SetString(nil, 1, ed_text)
  editor.Redraw()
  tx2, at2 = GetScreen()
  editor.Quit()
  asrt.istrue(Area.Dialog)
  tx3, at3 = GetScreen()
  hDlg:Close()
end

local function proc(hDlg, msg, param1, param2)
  if msg == F.DN_INITDIALOG then
    check_stage(1)
    mf.postmacro(Keys, "F4")
  elseif msg == F.DN_KEY and param2 == F.KEY_F4 then
    check_stage(2)
    tx1, at1 = GetScreen()
    mf.postmacro(in_editor, hDlg)
    editor.Editor("Modal editor", nil, nil, nil, nil, nil, F.EF_CREATENEW + F.EF_DISABLEHISTORY)
  elseif msg == F.DN_CLOSE then
    check_stage(4)
  end
end

local function main()
  asrt.istrue(Area.Shell)
  tx1, tx2, tx3, at1, at2, at3 = nil, nil, nil, nil, nil, nil
  stage = 1
  local dlg_w, dlg_h = 20, 4
  local Items = { { F.DI_DOUBLEBOX, 0, 0, dlg_w - 1, dlg_h - 1, 0, 0, 0, 0, dlg_title } }
  far.Dialog(nil, -1, -1, dlg_w, dlg_h, nil, Items, nil, proc)
  check_stage(5)
  Test()
  asrt.istrue(Area.Shell)
end

return main
