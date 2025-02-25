-- farlang.lua

local HppFile, InFile, OutPath, Verbose

local function ProcessCmdLine(...)
  local params = {...}
  local i = 1
  while i <= #params do
    if params[i] == "--hppfile" or params[i] == "-h" then
      HppFile = params[i+1]
      i = i + 2
    elseif params[i] == "--inputfile" or params[i] == "-i" then
      InFile = params[i+1]
      i = i + 2
    elseif params[i] == "--outdir" or params[i] == "-o" then
      OutPath = params[i+1]
      i = i + 2
    elseif params[i] == "--verbose" or params[i] == "-v" then
      Verbose = true
      i = i + 1
    else
      error("invalid command line argument #"..tostring(i))
    end
  end
  assert(HppFile, "command line: missing HPP file")
  assert(OutPath, "command line: missing output path")
end

local function ReadInputFile()
  local Langs = {}
  local Records = {}
  local IdMap = {}
  local TotalLangs
  local stHPP, stLANGNUM, stLANGDATA, stENUM, stRECORDS = 1,2,3,4,5
  local State = stHPP
  local linenum = 0
  local currecord
  local langindex = 0

  local fp = InFile and assert(io.open(InFile)) or io.stdin

  local MyAssert = function(cond, msg)
    if cond then return cond end
    if fp ~= io.stdin then fp:close() end
    msg = ("%s: [line %d]"):format(msg or "Error", linenum)
    error(msg)
  end

  local MyError = function(msg) MyAssert(false, msg) end

  local AppendLine = function(txt, txt2) return txt and txt.."\n"..txt2 or txt2; end

  for line in fp:lines() do
    linenum = linenum + 1
    line = line:match("(.-)%s*$") -- strip trailing space

    if line == "" or line:match("^%s*#") then
      State = State -- skip empty lines and comments

    elseif State == stHPP then
      State = stLANGNUM -- ignore: HppFile is taken from the command line

    elseif State == stLANGNUM then
      TotalLangs = MyAssert(tonumber(line), "invalid number of languages")
      MyAssert(TotalLangs > 0, "number of languages must be > 0")
      State = stLANGDATA

    elseif State == stLANGDATA then
      local fname, lngname, descr = line:match("(%S+)%s+(%S+)%s+\"(.+)\"$")
      MyAssert(fname, "invalid language description")
      Langs[#Langs+1] = { fname=fname; lngname=lngname; descr=descr; review=0; }
      if #Langs == TotalLangs then
        State = stENUM
      end

    elseif State == stENUM then
      if line:match("^%s*hhead:") then
        local txt = line:match("%s*hhead:(.*)")
        Langs.hhead = AppendLine(Langs.hhead, txt)

      elseif line:match("^%s*htail:") then
        local txt = line:match("%s*htail:(.*)")
        Langs.htail = AppendLine(Langs.htail, txt)

      elseif line:match("^%s*enum:") then
        State = stRECORDS -- do nothing as we don't define identifiers via enum

      else
        MyError("Unknown or out-of-order directive")

      end

    elseif State == stRECORDS then
      local AddEntry = function()
        local entry = line:match("^%s*(\".*\")") or line:match("^%s*(upd:\".*\")")
        MyAssert(entry, "invalid language entry")
        currecord[langindex] = entry
        if entry:match("^upd:") then
          Langs[langindex].review = Langs[langindex].review + 1
        end
        langindex = langindex + 1
        if langindex > TotalLangs then langindex = 0; end
      end

      if langindex == 0 then
        local id = MyAssert(line:match("^[_a-zA-Z][_a-zA-Z0-9]*$"), "invalid identifier")
        MyAssert(IdMap[id] == nil, "duplicate identifier")
        IdMap[id] = true
        currecord = { id = id }
        Records[#Records+1] = currecord
        langindex = langindex + 1

      elseif langindex == 1 then
        local prefix, comment = line:match("^(le?):(.*)")
        if prefix then
          MyAssert(comment=="" or comment:match("^%s*//"), "invalid comment")
          if prefix == "l" then
            currecord.before = AppendLine(currecord.before, comment)
          else -- if prefix == "le" then
            currecord.after = AppendLine(currecord.after, comment)
          end
        else
          AddEntry()
        end

      else
        AddEntry()

      end
    end
  end

  MyAssert(langindex == 0, "bad last record")

  if fp ~= io.stdin then fp:close() end
  return Langs, Records
end

local function WriteOutput(Langs, Records)
  -- write hpp file
  HppFile = HppFile:match("^/") and HppFile or OutPath.."/"..HppFile
  local fpHpp = assert(io.open(HppFile, "w"))
  if Langs.hhead then fpHpp:write(Langs.hhead, "\n") end
  for i,rec in ipairs(Records) do
    fpHpp:write(("DECLARE_FARLANGMSG(%s, %d)"):format(rec.id, i-1), "\n")
  end
  fpHpp:write("static const int MaxMsgId = ", tostring(#Records - 1), ";\n")
  if Langs.htail then fpHpp:write(Langs.htail, "\n") end
  fpHpp:close()

  -- write language files
  for langindex, data in ipairs(Langs) do
    if Verbose and data.review > 0 then
      io.write(("INFO: There are %d strings that require review in %s translation\n")
               :format(data.review, data.lngname))
    end

    local fp = assert(io.open(OutPath.."/"..data.fname, "w"))
    fp:write("\239\187\191") -- UTF-8 BOM
    fp:write((".Language=%s,%s"):format(data.lngname, data.descr), "\n\n")

    for _, rec in ipairs(Records) do
      if rec.before then fp:write(rec.before, "\n") end

      local entry = rec[langindex]:match("^upd:(.+)")
      if entry then
        fp:write("// need translation:", "\n")
      else
        entry = rec[langindex]
      end
      fp:write(("//[%s]"):format(rec.id), "\n")
      fp:write(entry, "\n")

      if rec.after then fp:write(rec.after, "\n") end
    end

    fp:close()
  end
end

ProcessCmdLine(...)
local Langs, Records = ReadInputFile()
WriteOutput(Langs, Records)
