-- Started:                 2021-04-02
-- Portability:             far3, far2m
-- Minimal far3 version:    3.0.3300
-- Far plugin:              Any LuaFAR plugin
-- Dependencies:            Lua modules: far2.simpledialog, far2.settings

local SETTINGS_KEY    = "shmuz"
local SETTINGS_SUBKEY = "equation"
local MAXITER = 1024
local Title = "Equation"

local F = far.Flags
local abs = math.abs
local KEEP_DIALOG_OPEN = 0

local function log(...)
  local n, t = select("#",...), {...}
  for k=1,n do t[k] = tostring(t[k]) end
  win.OutputDebugString(table.concat(t, ", "))
end

local function isnumber(v)
  return v==v and (v==0 or v~=v/2 and v~=v*2)
end

-- @param from     : range start; Search is always performed in direction 'from' --> 'to'.
-- @param to       : range end
-- @param deviat   : acceptable deviation (accuracy) of the result
-- @param getvalue : function calculating the result
--------------------------------------------------------------------------------
-- @return         : result deviation is OK   (boolean)
-- @return         : result itself            (number or false)
-- @return         : count of iterations      (number)
local function bisect(from, to, deviat, getvalue)
  -- log(from,to)
  deviat = abs(deviat)

  -- Ensure that getvalue(from) is a number.
  -- Then maintain 'from' to have this property all the way.
  local val, count

  for n = 1, MAXITER do
    local v = getvalue(from)
    if isnumber(v) then
      val, count = v, n
      break
    end
    from = (from + to) / 2
  end

  if val == nil then
    return false, false, MAXITER
  end

  local cur = from
  local sign = (val >= 0)

  for n = count, MAXITER do
    local val = getvalue(cur)
    if isnumber(val) then
      if abs(val) <= deviat then
        return true, cur, n
      end
      if (val>=0) == sign then
        from = cur
      else
        to = cur
      end
    end
    cur = (from + to) / 2
  end

  return false, isnumber(val) and cur, MAXITER
end

local function GetText(hDlg,Par1) return far.SendDlgMessage(hDlg,"DM_GETTEXT",Par1) end
local function SetText(hDlg,Par1,Par2) far.SendDlgMessage(hDlg,"DM_SETTEXT",Par1,Par2) end
local function Unverbose(s) return (string.gsub(s,".*%]:%d+: ","")) end -- note the extra parentheses
local function ChunkMessage(msg, title) far.Message(Unverbose(msg), title, nil, "w"); end

local function AddHotKeysToMenu (menu_items)
  for i = 1, math.min(36,#menu_items) do
    local offset = i <= 10 and 47 or 54
    local item = menu_items[i]
    item.text = "&"..string.char(offset+i)..". "..item.text
  end
end

local function showhelp()
  -- Note: the text parts in a line enclosed in ## get highlighted
  local msg = [[
This utility solves 1-variable equations, e.g. x + cos(x) = x^2
Functions of math library should be used without 'math' prefix.
                         #Dialog fields:#
#Variables#
  - Intended for assigning variables before the calculation;
    treated as a Lua chunk; may be left blank.
#Equation#
  - 2 Lua expressions delimited with the #=# character
  - Variable #x# should be used as the equation variable;
    all other variables used (if any) should be defined.
#Range of x#
  - 2 numbers or Lua expressions delimited with a comma.
  - If the field is left empty or starts with #::# then the program
    tries different ranges until it succeeds or eventually fails.
#Accuracy#
  - A number or Lua expression meaning the maximal acceptable
    difference between the left and right parts of the equation.
    If this field is empty, the default value of 0 is used.
#Expression#
  - The expression is calculated after equation's calculation
    and therefore may contain variable #x#. May be left blank.]]

  if far.CreateUserControl then -- since Far 3.0.3590
    local sd = require ("far2.simpledialog")
    local items = {
      {tp="dbox"; text=Title.." Help";                 },
      {tp="user2"; text=msg;                           },
      {tp="sep";                                       },
      {tp="butt"; text="OK"; centergroup=1; default=1; },
    }
    sd.New(items):Run()
  else
    far.Message(msg:gsub("#",""), Title.." Help", nil, "l")
  end
end

local function dialog()
  local sDialog  = require "far2.simpledialog"
  local settings = require "far2.settings"
  local wid1 = 34
  local ofs1 = wid1+10
  local items = {
    guid = "FF76240E-D4C6-4FCD-8BAA-226390D1C58B";
    help=showhelp;
    {tp="dbox"; name="title"; text=Title;                              },
    {tp="butt"; name="btnClear"; text="Clear"; x1=ofs1; nofocus=1;     },
    {tp="text"; text="&Variables:"; ystep=0; width=wid1;               },
    {tp="edit"; name="vars"; ext="lua";                        Save=1; },

    {tp="text"; text="&Equation:";                                     },
    {tp="edit"; name="equation"; ext="lua";                    Save=1; },

    {tp="text"; text="&Range of x:";                                   },
    {tp="edit"; name="range"; width=wid1;                      Save=1; },

    {tp="text"; text="&Accuracy:";  x1=ofs1; ystep=-1;                 },
    {tp="edit"; name="accuracy"; x1="";                        Save=1; },

    {tp="text"; text="E&xpression:";                                   },
    {tp="edit"; name="expr"; width=wid1;                       Save=1; },

    {tp="text"; text="Expression result";  x1=ofs1; ystep=-1;          },
    {tp="edit"; name="expr_result"; x1="";                             },

    {tp="sep";                                                         },
    {tp="text"; text="Result";                                         },
    {tp="edit"; name="result"; width=wid1;                             },

    {tp="text"; text="Iteration count"; x1=ofs1; ystep=-1;             },
    {tp="edit"; name="count"; x1="";                                   },

    {tp="butt"; name="btnCalc"; text="Calc"; centergroup=1; default=1; },
    {tp="butt"; name="btnLoad"; text="&Load"; centergroup=1;           },
    {tp="butt"; name="btnSave"; text="&Save"; centergroup=1;           },
    {tp="butt"; name="btnDescr"; text="&Descript"; centergroup=1;      },
  }

  local dlg = sDialog.New(items)
  local dPos = dlg:Indexes()
  local CurName, CurDescr
  ------------------------------------------------------------------------------
  local function UpdateState(hDlg, aName, aDescr)
    SetText(hDlg, dPos.title, aName and Title..": "..aName or Title)
    CurName, CurDescr = aName, aDescr
  end
  ------------------------------------------------------------------------------
  local function btnClear_action (hDlg,Par1,Par2)
    far.SendDlgMessage(hDlg, "DM_SETFOCUS", dPos.vars)
    UpdateState(hDlg)
    for i,v in ipairs(items) do
      if v.tp == "edit" then SetText(hDlg, i, ""); end
    end
  end
  ------------------------------------------------------------------------------
  local function btnSave_action (hDlg,Par1,Par2)
    far.SendDlgMessage(hDlg, "DM_SETFOCUS", dPos.vars)
    local my_items = {
      guid = "6DB23383-EB7B-4BE1-BA22-CD7C9359C149";
      {tp="dbox"; text="Save preset";                      },
      {tp="text"; text="Preset name:";                     },
      {tp="edit"; name="preset"; text=CurName;             },
      {tp="butt"; text="&OK"; centergroup=1; default=1;    },
      {tp="butt"; text="&Cancel"; centergroup=1; cancel=1; },
    }

    local data = settings.mload(SETTINGS_KEY, SETTINGS_SUBKEY) or {}
    data.presets = data.presets or {}

    function my_items.proc(_hDlg, _Msg, _Par1, _Par2)
      if _Msg == F.DN_CLOSE then
        local tOut = _Par2
        if not tOut.preset:find("%S") then
          far.Message("Invalid name for the preset","Error",nil,"w")
          return KEEP_DIALOG_OPEN
        end
        if tOut.preset ~= CurName and data.presets[tOut.preset] then
          local msg = ("Preset '%s' already exists.\nOverwrite?"):format(tOut.preset)
          if far.Message(msg, "Warning", ";YesNo", "w") ~= 1 then
            return KEEP_DIALOG_OPEN
          end
        end
      end
    end

    local out = sDialog.New(my_items):Run()
    if out then
      UpdateState(hDlg, out.preset, CurDescr)
      local t = { ["description"]=CurDescr }
      for i,v in ipairs(items) do
        if v.Save then t[v.name] = GetText(hDlg,i); end
      end
      data.presets[out.preset] = t
      settings.msave(SETTINGS_KEY, SETTINGS_SUBKEY, data)
    end
  end
  ------------------------------------------------------------------------------
  local function btnLoad_action (hDlg,Par1,Par2)
    far.SendDlgMessage(hDlg, "DM_SETFOCUS", dPos.vars)
    local data = settings.mload(SETTINGS_KEY, SETTINGS_SUBKEY) or {}
    data.presets = data.presets or {}
    while true do
      local menu_items = {}
      for k,v in pairs(data.presets) do
        table.insert(menu_items, {name=k; text=k; Preset=v;})
      end
      table.sort(menu_items, function(a,b) return a.name:lower() < b.name:lower(); end)
      AddHotKeysToMenu(menu_items)
      local item, pos = far.Menu({Title="Load preset"; Bottom="Ctrl-Del";}, menu_items,
        {{BreakKey="C+DELETE"}})
      if not item then break; end

      if item.Preset then
        UpdateState(hDlg, item.name, item.Preset.description)
        for i,v in ipairs(items) do
          if v.tp=="edit" then SetText(hDlg, i, v.Save and item.Preset[v.name] or ""); end
        end
        break
      elseif item.BreakKey and menu_items[pos] then
        local name = menu_items[pos].name
        if 1 == far.Message("Delete '"..name.."'?", "Confirm preset deletion", ";YesNo", "w") then
          if name == CurName then
            UpdateState(hDlg)
          end
          data.presets[name] = nil
          settings.msave(SETTINGS_KEY, SETTINGS_SUBKEY, data)
        end
      end
    end
  end
  ------------------------------------------------------------------------------
  local function btnDescr_action (hDlg,Par1,Par2)
    far.SendDlgMessage(hDlg, "DM_SETFOCUS", dPos.vars)
    local txt = sDialog.OpenInEditor(CurDescr, "txt")
    if txt then CurDescr = txt; end
  end
  ------------------------------------------------------------------------------
  local function btnCalc_action (hDlg,Par1,Par2)
    local accuracy, func, f_equat, f_expr, msg, ok, acc_ok
    local env = setmetatable({}, { __index=math; })

    -- get variables
    func, msg = loadstring(GetText(hDlg, dPos.vars))
    if not func then ChunkMessage(msg, "Variables error"); return; end
    setfenv(func, env)
    ok, msg = pcall(func)
    if not ok then far.Message(msg,"Variables error",nil,"w") return end

    -- process equation
    local left, right
    local eq = GetText(hDlg, dPos.equation)
    for pos,chars in eq:gmatch("()([<>~=]+)") do -- search for = but skip <=, >=, ~=, ==
      if chars == "=" then
        left, right = eq:sub(1,pos-1), eq:sub(pos+1)
        break
      end
    end
    if not left then far.Message("Equation error","Error",nil,"w"); return; end
    local chunk = ("local x=...; return (%s) - (%s)"):format(left, right)
    f_equat, msg = loadstring(chunk)
    if not f_equat then ChunkMessage(msg, "Equation error"); return; end

    -- get range of x
    local from, to
    local txt = GetText(hDlg, dPos.range)
    if txt:find("%S") and not txt:find("^%s*::") then
      func, msg = loadstring("return "..txt)
      if not func then ChunkMessage(msg, "Range error"); return; end
      setfenv(func, env)
      ok, from, to = pcall(func)
      if not ok then far.Message(from,"Range error",nil,"w"); return; end
      if not(type(from)=="number" and type(to)=="number") then
        far.Message("Range error","Error",nil,"w"); return; end
    end

    -- get accuracy
    func, msg = loadstring("return "..GetText(hDlg, dPos.accuracy))
    if not func then ChunkMessage(msg, "Accuracy error"); return; end
    setfenv(func, env)
    ok, accuracy = pcall(func)
    if not ok then ChunkMessage(accuracy, "Accuracy error") return; end
    if accuracy == nil then
      accuracy = 0
    elseif type(accuracy)~="number" then
      far.Message("Accuracy must result in a number","Accuracy error",nil,"w"); return
    end

    -- get expression
    txt = GetText(hDlg, dPos.expr)
    if txt:find("%S") then
      f_expr, msg = loadstring("return " .. txt)
      if not f_expr then ChunkMessage(msg, "Expression error"); return; end
    end

    -- run calculation
    setfenv(f_equat, env)
    local res, count
    local autorange = not from
    if from then
      ok, acc_ok, res, count = pcall(bisect, from, to, accuracy, f_equat)
      if not ok then ChunkMessage(acc_ok,"Equation error"); return; end
    else
      for k=-1023,1023 do
        from,to = -2^k, 2^k
        ok, acc_ok, res, count = pcall(bisect, from, to, accuracy, f_equat)
        if not ok then ChunkMessage(acc_ok,"Equation error"); return; end
        if acc_ok then break; end
      end
    end
    if res then
      env.x = res -- needed for expression calculation
      res = ("%.17g"):format(res)
      if not acc_ok then res=res.." (accuracy failed)"; end
      SetText(hDlg, dPos.result, res)
      SetText(hDlg, dPos.count, tostring(count))
    else
      SetText(hDlg, dPos.result, "(invalid)")
      SetText(hDlg, dPos.count, tostring(count))
    end
    if autorange then
      SetText(hDlg, dPos.range, (":: %.3g, %.3g"):format(from, to))
    end

    -- run expression
    if res and f_expr then
      setfenv(f_expr, env)
      local ok, res = pcall(f_expr)
      if ok then res = type(res)=="number" and ("%.17g"):format(res) or "<not a number>"
      else res = Unverbose(res)
      end
      SetText(hDlg, dPos.expr_result, res)
    else
      SetText(hDlg, dPos.expr_result, "")
    end
  end
  ------------------------------------------------------------------------------
  function items.proc(hDlg, Msg, Par1, Par2)
    if Msg == F.DN_CLOSE then
      return Par1 >= 1 and KEEP_DIALOG_OPEN or nil
    --------------------------------------------------------
    elseif Msg == F.DN_BTNCLICK then
      local f = Par1==dPos.btnClear and btnClear_action or
                Par1==dPos.btnSave  and btnSave_action  or
                Par1==dPos.btnLoad  and btnLoad_action  or
                Par1==dPos.btnDescr and btnDescr_action or
                Par1==dPos.btnCalc  and btnCalc_action
      if f then return f(hDlg,Par1,Par2) end
    --------------------------------------------------------
    elseif Msg == "EVENT_KEY" then
      if Par2=="Enter" or Par2=="NumEnter" then
        if items[Par1].tp ~= "butt" then
          btnCalc_action(hDlg,Par1)
        end
      end
    --------------------------------------------------------
    end
  end

  dlg:Run()
end

return {
  dialog = dialog;
  showhelp = showhelp;
}
