#include <dlfcn.h> //dlopen

#include <windows.h>
#include <farplug-mb.h>

extern "C"
{
#include "luafar.h"
#include "util.h"
#include "ustring.h"

  HANDLE OptHandle(lua_State *L);
  void FillPluginPanelItem(lua_State *L, struct PluginPanelItem *pi, int CollectorPos);
}


const char* GetStringFromTable(lua_State *L, const char *Key)
{
  lua_getfield(L, -1, Key);
  const char *val = lua_tostring(L,-1);
  lua_pop(L,1);
  return val;
}


// input table is on stack top (-1)
void FillAnsiPluginPanelItem (lua_State *L, oldfar::PluginPanelItem *pi)
{
  memset(pi, 0, sizeof(*pi));
  pi->FindData.dwFileAttributes = GetAttrFromTable(L);
  pi->FindData.ftCreationTime   = GetFileTimeFromTable(L, "CreationTime");
  pi->FindData.ftLastAccessTime = GetFileTimeFromTable(L, "LastAccessTime");
  pi->FindData.ftLastWriteTime  = GetFileTimeFromTable(L, "LastWriteTime");
  pi->FindData.nFileSize        = GetFileSizeFromTable(L, "FileSize");
  pi->NumberOfLinks             = GetOptIntFromTable  (L, "NumberOfLinks", 0);

  const char* FileName          = GetStringFromTable  (L, "FileName");
  if (FileName)
    strncpy(pi->FindData.cFileName, FileName, MAX_NAME-1);
  pi->Description        = (char*)GetStringFromTable  (L, "Description");
  pi->Owner              = (char*)GetStringFromTable  (L, "Owner");

  // custom column data
  lua_getfield(L, -1, "CustomColumnData");
  if (lua_istable(L,-1)) {
    int i;
    pi->CustomColumnNumber = lua_objlen(L,-1);
    pi->CustomColumnData = (char**) malloc(pi->CustomColumnNumber * sizeof(char*));
    for (i=0; i < pi->CustomColumnNumber; i++) {
      lua_rawgeti(L, -1, i+1);
      pi->CustomColumnData[i] = (char*)lua_tostring(L, -1);
      lua_pop(L,1);
    }
  }
  lua_pop(L,1);

  // prevent Far from treating UserData as pointer and copying data from it
  pi->Flags = GetOptIntFromTable(L, "Flags", 0) & ~oldfar::PPIF_USERDATA;
}


HMODULE GetPluginModuleHandle(lua_State *L, const PSInfo *psInfo, HANDLE plug_handle, int *ansi)
{
  HMODULE dll_handle = NULL;
  size_t size = psInfo->PluginsControlV3(plug_handle, PCTL_GETPLUGININFORMATION, 0, NULL);
  if (size != 0)
  {
    struct FarGetPluginInformation *piInfo = (struct FarGetPluginInformation *) malloc(size);
    if (piInfo != NULL)
    {
      piInfo->StructSize = sizeof(*piInfo);
      if (psInfo->PluginsControlV3(plug_handle, PCTL_GETPLUGININFORMATION, size, piInfo))
      {
        *ansi = piInfo->Flags & FPF_ANSI;
        push_utf8_string(L, piInfo->ModuleName, -1);
        dll_handle = dlopen(lua_tostring(L,-1), RTLD_NOW | RTLD_GLOBAL); //not to forget to dlclose later
        lua_pop(L, 1);
      }
      free(piInfo);
    }
  }
  return dll_handle;
}


int _GetFilesW(lua_State *L, HANDLE hPanel, HMODULE dll_handle)
{
  typedef int (WINAPI * T_GetFilesW)(HANDLE, struct PluginPanelItem*, int, int, const wchar_t**, int);

  luaL_checktype(L, 2, LUA_TTABLE);  //2-nd argument
  size_t numLines = lua_objlen(L, 2);
  int Move = lua_toboolean(L, 3);    //3-rd argument
  const wchar_t* DestPath = check_utf8_string(L, 4, NULL); //4-th argument
  int OpMode = luaL_optinteger(L, 5, OPM_FIND|OPM_SILENT); //5-th argument

  T_GetFilesW getfilesW;
  if (NULL == (getfilesW = (T_GetFilesW) dlsym(dll_handle, "GetFilesW"))) {
    dlclose(dll_handle);
    return FALSE;
  }

  PluginPanelItem *ppi, *ppi_curr;
  ppi = ppi_curr = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * numLines);
  if (ppi == NULL)
  {
    dlclose(dll_handle);
    return FALSE;
  }

  lua_newtable(L); //collector
  for (size_t i=1; i<=numLines; i++)
  {
    lua_pushinteger(L, i);
    lua_gettable(L, 2);
    if(lua_istable(L,-1))
      FillPluginPanelItem(L, ppi_curr++, 0);
    else
      i = numLines + 1; //break the loop
    lua_pop(L,1);
  }

  int ret = getfilesW(hPanel, ppi, ppi_curr-ppi, Move, &DestPath, OpMode);
  for(; ppi<ppi_curr; ppi++)
    free((void*)ppi->CustomColumnData);
  free(ppi);
  dlclose(dll_handle);
  return ret;
}


int _GetFilesA(lua_State *L, HANDLE hPanel, HMODULE dll_handle)
{
  typedef int (WINAPI * T_GetFilesA)(HANDLE, oldfar::PluginPanelItem*, int, int, char*, int);

  luaL_checktype(L, 2, LUA_TTABLE);  //2-nd argument
  size_t numLines = lua_objlen(L, 2);
  int Move = lua_toboolean(L, 3);    //3-rd argument

  const char* DestPathIn = luaL_checkstring(L, 4); //4-th argument
  char DestPathOut[MAX_PATH];
  strncpy(DestPathOut, DestPathIn, MAX_PATH-1);
  DestPathOut[MAX_PATH-1] = 0;

  int OpMode = luaL_optinteger(L, 5, oldfar::OPM_FIND|oldfar::OPM_SILENT); //5-th argument

  T_GetFilesA getfilesA;
  if (NULL == (getfilesA = (T_GetFilesA) dlsym(dll_handle, "GetFiles"))) {
    dlclose(dll_handle);
    return FALSE;
  }

  oldfar::PluginPanelItem *ppi, *ppi_curr;
  ppi = ppi_curr = (oldfar::PluginPanelItem*)malloc(sizeof(oldfar::PluginPanelItem) * numLines);
  if (ppi == NULL)
  {
    dlclose(dll_handle);
    return FALSE;
  }

  // collector table not needed for the ANSI variant
  for (size_t i=1; i<=numLines; i++)
  {
    lua_pushinteger(L, i);
    lua_gettable(L, 2);
    if(lua_istable(L,-1))
      FillAnsiPluginPanelItem(L, ppi_curr++);
    else
      i = numLines + 1; //break the loop
    lua_pop(L,1);
  }

  int ret = getfilesA(hPanel, ppi, ppi_curr-ppi, Move, DestPathOut, OpMode);
  for(; ppi<ppi_curr; ppi++)
    free((void*)ppi->CustomColumnData);
  free(ppi);
  dlclose(dll_handle);
  return ret;
}


// result = far.Host.GetFiles (handle, Items, Move, DestPath [, OpMode])
extern "C" int far_host_GetFiles(lua_State *L)
{
  PSInfo *psInfo = GetPluginStartupInfo(L);
  HANDLE panHandle = OptHandle(L);   //1-st argument

  // get plugin handle (for obtaining dll name)
  PanelInfo panInfo;
  if (! (panHandle && psInfo->Control(panHandle,FCTL_GETPANELINFO,0,(LONG_PTR)&panInfo) && panInfo.PluginHandle) )
    return lua_pushinteger(L,0), 1;

  // get panel handle (a parameter to dll function)
  HANDLE hPanel;
  psInfo->Control(panHandle, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)&hPanel);
  if (hPanel == INVALID_HANDLE_VALUE)
    return lua_pushinteger(L,0), 1;

  // get dll handle (for obtaining dll function address)
  HMODULE dll_handle;
  int ansi;
  if (NULL == (dll_handle = GetPluginModuleHandle(L, psInfo, panInfo.PluginHandle, &ansi)))
    return lua_pushinteger(L,0), 1;

  lua_pushinteger(L, (ansi ? _GetFilesA : _GetFilesW)(L, hPanel, dll_handle));
  return 1;
}

const luaL_Reg far_host_funcs[] =
{
  {"GetFiles",      far_host_GetFiles},
//  {"PutFiles",      far_host_PutFiles},
//  {"GetFindData",   far_host_GetFindData},
//  {"SetDirectory",  far_host_SetDirectory},
//  {"FreeUserData",  far_host_FreeUserData},

	{NULL, NULL}
};

extern "C" int luaopen_far_host(lua_State *L)
{
  lua_newtable(L);
  luaL_register(L, NULL, far_host_funcs);
  return 1;
}
