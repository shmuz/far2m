#ifndef LUAFAR_SERVICE_H
#define LUAFAR_SERVICE_H

#include <windows.h>

#include "lf_luafar.h"

#define TKEY_BINARY "__binary"

static const DWORD LuamacroId = 0x4EBBEFC8;

typedef int64_t flags_t;

flags_t  GetFlagCombination(lua_State *L, int pos, int *success);
flags_t  GetFlagsFromTable(lua_State *L, int pos, const char* key);
void     PutFlagsToTable(lua_State *L, const char* key, flags_t flags);

int      PushDMParams (lua_State *L, int Msg, int Param1);
int      PushDNParams (lua_State *L, int Msg, int Param1, LONG_PTR Param2);
LONG_PTR ProcessDNResult(lua_State *L, int Msg, LONG_PTR Param2);

void     FillInputRecord(lua_State *L, int pos, INPUT_RECORD *ir);
void     PushInputRecord(lua_State *L, const INPUT_RECORD* ir);

void     LF_Error(lua_State *L, const wchar_t* aMsg);
void     NewVirtualKeyTable(lua_State* L, BOOL twoways);
void     pushFileTime(lua_State *L, const FILETIME *ft);
void     ConvertLuaValue(lua_State *L, int pos, struct FarMacroValue *target);
TPluginData*  GetPluginData(lua_State* L);
HANDLE   OptHandle(lua_State *L);

extern const char* VirtualKeyStrings[256];
extern struct PluginStartupInfo PSInfo;
extern struct FarStandardFunctions FSF;

#endif // #ifndef LUAFAR_SERVICE_H
