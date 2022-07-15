#ifndef LUAFAR_UTIL_H
#define LUAFAR_UTIL_H

#include "farplug-wide.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

/* convert a stack index to positive */
#define abs_index(L,i) ((i)>0 || (i)<=LUA_REGISTRYINDEX ? (i):lua_gettop(L)+(i)+1)

#define COLLECTOR_UD "Collector_UserData"

typedef struct {
  struct PluginStartupInfo *Info;
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
  struct PluginStartupInfo *Info;
  HANDLE            hDlg;
  BOOL              isOwned;
  BOOL              wasError;
} TDialogData;

typedef struct
{
  intptr_t X,Y;
  intptr_t Size;
  CHAR_INFO VBuf[1];
} TFarUserControl;

void  Log(const char* str);
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
TDialogData* NewDialogData(lua_State* L, struct PluginStartupInfo *Info, HANDLE hDlg, BOOL isOwned);
struct PluginStartupInfo* GetPluginStartupInfo(lua_State* L);
TFarUserControl* CheckFarUserControl(lua_State* L, int pos);

#endif // LUAFAR_UTIL_H
