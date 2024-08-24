#include <sys/utsname.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "service.h"
#include "ustring.h"
#include "util.h"
#include "bit64.h"
#include "farlibs.h"

static BOOL dir_exist(const wchar_t* path)
{
	DWORD attr = WINPORT(GetFileAttributes)(path);
	return (attr != 0xFFFFFFFF) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

// os.getenv does not always work correctly, hence the following.
static int win_GetEnv (lua_State *L)
{
	const char* name = luaL_checkstring(L, 1);
	const char* val = getenv(name);
	if (val) lua_pushstring(L, val);
	else lua_pushnil(L);
	return 1;
}

static int win_SetEnv (lua_State *L)
{
	const char* name = luaL_checkstring(L, 1);
	const char* value = luaL_optstring(L, 2, NULL);
	int res = value ? setenv(name, value, 1) : unsetenv(name);
	lua_pushboolean (L, res == 0);
	return 1;
}

// Based on "CheckForEsc" function, by Ivan Sintyurin (spinoza@mail.ru)
static WORD ExtractKey(INPUT_RECORD* rec)
{
	DWORD ReadCount;
	HANDLE hConInp = NULL; //GetStdHandle(STD_INPUT_HANDLE);

	if (WINPORT(PeekConsoleInput)(hConInp,rec,1,&ReadCount), ReadCount)
	{
		WINPORT(ReadConsoleInput)(hConInp,rec,1,&ReadCount);
		if (rec->EventType==KEY_EVENT)
			return 1;
	}
	return 0;
}

// result = ExtractKey()
// -- general purpose function; not FAR dependent
static int win_ExtractKey(lua_State *L)
{
	INPUT_RECORD rec;
	if (ExtractKey(&rec) && rec.Event.KeyEvent.bKeyDown)
	{
		WORD vKey = rec.Event.KeyEvent.wVirtualKeyCode & 0xff;
		if (vKey && VirtualKeyStrings[vKey])
		{
			lua_pushstring(L, VirtualKeyStrings[vKey]);
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

static int win_ExtractKeyEx(lua_State *L)
{
	INPUT_RECORD rec;
	if (ExtractKey(&rec))
	{
		PushInputRecord(L, &rec);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

static void PushWinFindData (lua_State *L, const WIN32_FIND_DATAW *FData)
{
	lua_createtable(L, 0, 7);
	PutAttrToTable    (L,                      FData->dwFileAttributes);
	PutNumToTable     (L, "UnixMode",          FData->dwUnixMode);
	PutNumToTable     (L, "FileSize",          FData->nFileSize);
	PutNumToTable     (L, "PhysicalSize",      FData->nPhysicalSize);
	PutFileTimeToTable(L, "LastWriteTime",     FData->ftLastWriteTime);
	PutFileTimeToTable(L, "LastAccessTime",    FData->ftLastAccessTime);
	PutFileTimeToTable(L, "CreationTime",      FData->ftCreationTime);
	PutWStrToTable    (L, "FileName",          FData->cFileName, -1);
}

static int win_GetFileInfo (lua_State *L)
{
	WIN32_FIND_DATAW fd;
	const wchar_t *fname = check_utf8_string(L, 1, NULL);
	HANDLE h = WINPORT(FindFirstFile)(fname, &fd);
	if (h == INVALID_HANDLE_VALUE)
		lua_pushnil(L);
	else {
		PushWinFindData(L, &fd);
		WINPORT(FindClose)(h);
	}
	return 1;
}

static void pushSystemTime (lua_State *L, const SYSTEMTIME *st)
{
	lua_createtable(L, 0, 8);
	PutIntToTable(L, "wYear", st->wYear);
	PutIntToTable(L, "wMonth", st->wMonth);
	PutIntToTable(L, "wDayOfWeek", st->wDayOfWeek);
	PutIntToTable(L, "wDay", st->wDay);
	PutIntToTable(L, "wHour", st->wHour);
	PutIntToTable(L, "wMinute", st->wMinute);
	PutIntToTable(L, "wSecond", st->wSecond);
	PutIntToTable(L, "wMilliseconds", st->wMilliseconds);
}

static int win_GetSystemTime(lua_State *L)
{
	SYSTEMTIME st;
	WINPORT(GetSystemTime)(&st);
	pushSystemTime(L, &st);
	return 1;
}

static int win_GetLocalTime(lua_State *L)
{
	SYSTEMTIME st;
	WINPORT(GetLocalTime)(&st);
	pushSystemTime(L, &st);
	return 1;
}

static void pushFileTime (lua_State *L, const FILETIME *ft)
{
	long long llFileTime = ft->dwLowDateTime + 0x100000000ll * ft->dwHighDateTime;
	llFileTime /= 10000;
	lua_pushnumber(L, (double)llFileTime);
}

static int win_GetSystemTimeAsFileTime (lua_State *L)
{
	FILETIME ft;
	WINPORT(GetSystemTimeAsFileTime)(&ft);
	pushFileTime(L, &ft);
	return 1;
}

static int win_FileTimeToSystemTime (lua_State *L)
{
	FILETIME ft;
	SYSTEMTIME st;
	long long llFileTime = 10000 * (long long) luaL_checknumber(L, 1);
	ft.dwLowDateTime = llFileTime & 0xFFFFFFFF;
	ft.dwHighDateTime = llFileTime >> 32;
	if (! WINPORT(FileTimeToSystemTime)(&ft, &st))
		return lua_pushnil(L), 1;
	pushSystemTime(L, &st);
	return 1;
}

static int win_SystemTimeToFileTime (lua_State *L)
{
	FILETIME ft;
	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 1);
	st.wYear         = GetOptIntFromTable(L, "wYear", 0);
	st.wMonth        = GetOptIntFromTable(L, "wMonth", 0);
	st.wDayOfWeek    = GetOptIntFromTable(L, "wDayOfWeek", 0);
	st.wDay          = GetOptIntFromTable(L, "wDay", 0);
	st.wHour         = GetOptIntFromTable(L, "wHour", 0);
	st.wMinute       = GetOptIntFromTable(L, "wMinute", 0);
	st.wSecond       = GetOptIntFromTable(L, "wSecond", 0);
	st.wMilliseconds = GetOptIntFromTable(L, "wMilliseconds", 0);
	if (! WINPORT(SystemTimeToFileTime)(&st, &ft))
		return lua_pushnil(L), 1;
	pushFileTime(L, &ft);
	return 1;
}

static int win_FileTimeToLocalFileTime(lua_State *L)
{
	FILETIME ft, local_ft;
	long long llFileTime = (long long) luaL_checknumber(L, 1);
	llFileTime *= 10000; // convert from milliseconds to 1e-7

	ft.dwLowDateTime = llFileTime & 0xFFFFFFFF;
	ft.dwHighDateTime = llFileTime >> 32;

	if (WINPORT(FileTimeToLocalFileTime)(&ft, &local_ft))
		pushFileTime(L, &local_ft);
	else
		return SysErrorReturn(L);

	return 1;
}

static int win_CompareString (lua_State *L)
{
	size_t len1, len2;
	const wchar_t *ws1  = check_utf8_string(L, 1, &len1);
	const wchar_t *ws2  = check_utf8_string(L, 2, &len2);
	const char *sLocale = luaL_optstring(L, 3, "");
	const char *sFlags  = luaL_optstring(L, 4, "");

	LCID Locale = LOCALE_USER_DEFAULT;
	if      (!strcmp(sLocale, "s")) Locale = LOCALE_SYSTEM_DEFAULT;
	else if (!strcmp(sLocale, "n")) Locale = 0x0000; // LOCALE_NEUTRAL;

	DWORD dwFlags = 0;
	if (strchr(sFlags, 'c')) dwFlags |= NORM_IGNORECASE;
	if (strchr(sFlags, 'k')) dwFlags |= NORM_IGNOREKANATYPE;
	if (strchr(sFlags, 'n')) dwFlags |= NORM_IGNORENONSPACE;
	if (strchr(sFlags, 's')) dwFlags |= NORM_IGNORESYMBOLS;
	if (strchr(sFlags, 'w')) dwFlags |= NORM_IGNOREWIDTH;
	if (strchr(sFlags, 'S')) dwFlags |= SORT_STRINGSORT;

	int result = WINPORT(CompareString)(Locale, dwFlags, ws1, len1, ws2, len2) - 2;
	(result == -2) ? lua_pushnil(L) : lua_pushinteger(L, result);
	return 1;
}

static int win_wcscmp (lua_State *L)
{
	const wchar_t *ws1  = check_utf8_string(L, 1, NULL);
	const wchar_t *ws2  = check_utf8_string(L, 2, NULL);
	int insens = lua_toboolean(L, 3);
	lua_pushinteger(L, (insens ? wcscasecmp : wcscmp)(ws1, ws2));
	return 1;
}

static int win_GetVirtualKeys (lua_State *L)
{
	NewVirtualKeyTable(L, TRUE);
	return 1;
}

static int win_GetConsoleScreenBufferInfo (lua_State* L)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE h = NULL; // GetStdHandle(STD_OUTPUT_HANDLE); //TODO: probably incorrect
	if (!WINPORT(GetConsoleScreenBufferInfo)(h, &info))
		return lua_pushnil(L), 1;
	lua_createtable(L, 0, 11);
	PutIntToTable(L, "SizeX",              info.dwSize.X);
	PutIntToTable(L, "SizeY",              info.dwSize.Y);
	PutIntToTable(L, "CursorPositionX",    info.dwCursorPosition.X);
	PutIntToTable(L, "CursorPositionY",    info.dwCursorPosition.Y);
	PutIntToTable(L, "Attributes",         info.wAttributes);
	PutIntToTable(L, "WindowLeft",         info.srWindow.Left);
	PutIntToTable(L, "WindowTop",          info.srWindow.Top);
	PutIntToTable(L, "WindowRight",        info.srWindow.Right);
	PutIntToTable(L, "WindowBottom",       info.srWindow.Bottom);
	PutIntToTable(L, "MaximumWindowSizeX", info.dwMaximumWindowSize.X);
	PutIntToTable(L, "MaximumWindowSizeY", info.dwMaximumWindowSize.Y);
	return 1;
}

static int win_CopyFile (lua_State *L)
{
	FILE *inp, *out;
	int err;
	char buf[0x2000]; // 8 KiB
	const char* src = luaL_checkstring(L, 1);
	const char* trg = luaL_checkstring(L, 2);

	// a primitive (not sufficient) check but better than nothing
	if (!strcmp(src, trg)) {
		lua_pushnil(L);
		lua_pushstring(L, "input and output files are the same");
		return 2;
	}

	if (lua_gettop(L) > 2) {
		int fail_if_exists = lua_toboolean(L,3);
		if (fail_if_exists && (out=fopen(trg,"r"))) {
			fclose(out);
			lua_pushnil(L);
			lua_pushstring(L, "output file already exists");
			return 2;
		}
	}

	if (!(inp = fopen(src, "rb"))) {
		lua_pushnil(L);
		lua_pushstring(L, "cannot open input file");
		return 2;
	}

	if (!(out = fopen(trg, "wb"))) {
		fclose(inp);
		lua_pushnil(L);
		lua_pushstring(L, "cannot open output file");
		return 2;
	}

	while(1) {
		size_t rd, wr;
		rd = fread(buf, 1, sizeof(buf), inp);
		if (rd && (wr = fwrite(buf, 1, rd, out)) < rd)
			break;
		if (rd < sizeof(buf))
			break;
	}

	err = ferror(inp) || ferror(out);
	fclose(out);
	fclose(inp);
	if (!err) {
		lua_pushboolean(L,1);
		return 1;
	}
	lua_pushnil(L);
	lua_pushstring(L, "some error occured");
	return 2;
}

static int win_MoveFile (lua_State *L)
{
	const wchar_t* src = check_utf8_string(L, 1, NULL);
	const wchar_t* trg = check_utf8_string(L, 2, NULL);
	const char* sFlags = luaL_optstring(L, 3, NULL);
	int flags = 0;
	if (sFlags) {
		if (strchr(sFlags, 'c')) flags |= MOVEFILE_COPY_ALLOWED;
		if (strchr(sFlags, 'd')) flags |= MOVEFILE_DELAY_UNTIL_REBOOT;
		if (strchr(sFlags, 'r')) flags |= MOVEFILE_REPLACE_EXISTING;
		if (strchr(sFlags, 'w')) flags |= MOVEFILE_WRITE_THROUGH;
	}
	if (WINPORT(MoveFileEx)(src, trg, flags))
		return lua_pushboolean(L, 1), 1;
	return SysErrorReturn(L);
}

static int win_RenameFile (lua_State *L)
{
	return win_MoveFile(L);
}

static int win_DeleteFile (lua_State *L)
{
	if (WINPORT(DeleteFile)(check_utf8_string(L, 1, NULL)))
		return lua_pushboolean(L, 1), 1;
	return SysErrorReturn(L);
}

static BOOL makedir (const wchar_t* path)
{
	BOOL result = FALSE;
	const wchar_t* src = path;
	wchar_t *p = wcsdup(path), *trg = p;
	while (*src) {
		if (*src == L'/') {
			*trg++ = L'/';
			do src++; while (*src == L'/');
		}
		else *trg++ = *src++;
	}
	if (trg > p && trg[-1] == '/') trg--;
	*trg = 0;

	wchar_t* q;
	for (q=p; *q; *q++=L'/') {
		q = wcschr(q, L'/');
		if (q != NULL)  *q = 0;
		if (q != p && !dir_exist(p) && !WINPORT(CreateDirectory)(p, NULL)) break;
		if (q == NULL) { result=TRUE; break; }
	}
	free(p);
	return result;
}

static int win_CreateDir (lua_State *L)
{
	const wchar_t* path = check_utf8_string(L, 1, NULL);
	BOOL tolerant = lua_toboolean(L, 2);
	if (dir_exist(path)) {
		if (tolerant) return lua_pushboolean(L,1), 1;
		return lua_pushnil(L), lua_pushliteral(L, "directory already exists"), 2;
	}
	if (makedir(path))
		return lua_pushboolean(L, 1), 1;
	return SysErrorReturn(L);
}

static int win_RemoveDir (lua_State *L)
{
	if (WINPORT(RemoveDirectory)(check_utf8_string(L, 1, NULL)))
		return lua_pushboolean(L, 1), 1;
	return SysErrorReturn(L);
}

static int win_IsProcess64bit(lua_State *L)
{
	lua_pushboolean(L, sizeof(void*) == 8);
	return 1;
}

static int win_ExpandEnv (lua_State *L)
{
	const char *p = luaL_checkstring(L,1), *q, *r, *s;
	int remove = lua_toboolean(L,2);
	luaL_Buffer buf;
	luaL_buffinit(L, &buf);
	for (; *p; p=r+1) {
		if ( (q = strstr(p, "$(")) && (r = strchr(q+2, ')')) ) {
			lua_pushlstring(L, q+2, r-q-2);
			s = getenv(lua_tostring(L,-1));
			lua_pop(L,1);
			if (s) {
				luaL_addlstring(&buf, p, q-p);
				luaL_addstring(&buf, s);
			}
			else
				luaL_addlstring(&buf, p, remove ? q-p : r+1-p);
		}
		else {
			luaL_addstring(&buf, p);
			break;
		}
	}
	luaL_pushresult(&buf);
	return 1;
}

#if 0
static int win_GetTimeZoneInformation (lua_State *L)
{
	TIME_ZONE_INFORMATION tzi;
	DWORD res = GetTimeZoneInformation(&tzi);
	if (res == 0xFFFFFFFF)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 5);
	PutNumToTable(L, "Bias", tzi.Bias);
	PutNumToTable(L, "StandardBias", tzi.StandardBias);
	PutNumToTable(L, "DaylightBias", tzi.DaylightBias);
	PutLStrToTable(L, "StandardName", tzi.StandardName, sizeof(WCHAR)*wcslen(tzi.StandardName));
	PutLStrToTable(L, "DaylightName", tzi.DaylightName, sizeof(WCHAR)*wcslen(tzi.DaylightName));

	lua_pushnumber(L, res);
	return 2;
}
#endif

static int win_Sleep (lua_State *L)
{
	unsigned usec = (unsigned) (luaL_checknumber(L,1) * 1000); // msec -> mcsec
	usleep(usec);
	return 0;
}

static int win_Clock (lua_State *L)
{
	struct timespec ts;
	if (0 != clock_gettime(CLOCK_MONOTONIC, &ts))
		luaL_error(L, "clock_gettime failed");
	lua_pushnumber(L, ts.tv_sec + (double)ts.tv_nsec/1e9);
	return 1;
}

static int win_GetCurrentDir (lua_State *L)
{
	char *buf = (char*)lua_newuserdata(L, PATH_MAX*2);
	char *dir = getcwd(buf, PATH_MAX*2);
	if (dir) lua_pushstring(L,dir); else lua_pushnil(L);
	return 1;
}

static int win_SetCurrentDir (lua_State *L)
{
	const char *dir = luaL_checkstring(L,1);
	lua_pushboolean(L, chdir(dir) == 0);
	return 1;
}

static int win_system(lua_State *L)
{
	const char *str = luaL_optstring(L, 1, NULL);
	lua_pushinteger(L, system(str));
	return 1;
}

static int win_EnsureColorsAreInverted(lua_State *L)
{
	SHORT x = (SHORT) luaL_checkinteger(L,1);
	SHORT y = (SHORT) luaL_checkinteger(L,2);
	CHAR_INFO ci = {};
	SMALL_RECT Rect = {x, y, x, y};
	COORD Coord1={1,1}, Coord0={0,0}, CoordXY={x,y};
	WINPORT(ReadConsoleOutput)(0, &ci, Coord1, Coord0, &Rect);

	if (ci.Attributes & COMMON_LVB_REVERSE_VIDEO)
		return 0;	// this cell is already tweaked during prev paint

	DWORD64 InvColors = COMMON_LVB_REVERSE_VIDEO;

	InvColors|= ((ci.Attributes & 0x0f) << 4) | ((ci.Attributes & 0xf0) >> 4);

	InvColors|= (ci.Attributes & (COMMON_LVB_UNDERSCORE | COMMON_LVB_STRIKEOUT));

	if (ci.Attributes & FOREGROUND_TRUECOLOR) {
		SET_RGB_BACK(InvColors, GET_RGB_FORE(ci.Attributes));
	}

	if (ci.Attributes & BACKGROUND_TRUECOLOR) {
		SET_RGB_FORE(InvColors, GET_RGB_BACK(ci.Attributes));
	}

	DWORD NumberOfAttrsWritten = 0;
	WINPORT(FillConsoleOutputAttribute) (0, InvColors, 1, CoordXY, &NumberOfAttrsWritten);
	return 0;
}

static int win_JoinPath(lua_State *L)
{
	const int DELIM = '/';
	int empty=1, was_slash=0;
	int top = lua_gettop(L), idx;
	luaL_Buffer buf;
	luaL_buffinit(L, &buf);

	for (idx=1; idx <= top; idx++) {
		const char *s = luaL_optstring(L, idx, "");
		if (*s == 0)
			continue;
		if (!empty && !was_slash && *s != DELIM)
			luaL_addchar(&buf, DELIM);
		else if (was_slash && *s == DELIM)
			s++;
		luaL_addstring(&buf, s);
		was_slash = s[(int)strlen(s) - 1] == DELIM;
		empty = 0;
	}
	luaL_pushresult(&buf);
	return 1;
}

static void PutFileTimeToTableEx(lua_State *L, const FILETIME *FT, const char *key)
{
	INT64 FileTime = FT->dwLowDateTime + 0x100000000LL * FT->dwHighDateTime;
	bit64_push(L, FileTime);
	lua_setfield(L, -2, key);
}

static int win_GetFileTimes(lua_State *L)
{
	int res = 0;
	const wchar_t* FileName = check_utf8_string(L, 1, NULL);
	DWORD attr = GetFileAttributes(FileName);
	if (attr != INVALID_FILE_ATTRIBUTES)
	{
		DWORD flags = (attr & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : 0;
		HANDLE hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,flags,NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			FILETIME t_create, t_access, t_write;
			if (GetFileTime(hFile, &t_create, &t_access, &t_write))
			{
				lua_createtable(L, 0, 3);
				PutFileTimeToTableEx(L, &t_create, "CreationTime");
				PutFileTimeToTableEx(L, &t_access, "LastAccessTime");
				PutFileTimeToTableEx(L, &t_write,  "LastWriteTime");
				res = 1;
			}
			CloseHandle(hFile);
		}
	}
	if (res == 0)
		lua_pushnil(L);
	return 1;
}

static int ExtractFileTime(lua_State *L, const char *key, FILETIME* target, HANDLE hFile)
{
	int success = 0;
	lua_getfield(L, -1, key);
	if (!lua_isnil(L, -1))
	{
		INT64 DateTime = check64(L, -1, &success);
		if (success)
		{
			target->dwLowDateTime = DateTime & 0xFFFFFFFF;
			target->dwHighDateTime = DateTime >> 32;
		}
		else
		{
			if (hFile != INVALID_HANDLE_VALUE)
				CloseHandle(hFile);
			lua_pushfstring(L, "invalid value at key '%s'", key);
			return luaL_error(L, lua_tostring(L, -1));
		}
	}
	lua_pop(L, 1);
	return success;
}

static int win_SetFileTimes(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_pushvalue(L, 1); //duplicate as check_utf8_string() will destroy it

	int res = 0;
	FILETIME t_create, t_access, t_write;
	FILETIME *p_create=NULL, *p_access=NULL, *p_write=NULL;
	const wchar_t* FileName = check_utf8_string(L, 1, NULL);
	DWORD attr = GetFileAttributes(FileName);

	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		res = 0;
	}
	else if (attr & FILE_ATTRIBUTE_DIRECTORY)
	{
		struct timespec ts[2] = {};
		const char *path = lua_tostring(L, -1);
		lua_pushvalue(L, 2);
		if (ExtractFileTime(L, "LastAccessTime", &t_access, INVALID_HANDLE_VALUE)) {
			p_access = &t_access;
		}
		if (ExtractFileTime(L, "LastWriteTime", &t_write, INVALID_HANDLE_VALUE)) {
			p_write = &t_write;
		}
		if (p_access) {
			WINPORT(FileTime_Win32ToUnix)(p_access, &ts[0]);
		}
		if (p_write) {
			WINPORT(FileTime_Win32ToUnix)(p_write, &ts[1]);
		}
		res = (p_access||p_write) && (wrap_sdc_utimens(path, ts) != -1);
	}
	else
	{
		DWORD flags = 0;
		HANDLE hFile = CreateFile(FileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,flags,NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			lua_pushvalue(L, 2);
			if (ExtractFileTime(L, "CreationTime", &t_create, hFile)) {
				p_create = &t_create;
			}
			if (ExtractFileTime(L, "LastAccessTime", &t_access, hFile)) {
				p_access = &t_access;
			}
			if (ExtractFileTime(L, "LastWriteTime", &t_write, hFile)) {
				p_write = &t_write;
			}
			res = (p_create||p_access||p_write) && WINPORT(SetFileTime)(hFile,p_create,p_access,p_write);
			CloseHandle(hFile);
		}
	}
	lua_pushboolean(L, res);
	return 1;
}

static int win_uname(lua_State *L)
{
	struct utsname un;
	if (uname(&un) == 0)
	{
		lua_createtable(L, 0, 6);
		lua_pushstring(L, un.sysname);    lua_setfield(L, -2, "sysname");
		lua_pushstring(L, un.nodename);   lua_setfield(L, -2, "nodename");
		lua_pushstring(L, un.release);    lua_setfield(L, -2, "release");
		lua_pushstring(L, un.version);    lua_setfield(L, -2, "version");
		lua_pushstring(L, un.machine);    lua_setfield(L, -2, "machine");
#ifdef _GNU_SOURCE
		lua_pushstring(L, un.domainname); lua_setfield(L, -2, "domainname");
#endif
		return 1;
	}
	return SysErrorReturn(L);
}

static int win_GetHostName(lua_State *L)
{
	char buf[256];
	if (0 == gethostname(buf, sizeof(buf) - 1)) {
		buf[sizeof(buf) - 1] = 0;
		lua_pushstring(L, buf);
		return 1;
	}
	return SysErrorReturn(L);
}

static int win_chmod(lua_State *L)
{
	const char *pathname = luaL_checkstring(L, 1);
	mode_t mode = (mode_t) luaL_checkinteger(L, 2);
	if (0 == chmod(pathname, mode)) {
		lua_pushboolean(L, 1);
		return 1;
	}
	return SysErrorReturn(L);
}

static int win_stat(lua_State *L)
{
	const char *pathname = luaL_checkstring(L, 1);
	struct stat St;
	if (0 != stat(pathname, &St)) {
		return SysErrorReturn(L);
	}
	lua_createtable(L, 0, 14);
	PutNumToTable(L, "dev",     St.st_dev);
	PutNumToTable(L, "ino",     St.st_ino);
	PutNumToTable(L, "mode",    St.st_mode);
	PutNumToTable(L, "nlink",   St.st_nlink);
	PutNumToTable(L, "uid",     St.st_uid);
	PutNumToTable(L, "gid",     St.st_gid);
	PutNumToTable(L, "rdev",    St.st_rdev);
	PutNumToTable(L, "size",    St.st_size);
	PutNumToTable(L, "blksize", St.st_blksize);
	PutNumToTable(L, "blocks",  St.st_blocks);
	PutNumToTable(L, "atim",    St.st_atim.tv_sec + St.st_atim.tv_nsec/1E9);
	PutNumToTable(L, "mtim",    St.st_mtim.tv_sec + St.st_mtim.tv_nsec/1E9);
	PutNumToTable(L, "ctim",    St.st_ctim.tv_sec + St.st_ctim.tv_nsec/1E9);
	return 1;
}

#define PAIR(prefix,txt) {#txt, prefix ## _ ## txt}

static const luaL_Reg win_funcs[] = {
	PAIR( win, Clock),
	PAIR( win, CompareString),
	PAIR( win, CopyFile),
	PAIR( win, CreateDir),
	PAIR( win, DeleteFile),
	PAIR( win, EnsureColorsAreInverted),
	PAIR( win, ExpandEnv),
	PAIR( win, ExtractKey),
	PAIR( win, ExtractKeyEx),
	PAIR( win, FileTimeToLocalFileTime),
	PAIR( win, FileTimeToSystemTime),
	PAIR( win, GetConsoleScreenBufferInfo),
	PAIR( win, GetCurrentDir),
	PAIR( win, GetEnv),
	PAIR( win, GetFileInfo),
	PAIR( win, GetFileTimes),
	PAIR( win, GetHostName),
	PAIR( win, GetLocalTime),
	PAIR( win, GetSystemTime),
	PAIR( win, GetSystemTimeAsFileTime),
//PAIR( win, GetTimeZoneInformation),
	PAIR( win, GetVirtualKeys),
	PAIR( win, IsProcess64bit),
	PAIR( win, JoinPath),
	PAIR( win, MoveFile),
	PAIR( win, RemoveDir),
	PAIR( win, RenameFile),
	PAIR( win, SetCurrentDir),
	PAIR( win, SetEnv),
	PAIR( win, SetFileTimes),
	PAIR( win, Sleep),
	PAIR( win, system),
	PAIR( win, SystemTimeToFileTime),
	PAIR( win, uname),
	PAIR( win, wcscmp),
	PAIR( win, chmod),
	PAIR( win, stat),

	{NULL, NULL},
};

LUALIB_API int luaopen_win(lua_State *L)
{
	const char *libname = lua_istable(L,1) ? (lua_settop(L,1), NULL) : luaL_optstring(L, 1, "win");
	luaL_register(L, libname, win_funcs);

	lua_pushcfunction(L, luaopen_ustring);
	lua_pushvalue(L, -2);
	lua_call(L, 1, 0);

	return 1;
}
