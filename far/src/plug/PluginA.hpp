	#pragma once

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

#include "farplug-wide.h"
#include "plclass.hpp"
#include "farplug-mb.h"

typedef void (WINAPI *PLUGINCLOSEPLUGIN)(HANDLE hPanel);
typedef int  (WINAPI *PLUGINCOMPARE)(HANDLE hPanel,const oldfar::PluginPanelItem *Item1,const oldfar::PluginPanelItem *Item2,unsigned int Mode);
typedef int  (WINAPI *PLUGINCONFIGURE)(int ItemNumber);
typedef int  (WINAPI *PLUGINDELETEFILES)(HANDLE hPanel,oldfar::PluginPanelItem *PanelItem,int ItemsNumber,DWORD OpMode);
typedef void (WINAPI *PLUGINEXITFAR)();
typedef void (WINAPI *PLUGINFREEFINDDATA)(HANDLE hPanel,oldfar::PluginPanelItem *PanelItem,int ItemsNumber);
typedef void (WINAPI *PLUGINFREEVIRTUALFINDDATA)(HANDLE hPanel,oldfar::PluginPanelItem *PanelItem,int ItemsNumber);
typedef int  (WINAPI *PLUGINGETFILES)(HANDLE hPanel,oldfar::PluginPanelItem *PanelItem,int ItemsNumber,int Move,char *DestPath,DWORD OpMode);
typedef int  (WINAPI *PLUGINGETFINDDATA)(HANDLE hPanel,oldfar::PluginPanelItem **pPanelItem,int *pItemsNumber,DWORD OpMode);
typedef void (WINAPI *PLUGINGETGLOBALINFO)(GlobalInfo *gi);
typedef void (WINAPI *PLUGINGETOPENPLUGININFO)(HANDLE hPanel,oldfar::OpenPluginInfo *Info);
typedef void (WINAPI *PLUGINGETPLUGININFO)(oldfar::PluginInfo *Info);
typedef int  (WINAPI *PLUGINGETVIRTUALFINDDATA)(HANDLE hPanel,oldfar::PluginPanelItem **pPanelItem,int *pItemsNumber,const char *Path);
typedef int  (WINAPI *PLUGINMAKEDIRECTORY)(HANDLE hPanel,char *Name,DWORD OpMode);
typedef int  (WINAPI *PLUGINMAYEXITFAR)();
typedef int  (WINAPI *PLUGINMINFARVERSION)();
typedef HANDLE (WINAPI *PLUGINOPENFILEPLUGIN)(char *Name,const unsigned char *Data,int DataSize,DWORD OpMode);
typedef HANDLE (WINAPI *PLUGINOPENPLUGIN)(int OpenFrom,INT_PTR Item);
typedef int  (WINAPI *PLUGINPROCESSDIALOGEVENT)(int Event,void *Param);
typedef int  (WINAPI *PLUGINPROCESSEDITOREVENT)(int Event,void *Param);
typedef int  (WINAPI *PLUGINPROCESSEDITORINPUT)(const INPUT_RECORD *Rec);
typedef int  (WINAPI *PLUGINPROCESSEVENT)(HANDLE hPanel,int Event,void *Param);
typedef int  (WINAPI *PLUGINPROCESSHOSTFILE)(HANDLE hPanel,oldfar::PluginPanelItem *PanelItem,int ItemsNumber,DWORD OpMode);
typedef int  (WINAPI *PLUGINPROCESSKEY)(HANDLE hPanel,int Key,unsigned int ControlState);
typedef int  (WINAPI *PLUGINPROCESSVIEWEREVENT)(int Event,void *Param);
typedef int  (WINAPI *PLUGINPUTFILES)(HANDLE hPanel,oldfar::PluginPanelItem *PanelItem,int ItemsNumber,int Move,DWORD OpMode);
typedef int  (WINAPI *PLUGINSETDIRECTORY)(HANDLE hPanel,const char *Dir,DWORD OpMode);
typedef int  (WINAPI *PLUGINSETFINDLIST)(HANDLE hPanel,const oldfar::PluginPanelItem *PanelItem,int ItemsNumber);
typedef void (WINAPI *PLUGINSETSTARTUPINFO)(const oldfar::PluginStartupInfo *Info);


class PluginA: public Plugin
{
private:
	PluginInfo PI;
	OpenPluginInfo OPI;

	oldfar::PluginPanelItem  *pFDPanelItemA;
	oldfar::PluginPanelItem  *pVFDPanelItemA;

	PLUGINCLOSEPLUGIN           pClosePlugin;
	PLUGINCOMPARE               pCompare;
	PLUGINCONFIGURE             pConfigure;
	PLUGINDELETEFILES           pDeleteFiles;
	PLUGINEXITFAR               pExitFAR;
	PLUGINFREEFINDDATA          pFreeFindData;
	PLUGINFREEVIRTUALFINDDATA   pFreeVirtualFindData;
	PLUGINGETFILES              pGetFiles;
	PLUGINGETFINDDATA           pGetFindData;
	PLUGINGETOPENPLUGININFO     pGetOpenPluginInfo;
	PLUGINGETPLUGININFO         pGetPluginInfo;
	PLUGINGETVIRTUALFINDDATA    pGetVirtualFindData;
	PLUGINMAKEDIRECTORY         pMakeDirectory;
	PLUGINMAYEXITFAR            pMayExitFAR;
	PLUGINMINFARVERSION         pMinFarVersion;
	PLUGINOPENFILEPLUGIN        pOpenFilePlugin;
	PLUGINOPENPLUGIN            pOpenPlugin;
	PLUGINPROCESSDIALOGEVENT    pProcessDialogEvent;
	PLUGINPROCESSEDITOREVENT    pProcessEditorEvent;
	PLUGINPROCESSEDITORINPUT    pProcessEditorInput;
	PLUGINPROCESSEVENT          pProcessEvent;
	PLUGINPROCESSHOSTFILE       pProcessHostFile;
	PLUGINPROCESSKEY            pProcessKey;
	PLUGINPROCESSVIEWEREVENT    pProcessViewerEvent;
	PLUGINPUTFILES              pPutFiles;
	PLUGINSETDIRECTORY          pSetDirectory;
	PLUGINSETFINDLIST           pSetFindList;
	PLUGINSETSTARTUPINFO        pSetStartupInfo;

public:

	PluginA(PluginManager *owner,
			const FARString &strModuleName,
			const std::string &settingsName,
			const std::string &moduleID);
	~PluginA() override;

	const char* GetMsgA(int nID) { return Lang.GetMsgMB(nID); }

	bool  CheckWorkFlags(DWORD flags) override { return WorkFlags.Check(flags); }
	void  CloseLang() override { Lang.Close(); }
	const FARString& GetModuleName() const override { return m_strModuleName; }
	const char* GetSettingsName() override { return m_strSettingsName.c_str(); }
	DWORD GetWorkFlags() override { return WorkFlags.Flags; }
	bool  InitLang(const wchar_t *Path) override { return Lang.Init(Path,false); }
	bool  IsOemPlugin() override { return true; }
	bool  IsPanelPlugin() override;
	bool  Load() override;
	bool  LoadFromCache() override;
	bool  SaveToCache() override;
	int   Unload(bool bExitFAR) override;

	bool HasAnalyse()               override { return false; }
	bool HasCloseAnalyse()          override { return false; }
	bool HasClosePlugin()           override { return pClosePlugin != nullptr; }
	bool HasCompare()               override { return pCompare != nullptr; }
	bool HasConfigure()             override { return pConfigure != nullptr; }
	bool HasConfigureV3()           override { return false; }
	bool HasDeleteFiles()           override { return pDeleteFiles != nullptr; }
	bool HasExitFAR()               override { return pExitFAR != nullptr; }
	bool HasFreeCustomData()        override { return false; }
	bool HasFreeFindData()          override { return pFreeFindData != nullptr; }
	bool HasFreeVirtualFindData()   override { return pFreeVirtualFindData != nullptr; }
	bool HasGetCustomData()         override { return false; }
	bool HasGetFiles()              override { return pGetFiles != nullptr; }
	bool HasGetFindData()           override { return pGetFindData != nullptr; }
	bool HasGetLinkTarget()         override { return false; }
	bool HasGetOpenPluginInfo()     override { return pGetOpenPluginInfo != nullptr; }
	bool HasGetPluginInfo()         override { return pGetPluginInfo != nullptr; }
	bool HasGetVirtualFindData()    override { return pGetVirtualFindData != nullptr; }
	bool HasMakeDirectory()         override { return pMakeDirectory != nullptr; }
	bool HasMayExitFAR()            override { return pMayExitFAR != nullptr; }
	bool HasMinFarVersion()         override { return pMinFarVersion != nullptr; }
	bool HasOpenFilePlugin()        override { return pOpenFilePlugin != nullptr; }
	bool HasOpenPlugin()            override { return pOpenPlugin != nullptr; }
	bool HasProcessConsoleInput()   override { return false; }
	bool HasProcessDialogEvent()    override { return pProcessDialogEvent != nullptr; }
	bool HasProcessEditorEvent()    override { return pProcessEditorEvent != nullptr; }
	bool HasProcessEditorEventV3()  override { return false; } //TODO
	bool HasProcessEditorInput()    override { return pProcessEditorInput != nullptr; }
	bool HasProcessEvent()          override { return pProcessEvent != nullptr; }
	bool HasProcessHostFile()       override { return pProcessHostFile != nullptr; }
	bool HasProcessKey()            override { return pProcessKey != nullptr; }
	bool HasProcessSynchroEvent()   override { return false; }
	bool HasProcessViewerEvent()    override { return pProcessViewerEvent != nullptr; }
	bool HasPutFiles()              override { return pPutFiles != nullptr; }
	bool HasSetDirectory()          override { return pSetDirectory != nullptr; }
	bool HasSetFindList()           override { return pSetFindList != nullptr; }
	bool HasSetStartupInfo()        override { return pSetStartupInfo != nullptr; }

public:
	HANDLE Analyse(const AnalyseInfo *Info) override { return INVALID_HANDLE_VALUE; }
	bool   CheckMinFarVersion() override;
	void   CloseAnalyse(const CloseAnalyseInfo *Info) override {}
	void   ClosePanel(HANDLE hPanel) override;
	int    Compare(HANDLE hPanel, const PluginPanelItem *Item1, const PluginPanelItem *Item2, DWORD Mode) override;
	int    Configure(int MenuItem) override;
	int    ConfigureV3(const ConfigureInfo *Info) override { return 0; }
	int    DeleteFiles(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber, DWORD OpMode) override;
	void   ExitFAR() override;
	void   FreeCustomData(wchar_t *CustomData) override {}
	void   FreeFindData(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber) override;
	void   FreeVirtualFindData(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber) override;
	int    GetCustomData(const wchar_t *FilePath, wchar_t **CustomData) override { return 0; }
	int    GetFiles(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber, bool Move, const wchar_t **DestPath, DWORD OpMode) override;
	int    GetFindData(HANDLE hPanel, PluginPanelItem **pPanelItem, int *pItemsNumber, DWORD OpMode) override;
	bool   GetLinkTarget(HANDLE hPanel, PluginPanelItem *PanelItem, FARString &result, DWORD OpMode) override;
	void   GetOpenPluginInfo(HANDLE hPanel, OpenPluginInfo *Info) override;
	bool   GetPluginInfo(PluginInfo *pi) override;
	int    GetVirtualFindData(HANDLE hPanel, PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path) override;
	int    MakeDirectory(HANDLE hPanel, const wchar_t **Name, DWORD OpMode) override;
	bool   MayExitFAR() override;
	HANDLE OpenFilePlugin(const wchar_t *Name, const unsigned char *Data, int DataSize, DWORD OpMode) override;
	HANDLE OpenPlugin(int OpenFrom, const void *Item) override;
	int    ProcessConsoleInput(INPUT_RECORD *D) override { return 0; }
	int    ProcessDialogEvent(int Event, void *Param) override;
	int    ProcessEditorEvent(int Event, void *Param) override;
	int    ProcessEditorEventV3(const ProcessEditorEventInfo *Info) override { return 0; }; //TODO
	int    ProcessEditorInput(const INPUT_RECORD *D) override;
	int    ProcessEvent(HANDLE hPanel, int Event, void *Param) override;
	int    ProcessHostFile(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber, DWORD OpMode) override;
	int    ProcessKey(HANDLE hPanel, int Key, unsigned int dwControlState) override;
	int    ProcessSynchroEvent(int Event, void *Param) override { return 0; }
	int    ProcessViewerEvent(int Event, void *Param) override;
	int    PutFiles(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber, bool Move, DWORD OpMode) override;
	int    SetDirectory(HANDLE hPanel, const wchar_t *Dir, DWORD OpMode) override;
	int    SetFindList(HANDLE hPanel, const PluginPanelItem *PanelItem, int ItemsNumber) override;
	bool   SetStartupInfo() override;

private:
	void ClearExports();
	void FreePluginInfo();
	void ConvertPluginInfo(oldfar::PluginInfo &Src, PluginInfo *Dest);
	void FreeOpenPluginInfo();
	void ConvertOpenPluginInfo(oldfar::OpenPluginInfo &Src, OpenPluginInfo *Dest);
};
