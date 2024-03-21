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
	*share = FILE_SHARE_READ; // default

	const char *p = str;
	if      (!strcmp(p,"r")  || !strcmp(p,"rb"))  { *access = GENERIC_READ; *share |= FILE_SHARE_WRITE; }
	else if (!strcmp(p,"w")  || !strcmp(p,"wb"))    *access = GENERIC_WRITE;
	else if (!strcmp(p,"rw") || !strcmp(p,"rwb"))   *access = GENERIC_READ | GENERIC_WRITE;
	else
	{
		return FALSE;
		// if      (!strncmp(p, "r+",  2))  { *access = GENERIC_READ;  p += 2; }
		// else if (!strncmp(p, "w+",  2))  { *access = GENERIC_WRITE; p += 2; }
		// else if (!strncmp(p, "rw+", 3))  { *access = GENERIC_READ | GENERIC_WRITE; p += 3; }
		// else return FALSE;
		//
		// if      (!strcmp(p, "co"))  *share = FILE_SHARE_READ | FILE_SHARE_WRITE; // compatibility mode
		// else if (!strcmp(p, "ex"))  *share = 0;                                  // exclusive access
		// else if (!strcmp(p, "dw"))  *share = FILE_SHARE_READ;                    // deny write
		// else if (!strcmp(p, "dr"))  *share = FILE_SHARE_WRITE;                   // deny read
		// else if (!strcmp(p, "dn"))  *share = FILE_SHARE_READ | FILE_SHARE_WRITE; // deny none
		// else return FALSE;
	}
	return TRUE;
}

static BOOL DecodeDisposition(const char* str, DWORD access, DWORD *disposition)
{
	if (*str == 0) {
		if (access & GENERIC_WRITE)
			*disposition = CREATE_ALWAYS;
		else
			*disposition = OPEN_EXISTING;
	}
	// else if (!strcmp(str, "ca"))  *disposition = CREATE_ALWAYS;
	// else if (!strcmp(str, "cn"))  *disposition = CREATE_NEW;
	// else if (!strcmp(str, "oa"))  *disposition = OPEN_ALWAYS;
	// else if (!strcmp(str, "oe"))  *disposition = OPEN_EXISTING;
	// else if (!strcmp(str, "te"))  *disposition = TRUNCATE_EXISTING;
	// else return FALSE;

	return TRUE;
}

//		HANDLE CreateFileA(
//		  [in]           LPCSTR                lpFileName,
//		  [in]           DWORD                 dwDesiredAccess,
//		  [in]           DWORD                 dwShareMode,
//		  [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes,
//		  [in]           DWORD                 dwCreationDisposition,
//		  [in]           DWORD                 dwFlagsAndAttributes,
//		  [in, optional] HANDLE                hTemplateFile
//		);

//		HANDLE WINPORT(CreateFile)(
//			               LPCWSTR               lpFileName,
//			               DWORD                 dwDesiredAccess,
//			               DWORD                 dwShareMode,
//			               const DWORD*          UnixMode,
//			               DWORD                 dwCreationDisposition,
//			               DWORD                 dwFlagsAndAttributes,
//			               HANDLE                hTemplateFile
//		);

static int su_OpenFile(lua_State *L)
{
	DWORD access=0, share=0, dispos=0;

	const wchar_t *fname = check_utf8_string(L, 1, NULL);

	const char* strAS = luaL_optstring(L, 2, "r");
	if (!GetAccessAndShare(strAS, &access, &share))
		luaL_argerror(L, 2, "must be: r|rb|w|wb|rw|rwb");
		//luaL_argerror(L, 2, "must be: r|w|rw[+co|ex|dw|dr|dn]");

	const char *strDis = ""; // luaL_optstring(L, 3, "");
	if (!DecodeDisposition(strDis, access, &dispos))
		luaL_argerror(L, 3, "invalid 'disposition'");

	const char *strAttr = ""; // luaL_optstring(L, 4, "");
	DWORD attr = DecodeAttributes(strAttr);

	HANDLE handle = WINPORT(CreateFile)(fname, access, share, NULL, dispos, attr, NULL);
	if(handle == INVALID_HANDLE_VALUE)
		return SysErrorReturn(L);

	registerFileHandle(L, handle);
	return 1;
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
	HANDLE *pHandle = checkFileHandle(L);
	int idx;
	int top = lua_gettop(L);
	for (idx=2; idx <= top; idx++)
	{
		size_t len;
		const char* buf = luaL_checklstring(L, idx, &len);
		if (len > 0) {
			DWORD nWritten;
			if (FALSE == WINPORT(WriteFile)(*pHandle, buf, len, &nWritten, NULL))
				return SysErrorReturn(L);
		}
	}
	return 0;
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
	{"OpenFile", su_OpenFile},          //unicode

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
