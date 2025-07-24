#ifndef LUAFAR_H
#define LUAFAR_H

#define LUAFAR_INTERNALS

#include <farplug-wide.h>
#include <farkeys.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#ifndef DLLFUNC
#define DLLFUNC __attribute__ ((visibility ("default")))
#endif

enum PLUGINDATAFLAGS
{
	PDF_SETPACKAGEPATH        = 0x0001,
	PDF_DIALOGEVENTDRAWENABLE = 0x0002,
	PDF_PROCESSINGERROR       = 0x0004,
};

typedef struct
{
	const wchar_t* ModuleName;   // copied from PluginStartupInfo
	INT_PTR        ModuleNumber; // +
	const wchar_t* RootKey;      // +
	const void*    Private;      // +

	DWORD          PluginId;
	FARWINDOWPROC  DlgProc;
	lua_State*     MainLuaState;
	char*          ShareDir;
	DWORD          Flags;
	void         (*GetGlobalInfo)(struct GlobalInfo *aInfo);
} TPluginData;

DLLFUNC int  LF_LuaOpen(const struct PluginStartupInfo *aInfo, TPluginData* aPlugData, lua_CFunction aOpenLibs);
DLLFUNC int  LF_InitOtherLuaState (lua_State *L, lua_State *Lplug, lua_CFunction aOpenLibs);
DLLFUNC void LF_LuaClose(TPluginData* aPlugData);
DLLFUNC int  LF_Message(lua_State* L, const wchar_t* aMsg, const wchar_t* aTitle, const wchar_t* aButtons, const char* aFlags, const wchar_t* aHelpTopic, const GUID *aId);
DLLFUNC void LF_RunLuafarInit(lua_State *L);
DLLFUNC BOOL LF_RunDefaultScript(lua_State* L);
DLLFUNC const wchar_t *LF_Gsub (lua_State *L, const wchar_t *s, const wchar_t *p, const wchar_t *r);
DLLFUNC LONG_PTR LF_DlgProc(lua_State *L, HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);

DLLFUNC void   LF_ClosePanel (lua_State* L, HANDLE hPlugin);
DLLFUNC int    LF_Compare (lua_State* L, HANDLE hPlugin,const struct PluginPanelItem *Item1,const struct PluginPanelItem *Item2,unsigned int Mode);
DLLFUNC int    LF_Configure(lua_State* L, const struct ConfigureInfo *Info);
DLLFUNC int    LF_DeleteFiles (lua_State* L, HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
DLLFUNC void   LF_ExitFAR (lua_State* L);
DLLFUNC void   LF_FreeFindData (lua_State* L, HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber);
DLLFUNC void   LF_FreeVirtualFindData (lua_State* L, HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber);
DLLFUNC int    LF_GetFiles (lua_State* L, HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode);
DLLFUNC int    LF_GetFindData (lua_State* L, HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
DLLFUNC void   LF_GetOpenPanelInfo (lua_State* L, HANDLE hPlugin,struct OpenPluginInfo *Info);
DLLFUNC void   LF_GetPluginInfo (lua_State* L, struct PluginInfo *Info);
DLLFUNC int    LF_GetVirtualFindData (lua_State* L, HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,const wchar_t *Path);
DLLFUNC int    LF_MakeDirectory (lua_State* L, HANDLE hPlugin,const wchar_t **Name,int OpMode);
DLLFUNC int    LF_MayExitFAR (lua_State* L);
DLLFUNC HANDLE LF_OpenFilePlugin (lua_State* L, const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode);
DLLFUNC HANDLE LF_Open (lua_State* L, int OpenFrom,INT_PTR Item);
DLLFUNC int    LF_ProcessDialogEvent (lua_State* L, int Event,void *Param);
DLLFUNC int    LF_ProcessEditorEvent (lua_State* L, int Event,void *Param);
DLLFUNC int    LF_ProcessEditorInput (lua_State* L, const INPUT_RECORD *Rec);
DLLFUNC int    LF_ProcessPanelEvent (lua_State* L, HANDLE hPlugin,int Event,void *Param);
DLLFUNC int    LF_ProcessHostFile (lua_State* L, HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
DLLFUNC int    LF_ProcessKey (lua_State* L, HANDLE hPlugin,int Key,unsigned int ControlState);
DLLFUNC int    LF_ProcessSynchroEvent (lua_State* L, int Event,void *Param);
DLLFUNC int    LF_ProcessViewerEvent (lua_State* L, int Event,void *Param);
DLLFUNC int    LF_PutFiles (lua_State* L, HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t *SrcPath,int OpMode);
DLLFUNC int    LF_SetDirectory (lua_State* L, HANDLE hPlugin,const wchar_t *Dir,int OpMode);
DLLFUNC int    LF_SetFindList (lua_State* L, HANDLE hPlugin,const struct PluginPanelItem *PanelItem,int ItemsNumber);
DLLFUNC int    LF_GetCustomData(lua_State* L, const wchar_t *FilePath, wchar_t **CustomData);
DLLFUNC void   LF_FreeCustomData(lua_State* L, wchar_t *CustomData);
DLLFUNC int    LF_ProcessConsoleInput(lua_State* L, INPUT_RECORD *Rec);
DLLFUNC int    LF_GetLinkTarget(lua_State *L, HANDLE hPlugin,	struct PluginPanelItem *PanelItem, wchar_t *Target, size_t TargetSize, int OpMode);
DLLFUNC HANDLE LF_Analyse(lua_State* L, const struct AnalyseInfo *Info);
DLLFUNC void   LF_CloseAnalyse(lua_State* L, const struct CloseAnalyseInfo *Info);

#ifdef __cplusplus
}
#endif

#endif // LUAFAR_H
