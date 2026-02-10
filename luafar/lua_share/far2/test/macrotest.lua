-- encoding: utf-8
-- Started: 2012-08-20.

--[[
-- The following macro can be used to run all the tests.

Macro {
  description="Macro-engine test";
  area="Shell"; key="CtrlShiftF12";
  action = function()
    Far.DisableHistory(0x0F)
    local mod = require "far2.test.macrotest"
    mod.test_all()
    far.Message("All tests OK", "LuaMacro")
  end;
}
--]]

-- The keys that invoke the whole macrotest from a macro. Some tests depend on that.
local MacroKey1, MacroKey2 = "CtrlShiftF12", "RCtrlShiftF12"

local asrt = require "far2.assert"
local LF   = require "far2.test.test_luafar"
local TE   = require "far2.test.test_editor"
local FSF  = require "far2.test.test_fsf"
local TP   = require "far2.test.test_panel"

local MT = {} -- "macrotest", this module
local F = far.Flags
local band = bit64.band
local luamacroId = far.GetPluginId()
local hlfviewerId = 0x1AF0754D
local TKEY_BINARY = "__binary"

local function pack (...)
  return { n=select("#",...), ... }
end

local TmpFileName = far.InMyTemp("macrotest.tmp")
local TmpDir = far.InMyTemp("macrotest")

local function WriteTmpFile(...)
  local fp = assert(io.open(TmpFileName,"w"))
  fp:write(...)
  fp:close()
end

local function DeleteTmpFile()
  win.DeleteFile(TmpFileName)
end

local function TestArea (area, k_before, k_after)
  if k_before then Keys(k_before) end
  assert(Area[area]==true and Area.Current==area)
  if k_after then Keys(k_after) end
end

function MT.test_areas()
  TestArea ("Shell")
  TestArea ("Grabber",    "AltIns",     "Esc")
  TestArea ("Shell",      "F12 0")
  TestArea ("Editor",     "ShiftF4 CtrlY Enter", "Esc")
  TestArea ("Dialog",     "F7",         "Esc")
  TestArea ("Search",     "Alt?",       "Esc")
  TestArea ("Disks",      "AltF1",      "Esc")
  TestArea ("Disks",      "AltF2",      "Esc")
  TestArea ("MainMenu",   "F9",         "Esc")
  TestArea ("MainMenu",   "F9 Enter",   "Esc Esc")
  TestArea ("Menu",       "F12",        "Esc")
  TestArea ("Help",       "F1",         "Esc")
  TestArea ("Info",       "CtrlL Tab",  "Tab CtrlL")
  TestArea ("QView",      "CtrlQ Tab",  "Tab CtrlQ")
  TestArea ("Tree",       "CtrlT Tab",  "Tab CtrlT")
  TestArea ("FindFolder", "AltF10",     "Esc")
  TestArea ("UserMenu",   "F2",         "Esc")

  TestArea("Shell")
  asrt.isfalse (Area.Other)
  asrt.isfalse (Area.Viewer)
  asrt.isfalse (Area.Editor)
  asrt.isfalse (Area.Dialog)
  asrt.isfalse (Area.Search)
  asrt.isfalse (Area.Disks)
  asrt.isfalse (Area.MainMenu)
  asrt.isfalse (Area.Menu)
  asrt.isfalse (Area.Help)
  asrt.isfalse (Area.Info)
  asrt.isfalse (Area.QView)
  asrt.isfalse (Area.Tree)
  asrt.isfalse (Area.FindFolder)
  asrt.isfalse (Area.UserMenu)
  asrt.isfalse (Area.ShellAutoCompletion)
  asrt.isfalse (Area.DialogAutoCompletion)
  asrt.isfalse (Area.Grabber)
end

local function test_mf_akey()
  asrt.eq(akey, mf.akey)
  local key,name = akey(0),akey(1)
  assert(key==far.NameToKey(MacroKey1) and name==MacroKey1 or
         key==far.NameToKey(MacroKey2) and name==MacroKey2)
  -- (the 2nd parameter is tested in function test_mf_eval).
end

local function test_mf_eval()
  asrt.eq(eval, mf.eval)

  -- test arguments validity checking
  asrt.eq (eval(""), 0)
  asrt.eq (eval("", 0), 0)
  asrt.eq (eval(), -1)
  asrt.eq (eval(0), -1)
  asrt.eq (eval(true), -1)
  asrt.eq (eval("", -1), -1)
  asrt.eq (eval("", 5), -1)
  asrt.eq (eval("", true), -1)
  asrt.eq (eval("", 1, true), -1)
  asrt.eq (eval("",1,"javascript"), -1)

  -- test macro-not-found error
  asrt.eq (eval("", 2), -2)

  -- We will modify the global 'temp'. Let it be restored when the macro terminates.
  -- luacheck: globals temp
  mf.AddExitHandler(function(v) temp=v; end, temp)
  temp=3
  asrt.eq (eval("temp=5+7"), 0)
  asrt.eq (temp, 12)

  temp=3
  asrt.eq (eval("temp=5+7",0,"moonscript"), 0)
  asrt.eq (eval("temp=5+7",1,"lua"), 0)
  asrt.eq (eval("temp=5+7",3,"lua"), "")
  asrt.eq (eval("temp=5+7",1,"moonscript"), 0)
  asrt.eq (eval("temp=5+7",3,"moonscript"), "")
  asrt.eq (temp, 3)
  asrt.eq (eval("getfenv(1).temp=12",0,"moonscript"), 0)
  asrt.eq (temp, 12)

  asrt.eq (eval("5",0,"moonscript"), 0)
  asrt.eq (eval("5+7",1,"lua"), 11)
  asrt.eq (eval("5+7",1,"moonscript"), 0)
  asrt.eq (eval("5 7",1,"moonscript"), 11)

  -- test with Mode==2
  local code = ([[
    local key = akey(1,0)
    assert(key=="%s" or key=="%s")
    assert(akey(1,1)=="CtrlA")
    foobar = (foobar or 0) + 1
    return foobar,false,5,nil,"foo"
  ]]):format(MacroKey1, MacroKey2)
  local Id = asrt.udata(far.MacroAdd(nil,nil,"CtrlA",code))
  for k=1,3 do
    local ret1,a,b,c,d,e = eval("CtrlA",2)
    asrt.istrue(ret1==0 and a==k and b==false and c==5 and d==nil and e=="foo")
  end
  asrt.istrue(far.MacroDelete(Id))
end

local function test_mf_abs()
  asrt.eq (mf.abs(1.3), 1.3)
  asrt.eq (mf.abs(-1.3), 1.3)
  asrt.eq (mf.abs(0), 0)
end

local function test_mf_acall()
  local a,b,c,d = mf.acall(function(p) return 3, nil, p, "foo" end, 77)
  asrt.istrue (a==3 and b==nil and c==77 and d=="foo")
  asrt.istrue (mf.acall(far.Show))
  TestArea("Menu",nil,"Esc")
end

local function test_mf_asc()
  asrt.eq (mf.asc("0"), 48)
  asrt.eq (mf.asc("Я"), 1071)
end

local function test_mf_atoi()
  local function check(str, base)
    asrt.eq(mf.atoi(str,base), tonumber(str,base))
  end

  for _,v in ipairs { "0", "-10", "0x11" } do check(v) end

  check("1011", 2)
  check("1234", 5)
  asrt.eq(mf.atoi(-1234, 5), -194)

  for _,v in ipairs { "123456789123456789", "-123456789123456789",
                      "0x1B69B4BACD05F15", "-0x1B69B4BACD05F15" } do
    asrt.eq(mf.atoi(v), bit64.new(v))
  end
end

local function test_mf_chr()
  asrt.eq (mf.chr(48), "0")
  asrt.eq (mf.chr(1071), "Я")
end

local function test_mf_clip()
  local oldval = far.PasteFromClipboard() -- store

  mf.clip(5,2) -- turn on the internal clipboard
  asrt.eq (mf.clip(5,-1), 2)
  asrt.eq (mf.clip(5,1),  2) -- turn on the OS clipboard
  asrt.eq (mf.clip(5,-1), 1)

  for clipnum=1,2 do
    mf.clip(5,clipnum)
    local str = "foo"..clipnum
    assert(mf.clip(1,str) > 0)
    assert(mf.clip(0) == str)
    assert(mf.clip(2,"bar") > 0)
    assert(mf.clip(0) == str.."bar")
  end

  mf.clip(5,1); mf.clip(1,"foo")
  mf.clip(5,2); mf.clip(1,"bar")
  asrt.eq (mf.clip(0), "bar")
  mf.clip(5,1); asrt.eq (mf.clip(0), "foo")
  mf.clip(5,2); asrt.eq (mf.clip(0), "bar")

  mf.clip(3);   asrt.eq (mf.clip(0), "foo")
  mf.clip(5,1); asrt.eq (mf.clip(0), "foo")

  mf.clip(5,2); mf.clip(1,"bar")
  mf.clip(5,1); asrt.eq (mf.clip(0), "foo")
  mf.clip(4);   asrt.eq (mf.clip(0), "bar")
  mf.clip(5,2); asrt.eq (mf.clip(0), "bar")

  mf.clip(5,1) -- leave the OS clipboard active in the end
  far.CopyToClipboard(oldval or "") -- restore
end

local function test_mf_env()
  mf.env("Foo",1,"Bar")
  asrt.eq (mf.env("Foo"), "Bar")
  mf.env("Foo",1,"")
  asrt.eq (mf.env("Foo"), "")
  mf.env("Foo",1,nil)
  asrt.isnil(win.GetEnv("Foo"))
end

local function test_mf_fattr()
  DeleteTmpFile()
  asrt.eq (mf.fattr(TmpFileName), -1)
  WriteTmpFile("")
  local attr = mf.fattr(TmpFileName)
  DeleteTmpFile()
  assert(attr >= 0)
end

local function test_mf_fexist()
  WriteTmpFile("")
  asrt.istrue(mf.fexist(TmpFileName))
  DeleteTmpFile()
  asrt.isfalse(mf.fexist(TmpFileName))
end

local function test_mf_msgbox()
  asrt.eq (msgbox, mf.msgbox)
  mf.postmacro(Keys, "Esc")
  asrt.eq (0, msgbox("title","message"))
  mf.postmacro(Keys, "Enter")
  asrt.eq (1, msgbox("title","message"))
end

local function test_mf_prompt()
  asrt.eq (prompt, mf.prompt)
  mf.postmacro(Keys, "a b c Esc")
  asrt.isfalse (prompt())
  mf.postmacro(Keys, "a b c Enter")
  asrt.eq ("abc", prompt())
end

local function test_mf_date()
  asrt.str (mf.date())
  asrt.str (mf.date("%a"))
end

local function test_mf_fmatch()
  asrt.eq (mf.fmatch("Readme.txt", "*.txt"), 1)
  asrt.eq (mf.fmatch("Readme.txt", "Readme.*|*.txt"), 0)
  asrt.eq (mf.fmatch("/dir/Readme.txt", "/txt$/i"), 1)
  asrt.eq (mf.fmatch("/dir/Readme.txt", "/txt$"), -1)
end

local function test_mf_fsplit()
  local bRoot = 0x01
  local bPath = 0x02
  local bName = 0x04
  local bExt  = 0x08

  local path = "C:/Program Files/Far/Far.exe"
  asrt.eq (mf.fsplit(path, bRoot), "C:/")
  asrt.eq (mf.fsplit(path, bPath), "/Program Files/Far/")
  asrt.eq (mf.fsplit(path, bName), "Far")
  asrt.eq (mf.fsplit(path, bExt),  ".exe")

  asrt.eq (mf.fsplit(path, bRoot + bPath), "C:/Program Files/Far/")
  asrt.eq (mf.fsplit(path, bName + bExt),  "Far.exe")
  asrt.eq (mf.fsplit(path, bRoot + bPath + bName + bExt), path)
end

local function test_mf_iif()
  asrt.eq (mf.iif(true,  1, 2), 1)
  asrt.eq (mf.iif("a",   1, 2), 1)
  asrt.eq (mf.iif(100,   1, 2), 1)
  asrt.eq (mf.iif(false, 1, 2), 2)
  asrt.eq (mf.iif(nil,   1, 2), 2)
  asrt.eq (mf.iif(0,     1, 2), 2)
  asrt.eq (mf.iif("",    1, 2), 2)
end

local function test_mf_index()
  asrt.eq (mf.index("language","gua",0), 3)
  asrt.eq (mf.index("language","gua",1), 3)
  asrt.eq (mf.index("language","gUA",1), -1)
  asrt.eq (mf.index("language","gUA",0), 3)
end

local function test_mf_int()
  asrt.eq (mf.int("2.99"), 2)
  asrt.eq (mf.int("-2.99"), -2)
  asrt.eq (mf.int("0x10"), 0)
  for _,v in ipairs { "123456789123456789", "-123456789123456789" } do
    asrt.eq(mf.int(v), bit64.new(v))
  end
end

local function test_mf_itoa()
  asrt.eq (mf.itoa(100),    "100")
  asrt.eq (mf.itoa(100,10), "100")
  asrt.eq (mf.itoa(100,2),  "1100100")
  asrt.eq (mf.itoa(100,16), "64")
  asrt.eq (mf.itoa(100,36), "2s")
  for _,v in ipairs { "123456789123456789", "-123456789123456789" } do
    asrt.eq(mf.itoa(bit64.new(v)), v)
  end
end

local function test_mf_key()
  local ref = {
    F.KEY_CTRL,         "Ctrl",
    F.KEY_ALT,          "Alt",
    F.KEY_SHIFT,        "Shift",
    F.KEY_RCTRL,        "RCtrl",
    F.KEY_RALT,         "RAlt",
    F.KEY_CTRLALT + F.KEY_F10, "CtrlAltF10",
  }
  for i=1,#ref,2 do
    asrt.eq (mf.key        (ref[i]), ref[i+1])
    asrt.eq (far.KeyToName (ref[i]), ref[i+1])
  end
  asrt.eq (mf.key("AltShiftF11"), "AltShiftF11")
  asrt.eq (mf.key("foobar"), "")
end

-- Separate tests for mf.float and mf.string are locale-dependent, thus they are tested together.
local function test_mf_float_and_string()
  local t = { 0, -0, 2.56e1, -5.37, -2.2e100, 2.2e-100 }
  for _,num in ipairs(t) do
    asrt.eq (mf.float(mf.string(num)), num)
  end
end

local function test_mf_lcase()
  asrt.eq (mf.lcase("FOo БАр"), "foo бар")
end

local function test_mf_len()
  asrt.eq (mf.len(""), 0)
  asrt.eq (mf.len("FOo БАр"), 7)
end

local function test_mf_max()
  asrt.eq (mf.max(-2,-5), -2)
  asrt.eq (mf.max(2,5), 5)
end

local function test_mf_min()
  asrt.eq (mf.min(-2,-5), -5)
  asrt.eq (mf.min(2,5), 2)
end

local function test_mf_msave()
  local Key = "macrotest"

  -- test supported types, except tables
  local v1, v2, v3, v4, v5, v6 = nil, false, true, -5.67, "foo", bit64.new("0x1234567843218765")
  mf.msave(Key, "name1", v1)
  mf.msave(Key, "name2", v2)
  mf.msave(Key, "name3", v3)
  mf.msave(Key, "name4", v4)
  mf.msave(Key, "name5", v5)
  mf.msave(Key, "name6", v6)
  asrt.eq (mf.mload(Key, "name1"), v1)
  asrt.eq (mf.mload(Key, "name2"), v2)
  asrt.eq (mf.mload(Key, "name3"), v3)
  asrt.eq (mf.mload(Key, "name4"), v4)
  asrt.eq (mf.mload(Key, "name5"), v5)
  asrt.eq (mf.mload(Key, "name6"), v6)
  mf.mdelete(Key, "*")
  asrt.eq (mf.mload(Key, "name3"), nil)

  -- test tables
  mf.msave(Key, "name1", { a=5, {b="foo"}, c={d=false} })
  local t=mf.mload(Key, "name1")
  asrt.istrue(t.a==5 and t[1].b=="foo" and t.c.d==false)
  mf.mdelete(Key, "name1")
  asrt.isnil(mf.mload(Key, "name1"))

  -- test tables more
  local t1, t2, t3 = {5}, {6}, {}
  t1[2], t1[3], t1[4], t1[5] = t1, t2, t3, t3
  t2[2], t2[3] = t1, t2
  t1[t1], t1[t2] = 66, 77
  t2[t1], t2[t2] = 88, 99
  setmetatable(t3, { __index=t1 })
  mf.msave(Key, "name1", t1)

  local T1 = asrt.table(mf.mload(Key, "name1"))
  local T2 = asrt.table(T1[3])
  local T3 = T1[4]
  assert(type(T3)=="table" and T3==T1[5])
  assert(T1[1]==5 and T1[2]==T1 and T1[3]==T2)
  assert(T2[1]==6 and T2[2]==T1 and T2[3]==T2)
  assert(T1[T1]==66 and T1[T2]==77)
  assert(T2[T1]==88 and T2[T2]==99)
  assert(getmetatable(T3).__index==T1 and T3[1]==5 and rawget(T3,1)==nil)
  mf.mdelete(Key, "*")
  asrt.isnil(mf.mload(Key, "name1"))
end

local function test_mf_mod()
  asrt.eq (mf.mod(11,4), 3)
  asrt.eq (math.fmod(11,4), 3)
  asrt.eq (11 % 4, 3)

  asrt.eq (mf.mod(-1,4), -1)
  asrt.eq (math.fmod(-1,4), -1)
  asrt.eq (-1 % 4, 3)
end

local function test_mf_replace()
  asrt.eq (mf.replace("Foo Бар", "o", "1"), "F11 Бар")
  asrt.eq (mf.replace("Foo Бар", "o", "1", 1), "F1o Бар")
  asrt.eq (mf.replace("Foo Бар", "O", "1", 1, 1), "Foo Бар")
  asrt.eq (mf.replace("Foo Бар", "O", "1", 1, 0), "F1o Бар")
end

local function test_mf_rindex()
  asrt.eq (mf.rindex("language","a",0), 5)
  asrt.eq (mf.rindex("language","a",1), 5)
  asrt.eq (mf.rindex("language","A",1), -1)
  asrt.eq (mf.rindex("language","A",0), 5)
end

local function test_mf_strpad()
  asrt.eq (mf.strpad("Foo",10,"*",  2), '***Foo****')
  asrt.eq (mf.strpad("",   10,"-*-",2), '-*--*--*--')
  asrt.eq (mf.strpad("",   10,"-*-"), '-*--*--*--')
  asrt.eq (mf.strpad("Foo",10), 'Foo       ')
  asrt.eq (mf.strpad("Foo",10,"-"), 'Foo-------')
  asrt.eq (mf.strpad("Foo",10," ",  1), '       Foo')
  asrt.eq (mf.strpad("Foo",10," ",  2), '   Foo    ')
  asrt.eq (mf.strpad("Foo",10,"1234567890",2), '123Foo1234')
end

local function test_mf_strwrap()
  asrt.eq (mf.strwrap("Пример строки, которая будет разбита на несколько строк по ширине в 7 символов.", 7,"\n"),
[[
Пример
строки,
которая
будет
разбита
на
несколь
ко
строк
по
ширине
в 7
символо
в.]])
end

local function test_mf_substr()
  asrt.eq (mf.substr("abcdef", 1), "bcdef")
  asrt.eq (mf.substr("abcdef", 1, 3), "bcd")
  asrt.eq (mf.substr("abcdef", 0, 4), "abcd")
  asrt.eq (mf.substr("abcdef", 0, 8), "abcdef")
  asrt.eq (mf.substr("abcdef", -1), "f")
  asrt.eq (mf.substr("abcdef", -2), "ef")
  asrt.eq (mf.substr("abcdef", -3, 1), "d")
  asrt.eq (mf.substr("abcdef", 0, -1), "abcde")
  asrt.eq (mf.substr("abcdef", 2, -1), "cde")
  asrt.eq (mf.substr("abcdef", 4, -4), "")
  asrt.eq (mf.substr("abcdef", -3, -1), "de")
end

local function test_mf_testfolder()
  asrt.range (mf.testfolder("."), 1, 2)          -- exists
  asrt.eq    (mf.testfolder(far.GetMyHome()), 2) -- exists and not empty
  asrt.range (mf.testfolder("@:\\"), -1, 0)      -- doesn't exist or access denied
end

local function test_mf_trim()
  asrt.eq (mf.trim(" abc "), "abc")
  asrt.eq (mf.trim(" abc ",0), "abc")
  asrt.eq (mf.trim(" abc ",1), "abc ")
  asrt.eq (mf.trim(" abc ",2), " abc")
end

local function test_mf_ucase()
  asrt.eq (mf.ucase("FOo БАр"), "FOO БАР")
end

local function test_mf_waitkey()
  asrt.eq (mf.waitkey(50,0), "")
  asrt.eq (mf.waitkey(50,1), F.KEY_INVALID)
end

local function test_mf_size2str()
  asrt.eq (mf.size2str(123,0,5), "  123")
  asrt.eq (mf.size2str(123,0,-5), "123  ")
end

local function test_mf_xlat()
  asrt.str (mf.xlat("abc"))
  asrt.eq (mf.xlat("ghzybr"), "пряник")
  asrt.eq (mf.xlat("сщьзгеук"), "computer")
end

local function test_mf_beep()
  asrt.bool (mf.beep())
end

local function test_mf_flock()
  for k=0,2 do asrt.num (mf.flock(k,-1)) end
end

local function test_mf_GetMacroCopy()
  asrt.table (mf.GetMacroCopy(1))
  asrt.isnil   (mf.GetMacroCopy(1e9))
end

local function test_mf_Keys()
  asrt.eq (Keys, mf.Keys)
  Keys("Esc F a r Space M a n a g e r Space Ф А Р")
  asrt.eq (panel.GetCmdLine(), "Far Manager ФАР")
  Keys("Esc")
  asrt.eq (panel.GetCmdLine(), "")
  -- test invalid keys; see far2m/commit/1225dbeb2d769ba2da8570c20b0e96fd179a3442
  Keys("n1 n2 n3 n4 n5 n6 n7 n8 A B C")
  asrt.eq (panel.GetCmdLine(), "ABC")
  Keys("Esc")
end

local function test_mf_exit()
  asrt.eq (exit, mf.exit)
  local N
  mf.postmacro(
    function()
      local function f() N=50; exit(); end
      f(); N=100
    end)
  mf.postmacro(Keys, "Esc")
  far.Message("dummy")
  asrt.eq (N, 50)
end

local function test_mf_mmode()
  asrt.eq (mmode, mf.mmode)
  asrt.eq (1, mmode(1,-1))
end

local function test_mf_print()
  asrt.eq (print, mf.print)
  asrt.func (print)
  -- test on command line
  local str = "abc ABC абв АБВ"
  Keys("Esc")
  print(str)
  asrt.eq (panel.GetCmdLine(), str)
  Keys("Esc")
  asrt.eq (panel.GetCmdLine(), "")
  -- test on dialog input field
  Keys("F7 CtrlY")
  print(str)
  asrt.eq (Dlg.GetValue(-1,0), str)
  Keys("Esc")
  -- test on editor
  str = "abc ABC\nабв АБВ"
  Keys("ShiftF4")
  print(TmpFileName)
  Keys("Enter CtrlHome Enter Up")
  print(str)
  Keys("CtrlHome"); assert(Editor.Value == "abc ABC")
  Keys("Down");     assert(Editor.Value == "абв АБВ")
  editor.Quit()
end

local function test_mf_postmacro()
  asrt.func (mf.postmacro)
end

local function test_mf_sleep()
  asrt.func (mf.sleep)
end

local function test_mf_usermenu()
  mf.usermenu()
  TestArea("UserMenu",nil,"Esc")
end

local function test_mf_EnumScripts()
  local iter = asrt.func(mf.EnumScripts("Macro"))
  local Id = asrt.udata(far.MacroAdd(nil,nil,"foo","return")) -- assure at least 1 macro loaded
  local s,i = iter()
  far.MacroDelete(Id)
  asrt.table(s)
  asrt.num(i)
end

function MT.test_mf()
  test_mf_abs()
  test_mf_acall()
  test_mf_akey()
  test_mf_asc()
  test_mf_atoi()
--test_mf_beep()
  test_mf_chr()
  test_mf_clip()
  test_mf_date()
  test_mf_EnumScripts()
  test_mf_env()
  test_mf_eval()
  test_mf_exit()
  test_mf_fattr()
  test_mf_fexist()
  test_mf_float_and_string()
  test_mf_flock()
  test_mf_fmatch()
  test_mf_fsplit()
  test_mf_GetMacroCopy()
  test_mf_iif()
  test_mf_index()
  test_mf_int()
  test_mf_itoa()
  test_mf_key()
  test_mf_Keys()
  test_mf_lcase()
  test_mf_len()
  test_mf_max()
  test_mf_min()
  test_mf_mmode()
  test_mf_mod()
  test_mf_msave()
  test_mf_msgbox()
  test_mf_postmacro()
  test_mf_print()
  test_mf_prompt()
  test_mf_replace()
  test_mf_rindex()
  test_mf_size2str()
  test_mf_sleep()
  test_mf_strpad()
  test_mf_strwrap()
  test_mf_substr()
  test_mf_testfolder()
  test_mf_trim()
  test_mf_ucase()
  test_mf_usermenu()
  test_mf_waitkey()
  test_mf_xlat()
end

function MT.test_CmdLine()
  Keys"Esc f o o Space Б а р"
  asrt.isfalse(CmdLine.Bof)
  asrt.istrue(CmdLine.Eof)
  asrt.isfalse(CmdLine.Empty)
  asrt.isfalse(CmdLine.Selected)
  asrt.eq (CmdLine.Value, "foo Бар")
  asrt.eq (CmdLine.ItemCount, 7)
  asrt.eq (CmdLine.CurPos, 8)

  Keys"SelWord"
  asrt.istrue(CmdLine.Selected)

  Keys"CtrlHome"
  asrt.istrue(CmdLine.Bof)
  asrt.isfalse(CmdLine.Eof)

  Keys"Esc"
  asrt.istrue(CmdLine.Bof)
  asrt.istrue(CmdLine.Eof)
  asrt.istrue(CmdLine.Empty)
  asrt.isfalse(CmdLine.Selected)
  asrt.eq (CmdLine.Value, "")
  asrt.eq (CmdLine.ItemCount, 0)
  asrt.eq (CmdLine.CurPos, 1)

  Keys"Esc"
  print("foo Бар")
  asrt.eq (CmdLine.Value, "foo Бар")

  Keys"Esc"
  print(("%s %d %s"):format("foo", 5+7, "Бар"))
  asrt.eq (CmdLine.Value, "foo 12 Бар")

  Keys"Esc"
end

local function test_Far_GetConfig()
  local options = {
    "CmdLine.AskOnMultilinePaste",
    "Cmdline.AutoComplete",
    "Cmdline.DelRemovesBlocks",
    "Cmdline.EditBlock",
    "Cmdline.ImitateNumpadKeys",
    "Cmdline.PromptFormat",
    "Cmdline.ShellCmd",
    "Cmdline.Splitter",
    "Cmdline.UsePromptFormat",
    "Cmdline.UseShell",
    "Cmdline.VTLogLimit",
    "Cmdline.WaitKeypress",
    "CodePages.CPMenuMode",
    -- "Colors.CurrentPalette",
    -- "Colors.CurrentPaletteRGB",
    "Colors.TempColors256",
    "Colors.TempColorsRGB",
    "Confirmations.AllowReedit",
    "Confirmations.Copy",
    "Confirmations.Delete",
    "Confirmations.DeleteFolder",
    "Confirmations.Drag",
    "Confirmations.Esc",
    "Confirmations.EscTwiceToInterrupt",
    "Confirmations.Exit",
    "Confirmations.ExitOrBknd",
    "Confirmations.HistoryClear",
    "Confirmations.Move",
    "Confirmations.RemoveConnection",
    "Confirmations.RemoveHotPlug",
    "Confirmations.RO",
    "Descriptions.AnsiByDefault",
    "Descriptions.ListNames",
    "Descriptions.ROUpdate",
    "Descriptions.SaveInUTF",
    "Descriptions.SetHidden",
    "Descriptions.StartPos",
    "Descriptions.UpdateMode",
    "Dialog.AutoComplete",
    "Dialog.CBoxMaxHeight",
    "Dialog.DelRemovesBlocks",
    "Dialog.EditBlock",
    "Dialog.EditHistory",
    "Dialog.EditLine",
    "Dialog.EULBsClear",
    "Dialog.MouseButton",
    "Editor.AllowEmptySpaceAfterEof",
    "Editor.AutoDetectCodePage",
    "Editor.AutoIndent",
    "Editor.BSLikeDel",
    "Editor.CharCodeBase",
    "Editor.DefaultCodePage",
    "Editor.DelRemovesBlocks",
    "Editor.EditOpenedForWrite",
    "Editor.EditorCursorBeyondEOL",
    "Editor.ExpandTabs",
    "Editor.ExternalEditorName",
    "Editor.FileSizeLimit",
    "Editor.FileSizeLimitHi",
    "Editor.PersistentBlocks",
    "Editor.ReadOnlyLock",
    "Editor.SaveEditorPos",
    "Editor.SaveEditorShortPos",
    "Editor.SearchPickUpWord",
    "Editor.SearchRegexp",
    "Editor.SearchSelFound",
    "Editor.ShowKeyBar",
    "Editor.ShowScrollBar",
    "Editor.ShowTitleBar",
    "Editor.ShowWhiteSpace",
    "Editor.TabSize",
    "Editor.UndoDataSize",
    "Editor.UseExternalEditor",
    "Editor.WordDiv",
    "Help.ActivateURL",
    "Interface/Completion.Append",
    "Interface/Completion.Exceptions",
    "Interface/Completion.ModalList",
    "Interface/Completion.ShowList",
    "Interface.ConsolePaintSharp",
    "Interface.CopyShowTotal",
    "Interface.CtrlPgUp",
    "Interface.CursorSize1",
    "Interface.CursorSize2",
    "Interface.CursorSize3",
    "Interface.CursorSize4",
    "Interface.DateFormat",
    "Interface.DateSeparator",
    "Interface.DecimalSeparator",
    "Interface.DelShowTotal",
    "Interface.ExclusiveAltLeft",
    "Interface.ExclusiveAltRight",
    "Interface.ExclusiveCtrlLeft",
    "Interface.ExclusiveCtrlRight",
    "Interface.ExclusiveWinLeft",
    "Interface.ExclusiveWinRight",
    "Interface.FormatNumberSeparators",
    "Interface.Mouse",
    "Interface.OSC52ClipSet",
    "Interface.ShiftsKeyRules",
    "Interface.ShowMenuBar",
    "Interface.ShowTimeoutDACLFiles",
    "Interface.ShowTimeoutDelFiles",
    "Interface.TimeSeparator",
    "Interface.TTYPaletteOverride",
    "Interface.UseStickyKeyEvent",
    "Interface.UseVk_oem_x",
    "Interface.WindowTitle",
    "Language.Help",
    "Language.Main",
    "Layout.FullscreenHelp",
    "Layout.LeftHeightDecrement",
    "Layout.RightHeightDecrement",
    "Layout.WidthDecrement",
    "Macros.DateFormat",
    "Macros.KeyRecordCtrlDot",
    "Macros.KeyRecordCtrlShiftDot",
    "Macros.ShowPlayIndicator",
    "Notifications.OnConsole",
    "Notifications.OnFileOperation",
    "Notifications.OnlyIfBackground",
    "Panel.AttrStrStyle",
    "Panel.AutoUpdateLimit",
    "Panel.CaseSensitiveCompareSelect",
    "Panel.ClassicHotkeyLinkResolving",
    "Panel.CtrlAltShiftRule",
    "Panel.FilenameMarksAlign",
    "Panel.Highlight",
    "Panel/Layout.ColumnTitles",
    "Panel/Layout.FreeInfo",
    "Panel/Layout.ScreensNumber",
    "Panel/Layout.Scrollbar",
    "Panel/Layout.ScrollbarMenu",
    "Panel/Layout.SortMode",
    "Panel/Layout.StatusLine",
    "Panel/Layout.TotalInfo",
    "Panel/Left.CaseSensitiveSort",
    "Panel/Left.CurFile",
    "Panel/Left.DirectoriesFirst",
    "Panel/Left.Focus",
    "Panel/Left.Folder",
    "Panel/Left.NumericSort",
    "Panel/Left.SelectedFirst",
    "Panel/Left.SortGroups",
    "Panel/Left.SortMode",
    "Panel/Left.SortOrder",
    "Panel/Left.Type",
    "Panel/Left.ViewMode",
    "Panel/Left.Visible",
    "Panel.MaxFilenameIndentation",
    "Panel.MinFilenameIndentation",
    "Panel.RememberLogicalDrives",
    "Panel.ReverseSort",
    "Panel/Right.CaseSensitiveSort",
    "Panel.RightClickRule",
    "Panel/Right.CurFile",
    "Panel/Right.DirectoriesFirst",
    "Panel/Right.Focus",
    "Panel/Right.Folder",
    "Panel/Right.NumericSort",
    "Panel/Right.SelectedFirst",
    "Panel/Right.SortGroups",
    "Panel/Right.SortMode",
    "Panel/Right.SortOrder",
    "Panel/Right.Type",
    "Panel/Right.ViewMode",
    "Panel/Right.Visible",
    "Panel.SelectFolders",
    "Panel.ShellRightLeftArrowsRule",
    "Panel.ShowFilenameMarks",
    "Panel.ShowHidden",
    "Panel.SortFolderExt",
    "Panel/Tree.AutoChangeFolder",
    "Panel/Tree.ExclSubTreeMask",
    "Panel/Tree.LocalDisk",
    "Panel/Tree.MinTreeCount",
    "Panel/Tree.NetDisk",
    "Panel/Tree.NetPath",
    "Panel/Tree.RemovableDisk",
    "Panel/Tree.TreeFileAttr",
    "PluginConfirmations.EvenIfOnlyOnePlugin",
    "PluginConfirmations.OpenFilePlugin",
    "PluginConfirmations.Prefix",
    "PluginConfirmations.SetFindList",
    "PluginConfirmations.StandardAssociation",
    "SavedDialogHistory.HistoryCount",
    "SavedFolderHistory.HistoryCount",
    "SavedHistory.HistoryCount",
    "SavedViewHistory.HistoryCount",
    "Screen.Clock",
    "Screen.CursorBlinkInterval",
    "Screen.KeyBar",
    "Screen.ScreenSaver",
    "Screen.ScreenSaverTime",
    "Screen.ViewerEditorClock",
    "System.AllCtrlAltShiftRule",
    "System.AutoSaveSetup",
    "System.AutoUpdateRemoteDrive",
    "System.CASRule",
    "System.CmdHistoryRule",
    "System.CollectFiles",
    "System.ConsoleDetachKey",
    "System.CopyAccessMode",
    "System.CopyTimeRule",
    "System.CopyXAttr",
    "System.DeleteToRecycleBin",
    "System.DeleteToRecycleBinKillLink",
    "System.DriveColumn2",
    "System.DriveColumn3",
    "System.DriveDisconnectMode",
    "System.DriveExceptions",
    "System.DriveMenuMode2",
    "System.ExcludeCmdHistory",
    "System.FileSearchMode",
    "System.FindAlternateStreams",
    "System.FindCaseSensitiveFileMask",
    "System.FindCodePage",
    "System.FindFolders",
    "System.FindSymLinks",
    "System.FolderInfo",
    "System.HistoryShowDates",
    "System.HowCopySymlink",
    "System.InactivityExit",
    "System.InactivityExitTime",
    "System.MakeLinkSuggestSymlinkAlways",
    "System.MaxPositionCache",
    "System.MsHWheelDelta",
    "System.MsHWheelDeltaEdit",
    "System.MsHWheelDeltaView",
    "System.MsWheelDelta",
    "System.MsWheelDeltaEdit",
    "System.MsWheelDeltaHelp",
    "System.MsWheelDeltaView",
    "System.MultiCopy",
    "System.MultiMakeDir",
    "System.OnlyFilesSize",
    "System.PersonalPluginsPath",
    "System.PluginMaxReadData",
    "System.QuotedName",
    "System.QuotedSymbols",
    "System.SaveFoldersHistory",
    "System.SaveHistory",
    "System.SavePluginFoldersHistory",
    "System.SaveViewHistory",
    "System.ScanJunction",
    "System.ScanSymlinks",
    "System.SearchInFirstSize",
    "System.SearchOutFormat",
    "System.SearchOutFormatWidth",
    "System.SetAttrFolderRules",
    "System.ShowCheckingFile",
    "System.SilentLoadPlugin",
    "System.SparseFiles",
    "System.SubstNameRule",
    "System.SudoConfirmModify",
    "System.SudoEnabled",
    "System.SudoPasswordExpiration",
    "System.UseCOW",
    "System.UseFilterInSearch",
    "System.UsePrintManager",
    "System.WindowMode",
    "System.WipeSymbol",
    "System.WriteThrough",
    "Viewer.AutoDetectCodePage",
    "Viewer.DefaultCodePage",
    "Viewer.ExternalViewerName",
    "Viewer.IsWrap",
    "Viewer.PersistentBlocks",
    "Viewer.SaveViewerPos",
    "Viewer.SaveViewerShortPos",
    "Viewer.SearchRegexp",
    "Viewer.ShowArrows",
    "Viewer.ShowKeyBar",
    "Viewer.ShowScrollbar",
    "Viewer.ShowTitleBar",
    "Viewer.TabSize",
    "Viewer.UseExternalViewer",
    "Viewer.Wrap",
    "VMenu.LBtnClick",
    "VMenu.MBtnClick",
    "VMenu.MenuLoopScroll",
    "VMenu.RBtnClick",
    "XLat.EnableForDialogs",
    "XLat.EnableForFastFileFind",
    "XLat.Flags",
    "XLat.WordDivForXlat",
    "XLat.XLat",
  }

  asrt.eq(Far.GetConfig("#"), #options)
  asrt.err(Far.GetConfig, 0)
  asrt.err(Far.GetConfig, #options+1)
  asrt.err(Far.GetConfig, "foo.bar")

  for _,opt in ipairs(options) do
    local val,tp,val0,key,name,saved = Far.GetConfig(opt) -- an error is thrown if this function fails
    asrt.str(tp)
    asrt.str(key)
    asrt.str(name)
    asrt.bool(saved)

    if tp == "integer" then
      asrt.num(val)
      asrt.num(val0)
    elseif tp == "boolean" then
      asrt.bool(val)
      asrt.bool(val0)
    elseif tp == "3-state" then
      assert(val==false or val==true or val==2)
      assert(val0==false or val0==true or val0==2)
    elseif tp == "string" then
      asrt.str(val)
      asrt.str(val0)
    elseif tp == "binary" then
      asrt.table(val)
      asrt.str(val[TKEY_BINARY])
    else
      error("unknown type: "..tp)
    end
  end
end

function MT.test_Far()
  asrt.isfalse (Far.FullScreen)
  asrt.bool (Far.IsUserAdmin)
  asrt.num (Far.PID)
  asrt.str (Far.Title)

  local R = asrt.table(actl.GetFarRect())
  asrt.eq (Far.Height, R.Bottom - R.Top + 1)
  asrt.eq (Far.Width, R.Right - R.Left + 1)

  local temp = Far.UpTime
  mf.sleep(50)
  temp = Far.UpTime - temp
  asrt.range(temp, 40, 400)

  local val,typ,val0,key,name,saved = Far.GetConfig("EDITOR.defaultcodepage")
  asrt.num  (val)
  asrt.eq   (typ, "integer")
  asrt.num  (val0)
  asrt.eq   (key, "Editor")
  asrt.eq   (name, "DefaultCodePage")
  asrt.istrue (saved)

  asrt.func (Far.DisableHistory)
  asrt.num (Far.KbdLayout(0))
  asrt.num (Far.KeyBar_Show(0))
  asrt.func (Far.Window_Scroll)

  test_Far_GetConfig()
end

local function test_CheckAndGetHotKey()
  mf.acall(far.Menu, {Flags="FMENU_AUTOHIGHLIGHT"},
    {{text="abcd"},{text="abc&d"},{text="abcd"},{text="abcd"},{text="abcd"}})

  asrt.eq (Object.CheckHotkey("a"), 1)
  asrt.eq (Object.GetHotkey(1), "a")
  asrt.eq (Object.GetHotkey(), "a")
  asrt.eq (Object.GetHotkey(0), "a")

  asrt.eq (Object.CheckHotkey("b"), 3)
  asrt.eq (Object.GetHotkey(3), "b")

  asrt.eq (Object.CheckHotkey("c"), 4)
  asrt.eq (Object.GetHotkey(4), "c")

  asrt.eq (Object.CheckHotkey("d"), 2)
  asrt.eq (Object.GetHotkey(2), "d")

  asrt.eq (Object.CheckHotkey("e"), 0)

  asrt.eq (Object.CheckHotkey(""), 5)
  asrt.eq (Object.GetHotkey(5), "")
  asrt.eq (Object.GetHotkey(6), "")

  Keys("Esc")
end

function MT.test_Menu()
  Keys("F11")
  asrt.str(Menu.Value)
  asrt.eq (Menu.Value, Menu.GetValue())

  asrt.eq(Menu.Id, far.Guids.PluginsMenuId)
  asrt.eq(Menu.Id, "937F0B1C-7690-4F85-8469-AA935517F202")

  asrt.num(Menu.ItemStatus())

  asrt.eq(0, Menu.Filter(0))     -- get status
  asrt.eq(0, Menu.Filter(0,-1))  -- get status
  asrt.eq(1, Menu.Filter(0, 1))  -- turn filter on
  asrt.eq(1, Menu.Filter(0))     -- get status
  asrt.eq(1, Menu.Filter(0, 0))  -- turn filter off
  asrt.eq(0, Menu.Filter(0))     -- get status

  asrt.func(Menu.FilterStr)
  asrt.func(Menu.Select)
  asrt.func(Menu.Show)

  Keys("Esc")
end

function MT.test_Object()
  asrt.bool (Object.Bof)
  asrt.num (Object.CurPos)
  asrt.bool (Object.Empty)
  asrt.bool (Object.Eof)
  asrt.num (Object.Height)
  asrt.num (Object.ItemCount)
  asrt.bool (Object.Selected)
  asrt.str (Object.Title)
  asrt.num (Object.Width)

  test_CheckAndGetHotKey()
end

function MT.test_Drv()
  asrt.eq (Drv.ShowPos, 0) -- "Location" is not open
  Keys"AltF1"
  asrt.num (Drv.ShowMode)
  asrt.eq (Drv.ShowPos, 1) -- "Location" is open on the left
  Keys"Esc AltF2"
  asrt.num (Drv.ShowMode)
  asrt.eq (Drv.ShowPos, 2) -- "Location" is open on the right
  Keys"Esc"
  asrt.eq (Drv.ShowPos, 0) -- "Location" is not open
end

function MT.test_Help()
  Keys"F1"
  asrt.str (Help.FileName)
  asrt.str (Help.SelTopic)
  asrt.str (Help.Topic)
  Keys"Esc"
end

function MT.test_Mouse()
  asrt.num (Mouse.X)
  asrt.num (Mouse.Y)
  asrt.num (Mouse.Button)
  asrt.num (Mouse.CtrlState)
  asrt.num (Mouse.EventFlags)
  asrt.num (Mouse.LastCtrlState)
end

function MT.test_Dlg()
  Keys"F7 a b c"
  asrt.istrue(Area.Dialog)
  asrt.str(Dlg.Id)
  asrt.eq(Dlg.Id, far.Guids.MakeFolderId)
  asrt.eq(Dlg.Owner, 0)
  assert(Dlg.ItemCount > 6)
  asrt.eq(Dlg.ItemType, 4)
  asrt.eq(Dlg.CurPos, 3)
  asrt.eq(Dlg.PrevPos, 0)

  Keys"Tab"
  local pos = Dlg.CurPos
  assert(Dlg.CurPos > 3)
  asrt.eq(Dlg.PrevPos, 3)
  asrt.eq(pos, Dlg.SetFocus(3))
  asrt.eq(pos, Dlg.PrevPos)

  asrt.eq(Dlg.GetValue(0,0), Dlg.ItemCount)
  Keys"Esc"

  Keys"F10"
  asrt.istrue(Area.Dialog)
  asrt.str(Dlg.Id)
  asrt.eq(Dlg.Id, far.Guids.FarAskQuitId)
  Keys"Esc"
end

function MT.test_Plugin()
  -- Plugin.Menu
  asrt.isfalse(Plugin.Menu())
  asrt.istrue(Plugin.Menu(luamacroId, "EF6D67A2-59F7-4DF3-952E-F9049877B492")) -- call macrobrowser
  asrt.istrue(Area.Menu)
  asrt.eq(Menu.Id, "03DEFB28-8734-4EC0-8B25-C879846F0BE5")
  Keys("Esc")
  asrt.istrue(Area.Shell)

  -- Plugin.Config
  asrt.isfalse(Plugin.Config())
  if Plugin.Exist(hlfviewerId) then
    asrt.istrue(Plugin.Config(hlfviewerId))
    TestArea("Dialog", nil, "Esc")
    asrt.istrue(Area.Shell)
  end

  -- Plugin.Command
  asrt.isfalse(Plugin.Command())
  asrt.istrue(Plugin.Command(luamacroId))
  asrt.istrue(Plugin.Command(luamacroId, "view:$FARHOME/FarEng.lng"))
  TestArea("Viewer", nil, "Esc")
  asrt.istrue(Area.Shell)

  -- Plugin.Exist
  asrt.istrue(Plugin.Exist(luamacroId))
  asrt.isfalse(Plugin.Exist(luamacroId+1))

  -- Plugin.Call, Plugin.SyncCall
  local function test (func, N) -- test arguments and return
    local i1 = bit64.new("0x8765876587658765")
    local a1,a2,a3,a4,a5 = "foo", i1, -2.34, false, {[TKEY_BINARY]="foo\0bar"}
    local r1,r2,r3,r4,r5 = func(luamacroId, "argtest", a1,a2,a3,a4,a5)
    assert(r1==a1 and r2==a2 and r3==a3 and r4==a4
      and type(r5)=="table" and r5[TKEY_BINARY]==a5[TKEY_BINARY])

    local src = {}
    for k=1,N do src[k]=k end
    local trg = { func(luamacroId, "argtest", unpack(src)) }
    assert(#trg==N and trg[1]==1 and trg[N]==N)
  end
  test(Plugin.Call, 8000-8)
  test(Plugin.SyncCall, 8000-8)
end

-- Test in particular that Plugin.Call (a so-called "restricted" function) works properly
-- from inside a deeply nested coroutine.
function MT.test_coroutine()
  for k=1,2 do
    local Call = k==1 and Plugin.Call or Plugin.SyncCall
    local function f1()
      coroutine.yield(Call(luamacroId, "argtest", 1, false, "foo", nil))
    end
    local function f2() return coroutine.resume(coroutine.create(f1)) end
    local function f3() return coroutine.resume(coroutine.create(f2)) end
    local function f4() return coroutine.resume(coroutine.create(f3)) end
    local t = pack(f4())
    assert(t.n==7 and t[1]==true and t[2]==true and t[3]==true and
           t[4]==1 and t[5]==false and t[6]=="foo" and t[7]==nil)
  end
end

function MT.test_UserDefinedList()
  local ADDASTERISK      = 0x001
  local PACKASTERISKS    = 0x002
  local PROCESSBRACKETS  = 0x004
  local UNIQUE           = 0x010
  local SORT             = 0x020
  local NOTTRIM          = 0x040
  local ACCOUNTEMPTYLINE = 0x100
  local CASESENSITIVE    = 0x200

  local cases = {
    {0,                    "",                         false},
    {0,                    ",abc;",                    false},
    {ACCOUNTEMPTYLINE,     ",abc;",                    "abc"},
    ----------------------------------------------------------------------------
    {0,                    "abc",                      "abc"},
    {PACKASTERISKS,        "***abc***",                "*abc*"},
    ----------------------------------------------------------------------------
    {0,                    [[ab""c]],                  [[ab""c]]},        --double quotes inside mask
    {0,                    [["abc"]],                  "abc"},            --removing double quotes
    {0,                    [["ab""c"]],                [[ab"c]]},         --double quote inside double quotes
    {0,                    [["abc; def,",123]],        "abc; def,","123"},--spaces and delims inside dbl quotes
    ----------------------------------------------------------------------------
    {ADDASTERISK,          "abc;def",                  "abc*","def*"}, --add asterisk to every element
    {ADDASTERISK,          "abc?",                     "abc?"},        --don't add: contains ?
    {ADDASTERISK,          "ab*c",                     "ab*c"},        --don't add: contains *
    {ADDASTERISK,          "ab.c",                     "ab.c"},        --don't add: contains .
    ----------------------------------------------------------------------------
    {0,                    "abc,def;123",              "abc","def","123"},--used both , and ; delims
    ----------------------------------------------------------------------------
    {UNIQUE,               "abc,Abc;ABc",              "ABc"},             --case insensitive
    {UNIQUE+CASESENSITIVE, "abc,Abc;ABc",              "abc","Abc","ABc"}, --case sensitive
    {UNIQUE+CASESENSITIVE, "abc,abc;abc",              "abc"},
    ----------------------------------------------------------------------------
    {0,                    "789,456,123",              "789","456","123"}, --as is
    {SORT,                 "789,456,123",              "123","456","789"}, --sorted
    ----------------------------------------------------------------------------
    {0,                    "[a,z;t]",                  "[a","z","t]"},
    {PROCESSBRACKETS,      "[a,z;t]",                  "[a,z;t]"},
    ----------------------------------------------------------------------------
    {0,                    "  ab  cd  ",               "ab  cd"},
    {NOTTRIM,              "  ab  cd  ",               "  ab  cd  "},
    ----------------------------------------------------------------------------
  }

  for cnt,tt in ipairs(cases) do
    local ref = { unpack(tt,3) }
    local out = mf.udlsplit(tt[2],nil,tt[1]) or { false }
    assert(#out == #ref)
    for i=1,#ref do
      if out[i]~=ref[i] then
        error(("test %d, input: '%s'"):format(cnt,tt[2]))
      end
    end
  end
end

-- test F3,F4,F8 operations when the panels are hidden
function MT.test_F3_F4_F8()
  local farhome = asrt.str(os.getenv("FARHOME"))
  asrt.istrue(panel.SetPanelDirectory(nil,1,farhome))

  local API = panel.GetPanelInfo(nil,1)
  asrt.neq(0, band(API.Flags, F.PFLAGS_VISIBLE))

  Keys("CtrlO")
  API = panel.GetPanelInfo(nil,1)
  asrt.eq(0, band(API.Flags, F.PFLAGS_VISIBLE))
  assert(API.ItemsNumber >= 16)

  local R = asrt.table(actl.GetFarRect())
  local H = R.Bottom - R.Top + 1
  assert(H >= 8, "the FAR height is too small")

  Keys("F8 F4")
  asrt.istrue(Area.Editor)
  local EI = editor.GetInfo()
  asrt.eq(EI.TotalLines, 1)
  Keys("Esc")

  Keys("F8 F3")
  asrt.istrue(Area.Viewer)
  local VI = viewer.GetInfo()
  asrt.eq(VI.FileSize, 0)
  Keys("Esc")

  -- print not less than 5 screens of text
  local NScreens = 5
  Keys("CtrlY")
  for _=1, math.ceil(NScreens * H / API.ItemsNumber) do
    print("ls -l")
    Keys("Enter")
  end

  Keys("F4")
  asrt.istrue(Area.Editor)
  EI = editor.GetInfo()
  assert(EI.TotalLines > NScreens * H)
  Keys("Esc")

  Keys("F3")
  asrt.istrue(Area.Viewer)
  local prevpos = 1E6
  for _=1,NScreens do
    local VI = viewer.GetInfo()
    assert(VI.FilePos < prevpos)
    prevpos = VI.FilePos
    Keys("PgUp")
  end
  Keys("Esc")

  Keys("F8 F3")
  asrt.istrue(Area.Viewer)
  VI = viewer.GetInfo()
  asrt.eq(VI.FileSize, 0)
  Keys("Esc")

  Keys("CtrlO")
end

function MT.test_Delete_Wipe()
  local fname1, fname2 = "file-1", "file-2"
  os.execute("mkdir "..TmpDir)
  os.execute("rm -f " ..TmpDir.. "/*")
  asrt.istrue(panel.SetPanelDirectory(nil,1,TmpDir))
  asrt.eq(APanel.ItemCount, 1)
  io.open(fname1, "w"):close()
  io.open(fname2, "w"):close()
  asrt.istrue(panel.UpdatePanel(nil,1))
  asrt.eq(APanel.ItemCount, 3)

  asrt.neq(0, Panel.SetPos(0, fname1))
  Keys("F8")
  asrt.istrue(Area.Dialog)
  Keys("Enter")
  asrt.istrue(Area.Shell)
  asrt.eq(APanel.ItemCount, 2)

  asrt.neq(0, Panel.SetPos(0, fname2))
  Keys("AltDel")
  asrt.istrue(Area.Dialog)
  Keys("Enter")
  asrt.istrue(Area.Shell)
  asrt.eq(APanel.ItemCount, 1)
end

--[[------------------------------------------------------------------------------------------------
0001722: DN_EDITCHANGE приходит лишний раз и с ложной информацией

Description:
  [ Far 2.0.1807, Far 3.0.1897 ]
  Допустим диалог состоит из единственного элемента DI_EDIT, больше элементов нет. При появлении
  диалога сразу нажмём на клавишу, допустим, W. Приходят два события DN_EDITCHANGE вместо одного,
  причём в первом из них PtrData указывает на пустую строку.

  Последующие нажатия на клавиши, вызывающие изменения текста, отрабатываются правильно, лишние
  ложные события не приходят.
--]]------------------------------------------------------------------------------------------------
function MT.test_mantis_1722()
  local check = 0
  local function DlgProc (hDlg, msg, p1, p2)
    if msg == F.DN_EDITCHANGE then
      check = check + 1
      asrt.eq(p1, 1)
    end
  end
  local Dlg = { {"DI_EDIT", 3,1,56,10, 0,0,0,0, "a"}, }
  mf.acall(far.Dialog, nil,-1,-1,60,3,"Contents",Dlg, 0, DlgProc)
  asrt.istrue(Area.Dialog)
  Keys("W 1 2 3 4 BS Esc")
  asrt.eq(check, 6)
  asrt.eq(Dlg[1][10], "W123")
end

function MT.test_all()
  mf.AddExitHandler(panel.SetPanelDirectory, nil, 1, panel.GetPanelDirectory(nil,1))

  asrt.istrue(Area.Shell, "Run these tests from the Shell area.")
  asrt.isfalse(APanel.Plugin or PPanel.Plugin, "Run these tests when neither of panels is a plugin panel.")

  MT.test_areas()
  MT.test_mf()
  MT.test_CmdLine()
  MT.test_Help()
  MT.test_Dlg()
  MT.test_Drv()
  MT.test_Far()
  MT.test_Menu()
  MT.test_Mouse()
  MT.test_Object()
  MT.test_Plugin()
  MT.test_mantis_1722()
  MT.test_coroutine()
  MT.test_UserDefinedList()
  MT.test_F3_F4_F8()
  MT.test_Delete_Wipe()

  FSF.test_fsf_all()
  LF.test_luafar_all()
  TE.test_editor_all()
  TP.test_panel_all()

  actl.RedrawAll()
end

MT.test_fsf_all    = FSF.test_fsf_all
MT.test_luafar_all = LF.test_luafar_all
MT.test_editor_all = TE.test_editor_all
MT.test_panel_all  = TP.test_panel_all

return MT
