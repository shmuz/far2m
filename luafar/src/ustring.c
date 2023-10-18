#ifdef HAS_UUID
#include <uuid/uuid.h>
#endif

#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ustring.h"
#include "util.h"

// This function was initially taken from Lua 5.0.2 (loadlib.c)
void pusherrorcode(lua_State *L, int error)
{
#if 0
	const int BUFSZ = 256;
	wchar_t buffer[BUFSZ];
	int num = FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		0, error, 0, buffer, BUFSZ, 0);
	if (num)
		push_utf8_string(L, buffer, num);
	else
#endif
		lua_pushfstring(L, "system error %d\n", error);
}

int SysErrorReturn(lua_State *L)
{
	lua_pushnil(L);
	pusherrorcode(L, WINPORT(GetLastError)());
	return 2;
}

void PutIntToArray(lua_State *L, int key, int val)
{
	lua_pushinteger(L, key);
	lua_pushinteger(L, val);
	lua_settable(L, -3);
}

void PutNumToArray(lua_State *L, int key, double val)
{
	lua_pushinteger(L, key);
	lua_pushnumber(L, val);
	lua_settable(L, -3);
}

void PutIntToTable(lua_State *L, const char *key, int val)
{
	lua_pushinteger(L, val);
	lua_setfield(L, -2, key);
}

void PutNumToTable(lua_State *L, const char* key, double num)
{
	lua_pushnumber(L, num);
	lua_setfield(L, -2, key);
}

void PutBoolToTable(lua_State *L, const char* key, int num)
{
	lua_pushboolean(L, num);
	lua_setfield(L, -2, key);
}

void PutStrToTable(lua_State *L, const char* key, const char* str)
{
	lua_pushstring(L, str);
	lua_setfield(L, -2, key);
}

void PutStrToArray(lua_State *L, int key, const char* str)
{
	lua_pushinteger(L, key);
	lua_pushstring(L, str);
	lua_settable(L, -3);
}

void PutWStrToTable(lua_State *L, const char* key, const wchar_t* str, int numchars)
{
	push_utf8_string(L, str, numchars);
	lua_setfield(L, -2, key);
}

void PutWStrToArray(lua_State *L, int key, const wchar_t* str, int numchars)
{
	lua_pushinteger(L, key);
	push_utf8_string(L, str, numchars);
	lua_settable(L, -3);
}

void PutLStrToTable(lua_State *L, const char* key, const void* str, size_t len)
{
	lua_pushlstring(L, (const char*)str, len);
	lua_setfield(L, -2, key);
}

double GetOptNumFromTable(lua_State *L, const char* key, double dflt)
{
	double ret = dflt;
	lua_getfield(L, -1, key);
	if(lua_isnumber(L,-1))
		ret = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return ret;
}

int GetOptIntFromTable(lua_State *L, const char* key, int dflt)
{
	int ret = dflt;
	lua_getfield(L, -1, key);
	if(lua_isnumber(L,-1))
		ret = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return ret;
}

int GetOptIntFromArray(lua_State *L, int key, int dflt)
{
	int ret = dflt;
	lua_pushinteger(L, key);
	lua_gettable(L, -2);
	if(lua_isnumber(L,-1))
		ret = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return ret;
}

BOOL GetBoolFromTable(lua_State *L, const char* key)
{
	lua_getfield(L, -1, key);
	int ret = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return ret;
}

BOOL GetOptBoolFromTable(lua_State *L, const char* key, BOOL dflt)
{
	lua_getfield(L, -1, key);
	BOOL ret = lua_isnil(L, -1) ? dflt : lua_toboolean(L, -1);
	lua_pop(L, 1);
	return ret;
}

//---------------------------------------------------------------------------
// Check a multibyte string at 'pos' Lua stack position
// and convert it in place to UTF-32.
// Return a pointer to the converted string.
wchar_t* convert_multibyte_string (lua_State *L, int pos, UINT codepage,
	DWORD dwFlags, size_t* pTrgSize, int can_raise)
{
	if (pos < 0) pos += lua_gettop(L) + 1;

	if (!can_raise && !lua_isstring(L, pos))
		return NULL;

	size_t sourceLen;
	const char* source = luaL_checklstring(L, pos, &sourceLen);
	if (!pTrgSize)
		++sourceLen;

	int size = WINPORT(MultiByteToWideChar)(
		codepage,     // code page
		dwFlags,      // character-type options
		source,       // lpMultiByteStr, pointer to the character string to be converted
		sourceLen,    // size, in bytes, of the string pointed to by the lpMultiByteStr
		NULL,         // lpWideCharStr, address of wide-character buffer
		0             // size of buffer (in wide characters)
	);
	if (size == 0 && sourceLen != 0) {
		if (can_raise)
			luaL_argerror(L, pos, "invalid multibyte string");
		return NULL;
	}

	wchar_t* target = (wchar_t*)lua_newuserdata(L, (size+1) * sizeof(wchar_t));
	WINPORT(MultiByteToWideChar)(codepage, dwFlags, source, sourceLen, target, size);
	target[size] = L'\0';
	lua_replace(L, pos);
	if (pTrgSize) *pTrgSize = size;
	return target;
}

wchar_t* check_utf8_string (lua_State *L, int pos, size_t* pTrgSize)
{
	return convert_multibyte_string(L, pos, CP_UTF8, 0, pTrgSize, TRUE);
}

wchar_t* utf8_to_wcstring (lua_State *L, int pos, size_t* pTrgSize)
{
	return convert_multibyte_string(L, pos, CP_UTF8, 0, pTrgSize, FALSE);
}

const wchar_t* opt_utf8_string (lua_State *L, int pos, const wchar_t* dflt)
{
	return lua_isnoneornil(L,pos) ? dflt : check_utf8_string(L, pos, NULL);
}

wchar_t* oem_to_wcstring (lua_State *L, int pos, size_t* pTrgSize)
{
	return convert_multibyte_string (L, pos, CP_OEMCP, 0, pTrgSize, FALSE);
}

char* push_multibyte_string(lua_State* L, UINT CodePage, const wchar_t* str, int numchars, DWORD dwFlags)
{
	int targetSize;
	char *target;

	if(str == NULL) { lua_pushnil(L); return NULL; }

	targetSize = WINPORT(WideCharToMultiByte)(
									 CodePage, // UINT CodePage,
									 dwFlags,  // DWORD dwFlags,
									 str,      // LPCWSTR lpWideCharStr,
									 numchars, // int cchWideChar,
									 NULL,     // LPSTR lpMultiByteStr,
									 0,        // int cbMultiByte,
									 NULL,     // LPCSTR lpDefaultChar,
									 NULL      // LPBOOL lpUsedDefaultChar
							 );

	if(targetSize == 0 && numchars == -1 && str[0])
	{
		luaL_error(L, "invalid UTF-32 string");
	}

	target = (char*)lua_newuserdata(L, targetSize+1);
	WideCharToMultiByte(CodePage, dwFlags, str, (int)numchars, target, targetSize, NULL, NULL);

	if(numchars == -1)
		--targetSize;

	lua_pushlstring(L, target, targetSize);
	lua_remove(L, -2);
	return target;
}

void push_utf8_string (lua_State* L, const wchar_t* str, int numchars)
{
	push_multibyte_string(L, CP_UTF8, str, numchars, 0);
}

void push_oem_string (lua_State* L, const wchar_t* str, int numchars)
{
	push_multibyte_string(L, CP_OEMCP, str, numchars, 0);
}

void push_wcstring(lua_State* L, const wchar_t* str, int numchars)
{
	if(numchars < 0)
		numchars = wcslen(str);
	lua_pushlstring(L, (const char*)str, numchars*sizeof(wchar_t));
}

const wchar_t* check_wcstring(lua_State *L, int pos, size_t *len)
{
	size_t ln;
	const wchar_t* s = (const wchar_t*)luaL_checklstring(L, pos, &ln);

	if(len) *len = ln / sizeof(wchar_t);

	return s;
}

const wchar_t* opt_wcstring(lua_State *L, int pos, const wchar_t *dflt)
{
	const wchar_t* s = (const wchar_t*)luaL_optstring(L, pos, (const char*)dflt);
	return s;
}

int ustring_WideCharToMultiByte(lua_State *L)
{
	size_t numchars;
	const wchar_t* src = (const wchar_t*)luaL_checklstring(L, 1, &numchars);
	UINT codepage;
	DWORD dwFlags = 0;
	numchars /= sizeof(wchar_t);
	codepage = (UINT)luaL_checkinteger(L, 2);

	if(lua_isstring(L, 3))
	{
		const char *s = lua_tostring(L, 3);

		for(; *s; s++)
		{
			if      (*s == 'c') dwFlags |= WC_COMPOSITECHECK;
			else if (*s == 'd') dwFlags |= WC_DISCARDNS;
			else if (*s == 's') dwFlags |= WC_SEPCHARS;
			else if (*s == 'f') dwFlags |= WC_DEFAULTCHAR;
		}
	}

	push_multibyte_string(L, codepage, src, numchars, dwFlags);
	return 1;
}

int ustring_MultiByteToWideChar (lua_State *L)
{
	wchar_t* Trg;
	size_t TrgSize;
	(void) luaL_checkstring(L, 1);
	UINT codepage = luaL_checkinteger(L, 2);
	DWORD dwFlags = 0;
	if (lua_isstring(L, 3)) {
		const char *s = lua_tostring(L, 3);
		for (; *s; s++) {
			if      (*s == 'p') dwFlags |= MB_PRECOMPOSED;
			else if (*s == 'c') dwFlags |= MB_COMPOSITE;
			else if (*s == 'e') dwFlags |= MB_ERR_INVALID_CHARS;
			else if (*s == 'u') dwFlags |= MB_USEGLYPHCHARS;
		}
	}
	Trg = convert_multibyte_string(L, 1, codepage, dwFlags, &TrgSize, FALSE);
	if (Trg) {
		lua_pushlstring(L, (const char*)Trg, TrgSize * sizeof(wchar_t));
		return 1;
	}
	return SysErrorReturn(L);
}

int ustring_OemToUtf8 (lua_State *L)
{
	size_t len;
	(void) luaL_checklstring(L, 1, &len);
	wchar_t* buf = oem_to_wcstring(L, 1, &len);
	push_utf8_string(L, buf, len);
	return 1;
}

int ustring_Utf8ToOem (lua_State *L)
{
	size_t len;
	const wchar_t* buf = check_utf8_string(L, 1, &len);
	push_oem_string(L, buf, len);
	return 1;
}

int ustring_Utf32ToUtf8 (lua_State *L)
{
	size_t len;
	const wchar_t *ws = (const wchar_t*) luaL_checklstring(L, 1, &len);
	push_utf8_string(L, ws, len/sizeof(wchar_t));
	return 1;
}

int ustring_Utf8ToUtf32 (lua_State *L)
{
	size_t len;
	const wchar_t *ws = check_utf8_string(L, 1, &len);
	lua_pushlstring(L, (const char*) ws, len*sizeof(wchar_t));
	return 1;
}

int ustring_GetACP (lua_State* L) {
	return lua_pushinteger (L, WINPORT(GetACP)()), 1;
}

int ustring_GetOEMCP (lua_State* L) {
	return lua_pushinteger (L, WINPORT(GetOEMCP)()), 1;
}

#ifndef CP_INSTALLED
#  define CP_INSTALLED 0x00000001
#  define CP_SUPPORTED 0x00000002
#endif
struct EnumCP_struct {
	lua_State* L;
	int N;
} EnumCP;

BOOL CALLBACK EnumCodePagesProc(wchar_t* CodePageString)
{
	PutWStrToArray(EnumCP.L, ++EnumCP.N, CodePageString, -1);
	return TRUE;
}

int ustring_EnumSystemCodePages(lua_State *L)
{
	DWORD flags = lua_toboolean(L,1) ? CP_SUPPORTED : CP_INSTALLED;
	lua_newtable(L);
	EnumCP.L = L;
	EnumCP.N = 0;
	if (WINPORT(EnumSystemCodePages)(EnumCodePagesProc, flags))
		return 1;
	return SysErrorReturn(L);
}

int ustring_GetCPInfo(lua_State *L)
{
	UINT codepage;
	CPINFOEX info;
	memset(&info, 0, sizeof(info));
	codepage = luaL_checkinteger(L, 1);
	if (!WINPORT(GetCPInfoEx)(codepage, 0, &info))
		return SysErrorReturn(L);
	lua_createtable(L, 0, 6);
	PutNumToTable  (L, "MaxCharSize",  info.MaxCharSize);
	PutLStrToTable (L, "DefaultChar",  (const char*)info.DefaultChar, MAX_DEFAULTCHAR);
	PutLStrToTable (L, "LeadByte",     (const char*)info.LeadByte, MAX_LEADBYTES);
	PutWStrToTable (L, "UnicodeDefaultChar", &info.UnicodeDefaultChar, 1);
	PutNumToTable  (L, "CodePage",     info.CodePage);
	PutWStrToTable (L, "CodePageName", info.CodePageName, -1);
	return 1;
}

#ifndef HAS_UUID
int ustring_Uuid(lua_State* L)
{
	char buf[64];
	GUID uuid;
	memset(&uuid, 0, sizeof(uuid));

	if(lua_gettop(L) == 0 || !lua_toboolean(L, 1)) // generate new UUID
	{
		// TODO (currently it is a nil uuid)
		lua_pushlstring(L, (const char*)&uuid, sizeof(uuid));
		return 1;
	}
	else
	{
		size_t len;
		const char* arg1 = luaL_checklstring(L, 1, &len);

		if(len == sizeof(uuid)) // convert given UUID to string
		{
			memcpy(&uuid, arg1, sizeof(uuid));
			sprintf(buf, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
				uuid.Data1, (uint32_t)uuid.Data2, (uint32_t)uuid.Data3,
				(uint32_t)uuid.Data4[0], (uint32_t)uuid.Data4[1], (uint32_t)uuid.Data4[2], (uint32_t)uuid.Data4[3],
				(uint32_t)uuid.Data4[4], (uint32_t)uuid.Data4[5], (uint32_t)uuid.Data4[6], (uint32_t)uuid.Data4[7]);
			lua_pushstring(L, buf);
			return 1;
		}
		else if (len >= 36) // convert string UUID representation to UUID
		{
			int i=0, j=0;
			char tmp[] = {0,0,0};
			unsigned char *uch = (unsigned char*) buf;

			memset(buf, 0, sizeof(buf));
			for (; i < 36; i += 2) {
				uint32_t ui = 0;
				if (arg1[i] == '-')
					i++;
				memcpy(tmp, arg1+i, 2);
				sscanf(tmp, "%X", &ui);
				buf[j++] = ui;
			}

			uuid.Data1 = (uch[0]<<24) + (uch[1]<<16) + (uch[2]<<8) + uch[3];
			uuid.Data2 = (uch[4]<<8) + uch[5];
			uuid.Data3 = (uch[6]<<8) + uch[7];
			memcpy(uuid.Data4, uch+8, 8);
			lua_pushlstring(L, (char*)&uuid, sizeof(uuid));
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}
#else
// This function is used to achieve compatibility between Windows' GUID's and uuid_t values
// (uuid_t is just a byte array, i.e. always big-endian)
void shuffle_uuid(void* uuid)
{
	const unsigned char map[16] = {3,2,1,0,5,4,7,6,8,9,10,11,12,13,14,15};
	unsigned char buf[16];
	unsigned int tmp = 0xFF000000, idx;

	if (*(unsigned char*)&tmp != 0xFF) { //little endian
		char* ptr = (char*) uuid;
		for(idx=0; idx<16; idx++)
			buf[idx] = ptr[map[idx]];
		memcpy(ptr, buf, 16);
	}
}

int ustring_Uuid(lua_State* L)
{
	uuid_t uuid;
	char out[64]; // size must be >= 36 + 1
	enum { GEN_BIN, GEN_UPPER, GEN_LOWER, GEN_CONVERT };
	int Op = !lua_toboolean(L,1) ? GEN_BIN :
		(lua_isstring(L,1) && !strcasecmp(lua_tostring(L,1),"U")) ? GEN_UPPER :
		(lua_isstring(L,1) && !strcasecmp(lua_tostring(L,1),"L")) ? GEN_LOWER : GEN_CONVERT;

	if (Op != GEN_CONVERT) {
		// generate new UUID
		uuid_generate(uuid);
		if (Op == GEN_BIN) {
			shuffle_uuid(uuid);
			lua_pushlstring(L, (const char*)uuid, sizeof(uuid));
		}
		else {
			Op==GEN_UPPER ? uuid_unparse_upper(uuid,out) : uuid_unparse_lower(uuid,out);
			lua_pushstring(L, out);
		}
		return 1;
	}
	else {
		size_t len;
		const char* arg1 = luaL_checklstring(L, 1, &len);

		if(len == sizeof(uuid)) {
			// convert given UUID to string
			memcpy(uuid, arg1, len);
			shuffle_uuid(uuid);
			uuid_unparse_lower(uuid, out);
			lua_pushstring(L, out);
			return 1;
		}
		else if (len >= 2*sizeof(uuid)) {
			// convert string UUID representation to UUID
			if(0 == uuid_parse(arg1, uuid)) {
				shuffle_uuid(uuid);
				lua_pushlstring(L, (const char*)uuid, sizeof(uuid));
				return 1;
			}
		}
	}

	lua_pushnil(L);
	return 1;
}
#endif

int ustring_GetFileAttr(lua_State *L)
{
	DWORD attr = WINPORT(GetFileAttributes)(check_utf8_string(L,1,NULL));

	if(attr == 0xFFFFFFFF) return SysErrorReturn(L);

	PushAttrString(L, attr);
	return 1;
}

// for reusing code
int SetAttr(lua_State *L, const wchar_t* fname, unsigned attr)
{
	if (WINPORT(SetFileAttributes)(fname, attr))
		return lua_pushboolean(L, 1), 1;

	return SysErrorReturn(L);
}

int ustring_SetFileAttr(lua_State *L)
{
	return SetAttr(L, check_utf8_string(L,1,NULL), DecodeAttributes(luaL_checkstring(L,2)));
}

