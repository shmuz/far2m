---- started: 2026-11-22
-- Regression test for stacked modal frame redraw (Manager::DeleteCommit):
-- When a full-screen modal editor is opened on top of a smaller modal dialog
-- and then closed via keyboard input, both the underlying base frame (Panels)
-- and the top modal dialog itself must be redrawn immediately—including all
-- screen rows and 64-bit color attributes—without touching disk.

local asrt = require "far2.assert"
local F = far.Flags
local ed_text = "We are in the editor"
local tx1, tx2, tx3, at1, at2, at3
local stage

local function GetScreenRow() -- get the 2-nd row
  return far.GetScreenRect(1, 2, Far.Width, 2)
end

local function check_stage(val)
  asrt.eq(stage, val)
  stage = stage + 1
end

local function Test()
  asrt.table(tx1)
  asrt.table(tx2)
  asrt.table(tx3)
  asrt.eq(tx1[1], tx3[1]) -- same text
  for i = 1, #at1 do      -- same attributes
    asrt.eq(at1[i], at3[i])
  end
  asrt.neq(tx1[1], tx2[1])         -- texts must differ
  asrt.isnil(tx1[1]:find(ed_text)) -- known text should not be found
  asrt.eq(tx2[1]:find(ed_text), 1) -- known text should be found
  asrt.eq(#at1, #at2)
  asrt.eq(#at1, #at3)
end

local function in_editor(hDlg)
  check_stage(3)
  asrt.istrue(Area.Editor)
  editor.SetString(nil, 1, ed_text)
  editor.Redraw()
  tx2, at2 = GetScreenRow()
  editor.Quit()
  asrt.istrue(Area.Dialog)
  tx3, at3 = GetScreenRow()
  hDlg:Close()
end

local proc = function(hDlg, msg, param1, param2)
  if msg == F.DN_INITDIALOG then
    check_stage(1)
    mf.postmacro(Keys, "F4")
  elseif msg == F.DN_KEY and param2 == F.KEY_F4 then
    check_stage(2)
    tx1, at1 = GetScreenRow()
    mf.postmacro(in_editor, hDlg)
    editor.Editor("Modal editor")
  elseif msg == F.DN_CLOSE then
    check_stage(4)
  end
end

local function main()
  asrt.istrue(Area.Shell)
  tx1, tx2, tx3, at1, at2, at3 = nil, nil, nil, nil, nil, nil
  stage = 1
  local dlg_w, dlg_h = 20, 4
  local Items = { { F.DI_DOUBLEBOX, 0, 0, dlg_w - 1, dlg_h - 1, 0, 0, 0, 0, "Test Dialog" } }
  far.Dialog(nil, -1, -1, dlg_w, dlg_h, nil, Items, nil, proc)
  check_stage(5)
  Test()
  asrt.istrue(Area.Shell)
end

return main
