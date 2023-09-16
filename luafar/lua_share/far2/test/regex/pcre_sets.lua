-- See Copyright Notice in the file LICENSE

local luatest = require ("far2.test.regex.luatest")
local N = luatest.NT
local L = win.Utf8ToUtf32

local function norm(a) return a==nil and N or a end

local function set_f_find (lib, flg)
  return {
  Name = "Function find",
  Func = lib.find,
  --{subj,   patt,      st,cf,ef,lo},        { results }
  { {"abcd", ".+",      5},                  { N   } }, -- failing st
  { {"abcd", ".*?"},                         { 1,0 } }, -- non-greedy
  { {"abc",  "aBC",     N,"i"         },     { 1,3 } }, -- cf
}
end

local function set_f_findW (lib, flg)
  return {
  Name = "Function findW",
  Func = lib.findW,
  --{subj,   patt,      st,cf,ef,lo},        { results }
  { {L"abcd", ".+",      5},                  { N   } }, -- failing st
  { {L"abcd", ".*?"},                         { 1,0 } }, -- non-greedy
  { {L"abc",  "aBC",     N,"i"         },     { 1,3 } }, -- cf
}
end

local function set_f_match (lib, flg)
  return {
  Name = "Function match",
  Func = lib.match,
  --{subj,   patt,      st,cf,ef,lo},        { results }
  { {"abcd", ".+",      5},                  { N    }}, -- failing st
  { {"abcd", ".*?"},                         { ""   }}, -- non-greedy
----  { {"abc",  "aBC",     N,flg.CASELESS},     {"abc" }}, -- cf
  { {"abc",  "aBC",     N,"i"         },     {"abc" }}, -- cf
----  { {"abc",  "bc",      N,flg.ANCHORED},     { N    }}, -- cf
----  { {"abc",  "bc",      N,N,flg.ANCHORED},   { N    }}, -- ef
}
end

local function set_f_matchW (lib, flg)
  return {
  Name = "Function matchW",
  Func = lib.matchW,
  --{subj,   patt,      st,cf,ef,lo},        { results }
  { {L"abcd", ".+",      5},                  { N    }}, -- failing st
  { {L"abcd", ".*?"},                         { L""   }}, -- non-greedy
----  { {"abc",  "aBC",     N,flg.CASELESS},     {"abc" }}, -- cf
  { {L"abc",  "aBC",     N,"i"         },     {L"abc" }}, -- cf
----  { {"abc",  "bc",      N,flg.ANCHORED},     { N    }}, -- cf
----  { {"abc",  "bc",      N,N,flg.ANCHORED},   { N    }}, -- ef
}
end

local function set_f_gmatch (lib, flg)
  -- gmatch (s, p, [cf], [ef])
  local pCSV = "(^[^,]*)|,([^,]*)"
  local F = false
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
  --{  subj             patt   results }
    { {"a\0c",          "." }, {{"a",N},{"\0",N},{"c",N}} },--nuls in subj
    { {"",              pCSV}, {{"",F}} },
    { {"12",            pCSV}, {{"12",F}} },
----    { {",",             pCSV}, {{"", F},{F,""}} },
    { {"12,,45",        pCSV}, {{"12",F},{F,""},{F,"45"}} },
----    { {",,12,45,,ab,",  pCSV}, {{"",F},{F,""},{F,"12"},{F,"45"},{F,""},{F,"ab"},{F,""}} },
  }
end

local function set_f_gmatchW (lib, flg)
  -- gmatch (s, p, [cf], [ef])
  local pCSV = "(^[^,]*)|,([^,]*)"
  local F = false
  local function test_gmatch (subj, patt)
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
    Func = test_gmatch,
  --{  subj             patt   results }
    { {L"a\0c",          "." }, {{L"a",N},{L"\0",N},{L"c",N}} },--nuls in subj
    { {L"",              pCSV}, {{L"",F}} },
    { {L"12",            pCSV}, {{L"12",F}} },
----    { {",",             pCSV}, {{"", F},{F,""}} },
    { {L"12,,45",        pCSV}, {{L"12",F},{F,L""},{F,L"45"}} },
----    { {",,12,45,,ab,",  pCSV}, {{"",F},{F,""},{F,"12"},{F,"45"},{F,""},{F,"ab"},{F,""}} },
  }
end

return function (lib)
--  local flags = lib.flags ()
  local flags = nil
  return {
    set_f_match   (lib, flags),
    set_f_matchW  (lib, flags),

    set_f_find    (lib, flags),
    set_f_findW   (lib, flags),

    set_f_gmatch  (lib, flags),
    set_f_gmatchW (lib, flags),
  }
end
