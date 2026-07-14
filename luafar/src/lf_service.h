#ifndef LUAFAR_SERVICE_H
#define LUAFAR_SERVICE_H

#include <windows.h>
#include "lf_luafar.h"

#define TKEY_BINARY "__binary"

#define PAIR(prefix,txt) {#txt, prefix ## _ ## txt}

static const DWORD LuamacroId = 0x4EBBEFC8;

void         ConvertLuaValue(lua_State *L, int pos, struct FarMacroValue *target);
int          Dialog_getvalue(lua_State *L, int pos, HANDLE *target);
int          FillEditorSelect(lua_State *L, int pos_table, struct EditorSelect *es);
void         FillEditorSetPosition(lua_State *L, struct EditorSetPosition *esp);
void         FillInputRecord(lua_State *L, int pos, INPUT_RECORD *ir);
uint64_t     GetFarColor(lua_State *L, int pos, struct FarTrueColorForeAndBack *fullcolor, int *basecolor, int *isTrueColor);
TPluginData* GetPluginData(lua_State* L);
void         LF_Error(lua_State *L, const wchar_t* aMsg);
void         NewVirtualKeyTable(lua_State* L, BOOL twoways);
GUID         OptGuid(lua_State *L, int pos);
HANDLE       OptHandle(lua_State *L);
int          pcall_msg(lua_State* L, int narg, int nret);
LONG_PTR     ProcessDNResult(lua_State *L, int Msg, LONG_PTR Param2);
int          PushDMParams (lua_State *L, int Msg, int Param1);
int          PushDNParams (lua_State *L, int Msg, int Param1, LONG_PTR Param2);
void         PushEditorSetPosition(lua_State *L, const struct EditorSetPosition *esp);
void         PushInputRecord(lua_State *L, const INPUT_RECORD* ir);
void         PushOptPluginTable(lua_State *L, HANDLE handle);
void         PushPanelItems(lua_State *L, HANDLE handle, const struct PluginPanelItem *PanelItems, int ItemsNumber);
void         PushPluginObject(lua_State* L, HANDLE hPlugin);
void         PutMouseEvent(lua_State *L, const MOUSE_EVENT_RECORD* rec, BOOL table_exist);
void         PutRECTToTable(lua_State *L, const char* key, RECT rect);
DWORD        RGBFromFarTrueColor(const struct FarTrueColor *tc);
int          SetKeyBar(lua_State *L, BOOL IsEditor);

extern const char* VirtualKeyStrings[256];
extern struct PluginStartupInfo PSInfo;
extern struct FarStandardFunctions FSF;

#endif // #ifndef LUAFAR_SERVICE_H
