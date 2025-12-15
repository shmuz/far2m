local asrt = require "far2.assert"

local F = far.Flags

local function test_Sel_Cmdline(args)
  for _, str in ipairs(args) do
    asrt.istrue(Area.Shell)
    panel.SetCmdLine(nil, str)

    local act = 0
    for opt = 0,4 do
      asrt.eq(Editor.Sel(act,opt), 0) -- no block exists, zeros returned
    end

    local line, pos, w = 1, 13, 5

    panel.SetCmdLineSelection(nil, pos, pos+w)
    asrt.eq(Editor.Sel(act,0), line)
    asrt.eq(Editor.Sel(act,1), pos)
    asrt.eq(Editor.Sel(act,2), line)
    asrt.eq(Editor.Sel(act,3), pos + w)
    asrt.eq(Editor.Sel(act,4), 1)
    --------------------------------------------------------------------------------------------------
    act = 1
    panel.SetCmdLineSelection(nil, pos, pos+w)

    asrt.eq(1, Editor.Sel(act, 0)) -- set cursor at block start
    asrt.eq(panel.GetCmdLinePos(), pos)

    asrt.eq(1, Editor.Sel(act, 1)) -- set cursor next to block end
    asrt.eq(panel.GetCmdLinePos(), pos + w + 1)
    --------------------------------------------------------------------------------------------------
    for act = 2,3 do
      panel.SetCmdLinePos(nil,pos)             -- set block start
      asrt.eq(1, Editor.Sel(act,0))          -- +++
      panel.SetCmdLinePos(nil,pos + w)         -- set block end (it also selects the block)
      asrt.eq(1, Editor.Sel(act,1))          -- +++

      asrt.eq(Editor.Sel(0,0), 1)
      asrt.eq(Editor.Sel(0,1), pos)
      asrt.eq(Editor.Sel(0,2), 1)
      asrt.eq(Editor.Sel(0,3), pos + w - 1)
      asrt.eq(Editor.Sel(0,4), 1)
    end
    --------------------------------------------------------------------------------------------------
    asrt.eq(1, Editor.Sel(4)) -- reset the block
    asrt.eq(Editor.Sel(0,4), 0)

    panel.SetCmdLine(nil, "")
  end
end

local function test_Sel_Dialog(args)
  Keys("F7 Del")
  asrt.istrue(Area.Dialog)
  local inf = actl.GetWindowInfo()
  local hDlg = asrt.udata(inf.Id)
  local EditPos = asrt.num(hDlg:GetFocus())

  for _, str in ipairs(args) do
    hDlg:SetText(EditPos, str)

    local act = 0
    for opt = 0,4 do
      asrt.eq(Editor.Sel(act,opt), 0) -- no block exists, zeros returned
    end

    local tSel = { BlockType = F.BTYPE_STREAM; BlockStartLine = 1; BlockStartPos = 13,
                   BlockWidth = 5; BlockHeight = 1; }

    hDlg:SetSelection(EditPos, tSel)
    asrt.eq(Editor.Sel(act,0), tSel.BlockStartLine)
    asrt.eq(Editor.Sel(act,1), tSel.BlockStartPos)
    asrt.eq(Editor.Sel(act,2), tSel.BlockStartLine)
    asrt.eq(Editor.Sel(act,3), tSel.BlockStartPos + tSel.BlockWidth - 1) -- [-1: DIFFERENT FROM EDITOR]
    asrt.eq(Editor.Sel(act,4), tSel.BlockType)
    --------------------------------------------------------------------------------------------------
    act = 1
    hDlg:SetSelection(EditPos, tSel)

    asrt.eq(1, Editor.Sel(act, 0)) -- set cursor at block start
    local pos = asrt.table(hDlg:GetCursorPos(EditPos))
    asrt.eq(pos.Y+1, tSel.BlockStartLine)
    asrt.eq(pos.X+1, tSel.BlockStartPos)

    asrt.eq(1, Editor.Sel(act, 1)) -- set cursor next to block end
    pos = asrt.table(hDlg:GetCursorPos(EditPos))
    asrt.eq(pos.Y+1, tSel.BlockStartLine)
    asrt.eq(pos.X+1, tSel.BlockStartPos + tSel.BlockWidth)
    --------------------------------------------------------------------------------------------------
    for act = 2,3 do
      hDlg:SetCursorPos(EditPos, {X=10; Y=0})  -- set block start
      asrt.eq(1, Editor.Sel(act,0))          -- +++
      hDlg:SetCursorPos(EditPos, {X=20; Y=0})  -- set block end (it also selects the block)
      asrt.eq(1, Editor.Sel(act,1))          -- +++

      asrt.eq(Editor.Sel(0,0), 1)
      asrt.eq(Editor.Sel(0,1), 10+1)
      asrt.eq(Editor.Sel(0,2), 1)
      asrt.eq(Editor.Sel(0,3), 20)
      asrt.eq(Editor.Sel(0,4), 1)
    end
    --------------------------------------------------------------------------------------------------
    asrt.eq(1, Editor.Sel(4)) -- reset the block
    asrt.eq(Editor.Sel(0,4), 0)
  end

  Keys("Esc")
end

local function test_Sel_Editor(args)
  local R2T, T2R = editor.RealToTab, editor.TabToReal
  local T2R2T = function(id, y, x)
    return R2T(id, y, T2R(id, y, x)) -- needed when the column position is "inside" a tab
  end

  Keys("ShiftF4 Del Enter")
  asrt.istrue(Area.Editor)

  for _, str in ipairs(args) do
    Keys("CtrlA Del")

    for k=1,8 do
      editor.InsertString()
      editor.SetString(nil, k, str)
    end

    local act = 0
    for opt = 0,4 do
      asrt.eq(Editor.Sel(act,opt), 0) -- no block exists, zeros returned
    end

    local line, pos, w, h = 3, 13, 5, 4

    for ii=1,2 do
      local typ = ii==1 and "BTYPE_STREAM" or "BTYPE_COLUMN"
      local ref_x1 = (ii==1 and R2T or T2R2T)(nil, line, pos)
      local ref_x2 = (ii==1 and R2T or T2R2T)(nil, line-1+h, pos+w)

      asrt.istrue(editor.Select(nil, typ, line, pos, w, h)) -- select the block
      asrt.eq(Editor.Sel(act,0), line)      -- get block start line
      asrt.eq(Editor.Sel(act,1), ref_x1)    -- get block start pos
      asrt.eq(Editor.Sel(act,2), line-1+h)  -- get block end line
      asrt.eq(Editor.Sel(act,3), ref_x2)    -- get block end pos
      asrt.eq(Editor.Sel(act,4), ii)        -- get block type
    end
    --------------------------------------------------------------------------------------------------
    act = 1
    for ii=1,2 do
      local typ = ii==1 and "BTYPE_STREAM" or "BTYPE_COLUMN"
      local ref_x1 = ii==1 and pos or T2R(nil, line, pos) or pos
      local ref_x2 = ii==1 and pos+w or T2R(nil, line-1+h, pos+w)

      local inf
      asrt.istrue(editor.Select(nil, typ, line, pos, w, h)) -- select the block

      asrt.eq(1, Editor.Sel(act, 0)) -- set cursor at block start
      inf = editor.GetInfo()
      asrt.eq(inf.CurLine, line)
      asrt.eq(inf.CurPos, ref_x1)

      asrt.eq(1, Editor.Sel(act, 1)) -- set cursor next to block end
      inf = editor.GetInfo()
      asrt.eq(inf.CurLine, line-1+h)
      asrt.eq(inf.CurPos, ref_x2)
    end
    --------------------------------------------------------------------------------------------------
    local y1,x1,y2,x2 = 2,10,6,20
    for act = 2,3 do
      editor.SetPosition(nil,y1,x1)    -- set block start
      asrt.eq(1, Editor.Sel(act,0))  -- +++
      editor.SetPosition(nil,y2,x2)    -- set block end (it also selects the block)
      asrt.eq(1, Editor.Sel(act,1))  -- +++

      local ref_x1 = (act==2 and R2T or T2R2T)(nil,y1,x1)
      local ref_x2 = (act==2 and R2T or T2R2T)(nil,y2,x2)

      asrt.eq(Editor.Sel(0,0), y1)                 -- get block start line
      asrt.eq(Editor.Sel(0,1), ref_x1)             -- get block start pos
      asrt.eq(Editor.Sel(0,2), y2)                 -- get block end line
      asrt.eq(Editor.Sel(0,3), ref_x2)             -- get block end pos
      asrt.eq(Editor.Sel(0,4), act==2 and 1 or 2)  -- get block type
    end
    --------------------------------------------------------------------------------------------------
    asrt.eq(1, Editor.Sel(4)) -- reset the block
    asrt.eq(Editor.Sel(0,4), 0)
  end

  asrt.istrue(editor.Quit())
end

local function test_Misc()
  local fname = far.MkTemp()
  local flags = {EF_NONMODAL=1, EF_IMMEDIATERETURN=1, EF_DISABLEHISTORY=1, EF_DELETEONCLOSE=1}
  asrt.eq(editor.Editor(fname,nil,nil,nil,nil,nil,flags), F.EEC_MODIFIED)
  asrt.istrue(Area.Editor)

  local EI = asrt.table(editor.GetInfo())

  local str = ("123456789-"):rep(4)
  local str2, num

  -- test Editor.Value
  editor.SetString(nil,1,str)
  asrt.eq(Editor.Value, str)

  -- test insertion with overtype=OFF
  editor.SetString(nil,1,str)
  editor.SetPosition(nil,1,1)
  editor.InsertText(nil, "AB")
  asrt.eq(Editor.Value, "AB"..str)

  -- test insertion with overtype=ON
  editor.SetString(nil,1,str)
  Keys("Ins")
  editor.SetPosition(nil,1,1)
  editor.InsertText(nil, "CD")
  asrt.eq(Editor.Value, "CD"..str:sub(3))
  Keys("Ins")

  -- test insertion beyond EOL (overtype=ON then OFF)
  num = 20
  asrt.istrue(editor.SetParam(nil, "ESPT_CURSORBEYONDEOL", true))
  str2 = str .. (" "):rep(num) .. "AB"
  for _=1,2 do
    Keys("Ins")
    editor.SetString(nil,1,str)
    editor.SetPosition(nil, 1, #str + 1 + num)
    editor.InsertText(nil, "AB")
    asrt.eq(Editor.Value, str2)
  end
  asrt.istrue(editor.SetParam(nil, "ESPT_CURSORBEYONDEOL", band(EI.Options, F.EOPT_CURSORBEYONDEOL) ~= 0))

  editor.Quit()
end

-- "Several lines are merged into one".
local function test_issue_3129()
  local fname = far.InMyTemp("far2m-"..win.Uuid("L"):sub(1,8))
  local fp = assert(io.open(fname, "w"))
  fp:close()
  local flags = {EF_NONMODAL=1, EF_IMMEDIATERETURN=1, EF_DISABLEHISTORY=1}
  assert(editor.Editor(fname,nil,nil,nil,nil,nil,flags) == F.EEC_MODIFIED)
  for k=1,3 do
    editor.InsertString()
    editor.SetString(nil, k, "foo")
  end
  asrt.istrue (editor.SaveFile())
  asrt.istrue (editor.Quit())
  actl.Commit()
  fp = assert(io.open(fname))
  local k = 0
  for line in fp:lines() do
    k = k + 1
    asrt.eq (line, "foo")
  end
  fp:close()
  win.DeleteFile(fname)
  asrt.eq (k, 3)
end

local function test_editor_all()
  local args = {
    ("123456789-"):rep(4),     -- plain ASCII
    ("12\t4\t6789-"):rep(4),   -- includes tabs
    ("12🔥4🔥6789-"):rep(4),   -- includes 🔥 (a multi-byte double-width character)
    ("1Ю23456789-"):rep(4),    -- insert 'Ю' (a multi-byte character) into position 2
    ("1Ю2\t4\t6789-"):rep(4),  -- ditto
    ("1Ю2🔥4🔥6789-"):rep(4),  -- ditto
  }
  test_Sel_Cmdline(args)
  test_Sel_Dialog(args)
  test_Sel_Editor(args)
  test_Misc()
  test_issue_3129()
end

return {
  test_editor_all = test_editor_all;
}
