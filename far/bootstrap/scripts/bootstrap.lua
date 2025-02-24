-- luacheck: globals
-- globals MAJOR, MINOR, PATCH, ARCH must be set from the command line
local args = {...}

local COPYRIGHTYEARS = "2016-" .. os.date("%Y")
local FULLVERSION = ("%s.%s.%s %s"):format(MAJOR, MINOR, PATCH, ARCH)
local FULLVERSIONNOBRACES = FULLVERSION:gsub("[()]", "")

local Interpolate_Table = {
  -- from globals
  ARCH = ARCH;
  MAJOR = MAJOR;
  MINOR = MINOR;
  PATCH = PATCH;
  -- calculated
  COPYRIGHTYEARS = COPYRIGHTYEARS;
  FULLVERSION = FULLVERSION;
  FULLVERSIONNOBRACES = FULLVERSIONNOBRACES;
}

local function Interpolate(fp, line)
  if line:match("^%s*m4_include") then -- remove m4 things
    line = fp:read()
    if not line then return nil end
  end
  return (line:gsub("%$%{([A-Z_]+)%}", Interpolate_Table))
end

local function Write(str)
  io.write(tostring(str), "\n")
end

local StrCopyright = [[
const char *Copyright =
  "FAR2M, version %s\n"
  "Copyright (C) 1996-2000 Eugene Roshal, "
  "Copyright (C) 2000-2016 Far Group, "
  "Copyright (C) %s Far People";
]]

local function FarVersion()
  Write(("const uint32_t FAR_VERSION = 0x10000 * %s + %s;"):format(MAJOR, MINOR))
  Write(("const char *FAR_BUILD = \"%s.%s.%s\";"):format(MAJOR, MINOR, PATCH))
  Write(StrCopyright:format(FULLVERSION, COPYRIGHTYEARS))
end

local function FarLang(FileName)
  local fp = assert(io.open(FileName))
  for line in fp:lines() do
    line = Interpolate(fp, line)
    if line then
      Write(line)
    else
      break
    end
  end
  fp:close()
end

local function MakeHlf(FileName)
  assert (FileName, "File name not specified")
  local fp = assert(io.open(FileName))

  local atopic = {}
  local topic = ""
  local topicFound = false
  local Contents = ""

  local function ltrim(str)
    return (str:gsub("^[ \t]+", ""))
  end

  local function rtrim(str)
    return (str:gsub("[ \t]+$", ""))
  end

  -- Read the input file line by line
  for line in fp:lines() do
    line = rtrim(line)

    line = Interpolate(fp, line)
    if not line then break end

    local _, NF = line:gsub("%S+", "") -- number of fields

    if line ~= "<%INDEX%>" then Write(line) end

    if line == "@Contents" then
      -- Get the next line
      local nextLine = fp:read()
      if not nextLine then break end
      -- Extract Contents (e.g. $^#File and archive manager#)
      Contents = nextLine:match("^%$[ %^]#(.+)#")
      Write(nextLine)
    end

    if NF == 1 and line:match("^@%S") and line ~= "@Contents" and line ~= "@Index" then
      topic = line
      topicFound = true
    elseif topicFound and NF > 1 then
      local first, second = line:match("(%S+)%s+(%S+)")
      if first ~= "$"
        ---- mkhlf.awk was in CP 866 encoding and thus filtered out only the English variants.
        ---- Let us not filter out these topics.
        -- or second == "#Warning:"
        -- or second == "#Error:"
        -- or second == "#Предупреждение:"
        -- or second == "#Ошибка:"
      then
        topicFound = false
      else
        -- Extract sorting modes
        local title = line:gsub("^%$[ %^]#(.+)#", "%1")
        atopic[#atopic+1] =  title .. "~" .. topic .. "@"
        topicFound = false
      end
    end

    if line == "<%INDEX%>" then
      Write("   ~" .. Contents .. "~@Contents@")

      table.sort(atopic)

      local ch = ""
      for _, tp in ipairs(atopic) do
        local c1 = tp:sub(1, 1)
        if c1:byte() >= 128 then  -- poor man unicode hack
         c1 = tp:sub(1, 2)
        end
        if ch ~= c1 then
          ch = c1
          Write("") -- insert empty line
        end
        Write("   ~" .. ltrim(tp))
      end
    end
  end

  fp:close()
end

if args[1]     == "--farversion" then FarVersion()
elseif args[1] == "--farlang"    then FarLang(args[2])
elseif args[1] == "--mkhlf"      then MakeHlf(args[2])
end
