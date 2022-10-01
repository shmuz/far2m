//---------------------------------------------------------------------------
#include <windows.h>
#include <dlfcn.h> //dlclose
#include <farkeys.h>
#include "luafar.h"
#include "util.h"
#include "ustring.h"

extern void LF_Error(lua_State *L, const wchar_t* aMsg);
extern int  PushDMParams    (lua_State *L, int Msg, int Param1);
extern int  PushDNParams    (lua_State *L, int Msg, int Param1, LONG_PTR Param2);
extern int  ProcessDNResult (lua_State *L, int Msg, LONG_PTR Param2);
extern BOOL GetFlagCombination (lua_State *L, int stack_pos, int *trg);
extern int  GetFlagsFromTable(lua_State *L, int pos, const char* key);
extern HANDLE Open_Luamacro (lua_State* L, int OpenFrom, INT_PTR Item);
extern int  bit64_push(lua_State *L, INT64 v);
extern int  bit64_getvalue(lua_State *L, int pos, INT64 *target);
extern void FillInputRecord(lua_State *L, int pos, INPUT_RECORD *ir);
extern void PushInputRecord (lua_State* L, const INPUT_RECORD *Rec);

void PackMacroValues(lua_State* L, size_t Count, const struct FarMacroValue* Values); // forward declaration

// "Collector" is a Lua table referenced from the Plugin Object table by name.
// Collector contains an array of lightuserdata which are pointers to new[]'ed
// chars.
const char COLLECTOR_OPI[] = "Collector_OpenPluginInfo";
const char COLLECTOR_PI[]  = "Collector_PluginInfo";
const char COLLECTOR_FD[]  = "Collector_FindData";
const char KEY_OBJECT[]    = "Panel_Object";

// taken from lua.c v5.1.2
int traceback (lua_State *L) {
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

// taken from lua.c v5.1.2 (modified)
int docall (lua_State *L, int narg, int nret) {
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
  status = lua_pcall(L, narg, nret, base);
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}

// if the function is successfully retrieved, it's on the stack top; 1 is returned
// else 0 returned (and the stack is unchanged)
int GetExportFunction(lua_State* L, const char* FuncName)
{
  lua_getglobal(L, "export");
  if (lua_istable(L,-1))
  {
    lua_getfield(L, -1, FuncName);
    if(lua_isfunction(L,-1))
      return lua_remove(L,-2), 1;
    lua_pop(L,1);
  }
  return lua_pop(L,1), 0;
}

int pcall_msg (lua_State* L, int narg, int nret)
{
  // int status = lua_pcall(L, narg, nret, 0);
  int status = docall (L, narg, nret);
  if (status != 0) {
    int status2 = 1;
    if (GetExportFunction(L, "OnError")) {
      lua_insert(L,-2);
      status2 = lua_pcall(L,1,0,0);
    }
    if (status2 != 0) {
      LF_Error (L, check_utf8_string(L, -1, NULL));
      lua_pop (L, 1);
    }
  }
  return status;
}

void PushPluginTable(lua_State* L, HANDLE hPlugin)
{
  lua_pushlightuserdata(L, hPlugin);
  lua_rawget(L, LUA_REGISTRYINDEX);
}

void PushPluginObject(lua_State* L, HANDLE hPlugin)
{
  PushPluginTable(L, hPlugin);
  if (lua_istable(L, -1))
    lua_getfield(L, -1, KEY_OBJECT);
  else
    lua_pushnil(L);
  lua_remove(L, -2);
}

void PushPluginPair(lua_State* L, HANDLE hPlugin)
{
  PushPluginObject(L, hPlugin);
  lua_pushlightuserdata(L, hPlugin);
}

void CreatePluginInfoCollector (lua_State* L)
{
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, COLLECTOR_PI);
}

void DestroyPluginInfoCollector(lua_State* L)
{
  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, COLLECTOR_PI);
}

void DestroyCollector(lua_State* L, HANDLE hPlugin, const char* Collector)
{
  PushPluginTable(L, hPlugin);      //+1: Tbl
  lua_pushnil(L);                   //+2
  lua_setfield(L, -2, Collector);   //+1
  lua_pop(L,1);                     //+0
}

// the value is on stack top (-1)
// collector table is under the index 'pos' (this index cannot be a pseudo-index)
const wchar_t* _AddStringToCollector(lua_State *L, int pos)
{
  if (lua_isstring(L,-1)) {
    const wchar_t* s = check_utf8_string (L, -1, NULL);
    lua_rawseti(L, pos, lua_objlen(L, pos) + 1);
    return s;
  }
  lua_pop(L,1);
  return NULL;
}

// input table is on stack top (-1)
// collector table is under the index 'pos' (this index cannot be a pseudo-index)
const wchar_t* AddStringToCollectorField(lua_State *L, int pos, const char* key)
{
  lua_getfield(L, -1, key);
  return _AddStringToCollector(L, pos);
}

// input table is on stack top (-1)
// collector table is under the index 'pos' (this index cannot be a pseudo-index)
const wchar_t* AddStringToCollectorSlot(lua_State *L, int pos, int key)
{
  lua_pushinteger (L, key);
  lua_gettable(L, -2);
  return _AddStringToCollector(L, pos);
}

// collector table is under the index 'pos' (this index cannot be a pseudo-index)
void* AddBufToCollector(lua_State *L, int pos, size_t size)
{
  if (pos < 0) --pos;
  void* t = lua_newuserdata(L, size);
  memset (t, 0, size);
  lua_rawseti(L, pos, lua_objlen(L, pos) + 1);
  return t;
}

// -- a table is on stack top
// -- its field 'field' is an array of strings
// -- 'cpos' - collector stack position
const wchar_t** CreateStringsArray(lua_State* L, int cpos, const char* field, int *numstrings)
{
  const wchar_t **buf = NULL;
  if(numstrings) *numstrings = 0;
  lua_getfield(L, -1, field);
  if(lua_istable(L, -1)) {
    int n = lua_objlen(L, -1);
    if (numstrings) *numstrings = n;
    if (n > 0) {
      int i;
      buf = (const wchar_t**)AddBufToCollector(L, cpos, (n+1) * sizeof(wchar_t*));
      for (i=0; i < n; i++)
        buf[i] = AddStringToCollectorSlot(L, cpos, i+1);
      buf[n] = NULL;
    }
  }
  lua_pop(L, 1);
  return buf;
}

// input table is on stack top (-1)
// collector table is one under the top (-2)
// userdata table is two under the top (-3)
void FillPluginPanelItem (lua_State *L, struct PluginPanelItem *pi, int index)
{
  int Collector = lua_gettop(L) - 1;
  memset(pi, 0, sizeof(*pi));
  pi->FindData.dwFileAttributes = GetAttrFromTable(L);
  pi->FindData.ftCreationTime   = GetFileTimeFromTable(L, "CreationTime");
  pi->FindData.ftLastAccessTime = GetFileTimeFromTable(L, "LastAccessTime");
  pi->FindData.ftLastWriteTime  = GetFileTimeFromTable(L, "LastWriteTime");
  pi->FindData.nFileSize        = GetFileSizeFromTable(L, "FileSize");
  pi->NumberOfLinks             = GetOptIntFromTable  (L, "NumberOfLinks", 0);

  pi->FindData.lpwszFileName = (wchar_t*)AddStringToCollectorField(L, Collector, "FileName");
  pi->Description            = (wchar_t*)AddStringToCollectorField(L, Collector, "Description");
  pi->Owner                  = (wchar_t*)AddStringToCollectorField(L, Collector, "Owner");

  // custom column data
  lua_getfield(L, -1, "CustomColumnData");
  if (lua_istable(L,-1)) {
    int i;
    pi->CustomColumnNumber = lua_objlen(L,-1);
    pi->CustomColumnData = malloc(pi->CustomColumnNumber * sizeof(wchar_t**));
    for (i=0; i < pi->CustomColumnNumber; i++) {
      lua_rawgeti(L, -1, i+1);
      *(wchar_t**)(pi->CustomColumnData+i) = (wchar_t*)_AddStringToCollector(L, Collector);
    }
  }
  lua_pop(L,1);

  // prevent Far from treating UserData as pointer and copying data from it
  pi->Flags = GetOptIntFromTable(L, "Flags", 0) & ~PPIF_USERDATA;
  lua_getfield(L, -1, "UserData");
  if (!lua_isnil(L, -1)) {
    pi->UserData = index;
    lua_rawseti(L, Collector-1, index);
  }
  else {
    pi->UserData = 0;
    lua_pop(L, 1);
  }
}

// Two known values on the stack top: Plugin table (Tbl; at -2) and FindData (at -1).
void FillFindData(lua_State* L, struct PluginPanelItem **pPanelItems, int *pItemsNumber)
{
  struct PluginPanelItem *ppi;
  int i, num=0;
  int numLines = lua_objlen(L,-1);

  ppi = (struct PluginPanelItem*) malloc(sizeof(struct PluginPanelItem) * numLines);
  if (ppi) {
    lua_newtable(L);                     //+3  Tbl,FindData,UData
    lua_pushvalue(L,-1);                 //+4: Tbl,FindData,UData,UData
    lua_setfield(L, -4, COLLECTOR_UD);   //+3: Tbl,FindData,UData

    lua_newtable(L);                     //+4  Tbl,FindData,UData,Coll
    lua_pushvalue(L,-1);                 //+5: Tbl,FindData,UData,Coll,Coll
    lua_setfield(L, -5, COLLECTOR_FD);   //+4: Tbl,FindData,UData,Coll

    for (i=1; i<=numLines; i++) {
      lua_rawgeti(L, -3, i);             //+5: Tbl,FindData,UData,Coll,FindData[i]
      if (lua_istable(L,-1)) {
        FillPluginPanelItem(L, ppi+num, num+1);
        ++num;
      }
      lua_pop(L,1);                      //+4
    }
    lua_pop(L,2);                        //+2
  }
  *pItemsNumber = num;
  *pPanelItems = ppi;
}

int LF_GetFindData(lua_State* L, HANDLE hPlugin, struct PluginPanelItem **pPanelItem,
                   int *pItemsNumber, int OpMode)
{
  if (GetExportFunction(L, "GetFindData")) {   //+1: Func
    PushPluginPair(L, hPlugin);                //+3: Func,Pair
    lua_pushinteger(L, OpMode);                //+4: Func,Pair,OpMode
    if (!pcall_msg(L, 3, 1)) {                 //+1: FindData
      if (lua_istable(L, -1)) {
        if (lua_objlen(L,-1) == 0) {
          *pItemsNumber = 0;
          *pPanelItem = NULL;
          lua_pop(L,1);                        //+0
        }
        else {
          PushPluginTable(L, hPlugin);         //+2: FindData,Tbl
          lua_insert(L, -2);                   //+2: Tbl,FindData
          FillFindData(L, pPanelItem, pItemsNumber);
          lua_pop(L,2);                        //+0
        }
        return TRUE;
      }
      lua_pop(L,1);
    }
  }
  return FALSE;
}

int LF_GetVirtualFindData (lua_State* L, HANDLE hPlugin, struct PluginPanelItem **pPanelItem,
                           int *pItemsNumber, const wchar_t *Path)
{
  if (GetExportFunction(L, "GetVirtualFindData")) {   //+1: Func
    PushPluginPair(L, hPlugin);                //+3: Func,Pair
    push_utf8_string(L, Path, -1);             //+4: Func,Pair,Path
    if (!pcall_msg(L, 3, 1)) {                 //+1: FindData
      if (lua_istable(L, -1)) {
        if (lua_objlen(L,-1) == 0) {
          *pItemsNumber = 0;
          *pPanelItem = NULL;
          lua_pop(L,1);                        //+0
        }
        else {
          PushPluginTable(L, hPlugin);         //+2: FindData,Tbl
          lua_insert(L, -2);                   //+2: Tbl,FindData
          FillFindData(L, pPanelItem, pItemsNumber);
          lua_pop(L,2);                        //+0
        }
        return TRUE;
      }
      lua_pop(L,1);
    }
  }
  return FALSE;
}

void free_find_data(lua_State* L, HANDLE hPlugin, struct PluginPanelItem *PanelItems, int ItemsNumber)
{
  int i;
  for (i=0; i<ItemsNumber; i++) {
    free((void*)PanelItems[i].CustomColumnData);
  }
  PushPluginTable(L, hPlugin);
  lua_pushnil(L);
  lua_setfield(L, -2, COLLECTOR_FD); //free the collector
  lua_pop(L, 1);
  lua_gc(L, LUA_GCCOLLECT, 0); //free memory taken by Collector
  free(PanelItems);
}

void LF_FreeFindData(lua_State* L, HANDLE hPlugin, struct PluginPanelItem *PanelItems, int ItemsNumber)
{
  free_find_data(L, hPlugin, PanelItems, ItemsNumber);
}

void LF_FreeVirtualFindData(lua_State* L, HANDLE hPlugin, struct PluginPanelItem *PanelItems, int ItemsNumber)
{
  free_find_data(L, hPlugin, PanelItems, ItemsNumber);
}

// PanelItems table should be on Lua stack top
void UpdateFileSelection(lua_State* L, struct PluginPanelItem *PanelItems, int ItemsNumber)
{
  int i;
  for(i=0; i<(int)ItemsNumber; i++)
  {
    lua_rawgeti(L, -1, i+1);           //+1
    if(lua_istable(L,-1))
    {
      lua_getfield(L,-1,"Flags");      //+2
      if(lua_toboolean(L,-1))
      {
        int success = 0;
        int Flags = GetFlagCombination(L,-1,&success);
        if(success && ((Flags & PPIF_SELECTED) == 0))
          PanelItems[i].Flags &= ~PPIF_SELECTED;
      }
      lua_pop(L,1);         //+1
    }
    lua_pop(L,1);           //+0
  }
}
//---------------------------------------------------------------------------

int LF_GetFiles (lua_State* L, HANDLE hPlugin, struct PluginPanelItem *PanelItem,
  int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode)
{
  if (GetExportFunction(L, "GetFiles")) {      //+1: Func
    PushPanelItems(L, hPlugin, PanelItem, ItemsNumber); //+2: Func,Item
    lua_insert(L,-2);                          //+2: Item,Func
    PushPluginPair(L, hPlugin);                //+4: Item,Func,Pair
    lua_pushvalue(L,-4);                       //+5: Item,Func,Pair,Item
    lua_pushboolean(L, Move);
    push_utf8_string(L, *DestPath, -1);
    lua_pushinteger(L, OpMode);        //+8: Item,Func,Pair,Item,Move,Dest,OpMode
    int ret = pcall_msg(L, 6, 2);      //+3: Item,Res,Dest
    if (ret == 0) {
      if (lua_isstring(L,-1)) {
        *DestPath = check_utf8_string(L,-1,NULL);
        lua_setfield(L, LUA_REGISTRYINDEX, "GetFiles.DestPath"); // protect from GC
      }
      else
        lua_pop(L,1);                  //+2: Item,Res
      ret = lua_tointeger(L,-1);
      lua_pop(L,1);                    //+1: Item
      UpdateFileSelection(L, PanelItem, ItemsNumber);
      return lua_pop(L,1), ret;
    }
    return lua_pop(L,1), 0;
  }
  return 0;
}
//---------------------------------------------------------------------------

// return FALSE only if error occurred
BOOL CheckReloadDefaultScript (lua_State *L)
{
  // reload default script?
  int reload = 0;
  lua_getglobal(L, "far");
  if (lua_istable(L, -1))
  {
    lua_getfield(L, -1, "ReloadDefaultScript");
    reload = lua_toboolean(L, -1);
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
  return !reload || LF_RunDefaultScript(L);
}

// -- an object (any non-nil value) is on stack top;
// -- a new table is created, the object is put into it under the key KEY_OBJECT;
// -- the table is put into the registry, and reference to it is obtained;
// -- the function pops the object and returns the reference;
HANDLE RegisterObject(lua_State* L)
{
  void *ptr;
  lua_newtable(L);                  //+2: Obj,Tbl
  lua_pushvalue(L,-2);              //+3: Obj,Tbl,Obj
  lua_setfield(L,-2,KEY_OBJECT);    //+2: Obj,Tbl
  ptr = (void*)lua_topointer(L,-1);
  lua_pushlightuserdata(L, ptr);    //+3
  lua_pushvalue(L,-2);              //+4
  lua_rawset(L, LUA_REGISTRYINDEX); //+2
  lua_pop(L,2);                     //+0
  return ptr;
}

HANDLE LF_OpenFilePlugin(lua_State* L, const wchar_t *aName,
  const unsigned char *aData, int aDataSize, int OpMode)
{
  if (!CheckReloadDefaultScript(L))
    return INVALID_HANDLE_VALUE;

  if (GetExportFunction(L, "OpenFilePlugin")) {           //+1
    if(aName) {
      push_utf8_string(L, aName, -1);                     //+2
      lua_pushlstring(L, (const char*)aData, aDataSize);  //+3
    }
    else {
      lua_pushnil(L); lua_pushnil(L);
    }
    lua_pushinteger(L, OpMode);
    if (!pcall_msg(L, 3, 1)) {
      if (lua_type(L,-1) == LUA_TNUMBER && lua_tointeger(L,-1) == -2) {
        lua_pop(L,1);
        return PANEL_STOP;
      }
      if (lua_toboolean(L, -1))                   //+1
        return RegisterObject(L);                 //+0
      lua_pop (L, 1);                             //+0
    }
  }
  return INVALID_HANDLE_VALUE;
}
//---------------------------------------------------------------------------

void LF_GetOpenPluginInfo(lua_State* L, HANDLE hPlugin, struct OpenPluginInfo *aInfo)
{
  aInfo->StructSize = sizeof (struct OpenPluginInfo);
  if (!GetExportFunction(L, "GetOpenPluginInfo"))    //+1
    return;

  PushPluginPair(L, hPlugin);                        //+3
  if(pcall_msg(L, 2, 1) != 0)
    return;

  if(!lua_istable(L, -1)) {                          //+1: Info
    lua_pop(L, 1);
    return;
  }
  DestroyCollector(L, hPlugin, COLLECTOR_OPI);
  PushPluginTable(L, hPlugin);                       //+2: Info,Tbl
  lua_newtable(L);                                   //+3: Info,Tbl,Coll
  int cpos = lua_gettop (L);  // collector stack position
  lua_pushvalue(L,-1);                               //+4: Info,Tbl,Coll,Coll
  lua_setfield(L, -3, COLLECTOR_OPI);                //+3: Info,Tbl,Coll
  lua_pushvalue(L,-3);                               //+4: Info,Tbl,Coll,Info
  //---------------------------------------------------------------------------
  // First element in the collector; can be retrieved on later calls;
  struct OpenPluginInfo *Info =
    (struct OpenPluginInfo*) AddBufToCollector(L, cpos, sizeof(struct OpenPluginInfo));
  //---------------------------------------------------------------------------
  Info->StructSize = sizeof (struct OpenPluginInfo);
  Info->Flags      = GetOptIntFromTable(L, "Flags", 0);
  Info->HostFile   = AddStringToCollectorField(L, cpos, "HostFile");
  Info->CurDir     = AddStringToCollectorField(L, cpos, "CurDir");
  Info->Format     = AddStringToCollectorField(L, cpos, "Format");
  Info->PanelTitle = AddStringToCollectorField(L, cpos, "PanelTitle");
  //---------------------------------------------------------------------------
  lua_getfield(L, -1, "InfoLines");
  lua_getfield(L, -2, "InfoLinesNumber");
  if (lua_istable(L,-2) && lua_isnumber(L,-1)) {
    int InfoLinesNumber = lua_tointeger(L, -1);
    lua_pop(L,1);                         //+5: Info,Tbl,Coll,Info,Lines
    if (InfoLinesNumber > 0 && InfoLinesNumber <= 100) {
      int i;
      struct InfoPanelLine *pl = (struct InfoPanelLine*)
        AddBufToCollector(L, cpos, InfoLinesNumber * sizeof(struct InfoPanelLine));
      Info->InfoLines = pl;
      Info->InfoLinesNumber = InfoLinesNumber;
      for (i=0; i<InfoLinesNumber; ++i,++pl,lua_pop(L,1)) {
        lua_pushinteger(L, i+1);
        lua_gettable(L, -2);
        if(lua_istable(L, -1)) {          //+6: Info,Tbl,Coll,Info,Lines,Line
          pl->Text = AddStringToCollectorField(L, cpos, "Text");
          pl->Data = AddStringToCollectorField(L, cpos, "Data");
          pl->Separator = GetOptIntFromTable(L, "Separator", 0);
        }
      }
    }
    lua_pop(L,1);
  }
  else lua_pop(L, 2);
  //---------------------------------------------------------------------------
  Info->DescrFiles = CreateStringsArray(L, cpos, "DescrFiles", &Info->DescrFilesNumber);
  //---------------------------------------------------------------------------
  lua_getfield(L, -1, "PanelModesArray");
  lua_getfield(L, -2, "PanelModesNumber");
  if (lua_istable(L,-2) && lua_isnumber(L,-1)) {
    int PanelModesNumber = lua_tointeger(L, -1);
    lua_pop(L,1);                               //+5: Info,Tbl,Coll,Info,Modes
    if (PanelModesNumber > 0 && PanelModesNumber <= 100) {
      int i;
      struct PanelMode *pm = (struct PanelMode*)
        AddBufToCollector(L, cpos, PanelModesNumber * sizeof(struct PanelMode));
      Info->PanelModesArray = pm;
      Info->PanelModesNumber = PanelModesNumber;
      for (i=0; i<PanelModesNumber; ++i,++pm,lua_pop(L,1)) {
        lua_pushinteger(L, i+1);
        lua_gettable(L, -2);
        if(lua_istable(L, -1)) {                //+6: Info,Tbl,Coll,Info,Modes,Mode
          pm->ColumnTypes  = (wchar_t*)AddStringToCollectorField(L, cpos, "ColumnTypes");
          pm->ColumnWidths = (wchar_t*)AddStringToCollectorField(L, cpos, "ColumnWidths");
          pm->FullScreen   = (int)GetOptBoolFromTable(L, "FullScreen", FALSE);
          pm->DetailedStatus  = GetOptIntFromTable(L, "DetailedStatus", 0);
          pm->AlignExtensions = GetOptIntFromTable(L, "AlignExtensions", 0);
          pm->CaseConversion  = (int)GetOptBoolFromTable(L, "CaseConversion", FALSE);
          pm->StatusColumnTypes  = (wchar_t*)AddStringToCollectorField(L, cpos, "StatusColumnTypes");
          pm->StatusColumnWidths = (wchar_t*)AddStringToCollectorField(L, cpos, "StatusColumnWidths");
          pm->ColumnTitles = (const wchar_t* const*)CreateStringsArray(L, cpos, "ColumnTitles", NULL);
        }
      }
    }
    lua_pop(L,1);
  }
  else lua_pop(L, 2);
  //---------------------------------------------------------------------------
  Info->StartPanelMode = GetOptIntFromTable(L, "StartPanelMode", 0);
  Info->StartSortMode  = GetFlagsFromTable (L, -1, "StartSortMode");
  Info->StartSortOrder = GetOptIntFromTable(L, "StartSortOrder", 0);
  //---------------------------------------------------------------------------
  lua_getfield (L, -1, "KeyBar");
  if (lua_istable(L, -1)) {
    size_t i;
    int j;
    struct KeyBarTitles *kbt = (struct KeyBarTitles*)
      AddBufToCollector(L, cpos, sizeof(struct KeyBarTitles));
    Info->KeyBar = kbt;
    struct { const char* key; wchar_t** trg; } pairs[] = {
      {"Titles",          kbt->Titles},
      {"CtrlTitles",      kbt->CtrlTitles},
      {"AltTitles",       kbt->AltTitles},
      {"ShiftTitles",     kbt->ShiftTitles},
      {"CtrlShiftTitles", kbt->CtrlShiftTitles},
      {"AltShiftTitles",  kbt->AltShiftTitles},
      {"CtrlAltTitles",   kbt->CtrlAltTitles},
    };
    for (i=0; i < ARRAYSIZE(pairs); i++) {
      lua_getfield (L, -1, pairs[i].key);
      if (lua_istable (L, -1)) {
        for (j=0; j<12; j++)
          pairs[i].trg[j] = (wchar_t*)AddStringToCollectorSlot(L, cpos, j+1);
      }
      lua_pop (L, 1);
    }
  }
  lua_pop(L,1);
  //---------------------------------------------------------------------------
  Info->ShortcutData = AddStringToCollectorField (L, cpos, "ShortcutData");
  //---------------------------------------------------------------------------
  lua_pop(L,4);
  *aInfo = *Info;
}
//---------------------------------------------------------------------------

void PushFarMacroValue(lua_State* L, const struct FarMacroValue* val)
{
	switch(val->Type)
	{
		case FMVT_INTEGER:
			bit64_push(L, val->Value.Integer);
			break;
		case FMVT_DOUBLE:
			lua_pushnumber(L, val->Value.Double);
			break;
		case FMVT_STRING:
		case FMVT_ERROR:
			push_utf8_string(L, val->Value.String, -1);
			break;
		case FMVT_BOOLEAN:
			lua_pushboolean(L, (int)val->Value.Boolean);
			break;
		case FMVT_POINTER:
		case FMVT_PANEL:
			lua_pushlightuserdata(L, val->Value.Pointer);
			break;
		case FMVT_BINARY:
			lua_createtable(L,1,0);
			lua_pushlstring(L, (char*)val->Value.Binary.Data, val->Value.Binary.Size);
			lua_rawseti(L,-2,1);
			break;
		case FMVT_ARRAY:
			PackMacroValues(L, val->Value.Array.Count, val->Value.Array.Values); // recursion
			lua_pushliteral(L, "array");
			lua_setfield(L, -2, "type");
			break;
		default:
			lua_pushnil(L);
			break;
	}
}

void PackMacroValues(lua_State* L, size_t Count, const struct FarMacroValue* Values)
{
	size_t i;
	lua_createtable(L, (int)Count, 1);
	for(i=0; i < Count; i++)
	{
		PushFarMacroValue(L, Values + i);
		lua_rawseti(L, -2, (int)i+1);
	}
	lua_pushinteger(L, Count);
	lua_setfield(L, -2, "n");
}

static void WINAPI FillFarMacroCall_Callback (void *CallbackData, struct FarMacroValue *Values, size_t Count)
{
	size_t i;
	struct FarMacroCall *fmc = (struct FarMacroCall*)CallbackData;
	(void)Values; // not used
	(void)Count;  // not used
	for(i=0; i<fmc->Count; i++)
	{
		struct FarMacroValue *v = fmc->Values + i;
		if (v->Type == FMVT_STRING)
			free((void*)v->Value.String);
		else if (v->Type == FMVT_BINARY && v->Value.Binary.Size)
			free((void*)v->Value.Binary.Data);
	}
	free(CallbackData);
}

static HANDLE FillFarMacroCall (lua_State* L, int narg)
{
	INT64 val64;
	int i;

	struct FarMacroCall *fmc = (struct FarMacroCall*)
		malloc(sizeof(struct FarMacroCall) + narg*sizeof(struct FarMacroValue));

	fmc->StructSize = sizeof(*fmc);
	fmc->Count = narg;
	fmc->Values = (struct FarMacroValue*)(fmc+1);
	fmc->Callback = FillFarMacroCall_Callback;
	fmc->CallbackData = fmc;

	for (i=0; i<narg; i++)
	{
		int type = lua_type(L, i-narg);
		if (type == LUA_TNUMBER)
		{
			fmc->Values[i].Type = FMVT_DOUBLE;
			fmc->Values[i].Value.Double = lua_tonumber(L, i-narg);
		}
		else if (type == LUA_TBOOLEAN)
		{
			fmc->Values[i].Type = FMVT_BOOLEAN;
			fmc->Values[i].Value.Boolean = lua_toboolean(L, i-narg);
		}
		else if (type == LUA_TSTRING)
		{
			fmc->Values[i].Type = FMVT_STRING;
			fmc->Values[i].Value.String = wcsdup(check_utf8_string(L, i-narg, NULL));
		}
		else if (type == LUA_TLIGHTUSERDATA)
		{
			fmc->Values[i].Type = FMVT_POINTER;
			fmc->Values[i].Value.Pointer = lua_touserdata(L, i-narg);
		}
		else if (type == LUA_TTABLE)
		{
			size_t len;
			fmc->Values[i].Type = FMVT_BINARY;
			fmc->Values[i].Value.Binary.Data = (char*)"";
			fmc->Values[i].Value.Binary.Size = 0;
			lua_rawgeti(L, i-narg, 1);
			if (lua_type(L,-1) == LUA_TSTRING && (len=lua_objlen(L,-1)) != 0)
			{
				void* arr = malloc(len);
				memcpy(arr, lua_tostring(L,-1), len);
				fmc->Values[i].Value.Binary.Data = arr;
				fmc->Values[i].Value.Binary.Size = len;
			}
			lua_pop(L,1);
		}
		else if (bit64_getvalue(L, i-narg, &val64))
		{
			fmc->Values[i].Type = FMVT_INTEGER;
			fmc->Values[i].Value.Integer = val64;
		}
		else
		{
			fmc->Values[i].Type = FMVT_NIL;
		}
	}

	return (HANDLE)fmc;
}

HANDLE LF_OpenPlugin (lua_State* L, int OpenFrom, INT_PTR Item)
{
  if (!CheckReloadDefaultScript(L) || !GetExportFunction(L, "OpenPlugin"))
    return INVALID_HANDLE_VALUE;

  if(OpenFrom == OPEN_LUAMACRO)
    return Open_Luamacro(L, OpenFrom, Item);

  lua_pushinteger(L, OpenFrom); // 1-st argument

	// 2-nd argument

  if(OpenFrom == OPEN_FROMMACRO)
  {
    struct OpenMacroInfo* data = (struct OpenMacroInfo*)Item;
    PackMacroValues(L, data->Count, data->Values);
  }
  else if (OpenFrom==OPEN_SHORTCUT || OpenFrom==OPEN_COMMANDLINE) {
    push_utf8_string(L, (const wchar_t*)Item, -1);
  }
  else if (OpenFrom==OPEN_DIALOG) {
    struct OpenDlgPluginData *data = (struct OpenDlgPluginData*)Item;
    lua_createtable(L, 0, 2);
    PutIntToTable(L, "ItemNumber", data->ItemNumber);
    NewDialogData(L, NULL, data->hDlg, FALSE);
    lua_setfield(L, -2, "hDlg");
  }
  else
    lua_pushinteger(L, Item);

  // Call export.OpenPlugin()

  if(OpenFrom == OPEN_FROMMACRO)
  {
    int top = lua_gettop(L);
    if (pcall_msg(L, 2, LUA_MULTRET) == 0)
    {
      int nret = lua_gettop(L) - top + 3; // nret
      if (nret > 0 && lua_istable(L, -nret))
      {
        lua_getfield(L, -nret, "type"); // nret+1
        if (lua_type(L,-1)==LUA_TSTRING && lua_objlen(L,-1)==5 && !strcmp("panel",lua_tostring(L,-1)))
        {
          lua_pop(L,1); // nret
          lua_rawgeti(L,-nret,1); // nret+1
          if(lua_toboolean(L, -1))
          {
            struct FarMacroCall* fmc = (struct FarMacroCall*)
              malloc(sizeof(struct FarMacroCall)+sizeof(struct FarMacroValue));
            fmc->StructSize = sizeof(*fmc);
            fmc->Count = 1;
            fmc->Values = (struct FarMacroValue*)(fmc+1);
            fmc->Callback = FillFarMacroCall_Callback;
            fmc->CallbackData = fmc;
            fmc->Values[0].Type = FMVT_PANEL;
            fmc->Values[0].Value.Pointer = RegisterObject(L); // nret

            lua_pop(L,nret); // +0
            return fmc;
          }
          lua_pop(L,nret+1); // +0
          return INVALID_HANDLE_VALUE;
        }
        lua_pop(L,1); // nret
      }
      if (nret)
      {
        HANDLE hndl = FillFarMacroCall(L,nret);
        lua_pop(L,nret); // +0
        return hndl;
      }
      return INVALID_HANDLE_VALUE;
    }
  }
  else
  {
    if (pcall_msg(L, 2, 1) == 0)
    {
      if (lua_toboolean(L, -1))        //+1: Obj
        return RegisterObject(L);      //+0

      lua_pop(L,1);
    }
  }
  return INVALID_HANDLE_VALUE;
}

void LF_ClosePlugin(lua_State* L, HANDLE hPlugin)
{
  if (GetExportFunction(L, "ClosePlugin")) { //+1: Func
    PushPluginPair(L, hPlugin);              //+3: Func,Pair
    pcall_msg(L, 2, 0);
  }
  DestroyCollector(L, hPlugin, COLLECTOR_OPI);
  luaL_unref(L, LUA_REGISTRYINDEX, (INT_PTR)hPlugin);
}

int LF_Compare(lua_State* L, HANDLE hPlugin, const struct PluginPanelItem *Item1,
               const struct PluginPanelItem *Item2, unsigned int Mode)
{
  int res = -2; // default FAR compare function should be used
  if (GetExportFunction(L, "Compare")) { //+1: Func
    PushPluginPair(L, hPlugin);          //+3: Func,Pair
    PushPanelItem(L, Item1);             //+4
    PushPanelItem(L, Item2);             //+5
    lua_pushinteger(L, Mode);            //+6
    if (0 == pcall_msg(L, 5, 1)) {       //+1
      res = lua_tointeger(L,-1);
      lua_pop(L,1);
    }
  }
  return res;
}

int LF_Configure(lua_State* L, int ItemNumber)
{
  int res = FALSE;
  if (GetExportFunction(L, "Configure")) { //+1: Func
    lua_pushinteger(L, ItemNumber);
    if(0 == pcall_msg(L, 1, 1)) {        //+1
      res = lua_toboolean(L,-1);
      lua_pop(L,1);
    }
  }
  return res;
}

int LF_DeleteFiles(lua_State* L, HANDLE hPlugin, struct PluginPanelItem *PanelItem,
  int ItemsNumber, int OpMode)
{
  int res = FALSE;
  if (GetExportFunction(L, "DeleteFiles")) {   //+1: Func
    PushPluginPair(L, hPlugin);                //+3: Func,Pair
    PushPanelItems(L, hPlugin, PanelItem, ItemsNumber); //+4
    lua_pushinteger(L, OpMode);                //+5
    if(0 == pcall_msg(L, 4, 1))    {           //+1
      res = lua_toboolean(L,-1);
      lua_pop(L,1);
    }
  }
  return res;
}

// far.MakeDirectory returns 2 values:
//    a) status (an integer; in accordance to FAR API), and
//    b) new directory name (a string; optional)
int LF_MakeDirectory (lua_State* L, HANDLE hPlugin, const wchar_t **Name, int OpMode)
{
  int res = 0;
  if (GetExportFunction(L, "MakeDirectory")) { //+1: Func
    PushPluginPair(L, hPlugin);                //+3: Func,Pair
    push_utf8_string(L, *Name, -1);            //+4
    lua_pushinteger(L, OpMode);                //+5
    if(0 == pcall_msg(L, 4, 2)) {              //+2
      res = lua_tointeger(L,-2);
      if (res == 1 && lua_isstring(L,-1)) {
        *Name = check_utf8_string(L,-1,NULL);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "MakeDirectory.Name"); // protect from GC
      }
      else if (res != -1)
        res = 0;
      lua_pop(L,2);
    }
  }
  return res;
}

int LF_ProcessEvent(lua_State* L, HANDLE hPlugin, int Event, void *Param)
{
  int res = FALSE;
  if (GetExportFunction(L, "ProcessEvent")) { //+1: Func
    PushPluginPair(L, hPlugin);        //+3
    lua_pushinteger(L, Event);         //+4
    if (Event == FE_CHANGEVIEWMODE || Event == FE_COMMAND)
      push_utf8_string(L, (wchar_t*)Param, -1); //+5
    else
      lua_pushnil(L);                  //+5
    if(0 == pcall_msg(L, 4, 1))  {     //+1
      res = lua_toboolean(L,-1);
      lua_pop(L,1);                    //+0
    }
  }
  return res;
}

int LF_ProcessHostFile(lua_State* L, HANDLE hPlugin, struct PluginPanelItem *PanelItem,
  int ItemsNumber, int OpMode)
{
  if (GetExportFunction(L, "ProcessHostFile")) {   //+1: Func
    PushPanelItems(L, hPlugin, PanelItem, ItemsNumber); //+2: Func,Item
    lua_insert(L,-2);                  //+2: Item,Func
    PushPluginPair(L, hPlugin);        //+4: Item,Func,Pair
    lua_pushvalue(L,-4);               //+5: Item,Func,Pair,Item
    lua_pushinteger(L, OpMode);        //+6: Item,Func,Pair,Item,OpMode
    int ret = pcall_msg(L, 4, 1);      //+2: Item,Res
    if (ret == 0) {
      ret = lua_toboolean(L,-1);
      lua_pop(L,1);                    //+1: Item
      UpdateFileSelection(L, PanelItem, ItemsNumber);
      return lua_pop(L,1), ret;
    }
    lua_pop(L,1);
  }
  return FALSE;
}

int LF_ProcessKey(lua_State* L, HANDLE hPlugin, int Key, unsigned int ControlState)
{
  if ((Key & ~PKF_PREPROCESS) == KEY_NONE)
    return FALSE; //ignore garbage

  if (GetExportFunction(L, "ProcessKey")) {   //+1: Func
    PushPluginPair(L, hPlugin);        //+3: Func,Pair
    lua_pushinteger(L, Key);           //+4
    lua_pushinteger(L, ControlState);  //+5
    if (pcall_msg(L, 4, 1) == 0)    {  //+1: Res
      int ret = lua_toboolean(L,-1);
      return lua_pop(L,1), ret;
    }
  }
  return FALSE;
}

int LF_PutFiles(lua_State* L, HANDLE hPlugin, struct PluginPanelItem *PanelItems,
  int ItemsNumber, int Move, int OpMode)
{
  if (GetExportFunction(L, "PutFiles")) {   //+1: Func
    PushPanelItems(L, hPlugin, PanelItems, ItemsNumber); //+2: Func,Items
    lua_insert(L,-2);                  //+2: Items,Func
    PushPluginPair(L, hPlugin);        //+4: Items,Func,Pair
    lua_pushvalue(L,-4);               //+5: Items,Func,Pair,Item
    lua_pushboolean(L, Move);          //+6: Items,Func,Pair,Item,Move
    lua_pushinteger(L, OpMode);        //+7: Items,Func,Pair,Item,Move,OpMode
    int ret = pcall_msg(L, 5, 1);      //+2: Items,Res
    if (ret == 0) {
      ret = lua_tointeger(L,-1);
      lua_pop(L,1);                    //+1: Items
      UpdateFileSelection(L, PanelItems, ItemsNumber);
      return lua_pop(L,1), ret;
    }
    lua_pop(L,1);
  }
  return 0;
}

int LF_SetDirectory(lua_State* L, HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
  if (GetExportFunction(L, "SetDirectory")) {   //+1: Func
    PushPluginPair(L, hPlugin);        //+3: Func,Pair
    push_utf8_string(L, Dir, -1);      //+4: Func,Pair,Dir
    lua_pushinteger(L, OpMode);        //+5: Func,Pair,Dir,OpMode
    int ret = pcall_msg(L, 4, 1);      //+1: Res
    if (ret == 0) {
      ret = lua_toboolean(L,-1);
      return lua_pop(L,1), ret;
    }
  }
  return FALSE;
}

int LF_SetFindList(lua_State* L, HANDLE hPlugin, const struct PluginPanelItem *PanelItems,
  int ItemsNumber)
{
  if (GetExportFunction(L, "SetFindList")) {    //+1: Func
    PushPluginPair(L, hPlugin);                 //+3: Func,Pair
    PushPanelItems(L, hPlugin, PanelItems, ItemsNumber); //+4: Func,Pair,Items
    int ret = pcall_msg(L, 3, 1);               //+1: Res
    if (ret == 0) {
      ret = lua_toboolean(L,-1);
      return lua_pop(L,1), ret;
    }
  }
  return FALSE;
}

void LF_LuaClose(TPluginData* aPlugData)
{
  lua_close(aPlugData->MainLuaState);
  dlclose(aPlugData->dlopen_handle);
  free(aPlugData->ShareDir);
}

void LF_ExitFAR(lua_State* L)
{
  DestroyPluginInfoCollector(L);
  if (GetExportFunction(L, "ExitFAR"))   //+1: Func
    pcall_msg(L, 0, 0);                  //+0
}

int LF_MayExitFAR(lua_State* L)
{
  if (GetExportFunction(L, "MayExitFAR"))  { //+1: Func
    int ret = pcall_msg(L, 0, 1);            //+1
    if (ret == 0) {
      ret = lua_toboolean(L,-1);
      lua_pop(L,1);                          //+0
      return ret;
    }
  }
  return 1;
}

void LF_GetPluginInfo(lua_State* L, struct PluginInfo *aPI)
{
  aPI->StructSize = sizeof (struct PluginInfo);
  if (!GetExportFunction(L, "GetPluginInfo"))    //+1
    return;
  if (pcall_msg(L, 0, 1) != 0)
    return;
  if (!lua_istable(L, -1)) {
    lua_pop(L,1);
    return;
  }
  //--------------------------------------------------------------------------
  DestroyPluginInfoCollector (L);
  CreatePluginInfoCollector (L);
  lua_getfield(L, LUA_REGISTRYINDEX, COLLECTOR_PI);  //+2: Info,Coll
  int cpos = lua_gettop(L);  // collector position
  lua_pushvalue(L, -2);                              //+3: Info,Coll,Info
  //--------------------------------------------------------------------------
  struct PluginInfo *PI = (struct PluginInfo*)
    AddBufToCollector (L, cpos, sizeof(struct PluginInfo));
  PI->StructSize = sizeof (struct PluginInfo);
  //--------------------------------------------------------------------------
  PI->Flags = GetOptIntFromTable (L, "Flags", 0);
  //--------------------------------------------------------------------------
  PI->DiskMenuStrings = CreateStringsArray (L, cpos, "DiskMenuStrings", &PI->DiskMenuStringsNumber);
  PI->PluginMenuStrings = CreateStringsArray (L, cpos, "PluginMenuStrings", &PI->PluginMenuStringsNumber);
  PI->PluginConfigStrings = CreateStringsArray (L, cpos, "PluginConfigStrings", &PI->PluginConfigStringsNumber);
  PI->CommandPrefix = AddStringToCollectorField(L, cpos, "CommandPrefix");
  //--------------------------------------------------------------------------
  lua_pop(L, 3);
  *aPI = *PI;
}

int LF_ProcessEditorInput (lua_State* L, const INPUT_RECORD *Rec)
{
  if (!GetExportFunction(L, "ProcessEditorInput"))   //+1: Func
    return 0;
  PushInputRecord(L, Rec);
  int ret = pcall_msg(L, 1, 1);      //+1: Res
  if (ret == 0) {
    ret = lua_toboolean(L,-1);
    return lua_pop(L,1), ret;
  }
  return 0;
}

int LF_ProcessEditorEvent (lua_State* L, int Event, void *Param)
{
  int ret = 0;
  if (GetExportFunction(L, "ProcessEditorEvent"))  { //+1: Func
    PSInfo *Info = GetPluginStartupInfo(L);
    struct EditorInfo ei;
    if (Info->EditorControl(ECTL_GETINFO, &ei))
      lua_pushinteger(L, ei.EditorID);
    else
      lua_pushnil(L);
    lua_pushinteger(L, Event);  //+3;
    switch(Event) {
      case EE_CLOSE:
      case EE_GOTFOCUS:
      case EE_KILLFOCUS:
        lua_pushinteger(L, *(int*)Param);
        break;
      case EE_REDRAW:
        lua_pushinteger(L, (INT_PTR)Param);
        break;
      default:
      case EE_READ:
      case EE_SAVE:
        lua_pushnil(L);
        break;
    }
    if (pcall_msg(L, 3, 1) == 0) {    //+1
      if (lua_isnumber(L,-1)) ret = lua_tointeger(L,-1);
      lua_pop(L,1);
    }
  }
  return ret;
}

int LF_ProcessViewerEvent (lua_State* L, int Event, void* Param)
{
  int ret = 0;
  if (GetExportFunction(L, "ProcessViewerEvent"))  { //+1: Func
    PSInfo *Info = GetPluginStartupInfo(L);
    struct ViewerInfo vi;
    vi.StructSize = sizeof(vi);
    if (Info->ViewerControl(VCTL_GETINFO, &vi))
      lua_pushinteger(L, vi.ViewerID);
    else
      lua_pushnil(L);
    lua_pushinteger(L, Event);
    switch(Event) {
      case VE_GOTFOCUS:
      case VE_KILLFOCUS:
      case VE_CLOSE:  lua_pushinteger(L, *(int*)Param); break;
      default:        lua_pushnil(L); break;
    }
    if (pcall_msg(L, 3, 1) == 0) {      //+1
      if (lua_isnumber(L,-1)) ret = lua_tointeger(L,-1);
      lua_pop(L,1);
    }
  }
  return ret;
}

int LF_ProcessDialogEvent (lua_State* L, int Event, void *Param)
{
  int ret = 0;
  struct FarDialogEvent *fde = (struct FarDialogEvent*) Param;
  BOOL PushDN = FALSE;

#ifdef LOGGING_ON
  char buf[200];
  sprintf(buf, "%s: Event=0x%X, fde=%p, fde->Msg=0x%X, fde->Param1=0x%X, fde->Param2=0x%lX",
          __func__, Event,      fde,    fde->Msg,      fde->Param1,      fde->Param2);
  Log(buf);
#endif

  if (!GetExportFunction(L, "ProcessDialogEvent")) //+1: Func
    return 0;

  lua_pushinteger(L, Event);       //+2
  lua_createtable(L, 0, 5);        //+3
  NewDialogData(L, NULL, fde->hDlg, FALSE);
  lua_setfield(L, -2, "hDlg");     //+3

  if (PushDNParams(L, fde->Msg, fde->Param1, fde->Param2)) //+6
  {
    PushDN = TRUE;
    lua_setfield(L, -4, "Param2"); //+5
    lua_setfield(L, -3, "Param1"); //+4
    lua_setfield(L, -2, "Msg");    //+3
  }
  else if (PushDMParams(L, fde->Msg, fde->Param1)) //+5
  {
    lua_setfield(L, -3, "Param1"); //+4
    lua_setfield(L, -2, "Msg");    //+3
    PutIntToTable(L, "Param2", fde->Param2); //FIXME: temporary solution
  }
  else
  {
    PutIntToTable(L, "Msg", fde->Msg);
    PutIntToTable(L, "Param1", fde->Param1);
    PutIntToTable(L, "Param2", fde->Param2);
  }

  if(pcall_msg(L, 2, 1) == 0)      //+1
  {
    if((ret=lua_toboolean(L,-1)) != 0)
    {
      fde->Result = PushDN ? ProcessDNResult(L, fde->Msg, fde->Param2) : lua_tointeger(L,-1);
    }

    lua_pop(L,1);
  }

  return ret;
}

int LF_ProcessSynchroEvent (lua_State* L, int Event, void *Param)
{
  if (Event == SE_COMMONSYNCHRO) {
    TTimerData *td = (TTimerData*)Param;
    switch (td->closeStage) {
      case 0:
        lua_rawgeti(L, LUA_REGISTRYINDEX, td->funcRef);  //+1: Func
        if (lua_type(L, -1) == LUA_TFUNCTION) {
          lua_rawgeti(L, LUA_REGISTRYINDEX, td->objRef); //+2: Obj
          pcall_msg(L, 1, 0);  //+0
        }
        else lua_pop(L, 1);
        break;

      case 1:
        break;

      case 2:
        luaL_unref(L, LUA_REGISTRYINDEX, td->funcRef);
        luaL_unref(L, LUA_REGISTRYINDEX, td->threadRef);
        luaL_unref(L, LUA_REGISTRYINDEX, td->objRef);
        break;
    }
  }
  return 0;
}

int LF_GetCustomData(lua_State* L, const wchar_t *FilePath, wchar_t **CustomData)
{
  if (GetExportFunction(L, "GetCustomData"))  { //+1: Func
    push_utf8_string(L, FilePath, -1);  //+2
    if (pcall_msg(L, 1, 1) == 0) {  //+1
      if (lua_isstring(L, -1)) {
        const wchar_t* p = utf8_to_wcstring(L, -1, NULL);
        if (p) {
          *CustomData = wcsdup(p);
          lua_pop(L, 1);
          return TRUE;
        }
      }
      lua_pop(L, 1);
    }
  }
  return FALSE;
}

void  LF_FreeCustomData(lua_State* L, wchar_t *CustomData)
{
  (void) L;
  if (CustomData) free(CustomData);
}

int LF_ProcessConsoleInput(lua_State* L, INPUT_RECORD *Rec)
{
  int ret = 0;

  if(GetExportFunction(L, "ProcessConsoleInput"))    //+1: Func
  {
    PushInputRecord(L, Rec);                         //+2

    if(pcall_msg(L, 1, 1) == 0)                      //+1: Res
    {
      if(lua_type(L,-1) == LUA_TNUMBER && lua_tonumber(L,-1) == 0)
        ret = 0;
      else if(lua_type(L,-1) == LUA_TTABLE)
      {
        FillInputRecord(L, -1, Rec);
        ret = 2;
      }
      else
        ret = lua_toboolean(L,-1);

      lua_pop(L,1);
    }
  }

  return ret;
}
