//---------------------------------------------------------------------------

#include <windows.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "util.h"
#include "ustring.h"

const char strFileHandle[] = "sysutils.file_handle";

// lua stack index of 1 is assumed
HANDLE* checkFileHandle(lua_State *L)
{
  HANDLE* pHandle = (HANDLE*)luaL_checkudata(L, 1, strFileHandle);
  if(*pHandle == INVALID_HANDLE_VALUE)
    luaL_error(L, "operation on closed file handle");
  return pHandle;
}

void registerFileHandle(lua_State *L, HANDLE handle)
{
  HANDLE *pHandle = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE));
  *pHandle = handle;
  luaL_getmetatable(L, strFileHandle);
  lua_setmetatable(L, -2);
}

static BOOL GetAccessAndShare(const char* str, DWORD *access, DWORD *share)
{
  BOOL ok = TRUE;
  const char *p = str;
  if     (!strcasecmp(p, "r"))  *access = GENERIC_READ;
  else if(!strcasecmp(p, "w"))  *access = GENERIC_WRITE;
  else if(!strcasecmp(p, "rw")) *access = GENERIC_READ | GENERIC_WRITE;
  else
  {
    if     (!strncasecmp(p, "r+", 2))  { *access = GENERIC_READ;  p += 2; }
    else if(!strncasecmp(p, "w+", 2))  { *access = GENERIC_WRITE; p += 2; }
    else if(!strncasecmp(p, "rw+", 3)) { *access = GENERIC_READ | GENERIC_WRITE; p += 3; }
    else ok = FALSE;

    if (ok)
    {
      if     (!strcasecmp(p, "co")) *share = FILE_SHARE_READ | FILE_SHARE_WRITE;
      else if(!strcasecmp(p, "ex")) *share = 0;
      else if(!strcasecmp(p, "dw")) *share = FILE_SHARE_READ;
      else if(!strcasecmp(p, "dr")) *share = FILE_SHARE_WRITE;
      else if(!strcasecmp(p, "dn")) *share = FILE_SHARE_READ | FILE_SHARE_WRITE;
      else ok = FALSE;
    }
  }
  return ok;
}

static BOOL DecodeDisposition(const char* str, DWORD *disposition)
{
  if      (!strcasecmp(str,"ca") || !strcasecmp(str,"CREATE_ALWAYS"))     *disposition=CREATE_ALWAYS;
  else if (!strcasecmp(str,"cn") || !strcasecmp(str,"CREATE_NEW"))        *disposition=CREATE_NEW;
  else if (!strcasecmp(str,"oa") || !strcasecmp(str,"OPEN_ALWAYS"))       *disposition=OPEN_ALWAYS;
  else if (!strcasecmp(str,"oe") || !strcasecmp(str,"OPEN_EXISTING"))     *disposition=OPEN_EXISTING;
  else if (!strcasecmp(str,"te") || !strcasecmp(str,"TRUNCATE_EXISTING")) *disposition=TRUNCATE_EXISTING;
  else return FALSE;
  return TRUE;
}

static int _CreateFile(lua_State *L, DWORD access, DWORD share, const char* dflt_disposition)
{
  DWORD dispos = OPEN_EXISTING;
  LPCWSTR fname = check_utf8_string(L,1,NULL);

  if (!lua_isnoneornil(L,2))
  {
    const char* p = luaL_checkstring(L,2);
    if (!GetAccessAndShare(p, &access, &share))
      luaL_argerror(L, 2, "must be: r|w|rw[+co|ex|dw|dr|dn]");
  }

  if (!DecodeDisposition(luaL_optstring(L,3,dflt_disposition), &dispos))
    luaL_argerror(L, 3, "invalid 'disposition'");

  DWORD attr = DecodeAttributes(luaL_optstring(L,4,""));

  HANDLE handle = WINPORT(CreateFile)(fname, access, share, NULL, dispos, attr, NULL);
  if(handle != INVALID_HANDLE_VALUE)
    registerFileHandle(L, handle);
  else
    return SysErrorReturn(L);
  return 1;
}

int su_FileOpen (lua_State *L)
{
  return _CreateFile(L, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, "OPEN_EXISTING");
}

int su_FileCreate (lua_State *L)
{
  return _CreateFile(L, GENERIC_WRITE, FILE_SHARE_READ, "OPEN_ALWAYS");
}

int su_FileClose (lua_State *L)
{
  HANDLE *pHandle = checkFileHandle(L);
  BOOL res = WINPORT(CloseHandle)(*pHandle);
  if(res) {
    *pHandle = INVALID_HANDLE_VALUE;
    // prevent: a) 2nd closing by garbage collection handler;
    //          b) repetitive closings by bad Lua scripts.
    return lua_pushboolean(L, 1), 1;
  }
  return SysErrorReturn(L);
}

int su_FileRead (lua_State *L)
{
  HANDLE *pHandle = checkFileHandle(L);
  DWORD count = luaL_checkinteger(L,2);
  luaL_argcheck(L, count > 0, 2, "must be positive number");
  char* buf = malloc(count);
  if (buf) {
    DWORD nRead;
    BOOL res = WINPORT(ReadFile)(*pHandle, buf, count, &nRead, NULL);
    if(res)
      nRead ? lua_pushlstring(L, buf, nRead) : lua_pushnil(L);
    free(buf);
    if (res) return 1;
  }
  return SysErrorReturn(L);
}

int su_FileWrite (lua_State *L)
{
  size_t len, count;
  HANDLE *pHandle = checkFileHandle(L);
  const char* buf  = luaL_checklstring(L, 2, &len);
  count = luaL_optinteger(L, 3, len);
  if (count > len)
    count = len;
  if (count > 0) {
    DWORD nWritten;
    BOOL res = WINPORT(WriteFile)(*pHandle, buf, count, &nWritten, NULL);
    return res ? (lua_pushnumber(L, nWritten), 1) : SysErrorReturn(L);
  }
  lua_pushnumber(L, 0);
  return 1;
}

// taken from MSDN help
int64_t myFileSeek (HANDLE hf, int64_t distance, DWORD MoveMethod)
{
   LARGE_INTEGER li;
   li.QuadPart = distance;
   li.LowPart = WINPORT(SetFilePointer) (hf, li.LowPart, &li.HighPart, MoveMethod);

   if (li.LowPart == INVALID_SET_FILE_POINTER && WINPORT(GetLastError()) != NO_ERROR)
   {
      li.QuadPart = -1;
   }
   return li.QuadPart;
}

int su_FileSeek (lua_State *L)
{
  HANDLE *pHandle = checkFileHandle(L);
  DWORD origin;

  const char* whence = luaL_optstring(L, 2, "cur");
  if      (!strcmp(whence, "set"))    origin = FILE_BEGIN;
  else if (!strcmp(whence, "cur"))    origin = FILE_CURRENT;
  else if (!strcmp(whence, "end"))    origin = FILE_END;
  else return luaL_argerror(L, 2, "invalid parameter");

  int64_t offset = (int64_t)luaL_optnumber(L, 3, 0); // double -> int64_t

  int64_t pos = myFileSeek(*pHandle, offset, origin);
  return (pos >= 0) ? (lua_pushnumber(L, pos), 1) : SysErrorReturn(L);
}

int su_SetEndOfFile(lua_State *L)
{
  HANDLE *pHandle = checkFileHandle(L);
  BOOL res = WINPORT(SetEndOfFile)(*pHandle);
  lua_pushboolean(L, res);
  return res ? 1 : SysErrorReturn(L);
}

int su_FlushFileBuffers(lua_State *L)
{
  HANDLE *pHandle = checkFileHandle(L);
  BOOL res = WINPORT(FlushFileBuffers)(*pHandle);
  lua_pushboolean(L, res);
  return res ? 1 : SysErrorReturn(L);
}

// helper function
double L64toDouble (unsigned low, unsigned high)
{
  double result = low;
  if(high)
  {
    LARGE_INTEGER large;
    large.LowPart = low;
    large.HighPart = high;
    result = large.QuadPart;
  }
  return result;
}

const luaL_Reg su_funcs[] = {
  // operations on a single file
  {"FileCreate",        su_FileCreate},        //unicode
  {"FileOpen",          su_FileOpen},          //unicode

  {NULL, NULL}
};

int gc_FileHandle (lua_State *L)
{
  HANDLE *pHandle = (HANDLE*)lua_touserdata(L, 1); // no need to check here, IMO
  if(*pHandle != INVALID_HANDLE_VALUE)
    WINPORT(CloseHandle)(*pHandle);
  return 0;
}

static const luaL_Reg FileHandle_funcs[] = {
  {"close",             su_FileClose},
  {"read",              su_FileRead},
  {"write",             su_FileWrite},
  {"seek",              su_FileSeek},
  {"SetEndOfFile",      su_SetEndOfFile},
  {"FlushFileBuffers",  su_FlushFileBuffers},
  {"__gc",              gc_FileHandle},
  {NULL, NULL}
};

void createmeta(lua_State *L, const char *name)
{
  luaL_newmetatable(L, name);   /* create new metatable */
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);         /* push metatable */
  lua_rawset(L, -3);            /* metatable.__index = metatable */
}

int luaopen_sysutils (lua_State *L)
{
  createmeta(L, strFileHandle);
  luaL_register(L, NULL, FileHandle_funcs);
  luaL_register(L, "win", su_funcs);
  return 1;
}
