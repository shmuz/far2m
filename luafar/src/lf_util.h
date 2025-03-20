#ifndef LUAFAR_UTIL_H
#define LUAFAR_UTIL_H

#include "farplug-wide.h"

#include <lua.h>
#include "lf_luafar.h"

/* convert a stack index to positive */
#define abs_index(L,i) ((i)>0 || (i)<=LUA_REGISTRYINDEX ? (i):lua_gettop(L)+(i)+1)

#ifndef ARRAYSIZE
#  define ARRAYSIZE(buff) (sizeof(buff)/sizeof(buff[0]))
#endif

#define COLLECTOR_UD "Collector_UserData"

typedef struct {
	TPluginData *plugin_data;
	void     *timer_id;
	unsigned  interval;
	int       tabRef;
	int       closeStage;
	int       enabled;
	int       interval_changed; //TODO
} TTimerData;

enum {
	SYNCHRO_COMMON,
	SYNCHRO_TIMER,
	SYNCHRO_FUNCTION,
};

typedef struct
{
	TTimerData *timerData;
	int type;
	int data;
	int ref;
	int narg;
} TSynchroData;

typedef struct {
	lua_State         *L;
	HANDLE            hDlg;
	BOOL              isOwned;
	BOOL              wasError;
	BOOL              isModal;
	int               dataRef;
} TDialogData;

typedef struct
{
	intptr_t X,Y;
	intptr_t Size;
	CHAR_INFO VBuf[1];
} TFarUserControl;

TSynchroData* CreateSynchroData(int type, int data, TTimerData *td);
int   Log(lua_State *L, const char* Format, ...);
int   DecodeAttributes(const char* str);
int   GetAttrFromTable(lua_State *L);
int   GetIntFromArray(lua_State *L, int index);
int   luaLF_FieldError (lua_State *L, const char* key, const char* expected_typename);
int   luaLF_SlotError (lua_State *L, int key, const char* expected_typename);
void  PushAttrString(lua_State *L, int attr);
void  SetAttrWords(const wchar_t* str, DWORD* incl, DWORD* excl);
void  PushPanelItem(lua_State *L, const struct PluginPanelItem *PanelItem);
void  PushPanelItems(lua_State *L, HANDLE handle, const struct PluginPanelItem *PanelItems, int ItemsNumber);
void  PutAttrToTable(lua_State *L, int attr);
uint64_t GetFileSizeFromTable(lua_State *L, const char *key);
FILETIME GetFileTimeFromTable(lua_State *L, const char *key);
void  PutFileTimeToTable(lua_State *L, const char* key, FILETIME ft);
TDialogData* NewDialogData(lua_State* L, HANDLE hDlg, BOOL isOwned);
TPluginData* GetPluginData(lua_State* L);
TFarUserControl* CheckFarUserControl(lua_State* L, int pos);

#endif // LUAFAR_UTIL_H
