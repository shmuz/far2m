-- See Copyright Notice in the file LICENSE

local L = win.Utf8ToUtf32

local function set_f_gsub1 (lib, flg)
  --local subj, pat = "abcdef", "[abef]+"
  return {
    Name = "Function gsub, set1",
    Func = lib.gsub,
  --{ s,       p,    f,   n,    res1,  res2, res3 },
    { {"a\0c", ".",  "#"   },   {"###",   3, 3} }, -- subj contains nuls
  }
end

local function set_f_gsubW1 (lib, flg)
  --local subj, pat = "abcdef", "[abef]+"
  return {
    Name = "Function gsubW, set1",
    Func = lib.gsubW,
  --{ s,       p,    f,   n,    res1,  res2, res3 },
    { {L"a\0c", ".",  "#"   },   {L"###",   3, 3} }, -- subj contains nuls
  }
end

local function set_f_gsub4 (lib, flg)
  local pCSV = "[^,]*"
  local fCSV = function (a,b) return "["..(a or b).."]" end
  local set = {
    Name = "Function gsub, set4",
    Func = lib.gsub,
  --{ s,           p,              f, n,  res1,      res2, res3 },
    { {"/* */ */", [[\/\*(.*)\*\/]], "#" }, {"#",         1, 1} },
    { {"a2c3",     ".*?",          "#" },   {"#a#2#c#3#", 5, 5} },
    { {"/**/",     [[\/\*(.*?)\*\/]], "#" }, {"#",         1, 1} },
    { {"/* */ */", [[\/\*(.*?)\*\/]], "#" }, {"# */",      1, 1} },
    { {"a2c3",     "\\d",          "#" }, {"a#c#",      2, 2} }, -- test %d
    { {"a2c3",     "\\D",          "#" }, {"#2#3",      2, 2} }, -- test %D
    { {"a \t\nb",  "\\s",          "#" }, {"a###b",     3, 3} }, -- test %s
    { {"a \t\nb",  "\\S",          "#" }, {"# \t\n#",   2, 2} }, -- test %S
    { {"abcd",     "\\b",          "%1"}, {"abcd",      2, 2} },
    { {"",                    pCSV,fCSV}, {"[]",        1, 1} },
    { {"123",                 pCSV,fCSV}, {"[123]",     1, 1} },
    { {",",                   pCSV,fCSV}, {"[],[]",     2, 2} },
    { {"123,,456",            pCSV,fCSV}, {"[123],[],[456]", 3, 3}},
    { {",,123,456,,abc,789,", pCSV,fCSV}, {"[],[],[123],[456],[],[abc],[789],[]", 8, 8}},
  }
  return set
end

local function set_f_gsubW4 (lib, flg)
  local pCSV = "[^,]*"
  local fCSV = function (a,b) return L"["..(a or b)..L"]" end
  local set = {
    Name = "Function gsubW, set4",
    Func = lib.gsubW,
  --{ s,           p,              f, n,  res1,      res2, res3 },
    { {L"/* */ */", [[\/\*(.*)\*\/]], "#" }, {L"#",         1, 1} },
    { {L"a2c3",     ".*?",          "#" },   {L"#a#2#c#3#", 5, 5} },
    { {L"/**/",     [[\/\*(.*?)\*\/]], "#" }, {L"#",         1, 1} },
    { {L"/* */ */", [[\/\*(.*?)\*\/]], "#" }, {L"# */",      1, 1} },
    { {L"a2c3",     "\\d",          "#" }, {L"a#c#",      2, 2} }, -- test %d
    { {L"a2c3",     "\\D",          "#" }, {L"#2#3",      2, 2} }, -- test %D
    { {L"a \t\nb",  "\\s",          "#" }, {L"a###b",     3, 3} }, -- test %s
    { {L"a \t\nb",  "\\S",          "#" }, {L"# \t\n#",   2, 2} }, -- test %S
    { {L"abcd",     "\\b",          "%1"}, {L"abcd",      2, 2} },
    { {L"",                    pCSV,fCSV}, {L"[]",        1, 1} },
    { {L"123",                 pCSV,fCSV}, {L"[123]",     1, 1} },
    { {L",",                   pCSV,fCSV}, {L"[],[]",     2, 2} },
    { {L"123,,456",            pCSV,fCSV}, {L"[123],[],[456]", 3, 3}},
    { {L",,123,456,,abc,789,", pCSV,fCSV}, {L"[],[],[123],[456],[],[abc],[789],[]", 8, 8}},
  }
  return set
end

return function (lib)
  local flags = lib.flags and lib.flags ()
  return {
    set_f_gsub1  (lib, flags),
    set_f_gsubW1 (lib, flags),

    set_f_gsub4  (lib, flags),
    set_f_gsubW4 (lib, flags),
  }
end

