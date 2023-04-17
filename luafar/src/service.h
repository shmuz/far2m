#ifndef LUAFAR_SERVICE_H
#define LUAFAR_SERVICE_H

#include <windows.h>
#include <lua.h>
#include <farplug-wide.h>

#include "luafar.h"

TPluginData* GetPluginData (lua_State* L);
void   ConvertLuaValue (lua_State *L, int pos, struct FarMacroValue *target);
void   FillInputRecord (lua_State *L, int pos, INPUT_RECORD *Rec);
void   PushInputRecord (lua_State* L, const INPUT_RECORD *Rec);
DWORD  GetFlagCombination (lua_State *L, int stack_pos, int *success);
DWORD  GetFlagsFromTable (lua_State *L, int stack_pos, const char* key);
void   LF_Error (lua_State *L, const wchar_t* aMsg);
HANDLE OptHandle (lua_State *L);
int    ProcessDNResult (lua_State *L, int Msg, LONG_PTR Param2);
int    PushDMParams (lua_State *L, int Msg, int Param1);
int    PushDNParams (lua_State *L, int Msg, int Param1, LONG_PTR Param2);

#endif // #ifndef LUAFAR_SERVICE_H

