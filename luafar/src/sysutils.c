//---------------------------------------------------------------------------

#include <windows.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "util.h"
#include "ustring.h"

static const char strFileHandle[] = "sysutils.file_handle";

// lua stack index of 1 is assumed
static HANDLE* checkFileHandle(lua_State *L)
{
	HANDLE* pHandle = (HANDLE*)luaL_checkudata(L, 1, strFileHandle);
	if (*pHandle == INVALID_HANDLE_VALUE)
		luaL_error(L, "operation on closed file handle");
	return pHandle;
}

static void registerFileHandle(lua_State *L, HANDLE handle)
{
	HANDLE *pHandle = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE));
	*pHandle = handle;
	luaL_getmetatable(L, strFileHandle);
	lua_setmetatable(L, -2);
}

static BOOL GetFileModes(const char* str, DWORD *access, DWORD *share, DWORD *disposition)
{
	BOOL append = FALSE;
	*share = FILE_SHARE_READ; // default

	const char *p = str;
	if      (!strcmp(p,"r")  || !strcmp(p,"rb"))  { *access = GENERIC_READ; *share |= FILE_SHARE_WRITE; }
	else if (!strcmp(p,"w")  || !strcmp(p,"wb"))    *access = GENERIC_WRITE;
	else if (!strcmp(p,"a")  || !strcmp(p,"ab"))  { *access = GENERIC_WRITE; append = TRUE; }
	else
		return FALSE;

	if (append)                        *disposition = OPEN_ALWAYS;
	else if (*access & GENERIC_WRITE)  *disposition = CREATE_ALWAYS;
	else                               *disposition = OPEN_EXISTING;

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

	const char* strMode = luaL_optstring(L, 2, "r");
	if (!GetFileModes(strMode, &access, &share, &dispos))
		luaL_argerror(L, 2, "invalid mode");

	const char *strAttr = "";
	DWORD attr = DecodeAttributes(strAttr);

	HANDLE handle = WINPORT(CreateFile)(fname, access, share, NULL, dispos, attr, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return SysErrorReturn(L);

	if (strMode[0] == 'a') // append
		SetFilePointer(handle, 0, NULL, FILE_END);

	registerFileHandle(L, handle);
	return 1;
}

static int su_FileClose (lua_State *L)
{
	HANDLE *pHandle = checkFileHandle(L);
	BOOL res = WINPORT(CloseHandle)(*pHandle);
	if (res) {
		*pHandle = INVALID_HANDLE_VALUE;
		// prevent: a) 2nd closing by garbage collection handler;
		//          b) repetitive closings by bad Lua scripts.
		return lua_pushboolean(L, 1), 1;
	}
	return SysErrorReturn(L);
}

static int su_FileRead (lua_State *L)
{
	HANDLE *pHandle = checkFileHandle(L);
	DWORD count = luaL_checkinteger(L,2);
	luaL_argcheck(L, count > 0, 2, "must be positive number");
	char* buf = malloc(count);
	if (buf) {
		DWORD nRead;
		BOOL res = WINPORT(ReadFile)(*pHandle, buf, count, &nRead, NULL);
		if (res)
			nRead ? lua_pushlstring(L, buf, nRead) : lua_pushnil(L);
		free(buf);
		if (res) return 1;
	}
	return SysErrorReturn(L);
}

static int su_FileWrite (lua_State *L)
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
static int64_t myFileSeek (HANDLE hf, int64_t distance, DWORD MoveMethod)
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

static int su_FileSeek (lua_State *L)
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

static int su_SetEndOfFile(lua_State *L)
{
	HANDLE *pHandle = checkFileHandle(L);
	BOOL res = WINPORT(SetEndOfFile)(*pHandle);
	lua_pushboolean(L, res);
	return res ? 1 : SysErrorReturn(L);
}

static int su_FlushFileBuffers(lua_State *L)
{
	HANDLE *pHandle = checkFileHandle(L);
	BOOL res = WINPORT(FlushFileBuffers)(*pHandle);
	lua_pushboolean(L, res);
	return res ? 1 : SysErrorReturn(L);
}

static int gc_FileHandle (lua_State *L)
{
	HANDLE *pHandle = (HANDLE*)lua_touserdata(L, 1); // no need to check here, IMO
	if (*pHandle != INVALID_HANDLE_VALUE)
		WINPORT(CloseHandle)(*pHandle);
	return 0;
}

static const luaL_Reg su_funcs[] = {
	// operations on a single file
	{"OpenFile", su_OpenFile},          //unicode

	{NULL, NULL}
};

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

static void createmeta(lua_State *L, const char *name)
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
