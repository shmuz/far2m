-- coding: utf-8
-- Minimal Far3 version:     3.0.3300
-- Minimal far2m version:    2.5.0

-- http://forum.farmanager.com/viewtopic.php?p=141965#p141965
-- Исходный скрипт был написан для плагина LuaFAR for Editor.
-- Первоначальный автор: Максим Гончар ("maxdrfl" в форуме Far Manager).
-- Адаптация к Far3 и far2m API, плагину LuaMacro и изменения в коде: Shmuel Zeigerman.

--[[
        ОРИГИНАЛЬНАЯ СПРАВКА (адаптировано):
        ------------------------------------
Небольшой калькулятор. Написан в первую очередь для проверки работы диалогов и скорости сборки скриптов lua.
Свойства:
    - синтаксис lua
    - функции из math и константы из far.Flags находятся в _G
    - сборка на лету. Указание этапа работы на котором произошла ошибки
    - 4 строки вывода с настраиваемым форматом (см. lua:string.format)
    - если в строке есть команда return, результатом вычисления считается возвращаемое значение
    - поддерживаются пользовательские функции.

Пользовательские функции хранятся в модуле fl_calc.lua. Наличие модуля не обязательно для работы калькулятора.
Краткая справка пишется в том же файле в таблицу help. Вызывается из калькулятора по F1.
Для справки см. уже определённые функции fib, fact, mean, sum.
--]]

local FAR3 = package.config:sub(1,1)=="\\"

----------------------------------------------------------------------------
-->> SETTINGS START
----------------------------------------------------------------------------
-- @@ Lunatic Python library
-- Set to nil if the library is not available.
local Lib_Python = FAR3 and "312-python" or "python"
--<< SETTINGS END
----------------------------------------------------------------------------

local Lang = {
  English = {
    mMainDlgTitle = "Lua Calculator";
    mLabelStatus  = "Status:";
    mButtonCalc   = "Calculate";
    mButtonInsert = "Insert (Ins)";
    mButtonCopy   = "Copy (F5)";
    mHelpDlgTitle = "Functions:";
    mNoFuncAvail  = "<No user functions available>";
    mError        = "Error";
    mErrorCall    = "Error: call";
    mErrorCompile = "Error: compile";
    mErrorSubcall = "Error: subcall";
    mErrorFormat  = "Error: format";
    mSyntax       = "Syntax:";
  };
  Russian = {
    mMainDlgTitle = "Lua-калькулятор";
    mLabelStatus  = "Статус:";
    mButtonCalc   = "Вычислить";
    mButtonInsert = "Вставить (Ins)";
    mButtonCopy   = "Скопировать (F5)";
    mHelpDlgTitle = "Функции:";
    mNoFuncAvail  = "<Нет пользовательских функций>";
    mError        = "Ошибка";
    mErrorCall    = "Ошибка: вызов";
    mErrorCompile = "Ошибка: компиляция";
    mErrorSubcall = "Ошибка: подвызов";
    mErrorFormat  = "Ошибка: формат";
    mSyntax       = "Синтаксис:";
  };
}

local M  -- message localization table
local F = far.Flags
local KEEP_DIALOG_OPEN = 0
local DISABLE_CHANGE = 0
local HOTKEY_IS_DONE = 0
local Send = far.SendDlgMessage

local enable_custom_hotkeys = true
if FAR3 then
  local farbuild = select(4, far.AdvControl("ACTL_GETFARMANAGERVERSION", true))
  enable_custom_hotkeys = (farbuild >= 4269)  -- DN_HOTKEY was fixed in LuaFAR build 483
end

local python  -- Lunatic Python module
local py_globals
local py_help

local function init_python(py)
  python = py
  python.execute "import math"
  python.execute "from math import *"
  python.execute "from inspect import getmembers"
  python.execute [[
def my_eval(txt):
    try: return eval(txt), None
    except Exception as E: return None, str(E)
]]
  python.execute [[
def get_math_list():
    tb = [ "math (Python):\n" ]
    ind = 0
    for tup in getmembers(math):
        if tup[0][0] != "_":
            if   ind % 8 == 0: tb.append("  " + tup[0] + ", ")
            elif ind % 8 == 7: tb.append(tup[0] + ",\n")
            else:              tb.append(tup[0] + ", ")
            ind += 1
    return "".join(tb)
]]
  py_globals = python.globals()
  py_help = py_globals.get_math_list()
end

-- tobase(10,2)    --> "1010"
-- tobase(200,16)  --> "C8"
-- tobase(-200,16) --> "-C8"
local function tobase(num, base)
  local floor, fmod = math.floor, math.fmod
  base = assert(base >=2 and base <= 36) and floor(base)
  local offset = string.byte("A") - 10
  local t = {""}
  if num < 0 then t[1],num = "-",-num end
  num = floor(num)
  repeat
    local r = fmod(num, base) -- don't use % operation (it may be Lua implementation dependent)
    num = (num - r) / base
    table.insert(t, 2, r<10 and tostring(r):sub(1,1) or string.char(r+offset)) -- sub() needed for Lua 5.3
  until num == 0
  return table.concat(t)
end

local function loadhelp()
  -- load user functions and help message
  local userlib, strhelp
  local ok,lib = pcall(require, "far2.fl_calc")
  if ok then
    userlib, strhelp = lib, lib.help
    if strhelp then
      local message={}
      for i=1,#strhelp,2 do
        table.insert(message, ('%-20.20s - %s'):format(strhelp[i], strhelp[i+1]))
      end
      table.sort(message)
      strhelp = table.concat(message,'\n')
    end
  end
  strhelp = strhelp or M.mNoFuncAvail

  -- also list functions available in math
  local tb = {}
  for k in pairs(math) do table.insert(tb,k) end
  table.sort(tb)
  for i=1,#tb-1 do
    local suffix = i%8==0 and ",\n  " or ", "
    tb[i] = tb[i]..suffix
  end
  strhelp = strhelp.."\n\1\nmath (Lua):\n  "..table.concat(tb)

  -- add help for special formatting feature
  strhelp = strhelp.."\n\1\n"..
    "Format hint:\n  #n (where n=2...36) gives radix n result representation"
  return userlib, strhelp
end

local function getdata(item) return item.text; end
local function setdata(item,data) item.text=data; end

local function calculator()
  local sDialog = require "far2.simpledialog"
  M = Lang[win.GetEnv("FARLANG")] or Lang.English
  local xx = M.mSyntax:len()+8
  local items = {
    guid="E7588240-0523-4AA5-8A31-EE829E20CD26";
    { tp="dbox";                  text=M.mMainDlgTitle;                                  },

    { tp="text";                  text=M.mSyntax; x1=7;                                  },
    { tp="rbutt"; name="lng_lua"; text="&Lua";    x1=xx;    y1=""; group=1; val=1;       },
    { tp="rbutt"; name="lng_c";   text="&C";      x1=xx+9;  y1="";                       },
    { tp="rbutt"; name="lng_py";  text="&Python"; x1=xx+16; y1=""; disable=not Lib_Python; },

    { tp="edit";  name="calc";    x1=7; hist='LuaCalc'; focus=1;                         },

    { tp="text";                  x1=5;  x2=5;  text="&0";                    ystep=0;   },
    { tp="text";                  x1=""; x2=""; text="&1";                               },
    { tp="text";                  x1=""; x2=""; text="&2";                               },
    { tp="text";                  x1=""; x2=""; text="&3";                               },
    { tp="text";                  x1=""; x2=""; text="&4";                               },

    { tp="rbutt"; name="decrad";  x1=7;  group=1; val=1;        Item="dec";   ystep=-3;  },
    { tp="rbutt"; name="octrad";  x1="";                        Item="oct";              },
    { tp="rbutt"; name="hexrad";  x1="";                        Item="hex";              },
    { tp="rbutt"; name="rawrad";  x1="";                        Item="raw";              },

    { tp="edit",  name="decfmt";  x1=11; x2=16; text='%g' ;     Fmt="dec";    ystep=-3;  },
    { tp="edit",  name="octfmt";  x1=""; x2=""; text='%o' ;     Fmt="oct";               },
    { tp="edit",  name="hexfmt";  x1=""; x2=""; text='%#x';     Fmt="hex";               },
    { tp="edit",  name="rawfmt";  x1=""; x2=""; text="%s" ;     Fmt="raw";               },

    { tp="text";                  x1=18; x2=18; text=":";                     ystep=-3;  },
    { tp="text";                  x1=""; x2=""; text=":";                                },
    { tp="text";                  x1=""; x2=""; text=":";                                },
    { tp="text";                  x1=""; x2=""; text=":";                                },

    { tp="text";  name="dec";     x1=20;             Update=1;  Fmt="decfmt"; ystep=-3;  },
    { tp="text";  name="oct";     x1="";             Update=1;  Fmt="octfmt";            },
    { tp="text";  name="hex";     x1="";             Update=1;  Fmt="hexfmt";            },
    { tp="text";  name="raw";     x1="";             Update=1;  Fmt="rawfmt";            },

    { tp="text";                  x1=5;  text=M.mLabelStatus;                            },
    { tp="text";  name="status";  x1=13; text='ok';  Update=1;                ystep=0;   },

    { tp="butt";  name="btnCalc"; centergroup=1; text=M.mButtonCalc; default=1;          },
    { tp="butt";  name="btnIns";  centergroup=1; text=M.mButtonInsert;                   },
    { tp="butt";  name="btnCopy"; centergroup=1; text=M.mButtonCopy;                     },
  }

  local dlg = sDialog.New(items)
  local dPos, dItems = dlg:Indexes()
  local curlang = nil
  local compiled = nil
  local result = 0
  local active_item = dItems.dec
  local userlib, strhelp = loadhelp()
  local environ = setmetatable({},
    {
      __index = function(t,k)
          return userlib and userlib[k] or math[k] or _G[k] or far.Flags[k]
      end
    })
  local cfunction = function(c) return environ[c] end
  local keys = { Enter="btnCalc"; Ins="btnIns"; F5 = "btnCopy"; }

  local function btnCalc_Action(hDlg)
    if tonumber(result) then
      Send(hDlg, F.DM_SETTEXT, dPos.calc, result)
    end
    return KEEP_DIALOG_OPEN
  end

  local function btnCopy_Action()
    far.CopyToClipboard(getdata(active_item))
    return KEEP_DIALOG_OPEN
  end

  local function btnIns_Action(hDlg)
    local txt = getdata(active_item)
    if txt ~= "" then
      far.MacroPost(("print(%q)"):format(txt))
    else
      return KEEP_DIALOG_OPEN
    end
  end

  local function format(item, res)
    if type(res) == 'number' then
      local fmt = getdata(dItems[item.Fmt])
      local base = tonumber(fmt:match("^#(%d+)"))
      if base and base>=2 and base <=36 then
        setdata(item, tobase(res, base))
      else
        local ok,str = pcall(string.format, fmt, res)
        setdata(item, ok and str or M.mErrorFormat)
      end
    elseif curlang == "Python" then
      local fmt = getdata(dItems[item.Fmt])
      local s = ("%q %% %s"):format(fmt, res)
      local s2 = py_globals.my_eval(s)
      setdata(item, s2[0] or "<error>")
    end
  end

  local function reset()
    setdata(dItems.dec, '')
    setdata(dItems.oct, '')
    setdata(dItems.hex, '')
    setdata(dItems.raw, '')
    setdata(dItems.status, nil)
    compiled = nil
    result = 0
  end

  local function compile(hDlg)
    local str = Send(hDlg, F.DM_GETTEXT, dPos.calc)
    if not str:find("%S") then str = "0" end

    if curlang == "Lua" then
       -- add parentheses to avoid tail call (that gives better error messages)
      compiled = loadstring('return ('..str..')') or loadstring(str)
    elseif curlang == "C" then
      local lib = FAR3 and "shmuz.c_calc" or "c_calc"
      compiled = loadstring(( [[
        local getvar = ...
        local calc = require(%q)
        return calc.expr("%s", getvar)]] ):format(lib, str))
    elseif curlang == "Python" then
      local txt = ("str(%s)"):format(str)
      local res = py_globals.my_eval(txt)
      if res[0] then
        compiled = res[0]
      else
        local txt = res[1]:match("(.-)%s*%(<string")
        setdata(dItems.status, txt or res[1])
      end
    end

    if curlang ~= "Python" then
      if compiled then setfenv(compiled, environ)
      else setdata(dItems.status, M.mErrorCompile)
      end
    end
  end

  local function call()
    if curlang=="Python" then
      result = compiled
    else
      if curlang=="Lua"   then result = compiled()
      elseif curlang=="C" then result = compiled(cfunction)
      end
      if type(result)=='function' then
        setdata(dItems.status, M.mErrorSubcall)
      elseif not tonumber(result) then
        setdata(dItems.status, M.mErrorCall)
      end
    end
  end

  local function form()
    if curlang ~= "Python" then
      result = tonumber(result) or ''
    end
    format(dItems.raw, result)
    format(dItems.dec, result)
    format(dItems.oct, result)
    format(dItems.hex, result)
    setdata(dItems.status, 'ok')
  end

  local chain={reset, compile, call, form}
  local function do_chain(hDlg)
    local ok, msg
    for _,f in ipairs(chain) do
      ok, msg = pcall(f, hDlg)
      if not ok or getdata(dItems.status) then break end
    end
    if not ok then
      reset()
      msg = type(msg)=="string" and msg or "error message is not a string"
      msg = string.match(msg, ".*:%d+: (.*)") or msg
      setdata(dItems.status, msg)
    end
    for i,v in ipairs(items) do
      if v.Update then Send(hDlg, F.DM_SETTEXT, i, getdata(items[i])) end
    end
  end

  local function get_language(hDlg)
    return Send(hDlg,F.DM_GETCHECK,dPos.lng_lua)==1 and "Lua" or
           Send(hDlg,F.DM_GETCHECK,dPos.lng_c)==1 and "C" or "Python"
  end

  local function SetFocusOnInput(hDlg)
    if FAR3 then
      Send(hDlg, F.DM_SETFOCUS, dPos.calc)
    else
      far.Timer(10, function(h) -- timer is used due to FAR2 bug
          h:Close()
          Send(hDlg, F.DM_SETFOCUS, dPos.calc)
        end)
    end
  end

  local keyaction = function(hDlg,p1,key)
    if key == "F1" then
      local txt = Send(hDlg, F.DM_GETCHECK, dPos.lng_py)==1 and py_help or strhelp
      far.Message(txt, M.mHelpDlgTitle, nil, 'l')
    else
      local name = keys[key]
      if name then Send(hDlg, F.DM_CLOSE, dPos[name]) end
    end
  end

  items.proc = function(hDlg,msg,p1,p2)
    if msg==F.DN_INITDIALOG then
      curlang = get_language(hDlg)
      do_chain(hDlg)
    ----------------------------------------------------------------------------
    elseif msg==F.DN_EDITCHANGE then
      setdata(items[p1], p2[10])
      if p1==dPos.calc then
        do_chain(hDlg)
      else
        local fmt=items[p1].Fmt
        if fmt then
          format(dItems[fmt], result)
          Send(hDlg, F.DM_SETTEXT, dPos[fmt], getdata(dItems[fmt]))
        end
      end
    ----------------------------------------------------------------------------
    elseif msg==F.DN_BTNCLICK then
      if p2 ~= 0 then
        local btn = items[p1]
        if btn.Item then
          active_item = dItems[btn.Item]
        elseif btn.name:find("^lng") then
          if btn.name == "lng_py" and not python then
            local ok, ret = pcall(require, Lib_Python)
            if ok then
              init_python(ret)
            else
              far.Message(ret:match("[^\n]+"), M.mError, nil, "w")
              SetFocusOnInput(hDlg)
              return DISABLE_CHANGE
            end
          end
          curlang = get_language(hDlg)
          SetFocusOnInput(hDlg)
          do_chain(hDlg)
        end
      end
    ----------------------------------------------------------------------------
    elseif msg==F.DN_HOTKEY then
      if enable_custom_hotkeys then
        local n
        if FAR3 then
          n = tonumber(p2.UnicodeChar)
        else
          n = p2 - F.KEY_ALT0
        end
        if n and n>=0 and n<=4 then
          if n==0 then
            Send(hDlg, F.DM_SETFOCUS, dPos.calc)
          else
            Send(hDlg, F.DM_SETCHECK, dPos.decrad+n-1, 1)
            Send(hDlg, F.DM_SETFOCUS, dPos.decfmt+n-1)
          end
          return HOTKEY_IS_DONE
        end
      end
    ----------------------------------------------------------------------------
    elseif msg=="EVENT_KEY" then
      return keyaction(hDlg, p1, p2)
    ----------------------------------------------------------------------------
    elseif msg==F.DN_CLOSE then
      if     p1 == dPos.btnCalc then  return btnCalc_Action(hDlg)
      elseif p1 == dPos.btnCopy then  return btnCopy_Action(hDlg)
      elseif p1 == dPos.btnIns  then  return btnIns_Action(hDlg)
      end
    end
  end

  if not enable_custom_hotkeys then
    for _,v in ipairs(items) do
      if v.text and v.text:match("^&[0-4]$") then v.text=v.text:sub(2) end
    end
  end
  dlg:Run()

end -- local function calculator()

return calculator
