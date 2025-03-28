#include <lua.h>
#include <lauxlib.h>

#include "lf_string.h"
#include "lf_util.h"

int Log(lua_State *L, const char* Format, ...)
{
	va_list valist;
	va_start(valist, Format);

	static int N = 0;
	const char* home = getenv("HOME");
	if (home) {
		char* buf = (char*) malloc(strlen(home) + 64);
		if (buf) {
			strcpy(buf, home);
			strcat(buf, "/luafar_log.txt");
			FILE* fp = fopen(buf, "a");
			if (fp) {
				if (++N == 1) {
					time_t rtime = time(NULL);
					fprintf(fp, "\n%s------------------------\n", ctime(&rtime));
				}
				fprintf(fp, "%d: %08X: ", N, GetPluginData(L)->PluginId);
				vfprintf(fp, Format, valist);
				fprintf(fp, "\n");
				fclose(fp);
			}
			free(buf);
		}
	}
	va_end(valist);
	return N;
}

// stack[-2] - table
// stack[-1] - value
int luaLF_SlotError (lua_State *L, int key, const char* expected_typename)
{
	return luaL_error (L,
		"bad field [%d] in table stackpos=%d (%s expected got %s)",
		key, abs_index(L,-2), expected_typename, luaL_typename(L,-1));
}

// stack[-2] - table
// stack[-1] - value
int luaLF_FieldError (lua_State *L, const char* key, const char* expected_typename)
{
	return luaL_error (L,
		"bad field '%s' in table stackpos=%d (%s expected got %s)",
		key, abs_index(L,-2), expected_typename, luaL_typename(L,-1));
}

int GetIntFromArray(lua_State *L, int index)
{
	lua_pushinteger(L, index);
	lua_gettable(L, -2);
	if (!lua_isnumber (L,-1))
		return luaLF_SlotError (L, index, "number");
	int ret = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return ret;
}

uint64_t GetFileSizeFromTable(lua_State *L, const char *key)
{
	uint64_t size;
	lua_getfield(L, -1, key);
	if (lua_isnumber(L, -1))
		size = (uint64_t) lua_tonumber(L, -1);
	else
		size = 0;
	lua_pop(L, 1);
	return size;
}

FILETIME GetFileTimeFromTable(lua_State *L, const char *key)
{
	FILETIME ft;
	lua_getfield(L, -1, key);
	if (lua_isnumber(L, -1)) {
		long long tm = (long long) lua_tonumber(L, -1);
		tm *= 10000; // convert ms units to 100ns ones
		ft.dwHighDateTime = tm / 0x100000000ll;
		ft.dwLowDateTime  = tm % 0x100000000ll;
	}
	else
		ft.dwLowDateTime = ft.dwHighDateTime = 0;
	lua_pop(L, 1);
	return ft;
}

void PutFileTimeToTable(lua_State *L, const char* key, FILETIME ft)
{
	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	PutNumToTable(L, key, li.QuadPart/10000); // convert 100ns units to 1ms ones
}

int DecodeAttributes(const char* str)
{
	int attr = 0;
	for(; *str; str++)
	{
		char c = (*str >= 'a') ? (*str-'a'+'A') : *str;
		if      (c == 'A') attr |= FILE_ATTRIBUTE_ARCHIVE;
		else if (c == 'C') attr |= FILE_ATTRIBUTE_COMPRESSED;
		else if (c == 'D') attr |= FILE_ATTRIBUTE_DIRECTORY;
		else if (c == 'E') attr |= FILE_ATTRIBUTE_REPARSE_POINT;
		else if (c == 'H') attr |= FILE_ATTRIBUTE_HIDDEN;
		else if (c == 'I') attr |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
		else if (c == 'N') attr |= FILE_ATTRIBUTE_ENCRYPTED;
		else if (c == 'O') attr |= FILE_ATTRIBUTE_OFFLINE;
		else if (c == 'P') attr |= FILE_ATTRIBUTE_SPARSE_FILE;
		else if (c == 'R') attr |= FILE_ATTRIBUTE_READONLY;
		else if (c == 'S') attr |= FILE_ATTRIBUTE_SYSTEM;
		else if (c == 'T') attr |= FILE_ATTRIBUTE_TEMPORARY;
		else if (c == 'U') attr |= FILE_ATTRIBUTE_NO_SCRUB_DATA;
		else if (c == 'V') attr |= FILE_ATTRIBUTE_VIRTUAL;

		else if (c == 'B') attr |= FILE_ATTRIBUTE_BROKEN;
		else if (c == 'X') attr |= FILE_ATTRIBUTE_EXECUTABLE;
		else if (c == 'J') attr |= FILE_ATTRIBUTE_DEVICE_BLOCK;
		else if (c == 'G') attr |= FILE_ATTRIBUTE_DEVICE_CHAR;
		else if (c == 'F') attr |= FILE_ATTRIBUTE_DEVICE_FIFO;
		else if (c == 'K') attr |= FILE_ATTRIBUTE_DEVICE_SOCK;
	}
	return attr;
}

int GetAttrFromTable(lua_State *L)
{
	int attr = 0;
	lua_getfield(L, -1, "FileAttributes");
	if (lua_isstring(L, -1))
		attr = DecodeAttributes(lua_tostring(L, -1));
	lua_pop(L, 1);
	return attr;
}

void PushAttrString(lua_State *L, int attr)
{
	char buf[32], *p = buf;
	if (attr & FILE_ATTRIBUTE_ARCHIVE)             *p++ = 'a';
	if (attr & FILE_ATTRIBUTE_COMPRESSED)          *p++ = 'c';
	if (attr & FILE_ATTRIBUTE_DIRECTORY)           *p++ = 'd';
	if (attr & FILE_ATTRIBUTE_REPARSE_POINT)       *p++ = 'e';
	if (attr & FILE_ATTRIBUTE_HIDDEN)              *p++ = 'h';
	if (attr & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) *p++ = 'i';
	if (attr & FILE_ATTRIBUTE_ENCRYPTED)           *p++ = 'n';
	if (attr & FILE_ATTRIBUTE_OFFLINE)             *p++ = 'o';
	if (attr & FILE_ATTRIBUTE_SPARSE_FILE)         *p++ = 'p';
	if (attr & FILE_ATTRIBUTE_READONLY)            *p++ = 'r';
	if (attr & FILE_ATTRIBUTE_SYSTEM)              *p++ = 's';
	if (attr & FILE_ATTRIBUTE_TEMPORARY)           *p++ = 't';
	if (attr & FILE_ATTRIBUTE_NO_SCRUB_DATA)       *p++ = 'u';
	if (attr & FILE_ATTRIBUTE_VIRTUAL)             *p++ = 'v';

	if (attr & FILE_ATTRIBUTE_BROKEN)              *p++ = 'b';
	if (attr & FILE_ATTRIBUTE_EXECUTABLE)          *p++ = 'x';
	if (attr & FILE_ATTRIBUTE_DEVICE_BLOCK)        *p++ = 'j';
	if (attr & FILE_ATTRIBUTE_DEVICE_CHAR)         *p++ = 'g';
	if (attr & FILE_ATTRIBUTE_DEVICE_FIFO)         *p++ = 'f';
	if (attr & FILE_ATTRIBUTE_DEVICE_SOCK)         *p++ = 'k';

	lua_pushlstring(L, buf, p-buf);
}

void PutAttrToTable(lua_State *L, int attr)
{
	PushAttrString(L, attr);
	lua_setfield(L, -2, "FileAttributes");
}

void SetAttrWords(const wchar_t* str, DWORD* incl, DWORD* excl)
{
	*incl=0; *excl=0;
	for (; *str; str++) {
		wchar_t c = *str;
		if      (c == L'a')  *incl |= FILE_ATTRIBUTE_ARCHIVE;
		else if (c == L'c')  *incl |= FILE_ATTRIBUTE_COMPRESSED;
		else if (c == L'd')  *incl |= FILE_ATTRIBUTE_DIRECTORY;
		else if (c == L'e')  *incl |= FILE_ATTRIBUTE_REPARSE_POINT;
		else if (c == L'h')  *incl |= FILE_ATTRIBUTE_HIDDEN;
		else if (c == L'i')  *incl |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
		else if (c == L'n')  *incl |= FILE_ATTRIBUTE_ENCRYPTED;
		else if (c == L'o')  *incl |= FILE_ATTRIBUTE_OFFLINE;
		else if (c == L'p')  *incl |= FILE_ATTRIBUTE_SPARSE_FILE;
		else if (c == L'r')  *incl |= FILE_ATTRIBUTE_READONLY;
		else if (c == L's')  *incl |= FILE_ATTRIBUTE_SYSTEM;
		else if (c == L't')  *incl |= FILE_ATTRIBUTE_TEMPORARY;
		else if (c == L'u')  *incl |= FILE_ATTRIBUTE_NO_SCRUB_DATA;
		else if (c == L'v')  *incl |= FILE_ATTRIBUTE_VIRTUAL;

		else if (c == L'b')  *incl |= FILE_ATTRIBUTE_BROKEN;
		else if (c == L'x')  *incl |= FILE_ATTRIBUTE_EXECUTABLE;
		else if (c == L'j')  *incl |= FILE_ATTRIBUTE_DEVICE_BLOCK;
		else if (c == L'g')  *incl |= FILE_ATTRIBUTE_DEVICE_CHAR;
		else if (c == L'f')  *incl |= FILE_ATTRIBUTE_DEVICE_FIFO;
		else if (c == L'k')  *incl |= FILE_ATTRIBUTE_DEVICE_SOCK;

		else if (c == L'A')  *excl |= FILE_ATTRIBUTE_ARCHIVE;
		else if (c == L'C')  *excl |= FILE_ATTRIBUTE_COMPRESSED;
		else if (c == L'D')  *excl |= FILE_ATTRIBUTE_DIRECTORY;
		else if (c == L'E')  *excl |= FILE_ATTRIBUTE_REPARSE_POINT;
		else if (c == L'H')  *excl |= FILE_ATTRIBUTE_HIDDEN;
		else if (c == L'I')  *excl |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
		else if (c == L'N')  *excl |= FILE_ATTRIBUTE_ENCRYPTED;
		else if (c == L'O')  *excl |= FILE_ATTRIBUTE_OFFLINE;
		else if (c == L'P')  *excl |= FILE_ATTRIBUTE_SPARSE_FILE;
		else if (c == L'R')  *excl |= FILE_ATTRIBUTE_READONLY;
		else if (c == L'S')  *excl |= FILE_ATTRIBUTE_SYSTEM;
		else if (c == L'T')  *excl |= FILE_ATTRIBUTE_TEMPORARY;
		else if (c == L'U')  *excl |= FILE_ATTRIBUTE_NO_SCRUB_DATA;
		else if (c == L'V')  *excl |= FILE_ATTRIBUTE_VIRTUAL;

		else if (c == L'B')  *excl |= FILE_ATTRIBUTE_BROKEN;
		else if (c == L'X')  *excl |= FILE_ATTRIBUTE_EXECUTABLE;
		else if (c == L'J')  *excl |= FILE_ATTRIBUTE_DEVICE_BLOCK;
		else if (c == L'G')  *excl |= FILE_ATTRIBUTE_DEVICE_CHAR;
		else if (c == L'F')  *excl |= FILE_ATTRIBUTE_DEVICE_FIFO;
		else if (c == L'K')  *excl |= FILE_ATTRIBUTE_DEVICE_SOCK;
	}
}
