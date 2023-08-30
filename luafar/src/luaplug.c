//---------------------------------------------------------------------------
#define FAR_DONT_USE_INTERNALS
#include "luafar.h"

#ifndef LUAPLUG
#define LUAPLUG __attribute__ ((visibility ("default")))
#endif

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

#ifdef FUNC_OPENLIBS
extern int FUNC_OPENLIBS (lua_State*);
#else
#define FUNC_OPENLIBS NULL
#endif

static lua_State* LS;
static TPluginData PluginData;
//---------------------------------------------------------------------------

lua_State* GetLuaState()
{
  return LS;
}
//---------------------------------------------------------------------------

int LUAPLUG luaopen_luaplug (lua_State *L)
{
  return LF_InitOtherLuaState(L, LS, FUNC_OPENLIBS);
}
//---------------------------------------------------------------------------

static LONG_PTR WINAPI DlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
  return LF_DlgProc(LS, hDlg, Msg, Param1, Param2);
}

void LUAPLUG GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { PLUG_VERSION };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = SYS_ID;
  aInfo->Version       = Version;
  aInfo->Title         = WIDEN(PLUG_TITLE);
  aInfo->Description   = WIDEN(PLUG_DESCRIPTION);
  aInfo->Author        = WIDEN(PLUG_AUTHOR);
}

void LUAPLUG SetStartupInfoW(const struct PluginStartupInfo *aInfo)
{
  if (!aInfo->LuafarHandle)
    return; // luafar.so is not loaded

  PluginData.ModuleName    = aInfo->ModuleName;
  PluginData.ModuleNumber  = aInfo->ModuleNumber;
  PluginData.RootKey       = aInfo->RootKey;
  PluginData.Private       = aInfo->Private;
  PluginData.DlgProc       = DlgProc;
  PluginData.PluginId      = SYS_ID;
  PluginData.GetGlobalInfo = GetGlobalInfoW;
#ifdef SETPACKAGEPATH
  PluginData.Flags |= LPF_SETPACKAGEPATH;
#endif

  if (!LS && LF_LuaOpen(aInfo, &PluginData, FUNC_OPENLIBS)) //includes opening "far" library
    LS = PluginData.MainLuaState;

  if (LS && !LF_RunDefaultScript(LS))  {
    LF_LuaClose(&PluginData);
    LS = NULL;
  }
}
//---------------------------------------------------------------------------

void LUAPLUG GetPluginInfoW(struct PluginInfo *aInfo)
{
  if(LS) {
    LF_GetPluginInfo (LS, aInfo);
    aInfo->SysID = SYS_ID;
  }
}
//---------------------------------------------------------------------------

#if defined(EXPORT_OPENPLUGIN) || defined(EXPORT_ALL)
HANDLE LUAPLUG OpenPluginW(int OpenFrom, INT_PTR Item)
{
  return LS ? LF_OpenPlugin(LS, OpenFrom, Item) : INVALID_HANDLE_VALUE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_OPENFILEPLUGIN) || defined(EXPORT_ALL)
HANDLE LUAPLUG OpenFilePluginW(const wchar_t *Name, const unsigned char *Data,
  int DataSize, int OpMode)
{
  return LS ? LF_OpenFilePlugin(LS, Name, Data, DataSize, OpMode) : INVALID_HANDLE_VALUE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETFINDDATA) || defined(EXPORT_ALL)
int LUAPLUG GetFindDataW(HANDLE hPlugin, struct PluginPanelItem **pPanelItem,
                        int *pItemsNumber, int OpMode)
{
  return LS ? LF_GetFindData(LS, hPlugin, pPanelItem, pItemsNumber, OpMode) : FALSE;
}
//---------------------------------------------------------------------------

void LUAPLUG FreeFindDataW(HANDLE hPlugin, struct PluginPanelItem *PanelItem,
                         int ItemsNumber)
{
  if(LS) LF_FreeFindData(LS, hPlugin, PanelItem, ItemsNumber);
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_CLOSEPLUGIN) || defined(EXPORT_ALL)
void LUAPLUG ClosePluginW(HANDLE hPlugin)
{
  if(LS) LF_ClosePanel(LS, hPlugin);
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETFILES) || defined(EXPORT_ALL)
int LUAPLUG GetFilesW(HANDLE hPlugin, struct PluginPanelItem *PanelItem,
  int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode)
{
  return LS ? LF_GetFiles(LS,hPlugin,PanelItem,ItemsNumber,Move,DestPath,OpMode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETOPENPLUGININFO) || defined(EXPORT_ALL)
void LUAPLUG GetOpenPluginInfoW(HANDLE hPlugin, struct OpenPluginInfo *Info)
{
  if(LS) LF_GetOpenPluginInfo(LS, hPlugin, Info);
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_EXITFAR) || defined(EXPORT_ALL)
void LUAPLUG ExitFARW()
{
  if(LS) {
    LF_ExitFAR(LS);
    LF_LuaClose(&PluginData);
    LS = NULL;
  }
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_MAYEXITFAR) || defined(EXPORT_ALL)
int LUAPLUG MayExitFARW()
{
  return LS ? LF_MayExitFAR(LS) : 1;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_COMPARE) || defined(EXPORT_ALL)
int LUAPLUG CompareW(HANDLE hPlugin, const struct PluginPanelItem *Item1,
                    const struct PluginPanelItem *Item2, unsigned int Mode)
{
  return LS ? LF_Compare(LS, hPlugin, Item1, Item2, Mode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_CONFIGURE) || defined(EXPORT_ALL)
int LUAPLUG ConfigureW(int ItemNumber)
{
  return LS ? LF_Configure(LS, ItemNumber) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_DELETEFILES) || defined(EXPORT_ALL)
int LUAPLUG DeleteFilesW(HANDLE hPlugin, struct PluginPanelItem *PanelItem,
  int ItemsNumber, int OpMode)
{
  return LS ? LF_DeleteFiles(LS, hPlugin, PanelItem, ItemsNumber, OpMode) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETVIRTUALFINDDATA) || defined(EXPORT_ALL)
int LUAPLUG GetVirtualFindDataW(HANDLE hPlugin,
  struct PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path)
{
  if(LS) return LF_GetVirtualFindData(LS,hPlugin,pPanelItem,pItemsNumber,Path);
  return FALSE;
}

void LUAPLUG FreeVirtualFindDataW(HANDLE hPlugin,
  struct PluginPanelItem *PanelItem, int ItemsNumber)
{
  if(LS) LF_FreeVirtualFindData(LS, hPlugin, PanelItem, ItemsNumber);
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_MAKEDIRECTORY) || defined(EXPORT_ALL)
int LUAPLUG MakeDirectoryW(HANDLE hPlugin, const wchar_t **Name, int OpMode)
{
  return LS ? LF_MakeDirectory(LS, hPlugin, Name, OpMode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSEVENT) || defined(EXPORT_ALL)
int LUAPLUG ProcessEventW(HANDLE hPlugin, int Event, void *Param)
{
  return LS ? LF_ProcessEvent(LS, hPlugin, Event, Param) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSHOSTFILE) || defined(EXPORT_ALL)
int LUAPLUG ProcessHostFileW(HANDLE hPlugin, struct PluginPanelItem *PanelItem,
  int ItemsNumber, int OpMode)
{
  return LS ? LF_ProcessHostFile(LS, hPlugin, PanelItem, ItemsNumber, OpMode) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSKEY) || defined(EXPORT_ALL)
int LUAPLUG ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState)
{
  return LS ? LF_ProcessKey(LS, hPlugin, Key, ControlState) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PUTFILES) || defined(EXPORT_ALL)
int LUAPLUG PutFilesW(HANDLE hPlugin, struct PluginPanelItem *PanelItem,
  int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode)
{
  return LS ? LF_PutFiles(LS, hPlugin, PanelItem, ItemsNumber, Move, OpMode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_SETDIRECTORY) || defined(EXPORT_ALL)
int LUAPLUG SetDirectoryW(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
  return LS ? LF_SetDirectory(LS, hPlugin, Dir, OpMode) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_SETFINDLIST) || defined(EXPORT_ALL)
int LUAPLUG SetFindListW(HANDLE hPlugin, const struct PluginPanelItem *PanelItem, int ItemsNumber)
{
  return LS ? LF_SetFindList(LS, hPlugin, PanelItem, ItemsNumber) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETMINFARVERSION) || defined(EXPORT_ALL)
int LUAPLUG GetMinFarVersionW (void)
{
  return PLUG_MINFARVERSION;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSEDITORINPUT) || defined(EXPORT_ALL)
int LUAPLUG ProcessEditorInputW(const INPUT_RECORD *Rec)
{
  return LS ? LF_ProcessEditorInput(LS, Rec) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSEDITOREVENT) || defined(EXPORT_ALL)
int LUAPLUG ProcessEditorEventW(int Event, void *Param)
{
  return LS ? LF_ProcessEditorEvent(LS, Event, Param) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSVIEWEREVENT) || defined(EXPORT_ALL)
int LUAPLUG ProcessViewerEventW(int Event, void *Param)
{
  return LS ? LF_ProcessViewerEvent(LS, Event, Param) : 0;
}
#endif
//---------------------------------------------------------------------------

//exported unconditionally to enable far.Timer's work
int LUAPLUG ProcessSynchroEventW(int Event, void *Param)
{
  return LS ? LF_ProcessSynchroEvent(LS, Event, Param) : 0;
}
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSDIALOGEVENT) || defined(EXPORT_ALL)
int LUAPLUG ProcessDialogEventW(int Event, void *Param)
{
  return LS ? LF_ProcessDialogEvent(LS, Event, Param) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETCUSTOMDATA) || defined(EXPORT_ALL)
int LUAPLUG GetCustomDataW(const wchar_t *FilePath, wchar_t **CustomData)
{
  return LS ? LF_GetCustomData(LS, FilePath, CustomData) : 0;
}

void LUAPLUG FreeCustomDataW(wchar_t *CustomData)
{
  if(LS) LF_FreeCustomData(LS, CustomData);
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSCONSOLEINPUT) || defined(EXPORT_ALL)
int LUAPLUG ProcessConsoleInputW(INPUT_RECORD *Rec)
{
  return LS ? LF_ProcessConsoleInput(LS, Rec) : 0;
}
#endif
//---------------------------------------------------------------------------
