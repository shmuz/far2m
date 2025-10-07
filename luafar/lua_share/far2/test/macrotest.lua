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

local AF = "my assertion failed"
local function assert_eq(a,b,m)    assert(a == b, m or AF)               return true; end
local function assert_neq(a,b,m)   assert(a ~= b, m or AF)               return true; end
local function assert_num(v,m)     assert(type(v)=="number", m or AF)    return v; end
local function assert_str(v,m)     assert(type(v)=="string", m or AF)    return v; end
local function assert_table(v,m)   assert(type(v)=="table", m or AF)     return v; end
local function assert_bool(v,m)    assert(type(v)=="boolean", m or AF)   return true; end
local function assert_func(v,m)    assert(type(v)=="function", m or AF)  return v; end
local function assert_udata(v,m)   assert(type(v)=="userdata", m or AF)  return v; end
local function assert_nil(v,m)     assert(v==nil, m or AF)               return true; end
local function assert_false(v,m)   assert(v==false, m or AF)             return true; end
local function assert_true(v,m)    assert(v==true, m or AF)              return true; end
local function assert_err(...)     assert(pcall(...)==false, AF)         return true; end

local function assert_range(v, low, high)
  if low then assert(v >= low, v) end
  if high then assert(v <= high, v) end
  return v
end

local function assert_numint(v,m)
  assert(type(v)=="number" or bit64.type(v), m or AF)
  return v
end

local MT = {} -- "macrotest", this module
local F = far.Flags
local band, bor, bnot = bit64.band, bit64.bor, bit64.bnot
local luamacroId = far.GetPluginId()
local hlfviewerId = 0x1AF0754D

local function pack (...)
  return { n=select("#",...), ... }
end

local TmpFileName = far.InMyTemp("macrotest.tmp")

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
  assert_false (Area.Other)
  assert_false (Area.Viewer)
  assert_false (Area.Editor)
  assert_false (Area.Dialog)
  assert_false (Area.Search)
  assert_false (Area.Disks)
  assert_false (Area.MainMenu)
  assert_false (Area.Menu)
  assert_false (Area.Help)
  assert_false (Area.Info)
  assert_false (Area.QView)
  assert_false (Area.Tree)
  assert_false (Area.FindFolder)
  assert_false (Area.UserMenu)
  assert_false (Area.ShellAutoCompletion)
  assert_false (Area.DialogAutoCompletion)
  assert_false (Area.Grabber)
end

local function test_mf_akey()
  assert_eq(akey, mf.akey)
  local key,name = akey(0),akey(1)
  assert(key==far.NameToKey(MacroKey1) and name==MacroKey1 or
         key==far.NameToKey(MacroKey2) and name==MacroKey2)
  -- (the 2nd parameter is tested in function test_mf_eval).
end

local function test_bit64()
  for _,name in ipairs{"band","bnot","bor","bxor","lshift","rshift"} do
    assert_eq   (_G[name], bit64[name])
    assert_func (bit64[name])
  end

  local a,b,c = 0xFF,0xFE,0xFD
  assert_eq(band(a,b,c,a,b,c), 0xFC)
  a,b,c = bit64.new(0xFF),bit64.new(0xFE),bit64.new(0xFD)
  assert_eq(band(a,b,c,a,b,c), 0xFC)

  a,b = bit64.new("0xFFFF0000FFFF0000"),bit64.new("0x0000FFFF0000FFFF")
  assert_eq(band(a,b), 0)
  assert_eq(bor(a,b), -1)
  assert_eq(a+b, -1)

  a,b,c = 1,2,4
  assert_eq(bor(a,b,c,a,b,c), 7)

  for k=-3,3 do assert_eq(bnot(k), -1-k) end
  assert_eq(bnot(bit64.new(5)), -6)

  assert_eq(bxor(0x01,0xF0,0xAA), 0x5B)
  assert_eq(lshift(0xF731,4),  0xF7310)
  assert_eq(rshift(0xF7310,4), 0xF731)

  local v = bit64.new(5)
  assert_true(v+2==7  and 2+v==7)
  assert_true(v-2==3  and 2-v==-3)
  assert_true(v*2==10 and 2*v==10)
  assert_true(v/2==2  and 2/v==0)
  assert_true(v%2==1  and 2%v==2)
  assert_true(v+v==10 and v-v==0 and v*v==25 and v/v==1 and v%v==0)

  local w = lshift(1,63)
  assert_eq(w, bit64.new("0x8000".."0000".."0000".."0000"))
  assert_eq(rshift(w,63), 1)
  assert_eq(rshift(w,64), 0)
  assert_eq(bit64.arshift(w,62), -2)
  assert_eq(bit64.arshift(w,63), -1)
  assert_eq(bit64.arshift(w,64), -1)
end

local function test_mf_eval()
  assert_eq(eval, mf.eval)

  -- test arguments validity checking
  assert_eq (eval(""), 0)
  assert_eq (eval("", 0), 0)
  assert_eq (eval(), -1)
  assert_eq (eval(0), -1)
  assert_eq (eval(true), -1)
  assert_eq (eval("", -1), -1)
  assert_eq (eval("", 5), -1)
  assert_eq (eval("", true), -1)
  assert_eq (eval("", 1, true), -1)
  assert_eq (eval("",1,"javascript"), -1)

  -- test macro-not-found error
  assert_eq (eval("", 2), -2)

  -- We will modify the global 'temp'. Let it be restored when the macro terminates.
  -- luacheck: globals temp
  mf.AddExitHandler(function(v) temp=v; end, temp)
  temp=3
  assert_eq (eval("temp=5+7"), 0)
  assert_eq (temp, 12)

  temp=3
  assert_eq (eval("temp=5+7",0,"moonscript"), 0)
  assert_eq (eval("temp=5+7",1,"lua"), 0)
  assert_eq (eval("temp=5+7",3,"lua"), "")
  assert_eq (eval("temp=5+7",1,"moonscript"), 0)
  assert_eq (eval("temp=5+7",3,"moonscript"), "")
  assert_eq (temp, 3)
  assert_eq (eval("getfenv(1).temp=12",0,"moonscript"), 0)
  assert_eq (temp, 12)

  assert_eq (eval("5",0,"moonscript"), 0)
  assert_eq (eval("5+7",1,"lua"), 11)
  assert_eq (eval("5+7",1,"moonscript"), 0)
  assert_eq (eval("5 7",1,"moonscript"), 11)

  -- test with Mode==2
  local code = ([[
    local key = akey(1,0)
    assert(key=="%s" or key=="%s")
    assert(akey(1,1)=="CtrlA")
    foobar = (foobar or 0) + 1
    return foobar,false,5,nil,"foo"
  ]]):format(MacroKey1, MacroKey2)
  local Id = assert_udata(far.MacroAdd(nil,nil,"CtrlA",code))
  for k=1,3 do
    local ret1,a,b,c,d,e = eval("CtrlA",2)
    assert_true(ret1==0 and a==k and b==false and c==5 and d==nil and e=="foo")
  end
  assert_true(far.MacroDelete(Id))
end

local function test_mf_abs()
  assert_eq (mf.abs(1.3), 1.3)
  assert_eq (mf.abs(-1.3), 1.3)
  assert_eq (mf.abs(0), 0)
end

local function test_mf_acall()
  local a,b,c,d = mf.acall(function(p) return 3, nil, p, "foo" end, 77)
  assert_true (a==3 and b==nil and c==77 and d=="foo")
  assert_true (mf.acall(far.Show))
  TestArea("Menu",nil,"Esc")
end

local function test_mf_asc()
  assert_eq (mf.asc("0"), 48)
  assert_eq (mf.asc("Я"), 1071)
end

local function test_mf_atoi()
  for _,v in ipairs { "0", "-10", "0x11" } do
    assert_eq(mf.atoi(v), tonumber(v))
  end

  assert_eq (mf.atoi("1011",2),  tonumber("1011",2))
  assert_eq (mf.atoi("1234",5),  tonumber("1234",5))
  assert_eq (mf.atoi("-1234",5), -194)

  for _,v in ipairs { "123456789123456789", "-123456789123456789",
                      "0x1B69B4BACD05F15", "-0x1B69B4BACD05F15" } do
    assert_eq(mf.atoi(v), bit64.new(v))
  end
end

local function test_mf_chr()
  assert_eq (mf.chr(48), "0")
  assert_eq (mf.chr(1071), "Я")
end

local function test_mf_clip()
  local oldval = far.PasteFromClipboard() -- store

  mf.clip(5,2) -- turn on the internal clipboard
  assert_eq (mf.clip(5,-1), 2)
  assert_eq (mf.clip(5,1),  2) -- turn on the OS clipboard
  assert_eq (mf.clip(5,-1), 1)

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
  assert_eq (mf.clip(0), "bar")
  mf.clip(5,1); assert_eq (mf.clip(0), "foo")
  mf.clip(5,2); assert_eq (mf.clip(0), "bar")

  mf.clip(3);   assert_eq (mf.clip(0), "foo")
  mf.clip(5,1); assert_eq (mf.clip(0), "foo")

  mf.clip(5,2); mf.clip(1,"bar")
  mf.clip(5,1); assert_eq (mf.clip(0), "foo")
  mf.clip(4);   assert_eq (mf.clip(0), "bar")
  mf.clip(5,2); assert_eq (mf.clip(0), "bar")

  mf.clip(5,1) -- leave the OS clipboard active in the end
  far.CopyToClipboard(oldval or "") -- restore
end

local function test_mf_env()
  mf.env("Foo",1,"Bar")
  assert_eq (mf.env("Foo"), "Bar")
  mf.env("Foo",1,"")
  assert_eq (mf.env("Foo"), "")
  mf.env("Foo",1,nil)
  assert_nil(win.GetEnv("Foo"))
end

local function test_mf_fattr()
  DeleteTmpFile()
  assert_eq (mf.fattr(TmpFileName), -1)
  WriteTmpFile("")
  local attr = mf.fattr(TmpFileName)
  DeleteTmpFile()
  assert(attr >= 0)
end

local function test_mf_fexist()
  WriteTmpFile("")
  assert_true(mf.fexist(TmpFileName))
  DeleteTmpFile()
  assert_false(mf.fexist(TmpFileName))
end

local function test_mf_msgbox()
  assert_eq (msgbox, mf.msgbox)
  mf.postmacro(function() Keys("Esc") end)
  assert_eq (0, msgbox("title","message"))
  mf.postmacro(function() Keys("Enter") end)
  assert_eq (1, msgbox("title","message"))
end

local function test_mf_prompt()
  assert_eq (prompt, mf.prompt)
  mf.postmacro(Keys, "a b c Esc")
  assert_false (prompt())
  mf.postmacro(Keys, "a b c Enter")
  assert_eq ("abc", prompt())
end

local function test_mf_date()
  assert_str (mf.date())
  assert_str (mf.date("%a"))
end

local function test_mf_fmatch()
  assert_eq (mf.fmatch("Readme.txt", "*.txt"), 1)
  assert_eq (mf.fmatch("Readme.txt", "Readme.*|*.txt"), 0)
  assert_eq (mf.fmatch("/dir/Readme.txt", "/txt$/i"), 1)
  assert_eq (mf.fmatch("/dir/Readme.txt", "/txt$"), -1)
end

local function test_mf_fsplit()
  local bRoot = 0x01
  local bPath = 0x02
  local bName = 0x04
  local bExt  = 0x08

  local path = "C:/Program Files/Far/Far.exe"
  assert_eq (mf.fsplit(path, bRoot), "C:/")
  assert_eq (mf.fsplit(path, bPath), "/Program Files/Far/")
  assert_eq (mf.fsplit(path, bName), "Far")
  assert_eq (mf.fsplit(path, bExt),  ".exe")

  assert_eq (mf.fsplit(path, bRoot + bPath), "C:/Program Files/Far/")
  assert_eq (mf.fsplit(path, bName + bExt),  "Far.exe")
  assert_eq (mf.fsplit(path, bRoot + bPath + bName + bExt), path)
end

local function test_mf_iif()
  assert_eq (mf.iif(true,  1, 2), 1)
  assert_eq (mf.iif("a",   1, 2), 1)
  assert_eq (mf.iif(100,   1, 2), 1)
  assert_eq (mf.iif(false, 1, 2), 2)
  assert_eq (mf.iif(nil,   1, 2), 2)
  assert_eq (mf.iif(0,     1, 2), 2)
  assert_eq (mf.iif("",    1, 2), 2)
end

local function test_mf_index()
  assert_eq (mf.index("language","gua",0), 3)
  assert_eq (mf.index("language","gua",1), 3)
  assert_eq (mf.index("language","gUA",1), -1)
  assert_eq (mf.index("language","gUA",0), 3)
end

local function test_mf_int()
  assert_eq (mf.int("2.99"), 2)
  assert_eq (mf.int("-2.99"), -2)
  assert_eq (mf.int("0x10"), 0)
  for _,v in ipairs { "123456789123456789", "-123456789123456789" } do
    assert_eq(mf.int(v), bit64.new(v))
  end
end

local function test_mf_itoa()
  assert_eq (mf.itoa(100),    "100")
  assert_eq (mf.itoa(100,10), "100")
  assert_eq (mf.itoa(100,2),  "1100100")
  assert_eq (mf.itoa(100,16), "64")
  assert_eq (mf.itoa(100,36), "2s")
  for _,v in ipairs { "123456789123456789", "-123456789123456789" } do
    assert_eq(mf.itoa(bit64.new(v)), v)
  end
end

local function test_mf_key()
  local ref = {
    F.KEY_CTRL,         "Ctrl",
    F.KEY_ALT,          "Alt",
    F.KEY_SHIFT,        "Shift",
    F.KEY_RCTRL,        "RCtrl",
    F.KEY_RALT,         "RAlt",
    F.KEY_CTRLALTF10,   "CtrlAltF10",
  }
  for i=1,#ref,2 do
    assert_eq (mf.key        (ref[i]), ref[i+1])
    assert_eq (far.KeyToName (ref[i]), ref[i+1])
  end
  assert_eq (mf.key("AltShiftF11"), "AltShiftF11")
  assert_eq (mf.key("foobar"), "")
end

-- Separate tests for mf.float and mf.string are locale-dependent, thus they are tested together.
local function test_mf_float_and_string()
  local t = { 0, -0, 2.56e1, -5.37, -2.2e100, 2.2e-100 }
  for _,num in ipairs(t) do
    assert_eq (mf.float(mf.string(num)), num)
  end
end

local function test_mf_lcase()
  assert_eq (mf.lcase("FOo БАр"), "foo бар")
end

local function test_mf_len()
  assert_eq (mf.len(""), 0)
  assert_eq (mf.len("FOo БАр"), 7)
end

local function test_mf_max()
  assert_eq (mf.max(-2,-5), -2)
  assert_eq (mf.max(2,5), 5)
end

local function test_mf_min()
  assert_eq (mf.min(-2,-5), -5)
  assert_eq (mf.min(2,5), 2)
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
  assert_eq (mf.mload(Key, "name1"), v1)
  assert_eq (mf.mload(Key, "name2"), v2)
  assert_eq (mf.mload(Key, "name3"), v3)
  assert_eq (mf.mload(Key, "name4"), v4)
  assert_eq (mf.mload(Key, "name5"), v5)
  assert_eq (mf.mload(Key, "name6"), v6)
  mf.mdelete(Key, "*")
  assert_eq (mf.mload(Key, "name3"), nil)

  -- test tables
  mf.msave(Key, "name1", { a=5, {b="foo"}, c={d=false} })
  local t=mf.mload(Key, "name1")
  assert_true(t.a==5 and t[1].b=="foo" and t.c.d==false)
  mf.mdelete(Key, "name1")
  assert_nil(mf.mload(Key, "name1"))

  -- test tables more
  local t1, t2, t3 = {5}, {6}, {}
  t1[2], t1[3], t1[4], t1[5] = t1, t2, t3, t3
  t2[2], t2[3] = t1, t2
  t1[t1], t1[t2] = 66, 77
  t2[t1], t2[t2] = 88, 99
  setmetatable(t3, { __index=t1 })
  mf.msave(Key, "name1", t1)

  local T1 = assert_table(mf.mload(Key, "name1"))
  local T2 = assert_table(T1[3])
  local T3 = T1[4]
  assert(type(T3)=="table" and T3==T1[5])
  assert(T1[1]==5 and T1[2]==T1 and T1[3]==T2)
  assert(T2[1]==6 and T2[2]==T1 and T2[3]==T2)
  assert(T1[T1]==66 and T1[T2]==77)
  assert(T2[T1]==88 and T2[T2]==99)
  assert(getmetatable(T3).__index==T1 and T3[1]==5 and rawget(T3,1)==nil)
  mf.mdelete(Key, "*")
  assert_nil(mf.mload(Key, "name1"))
end

local function test_mf_mod()
  assert_eq (mf.mod(11,4), 3)
  assert_eq (math.fmod(11,4), 3)
  assert_eq (11 % 4, 3)

  assert_eq (mf.mod(-1,4), -1)
  assert_eq (math.fmod(-1,4), -1)
  assert_eq (-1 % 4, 3)
end

local function test_mf_replace()
  assert_eq (mf.replace("Foo Бар", "o", "1"), "F11 Бар")
  assert_eq (mf.replace("Foo Бар", "o", "1", 1), "F1o Бар")
  assert_eq (mf.replace("Foo Бар", "O", "1", 1, 1), "Foo Бар")
  assert_eq (mf.replace("Foo Бар", "O", "1", 1, 0), "F1o Бар")
end

local function test_mf_rindex()
  assert_eq (mf.rindex("language","a",0), 5)
  assert_eq (mf.rindex("language","a",1), 5)
  assert_eq (mf.rindex("language","A",1), -1)
  assert_eq (mf.rindex("language","A",0), 5)
end

local function test_mf_strpad()
  assert_eq (mf.strpad("Foo",10,"*",  2), '***Foo****')
  assert_eq (mf.strpad("",   10,"-*-",2), '-*--*--*--')
  assert_eq (mf.strpad("",   10,"-*-"), '-*--*--*--')
  assert_eq (mf.strpad("Foo",10), 'Foo       ')
  assert_eq (mf.strpad("Foo",10,"-"), 'Foo-------')
  assert_eq (mf.strpad("Foo",10," ",  1), '       Foo')
  assert_eq (mf.strpad("Foo",10," ",  2), '   Foo    ')
  assert_eq (mf.strpad("Foo",10,"1234567890",2), '123Foo1234')
end

local function test_mf_strwrap()
  assert_eq (mf.strwrap("Пример строки, которая будет разбита на несколько строк по ширине в 7 символов.", 7,"\n"),
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
  assert_eq (mf.substr("abcdef", 1), "bcdef")
  assert_eq (mf.substr("abcdef", 1, 3), "bcd")
  assert_eq (mf.substr("abcdef", 0, 4), "abcd")
  assert_eq (mf.substr("abcdef", 0, 8), "abcdef")
  assert_eq (mf.substr("abcdef", -1), "f")
  assert_eq (mf.substr("abcdef", -2), "ef")
  assert_eq (mf.substr("abcdef", -3, 1), "d")
  assert_eq (mf.substr("abcdef", 0, -1), "abcde")
  assert_eq (mf.substr("abcdef", 2, -1), "cde")
  assert_eq (mf.substr("abcdef", 4, -4), "")
  assert_eq (mf.substr("abcdef", -3, -1), "de")
end

local function test_mf_testfolder()
  assert_range (mf.testfolder("."), 1, 2)          -- exists
  assert_eq    (mf.testfolder(far.GetMyHome()), 2) -- exists and not empty
  assert_range (mf.testfolder("@:\\"), -1, 0)      -- doesn't exist or access denied
end

local function test_mf_trim()
  assert_eq (mf.trim(" abc "), "abc")
  assert_eq (mf.trim(" abc ",0), "abc")
  assert_eq (mf.trim(" abc ",1), "abc ")
  assert_eq (mf.trim(" abc ",2), " abc")
end

local function test_mf_ucase()
  assert_eq (mf.ucase("FOo БАр"), "FOO БАР")
end

local function test_mf_waitkey()
  assert_eq (mf.waitkey(50,0), "")
  assert_eq (mf.waitkey(50,1), F.KEY_INVALID)
end

local function test_mf_size2str()
  assert_eq (mf.size2str(123,0,5), "  123")
  assert_eq (mf.size2str(123,0,-5), "123  ")
end

local function test_mf_xlat()
  assert_str (mf.xlat("abc"))
  assert_eq (mf.xlat("ghzybr"), "пряник")
  assert_eq (mf.xlat("сщьзгеук"), "computer")
end

local function test_mf_beep()
  assert_bool (mf.beep())
end

local function test_mf_flock()
  for k=0,2 do assert_num (mf.flock(k,-1)) end
end

local function test_mf_GetMacroCopy()
  assert_table (mf.GetMacroCopy(1))
  assert_nil   (mf.GetMacroCopy(1e9))
end

local function test_mf_Keys()
  assert_eq (Keys, mf.Keys)
  Keys("Esc F a r Space M a n a g e r Space Ф А Р")
  assert_eq (panel.GetCmdLine(), "Far Manager ФАР")
  Keys("Esc")
  assert_eq (panel.GetCmdLine(), "")
  -- test invalid keys; see far2m/commit/1225dbeb2d769ba2da8570c20b0e96fd179a3442
  Keys("n1 n2 n3 n4 n5 n6 n7 n8 A B C")
  assert_eq (panel.GetCmdLine(), "ABC")
  Keys("Esc")
end

local function test_mf_exit()
  assert_eq (exit, mf.exit)
  local N
  mf.postmacro(
    function()
      local function f() N=50; exit(); end
      f(); N=100
    end)
  mf.postmacro(Keys, "Esc")
  far.Message("dummy")
  assert_eq (N, 50)
end

local function test_mf_mmode()
  assert_eq (mmode, mf.mmode)
  assert_eq (1, mmode(1,-1))
end

local function test_mf_print()
  assert_eq (print, mf.print)
  assert_func (print)
  -- test on command line
  local str = "abc ABC абв АБВ"
  Keys("Esc")
  print(str)
  assert_eq (panel.GetCmdLine(), str)
  Keys("Esc")
  assert_eq (panel.GetCmdLine(), "")
  -- test on dialog input field
  Keys("F7 CtrlY")
  print(str)
  assert_eq (Dlg.GetValue(-1,0), str)
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
  assert_func (mf.postmacro)
end

local function test_mf_sleep()
  assert_func (mf.sleep)
end

local function test_mf_usermenu()
  mf.usermenu()
  TestArea("UserMenu",nil,"Esc")
end

local function test_mf_EnumScripts()
  local iter = assert_func(mf.EnumScripts("Macro"))
  local Id = assert_udata(far.MacroAdd(nil,nil,"foo","return")) -- assure at least 1 macro loaded
  local s,i = iter()
  far.MacroDelete(Id)
  assert_table(s)
  assert_num(i)
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
  assert_false(CmdLine.Bof)
  assert_true(CmdLine.Eof)
  assert_false(CmdLine.Empty)
  assert_false(CmdLine.Selected)
  assert_eq (CmdLine.Value, "foo Бар")
  assert_eq (CmdLine.ItemCount, 7)
  assert_eq (CmdLine.CurPos, 8)

  Keys"SelWord"
  assert_true(CmdLine.Selected)

  Keys"CtrlHome"
  assert_true(CmdLine.Bof)
  assert_false(CmdLine.Eof)

  Keys"Esc"
  assert_true(CmdLine.Bof)
  assert_true(CmdLine.Eof)
  assert_true(CmdLine.Empty)
  assert_false(CmdLine.Selected)
  assert_eq (CmdLine.Value, "")
  assert_eq (CmdLine.ItemCount, 0)
  assert_eq (CmdLine.CurPos, 1)

  Keys"Esc"
  print("foo Бар")
  assert_eq (CmdLine.Value, "foo Бар")

  Keys"Esc"
  print(("%s %d %s"):format("foo", 5+7, "Бар"))
  assert_eq (CmdLine.Value, "foo 12 Бар")

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
    "Editor.EditorUndoSize",
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
    "Panel.CtrlFRule",
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

  assert_eq(Far.GetConfig("#"), #options)
  assert_err(Far.GetConfig, 0)
  assert_err(Far.GetConfig, #options+1)
  assert_err(Far.GetConfig, "foo.bar")

  for _,opt in ipairs(options) do
    local val,tp,val0,key,name,saved = Far.GetConfig(opt) -- an error is thrown if this function fails
    assert_str(tp)
    assert_str(key)
    assert_str(name)
    assert_bool(saved)

    if tp == "integer" then
      assert_num(val)
      assert_num(val0)
    elseif tp == "boolean" then
      assert_bool(val)
      assert_bool(val0)
    elseif tp == "3-state" then
      assert(val==false or val==true or val==2)
      assert(val0==false or val0==true or val0==2)
    elseif tp == "string" then
      assert_str(val)
      assert_str(val0)
    elseif tp == "binary" then
      assert_table(val)
      assert_str(val[1])
    else
      error("unknown type: "..tp)
    end
  end
end

function MT.test_Far()
  assert_false (Far.FullScreen)
  assert_bool (Far.IsUserAdmin)
  assert_num (Far.PID)
  assert_str (Far.Title)

  local R = assert_table(actl.GetFarRect())
  assert_eq (Far.Height, R.Bottom - R.Top + 1)
  assert_eq (Far.Width, R.Right - R.Left + 1)

  local temp = Far.UpTime
  mf.sleep(50)
  temp = Far.UpTime - temp
  assert_range(temp, 40, 400)

  local val,typ,val0,key,name,saved = Far.GetConfig("EDITOR.defaultcodepage")
  assert_num  (val)
  assert_eq   (typ, "integer")
  assert_num  (val0)
  assert_eq   (key, "Editor")
  assert_eq   (name, "DefaultCodePage")
  assert_true (saved)

  assert_func (Far.DisableHistory)
  assert_num (Far.KbdLayout(0))
  assert_num (Far.KeyBar_Show(0))
  assert_func (Far.Window_Scroll)

  test_Far_GetConfig()
end

local function test_CheckAndGetHotKey()
  mf.acall(far.Menu, {Flags="FMENU_AUTOHIGHLIGHT"},
    {{text="abcd"},{text="abc&d"},{text="abcd"},{text="abcd"},{text="abcd"}})

  assert_eq (Object.CheckHotkey("a"), 1)
  assert_eq (Object.GetHotkey(1), "a")
  assert_eq (Object.GetHotkey(), "a")
  assert_eq (Object.GetHotkey(0), "a")

  assert_eq (Object.CheckHotkey("b"), 3)
  assert_eq (Object.GetHotkey(3), "b")

  assert_eq (Object.CheckHotkey("c"), 4)
  assert_eq (Object.GetHotkey(4), "c")

  assert_eq (Object.CheckHotkey("d"), 2)
  assert_eq (Object.GetHotkey(2), "d")

  assert_eq (Object.CheckHotkey("e"), 0)

  assert_eq (Object.CheckHotkey(""), 5)
  assert_eq (Object.GetHotkey(5), "")
  assert_eq (Object.GetHotkey(6), "")

  Keys("Esc")
end

function MT.test_Menu()
  Keys("F11")
  assert_str(Menu.Value)
  assert_eq (Menu.Value, Menu.GetValue())

  assert_eq(Menu.Id, far.Guids.PluginsMenuId)
  assert_eq(Menu.Id, "937F0B1C-7690-4F85-8469-AA935517F202")

  assert_num(Menu.ItemStatus())

  assert_eq(0, Menu.Filter(0))     -- get status
  assert_eq(0, Menu.Filter(0,-1))  -- get status
  assert_eq(1, Menu.Filter(0, 1))  -- turn filter on
  assert_eq(1, Menu.Filter(0))     -- get status
  assert_eq(1, Menu.Filter(0, 0))  -- turn filter off
  assert_eq(0, Menu.Filter(0))     -- get status

  assert_func(Menu.FilterStr)
  assert_func(Menu.Select)
  assert_func(Menu.Show)

  Keys("Esc")
end

function MT.test_Object()
  assert_bool (Object.Bof)
  assert_num (Object.CurPos)
  assert_bool (Object.Empty)
  assert_bool (Object.Eof)
  assert_num (Object.Height)
  assert_num (Object.ItemCount)
  assert_bool (Object.Selected)
  assert_str (Object.Title)
  assert_num (Object.Width)

  test_CheckAndGetHotKey()
end

function MT.test_Drv()
  assert_eq (Drv.ShowPos, 0) -- "Location" is not open
  Keys"AltF1"
  assert_num (Drv.ShowMode)
  assert_eq (Drv.ShowPos, 1) -- "Location" is open on the left
  Keys"Esc AltF2"
  assert_num (Drv.ShowMode)
  assert_eq (Drv.ShowPos, 2) -- "Location" is open on the right
  Keys"Esc"
  assert_eq (Drv.ShowPos, 0) -- "Location" is not open
end

function MT.test_Help()
  Keys"F1"
  assert_str (Help.FileName)
  assert_str (Help.SelTopic)
  assert_str (Help.Topic)
  Keys"Esc"
end

function MT.test_Mouse()
  assert_num (Mouse.X)
  assert_num (Mouse.Y)
  assert_num (Mouse.Button)
  assert_num (Mouse.CtrlState)
  assert_num (Mouse.EventFlags)
  assert_num (Mouse.LastCtrlState)
end

local function test_XPanel(pan) -- (@pan: either APanel or PPanel)
  local pNum = pan==APanel and 1 or 0
  local panInfo = assert_table(panel.GetPanelInfo(nil, pNum))
  local curItem = assert_table(panel.GetCurrentPanelItem(nil, pNum))
  local colTypes = assert_str(panel.GetColumnTypes(nil, pNum))
  local colCount = select(2, colTypes:gsub("%f[%w]N", "")) -- only "Name" columns
  local R = assert_table(panInfo.PanelRect)

  assert_bool (pan.Bof)
  assert_eq   (pan.ColumnCount, colCount)
  assert_eq   (pan.CurPos, assert_num(panInfo.CurrentItem))
  assert_eq   (pan.Current, assert_str(curItem.FileName))
  assert_num  (pan.DriveType)
  assert_eq   (pan.Empty, panInfo.ItemsNumber==1) -- an empty panel has a ".." item
  assert_bool (pan.Eof)
  assert_eq   (pan.FilePanel, panInfo.PanelType==F.PTYPE_FILEPANEL)
  assert_bool (pan.Filter)
  assert_eq   (pan.Folder, curItem.FileAttributes:match("d")=="d")
  assert_eq   (pan.Format, assert_str(panel.GetPanelFormat(nil, pNum)))
  assert_eq   (pan.Height, R.bottom - R.top + 1)
  assert_eq   (pan.HostFile, assert_str(panel.GetPanelHostFile(nil, pNum)))
  assert_eq   (pan.ItemCount, panInfo.ItemsNumber)
  assert_eq   (pan.Left, 0 ~= band(panInfo.Flags,F.PFLAGS_PANELLEFT))
  assert_num  (pan.OPIFlags)
  assert_eq   (pan.Path, assert_table(panel.GetPanelDirectory(nil, pNum)).Name)
  assert_str  (pan.Path0)
  assert_eq   (pan.Plugin, 0 ~= band(panInfo.Flags,F.PFLAGS_PLUGIN))
  assert_eq   (pan.Prefix, assert_str(panel.GetPanelPrefix(nil, pNum)))
  assert_bool (pan.Root)
  assert_num  (pan.SelCount)
  assert_bool (pan.Selected)
  assert_eq   (pan.Type, panInfo.PanelType)
  assert_str  (pan.UNCPath)
  assert_eq   (pan.Visible, 0 ~= band(panInfo.Flags,F.PFLAGS_VISIBLE))
  assert_eq   (pan.Width, R.right - R.left + 1)

  if pan == APanel then
    Keys "End"  assert_true(pan.Eof); assert_true(pan.Empty or not pan.Bof);
    Keys "Home" assert_true(pan.Bof); assert_true(pan.Empty or not pan.Eof);
  end
end

MT.test_APanel = function() test_XPanel(APanel) end
MT.test_PPanel = function() test_XPanel(PPanel) end

local function test_Panel_Item()
  local index = 0 -- 0 is the current element, otherwise element index
  for pan=0,1 do
    assert_str    (Panel.Item(pan,index,0))  -- file name
    assert_str    (Panel.Item(pan,index,1))  -- short file name
    assert_num    (Panel.Item(pan,index,2))  -- file attributes
    assert_str    (Panel.Item(pan,index,3))  -- creation time
    assert_str    (Panel.Item(pan,index,4))  -- last access time
    assert_str    (Panel.Item(pan,index,5))  -- modification time
    assert_numint (Panel.Item(pan,index,6))  -- size
    assert_numint (Panel.Item(pan,index,7))  -- packed size
    assert_bool   (Panel.Item(pan,index,8))  -- selected
    assert_num    (Panel.Item(pan,index,9))  -- number of links
    assert_num    (Panel.Item(pan,index,10)) -- sort group
    assert_str    (Panel.Item(pan,index,11)) -- diz text
    assert_str    (Panel.Item(pan,index,12)) -- owner
    assert_num    (Panel.Item(pan,index,13)) -- crc32
    assert_num    (Panel.Item(pan,index,14)) -- position when read from the file system
    assert_numint (Panel.Item(pan,index,15)) -- creation time
    assert_numint (Panel.Item(pan,index,16)) -- last access time
    assert_numint (Panel.Item(pan,index,17)) -- modification time
    assert_num    (Panel.Item(pan,index,18)) -- number of streams
    assert_numint (Panel.Item(pan,index,19)) -- size of streams
    assert_str    (Panel.Item(pan,index,20)) -- change time
    assert_numint (Panel.Item(pan,index,21)) -- change time
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
  assert_true(Panel.SetPath(1, pdir))
  assert_true(Panel.SetPath(0, adir, afile))
  assert_eq (pdir, panel.GetPanelDirectory(nil,0).Name)
  assert_eq (adir, panel.GetPanelDirectory(nil,1).Name)
  assert_eq (panel.GetCurrentPanelItem(nil,1).FileName, afile)
  -- restore
  assert_true(Panel.SetPath(1, pdir_old))
  assert_true(Panel.SetPath(0, adir_old))
  actl.Commit()
end

-- N=Panel.Select(panelType,Action[,Mode[,Items]])
local function Test_Panel_Select(pan)
  local adir_old = panel.GetPanelDirectory(nil,pan).Name -- store panel directory

  local PS = assert_func(Panel.Select)
  local ACT_RM, ACT_ADD, ACT_INV, ACT_RST = 0,1,2,3       -- Action
  local MOD_ALL, MOD_INDEX, MOD_LIST, MOD_MASK = 0,1,2,3  -- Mode

  local dir = assert_str(win.GetEnv("FARHOME"))
  assert_true(panel.SetPanelDirectory(nil,pan,dir))
  local pi = assert_table(panel.GetPanelInfo(nil,pan))
  local ItemsCount = assert_num(pi.ItemsNumber)-1 -- don't count ".."
  assert(ItemsCount>=10, "not enough files to test")

  --------------------------------------------------------------
  assert_eq(ItemsCount, PS(1-pan, ACT_ADD, MOD_ALL)) -- select all
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(ItemsCount, pi.SelectedItemsNumber)

  assert_eq(ItemsCount, PS(1-pan, ACT_RM, MOD_ALL)) -- clear all
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  assert_eq(ItemsCount, PS(1-pan, ACT_INV, MOD_ALL)) -- invert
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(ItemsCount, pi.SelectedItemsNumber)

  assert_eq(0, PS(1-pan, ACT_INV, MOD_ALL)) -- invert again (return value is the selection count, contrary to docs)
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  assert_eq(ItemsCount, PS(1-pan, ACT_RST, MOD_ALL)) -- restore (same as Ctrl+M)
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(ItemsCount, pi.SelectedItemsNumber)

  assert_eq(ItemsCount, PS(1-pan, ACT_RM, MOD_ALL)) -- clear all
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  --------------------------------------------------------------
  assert_eq(1, PS(1-pan, ACT_ADD, MOD_INDEX, 5))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(1, pi.SelectedItemsNumber)

  assert_eq(1, PS(1-pan, ACT_RM, MOD_INDEX, 5))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  assert_eq(1, PS(1-pan, ACT_INV, MOD_INDEX, 5))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(1, pi.SelectedItemsNumber)

  assert_eq(1, PS(1-pan, ACT_INV, MOD_INDEX, 5))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  --------------------------------------------------------------
  local list = dir.."/FarEng.hlf\nFarEng.lng" -- the 1-st file with path, the 2-nd without
  assert_eq(2, PS(1-pan, ACT_ADD, MOD_LIST, list))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(2, pi.SelectedItemsNumber)

  assert_eq(2, PS(1-pan, ACT_RM, MOD_LIST, list))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  assert_eq(2, PS(1-pan, ACT_INV, MOD_LIST, list))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(2, pi.SelectedItemsNumber)

  assert_eq(2, PS(1-pan, ACT_INV, MOD_LIST, list))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  --------------------------------------------------------------
  local mask = "*.hlf;*.lng"
  local count = 0
  for i=1, pi.ItemsNumber do
    local item = assert_table(panel.GetPanelItem(nil,pan,i))
    if far.CmpNameList(mask, item.FileName) then count=count+1 end
  end
  assert(count>1, "not enough files to test")

  assert_eq(count, PS(1-pan, ACT_ADD, MOD_MASK, mask))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(count, pi.SelectedItemsNumber)

  assert_eq(count, PS(1-pan, ACT_RM, MOD_MASK, mask))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  assert_eq(count, PS(1-pan, ACT_INV, MOD_MASK, mask))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(count, pi.SelectedItemsNumber)

  assert_eq(count, PS(1-pan, ACT_INV, MOD_MASK, mask))
  pi = assert_table(panel.GetPanelInfo(nil, pan))
  assert_eq(0, pi.SelectedItemsNumber)

  panel.SetPanelDirectory(nil, pan, adir_old) -- restore panel directory
end

function MT.test_Panel()
  test_Panel_Item()

  assert_eq (Panel.FAttr(0,":"), -1)
  assert_eq (Panel.FAttr(1,":"), -1)

  assert_eq (Panel.FExist(0,":"), 0)
  assert_eq (Panel.FExist(1,":"), 0)

  Test_Panel_Select(0)
  Test_Panel_Select(1)
  test_Panel_SetPath()
  assert_func (Panel.SetPos)
  assert_func (Panel.SetPosIdx)
end

function MT.test_Dlg()
  Keys"F7 a b c"
  assert_true(Area.Dialog)
  assert_str(Dlg.Id)
  assert_eq(Dlg.Id, far.Guids.MakeFolderId)
  assert_eq(Dlg.Owner, 0)
  assert(Dlg.ItemCount > 6)
  assert_eq(Dlg.ItemType, 4)
  assert_eq(Dlg.CurPos, 3)
  assert_eq(Dlg.PrevPos, 0)

  Keys"Tab"
  local pos = Dlg.CurPos
  assert(Dlg.CurPos > 3)
  assert_eq(Dlg.PrevPos, 3)
  assert_eq(pos, Dlg.SetFocus(3))
  assert_eq(pos, Dlg.PrevPos)

  assert_eq(Dlg.GetValue(0,0), Dlg.ItemCount)
  Keys"Esc"

  Keys"F10"
  assert_true(Area.Dialog)
  assert_str(Dlg.Id)
  assert_eq(Dlg.Id, far.Guids.FarAskQuitId)
  Keys"Esc"
end

function MT.test_Plugin()
  -- Plugin.Menu
  assert_false(Plugin.Menu())
  assert_true(Plugin.Menu(luamacroId, "EF6D67A2-59F7-4DF3-952E-F9049877B492")) -- call macrobrowser
  assert_true(Area.Menu)
  assert_eq(Menu.Id, "03DEFB28-8734-4EC0-8B25-C879846F0BE5")
  Keys("Esc")
  assert_true(Area.Shell)

  -- Plugin.Config
  assert_false(Plugin.Config())
  if Plugin.Exist(hlfviewerId) then
    assert_true(Plugin.Config(hlfviewerId))
    assert_true(Area.Dialog)
    Keys("Esc")
    assert_true(Area.Shell)
  end

  -- Plugin.Command
  assert_false(Plugin.Command())
  assert_true(Plugin.Command(luamacroId))
  assert_true(Plugin.Command(luamacroId, "view:$FARHOME/FarEng.lng"))
  assert_true(Area.Viewer)
  Keys("Esc")
  assert_true(Area.Shell)

  -- Plugin.Exist
  assert_true(Plugin.Exist(luamacroId))
  assert_false(Plugin.Exist(luamacroId+1))

  -- Plugin.Call, Plugin.SyncCall
  local function test (func, N) -- test arguments and return
    local i1 = bit64.new("0x8765876587658765")
    local r1,r2,r3,r4,r5 = func(luamacroId, "argtest", "foo", i1, -2.34, false, {"foo\0bar"})
    assert(r1=="foo" and r2==i1 and r3==-2.34 and r4==false and type(r5)=="table" and r5[1]=="foo\0bar")

    local src = {}
    for k=1,N do src[k]=k end
    local trg = { func(luamacroId, "argtest", unpack(src)) }
    assert(#trg==N and trg[1]==1 and trg[N]==N)
  end
  test(Plugin.Call, 8000-8)
  test(Plugin.SyncCall, 8000-8)
end

local function test_far_MacroExecute()
  local function test(code, flags)
    local t = far.MacroExecute(code, flags,
      "foo",
      false,
      5,
      nil,
      bit64.new("0x8765876587658765"),
      {"bar"})
    assert_table (t)
    assert_eq    (t.n,  6)
    assert_eq    (t[1], "foo")
    assert_false (t[2])
    assert_eq    (t[3], 5)
    assert_nil   (t[4])
    assert_eq    (t[5], bit64.new("0x8765876587658765"))
    assert_eq    (assert_table(t[6])[1], "bar")
  end
  test("return ...", nil)
  test("return ...", "KMFLAGS_LUA")
  test("...", "KMFLAGS_MOONSCRIPT")
end

local function test_far_MacroAdd()
  local area, key, descr = "MACROAREA_SHELL", "CtrlA", "Test MacroAdd"

  local Id = far.MacroAdd(area, nil, key, [[A = { b=5 }]], descr)
  assert_true(far.MacroDelete(assert_udata(Id)))

  Id = far.MacroAdd(-1, nil, key, [[A = { b=5 }]], descr)
  assert_nil(Id) -- bad area

  Id = far.MacroAdd(area, nil, key, [[A = { b:5 }]], descr)
  assert_nil(Id) -- bad code

  Id = far.MacroAdd(area, "KMFLAGS_MOONSCRIPT", key, [[A = { b:5 }]], descr)
  assert_true(far.MacroDelete(assert_udata(Id)))

  Id = far.MacroAdd(area, "KMFLAGS_MOONSCRIPT", key, [[A = { b=5 }]], descr)
  assert_nil(Id) -- bad code

  Id = far.MacroAdd(area, nil, key, [[@c:\myscript 5+6,"foo"]], descr)
  assert_true(far.MacroDelete(assert_udata(Id)))

  Id = far.MacroAdd(area, nil, key, [[@c:\myscript 5***6,"foo"]], descr)
  assert_true(far.MacroDelete(assert_udata(Id))) -- with @ there is no syntax check till the macro runs

  Id = far.MacroAdd(nil, nil, key, [[@c:\myscript]])
  assert_true(far.MacroDelete(assert_udata(Id))) -- check default area (MACROAREA_COMMON)

  Id = far.MacroAdd(area,nil,key,[[Keys"F7" assert(Dlg.Id=="FAD00DBE-3FFF-4095-9232-E1CC70C67737") Keys"Esc"]],descr)
  assert_eq(0, mf.eval("Shell/"..key, 2))
  assert_true(far.MacroDelete(Id))

  Id = far.MacroAdd(area,nil,key,[[a=5]],descr,function(id,flags) return false end)
  assert_eq(-2, mf.eval("Shell/"..key, 2))
  assert_true(far.MacroDelete(Id))

  Id = far.MacroAdd(area,nil,key,[[a=5]],descr,function(id,flags) error"deliberate error" end)
  assert_eq(-2, mf.eval("Shell/"..key, 2))
  assert_true(far.MacroDelete(Id))

  Id = far.MacroAdd(area,nil,key,[[a=5]],descr,function(id,flags) return id==Id end)
  assert_eq(0, mf.eval("Shell/"..key, 2))
  assert_true(far.MacroDelete(Id))

end

local function test_far_MacroCheck()
  assert_true(far.MacroCheck([[A = { b=5 }]]))
  assert_true(far.MacroCheck([[A = { b=5 }]], "KMFLAGS_LUA"))

  assert_false(far.MacroCheck([[A = { b:5 }]], "KMFLAGS_SILENTCHECK"))

  assert_true(far.MacroCheck([[A = { b:5 }]], "KMFLAGS_MOONSCRIPT"))

  assert_false(far.MacroCheck([[A = { b=5 }]], {KMFLAGS_MOONSCRIPT=1,KMFLAGS_SILENTCHECK=1} ))

  WriteTmpFile [[A = { b=5 }]] -- valid Lua, invalid MoonScript
  assert_true (far.MacroCheck("@"..TmpFileName, "KMFLAGS_LUA"))
  assert_true (far.MacroCheck("@"..TmpFileName.." 5+6,'foo'", "KMFLAGS_LUA")) -- valid file arguments
  assert_false(far.MacroCheck("@"..TmpFileName.." 5***6,'foo'", "KMFLAGS_SILENTCHECK")) -- invalid file arguments
  assert_false(far.MacroCheck("@"..TmpFileName, {KMFLAGS_MOONSCRIPT=1,KMFLAGS_SILENTCHECK=1}))

  WriteTmpFile [[A = { b:5 }]] -- invalid Lua, valid MoonScript
  assert_false (far.MacroCheck("@"..TmpFileName, "KMFLAGS_SILENTCHECK"))
  assert_true  (far.MacroCheck("@"..TmpFileName, "KMFLAGS_MOONSCRIPT"))
  DeleteTmpFile()

  assert_false (far.MacroCheck([[@//////]], "KMFLAGS_SILENTCHECK"))
end

local function test_far_MacroGetArea()
  assert_eq(far.MacroGetArea(), F.MACROAREA_SHELL)
end

local function test_far_MacroGetLastError()
  assert_true  (far.MacroCheck("a=1"))
  assert_eq    (far.MacroGetLastError().ErrSrc, "")
  assert_false (far.MacroCheck("a=", "KMFLAGS_SILENTCHECK"))
  assert       (far.MacroGetLastError().ErrSrc:len() > 0)
end

local function test_far_MacroGetState()
  local st = far.MacroGetState()
  assert(st==F.MACROSTATE_EXECUTING or st==F.MACROSTATE_EXECUTING_COMMON)
end

local function test_MacroControl()
  test_far_MacroAdd()
  test_far_MacroCheck()
  test_far_MacroExecute()
  test_far_MacroGetArea()
  test_far_MacroGetLastError()
  test_far_MacroGetState()
end

local function test_RegexControl()
  local L = win.Utf8ToUtf32
  local pat = "([bc]+)"
  local pat2 = "([bc]+)|(zz)"
  local rep = "%1%1"
  local R = regex.new(pat)
  local R2 = regex.new(pat2)

  local fr,to,cap
  local str, nfound, nrep

  assert_eq(R:bracketscount(), 2)

  fr,to,cap = regex.find("abc", pat)
  assert(fr==2 and to==3 and cap=="bc")
  fr,to,cap = regex.findW(L"abc", pat)
  assert(fr==2 and to==3 and cap==L"bc")

  fr,to,cap = R:find("abc")
  assert(fr==2 and to==3 and cap=="bc")
  fr,to,cap = R:findW(L"abc")
  assert(fr==2 and to==3 and cap==L"bc")

  fr,to,cap = regex.exec("abc", pat2)
  assert(fr==2 and to==3 and #cap==4 and cap[1]==2 and cap[2]==3 and cap[3]==false and cap[4]==false)
  fr,to,cap = regex.execW(L"abc", pat2)
  assert(fr==2 and to==3 and #cap==4 and cap[1]==2 and cap[2]==3 and cap[3]==false and cap[4]==false)

  fr,to,cap = R2:exec("abc")
  assert(fr==2 and to==3 and #cap==4 and cap[1]==2 and cap[2]==3 and cap[3]==false and cap[4]==false)
  fr,to,cap = R2:execW(L"abc")
  assert(fr==2 and to==3 and #cap==4 and cap[1]==2 and cap[2]==3 and cap[3]==false and cap[4]==false)

  assert(regex.match("abc", pat)=="bc")
  assert(regex.matchW(L"abc", pat)==L"bc")

  assert(R:match("abc")=="bc")
  assert(R:matchW(L"abc")==L"bc")

  str, nfound, nrep = regex.gsub("abc", pat, rep)
  assert(str=="abcbc" and nfound==1 and nrep==1)
  str, nfound, nrep = regex.gsubW(L"abc", pat, rep)
  assert(str==L"abcbc" and nfound==1 and nrep==1)

  str, nfound, nrep = R:gsub("abc", rep)
  assert(str=="abcbc" and nfound==1 and nrep==1)
  str, nfound, nrep = R:gsubW(L"abc", rep)
  assert(str==L"abcbc" and nfound==1 and nrep==1)

  local t = {}
  for cap in regex.gmatch("abc", ".") do t[#t+1]=cap end
  assert(#t==3 and t[1]=="a" and t[2]=="b" and t[3]=="c")
  for cap in regex.gmatchW(L"abc", ".") do t[#t+1]=cap end
  assert(#t==6 and t[4]==L"a" and t[5]==L"b" and t[6]==L"c")

  t, R = {}, regex.new(".")
  for cap in R:gmatch("abc") do t[#t+1]=cap end
  assert(#t==3 and t[1]=="a" and t[2]=="b" and t[3]=="c")
  for cap in R:gmatchW(L"abc") do t[#t+1]=cap end
  assert(#t==6 and t[4]==L"a" and t[5]==L"b" and t[6]==L"c")

  str, nfound, nrep = regex.gsub(";a;", "a*", "ITEM")
  assert(str=="ITEM;ITEM;ITEM" and nfound==3 and nrep==3)
  str, nfound, nrep = regex.gsub(";a;", "a*?", "ITEM")
  assert(str=="ITEM;ITEMaITEM;ITEM" and nfound==4 and nrep==4)

  -- separate tests from lrexlib
  local test = require "far2.test.regex.runtest"
  local numerr = test(function() end)
  assert_eq(numerr, 0)

  -- this test used to crash Far2L (fixed in a commit from 2022-09-02)
  local rx = regex.new("abcd", "o")
  local txt = "\233\149\254\255".."\255\15\31\0".."\76\137\226\190".."\100\0\0\0"
  assert_nil(rx:findW(txt))

  -- Mantis 3336 (https://bugs.farmanager.com/view.php?id=3336)
  local c1,c2,c3
  fr,to,c1 = regex.find("{}", "\\{(.)?\\}")
  assert(fr==1 and to==2 and c1==false)
  fr,to,c1,c2,c3 = regex.find("bbb", "(b)?b(b)?(b)?b")
  assert(fr==1 and to==3 and c1=="b" and c2==false and c3==false)

  -- Mantis 1388 (https://bugs.farmanager.com/view.php?id=1388)
  c1,c2 = regex.match("123", "(\\d+)A|(\\d+)")
  assert(c1==false and c2=="123")

  -- Issue #609 (https://github.com/FarGroup/FarManager/issues/609)
  c1 = regex.match("88", "(8)+")
  assert_eq(c1, "8")
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
      assert_eq(p1, 1)
    end
  end
  local Dlg = { {"DI_EDIT", 3,1,56,10, 0,0,0,0, "a"}, }
  mf.acall(far.Dialog, nil,-1,-1,60,3,"Contents",Dlg, 0, DlgProc)
  assert_true(Area.Dialog)
  Keys("W 1 2 3 4 BS Esc")
  assert_eq(check, 6)
  assert_eq(Dlg[1][10], "W123")
end

local function test_utf8_len()
  assert_eq ((""):len(), 0)
  assert_eq (("FOo БАр"):len(), 7)
  assert_nil (("\239"):len()) -- invalid UTF-8
end

local function test_utf8_sub()
  local text = "abcdабвг"
  local len = assert(text:len()==8) and 8

  for _,start in ipairs{-len*3, 0, 1} do
    assert_eq (text:sub(start, -len*4), "")
    assert_eq (text:sub(start, -len*3), "")
    assert_eq (text:sub(start, -len*2), "")
    assert_eq (text:sub(start, -len-1 + 0), "")
    assert_eq (text:sub(start,          0), "")
    assert_eq (text:sub(start, -len-1 + 1), "a")
    assert_eq (text:sub(start,          1), "a")
    assert_eq (text:sub(start, -len-1 + 6), "abcdаб")
    assert_eq (text:sub(start,          6), "abcdаб")
    assert_eq (text:sub(start, len*1), text)
    assert_eq (text:sub(start, len*2), text)
  end

  for _,start in ipairs{3, -6} do
    assert_eq (text:sub(start, -len*2), "")
    assert_eq (text:sub(start,      0), "")
    assert_eq (text:sub(start,      1), "")
    assert_eq (text:sub(start, start-1), "")
    assert_eq (text:sub(start,      -6), "c")
    assert_eq (text:sub(start, start+0), "c")
    assert_eq (text:sub(start,      -5), "cd")
    assert_eq (text:sub(start, start+3), "cdаб")
    assert_eq (text:sub(start,      -3), "cdаб")
    assert_eq (text:sub(start, len), "cdабвг")
    assert_eq (text:sub(start, 2*len), "cdабвг")
  end

  for _,start in ipairs{len+1, 2*len} do
    for _,fin in ipairs{-2*len, -1*len, -1, 0, 1, len-1, len, 2*len} do
      assert_eq(text:sub(start,fin), "")
    end
  end

  for _,start in ipairs{-2*len,-len-1,-len,-len+1,-1,0,1,len-1,len,len+1} do
    assert_eq(text:sub(start), text:sub(start,len))
  end

  assert_err(text.sub, text)
  assert_err(text.sub, text, {})
  assert_err(text.sub, text, nil)
end

local function test_utf8_lower_upper()
  assert_eq ((""):lower(), "")
  assert_eq (("abc"):lower(), "abc")
  assert_eq (("ABC"):lower(), "abc")

  assert_eq ((""):upper(), "")
  assert_eq (("abc"):upper(), "ABC")
  assert_eq (("ABC"):upper(), "ABC")

  local russian_abc = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя"
  local part1, part2 = russian_abc:sub(1,33), russian_abc:sub(34)
  assert_eq (part1:lower(), part2)
  assert_eq (part2:lower(), part2)
  assert_eq (part1:upper(), part1)
  assert_eq (part2:upper(), part1)

  local noletters = "1234567890~@#$%^&*()_+-=[]{}|/\\';.,"
  assert_eq (noletters:lower(), noletters)
  assert_eq (noletters:upper(), noletters)
end

---------------------------------------------------------------------------------------------------
-- ACTL_GETWINDOWCOUNT, ACTL_GETWINDOWTYPE, ACTL_GETWINDOWINFO, ACTL_SETCURRENTWINDOW, ACTL_COMMIT
---------------------------------------------------------------------------------------------------
local function test_AdvControl_Window()
  local num, t

  num = far.AdvControl("ACTL_GETWINDOWCOUNT")
  assert(num == 1) -- no "desktop" in Far2
  mf.acall(far.Show); mf.acall(far.Show)
--  assert(far.AdvControl("ACTL_GETSHORTWINDOWINFO").Type == F.WTYPE_VMENU)
--  assert(num+2 == far.AdvControl("ACTL_GETWINDOWCOUNT")) -- menus don't count as windows?
  Keys("Esc Esc")
  assert_eq(num, far.AdvControl("ACTL_GETWINDOWCOUNT"))

  -- Get information about 2 available windows
  t = assert_table(far.AdvControl("ACTL_GETWINDOWINFO", 1))
  assert(t.Type==F.WTYPE_DESKTOP and t.Id==0 and t.Pos==1 and t.Flags==0 and #t.TypeName>0 and
         t.Name=="")

  t = assert_table(far.AdvControl("ACTL_GETWINDOWINFO", 2))
  assert(t.Type==F.WTYPE_PANELS and t.Id==0 and t.Pos==2 and t.Flags==F.WIF_CURRENT and
         #t.TypeName>0 and #t.Name>0)
  assert_eq(far.AdvControl("ACTL_GETWINDOWTYPE").Type, F.WTYPE_PANELS)

  -- Set "Desktop" as the current window
  assert_eq (1, far.AdvControl("ACTL_SETCURRENTWINDOW", 1))
  assert_eq (1, far.AdvControl("ACTL_COMMIT"))
  t = assert_table(far.AdvControl("ACTL_GETWINDOWINFO", 2)) -- "z-order": the window that was #1 is now #2
  assert(t.Type==0 and t.Id==0 and t.Pos==2 and t.Flags==F.WIF_CURRENT and #t.TypeName>0 and
         t.Name=="")
  assert_eq (far.AdvControl("ACTL_GETWINDOWTYPE").Type, F.WTYPE_DESKTOP)
  t = assert_table(far.AdvControl("ACTL_GETWINDOWINFO", 1))
  assert(t.Type==F.WTYPE_PANELS and t.Id==0 and t.Pos==1 and t.Flags==0 and #t.TypeName>0 and
         #t.Name>0)

  -- Restore "Panels" as the current window
  assert_eq (1, far.AdvControl("ACTL_SETCURRENTWINDOW", 1))
  assert_eq (1, far.AdvControl("ACTL_COMMIT"))
  assert_eq (far.AdvControl("ACTL_GETWINDOWTYPE").Type, F.WTYPE_PANELS)
end

local function test_AdvControl_Colors()
  local allcolors = assert_table(far.AdvControl("ACTL_GETARRAYCOLOR"))
  assert_eq(#allcolors, 147)
  for i,color in ipairs(allcolors) do
    assert_eq(far.AdvControl("ACTL_GETCOLOR", i-1), color)
  end
  assert_nil(far.AdvControl("ACTL_GETCOLOR", #allcolors))
  assert_nil(far.AdvControl("ACTL_GETCOLOR", -1))

  -- change the colors
  local arr, elem = {StartIndex=0; Flags=0}, 123
  for n=1,#allcolors do arr[n]=elem end
  assert_eq(1, far.AdvControl("ACTL_SETARRAYCOLOR", arr))
  for n=1,#allcolors do
    assert_eq(elem, far.AdvControl("ACTL_GETCOLOR", n-1))
  end

  -- restore the colors
  assert_eq(1, far.AdvControl("ACTL_SETARRAYCOLOR", allcolors))
end

local function test_AdvControl_Misc()
  local t

  assert_eq (far.AdvControl("ACTL_GETFARVERSION"):sub(1,1), "2")
  assert_eq (far.AdvControl("ACTL_GETFARVERSION",true), 2)

  t = far.AdvControl("ACTL_GETFARRECT")
  assert(t.Left>=0 and t.Top>=0 and t.Right>t.Left and t.Bottom>t.Top)

  assert_eq(1, far.AdvControl("ACTL_SETCURSORPOS", {X=-1,Y=0}))
  for k=0,2 do
    assert_eq(1, far.AdvControl("ACTL_SETCURSORPOS", {X=k,Y=k+1}))
    t = assert_table(far.AdvControl("ACTL_GETCURSORPOS"))
    assert(t.X==k and t.Y==k+1)
  end

  assert_true(mf.acall(far.AdvControl, "ACTL_WAITKEY", "KEY_F4"))
  Keys("F4")
  assert_true(mf.acall(far.AdvControl, "ACTL_WAITKEY"))
  Keys("F2")

  local val=0
  local incr,count = 2,100
  for _=1,count do
    far.AdvControl("ACTL_SYNCHRO", function(a) val=val+a end, incr)
  end
  Keys("foo")
  assert_eq(val, incr*count)
end

local function test_ACTL()
  assert_func  ( actl.Commit)
  assert_table ( actl.GetArrayColor())
  assert_range ( #actl.GetArrayColor(),142,152)
  assert_numint( actl.GetColor("COL_DIALOGBOXTITLE"))
  assert_num   ( actl.GetConfirmations())
  assert_table ( actl.GetCursorPos())
  assert_num   ( actl.GetDescSettings())
  assert_num   ( actl.GetDialogSettings())
  assert_table ( actl.GetFarRect())
  assert_str   ( actl.GetFarManagerVersion())
  assert_num   ( actl.GetFarManagerVersion(true))
  assert_num   ( actl.GetInterfaceSettings())
  assert_num   ( actl.GetPanelSettings())
  assert_range ( actl.GetPluginMaxReadData(), 0x1000, 0x80000)
  assert_num   ( actl.GetSystemSettings())
  assert_str   ( actl.GetSysWordDiv())
  assert_range ( actl.GetWindowCount(), 1)
  assert_table ( actl.GetWindowInfo(1))
  assert_func  ( actl.Quit)
  assert_func  ( actl.RedrawAll)
  assert_func  ( actl.SetArrayColor)
  assert_func  ( actl.SetCurrentWindow)
  assert_func  ( actl.SetCursorPos)
  assert_func  ( actl.Synchro)
  assert_func  ( actl.WaitKey)
end

local function test_AdvControl()
--test_AdvControl_Window()
  test_AdvControl_Colors()
  test_AdvControl_Misc()
  test_ACTL()
end

local function test_far_GetMsg()
  assert_str (far.GetMsg(0))
end

local function test_clipboard()
  local orig = far.PasteFromClipboard()
  local values = { "Человек", "foo", "", n=3 }
  for k=1,values.n do
    local v = values[k]
    far.CopyToClipboard(v)
    assert_eq(far.PasteFromClipboard(), v)
  end
  if orig then
    far.CopyToClipboard(orig)
    assert_eq(far.PasteFromClipboard(), orig)
  end
end

local function test_ProcessName()
  assert_true  (far.CheckMask("f*.ex?"))
  assert_true  (far.CheckMask("/(abc)?def/"))
  assert_false (far.CheckMask("/[[[/"))

  assert_eq    (far.GenerateName("a??b.*", "cdef.txt"),     "adeb.txt")
  assert_eq    (far.GenerateName("a??b.*", "cdef.txt", 50), "adeb.txt")
  assert_eq    (far.GenerateName("a??b.*", "cdef.txt", 2),  "adbef.txt")

  assert_true  (far.CmpName("f*.ex?",      "ftp.exe"        ))
  assert_true  (far.CmpName("f*.ex?",      "fc.exe"         ))
  assert_true  (far.CmpName("f*.ex?",      "f.ext"          ))

  assert_false (far.CmpName("f*.ex?",      "FTP.exe", "PN_CASESENSITIVE" ))
  assert_true  (far.CmpName("f*.ex?",      "FTP.exe"        ))

  assert_false (far.CmpName("f*.ex?",      "a/f.ext"        ))
  assert_false (far.CmpName("f*.ex?",      "a/f.ext", 0     ))
  assert_true  (far.CmpName("f*.ex?",      "a/f.ext", "PN_SKIPPATH" ))

  assert_true  (far.CmpName("*co*",        "color.ini"      ))
  assert_true  (far.CmpName("*co*",        "edit.com"       ))
  assert_true  (far.CmpName("[c-ft]*.txt", "config.txt"     ))
  assert_true  (far.CmpName("[c-ft]*.txt", "demo.txt"       ))
  assert_true  (far.CmpName("[c-ft]*.txt", "faq.txt"        ))
  assert_true  (far.CmpName("[c-ft]*.txt", "tips.txt"       ))
  assert_true  (far.CmpName("*",           "foo.bar"        ))
  assert_true  (far.CmpName("*.cpp",       "foo.cpp"        ))
  assert_false (far.CmpName("*.cpp",       "foo.abc"        ))
  assert_false (far.CmpName("*|*.cpp",     "foo.abc"        )) -- exclude mask not supported
  assert_false (far.CmpName("*,*",         "foo.bar"        )) -- mask list not supported

  assert_true (far.CmpNameList("*",          "foo.bar"    ))
  assert_true (far.CmpNameList("*.cpp",      "foo.cpp"    ))
  assert_false(far.CmpNameList("*.cpp",      "foo.abc"    ))

  assert_true (far.CmpNameList("*|*.cpp",    "foo.abc"    )) -- exclude mask IS supported
  assert_true (far.CmpNameList("|*.cpp",     "foo.abc"    ))
  assert_true (far.CmpNameList("*|",         "foo.abc"    ))
  assert_true (far.CmpNameList("*|bar|*",    "foo.abc"    ))
  assert_false(far.CmpNameList("*|*.abc",    "foo.abc"    ))
  assert_false(far.CmpNameList("|",          "foo.abc"    ))

  assert_true (far.CmpNameList("*.aa,*.bar", "foo.bar"    ))
  assert_true (far.CmpNameList("*.aa,*.bar", "c:/foo.bar" ))
  assert_true (far.CmpNameList("/.+/",       "c:/foo.bar" ))
  assert_true (far.CmpNameList("/bar$/",     "c:/foo.bar" ))
  assert_false(far.CmpNameList("/dar$/",     "c:/foo.bar" ))
  assert_true (far.CmpNameList("/abcd/;*",    "/abcd/foo.bar", "PN_SKIPPATH"))
  assert_true (far.CmpNameList("/Makefile(.+)?/", "Makefile"))
  assert_true (far.CmpNameList("/makefile([._\\-].+)?$/i", "Makefile", "PN_SKIPPATH"))

  assert_false (far.CmpNameList("f*.ex?",    "a/f.ext", 0     ))
  assert_true  (far.CmpNameList("f*.ex?",    "a/f.ext", "PN_SKIPPATH" ))

  assert_true  (far.CmpNameList("/ BAR ; /xi  ;*.md", "bar;foo"))
  assert_false (far.CmpNameList("/ BAR ; /xi  ;*.md", "bar,foo"))
  assert_true  (far.CmpNameList("/ BAR ; /xi  ;*.md", "README.md"))
  assert_false (far.CmpNameList("/ BAR ; /xi  ;*.md", "README.me"))
end

local function test_FarStandardFunctions()
  test_clipboard()

  test_ProcessName()

  assert_eq(far.ConvertPath([[/foo/bar/../../abc]], "CPM_FULL"), [[/abc]])

  assert_eq(far.FormatFileSize(123456, 8), "  123456")
  assert_eq(far.FormatFileSize(123456, -8), "123456  ")

  assert_str (far.GetCurrentDirectory())

  assert_true  (far.LIsAlpha("A"))
  assert_true  (far.LIsAlpha("Я"))
  assert_false (far.LIsAlpha("7"))
  assert_false (far.LIsAlpha(";"))

  assert_true  (far.LIsAlphanum("A"))
  assert_true  (far.LIsAlphanum("Я"))
  assert_true  (far.LIsAlphanum("7"))
  assert_false (far.LIsAlphanum(";"))

  assert_false (far.LIsLower("A"))
  assert_true  (far.LIsLower("a"))
  assert_false (far.LIsLower("Я"))
  assert_true  (far.LIsLower("я"))
  assert_false (far.LIsLower("7"))
  assert_false (far.LIsLower(";"))

  assert_true  (far.LIsUpper("A"))
  assert_false (far.LIsUpper("a"))
  assert_true  (far.LIsUpper("Я"))
  assert_false (far.LIsUpper("я"))
  assert_false (far.LIsUpper("7"))
  assert_false (far.LIsUpper(";"))

  assert_eq (far.LLowerBuf("abc-ABC-эюя-ЭЮЯ-7;"), "abc-abc-эюя-эюя-7;")
  assert_eq (far.LUpperBuf("abc-ABC-эюя-ЭЮЯ-7;"), "ABC-ABC-ЭЮЯ-ЭЮЯ-7;")

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

  assert(6 == far.StrCellsCount("🔥🔥🔥"))
  assert(4 == far.StrCellsCount("🔥🔥🔥", 2))

  local a, b
  a, b = far.StrSizeOfCells("🔥🔥🔥", 100)
  assert(a == 3 and b == 6)
  a, b = far.StrSizeOfCells("🔥🔥🔥", 100, true)
  assert(a == 3 and b == 6)
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
  assert_true (editor.SaveFile())
  assert_true (editor.Quit())
  actl.Commit()
  fp = assert(io.open(fname))
  local k = 0
  for line in fp:lines() do
    k = k + 1
    assert_eq (line, "foo")
  end
  fp:close()
  win.DeleteFile(fname)
  assert_eq (k, 3)
end

local function test_gmatch_coro()
  local function yieldFirst(it)
    return coroutine.wrap(function()
      coroutine.yield(it())
    end)
  end

  local it = ("1 2 3"):gmatch("(%d+)")
  local head = yieldFirst(it)
  assert_eq (head(), "1")
end

local function test_PluginsControl()
  local mod = assert(far.PluginStartupInfo().ModuleName)
  local hnd1 = assert_udata(far.FindPlugin("PFM_MODULENAME", mod))
  local hnd2 = assert_udata(far.FindPlugin("PFM_SYSID", luamacroId))
  assert_eq(hnd1:rawhandle(), hnd2:rawhandle())

  assert_nil(far.LoadPlugin("PLT_PATH", mod:sub(1,-2)))
  hnd2 = assert_udata(far.LoadPlugin("PLT_PATH", mod))
  assert_eq(hnd1:rawhandle(), hnd2:rawhandle())

  local info = far.GetPluginInformation(hnd1)
  assert_table(info)
  assert_table(info.GInfo)
  assert_table(info.PInfo)
  assert_eq(mod, info.ModuleName)
  assert_num(info.Flags)
  assert(0 ~= band(info.Flags, F.FPF_LOADED))
  assert(0 == band(info.Flags, F.FPF_ANSI))

  local pluglist = far.GetPlugins()
  assert_table(pluglist)
  assert(#pluglist >= 1)
  for _,plug in ipairs(pluglist) do
    assert_udata(plug)
  end

  hnd1 = far.FindPlugin("PFM_SYSID", hlfviewerId)
  if hnd1 then
    local info = far.GetPluginInformation(hnd1)
    assert_table(info)
    assert_str(info.ModuleName)
    assert_true  (far.UnloadPlugin(hnd1))
    assert_nil   (far.FindPlugin("PFM_SYSID", hlfviewerId))
    assert_udata (far.LoadPlugin("PLT_PATH", info.ModuleName))
    assert_udata (far.FindPlugin("PFM_SYSID", hlfviewerId))
--  assert_false (far.IsPluginLoaded(hlfviewerId)) --> fails if a plugin's cache was empty before the test
    assert_udata (far.ForcedLoadPlugin("PLT_PATH", info.ModuleName))
    assert_true  (far.IsPluginLoaded(hlfviewerId))
  end

  assert_true(far.IsPluginLoaded(luamacroId))
  assert_false(far.IsPluginLoaded(luamacroId + 1))
  assert_false(far.IsPluginLoaded(0))

  assert_func(far.ClearPluginCache)
  assert_func(far.ForcedLoadPlugin)
  assert_func(far.UnloadPlugin)
end

local function test_far_timer()
  if far.Timer == nil then -- TODO (FreeBSD, DragonFly BSD)
    return
  end
  local N = 0
  local timer = far.Timer(50, function(hnd)
      N = N+1
      if N==3 then hnd:Close() end
    end)
  while not timer.Closed do
    win.Sleep(5) -- prevents "saturation" and hanging of Far
    Keys("foobar")
  end
  assert_eq (N, 3)
end

local function test_win_functions()
  assert_func( win.CopyFile)
  assert_func( win.CreateDir)
  assert_func( win.DeleteFile)
  assert_func( win.EnsureColorsAreInverted)
  assert_func( win.ExtractKey)
  assert_func( win.ExtractKeyEx)
  assert_func( win.FileTimeToLocalFileTime)
  assert_func( win.FileTimeToSystemTime)
  assert_func( win.GetConsoleScreenBufferInfo)
  assert_func( win.GetSystemTimeAsFileTime)
  assert_func( win.GetVirtualKeys)
  assert_func( win.IsProcess64bit)
  assert_func( win.lenW)
  assert_func( win.MoveFile)
  assert_func( win.MultiByteToWideChar)
  assert_func( win.OemToUtf8)
  assert_func( win.RemoveDir)
  assert_func( win.RenameFile)
  assert_func( win.SetCurrentDir)
  assert_func( win.SetEnv)
  assert_func( win.SetFileAttr)
  assert_func( win.Sleep)
  assert_func( win.subW)
  assert_func( win.system)
  assert_func( win.SystemTimeToFileTime)
  assert_func( win.Utf8ToOem)
  assert_func( win.Utf8ToUtf32)
  assert_func( win.wcscmp)
  assert_func( win.WideCharToMultiByte)
end

local function test_win_Clock()
  -- check time difference
  local temp = win.Clock()
  win.Sleep(500)
  temp = (win.Clock() - temp)
  assert(temp > 0.480 and temp < 2.000, temp)
  -- check granularity
  local OK = false
  temp = math.floor(win.Clock()*1e6) % 10
  for _=1,10 do
    win.Sleep(20)
    local temp2 = math.floor(win.Clock()*1e6) % 10
    if temp ~= temp2 then OK=true; break; end
  end
  assert_true(OK)
end

local function test_win_CompareString()
  assert(win.CompareString("a","b") < 0)
  assert(win.CompareString("b","a") > 0)
  assert(win.CompareString("b","b") == 0)
end

local function test_win_ExpandEnv()
  local s1 = "$(HOME)/abc"
  local s2 = win.ExpandEnv(s1)
  assert_neq(s1, s2)
  assert_num(s2:find("abc$"))

  local s3 = "$(HOME-HOME-HOME)/abc"
  local s4 = win.ExpandEnv(s3)
  assert_eq(s3, s4)
  local s5 = win.ExpandEnv(s3, true)
  assert_eq(s5, "/abc")
end

local function test_win_Uuid()
  local uuid = win.Uuid()
  assert_eq(#uuid, 16)
  assert_neq(uuid, ("\0"):rep(16))

  local u1 = win.Uuid(uuid)
  assert_eq(#u1, 36)
  assert_eq(win.Uuid(u1), uuid)

  local u2 = win.Uuid("L")
  assert_eq(#u2, 36)
  assert_eq(u2, u2:match("^[0-9a-f%-]+$"))

  local u3 = win.Uuid("U")
  assert_eq(#u3, 36)
  assert_eq(u3, u3:match("^[0-9A-F%-]+$"))
end

local function test_win()
  test_win_functions()

  test_win_Clock()
  test_win_CompareString()
  test_win_ExpandEnv()
  test_win_Uuid()

  assert_table (win.EnumSystemCodePages())
  assert_num   (win.GetACP())
  assert_str   (win.GetCurrentDir())
  assert_num   (win.GetOEMCP())

  local dir  = assert_str (win.GetEnv("FARHOME"))
  local attr = assert_str (win.GetFileAttr(dir))
  assert_num(attr:find("d"))
  assert_table (win.GetFileInfo(dir))

  assert_table (win.GetCPInfo(65001))
  assert_table (win.GetLocalTime())
  assert_table (win.GetSystemTime())
end

local function test_utf8()
  test_utf8_len()
  test_utf8_sub()
  test_utf8_lower_upper()
end

local function test_dialog_1()
  local sd = require"far2.simpledialog"
  local tt = {"foo","bar","baz"}

  local Items =  {
    guid="7C5407A5-7F41-47AD-9F7E-CBD78E60438C";
    data = tt[1];

    {tp="dbox"; val="Text";    },
    {tp="text"; val="Text1";   },
    {tp="edit"; val="Text20";  },
    {tp="butt"; val="Text300"; },
  }
  Items.proc = function(hDlg, Msg, Par1, Par2)
    if Msg == F.DN_INITDIALOG then
      assert(Par1==3 and Par2==Items.data)

      assert_eq(hDlg:send("DM_GETDLGDATA"), Par2)
      assert_eq(hDlg:send("DM_SETDLGDATA", tt[2]), tt[1])
      assert_eq(hDlg:send("DM_GETDLGDATA"), tt[2])

      local info = assert_table(hDlg:send("DM_GETDIALOGINFO"))
      assert_eq(info.Id, win.Uuid(Items.guid))
      assert_eq(info.Owner, luamacroId)

      for k,item in ipairs(Items) do
        assert_eq(hDlg:send("DM_GETTEXT", k), item.val)
        local newtext = "---" .. item.val
        local ref = newtext:len()
        if item.tp == "butt" then ref = ref+4 end
        assert_eq(ref, hDlg:send("DM_SETTEXT", k, newtext))
      end

      hDlg:send("DM_CLOSE")
      Items.data = tt[3]
    end
  end

  sd.New(Items):Run()
  assert(Items.data == tt[3])
end

local function test_dialog_issue_28()
  local sd=require "far2.simpledialog"
  local checkValue = nil
  local items = {
    width=30;
    {tp="dbox"; text="Issue #28"},
    {tp="butt"; btnnoclose=1; text="CRASH"; centergroup=1; },
  }
  items.proc=function(hDlg,msg,p1,p2)
    if msg==F.DN_BTNCLICK then
      hDlg:Close()
      far.Message("hello") -- USED TO CAUSE CRASH
      checkValue = true
      return 1
    end
  end
  local Dlg = sd.New(items)
  mf.acall(Dlg.Run, Dlg)
  Keys("Enter")
  assert_true(Area.Dialog)
  Keys("Esc")
  assert_true(Area.Shell)
  assert_true(checkValue)
end

local function test_dialog()
  test_dialog_1()
  test_dialog_issue_28()
end

local function test_one_guid(val, func, keys, numEsc)
  numEsc = numEsc or 1
  val = far.Guids[val]
  assert_str(val)
  assert_eq(#val, 36)

  if func then func() end
  if keys then Keys(keys) end

  assert_eq(Area.Dialog and Dlg.Id or Menu.Id, val)
  for _ = 1,numEsc do Keys("Esc") end
end

local function test_Guids()
  assert_true(Far.GetConfig("Confirmations.Delete"), "Confirmations.Delete must be set")
  assert_true(Far.GetConfig("Confirmations.DeleteFolder"), "Confirmations.DeleteFolder must be set")

  assert_table(far.Guids)

  Keys("Esc"); print("lm:farconfig"); Keys("Enter")
  test_one_guid( "AdvancedConfigId")

  test_one_guid( "ApplyCommandId",           nil, "CtrlG")
  test_one_guid( "AskInsertMenuOrCommandId", nil, "F2 Ins", 2)
  test_one_guid( "ChangeDiskMenuId",         nil, "AltF1")
  test_one_guid( "ChangeDiskMenuId",         nil, "AltF2")
  test_one_guid( "CodePagesMenuId",          nil, "F9 Home 3*Right Enter End 4*Up Enter")

  local was_empty = APanel.Empty
  if was_empty then
    Keys("F7 1 Enter")
    assert_false(APanel.Empty)
  end
  test_one_guid( "CopyCurrentOnlyFileId",    nil, "End ShiftF5")
  test_one_guid( "CopyFilesId",              nil, "End F5")
  local keyname = Far.GetConfig("System.DeleteToRecycleBin") and "DeleteRecycleId" or "DeleteFileFolderId"
  test_one_guid( keyname,                    nil, "End F8")
  test_one_guid( "DeleteWipeId",             nil, "End AltDel")
  test_one_guid( "DescribeFileId",           nil, "End CtrlZ")
  test_one_guid( "FileAttrDlgId",            nil, "End CtrlA")
  test_one_guid( "HardSymLinkId",            nil, "End AltF6")
  test_one_guid( "MoveCurrentOnlyFileId",    nil, "End ShiftF6")
  test_one_guid( "MoveFilesId",              nil, "End F6")
  if was_empty then
    assert_eq(APanel.Current, "1")
    Keys("F8")
    if Area.Dialog then Keys("Enter") end
    assert_true(APanel.Empty)
  end

  test_one_guid( "EditUserMenuId",           nil, "F2 Ins Enter", 2)
  test_one_guid( "EditorReplaceId",          nil, "ShiftF4 Del Enter CtrlF7", 2)
  test_one_guid( "EditorSearchId",           nil, "ShiftF4 Del Enter F7", 2)
  test_one_guid( "EditorCanNotEditDirectoryId", nil, "ShiftF4 . . Enter", 1)
  test_one_guid( "SelectFromEditHistoryId",  nil, "ShiftF4 A Enter Esc ShiftF4 CtrlDown", 2)
  test_one_guid( "FarAskQuitId",             nil, "F10")

  local myMenu
  myMenu = function() mf.mainmenu("fileassociations") end
  test_one_guid( "FileAssocMenuId",          myMenu)
  test_one_guid( "FileAssocModifyId",        myMenu, "Ins", 2)

  myMenu = function() mf.mainmenu("foldershortcuts") end
  test_one_guid( "FolderShortcutsId",        myMenu)
  test_one_guid( "FolderShortcutsDlgId",     myMenu, "F4", 2)

  myMenu = function() mf.mainmenu("filehighlight") end
  test_one_guid( "HighlightMenuId",          myMenu)
  test_one_guid( "HighlightConfigId",        myMenu, "Ins", 2)

  myMenu = function() mf.mainmenu("filepanelmodes") end
  test_one_guid( "PanelViewModesId",         myMenu)
  test_one_guid( "PanelViewModesEditId",     myMenu, "Enter", 2)

  myMenu = function() mf.mainmenu("filemaskgroups") end
  test_one_guid( "MaskGroupsMenuId",         myMenu)
  test_one_guid( "EditMaskGroupId",          myMenu, "Ins", 2)

  test_one_guid( "FileOpenCreateId",         nil, "ShiftF4")
  test_one_guid( "FileSaveAsId",             nil, "ShiftF4 Del Enter ShiftF2", 2)
  test_one_guid( "FiltersConfigId",          nil, "CtrlI Ins", 2)
  test_one_guid( "FiltersMenuId",            nil, "CtrlI")
  test_one_guid( "FindFileId",               nil, "AltF7")
  test_one_guid( "HelpSearchId",             nil, "F1 F7", 2)
  test_one_guid( "HistoryCmdId",             nil, "AltF8")
  test_one_guid( "HistoryEditViewId",        nil, "AltF11")
  test_one_guid( "HistoryFolderId",          nil, "AltF12")
  test_one_guid( "MakeFolderId",             nil, "F7")
  test_one_guid( "PluginInformationId",      nil, "F11 F3", 2)
  test_one_guid( "PluginsConfigMenuId",      nil, "AltShiftF9")
  test_one_guid( "PluginsMenuId",            nil, "F11")
  test_one_guid( "ScreensSwitchId",          nil, "F12")
  test_one_guid( "SelectDialogId",           nil, "Add")
  test_one_guid( "SelectSortModeId",         nil, "CtrlF12")
  test_one_guid( "UnSelectDialogId",         nil, "Subtract")

  Plugin.Command(luamacroId, "view:$FARHOME/FarEng.lng")
  assert_true(Area.Viewer)
  test_one_guid( "ViewerSearchId",           nil, "F7", 2)

  -- test_one_guid( "BadEditorCodePageId", nil, "")
  -- test_one_guid( "CannotRecycleFileId", nil, "")
  -- test_one_guid( "CannotRecycleFolderId", nil, "")
  -- test_one_guid( "ChangeDriveCannotReadDiskErrorId", nil, "")
  -- test_one_guid( "ChangeDriveModeId", nil, "")
  -- test_one_guid( "CopyOverwriteId", nil, "")
  -- test_one_guid( "CopyReadOnlyId", nil, "")
  -- test_one_guid( "DeleteAskDeleteROId", nil, "")
  -- test_one_guid( "DeleteAskWipeROId", nil, "")
  -- test_one_guid( "DeleteFolderId", nil, "")
  -- test_one_guid( "DeleteFolderRecycleId", nil, "")
  -- test_one_guid( "DeleteLinkId", nil, "")
  -- test_one_guid( "DisconnectDriveId", nil, "")
  -- test_one_guid( "EditAskSaveExtId", nil, "")
  -- test_one_guid( "EditAskSaveId", nil, "")
  -- test_one_guid( "EditorAskOverwriteId", nil, "")
  -- test_one_guid( "EditorConfirmReplaceId", nil, "")
  -- test_one_guid( "EditorFileGetSizeErrorId", nil, "")
  -- test_one_guid( "EditorFileLongId", nil, "")
  -- test_one_guid( "EditorFindAllListId", nil, "")
  -- test_one_guid( "EditorOpenRSHId", nil, "")
  -- test_one_guid( "EditorReloadId", nil, "")
  -- test_one_guid( "EditorReloadModalId", nil, "")
  -- test_one_guid( "EditorSavedROId", nil, "")
  -- test_one_guid( "EditorSaveExitDeletedId", nil, "")
  -- test_one_guid( "EditorSaveF6DeletedId", nil, "")
  -- test_one_guid( "EditorSwitchUnicodeCPDisabledId", nil, "")
  -- test_one_guid( "EjectHotPlugMediaErrorId", nil, "")
  -- test_one_guid( "FindFileResultId", nil, "")
  -- test_one_guid( "FolderShortcutsMoreId", nil, "")
  -- test_one_guid( "GetNameAndPasswordId", nil, "")
  -- test_one_guid( "RecycleFolderConfirmDeleteLinkId", nil, "")
  -- test_one_guid( "RemoteDisconnectDriveError1Id", nil, "")
  -- test_one_guid( "RemoteDisconnectDriveError2Id", nil, "")
  -- test_one_guid( "SelectAssocMenuId", nil, "")
  -- test_one_guid( "SUBSTDisconnectDriveError1Id", nil, "")
  -- test_one_guid( "SUBSTDisconnectDriveError2Id", nil, "")
  -- test_one_guid( "SUBSTDisconnectDriveId", nil, "")
  -- test_one_guid( "UserMenuUserInputId", nil, "")
  -- test_one_guid( "VHDDisconnectDriveErrorId", nil, "")
  -- test_one_guid( "VHDDisconnectDriveId", nil, "")
  -- test_one_guid( "WipeFolderId", nil, "")
  -- test_one_guid( "WipeHardLinkId", nil, "")
end

local function test_far_Menu1()
  local items = { { text="line1" }, { text="line2" } }

  local bkeys1 = {
    {BreakKey="F1"}, {BreakKey="F2"},
    {BreakKey="1" }, {BreakKey="2" },
    {BreakKey="A" }, {BreakKey="B" },
  }

  local bkeys2 = "F1 F2 1 2 A B"

  for _, BK in ipairs { bkeys1,bkeys2 } do
    for _, item  in ipairs(bkeys1) do
      mf.acall(far.Menu, {}, items, BK)
      assert(Area.Menu)
      Keys(item.BreakKey)
      assert(Area.Shell)
    end
  end
end

local function test_far_Menu2()
  -- test far.Menu and Menu.GetValue
  local items = {"foo", "bar", "baz"}
  mf.acall(far.Menu, {}, items)
  assert_true(Area.Menu)
  for j=1,#items do
    assert_eq(Menu.GetValue(j), items[j])
  end
  for j=1,#items do
    assert_eq(Menu.GetValue(0), items[j])
    Keys("Down")
  end
  assert_eq(Menu.GetValue(-1), "")
  assert_eq(Menu.GetValue(#items+1), "")
  Keys("Esc")
  assert_true(Area.Shell)
end

local function test_far_Menu3()
  -- test callback
  local tPos, tKeys, Ret
  local v1, v2 = 57, "foo"
  local items = { { text="line1" }, { text="line2" }, { text="line3" } }

  local function callback(pos, key, data1, data2)
    table.insert(tPos, pos)
    table.insert(tKeys, key)
    assert_eq(data1, v1)
    assert_eq(data2, v2)
    return Ret
  end

  mf.acall(far.Menu, {}, items, nil, callback, v1, v2)
  assert_true(Area.Menu)

  local keys = { "End", "CtrlF", "AltY", "Home", "CtrlShiftF1" } -- key names
  local poses = { 1, #items, #items, #items, 1 } -- positions
  local rets = { 0, false, nil } -- return values that shouldn't affect anything
  for i=1,3 do
    Ret = rets[i]        -- set callback return value
    tPos, tKeys = {}, {} -- clear callback collectors
    for _,v in ipairs(keys)  do Keys(v) end -- run test
    for k,v in ipairs(poses) do assert_eq(v, tPos[k]) end  -- check results
    for k,v in ipairs(keys)  do assert_eq(v, tKeys[k]) end -- check results
  end

  -- test callback return ==1 ("don't process the key")
  Ret = 1
  Keys("Esc Enter")
  assert_true(Area.Menu)

  -- test callback return ==-1 ("cancel the menu")
  Ret = -1
  Keys("F1")
  assert_true(Area.Shell)

  -- test callback return ==2 ("close the menu as if Enter was pressed")
  mf.acall(far.Menu, {}, items, nil, callback, v1, v2)
  assert_true(Area.Menu)
  Ret = 2
  Keys("F1")
  assert_true(Area.Shell)
end

local function test_far_MakeMenuItems()
  local t = assert_table (far.MakeMenuItems("foo","bar","baz"))
  assert_eq(#t, 3)
end

local function test_far_Menu()
  test_far_Menu1()
  test_far_Menu2()
  test_far_Menu3()
  test_far_MakeMenuItems()
end

local function test_SplitCmdLine()
  local a,b,c,d

  a,b,c,d = far.SplitCmdLine("ab cd ef gh")
  assert(a=="ab" and b=="cd" and c=="ef" and d=="gh")

  a,b,c,d = far.SplitCmdLine("  ab  cd\\ ef gh  ")
  assert(a=="ab" and b=="cd ef" and c=="gh" and d==nil)

  a,b,c,d = far.SplitCmdLine [["ab  cd"  ef "gh  "]]
  assert(a=="ab  cd" and b=="ef" and c=="gh  " and d==nil)

  a,b,c,d = far.SplitCmdLine [[-r""]]
  assert(a==[[-r""]] and b==nil and c==nil and d==nil)

  a,b,c,d = far.SplitCmdLine [[-e  "far.Show(4 + 7)"]]
  assert(a=="-e" and b=="far.Show(4 + 7)" and c==nil and d==nil)
end

local function test_far_SaveScreen()
  local scr = assert_udata(far.SaveScreen(0,0,20,10))
  far.FreeScreen(scr)
  assert_func(far.RestoreScreen)
end

local function test_far_GetDirList()
  local dir = os.getenv("FARHOME") .. "/Plugins/luafar/luamacro"
  local list = assert_table(far.GetDirList(dir))
  local item = assert_table(list[1])
  assert_str(item.FileName)
end

local function test_far_GetPluginDirList()
  Plugin.Command(far.GetPluginId(), "macro:panel 4")
  assert_true(APanel.Plugin)
  local items = assert_table(far.GetPluginDirList(1, "."))
  assert_eq(#items, 4)
  assert_table(items[1])
  Keys("Home Enter")
  far.Text()
end

local function test_far_ShowHelp()
  mf.postmacro(function() assert_eq(Area.Current,"Help"); Keys("Esc"); end)
  assert_true(far.ShowHelp(nil, nil, "FHELP_FARHELP FHELP_USECONTENTS"))
  assert_false(far.ShowHelp())
end

function MT.test_luafar()
  test_AdvControl()
  test_bit64()
  test_dialog()
  test_far_GetDirList()
  test_far_GetPluginDirList()
  test_far_GetMsg()
  test_far_Menu()
  test_far_SaveScreen()
  test_far_ShowHelp()
  test_FarStandardFunctions()
  test_far_timer()
  test_gmatch_coro()
  test_Guids()
  test_issue_3129()
  test_MacroControl()
  test_PluginsControl()
  test_RegexControl()
  test_SplitCmdLine()
  test_utf8()
  test_win()
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

function MT.test_far_regex(printfunc, verbose)
  local test = require "far2.test.regex.runtest"
  local numerr = test(printfunc, verbose)
  assert_eq (numerr, 0)
end

function MT.test_far_DetectCodePage()
  local test = require "far2.test.codepage.test_codepage"
  local dir = os.getenv("FARHOME").."/Plugins/luafar/lua_share/far2/test/codepage"
  local pass, total = test(dir)
  assert_num(pass)
  assert_num(total)
  assert(pass > 0 and pass == total)
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

local function test_Editor_Sel_Cmdline(args)
  for _, str in ipairs(args) do
    assert_true(Area.Shell)
    panel.SetCmdLine(nil, str)

    local act = 0
    for opt = 0,4 do
      assert_eq(Editor.Sel(act,opt), 0) -- no block exists, zeros returned
    end

    local line, pos, w, h = 1, 13, 5, 1

    panel.SetCmdLineSelection(nil, pos, pos+w)
    assert_eq(Editor.Sel(act,0), line)
    assert_eq(Editor.Sel(act,1), pos)
    assert_eq(Editor.Sel(act,2), line)
    assert_eq(Editor.Sel(act,3), pos + w)
    assert_eq(Editor.Sel(act,4), 1)
    --------------------------------------------------------------------------------------------------
    act = 1
    panel.SetCmdLineSelection(nil, pos, pos+w)

    assert_eq(1, Editor.Sel(act, 0)) -- set cursor at block start
    assert_eq(panel.GetCmdLinePos(), pos)

    assert_eq(1, Editor.Sel(act, 1)) -- set cursor next to block end
    assert_eq(panel.GetCmdLinePos(), pos + w + 1)
    --------------------------------------------------------------------------------------------------
    for act = 2,3 do
      panel.SetCmdLinePos(nil,pos)             -- set block start
      assert_eq(1, Editor.Sel(act,0))          -- +++
      panel.SetCmdLinePos(nil,pos + w)         -- set block end (it also selects the block)
      assert_eq(1, Editor.Sel(act,1))          -- +++

      assert_eq(Editor.Sel(0,0), 1)
      assert_eq(Editor.Sel(0,1), pos)
      assert_eq(Editor.Sel(0,2), 1)
      assert_eq(Editor.Sel(0,3), pos + w - 1)
      assert_eq(Editor.Sel(0,4), 1)
    end
    --------------------------------------------------------------------------------------------------
    assert_eq(1, Editor.Sel(4)) -- reset the block
    assert_eq(Editor.Sel(0,4), 0)

    panel.SetCmdLine(nil, "")
  end
end

local function test_Editor_Sel_Dialog(args)
  Keys("F7 Del")
  assert_true(Area.Dialog)
  local inf = actl.GetWindowInfo()
  local hDlg = assert_udata(inf.Id)
  local EditPos = assert_num(hDlg:GetFocus())

  for _, str in ipairs(args) do
    hDlg:SetText(EditPos, str)

    local act = 0
    for opt = 0,4 do
      assert_eq(Editor.Sel(act,opt), 0) -- no block exists, zeros returned
    end

    local tSel = { BlockType = F.BTYPE_STREAM; BlockStartLine = 1; BlockStartPos = 13,
                   BlockWidth = 5; BlockHeight = 1; }

    hDlg:SetSelection(EditPos, tSel)
    assert_eq(Editor.Sel(act,0), tSel.BlockStartLine)
    assert_eq(Editor.Sel(act,1), tSel.BlockStartPos)
    assert_eq(Editor.Sel(act,2), tSel.BlockStartLine)
    assert_eq(Editor.Sel(act,3), tSel.BlockStartPos + tSel.BlockWidth - 1) -- [-1: DIFFERENT FROM EDITOR]
    assert_eq(Editor.Sel(act,4), tSel.BlockType)
    --------------------------------------------------------------------------------------------------
    act = 1
    hDlg:SetSelection(EditPos, tSel)

    assert_eq(1, Editor.Sel(act, 0)) -- set cursor at block start
    local pos = assert_table(hDlg:GetCursorPos(EditPos))
    assert_eq(pos.Y+1, tSel.BlockStartLine)
    assert_eq(pos.X+1, tSel.BlockStartPos)

    assert_eq(1, Editor.Sel(act, 1)) -- set cursor next to block end
    pos = assert_table(hDlg:GetCursorPos(EditPos))
    assert_eq(pos.Y+1, tSel.BlockStartLine)
    assert_eq(pos.X+1, tSel.BlockStartPos + tSel.BlockWidth)
    --------------------------------------------------------------------------------------------------
    for act = 2,3 do
      hDlg:SetCursorPos(EditPos, {X=10; Y=0})  -- set block start
      assert_eq(1, Editor.Sel(act,0))          -- +++
      hDlg:SetCursorPos(EditPos, {X=20; Y=0})  -- set block end (it also selects the block)
      assert_eq(1, Editor.Sel(act,1))          -- +++

      assert_eq(Editor.Sel(0,0), 1)
      assert_eq(Editor.Sel(0,1), 10+1)
      assert_eq(Editor.Sel(0,2), 1)
      assert_eq(Editor.Sel(0,3), 20)
      assert_eq(Editor.Sel(0,4), 1)
    end
    --------------------------------------------------------------------------------------------------
    assert_eq(1, Editor.Sel(4)) -- reset the block
    assert_eq(Editor.Sel(0,4), 0)
  end

  Keys("Esc")
end

local function test_Editor_Sel_Editor(args)
  local R2T, T2R = editor.RealToTab, editor.TabToReal
  local T2R2T = function(id, y, x)
    return R2T(id, y, T2R(id, y, x)) -- needed when the column position is "inside" a tab
  end

  Keys("ShiftF4 Del Enter")
  assert_true(Area.Editor)

  for _, str in ipairs(args) do
    Keys("CtrlA Del")

    for k=1,8 do
      editor.InsertString()
      editor.SetString(nil, k, str)
    end

    local act = 0
    for opt = 0,4 do
      assert_eq(Editor.Sel(act,opt), 0) -- no block exists, zeros returned
    end

    local line, pos, w, h = 3, 13, 5, 4

    for ii=1,2 do
      local typ = ii==1 and "BTYPE_STREAM" or "BTYPE_COLUMN"
      local ref_x1 = (ii==1 and R2T or T2R2T)(nil, line, pos)
      local ref_x2 = (ii==1 and R2T or T2R2T)(nil, line-1+h, pos+w)

      assert_true(editor.Select(nil, typ, line, pos, w, h)) -- select the block
      assert_eq(Editor.Sel(act,0), line)      -- get block start line
      assert_eq(Editor.Sel(act,1), ref_x1)    -- get block start pos
      assert_eq(Editor.Sel(act,2), line-1+h)  -- get block end line
      assert_eq(Editor.Sel(act,3), ref_x2)    -- get block end pos
      assert_eq(Editor.Sel(act,4), ii)        -- get block type
    end
    --------------------------------------------------------------------------------------------------
    act = 1
    for ii=1,2 do
      local typ = ii==1 and "BTYPE_STREAM" or "BTYPE_COLUMN"
      local ref_x1 = ii==1 and pos or T2R(nil, line, pos) or pos
      local ref_x2 = ii==1 and pos+w or T2R(nil, line-1+h, pos+w)

      local inf
      assert_true(editor.Select(nil, typ, line, pos, w, h)) -- select the block

      assert_eq(1, Editor.Sel(act, 0)) -- set cursor at block start
      inf = editor.GetInfo()
      assert_eq(inf.CurLine, line)
      assert_eq(inf.CurPos, ref_x1)

      assert_eq(1, Editor.Sel(act, 1)) -- set cursor next to block end
      inf = editor.GetInfo()
      assert_eq(inf.CurLine, line-1+h)
      assert_eq(inf.CurPos, ref_x2)
    end
    --------------------------------------------------------------------------------------------------
    local y1,x1,y2,x2 = 2,10,6,20
    for act = 2,3 do
      editor.SetPosition(nil,y1,x1)    -- set block start
      assert_eq(1, Editor.Sel(act,0))  -- +++
      editor.SetPosition(nil,y2,x2)    -- set block end (it also selects the block)
      assert_eq(1, Editor.Sel(act,1))  -- +++

      local ref_x1 = (act==2 and R2T or T2R2T)(nil,y1,x1)
      local ref_x2 = (act==2 and R2T or T2R2T)(nil,y2,x2)

      assert_eq(Editor.Sel(0,0), y1)                 -- get block start line
      assert_eq(Editor.Sel(0,1), ref_x1)             -- get block start pos
      assert_eq(Editor.Sel(0,2), y2)                 -- get block end line
      assert_eq(Editor.Sel(0,3), ref_x2)             -- get block end pos
      assert_eq(Editor.Sel(0,4), act==2 and 1 or 2)  -- get block type
    end
    --------------------------------------------------------------------------------------------------
    assert_eq(1, Editor.Sel(4)) -- reset the block
    assert_eq(Editor.Sel(0,4), 0)
  end

  assert_true(editor.Quit())
end

local function test_Editor_Misc()
  local fname = far.MkTemp()
  local flags = {EF_NONMODAL=1, EF_IMMEDIATERETURN=1, EF_DISABLEHISTORY=1, EF_DELETEONCLOSE=1}
  assert_eq(editor.Editor(fname,nil,nil,nil,nil,nil,flags), F.EEC_MODIFIED)
  assert_true(Area.Editor)

  local EI = assert_table(editor.GetInfo())

  local str = ("123456789-"):rep(4)
  local str2, num

  -- test Editor.Value
  editor.SetString(nil,1,str)
  assert_eq(Editor.Value, str)

  -- test insertion with overtype=OFF
  editor.SetString(nil,1,str)
  editor.SetPosition(nil,1,1)
  editor.InsertText(nil, "AB")
  assert_eq(Editor.Value, "AB"..str)

  -- test insertion with overtype=ON
  editor.SetString(nil,1,str)
  Keys("Ins")
  editor.SetPosition(nil,1,1)
  editor.InsertText(nil, "CD")
  assert_eq(Editor.Value, "CD"..str:sub(3))
  Keys("Ins")

  -- test insertion beyond EOL (overtype=ON then OFF)
  num = 20
  assert_true(editor.SetParam(nil, "ESPT_CURSORBEYONDEOL", true))
  str2 = str .. (" "):rep(num) .. "AB"
  for _=1,2 do
    Keys("Ins")
    editor.SetString(nil,1,str)
    editor.SetPosition(nil, 1, #str + 1 + num)
    editor.InsertText(nil, "AB")
    assert_eq(Editor.Value, str2)
  end
  assert_true(editor.SetParam(nil, "ESPT_CURSORBEYONDEOL", band(EI.Options, F.EOPT_CURSORBEYONDEOL) ~= 0))

  editor.Quit()
end

function MT.test_Editor()
  local args = {
    ("123456789-"):rep(4),     -- plain ASCII
    ("12\t4\t6789-"):rep(4),   -- includes tabs
    ("12🔥4🔥6789-"):rep(4),   -- includes 🔥 (a multi-byte double-width character)
    ("1Ю23456789-"):rep(4),    -- insert 'Ю' (a multi-byte character) into position 2
    ("1Ю2\t4\t6789-"):rep(4),  -- ditto
    ("1Ю2🔥4🔥6789-"):rep(4),  -- ditto
  }
  test_Editor_Sel_Cmdline(args)
  test_Editor_Sel_Dialog(args)
  test_Editor_Sel_Editor(args)
  test_Editor_Misc()
end

function MT.test_all()
  assert(Area.Shell, "Run these tests from the Shell area.")
  assert(not APanel.Plugin and not PPanel.Plugin, "Run these tests when neither of panels is a plugin panel.")

  MT.test_areas()
  MT.test_mf()
  MT.test_CmdLine()
  MT.test_Help()
  MT.test_Dlg()
  MT.test_Drv()
  MT.test_Editor()
  MT.test_Far()
  MT.test_Menu()
  MT.test_Mouse()
  MT.test_Object()
  MT.test_Panel()
  MT.test_Plugin()
  MT.test_APanel()
  MT.test_PPanel()
  MT.test_mantis_1722()
  MT.test_luafar()
  MT.test_coroutine()
  MT.test_UserDefinedList()
  MT.test_far_regex( --[[far.Log, true]] ) -- external test files
  MT.test_far_DetectCodePage() -- external
  actl.RedrawAll()
end

return MT
