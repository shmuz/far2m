-- test Far Standard Functions

local asrt = require "far2.assert"


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


local function test_text_funcs()
  asrt.eq(far.ConvertPath([[/foo/bar/../../abc]], "CPM_FULL"), [[/abc]])

  asrt.eq(far.FormatFileSize(123456, 8), "  123456")
  asrt.eq(far.FormatFileSize(123456, -8), "123456  ")

  asrt.str (far.GetCurrentDirectory())

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

  assert(far.LStricmp("abc","def") < 0)
  assert(far.LStricmp("def","abc") > 0)
  assert(far.LStricmp("abc","abc") == 0)
  assert(far.LStricmp("ABC","def") < 0)
  assert(far.LStricmp("DEF","abc") > 0)
  assert(far.LStricmp("ABC","abc") == 0)

  assert(far.LStrnicmp("abc","def",3) < 0)
  assert(far.LStrnicmp("def","abc",3) > 0)
  assert(far.LStrnicmp("abc","abc",3) == 0)
  assert(far.LStrnicmp("ABC","def",3) < 0)
  assert(far.LStrnicmp("DEF","abc",3) > 0)
  assert(far.LStrnicmp("ABC","abc",3) == 0)
  assert(far.LStrnicmp("111abc","111def",3) == 0)
  assert(far.LStrnicmp("111abc","111def",4) < 0)

  assert(far.LStrcmp("abc","def") < 0)
  assert(far.LStrcmp("def","abc") > 0)
  assert(far.LStrcmp("abc","abc") == 0)
  assert(far.LStrcmp("ABC","def") < 0)
  assert(far.LStrcmp("DEF","abc") < 0)
  assert(far.LStrcmp("ABC","abc") < 0)

  assert(far.LStrncmp("abc","def",3) < 0)
  assert(far.LStrncmp("def","abc",3) > 0)
  assert(far.LStrncmp("abc","abc",3) == 0)
  assert(far.LStrncmp("ABC","def",3) < 0)
  assert(far.LStrncmp("DEF","abc",3) < 0)
  assert(far.LStrncmp("ABC","abc",3) < 0)
  assert(far.LStrncmp("111abc","111def",3) == 0)
  assert(far.LStrncmp("111abc","111def",4) < 0)
end


local function test_Cells()
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


local function test_fsf_all()
  test_Cells()
  test_Clipboard()
  test_ProcessName()
  test_text_funcs()
end


return {
  test_fsf_all = test_fsf_all;
}
