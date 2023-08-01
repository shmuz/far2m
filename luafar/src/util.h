#ifndef LUAFAR_UTIL_H
#define LUAFAR_UTIL_H

#include "farplug-wide.h"

#include <lua.h>
#include "luafar.h"

/* convert a stack index to positive */
#define abs_index(L,i) ((i)>0 || (i)<=LUA_REGISTRYINDEX ? (i):lua_gettop(L)+(i)+1)

#ifndef ARRAYSIZE
#  define ARRAYSIZE(buff) (sizeof(buff)/sizeof(buff[0]))
#endif

#define COLLECTOR_UD "Collector_UserData"

extern struct PluginStartupInfo PSInfo;
extern struct FarStandardFunctions FSF;

typedef struct {
  TPluginData *plugin_data;
  size_t    timer_id;
  unsigned  interval;
  int       objRef;
  int       funcRef;
  int       threadRef;
  int       closeStage;
  int       enabled;
  int       interval_changed; //TODO
}
TTimerData;

typedef struct {
  lua_State         *L;
  HANDLE            hDlg;
  BOOL              isOwned;
  BOOL              wasError;
  BOOL              isModal;
  GUID              Guid;
} TDialogData;

typedef struct
{
  intptr_t X,Y;
  intptr_t Size;
  CHAR_INFO VBuf[1];
} TFarUserControl;

int   DecodeAttributes(const char* str);
int   GetAttrFromTable(lua_State *L);
int   GetIntFromArray(lua_State *L, int index);
int   luaLF_FieldError (lua_State *L, const char* key, const char* expected_typename);
int   luaLF_SlotError (lua_State *L, int key, const char* expected_typename);
void  PushAttrString(lua_State *L, int attr);
void  PushPanelItem(lua_State *L, const struct PluginPanelItem *PanelItem);
void  PushPanelItems(lua_State *L, HANDLE handle, const struct PluginPanelItem *PanelItems, int ItemsNumber);
void  PutAttrToTable(lua_State *L, int attr);
void  PutMouseEvent(lua_State *L, const MOUSE_EVENT_RECORD* rec, BOOL table_exist);
uint64_t GetFileSizeFromTable(lua_State *L, const char *key);
FILETIME GetFileTimeFromTable(lua_State *L, const char *key);
void  PutFileTimeToTable(lua_State *L, const char* key, FILETIME ft);
TDialogData* NewDialogData(lua_State* L, HANDLE hDlg, BOOL isOwned);
TPluginData* GetPluginData(lua_State* L);
TFarUserControl* CheckFarUserControl(lua_State* L, int pos);
int far_InMyConfig(lua_State *L);
int far_InMyCache(lua_State *L);
int far_InMyTemp(lua_State *L);
int far_GetMyHome(lua_State *L);

#endif // LUAFAR_UTIL_H
