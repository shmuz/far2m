#ifndef LUAFAR_USTRING_H
#define LUAFAR_USTRING_H

#include <windows.h>
#include <lua.h>

#include "luafar.h"

LUALIB_API int luaopen_ustring(lua_State *L);

int  Log(const char* Format, ...);
int  SysErrorReturn (lua_State *L);

BOOL   GetBoolFromTable   (lua_State *L, const char* key);
BOOL   GetOptBoolFromTable(lua_State *L, const char* key, BOOL dflt);
int    GetOptIntFromArray (lua_State *L, int key, int dflt);
int    GetOptIntFromTable (lua_State *L, const char* key, int dflt);
double GetOptNumFromTable (lua_State *L, const char* key, double dflt);
void   PutBoolToTable     (lua_State *L, const char* key, int num);
void   PutIntToArray      (lua_State *L, int key, int val);
void   PutNumToArray      (lua_State *L, int key, double val);
void   PutIntToTable      (lua_State *L, const char *key, int val);
void   PutLStrToTable     (lua_State *L, const char* key, const void* str, size_t len);
void   PutNumToTable      (lua_State *L, const char* key, double num);
void   PutStrToArray      (lua_State *L, int key, const char* str);
void   PutStrToTable      (lua_State *L, const char* key, const char* str);
void   PutWStrToArray     (lua_State *L, int key, const wchar_t* str, int numchars);
void   PutWStrToTable     (lua_State *L, const char* key, const wchar_t* str, int numchars);

DLLFUNC wchar_t* check_utf8_string (lua_State *L, int pos, size_t* pTrgSize);
DLLFUNC const wchar_t* opt_utf8_string (lua_State *L, int pos, const wchar_t* dflt);
DLLFUNC void push_utf8_string (lua_State* L, const wchar_t* str, int numchars);

wchar_t* utf8_to_wcstring (lua_State *L, int pos, size_t* pTrgSize);
wchar_t* oem_to_wcstring (lua_State *L, int pos, size_t* pTrgSize);
void push_oem_string (lua_State* L, const wchar_t* str, int numchars);
void push_wcstring(lua_State* L, const wchar_t* str, int numchars);

const wchar_t* check_wcstring(lua_State *L, int pos, size_t *len);
const wchar_t* opt_wcstring(lua_State *L, int pos, const wchar_t *dflt);

#endif // #ifndef LUAFAR_USTRING_H
