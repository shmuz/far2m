#ifndef LUAFAR_SERVICE_H
#define LUAFAR_SERVICE_H

#include <windows.h>

#include "luafar.h"

static const DWORD LuamacroId = 0x4EBBEFC8;

typedef int64_t flags_t;

void          ConvertLuaValue (lua_State *L, int pos, struct FarMacroValue *target);
void          FillInputRecord (lua_State *L, int pos, INPUT_RECORD *Rec);
flags_t       GetFlagCombination(lua_State *L, int stack_pos, int *success);
flags_t       GetFlagsFromTable (lua_State *L, int stack_pos, const char* key);
TPluginData*  GetPluginData (lua_State* L);
void          LF_Error (lua_State *L, const wchar_t* aMsg);
void          NewVirtualKeyTable(lua_State* L, BOOL twoways);
HANDLE        OptHandle (lua_State *L);
LONG_PTR      ProcessDNResult (lua_State *L, int Msg, LONG_PTR Param2);
int           PushDMParams (lua_State *L, int Msg, int Param1);
int           PushDNParams (lua_State *L, int Msg, int Param1, LONG_PTR Param2);
void          PushInputRecord (lua_State* L, const INPUT_RECORD *Rec);

extern const char* VirtualKeyStrings[256];
extern struct PluginStartupInfo PSInfo;

#endif // #ifndef LUAFAR_SERVICE_H

