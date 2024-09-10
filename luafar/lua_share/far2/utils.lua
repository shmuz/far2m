-- utils.lua --

local F = far.Flags
local bor = bit64.bor
local dirsep = package.config:sub(1,1)
local PluginDir = far.PluginStartupInfo().ShareDir .. dirsep

local function CheckLuafarVersion (reqVersion, msgTitle)
  local v1, v2 = far.LuafarVersion(true)
  local r1, r2 = reqVersion:match("^(%d+)%.(%d+)")
  r1, r2 = tonumber(r1), tonumber(r2)
  if (v1 > r1) or (v1 == r1 and v2 >= r2) then return true end
  far.Message(
    ("LuaFAR %s or newer is required\n(loaded version is %s)")
    :format(reqVersion, far.LuafarVersion()),
    msgTitle, ";Ok", "w")
  return false
end

local function OnError (msg)
  local tPaths = { PluginDir }
  for dir in package.path:gmatch("[^;]+") do
    tPaths[#tPaths+1] = dir:gsub("[^/]+$", "")
  end

  local function repair(str)
    for _, dir in ipairs(tPaths) do
      local part1, part2 = str, ""
      while true do
        local p1, p2 = part1:match("(.*/)(.+)")
        if not p1 then break end
        part1, part2 = p1, p2..part2
        if part1 == dir:sub(-part1:len()) then
          return dir .. str:sub(-part2:len())
        end
      end
    end
  end

  local jumps, buttons = {}, "&OK"
  msg = string.gsub(tostring(msg), "[^\n]+", -- use string.gsub to avoid "invalid UTF-8 code" error
    function(line)
      line = line:gsub("^(\t?)(.-)%:(%d+)%:(%s*)",
        function(_, file, numline, space)
          if #jumps < 9 then
            local file2 = file:sub(1,3) ~= "..." and file or repair(file:sub(4))
            if file2 then
              local name = file2:match('^%[string "(.*)"%]$')
              if not name or name=="all text" or name=="selection" then
                jumps[#jumps+1] = { file=file2, line=tonumber(numline) }
                buttons = buttons .. ";[J&" .. (#jumps) .. "]"
                return ("\16[J%d]:%s:%s:%s"):format(#jumps, file, numline, space)
              end
            end
          end
          return "[?]:" .. file .. ":" .. numline .. ":" .. space
        end)
      return (line:gsub("^\t", "   "))
    end)
  collectgarbage "collect"
  local caption = ("Error [used: %d Kb]"):format(collectgarbage "count")
  local ret = far.Message(msg, caption, buttons, "wl")
  if ret <= 1 then return end
  ret = ret - 1 -- skip the leftmost button "OK"

  local file, line = jumps[ret].file, jumps[ret].line
  local luaScript = file=='[string "all text"]' or file=='[string "selection"]'
  if not luaScript then
    local trgInfo
    for i=1,actl.GetWindowCount() do
      local wInfo = actl.GetWindowInfo(i)
      if wInfo.Type==F.WTYPE_EDITOR and
        wInfo.Name:gsub("/",dirsep) == file:gsub("/",dirsep)
      then
        trgInfo = wInfo
        if wInfo.Current then break end
      end
    end
    if trgInfo then
      if not trgInfo.Current then
        actl.SetCurrentWindow(trgInfo.Pos, true)
      end
    else
      editor.Editor(file, nil,nil,nil,nil,nil, {EF_NONMODAL=1,EF_IMMEDIATERETURN=1})
    end
  end

  local eInfo = editor.GetInfo()
  if eInfo then
    if file == '[string "selection"]' then
      local startsel = eInfo.BlockType~=F.BTYPE_NONE and eInfo.BlockStartLine or 0
      line = line + startsel
    end
    local offs = math.floor(eInfo.WindowSizeY / 2)
    editor.SetPosition(nil,line, 1, 1, line>offs and line-offs or 0)
    editor.Redraw()
  end
end

local function LoadEmbeddedScript (name)
  local embed_name = "<"..name
  local loader = package.preload[embed_name]
  return loader and loader(embed_name)
end

local function RunInternalScript (name, ...)
  local f = LoadEmbeddedScript(name)
  if f then return f(...) end
  local f2, errmsg = loadfile(PluginDir..name..".lua")
  if f2 then return f2(...) end
  error(errmsg)
end

local function LoadName (str)
  local f = LoadEmbeddedScript(str)
  if f then return f end
  for part in package.path:gmatch("[^;]+") do
    local name = part:gsub("%?", str)
    local attr = win.GetFileAttr(name)
    if attr and not attr:find("d") then
      return assert(loadfile(name))
    end
  end
  error(str..": file not found")
end

-- @aItem.filename:  script file name
-- @aItem.env:       environment to run the script in
-- @aItem.arg:       array of arguments associated with aItem
--
-- @aProperties:     table with property-like arguments, e.g.: "From", "hDlg"
--
-- ...:              sequence of additional arguments (appended to existing arguments)
--
local function RunUserItem (aItem, aProperties, ...)
  assert(aItem.filename, "no file name")
  assert(aItem.env, "no environment")

  -- Get the chunk. If it is not given directly then find and compile the file.
  local chunk = type(aItem.filename)=="function" and aItem.filename or LoadName(aItem.filename)

  -- Set environment. Use pcall since if chunk is not a Lua function an error is thrown.
  pcall(setfenv, chunk, aItem.env)

  -- Copy "fixed" and append "variable" arguments, then run the chunk.
  if aItem.unpack then
    local args = { unpack(aItem.arg, 1, aItem.arg.n) }
    local n2 = select("#", ...)
    for k=1,n2 do args[aItem.arg.n + k] = select(k, ...); end
    return chunk(unpack(args, 1, aItem.arg.n + n2))
  else
    local args = {}
    for k,v in pairs(aProperties) do args[k] = v end
    for i,v in ipairs(aItem.arg)  do args[i] = v end
    local n, n2 = #args, select("#", ...)
    args.n = n + n2
    for i=1,n2 do args[n+i] = select(i, ...) end
    return chunk(args)
  end
end

local function ConvertUserHotkey(str)
  local d = 0
  for elem in str:upper():gmatch("[^+%-]+") do
    if elem == "ALT" then d = bor(d, 0x01)
    elseif elem == "CTRL" then d = bor(d, 0x02)
    elseif elem == "SHIFT" then d = bor(d, 0x04)
    else d = d .. "+" .. elem; break
    end
  end
  return d
end

local function MakeAddToMenu (Items, Env, HotKeyTable, Unpack)
  local function AddToMenu (aWhere, aItemText, aHotKey, aFileName, ...)
    if type(aWhere) ~= "string" then return end
    aWhere = aWhere:lower()
    if not aWhere:find("[evpdc]") then return end
    ---------------------------------------------------------------------------
    local SepText = type(aItemText)=="string" and aItemText:match("^:sep:(.*)")
    local bUserItem = SepText or type(aFileName)=="string" or type(aFileName)=="function"
    if not bUserItem then
      if aItemText~=true or type(aFileName)~="number" then
        return
      end
    end
    ---------------------------------------------------------------------------
    if HotKeyTable and not SepText and aWhere:find("[ec]") and type(aHotKey)=="string" then
      local key = ConvertUserHotkey (aHotKey)
      if HotKeyTable[key] then
        far.Message(("Key `%s' is already allocated"):format(aHotKey),"AddToMenu",nil,"w")
      elseif bUserItem then
        local n = select("#", ...)
        HotKeyTable[key] = {filename=aFileName, env=Env, arg={n=n, ...}, unpack=Unpack}
      else
        HotKeyTable[key] = aFileName -- menu position of a built-in utility
      end
    end
    ---------------------------------------------------------------------------
    if bUserItem and aItemText then
      local item
      if SepText then
        item = { text=SepText, separator=true }
      else
        local n = select("#", ...)
        item = { text=tostring(aItemText), filename=aFileName, env=Env, arg={n=n, ...}, unpack=Unpack }
      end
      if aWhere:find"c" then table.insert(Items.config, item) end
      if aWhere:find"d" then table.insert(Items.dialog, item) end
      if aWhere:find"e" then table.insert(Items.editor, item) end
      if aWhere:find"p" then table.insert(Items.panels, item) end
      if aWhere:find"v" then table.insert(Items.viewer, item) end
    end
  end
  return AddToMenu
end

local function MakeAddCommand (CommandTable, Env, Unpack)
  return function (aCommand, aFileName, ...)
    local fntype = type(aFileName)
    if type(aCommand)=="string" and (fntype=="string" or fntype=="function") then
      local n = select("#", ...)
      CommandTable[aCommand] = { filename=aFileName, env=Env, arg={n=n, ...}, unpack=Unpack }
    end
  end
end

local function MakeAutoInstall (AddUserFile)
  local function AutoInstall (startpath, filepattern, depth)
    assert(type(startpath)=="string", "bad arg. #1 to AutoInstall")
    assert(filepattern==nil or type(filepattern)=="string", "bad arg. #2 to AutoInstall")
    assert(depth==nil or type(depth)=="number", "bad arg. #3 to AutoInstall")
    ---------------------------------------------------------------------------
    startpath = PluginDir .. startpath:gsub("/*$", "/", 1)
    filepattern = filepattern or "^_usermenu%.lua$"
    ---------------------------------------------------------------------------
    local first = depth
    local offset = PluginDir:len() + 1

    local DirList = {}
    far.RecursiveSearch(startpath, "*",
      function(item, fullname) item.FileName=fullname; DirList[#DirList+1]=item end,
      bor(F.FRS_RECUR,F.FRS_SCANSYMLINK))

    for _, item in ipairs(DirList) do
      if first then
        first = false
        local _, m = item.FileName:gsub("/", "")
        depth = depth + m
      end
      if not item.FileAttributes:find"d" then
        local try = true
        if depth then
          local _, n = item.FileName:gsub("/", "")
          try = (n <= depth)
        end
        if try then
          local relName = item.FileName:sub(offset)
          local Name = relName:match("[^/]+$")
          if Name:match(filepattern) then AddUserFile(relName) end
        end
      end
    end
  end
  return AutoInstall
end

local function LoadUserMenu (aFileName)
  local userItems = { editor={},viewer={},panels={},config={},dialog={} }
  local commandTable, hotKeyTable = {}, {}
  local handlers = { EditorInput={}, EditorEvent={}, ViewerEvent={}, ExitScript={} }
  local mapHandlers = {
    ProcessEditorInput = handlers.EditorInput,
    ProcessEditorEvent = handlers.EditorEvent,
    ProcessViewerEvent = handlers.ViewerEvent,
    ExitScript         = handlers.ExitScript,
  }
  local uStack, uDepth, uMeta = {}, 0, {__index = _G}
  local env = setmetatable({}, {__index=_G})
  ------------------------------------------------------------------------------
  env.MakeResident = function (source)
    if type(source) == "string" then
      local chunk = LoadName(source)
      local env2 = setmetatable({}, { __index=_G })
      local ok, errmsg = pcall(setfenv(chunk, env2))
      if not ok then error(errmsg, 2) end
      for name, target in pairs(mapHandlers) do
        local f = rawget(env2, name)
        if type(f)=="function" then table.insert(target, f) end
      end
    end
  end
  ------------------------------------------------------------------------------
  env.AddUserFile = function (filename)
    uDepth = uDepth + 1
    filename = PluginDir .. filename
    if uDepth == 1 then
      -- if top-level _usermenu.lua doesn't exist, it isn't error
      local attr = win.GetFileAttr(filename)
      if not attr or attr:find("d") then return end
    end
    local chunk = assert(loadfile(filename))
    uStack[uDepth] = setmetatable({}, uMeta)
    env.AddToMenu    = MakeAddToMenu(userItems, uStack[uDepth], hotKeyTable, false)
    env.AddToMenuEx  = MakeAddToMenu(userItems, uStack[uDepth], hotKeyTable, true)
    env.AddCommand   = MakeAddCommand(commandTable, uStack[uDepth], false)
    env.AddCommandEx = MakeAddCommand(commandTable, uStack[uDepth], true)
    setfenv(chunk, env)()
    uDepth = uDepth - 1
  end
  ------------------------------------------------------------------------------
  env.AutoInstall = MakeAutoInstall(env.AddUserFile)
  env.AddUserFile(aFileName)
  return userItems, commandTable, hotKeyTable, handlers
end

local function AddMenuItems (trg, src, msgtable)
  trg = trg or {}
  for _, item in ipairs(src) do
    local text = item.text
    if type(text)=="string" and text:sub(1,2)=="::" then
      local newitem = {}
      for k,v in pairs(item) do newitem[k] = v end
      newitem.text = msgtable[text:sub(3)]
      trg[#trg+1] = newitem
    else
      trg[#trg+1] = item
    end
  end
  return trg
end

local function CommandSyntaxMessage (tCommands, sTitle)
  local pluginInfo = export.GetPluginInfo()
  local syn = [[
Command line syntax:
  %s: [<options>] <command>|-r<filename> [<arguments>]

Options:
  -a          asynchronous execution
  -e <str>    execute string <str>
  -l <lib>    load library <lib>

Macro call syntax (Id = 0x%08X):
  Plugin.Call(Id, "code",    <code>     [,<arguments>])
  Plugin.Call(Id, "file",    <filename> [,<arguments>])
  Plugin.Call(Id, "command", <command>  [,<arguments>])
  Plugin.Call(Id, "own",     <command>  [,<arguments>])

Available commands:
]]

  syn = syn:format(pluginInfo.CommandPrefix, far.GetPluginId())
  if tCommands and next(tCommands) then
    local arr = {}
    for k in pairs(tCommands) do arr[#arr+1] = k end
    table.sort(arr)
    syn = syn .. "  " .. table.concat(arr, ", ")
  else
    syn = syn .. "  <no commands available>"
  end
  far.Message(syn, sTitle, ";Ok", "l")
end

local function CompileCommandLine (sCommandLine, tCommands)
  local actions = {}
  local opt
  local args = { far.SplitCmdLine(sCommandLine) }
  for i,v in ipairs(args) do
    local curropt, param
    if opt then
      curropt, param, opt = opt, v, nil
    else
      if v:sub(1,1) == "-" then
        local newopt
        newopt, param = v:match("^%-([aelr])(.*)")
        if newopt == nil then
          error("invalid option: "..v)
        end
        if newopt == "a" then actions.async = true
        elseif param == "" then  opt = newopt
        else curropt = newopt
        end
      else
        if not tCommands[v] then
          error("invalid command: "..v)
        end
        actions[#actions+1] = { command=v, unpack(args, i+1) }
        break
      end
    end
    if curropt == "r" then
      actions[#actions+1] = { opt=curropt, param=param, unpack(args, i+1) }
      break
    elseif curropt then
      actions[#actions+1] = { opt=curropt, param=param }
    end
  end
  return actions
end

local function ExecuteCommandLine (tActions, tCommands, sFrom, fConfig)
  local function wrapfunc()
    local env = setmetatable({}, {__index=_G})
    for _,v in ipairs(tActions) do
      if v.command then
        local fileobject = tCommands[v.command]
        RunUserItem(fileobject, {From=sFrom}, unpack(v))
        break
      elseif v.opt == "r" then
        local path = v.param
        if not path:find("^/") then
          local panelDir = panel.GetPanelDirectory(nil, 1).Name
          path = panelDir:gsub("[^/]$", "%1/") .. path
        end
        local f = assert(loadfile(path))
        setfenv(f, env)(unpack(v))
      elseif v.opt == "e" then
        local f = assert(loadstring(v.param))
        setfenv(f, env)()
      elseif v.opt == "l" then
        require(v.param)
      end
    end
  end
  local oldConfig = fConfig and fConfig()
  local ok, res = xpcall(wrapfunc, function(msg) return debug.traceback(msg, 2) end)
  if fConfig then fConfig(oldConfig) end
  if not ok then export.OnError(res) end
end

local function OpenCommandLine (sCommandLine, tCommands, fConfig, sTitle)
  local tActions = CompileCommandLine(sCommandLine, tCommands)
  if not tActions[1] then
    CommandSyntaxMessage(tCommands, sTitle)
  elseif tActions.async then
    ---- autocomplete:good; Escape response:bad when timer period < 20;
    far.Timer(30,
      function(h)
        h:Close(); ExecuteCommandLine(tActions, tCommands, "panels", fConfig)
      end)
  else
    ---- autocomplete:bad; Escape responsiveness:good;
    ExecuteCommandLine(tActions, tCommands, "panels", fConfig)
  end
end

local function OpenMacro (Args, CommandTable, fConfig, sTitle)
  local op = Args[1]
  if op=="command" then
    if type(Args[2]) == "string" then
      local fileobject = CommandTable[Args[2]]
      if fileobject then
        local oldConfig = fConfig and fConfig()
        local map = {
          [F.MACROAREA_SHELL]  = "panels", [F.MACROAREA_EDITOR] = "editor",
          [F.MACROAREA_VIEWER] = "viewer", [F.MACROAREA_DIALOG] = "dialog",
        }
        local wrapfunc = function()
          return RunUserItem(fileobject, { From=map[far.MacroGetArea()] }, unpack(Args,3,Args.n))
        end
        local retfunc = function (ok, ...)
          if fConfig then fConfig(oldConfig) end
          if ok then return ... end
          export.OnError(...)
          return true
        end
        return retfunc(xpcall(wrapfunc, function(msg) return debug.traceback(msg, 3) end))
      else
        CommandSyntaxMessage(CommandTable, sTitle)
      end
    end
  elseif op=="code" or op=="file" then
    if type(Args[2]) == "string" then
      local chunk = op=="file" and assert(loadfile(win.ExpandEnv(Args[2])))
                                or assert(loadstring(Args[2]))
      local env = setmetatable({}, { __index=_G })
      setfenv(chunk, env)
      return chunk(unpack(Args,3,Args.n))
    end
  end
  return false
end

return {
  AddMenuItems       = AddMenuItems;
  CheckLuafarVersion = CheckLuafarVersion;
  LoadUserMenu       = LoadUserMenu;
  OnError            = OnError;
  OpenCommandLine    = OpenCommandLine;
  OpenMacro          = OpenMacro;
  RunInternalScript  = RunInternalScript;
  RunUserItem        = RunUserItem;
}
