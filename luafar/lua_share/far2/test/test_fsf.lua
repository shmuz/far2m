-- test Far Standard Functions

local asrt = require "far2.assert"
local F = far.Flags
local VK = win.GetVirtualKeys()
local band, bor = bit64.band, bit64.bor


local function test_Clipboard()
  local orig = far.PasteFromClipboard()
  local values = { "Человек", "foo", "", n=3 }
  for k=1,values.n do
    local v = values[k]
    far.CopyToClipboard(v)
    asrt.eq(far.PasteFromClipboard(), v)
  end
  if orig then
    far.CopyToClipboard(orig)
    asrt.eq(far.PasteFromClipboard(), orig)
  end
end


local function test_ProcessName()
  asrt.istrue  (far.ProcessName("PN_CHECKMASK", "f*.ex?"))
  asrt.istrue  (far.CheckMask("f*.ex?"))
  asrt.istrue  (far.CheckMask("/(abc)?def/"))
  asrt.isfalse (far.CheckMask("/[[[/"))

  asrt.eq      (far.ProcessName("PN_GENERATENAME", "a??b.*", "cdef.txt"), "adeb.txt")
  asrt.eq      (far.GenerateName("a??b.*", "cdef.txt"),     "adeb.txt")
  asrt.eq      (far.GenerateName("a??b.*", "cdef.txt", 50), "adeb.txt")
  asrt.eq      (far.GenerateName("a??b.*", "cdef.txt", 2),  "adbef.txt")

  asrt.istrue  (far.ProcessName("PN_CMPNAME", "f*.ex?", "ftp.exe"))
  asrt.istrue  (far.CmpName("f*.ex?",      "ftp.exe"        ))
  asrt.istrue  (far.CmpName("f*.ex?",      "fc.exe"         ))
  asrt.istrue  (far.CmpName("f*.ex?",      "f.ext"          ))

  asrt.isfalse (far.CmpName("f*.ex?",      "FTP.exe", "PN_CASESENSITIVE" ))
  asrt.istrue  (far.CmpName("f*.ex?",      "FTP.exe"        ))

  asrt.isfalse (far.CmpName("f*.ex?",      "a/f.ext"        ))
  asrt.isfalse (far.CmpName("f*.ex?",      "a/f.ext", 0     ))
  asrt.istrue  (far.CmpName("f*.ex?",      "a/f.ext", "PN_SKIPPATH" ))

  asrt.istrue  (far.CmpName("*co*",        "color.ini"      ))
  asrt.istrue  (far.CmpName("*co*",        "edit.com"       ))
  asrt.istrue  (far.CmpName("[c-ft]*.txt", "config.txt"     ))
  asrt.istrue  (far.CmpName("[c-ft]*.txt", "demo.txt"       ))
  asrt.istrue  (far.CmpName("[c-ft]*.txt", "faq.txt"        ))
  asrt.istrue  (far.CmpName("[c-ft]*.txt", "tips.txt"       ))
  asrt.istrue  (far.CmpName("*",           "foo.bar"        ))
  asrt.istrue  (far.CmpName("*.cpp",       "foo.cpp"        ))
  asrt.isfalse (far.CmpName("*.cpp",       "foo.abc"        ))
  asrt.isfalse (far.CmpName("*|*.cpp",     "foo.abc"        )) -- exclude mask not supported
  asrt.isfalse (far.CmpName("*,*",         "foo.bar"        )) -- mask list not supported

  asrt.istrue  (far.ProcessName("PN_CMPNAMELIST", "*", "foo.bar"))
  asrt.istrue  (far.CmpNameList("*",          "foo.bar"    ))
  asrt.istrue  (far.CmpNameList("*.cpp",      "foo.cpp"    ))
  asrt.isfalse (far.CmpNameList("*.cpp",      "foo.abc"    ))

  asrt.istrue  (far.CmpNameList("*|*.cpp",    "foo.abc"    )) -- exclude mask IS supported
  asrt.istrue  (far.CmpNameList("|*.cpp",     "foo.abc"    ))
  asrt.istrue  (far.CmpNameList("*|",         "foo.abc"    ))
  asrt.istrue  (far.CmpNameList("*|bar|*",    "foo.abc"    ))
  asrt.isfalse (far.CmpNameList("*|*.abc",    "foo.abc"    ))
  asrt.isfalse (far.CmpNameList("|",          "foo.abc"    ))

  asrt.istrue  (far.CmpNameList("*.aa,*.bar", "foo.bar"    ))
  asrt.istrue  (far.CmpNameList("*.aa,*.bar", "c:/foo.bar" ))
  asrt.istrue  (far.CmpNameList("/.+/",       "c:/foo.bar" ))
  asrt.istrue  (far.CmpNameList("/bar$/",     "c:/foo.bar" ))
  asrt.isfalse (far.CmpNameList("/dar$/",     "c:/foo.bar" ))
  asrt.istrue  (far.CmpNameList("/abcd/;*",    "/abcd/foo.bar", "PN_SKIPPATH"))
  asrt.istrue  (far.CmpNameList("/Makefile(.+)?/", "Makefile"))
  asrt.istrue  (far.CmpNameList("/makefile([._\\-].+)?$/i", "Makefile", "PN_SKIPPATH"))

  asrt.isfalse (far.CmpNameList("f*.ex?",    "a/f.ext", 0     ))
  asrt.istrue  (far.CmpNameList("f*.ex?",    "a/f.ext", "PN_SKIPPATH" ))

  asrt.istrue  (far.CmpNameList("/ BAR ; /xi  ;*.md", "bar;foo"))
  asrt.isfalse (far.CmpNameList("/ BAR ; /xi  ;*.md", "bar,foo"))
  asrt.istrue  (far.CmpNameList("/ BAR ; /xi  ;*.md", "README.md"))
  asrt.isfalse (far.CmpNameList("/ BAR ; /xi  ;*.md", "README.me"))
end


local function test_path_funcs()
  local dir = asrt.table(panel.GetPanelDirectory(nil,1)).Name
  asrt.eq (far.GetCurrentDirectory(), dir)
  asrt.eq (far.ConvertPath("abcd"), win.JoinPath(dir, "abcd"))
  asrt.eq (far.ConvertPath([[/foo/bar/../../abc]], "CPM_FULL"), [[/abc]])
end


local function test_text_funcs()
  asrt.eq(far.FormatFileSize(123456, 8), "  123456")
  asrt.eq(far.FormatFileSize(123456, -8), "123456  ")

  asrt.istrue  (far.LIsAlpha("A"))
  asrt.istrue  (far.LIsAlpha("Я"))
  asrt.isfalse (far.LIsAlpha("7"))
  asrt.isfalse (far.LIsAlpha(";"))

  asrt.istrue  (far.LIsAlphanum("A"))
  asrt.istrue  (far.LIsAlphanum("Я"))
  asrt.istrue  (far.LIsAlphanum("7"))
  asrt.isfalse (far.LIsAlphanum(";"))

  asrt.isfalse (far.LIsLower("A"))
  asrt.istrue  (far.LIsLower("a"))
  asrt.isfalse (far.LIsLower("Я"))
  asrt.istrue  (far.LIsLower("я"))
  asrt.isfalse (far.LIsLower("7"))
  asrt.isfalse (far.LIsLower(";"))

  asrt.istrue  (far.LIsUpper("A"))
  asrt.isfalse (far.LIsUpper("a"))
  asrt.istrue  (far.LIsUpper("Я"))
  asrt.isfalse (far.LIsUpper("я"))
  asrt.isfalse (far.LIsUpper("7"))
  asrt.isfalse (far.LIsUpper(";"))

  asrt.eq (far.LLowerBuf("abc-ABC-эюя-ЭЮЯ-7;"), "abc-abc-эюя-эюя-7;")
  asrt.eq (far.LUpperBuf("abc-ABC-эюя-ЭЮЯ-7;"), "ABC-ABC-ЭЮЯ-ЭЮЯ-7;")

  asrt.lt (far.LStricmp("abc", "def") , 0)
  asrt.gt (far.LStricmp("def", "abc") , 0)
  asrt.eq (far.LStricmp("abc", "abc") , 0)
  asrt.lt (far.LStricmp("ABC", "def") , 0)
  asrt.gt (far.LStricmp("DEF", "abc") , 0)
  asrt.eq (far.LStricmp("ABC", "abc") , 0)

  asrt.lt (far.LStrnicmp("abc", "def", 3), 0)
  asrt.gt (far.LStrnicmp("def", "abc", 3), 0)
  asrt.eq (far.LStrnicmp("abc", "abc", 3), 0)
  asrt.lt (far.LStrnicmp("ABC", "def", 3), 0)
  asrt.gt (far.LStrnicmp("DEF", "abc", 3), 0)
  asrt.eq (far.LStrnicmp("ABC", "abc", 3), 0)
  asrt.eq (far.LStrnicmp("111abc", "111def", 3), 0)
  asrt.lt (far.LStrnicmp("111abc", "111def", 4), 0)

  asrt.lt (far.LStrcmp("abc", "def"), 0)
  asrt.gt (far.LStrcmp("def", "abc"), 0)
  asrt.eq (far.LStrcmp("abc", "abc"), 0)
  asrt.lt (far.LStrcmp("ABC", "def"), 0)
  asrt.lt (far.LStrcmp("DEF", "abc"), 0)
  asrt.lt (far.LStrcmp("ABC", "abc"), 0)

  asrt.lt (far.LStrncmp("abc", "def", 3), 0)
  asrt.gt (far.LStrncmp("def", "abc", 3), 0)
  asrt.eq (far.LStrncmp("abc", "abc", 3), 0)
  asrt.lt (far.LStrncmp("ABC", "def", 3), 0)
  asrt.lt (far.LStrncmp("DEF", "abc", 3), 0)
  asrt.lt (far.LStrncmp("ABC", "abc", 3), 0)
  asrt.eq (far.LStrncmp("111abc", "111def", 3), 0)
  asrt.lt (far.LStrncmp("111abc", "111def", 4), 0)
end


local function test_Cells()
  asrt.eq(6, far.StrCellsCount("abcdef"))
  asrt.eq(4, far.StrCellsCount("abcdef", 4))
  asrt.eq(6, far.StrCellsCount("🔥🔥🔥"))
  asrt.eq(4, far.StrCellsCount("🔥🔥🔥", 2))

  local data = {
  --  / input -------------- / output /
    { "🔥🔥🔥", 100, false,    3, 6 },
    { "🔥🔥🔥", 100, true,     3, 6 },
    { "🔥🔥🔥",   2, false,    1, 2 },
    { "A🔥",      2, false,    1, 1 },
    { "A🔥",      2, true,     2, 3 },
  }
  for _,v in ipairs(data) do
    local nChars, nCells = far.StrSizeOfCells(unpack(v, 1, 3))
    asrt.eq(nChars, v[4])
    asrt.eq(nCells, v[5])
  end
end


local function test_DetectCodePage()
  asrt.func(far.DetectCodePage)
  local test = require "far2.test.codepage.test_codepage"
  local dir = os.getenv("FARHOME").."/Plugins/luafar/lua_share/far2/test/codepage"
  local pass, total = test(dir)
  asrt.num(pass)
  asrt.num(total)
  assert(pass > 0 and pass == total)
end


local function test_group_owner_links()
  local stinfo = asrt.table(far.PluginStartupInfo())
  local fname = asrt.str(stinfo.ModuleName)
  --------
  local group = asrt.str(far.GetFileGroup(nil, fname))
  asrt.neq(group, "")
  --------
  local owner = asrt.str(far.GetFileOwner(nil, fname))
  asrt.neq(owner, "")
  --------
  asrt.eq(1, far.GetNumberOfLinks(fname))
  --------
  local linkname = far.InMyTemp("templink-1")
  win.DeleteFile(linkname)
  asrt.istrue(far.MkLink(fname, linkname))
  local dest = asrt.str(far.GetReparsePointInfo(linkname))
  asrt.eq(dest, fname)
  asrt.istrue(win.DeleteFile(linkname))
  --------
end


local function test_misc()
  asrt.func(far.BackgroundTask)
end


local function test_keys_names()
  -------- InputRecordToKey, InputRecordToName
  local rec = { EventType = asrt.num(F.KEY_EVENT); }

  local codes = { VK.INSERT, VK.NUMPAD0 }

  local bits = { F.ENHANCED_KEY, F.LEFT_CTRL_PRESSED, F.LEFT_ALT_PRESSED, F.SHIFT_PRESSED }
  local states = {} -- will contain all bitwise OR combinations of 'bits'
  for i = 0, 2^#bits - 1 do
    local v = 0
    for j = 0, #bits - 1 do
      if band(i, 2^j) ~= 0 then v = bor(v, bits[j+1]) end
    end
    table.insert(states, v)
  end

  for _,state in ipairs(states) do
    local key_ref, name_ref = F.KEY_NUMPAD0, "Num0"
    if band(state, F.ENHANCED_KEY) ~= 0 then
      key_ref, name_ref = F.KEY_INS, "Ins"
    end
    if band(state, F.SHIFT_PRESSED) ~= 0 then
      key_ref = bor(key_ref, F.KEY_SHIFT)
      name_ref = "Shift"..name_ref
    end
    if band(state, F.LEFT_ALT_PRESSED) ~= 0 then
      key_ref = bor(key_ref, F.KEY_ALT)
      name_ref = "Alt"..name_ref
    end
    if band(state, F.LEFT_CTRL_PRESSED) ~= 0 then
      key_ref = bor(key_ref, F.KEY_CTRL)
      name_ref = "Ctrl"..name_ref
    end
    rec.ControlKeyState = state
    for cd = 1,#codes do -- nothing depends on it (ENHANCED_KEY determines the result)
      rec.VirtualKeyCode = codes[cd]
      asrt.eq(far.InputRecordToKey(rec), key_ref)
      asrt.eq(far.InputRecordToName(rec), name_ref)
    end
  end

  -------- far.KeyToName
  asrt.eq(far.KeyToName(F.KEY_CTRL + F.KEY_NUMPAD0), "CtrlNum0")
  asrt.eq(far.KeyToName(F.KEY_CTRL + F.KEY_INS), "CtrlIns")

  -------- far.NameToKey
  asrt.eq(far.NameToKey("CtrlNum0"), F.KEY_CTRL + F.KEY_NUMPAD0)
  asrt.eq(far.NameToKey("CtrlIns"), F.KEY_CTRL + F.KEY_INS)

  -------- far.NameToInputRecord
  rec = asrt.table(far.NameToInputRecord("CtrlNum0"))
  asrt.eq(rec.EventType, F.KEY_EVENT)
  asrt.eq(rec.VirtualKeyCode, VK.NUMPAD0)
  asrt.eq(rec.ControlKeyState, F.LEFT_CTRL_PRESSED)

  -------- far.NameToInputRecord
  rec = asrt.table(far.NameToInputRecord("CtrlIns"))
  asrt.eq(rec.EventType, F.KEY_EVENT)
  asrt.eq(rec.VirtualKeyCode, VK.INSERT)
  asrt.eq(rec.ControlKeyState, F.LEFT_CTRL_PRESSED)
  --------
end


local function test_fsf_all()
  test_Cells()
  test_Clipboard()
  test_DetectCodePage() -- external
  test_group_owner_links()
  test_keys_names()
  test_misc()
  test_path_funcs()
  test_ProcessName()
  test_text_funcs()
end


return {
  test_fsf_all = test_fsf_all;
}
