#pragma once

/*
plugins.hpp

Работа с плагинами (низкий уровень, кое-что повыше в flplugin.cpp)
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "bitflags.hpp"
#include <farplug-wide.h>
#include "plclass.hpp"
#include "PluginA.hpp"
#include "PluginW.hpp"
#include <string>
#include <map>
#include <unordered_map>
#include <mutex>

extern const char *FmtDiskMenuStringD;
extern const char *FmtPluginMenuStringD;
extern const char *FmtPluginConfigStringD;


class SaveScreen;
class FileEditor;
class Viewer;
class Frame;
class Panel;
struct FileListItem;

enum
{
	PLUGIN_FARGETFILE,
	PLUGIN_FARGETFILES,
	PLUGIN_FARPUTFILES,
	PLUGIN_FARDELETEFILES,
	PLUGIN_FARMAKEDIRECTORY,
	PLUGIN_FAROTHER
};

// флаги для поля Plugin.WorkFlags
enum PLUGINITEMWORKFLAGS
{
	PIWF_CACHED        = 0x00000001, // кешируется
	PIWF_PRELOADED     = 0x00000002, //
	PIWF_DONTLOADAGAIN = 0x00000004, // не загружать плагин снова, ставится в
	//   результате проверки требуемой версии фара
};

// флаги для поля Plugin.FuncFlags - активности функций
enum PLUGINITEMCALLFUNCFLAGS
{
	PICFF_LOADED               = 0x00000001, // DLL загружен ;-)
	PICFF_SETSTARTUPINFO       = 0x00000002, //
	PICFF_OPENPLUGIN           = 0x00000004, //
	PICFF_OPENFILEPLUGIN       = 0x00000008, //
	PICFF_CLOSEPLUGIN          = 0x00000010, //
	PICFF_GETPLUGININFO        = 0x00000020, //
	PICFF_GETOPENPLUGININFO    = 0x00000040, //
	PICFF_GETFINDDATA          = 0x00000080, //
	PICFF_FREEFINDDATA         = 0x00000100, //
	PICFF_GETVIRTUALFINDDATA   = 0x00000200, //
	PICFF_FREEVIRTUALFINDDATA  = 0x00000400, //
	PICFF_SETDIRECTORY         = 0x00000800, //
	PICFF_GETFILES             = 0x00001000, //
	PICFF_PUTFILES             = 0x00002000, //
	PICFF_DELETEFILES          = 0x00004000, //
	PICFF_MAKEDIRECTORY        = 0x00008000, //
	PICFF_PROCESSHOSTFILE      = 0x00010000, //
	PICFF_SETFINDLIST          = 0x00020000, //
	PICFF_CONFIGURE            = 0x00040000, //
	PICFF_EXITFAR              = 0x00080000, //
	PICFF_PROCESSKEY           = 0x00100000, //
	PICFF_PROCESSEVENT         = 0x00200000, //
	PICFF_PROCESSEDITOREVENT   = 0x00400000, //
	PICFF_COMPARE              = 0x00800000, //
	PICFF_PROCESSEDITORINPUT   = 0x01000000, //
	PICFF_MINFARVERSION        = 0x02000000, //
	PICFF_PROCESSVIEWEREVENT   = 0x04000000, //
	PICFF_PROCESSDIALOGEVENT   = 0x08000000, //
	PICFF_PROCESSSYNCHROEVENT  = 0x10000000, //
	// PICFF_PANELPLUGIN - первая попытка определиться с понятием "это панель"
	PICFF_PANELPLUGIN          = PICFF_OPENFILEPLUGIN|
	PICFF_GETFINDDATA|
	PICFF_FREEFINDDATA|
	PICFF_GETVIRTUALFINDDATA|
	PICFF_FREEVIRTUALFINDDATA|
	PICFF_SETDIRECTORY|
	PICFF_GETFILES|
	PICFF_PUTFILES|
	PICFF_DELETEFILES|
	PICFF_MAKEDIRECTORY|
	PICFF_PROCESSHOSTFILE|
	PICFF_SETFINDLIST|
	PICFF_PROCESSKEY|
	PICFF_PROCESSEVENT|
	PICFF_COMPARE|
	PICFF_GETOPENPLUGININFO,
};

// флаги для поля PluginManager.Flags
enum PLUGINSETFLAGS
{
	PSIF_ENTERTOOPENPLUGIN        = 0x00000001, // ввалились в плагин OpenPlugin
	PSIF_DIALOG                   = 0x00000002, // была бадяга с диалогом
	PSIF_PLUGINSLOADDED           = 0x80000000, // плагины загружены
};

enum OPENFILEPLUGINTYPE
{
	OFP_NORMAL,
	OFP_ALTERNATIVE,
	OFP_SEARCH,
	OFP_SHORTCUT,
	OFP_CREATE,
	OFP_EXTRACT,
	OFP_COMMANDS,
};

struct PanelHandle
{
	HANDLE hPanel;
	Plugin *pPlugin;
	unsigned int RefCnt;

	PanelHandle() : hPanel(nullptr), pPlugin(nullptr), RefCnt(1) {}
	PanelHandle(HANDLE panel, Plugin *plugin) : hPanel(panel), pPlugin(plugin), RefCnt(1) {}
};

typedef PanelHandle * PHPTR;
#define PHPTR_STOP ((PHPTR)(-2))

// параметры вызова макрофункций Plugin.Menu и т.п.
enum CALLPLUGINFLAGS
{
	CPT_MENU        = 0x01,
	CPT_CONFIGURE   = 0x02,
	CPT_CMDLINE     = 0x04,
	CPT_MASK        = 0x07,
	CPT_CHECKONLY   = 0x10000000,
};

enum MENUTYPE
{
	MTYPE_COMMANDSMENU,
	MTYPE_CONFIGSMENU,
	MTYPE_DISKSMENU,
};

class PluginManager
{
	private:

		Plugin **PluginsData;
		int PluginsCount;
		int OemPluginsCount;
		struct BackgroundTasks : std::map<std::wstring, unsigned int>, std::mutex {} BgTasks;
		std::unordered_map<DWORD, Plugin*> SysIdMap;

	public:

		struct CallPluginInfo
		{
			unsigned int CallFlags; // CALLPLUGINFLAGS
			int OpenFrom;
			union
			{
				int ItemNumber;
				GUID *ItemUuid;
				const wchar_t *Command;
			};
			// Используется в функции CallPluginItem для внутренних нужд
			Plugin *pPlugin;
			GUID FoundUuid;
			int FoundItemNumber;
		};

		BitFlags Flags;        // флаги манагера плагинов

		FileEditor *CurEditor;
		Viewer *CurViewer;     // 27.09.2000 SVS: Указатель на текущий Viewer

	private:

		void LoadIfCacheAbsent();
		void ReadUserBackgound(SaveScreen *SaveScr);

		void GetPluginHotKey(Plugin *pPlugin, int ItemNumber,  const GUID *Guid, MENUTYPE MenuType, FARString &strHotKey);
		std::string GetHotKeySettingName(Plugin *pPlugin, int ItemNumber, const GUID *Guid, MENUTYPE MenuType);

		bool TestPluginInfo(Plugin *Item,PluginInfo *Info);
		bool TestOpenPluginInfo(Plugin *Item,OpenPluginInfo *Info);
		bool CheckIfHotkeyPresent(MENUTYPE MenuType);

		Plugin* LoadPlugin(const FARString &strModuleName, bool LoadUncached);

		bool AddPlugin(Plugin *pPlugin);
		bool RemovePlugin(Plugin *pPlugin);

		void LoadPluginsFromCache();

		void SetFlags(DWORD NewFlags) { Flags.Set(NewFlags); }
		void SkipFlags(DWORD NewFlags) { Flags.Clear(NewFlags); }

	public:

		PluginManager();
		~PluginManager();

	public:

		bool CacheForget(const wchar_t *lpwszModuleName);
		bool LoadPluginExternal(const wchar_t *ModuleName, bool LoadToMem);

		int UnloadPlugin(Plugin *pPlugin, DWORD dwException, bool bRemove = false);
		int UnloadPluginExternal(const wchar_t *lpwszModuleName);

		Plugin* LoadPluginExternalV3(const wchar_t *ModuleName, bool LoadToMem);
		int UnloadPluginExternalV3(Plugin* pPlugin);
		size_t GetPluginInformation(Plugin *pPlugin, FarGetPluginInformation *pInfo, size_t BufferSize);

		void LoadPlugins();
		void ShowPluginInfo(Plugin *pPlugin);

		Plugin *GetPlugin(int PluginNumber);
		Plugin *FindPlugin(const wchar_t *lpwszModuleName);
		Plugin *FindPlugin(DWORD SysID);
		Plugin *FindPlugin(Plugin *pPlugin);

		int GetPluginsCount() { return PluginsCount; }
		int GetOemPluginsCount() { return OemPluginsCount; }

		BOOL IsPluginsLoaded() { return Flags.Check(PSIF_PLUGINSLOADDED); }

		BOOL CheckFlags(DWORD NewFlags) { return Flags.Check(NewFlags); }

		void Configure(int StartPos=0);
		void ConfigureCurrent(Plugin *pPlugin, int INum, const GUID *Guid);
		int CommandsMenu(int ModalType,int StartPos,const wchar_t *HistoryName=nullptr);
		bool GetDiskMenuItem(Plugin *pPlugin,int PluginItem, wchar_t& PluginHotkey, FARString &strPluginText, GUID &Guid);

		int UseFarCommand(PHPTR ph,int CommandType);
		void ReloadLanguage();
		void DiscardCache();
		int ProcessCommandLine(const wchar_t *Command,Panel *Target=nullptr);

		bool SetHotKeyDialog(const wchar_t *DlgPluginTitle, Plugin *pPlugin, int ItemNumber, const GUID *Guids, MENUTYPE MenuType);

		// $ .09.2000 SVS - Функция CallPlugin - найти плагин по ID и запустить OpenFrom = OPEN_*
		int CallPlugin(DWORD SysID,int OpenFrom, void *Data, void **Ret=nullptr);
		bool CallPluginItem(DWORD SysID, CallPluginInfo* Data);

//api functions

	public:
		Plugin *Analyse(const AnalyseData *pData);

		PHPTR OpenPlugin(Plugin *pPlugin,int OpenFrom,INT_PTR Item);
		PHPTR OpenFilePlugin(const wchar_t *Name, int OpMode, OPENFILEPLUGINTYPE Type, Plugin *pDesiredPlugin = nullptr);
		PHPTR OpenFindListPlugin(const PluginPanelItem *PanelItem,int ItemsNumber);
		FARString GetPluginModuleName(PHPTR ph);
		void ClosePanel(PHPTR ph); // decreases refcnt and actually closes plugin if refcnt reached zero
		void RetainPanel(PHPTR ph); // increments refcnt
		void GetOpenPluginInfo(PHPTR ph, OpenPluginInfo *Info);
		int GetFindData(PHPTR ph,PluginPanelItem **pPanelItem,int *pItemsNumber,int Silent);
		void FreeFindData(PHPTR ph,PluginPanelItem *PanelItem,int ItemsNumber);
		int GetVirtualFindData(PHPTR ph,PluginPanelItem **pPanelItem,int *pItemsNumber,const wchar_t *Path);
		void FreeVirtualFindData(PHPTR ph,PluginPanelItem *PanelItem,int ItemsNumber);
		int SetDirectory(PHPTR ph,const wchar_t *Dir,int OpMode);
		int GetFile(PHPTR ph,PluginPanelItem *PanelItem,const wchar_t *DestPath,FARString &strResultName,int OpMode);
		bool GetLinkTarget(PHPTR ph,PluginPanelItem *PanelItem,FARString &result,int OpMode);
		int GetFiles(PHPTR ph,PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode);
		int PutFiles(PHPTR ph,PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode);
		int DeleteFiles(PHPTR ph,PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
		int MakeDirectory(PHPTR ph,const wchar_t **Name,int OpMode);
		int ProcessHostFile(PHPTR ph,PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
		int ProcessKey(PHPTR ph,int Key,unsigned int ControlState);
		int ProcessEvent(PHPTR ph,int Event,void *Param);
		int Compare(PHPTR ph,const PluginPanelItem *Item1,const PluginPanelItem *Item2,unsigned int Mode);
		int ProcessEditorInput(INPUT_RECORD *Rec);
		int ProcessEditorEvent(int Event,void *Param);
		int ProcessViewerEvent(int Event,void *Param);
		int ProcessDialogEvent(int Event,void *Param);
		int ProcessConsoleInput(INPUT_RECORD *Rec);
		void GetCustomData(FileListItem *ListItem);
		bool MayExitFar();

		void BackroundTaskStarted(const wchar_t *Info);
		void BackroundTaskFinished(const wchar_t *Info);
		bool HasBackgroundTasks();
		std::map<std::wstring, unsigned int> BackgroundTasks();

		friend class Plugin;
};

const char *PluginsIni();
