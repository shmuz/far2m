-- Goal: strip comments and redundant whitespace from Lua code
-- The Lua code to be stripped is assumed to be Lua 5.1
-- This script is written in Lua 5.1
-- Started by Shmuel Zeigerman, 2008-08-22

local rex = require "rex_pcre"


local patt1 do
  local nl = "(\n+)"
  local ws = "[^\\S\n]+"  -- whitespace other than newline
  local str1 = [["(?:\\.|[^"])*"?]]
  local str2 = [['(?:\\.|[^'])*'?]]
  local str = "("..str1.."|"..str2..")"
  local lstr = "\\[(=*)\\["
  local lcmt = "--\\[(=*)\\["
  local cmt = "--.*"
  patt1 = rex.new(ws.."|"..nl.."|"..str.."|"..lstr.."|"..lcmt.."|"..cmt)
end


local function worker (opt, infile, outfile, handles)
  assert(type(opt)=="string" and opt:match("^[fs][fs]%a*"), "strip: arg #1 invalid")
  assert(type(infile)=="string", "strip: arg #2: string expected")
  local inobj, outobj, outfunc
  inobj = (opt:sub(1,1) == "f") and assert(io.open(infile)) or infile
  handles.inobj = inobj
  if opt:sub(2,2) == "f" then
    assert(type(outfile)=="string", "strip: arg #3: string expected")
    outobj = assert(io.open(outfile, "wb"))
    handles.outobj, outfunc = outobj, outobj.write
  else
    outobj, outfunc = {}, table.insert
  end
  local keeplines = opt:sub(3,3) == "k"

  local subject = (type(inobj) == "string") and inobj or inobj:read("*all")
  local prev, start = false, 1
  local function needspace ()
    if not prev then return false end
    local s = subject:sub(prev,prev) .. subject:sub(start,start)
    return s == "--" or s:find("[%w_][%w_]")
  end
  while true do
    -- search for either of: newlines, whitespace, string, comment
    local s, e, nl, str, lstr, lcmt = patt1:find(subject, start)
    if not s then
      if start <= #subject then
        if needspace() then outfunc(outobj, " ") end
        outfunc(outobj, subject:sub(start))
      end
      break
    end
    if s > start then
      if needspace() then outfunc(outobj, " ") end
      outfunc(outobj, subject:sub(start, s - 1))
      prev = s - 1
    end
    if nl then
      if keeplines then
        outfunc(outobj, nl)
        prev = false
      end
    elseif str then
      outfunc(outobj, str)
      prev = false
    elseif lstr then
      outfunc(outobj, "["..lstr.."[")
      start = e + 1
      s,e = rex.find(subject, "]"..lstr.."]", start)
      assert(s, "long string not closed")
      outfunc(outobj, subject:sub(start, e))
      prev = false
    elseif lcmt then
      start = e + 1
      s,e = rex.find(subject, "]"..lcmt.."]", start)
      assert(s, "long comment not closed")
      if keeplines then
        local nl = subject:sub(start,s-1):gsub("[^\n]+", "")
        if nl ~= "" then
          outfunc(outobj, nl)
          prev = false
        end
      end
    end
    start = e + 1
  end
  return type(outobj) == "table" and table.concat(outobj) or true
end


-- @param opt: a string;
--     1-st letter is 's' if <infile> is a string; 'f' if it is a file name
--     2-nd letter: same as the 1-st letter but for the output
--     3-nd letter (optional): 'k' (keep line feeds, don't strip)
-- @param infile: a string
--     either string to be stripped or filename
-- @param outfile:
--     name of output file (not needed if output is a string)
-- @returns:
--     a stripped source (if the output is a string)
--     true (if the output is a file)
--     nil followed by error message (if error occurred)
local function strip (opt, infile, outfile)
  local handles = {}
  local ok, result = pcall(worker, opt, infile, outfile, handles)
  for _, v in pairs(handles) do
    if io.type(v) == "file" then v:close() end
  end
  if ok then return result end
  return ok, result
end


return strip
