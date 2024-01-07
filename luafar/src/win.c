#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "util.h"
#include "ustring.h"

extern const char* VirtualKeyStrings[256];
extern void NewVirtualKeyTable(lua_State* L, BOOL twoways);

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
static WORD ExtractKey()
{
	INPUT_RECORD rec;
	DWORD ReadCount;
	HANDLE hConInp = NULL; //GetStdHandle(STD_INPUT_HANDLE);
	while (WINPORT(PeekConsoleInput)(hConInp,&rec,1,&ReadCount), ReadCount) {
		WINPORT(ReadConsoleInput)(hConInp,&rec,1,&ReadCount);
		if (rec.EventType==KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
			return rec.Event.KeyEvent.wVirtualKeyCode;
	}
	return 0;
}

// result = ExtractKey()
// -- general purpose function; not FAR dependent
static int win_ExtractKey(lua_State *L)
{
	WORD vKey = ExtractKey() & 0xff;
	if (vKey && VirtualKeyStrings[vKey])
		lua_pushstring(L, VirtualKeyStrings[vKey]);
	else
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

	if(WINPORT(FileTimeToLocalFileTime)(&ft, &local_ft))
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

	if(lua_gettop(L) > 2) {
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
	unsigned usec = (unsigned) luaL_checknumber(L,1) * 1000; // msec -> mcsec
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

static const luaL_Reg win_funcs[] = {
	{"GetConsoleScreenBufferInfo", win_GetConsoleScreenBufferInfo},
	{"CopyFile",                   win_CopyFile},
	{"DeleteFile",                 win_DeleteFile},
	{"MoveFile",                   win_MoveFile},
	{"RenameFile",                 win_MoveFile}, // alias
	{"CreateDir",                  win_CreateDir},
	{"RemoveDir",                  win_RemoveDir},

	{"GetEnv",                     win_GetEnv},
	{"SetEnv",                     win_SetEnv},
	{"ExpandEnv",                  win_ExpandEnv},
//$  {"GetTimeZoneInformation",  win_GetTimeZoneInformation},
	{"GetFileInfo",                win_GetFileInfo},
	{"FileTimeToLocalFileTime",    win_FileTimeToLocalFileTime},
	{"FileTimeToSystemTime",       win_FileTimeToSystemTime},
	{"SystemTimeToFileTime",       win_SystemTimeToFileTime},
	{"GetSystemTimeAsFileTime",    win_GetSystemTimeAsFileTime},
	{"GetSystemTime",              win_GetSystemTime},
	{"GetLocalTime",               win_GetLocalTime},
	{"CompareString",              win_CompareString},
	{"wcscmp",                     win_wcscmp},
	{"ExtractKey",                 win_ExtractKey},
	{"GetVirtualKeys",             win_GetVirtualKeys},
	{"Sleep",                      win_Sleep},
	{"Clock",                      win_Clock},
	{"GetCurrentDir",              win_GetCurrentDir},
	{"SetCurrentDir",              win_SetCurrentDir},
	{"IsProcess64bit",             win_IsProcess64bit},
	{"system",                     win_system},
	{"EnsureColorsAreInverted",    win_EnsureColorsAreInverted},
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
