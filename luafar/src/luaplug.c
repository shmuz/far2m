//---------------------------------------------------------------------------
#include <lua.h>
#include "lf_luafar.h"

#ifndef LUAPLUG
#define LUAPLUG __attribute__ ((visibility ("default")))
#endif

#ifdef FUNC_OPENLIBS
extern int FUNC_OPENLIBS (lua_State*);
#else
#define FUNC_OPENLIBS NULL
#endif

static lua_State* LS;
static TPluginData PluginData;
//---------------------------------------------------------------------------

LUAPLUG lua_State* GetLuaState()
{
	return LS;
}
//---------------------------------------------------------------------------

LUAPLUG int luaopen_luaplug (lua_State *L)
{
	return LF_InitOtherLuaState(L, LS, FUNC_OPENLIBS);
}
//---------------------------------------------------------------------------

static LONG_PTR WINAPI DlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	return LF_DlgProc(LS, hDlg, Msg, Param1, Param2);
}

LUAPLUG void SetStartupInfoW(const struct PluginStartupInfo *aInfo)
{
	if (!aInfo->LuafarLoaded)
		return; // luafar.so is not loaded

	struct GlobalInfo globInfo;
	GetGlobalInfoW(&globInfo);

	PluginData.ModuleName    = aInfo->ModuleName;
	PluginData.ModuleNumber  = aInfo->ModuleNumber;
	PluginData.Private       = aInfo->Private;
	PluginData.DlgProc       = DlgProc;
	PluginData.PluginId      = globInfo.SysID;
	PluginData.GetGlobalInfo = GetGlobalInfoW;
#ifdef SETPACKAGEPATH
	PluginData.Flags |= PDF_SETPACKAGEPATH;
#endif

	if (!LS && LF_LuaOpen(aInfo, &PluginData, FUNC_OPENLIBS)) //includes opening "far" library
		LS = PluginData.MainLuaState;

	if (LS) {
#ifndef NO_RUN_LUAFAR_INIT
		LF_RunLuafarInit(LS);
#endif

		if (!LF_RunDefaultScript(LS))  {
			LF_LuaClose(&PluginData);
			LS = NULL;
		}
	}
}
//---------------------------------------------------------------------------

LUAPLUG void GetPluginInfoW(struct PluginInfo *aInfo)
{
	if (LS) {
		LF_GetPluginInfo (LS, aInfo);
		aInfo->SysID = PluginData.PluginId;
	}
}
//---------------------------------------------------------------------------

#if defined(EXPORT_OPENPLUGIN) || defined(EXPORT_ALL)
LUAPLUG HANDLE OpenPluginW(int OpenFrom, INT_PTR Item)
{
	return LS ? LF_Open(LS, OpenFrom, Item) : INVALID_HANDLE_VALUE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETFINDDATA) || defined(EXPORT_ALL)
LUAPLUG int GetFindDataW(HANDLE hPanel, struct PluginPanelItem **pPanelItem,
												int *pItemsNumber, int OpMode)
{
	return LS ? LF_GetFindData(LS, hPanel, pPanelItem, pItemsNumber, OpMode) : FALSE;
}
//---------------------------------------------------------------------------

LUAPLUG void FreeFindDataW(HANDLE hPanel, struct PluginPanelItem *PanelItem,
												 int ItemsNumber)
{
	if (LS) LF_FreeFindData(LS, hPanel, PanelItem, ItemsNumber);
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_CLOSEPLUGIN) || defined(EXPORT_ALL)
LUAPLUG void ClosePluginW(HANDLE hPanel)
{
	if (LS) LF_ClosePanel(LS, hPanel);
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETFILES) || defined(EXPORT_ALL)
LUAPLUG int GetFilesW(HANDLE hPanel, struct PluginPanelItem *PanelItem,
	int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode)
{
	return LS ? LF_GetFiles(LS,hPanel,PanelItem,ItemsNumber,Move,DestPath,OpMode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETOPENPLUGININFO) || defined(EXPORT_ALL)
LUAPLUG void GetOpenPluginInfoW(HANDLE hPanel, struct OpenPluginInfo *Info)
{
	if (LS) LF_GetOpenPanelInfo(LS, hPanel, Info);
}
#endif
//---------------------------------------------------------------------------

LUAPLUG void ExitFARW()
{
	if (LS) {
		LF_ExitFAR(LS);
		LF_LuaClose(&PluginData);
		LS = NULL;
	}
}
//---------------------------------------------------------------------------

#if defined(EXPORT_MAYEXITFAR) || defined(EXPORT_ALL)
LUAPLUG int MayExitFARW()
{
	return LS ? LF_MayExitFAR(LS) : 1;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_COMPARE) || defined(EXPORT_ALL)
LUAPLUG int CompareW(HANDLE hPanel, const struct PluginPanelItem *Item1,
										const struct PluginPanelItem *Item2, unsigned int Mode)
{
	return LS ? LF_Compare(LS, hPanel, Item1, Item2, Mode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_CONFIGURE) || defined(EXPORT_ALL)
LUAPLUG int ConfigureV3W(const struct ConfigureInfo *Info)
{
	return LS ? LF_Configure(LS, Info) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_DELETEFILES) || defined(EXPORT_ALL)
LUAPLUG int DeleteFilesW(HANDLE hPanel, struct PluginPanelItem *PanelItem,
	int ItemsNumber, int OpMode)
{
	return LS ? LF_DeleteFiles(LS, hPanel, PanelItem, ItemsNumber, OpMode) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_MAKEDIRECTORY) || defined(EXPORT_ALL)
LUAPLUG int MakeDirectoryW(HANDLE hPanel, const wchar_t **Name, int OpMode)
{
	return LS ? LF_MakeDirectory(LS, hPanel, Name, OpMode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSEVENT) || defined(EXPORT_ALL)
LUAPLUG int ProcessEventW(HANDLE hPanel, int Event, void *Param)
{
	return LS ? LF_ProcessPanelEvent(LS, hPanel, Event, Param) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSHOSTFILE) || defined(EXPORT_ALL)
LUAPLUG int ProcessHostFileW(HANDLE hPanel, struct PluginPanelItem *PanelItem,
	int ItemsNumber, int OpMode)
{
	return LS ? LF_ProcessHostFile(LS, hPanel, PanelItem, ItemsNumber, OpMode) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSKEY) || defined(EXPORT_ALL)
LUAPLUG int ProcessKeyW(HANDLE hPanel, int Key, unsigned int ControlState)
{
	return LS ? LF_ProcessKey(LS, hPanel, Key, ControlState) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PUTFILES) || defined(EXPORT_ALL)
LUAPLUG int PutFilesW(HANDLE hPanel, struct PluginPanelItem *PanelItem,
	int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode)
{
	return LS ? LF_PutFiles(LS, hPanel, PanelItem, ItemsNumber, Move, SrcPath, OpMode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_SETDIRECTORY) || defined(EXPORT_ALL)
LUAPLUG int SetDirectoryW(HANDLE hPanel, const wchar_t *Dir, int OpMode)
{
	return LS ? LF_SetDirectory(LS, hPanel, Dir, OpMode) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_SETFINDLIST) || defined(EXPORT_ALL)
LUAPLUG int SetFindListW(HANDLE hPanel, const struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	return LS ? LF_SetFindList(LS, hPanel, PanelItem, ItemsNumber) : FALSE;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSEDITORINPUT) || defined(EXPORT_ALL)
LUAPLUG int ProcessEditorInputW(const INPUT_RECORD *Rec)
{
	return LS ? LF_ProcessEditorInput(LS, Rec) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSEDITOREVENT) || defined(EXPORT_ALL)
LUAPLUG int ProcessEditorEventV3W(const struct ProcessEditorEventInfo *Info)
{
	return LS ? LF_ProcessEditorEvent(LS, Info) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSVIEWEREVENT) || defined(EXPORT_ALL)
LUAPLUG int ProcessViewerEventW(int Event, void *Param)
{
	return LS ? LF_ProcessViewerEvent(LS, Event, Param) : 0;
}
#endif
//---------------------------------------------------------------------------

//exported unconditionally to enable far.Timer's work
LUAPLUG int ProcessSynchroEventW(int Event, void *Param)
{
	return LS ? LF_ProcessSynchroEvent(LS, Event, Param) : 0;
}
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSDIALOGEVENT) || defined(EXPORT_ALL)
LUAPLUG int ProcessDialogEventW(int Event, void *Param)
{
	return LS ? LF_ProcessDialogEvent(LS, Event, Param) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETCUSTOMDATA) || defined(EXPORT_ALL)
LUAPLUG int GetCustomDataW(const wchar_t *FilePath, wchar_t **CustomData)
{
	return LS ? LF_GetCustomData(LS, FilePath, CustomData) : 0;
}

LUAPLUG void FreeCustomDataW(wchar_t *CustomData)
{
	if (LS) LF_FreeCustomData(LS, CustomData);
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_PROCESSCONSOLEINPUT) || defined(EXPORT_ALL)
LUAPLUG int ProcessConsoleInputW(INPUT_RECORD *Rec)
{
	return LS ? LF_ProcessConsoleInput(LS, Rec) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_GETLINKTARGET) || defined(EXPORT_ALL)
LUAPLUG int GetLinkTargetW(
	HANDLE hPanel, struct PluginPanelItem *PanelItem, wchar_t *Target,
	size_t TargetSize, int OpMode)
{
	return LS ? LF_GetLinkTarget(LS, hPanel, PanelItem, Target, TargetSize, OpMode) : 0;
}
#endif
//---------------------------------------------------------------------------

#if defined(EXPORT_ANALYSE) || defined(EXPORT_ALL)
LUAPLUG HANDLE AnalyseW(const struct AnalyseInfo *Info)
{
	return LS ? LF_Analyse(LS, Info) : INVALID_HANDLE_VALUE;
}

LUAPLUG void CloseAnalyseW(const struct CloseAnalyseInfo *Info)
{
	if (LS)
		LF_CloseAnalyse(LS, Info);
}
#endif
//---------------------------------------------------------------------------
