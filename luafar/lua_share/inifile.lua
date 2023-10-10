-- started : 2020-12-10
-- author  : Shmuel Zeigerman
-- action  : INI-file parser

local lib, section = {}, {}
local mt_lib     = {__index=lib}
local mt_section = {__index=section}

local function get_secname(line)
  return line:match("^%s*%[%s*([^%s%]].-)%s*%]")
end

local function get_key_val(line)
 return line:match("^%s*([^=%s][^=]-)%s*=%s*(.-)%s*$")
end

-- @param fname     : name of ini-file
-- @param nocomment : if true then values can contain any chars including semicolons
--                    if false then a semicolon means start of a comment
function lib.New(fname, nocomment)
  local fp, msg
  for k=1,2 do
    fp, msg = io.open(fname)
    if fp then
      break
    elseif k==1 then
      fp = io.open(fname, "w")
      if fp then fp:close() end
    else
      return nil, msg
    end
  end

  local self = { map={}; }
  setmetatable(self, mt_lib)
  local cursection
  for line in fp:lines() do
    if line:find("^%s*[^;%s]") then -- not comment or empty line
      local secname = get_secname(line)
      if secname then
        cursection = self:add_section(secname) -- this allows a section to appear multiple times in the file
      elseif cursection then
        local key,val = get_key_val(line)
        if key then
          local first = val:sub(1,1)
          if first == '"' then
            val = val:match('^"(.*)"') -- in double quotes
          elseif first == "'" then
            val = val:match("^'(.*)'") -- in single quotes
          else
            if not nocomment then
              val = val:match("^[^;]+")  -- semicolon starts the comment
            end
            val = val and val:gsub("%s+$","")
          end
          if val then
            cursection:set(key,val)
          end
        end
      end
    end
  end

  fp:close()
  return self
end


function lib:add_section(name)
  local lname = name:lower()
  local sec = self.map[lname]
  if not sec then
    sec = { name=name; lname=lname; map={}; }
    setmetatable(sec, mt_section)
    self.map[lname] = sec
  end
  return sec
end


function lib:sections()
  local k,v
  return function()
    k,v = next(self.map, k)
    return v
  end
end

function lib:get_section(name)
  return self.map[name:lower()]
end

function lib:del_section(name)
  self.map[name:lower()] = nil
end

function lib:ren_section(name, newname)
  local sec = self.map[name:lower()]
  if sec then
    self.map[name:lower()] = nil
    self.map[newname:lower()] = sec
    sec.name = newname
    sec.lname = newname:lower()
    -- sec.map stays unchanged
  end
end

function lib:clear()
  self.map = {}
end

function section:clear()
  self.map = {}
end

function section:set(key,val)
  local lkey = key:lower()
  self.map[lkey] = { name=key; lname=lkey; val=val; }
end

function section:records()
  local k,v
  return function()
    k,v = next(self.map, k)
    if k then return v.name, v.val end
    return nil
  end
end

function section:write(fp, sortmethod)
  sortmethod = sortmethod or function(a,b) return a.lname < b.lname end
  local arr = {}
  for _,rec in pairs(self.map) do table.insert(arr, rec) end
  table.sort(arr, sortmethod)

  fp:write("[", self.name, "]\n")
  for _,rec in ipairs(arr) do
    fp:write(rec.name, "=", rec.val, "\n")
  end
end

function section:dict()
  local t = {}
  for _,rec in pairs(self.map) do t[rec.name]=rec.val end
  return t
end

function lib:write(fname, sortmethod)
  local fp = io.open(fname, "w")
  if not fp then return nil end

  sortmethod = sortmethod or function(a,b) return a.lname < b.lname end
  local arr = {}
  for sec in self:sections() do table.insert(arr, sec) end
  table.sort(arr, sortmethod)

  for _,sec in ipairs(arr) do
    sec:write(fp)
    fp:write("\n")
  end
  fp:close()
end

function lib:GetString(sec, key)
  local sect = self.map[sec:lower()]
  if sect then
    local item = sect.map[key:lower()]
    return item and item.val
  end
end

function lib:GetNumber(sec, key)
  return tonumber(self:GetString(sec,key))
end

function lib:GetBoolean(sec, key)
  local s = (self:GetString(sec,key) or ""):lower()
  if s=="1" or s=="yes" or s=="true" then return true end
  if s=="0" or s=="no" or s=="false" then return false end
  return nil
end

return lib
