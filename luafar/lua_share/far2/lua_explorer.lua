--[[
    Lua Explorer

    Explore Lua environment in your Far manager

    Author: Eugen Gez (EGez/http://forum.farmanager.com)
    updates, suggestions, etc.:
    http://forum.farmanager.com/viewtopic.php?f=15&t=7521


    BE CAREFUL:
        calling some functions could cause deadlocks!!!
        you will need to kill far process in such cases

    do not call functions like debug.debug() or io.* functions
    that read from stdin unless you know what you are doing


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
    ANY USE IS AT YOUR OWN RISK.

    Do what you want with this code, but please do not remove this comment
    and write your changes down.


    todo:
    incorrect title while inserting into arrays
    truncate too long keys/values
    module-like behavior

    changes:
    EGez - !!! NOT RELEASED YET !!!
    * correct Lua syntax in menu title FIXME
    * left aligned message boxes
    + immediately exit using Alt+X

    EGez 03.02.13 04:21:56 +0100 - v1.1.2
    * minor changes/corrections

    EGez 14.12.2012 22:04:01 +0100 - v1.1.1
    * quote strings using "%q" instead of own addslashes
    * minor changes/optimisations

    EGez 08.12.2012 13:34:02 +0100 - v1.1
    + edit/remove/insert objects (F4/Del/Ins)
    + show function info (F3/Shift+F3)
    + show/hide functions (Ctrl+F)
    + show metatable (Ctrl+M)
    + display numbers as hex and dec
    + history for input fields
    + help (F1)
    * minor changes/optimisations/refactoring

    EGez 27.11.2012 20:52:26 +0100 - v1.0
    first release
]]

local F = far.Flags
local uuid    = win.Uuid('7646f761-8954-42ca-9cfc-e3f98a1c54d3')
local MenuFlags = F.FMENU_SHOWAMPERSAND + F.FMENU_WRAPMODE + F.FMENU_CHANGECONSOLETITLE
local help    = [[

There are some keys available:

F1         Show this help
F3         Show some function info
Shift+F3   Show some function info (LuaJIT required)
F4         Edit selected object
Del        Delete selected object
Ins        Add an object to current table
Ctrl+F     Show/hide functions
Ctrl+M     Show metatable
Alt+X      Immediately exit
]]

-- forward declarations for functions
local process, deleteValue, editValue, insertValue

local omit = {}
local brkeys    = {
  {BreakKey = 'Alt+X', exit = true},

  {BreakKey = 'F3',    action = function(obj, key, kpath)
    if key ~= nil and type(obj[key]) == 'function' then
      process(debug.getinfo(obj[key]), 'debug.getinfo: ' .. kpath)
    end
  end},

  {BreakKey = 'F4',    action = function(obj, key, kpath, callback)
    return key ~= nil and editValue(obj, key, kpath, callback)
  end},

  {BreakKey = 'Ctrl+F',    action = function()
    omit['function'] = (not omit['function'])
  end},

  {BreakKey = 'Ctrl+M',    action = function(obj, key, kpath)
    return key ~= nil and process(debug.getmetatable(obj[key]), 'METATABLE: ' .. kpath)
  end},

  {BreakKey = 'DELETE',    action = function(obj, key, kpath, callback)
    return key ~= nil and deleteValue(obj, key, kpath, callback)
  end},

  {BreakKey = 'INSERT',    action = function(obj, key, kpath, callback)
    -- FIXME: Incorrect title while insertig into arrays (ends with [)
    insertValue(obj, kpath:sub(1, -(#tostring(key) + 2)), callback)
  end},

  {BreakKey = 'F1',    action = function()
    far.Message(help, 'Lua Explorer - Help', nil, "l")
  end},
}

-- if LuaJIT is used, maybe we can show some more function info
if jit and jit.util then -- non-public jit.* API
table.insert(brkeys,     {BreakKey = 'Shift+F3',    action = function(obj, key, kpath)
    if key ~= nil and type(obj[key]) == 'function' then
      process(jit.util.funcinfo(obj[key]), 'jit.util.funcinfo: ' .. kpath)
    end
  end})
end

-- format values for menu items and message boxes
local function valfmt(val, edit)
  local t = type(val)
  if t == 'string' then
    return (edit and ('%q'):format(val) or '"' ..  val .. '"'), t
  elseif t == 'number' then
    return (edit and '0x%x --[[ %s ]]' or '0x%08x (%s)'):format(val, val), t
  end
  return tostring(val), t
end

-- make menu item for far.Menu(...)
local function makeItem(key, sval, vt)
  return {
--    checked    = '',
    text    = ('%-30s│%-8s│%-25s'):format(valfmt(key), vt, sval),
    key    = key
  }
end

-- create sorted menu items with associated keys
local function makeMenuItems(obj)
  local items = {}

  -- grab all 'real' keys
  for key in pairs(obj) do
    local sval, vt = valfmt(obj[key])
    if not omit[vt] then
      table.insert(items, makeItem(key, sval, vt))
    end
  end

  -- Far uses some properties that in fact are functions in obj.properties
  -- but they logically belong to the object itself. It's all Lua magic ;)
  if type(obj.properties) == 'table' and not rawget(obj, 'properties') then
    for key in pairs(obj.properties) do
      local sval, vt = valfmt(obj[key])
      if not omit[vt] then
        table.insert(items, makeItem(key, sval, vt))
      end
    end
  end

  table.sort(items, function(v1, v2) return v1.text < v2.text end)

  return items
end

-- remove object at obj[key]
deleteValue = function(obj, key, title, callback)
  if 1 == far.Message(
    ('%s is a %s, do you want to remove it?'):format(valfmt(key), type(obj[key]):upper()),
    'REMOVE: ' .. title, "Yes;No", "w")
  then
    if type(callback)~="function" or callback(obj, key, nil) then
      obj[key] = nil
    end
  end
end

-- edit object at obj[key]
editValue = function(obj, key, title, callback)
  local v, t = valfmt(obj[key], true)
  if t == 'table' or t == 'function' then v = '' end
  local sval = far.InputBox(nil, 'EDIT: ' .. title,
    ('%s is a %s, type new value as Lua code'):format(valfmt(key), t:upper()), 'edit.' .. title, v)
  if sval and #sval > 0 then
    local val = loadstring('return ' .. sval)()
    if type(callback)~="function" or callback(obj, key, val) then
      obj[key] = val
    end
  end
end

-- add new element to obj
insertValue = function(obj, title, callback)
  local args = far.InputBox(nil, 'INSERT: ' .. title,
    'type the key and value comma separated as Lua code', 'insert.' .. title)
  if args and #args > 0 then
    local k, v = loadstring('return ' .. args)()
    if type(callback)~="function" or callback(obj, k, v) then
      obj[k] = v
    end
  end
end

-- show a menu whose items are associated with the members of given object
process = function(obj, title, callback)
  local mprops = {Id = uuid, Flags = MenuFlags, Bottom = 'F1, F3, F4, Del, Ins, Ctrl+F, Ctrl+M, Alt+X'}
  local otype = type(obj)
  local item, index


  -- some member types, need specific behavior:
  -- tables are submenus

  -- functions can be called
  if otype == 'function' then
    local args = far.InputBox(nil, 'CALL: ' .. title .. '(...)',
      'Type arguments as Lua code or leave empty:', 'args.' .. title)
    if args then
      -- overwrite the function object with its return values
      obj = {pcall(obj, loadstring('return ' .. args)())}
      title = '({pcall(' .. title .. ')})'
    else
      return
    end

  -- other values are simply displayed in a message box
  elseif otype ~= 'table' then
    far.Message(valfmt(obj), title, nil, "l")
    return
  end


  -- show this menu level again after each return from a submenu/function call ...
  repeat
    local items = makeMenuItems(obj)
    mprops.Title = title .. '  (' .. #items .. ')' .. (omit['function'] and '*' or '')

    item, index = far.Menu(mprops, items, brkeys)
    mprops.SelectIndex = index

    -- show submenu/call function ...
    if item then
      local key = item.key or (index > 0 and items[index].key) or nil
      local newtitle = title .. (type(key) == 'number' and '[' .. key .. ']' or '.' .. tostring(key))
      if item.key ~= nil then
        if process(obj[key], newtitle, callback) == "exit" then return "exit" end
      elseif item.action then
        item.action(obj, key, newtitle, callback)
      elseif item.exit then
        return "exit"
      end
    end
    -- until the user is bored and goes back ;)
  until not item
end

local function process2(obj, title, callback)
  omit = {} -- as we are a module, we should avoid dependence on previous calls.
  return process(obj, title or "<no name>", callback)
end

return process2
