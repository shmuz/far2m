-- farlang.lua

local function ProcessCmdLine(...)
  local OutPath, InFile
  local params = {...}
  for i = 1, #params, 2 do
    if params[i] == "--outdir" or params[i] == "-o" then
      OutPath = params[i+1]
    elseif params[i] == "--inputfile" or params[i] == "-i" then
      InFile = params[i+1]
    else
      error("invalid command line argument #"..tostring(i))
    end
  end
  assert(OutPath, "incorrect command line parameters")
  return OutPath, InFile
end

local function ReadInputFile(FileName)
  local Langs = {}
  local Records = {}
  local IdMap = {}
  local TotalLangs
  local stHPP, stLANGNUM, stLANGDATA, stENUM, stRECORDS = 1,2,3,4,5
  local State = stHPP
  local linenum = 0
  local currecord
  local langindex = 0

  local fp = FileName and assert(io.open(FileName)) or io.stdin

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
      Langs.hppfile = line:match("^%s*(.*)")
      State = stLANGNUM

    elseif State == stLANGNUM then
      TotalLangs = MyAssert(tonumber(line), "invalid number of languages")
      MyAssert(TotalLangs > 0, "number of languages must be > 0")
      State = stLANGDATA

    elseif State == stLANGDATA then
      local fname, lngname, descr = line:match("(%S+)%s+(%S+)%s+\"(.+)\"$")
      MyAssert(fname, "invalid language description")
      Langs[#Langs+1] = { fname=fname; lngname=lngname; descr=descr; }
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
      if langindex == 0 then
        local id = MyAssert(line:match("^[_a-zA-Z][_a-zA-Z0-9]*$"), "invalid identifier")
        MyAssert(IdMap[id] == nil, "duplicate identifier")
        IdMap[id] = true
        currecord = { id = id }
        Records[#Records+1] = currecord
        langindex = langindex + 1

      elseif langindex == 1 then
        local kind, comment = line:match("^(le?):(.*)")
        if kind then
          MyAssert(comment=="" or comment:match("^%s*//"), "invalid comment")
          if kind == "l" then
            currecord.before = AppendLine(currecord.before, comment)
          else -- if kind == "le" then
            currecord.after = AppendLine(currecord.after, comment)
          end
        else
          local entry = line:match("^%s*(\".*\")") or line:match("^%s*(upd:\".*\")")
          if entry then
            currecord[langindex] = entry
            langindex = langindex + 1
          else
            MyError("Unrecognized line syntax")
          end
        end

      else
        local langentry = line:match("^%s*(\".*\")") or line:match("^%s*(upd:\".*\")")
        MyAssert(langentry, "invalid item")
        currecord[langindex] = langentry
        langindex = langindex + 1
        if langindex > TotalLangs then langindex = 0; end

      end
    end
  end

  MyAssert(langindex == 0, "bad last record")

  if fp ~= io.stdin then fp:close() end
  return Langs, Records
end

local function WriteOutput(OutPath, Langs, Records)
  -- write hpp file
  local fpHpp = assert(io.open(OutPath.."/"..Langs.hppfile, "w"))
  if Langs.hhead then fpHpp:write(Langs.hhead, "\n") end
  for i,rec in ipairs(Records) do
    fpHpp:write(("DECLARE_FARLANGMSG(%s, %d)"):format(rec.id, i-1), "\n")
  end
  if Langs.htail then fpHpp:write(Langs.htail, "\n") end
  fpHpp:close()

  -- write language files
  for langindex, data in ipairs(Langs) do
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

local OutPath, InFile = ProcessCmdLine(...)
local Langs, Records = ReadInputFile(InFile)
WriteOutput(OutPath, Langs, Records)
