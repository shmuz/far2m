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

local function Interpolate(line)
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
    line = Interpolate(line)
    Write(line)
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
    return (str:gsub("^%s+", ""))
  end

  local function rtrim(str)
    return (str:gsub("%s+$", ""))
  end

  -- Read the input file line by line
  for line in fp:lines() do
    line = rtrim(line)
    line = Interpolate(line)

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
        -- make an index item
        local text = line:gsub("^%$[ %^]#%s*(.+)%s*#", "%1")
        atopic[#atopic+1] = { text=ltrim(text); topic=topic; } -- will concatenate after sorting
        topicFound = false
      end
    end

    if line == "<%INDEX%>" then
      Write("   ~" .. Contents .. "~@Contents@")

      table.sort(atopic, function(a,b) return a.text < b.text; end)

      local ch = ""
      for _, item in ipairs(atopic) do
        local c1 = item.text:sub(1, 1)
        if c1:byte() >= 128 then  -- poor man unicode hack
          c1 = item.text:sub(1, 2)
        end
        if ch ~= c1 then
          ch = c1
          Write("") -- insert empty line
        end
        local str = ("   ~%s~%s@"):format(item.text, item.topic)
        Write(str)
      end
    end
  end

  fp:close()
end

if args[1]     == "--farversion" then FarVersion()
elseif args[1] == "--farlang"    then FarLang(args[2])
elseif args[1] == "--mkhlf"      then MakeHlf(args[2])
end
