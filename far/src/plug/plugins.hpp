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
extern const char *FmtDiskMenuGuidD;
extern const char *FmtPluginMenuGuidD;
extern const char *FmtPluginConfigGuidD;


class SaveScreen;
class FileEditor;
class Viewer;
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

// флаги для поля PluginManager.Flags
enum PLUGINSETFLAGS
{
	PSIF_ENTERTOOPENPLUGIN        = 0x00000001, // ввалились в плагин OpenPlugin
	PSIF_DIALOG                   = 0x00000002, // была бадяга с диалогом
	PSIF_PLUGINSLOADED            = 0x80000000, // плагины загружены
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
	CPT_MENU,
	CPT_CONFIGURE,
	CPT_CMDLINE,
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
		struct BackgroundTasks : std::map<std::wstring, unsigned int>, std::mutex {} BgTasks;
		std::unordered_map<DWORD, Plugin*> SysIdMap;
		BitFlags m_Flags;      // флаги манагера плагинов

	public:

		struct CallPluginInfo
		{
			CALLPLUGINFLAGS CallFlags;
			int OpenFrom;
			union
			{
				intptr_t ItemNumber;
				GUID *ItemUuid;
				const wchar_t *Command;
			};
			// Используется в функции CallPluginItem для внутренних нужд
			GUID FoundUuid;
			intptr_t FoundItemNumber;
		};

		FileEditor *CurEditor;
		Viewer *CurViewer;     // 27.09.2000 SVS: Указатель на текущий Viewer

	private:

		void LoadIfCacheAbsent();
		void ReadUserBackground(SaveScreen *SaveScr);

		Plugin* LoadPlugin(const FARString &strModuleName, bool LoadUncached);

		bool AddPlugin(Plugin *pPlugin);
		bool RemovePlugin(Plugin *pPlugin);

		void LoadPluginsFromCache();

	public:

		PluginManager();
		~PluginManager();

	public:

		bool CacheForget(const wchar_t *lpwszModuleName);
		Plugin* LoadPluginExternal(const wchar_t *ModuleName, bool LoadToMem);

		int UnloadPlugin(Plugin *pPlugin, DWORD dwException, bool bRemove = false);
		int UnloadPluginExternal(const wchar_t *lpwszModuleName);
		int UnloadPluginExternal(Plugin* pPlugin);

		size_t GetPluginInformation(Plugin *pPlugin, FarGetPluginInformation *pInfo, size_t BufferSize);

		void LoadPlugins();
		void ShowPluginInfo(Plugin *pPlugin, int nItem, const GUID &Guid);

		Plugin *GetPlugin(int PluginNumber);
		Plugin *FindPlugin(const wchar_t *lpwszModuleName);
		Plugin *FindPlugin(DWORD SysID);
		bool FindPlugin(Plugin *pPlugin);

		int GetPluginsCount() { return PluginsCount; }

		bool IsPluginsLoaded() { return m_Flags.Check(PSIF_PLUGINSLOADED); }

		void Configure(int StartPos=0);
		void ConfigureCurrent(Plugin *pPlugin, int INum, const GUID *Guid);
		int CommandsMenu(int ModalType, int StartPos, const wchar_t *HistoryName=nullptr);
		bool GetDiskMenuItem(Plugin *pPlugin, int PluginItem, wchar_t& PluginHotkey, FARString &strPluginText, GUID &Guid);

		bool UseFarCommand(PHPTR ph, int CommandType);
		void ReloadLanguage();
		void DiscardCache();
		bool ProcessCommandLine(const wchar_t *Command, Panel *Target=nullptr);

		bool SetHotKeyDialog(const wchar_t *DlgPluginTitle, Plugin *pPlugin, int ItemNumber, const GUID *Guids, MENUTYPE MenuType);

		// $ .09.2000 SVS - Функция CallPlugin - найти плагин по ID и запустить OpenFrom = OPEN_*
		bool CallPlugin(DWORD SysID, int OpenFrom, void *Data);
		bool CallPluginItem(Plugin *pPlugin, CallPluginInfo* Data, bool CheckOnly);
		bool CallMacroPlugin(OpenMacroPluginInfo *Info);
		void* CallPluginFromMacro(DWORD SysID, OpenMacroInfo *Info);

//api functions

	public:
		void  ClosePanel(PHPTR ph); // decreases refcnt and actually closes plugin if refcnt reached zero
		int   Compare(PHPTR ph, const PluginPanelItem *Item1, const PluginPanelItem *Item2, unsigned int Mode);
		int   DeleteFiles(PHPTR ph, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
		void  FreeFindData(PHPTR ph, PluginPanelItem *PanelItem, int ItemsNumber);
		void  FreeVirtualFindData(PHPTR ph, PluginPanelItem *PanelItem, int ItemsNumber);
		void  GetCustomData(FileListItem *ListItem);
		bool  GetFile(PHPTR ph, PluginPanelItem *PanelItem, const wchar_t *DestPath, FARString &strResultName, int OpMode);
		int   GetFiles(PHPTR ph, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
		int   GetFindData(PHPTR ph, PluginPanelItem **pPanelItem, int *pItemsNumber, int Silent);
		bool  GetLinkTarget(PHPTR ph, PluginPanelItem *PanelItem, FARString &result, int OpMode);
		void  GetOpenPluginInfo(PHPTR ph, OpenPluginInfo *Info);
		FARString GetPluginModuleName(PHPTR ph);
		int   GetVirtualFindData(PHPTR ph, PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path);
		int   MakeDirectory(PHPTR ph, const wchar_t **Name, int OpMode);
		bool  MayExitFar();
		PHPTR OpenFilePlugin(const wchar_t *Name, int OpMode, OPENFILEPLUGINTYPE Type, Plugin *pDesiredPlugin = nullptr);
		PHPTR OpenFindListPlugin(const PluginPanelItem *PanelItem, int ItemsNumber);
		PHPTR OpenPlugin(Plugin *pPlugin, int OpenFrom, const void *Item);
		int   ProcessConsoleInput(INPUT_RECORD *Rec);
		int   ProcessDialogEvent(int Event, void *Param);
		int   ProcessEditorEvent(int Event, void *Param);
		int   ProcessEditorInput(INPUT_RECORD *Rec);
		int   ProcessEvent(PHPTR ph, int Event, void *Param);
		int   ProcessHostFile(PHPTR ph, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
		int   ProcessKey(PHPTR ph, int Key, unsigned int ControlState);
		int   ProcessSynchroEvent(int Event, void* Param);
		int   ProcessViewerEvent(int Event, void *Param);
		int   PutFiles(PHPTR ph, PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode);
		void  RetainPanel(PHPTR ph); // increments refcnt
		int   SetDirectory(PHPTR ph, const wchar_t *Dir, int OpMode);

		void BackgroundTaskStarted(const wchar_t *Info);
		void BackgroundTaskFinished(const wchar_t *Info);
		bool HasBackgroundTasks();
		std::map<std::wstring, unsigned int> BackgroundTasks();
};

const char *PluginsIni();
