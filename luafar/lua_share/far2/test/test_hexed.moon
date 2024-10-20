filename=far.InMyTemp "hexed_test"
filesize=0x10000
fillchar="-"

F=far.Flags
textarea=false

GoTo=(addr)->
  Keys "AltF8"
  assert Area.Dialog
  assert Dlg.Id == "4FEA7612-507B-453F-A83D-53837CAD86ED"
  print string.format "0x%X", addr
  Keys "Enter"

Write=(addr,str)->
  GoTo(addr)
  Keys "F3"
  for c in str\gmatch"."
    if textarea
      Keys c
    else
      hex = string.format "%02X", string.byte c
      Keys hex\sub(1,1), hex\sub(2,2)
    --win.Sleep 100 -- debugging
  Keys "Tab F9" -- switch editing area, save the file, exit from editing mode
  textarea = not textarea

pieces = { -- pieces must not overlap
  {addr:0x0,    text:"foo bar baz"},
  {addr:0x3579, text:"123456"},
  {addr:0x789A, text:"----- hex editor ------"},
  {addr:0xD000, text:"abc/ABC"},
}

Test=->
  -- Keys "EnOut" -- debugging

  -- create a test file
  fp = assert io.open filename,"w"
  str = fillchar\rep filesize
  fp\write str
  fp\close!

  -- open the viewer and run Hex editor
  viewer.Viewer filename,nil,nil,nil,nil,nil,F.VF_NONMODAL+F.VF_IMMEDIATERETURN+F.VF_DISABLEHISTORY
  assert Area.Viewer
  mf.eval "CtrlF4",2
  assert Area.Dialog
  assert Dlg.Id == "02FFA2B9-98F8-4A73-B311-B3431340E272"

  -- write pieces
  textarea=false
  for pp in *pieces do Write pp.addr,pp.text

  -- quit hex editor and viewer
  assert Area.Dialog
  assert Dlg.Id == "02FFA2B9-98F8-4A73-B311-B3431340E272"
  Keys "Esc"
  assert Area.Viewer
  Keys "Esc"

  -- prepare the reference string
  table.sort pieces, (a1,a2)-> a1.addr<a2.addr
  ref=""
  cur=0
  for pp in *pieces
    ref ..= (string.rep fillchar,pp.addr-cur)..pp.text
    cur = pp.addr+#pp.text
  ref..=string.rep fillchar,filesize-cur

  -- read the resulting file and compare with reference
  fp = assert io.open filename
  str = fp\read "*all"
  fp\close!
  assert str==ref, "file has wrong content"

return Test
