local function bin2c (subj, name, len)
  local insert = table.insert
  name = name or "array"
  len = len or 18
  local tout = {}
  local count = 0
  insert(tout, "char ")
  insert(tout, name)
  insert(tout, "[] = {\n")
  for c in string.gmatch(subj, ".") do
    if count == 0 then insert(tout, "  ") end
    local v = string.byte(c)
    if v < 100 then insert(tout, " ") end
    insert(tout, v)
    insert(tout, ",")
    count = count + 1
    if count == len then count = 0; insert(tout, "\n"); end
  end
  if count ~= 0 then insert(tout, "\n") end
  insert(tout, "};\n")
  return table.concat(tout)
end

local function arrname(item, boot)
  local s
  if boot then
    return "boot"
  elseif item.script then
    s = item.name:gsub(".*/", "")
  else -- module
    s = item.name:gsub(".*%*", ""):gsub("/", "_")
  end
  return (s:gsub("%.[^.]*$", ""))
end

local function requirename(item)
  local s = item.name:gsub(".*%*", ""):gsub("/", ".")
  return (s:gsub("%.[^.]*$", ""))
end

local linit = [[
/*
** $Id: linit.c,v 1.14.1.1 2007/12/27 13:02:25 roberto Exp $
** Initialization of libraries for lua.c
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"

<$declarations>

static const luaL_Reg lualibs[] = {
<$binmodules>
<$modules>
<$scripts>
  {NULL, NULL}
};


int luafar_openlibs (lua_State *L) {
  const luaL_Reg *lib = lualibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }
  return 0;
}

]]

local code = [[
static int loader (lua_State *L) {
  void *arr = lua_touserdata(L, lua_upvalueindex(1));
  size_t arrsize = lua_tointeger(L, lua_upvalueindex(2));
  const char *name = lua_tostring(L,1);
  if (0 == luaL_loadbuffer(L, arr, arrsize, name)) {
    if (*name != '<') {  /* it's a module */
      lua_pushvalue(L,1);
      lua_call(L,1,1);
    }
    return 1;
  }
  return 0;
}

static int preload (lua_State *L, char *arr, size_t arrsize) {
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  lua_pushlightuserdata(L, arr);
  lua_pushinteger(L, arrsize);
  lua_pushcclosure(L, loader, 2);
  lua_setfield(L, -2, lua_tostring(L,1));
  lua_pop(L,2);
  return 0;
}

]]

local function addfiles(aLuafiles, target, method, luac)
  local strip = (method == "-strip") and require "lstrip51"
  for i, f in ipairs(aLuafiles) do
    local s
    local fname = f.name:gsub("%*", "/")
    if method == "-plain" then
      local fp = assert(io.open(fname))
      s = fp:read("*all")
      fp:close()
    elseif strip then
      s = assert(strip("fsk", fname))
    else
      assert(0 == os.execute((luac or "luac") .. " -o luac.out -s " .. fname))
      local fp = assert(io.open("luac.out", "rb"))
      s = fp:read("*all")
      fp:close()
    end
    s = "static " .. bin2c(s, arrname(f, i==1))
    target:write(s, "\n")
  end
end

local function create_linit (aLuafiles, aBinlibs)
  return function(tag)
    local ret = ""
    if tag == "declarations" then
      ret = "/*---- forward declarations ----*/\n"
      for _,libname in ipairs(aBinlibs) do
        ret = ret .. "int luaopen_" .. libname .. " (lua_State*);\n"
      end
      for i,f in ipairs(aLuafiles) do
        ret = ret .. "static int preload_" .. arrname(f,i==1) .. " (lua_State*);\n"
      end
    elseif tag == "binmodules" and #aBinlibs > 0 then
      ret = "  /*------ bin.modules ------*/\n"
      for _,libname in ipairs(aBinlibs) do
        ret = ('%s  {"%s", luaopen_%s},\n'):format(ret, libname, libname)
      end
    elseif tag == "modules" then
      ret = "  /*-------- modules --------*/\n"
      for _,v in ipairs(aLuafiles) do
        if v.module then
          ret = ("%s  {\"%s\", preload_%s},\n"):format(ret, requirename(v), arrname(v,false))
        end
      end
    elseif tag == "scripts" then
      ret = "  /*-------- scripts --------*/\n"
      for i,v in ipairs(aLuafiles) do
        if v.script then
          local name = arrname(v,i==1)
          ret = ret .. "  {\"<" .. name .. "\", preload_" .. name .. "},\n"
        end
      end
    end
    return ret
  end
end

local function generate(bootscript, scripts, modules, target, method, luac)
  local luafiles = { [1]={name=bootscript; script=true;} }
  for name in scripts:gmatch("%S+") do
    if name ~= "-" then table.insert(luafiles, {name=name; script=true;}) end
  end
  for name in modules:gmatch("%S+") do
    if name ~= "-" then table.insert(luafiles, {name=name; module=true;}) end
  end
  local binlibs = {}

  local fp = assert(io.open(target, "w"))
  --------------------------------------------------------------
  fp:write("/* This is a generated file. */\n\n")
  linit = linit:gsub("<$([^>]+)>", create_linit(luafiles, binlibs))
  fp:write(linit)
  fp:write(code)
  addfiles(luafiles, fp, method, luac)
  for i,f in ipairs(luafiles) do
    local name = arrname(f, i==1)
    fp:write(string.format([[
int preload_%s (lua_State *L)
    { return preload(L, %s, sizeof(%s)); }
]], name, name, name))
  end
  --------------------------------------------------------------
  fp:close()
end

if select("#", ...) <= 1 then -- called by require()
  return generate
else
  generate(...) -- use as an executable
end
