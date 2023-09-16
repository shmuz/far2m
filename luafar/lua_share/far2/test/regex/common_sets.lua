-- See Copyright Notice in the file LICENSE

-- This file should contain only test sets that behave identically
-- when being run with pcre or posix regex libraries.

local luatest = require ("far2.test.regex.luatest")
local N = luatest.NT
local L = win.Utf8ToUtf32

local function norm(a) return a==nil and N or a end

local function set_f_gmatch (lib, flg)
  -- gmatch (s, p, [cf], [ef])
  local function test_gmatch (subj, patt)
    local out, guard = {}, 10
    for a, b in lib.gmatch (subj, patt) do
      table.insert (out, { norm(a), norm(b) })
      guard = guard - 1
      if guard == 0 then break end
    end
    return unpack (out)
  end
  return {
    Name = "Function gmatch",
    Func = test_gmatch,
  --{  subj             patt         results }
    { {"ab",            "."  }, {{"a",N}, {"b",N} } },
    { {("abcd"):rep(3), "(.)b.(d)"}, {{"a","d"},{"a","d"},{"a","d"}} },
    { {"abcd",          ".*" },      {{"abcd",N},{"",N} } },
    { {"abc",           "^." },      {{"a",N}} },--anchored pattern
  }
end

local function set_f_gmatchW (lib, flg)
  -- gmatch (s, p, [cf], [ef])
  local function test_gmatchW (subj, patt)
    local out, guard = {}, 10
    for a, b in lib.gmatchW (subj, patt) do
      table.insert (out, { norm(a), norm(b) })
      guard = guard - 1
      if guard == 0 then break end
    end
    return unpack (out)
  end
  return {
    Name = "Function gmatchW",
    Func = test_gmatchW,
  --{  subj             patt         results }
    { {L"ab",            "."  }, {{L"a",N}, {L"b",N} } },
    { {L("abcd"):rep(3), "(.)b.(d)"}, {{L"a",L"d"},{L"a",L"d"},{L"a",L"d"}} },
    { {L"abcd",          ".*" },      {{L"abcd",N},{L"",N} } },
    { {L"abc",           "^." },      {{L"a",N}} },--anchored pattern
  }
end

local function set_f_find (lib, flg)
  return {
    Name = "Function find",
    Func = lib.find,
  --  {subj, patt, st},         { results }
    { {"abcd", ".+"},           { 1,4 }   },      -- [none]
    { {"abcd", ".+", 2},        { 2,4 }   },      -- positive st
    { {"abcd", ".+", -2},       { 3,4 }   },      -- negative st
    { {"abcd", ".*"},           { 1,4 }   },      -- [none]
    { {"abc",  "bc"},           { 2,3 }   },      -- [none]
    { {"abcd", "(.)b.(d)"},     { 1,4,"a","d" }}, -- [captures]
    { {"abc\r\nd", "/$/m"},     { 4,3 } },        -- [Mantis 0002124]
    { {"CtrlShiftG", "^(R?Ctrl)?(R?Alt)?(Shift)?(.*)$"}, {1,10,"Ctrl",false,"Shift","G"} }, -- [Mantis 0002242]
  }
end

local function set_f_findW (lib, flg)
  return {
    Name = "Function findW",
    Func = lib.findW,
  --  {subj, patt, st},         { results }
    { {L"abcd", ".+"},           { 1,4 }   },      -- [none]
    { {L"abcd", ".+", 2},        { 2,4 }   },      -- positive st
    { {L"abcd", ".+", -2},       { 3,4 }   },      -- negative st
    { {L"abcd", ".*"},           { 1,4 }   },      -- [none]
    { {L"abc",  "bc"},           { 2,3 }   },      -- [none]
    { {L"abcd", "(.)b.(d)"},     { 1,4,L"a",L"d" }}, -- [captures]
    { {L"abc\r\nd", "/$/m"},     { 4,3 } },        -- [Mantis 0002124]
  }
end

local function set_f_match (lib, flg)
  return {
    Name = "Function match",
    Func = lib.match,
  --  {subj, patt, st},         { results }
    { {"abcd", ".+"},           {"abcd"}  }, -- [none]
    { {"abcd", ".+", 2},        {"bcd"}   }, -- positive st
    { {"abcd", ".+", -2},       {"cd"}    }, -- negative st
    { {"abcd", ".*"},           {"abcd"}  }, -- [none]
    { {"abc",  "bc"},           {"bc"}    }, -- [none]
    { {"abcd", "(.)b.(d)"},     {"a","d"} }, -- [captures]
    { {"abc\r\nd", "/$/m"},     { "" } },    -- [Mantis 0002124]
    { {"CtrlShiftG", "^(R?Ctrl)?(R?Alt)?(Shift)?(.*)$"}, {"Ctrl",false,"Shift","G"} }, -- [Mantis 0002242]
  }
end

local function set_f_matchW (lib, flg)
  return {
    Name = "Function matchW",
    Func = lib.matchW,
  --  {subj, patt, st},         { results }
    { {L"abcd", ".+"},           {L"abcd"}  }, -- [none]
    { {L"abcd", ".+", 2},        {L"bcd"}   }, -- positive st
    { {L"abcd", ".+", -2},       {L"cd"}    }, -- negative st
    { {L"abcd", ".*"},           {L"abcd"}  }, -- [none]
    { {L"abc",  "bc"},           {L"bc"}    }, -- [none]
    { {L"abcd", "(.)b.(d)"},     {L"a",L"d"} }, -- [captures]
    { {L"abc\r\nd", "/$/m"},     { L"" } },     -- [Mantis 0002124]
  }
end

local function set_m_find (lib, flg)
  return {
    Name = "Method find",
    Method = "find",
  --{patt},                 {subj, st}           { results }
    { {".+"},               {"abcd"},            {1,4}  }, -- [none]
    { {".+"},               {"abcd",2},          {2,4}  }, -- positive st
    { {".+"},               {"abcd",-2},         {3,4}  }, -- negative st
    { {".*"},               {"abcd"},            {1,4}  }, -- [none]
    { {"bc"},               {"abc"},             {2,3}  }, -- [none]
    { {"(.)b.(d)"},         {"abcd"},            {1,4,"a","d"}},--[captures]
    { {"/$/m"},             { "abc\r\nd"},       {4,3}},   -- [Mantis 0002124]
  }
end

local function set_m_findW (lib, flg)
  return {
    Name = "Method findW",
    Method = "findW",
  --{patt},                 {subj, st}           { results }
    { {".+"},               {L"abcd"},            {1,4}  }, -- [none]
    { {".+"},               {L"abcd",2},          {2,4}  }, -- positive st
    { {".+"},               {L"abcd",-2},         {3,4}  }, -- negative st
    { {".*"},               {L"abcd"},            {1,4}  }, -- [none]
    { {"bc"},               {L"abc"},             {2,3}  }, -- [none]
    { {"(.)b.(d)"},         {L"abcd"},            {1,4,L"a",L"d"}},--[captures]
    { {"/$/m"},             {L"abc\r\nd"},        {4,3}},   -- [Mantis 0002124]
  }
end

local function set_m_match (lib, flg)
  return {
    Name = "Method match",
    Method = "match",
  --{patt},                 {subj, st}           { results }
    { {".+"},               {"abcd"},            {"abcd"}  }, -- [none]
    { {".+"},               {"abcd",2},          {"bcd" }  }, -- positive st
    { {".+"},               {"abcd",-2},         {"cd"  }  }, -- negative st
    { {".*"},               {"abcd"},            {"abcd"}  }, -- [none]
    { {"bc"},               {"abc"},             {"bc"  }  }, -- [none]
    { {"(.)b.(d)"},         {"abcd"},            {"a","d"} }, --[captures]
    { {"/$/m"},             {"abc\r\nd"},        {""}},       -- [Mantis 0002124]
  }
end

local function set_m_matchW (lib, flg)
  return {
    Name = "Method matchW",
    Method = "matchW",
  --{patt},                 {subj, st}           { results }
    { {".+"},               {L"abcd"},            {L"abcd"}  }, -- [none]
    { {".+"},               {L"abcd",2},          {L"bcd" }  }, -- positive st
    { {".+"},               {L"abcd",-2},         {L"cd"  }  }, -- negative st
    { {".*"},               {L"abcd"},            {L"abcd"}  }, -- [none]
    { {"bc"},               {L"abc"},             {L"bc"  }  }, -- [none]
    { {"(.)b.(d)"},         {L"abcd"},            {L"a",L"d"} }, --[captures]
    { {"/$/m"},             {L"abc\r\nd"},        {L""}},       -- [Mantis 0002124]
  }
end

local function set_f_gsub1 (lib, flg)
  local subj, pat = "abcdef", "[abef]+"
  --local cpat = lib.new(pat)
  return {
    Name = "Function gsub, set1",
    Func = lib.gsub,
  --{ s,       p,    f,   n,    res1,  res2, res3 },
----    { {subj,  cpat,  "",  0},   {subj,    0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  "",  0},   {subj,    0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  "", -1},   {subj,    0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  "",  1},   {"cdef",  1, 1} },
    { {subj,   pat,  "",  2},   {"cd",    2, 2} },
    { {subj,   pat,  "",  3},   {"cd",    2, 2} },
    { {subj,   pat,  ""    },   {"cd",    2, 2} },
    { {subj,   pat,  "#", 0},   {subj,    0, 0} }, -- test "n" + non-empty_replace
    { {subj,   pat,  "#", 1},   {"#cdef", 1, 1} },
    { {subj,   pat,  "#", 2},   {"#cd#",  2, 2} },
    { {subj,   pat,  "#", 3},   {"#cd#",  2, 2} },
    { {subj,   pat,  "#"   },   {"#cd#",  2, 2} },
    { {"abc",  "^.", "#"   },   {"#bc",   1, 1} }, -- anchored pattern
  }
end

local function set_f_gsubW1 (lib, flg)
  local subj, pat = L"abcdef", "[abef]+"
  --local cpat = lib.new(pat)
  return {
    Name = "Function gsubW, set1",
    Func = lib.gsubW,
  --{ s,       p,    f,   n,    res1,  res2, res3 },
----    { {subj,  cpat,  "",  0},   {subj,    0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  "",  0},   {subj,    0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  "", -1},   {subj,    0, 0} }, -- test "n" + empty_replace
    { {subj,   pat,  "",  1},   {L"cdef",  1, 1} },
    { {subj,   pat,  "",  2},   {L"cd",    2, 2} },
    { {subj,   pat,  "",  3},   {L"cd",    2, 2} },
    { {subj,   pat,  ""    },   {L"cd",    2, 2} },
    { {subj,   pat,  "#", 0},   {subj,    0, 0} }, -- test "n" + non-empty_replace
    { {subj,   pat,  "#", 1},   {L"#cdef", 1, 1} },
    { {subj,   pat,  "#", 2},   {L"#cd#",  2, 2} },
    { {subj,   pat,  "#", 3},   {L"#cd#",  2, 2} },
    { {subj,   pat,  "#"   },   {L"#cd#",  2, 2} },
    { {L"abc",  "^.", "#"   },   {L"#bc",   1, 1} }, -- anchored pattern
  }
end

local function set_f_gsub2 (lib, flg)
  local subj, pat = "abc", "([ac])"
  return {
    Name = "Function gsub, set2",
    Func = lib.gsub,
  --{ s,     p,   f,   n,     res1,    res2, res3 },
    { {subj, pat, "<%1>" },   {"<a>b<c>", 2, 2} }, -- test non-escaped chars in f
    { {subj, pat, "%<%1%>" }, {"<a>b<c>", 2, 2} }, -- test escaped chars in f
    { {subj, pat, "" },       {"b",       2, 2} }, -- test empty replace
    { {subj, pat, "1" },      {"1b1",     2, 2} }, -- test odd and even %'s in f
    { {subj, pat, "%1" },     {"abc",     2, 2} },
    { {subj, pat, "%%1" },    {"%1b%1",   2, 2} },
    { {subj, pat, "%%%1" },   {"%ab%c",   2, 2} },
    { {subj, pat, "%%%%1" },  {"%%1b%%1", 2, 2} },
    { {subj, pat, "%%%%%1" }, {"%%ab%%c", 2, 2} },
  }
end

local function set_f_gsubW2 (lib, flg)
  local subj, pat = L"abc", "([ac])"
  return {
    Name = "Function gsubW, set2",
    Func = lib.gsubW,
  --{ s,     p,   f,   n,     res1,    res2, res3 },
    { {subj, pat, "<%1>" },   {L"<a>b<c>", 2, 2} }, -- test non-escaped chars in f
    { {subj, pat, "%<%1%>" }, {L"<a>b<c>", 2, 2} }, -- test escaped chars in f
    { {subj, pat, "" },       {L"b",       2, 2} }, -- test empty replace
    { {subj, pat, "1" },      {L"1b1",     2, 2} }, -- test odd and even %'s in f
    { {subj, pat, "%1" },     {L"abc",     2, 2} },
    { {subj, pat, "%%1" },    {L"%1b%1",   2, 2} },
    { {subj, pat, "%%%1" },   {L"%ab%c",   2, 2} },
    { {subj, pat, "%%%%1" },  {L"%%1b%%1", 2, 2} },
    { {subj, pat, "%%%%%1" }, {L"%%ab%%c", 2, 2} },
  }
end

local function set_f_gsub3 (lib, flg)
  return {
    Name = "Function gsub, set3",
    Func = lib.gsub,
  --{ s,      p,      f,  n,   res1,res2,res3 },
    { {"abc", "a",    "%0" }, {"abc", 1, 1} }, -- test (in)valid capture index
    { {"abc", "a",    "%1" }, {"abc", 1, 1} },
    { {"abc", "[ac]", "%1" }, {"abc", 2, 2} },
    { {"abc", "(a)",  "%1" }, {"abc", 1, 1} },
    { {"abc", "(a)",  "%2" }, "invalid capture index" },
  }
end

local function set_f_gsubW3 (lib, flg)
  return {
    Name = "Function gsubW, set3",
    Func = lib.gsubW,
  --{ s,      p,      f,  n,   res1,res2,res3 },
    { {L"abc", "a",    "%0" }, {L"abc", 1, 1} }, -- test (in)valid capture index
    { {L"abc", "a",    "%1" }, {L"abc", 1, 1} },
    { {L"abc", "[ac]", "%1" }, {L"abc", 2, 2} },
    { {L"abc", "(a)",  "%1" }, {L"abc", 1, 1} },
    { {L"abc", "(a)",  "%2" }, "invalid capture index" },
  }
end

local function set_f_gsub4 (lib, flg)
  return {
    Name = "Function gsub, set4",
    Func = lib.gsub,
  --{ s,           p,              f, n,  res1,      res2, res3 },
    { {"a2c3",     ".",            "#" }, {"####",      4, 4} }, -- test .
    { {"a2c3",     ".+",           "#" }, {"#",         1, 1} }, -- test .+
    { {"a2c3",     ".*",           "#" }, {"#",         1, 1} }, -- test .*
    { {"/* */ */", "\\/\\*(.*)\\*\\/", "#" }, {"#",     1, 1} },
    { {"a2c3",     "[0-9]",        "#" }, {"a#c#",      2, 2} }, -- test %d
    { {"a2c3",     "[^0-9]",       "#" }, {"#2#3",      2, 2} }, -- test %D
    { {"a \t\nb",  "[ \t\n]",      "#" }, {"a###b",     3, 3} }, -- test %s
    { {"a \t\nb",  "[^ \t\n]",     "#" }, {"# \t\n#",   2, 2} }, -- test %S
  }
end

local function set_f_gsubW4 (lib, flg)
  return {
    Name = "Function gsubW, set4",
    Func = lib.gsubW,
  --{ s,           p,              f, n,  res1,      res2, res3 },
    { {L"a2c3",     ".",            "#" }, {L"####",      4, 4} }, -- test .
    { {L"a2c3",     ".+",           "#" }, {L"#",         1, 1} }, -- test .+
    { {L"a2c3",     ".*",           "#" }, {L"#",         1, 1} }, -- test .*
    { {L"/* */ */", "\\/\\*(.*)\\*\\/", "#" }, {L"#",     1, 1} },
    { {L"a2c3",     "[0-9]",        "#" }, {L"a#c#",      2, 2} }, -- test %d
    { {L"a2c3",     "[^0-9]",       "#" }, {L"#2#3",      2, 2} }, -- test %D
    { {L"a \t\nb",  "[ \t\n]",      "#" }, {L"a###b",     3, 3} }, -- test %s
    { {L"a \t\nb",  "[^ \t\n]",     "#" }, {L"# \t\n#",   2, 2} }, -- test %S
  }
end

local function set_f_gsub5 (lib, flg)
  local function frep1 () end                       -- returns nothing
  local function frep2 () return "#" end            -- ignores arguments
  local function frep3 (...) return table.concat({...}, ",") end -- "normal"
  local function frep4 () return {} end             -- invalid return type
  local function frep5 () return "7", "a" end       -- 2-nd return is "a"
  local function frep6 () return "7", "break" end   -- 2-nd return is "break"
  local subj = "a2c3"
  return {
    Name = "Function gsub, set5",
    Func = lib.gsub,
  --{ s,     p,          f,   n,   res1,     res2, res3 },
    { {subj, "a(.)c(.)", frep1 }, {subj,        1, 0} },
    { {subj, "a(.)c(.)", frep2 }, {"#",         1, 1} },
    { {subj, "a(.)c(.)", frep3 }, {"2,3",       1, 1} },
    { {subj, "a.c.",     frep3 }, {subj,        1, 1} },
    { {subj, "z*",       frep1 }, {subj,        5, 0} },
    { {subj, "z*",       frep2 }, {"#a#2#c#3#", 5, 5} },
    { {subj, "z*",       frep3 }, {subj,        5, 5} },
    { {subj, subj,       frep4 }, "invalid return type" },
    { {"abc",".",        frep5 }, {"777",       3, 3} },
    { {"abc",".",        frep6 }, {"777",       3, 3} },
  }
end

local function set_f_gsubW5 (lib, flg)
  local function frep1 () end                       -- returns nothing
  local function frep2 () return L"#" end            -- ignores arguments
  local function frep3 (...) return table.concat({...}, L",") end -- "normal"
  local function frep4 () return {} end             -- invalid return type
  local function frep5 () return L"7", L"a" end       -- 2-nd return is "a"
  local function frep6 () return L"7", L"break" end   -- 2-nd return is "break"
  local subj = L"a2c3"
  return {
    Name = "Function gsubW, set5",
    Func = lib.gsubW,
  --{ s,     p,          f,   n,   res1,     res2, res3 },
    { {subj, "a(.)c(.)", frep1 }, {subj,        1, 0} },
    { {subj, "a(.)c(.)", frep2 }, {L"#",         1, 1} },
    { {subj, "a(.)c(.)", frep3 }, {L"2,3",       1, 1} },
    { {subj, "a.c.",     frep3 }, {subj,        1, 1} },
    { {subj, "z*",       frep1 }, {subj,        5, 0} },
    { {subj, "z*",       frep2 }, {L"#a#2#c#3#", 5, 5} },
    { {subj, "z*",       frep3 }, {subj,        5, 5} },
    { {subj, subj,       frep4 }, "invalid return type" },
    { {L"abc",".",        frep5 }, {L"777",       3, 3} },
    { {L"abc",".",        frep6 }, {L"777",       3, 3} },
  }
end

local function set_f_gsub6 (lib, flg)
  local tab1, tab2, tab3 = {}, { ["2"] = 56 }, { ["2"] = {} }
  local subj = "a2c3"
  return {
    Name = "Function gsub, set6",
    Func = lib.gsub,
  --{ s,     p,          f, n,   res1,res2,res3 },
    { {subj, "a(.)c(.)", tab1 }, {subj,  1, 0} },
    { {subj, "a(.)c(.)", tab2 }, {"56",  1, 1} },
    { {subj, "a(.)c(.)", tab3 }, "invalid replacement type" },
    { {subj, "a.c.",     tab1 }, {subj,  1, 0} },
    { {subj, "a.c.",     tab2 }, {subj,  1, 0} },
    { {subj, "a.c.",     tab3 }, {subj,  1, 0} },
  }
end

local function set_f_gsubW6 (lib, flg)
  local tab1, tab2, tab3 = {}, { [L"2"] = 56 }, { [L"2"] = {} }
  local subj = L"a2c3"
  return {
    Name = "Function gsubW, set6",
    Func = lib.gsubW,
  --{ s,     p,          f, n,   res1,res2,res3 },
    { {subj, "a(.)c(.)", tab1 }, {subj,  1, 0} },
    { {subj, "a(.)c(.)", tab2 }, {"56",  1, 1} },
    { {subj, "a(.)c(.)", tab3 }, "invalid replacement type" },
    { {subj, "a.c.",     tab1 }, {subj,  1, 0} },
    { {subj, "a.c.",     tab2 }, {subj,  1, 0} },
    { {subj, "a.c.",     tab3 }, {subj,  1, 0} },
  }
end

return function (lib)
  return {
    set_f_gmatch  (lib),
    set_f_gmatchW (lib),

    set_f_find    (lib),
    set_f_findW   (lib),

    set_f_match   (lib),
    set_f_matchW  (lib),

    set_m_find    (lib),
    set_m_findW   (lib),

    set_m_match   (lib),
    set_m_matchW  (lib),

    set_f_gsub1   (lib),
    set_f_gsubW1  (lib),

    set_f_gsub2   (lib),
    set_f_gsubW2  (lib),

    set_f_gsub3   (lib),
    set_f_gsubW3  (lib),

    set_f_gsub4   (lib),
    set_f_gsubW4  (lib),

    set_f_gsub5   (lib),
    set_f_gsubW5  (lib),

    set_f_gsub6   (lib),
    set_f_gsubW6  (lib),
  }
end
