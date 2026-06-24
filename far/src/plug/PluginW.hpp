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

#include <farplug-wide.h>
#include "plclass.hpp"
#include "FARString.hpp"


typedef HANDLE (WINAPI *PLUGINANALYSEW)(const AnalyseInfo *Info);
typedef void (WINAPI *PLUGINCLOSEANALYSEW)(const CloseAnalyseInfo *Info);
typedef void (WINAPI *PLUGINCLOSEPLUGINW)(HANDLE hPanel);
typedef int  (WINAPI *PLUGINCOMPAREW)(HANDLE hPanel,const PluginPanelItem *Item1,const PluginPanelItem *Item2,unsigned int Mode);
typedef int  (WINAPI *PLUGINCONFIGUREV3W)(const ConfigureInfo *Info);
typedef int  (WINAPI *PLUGINCONFIGUREW)(int ItemNumber);
typedef int  (WINAPI *PLUGINDELETEFILESW)(HANDLE hPanel,PluginPanelItem *PanelItem,int ItemsNumber,DWORD OpMode);
typedef void (WINAPI *PLUGINEXITFARW)();
typedef void (WINAPI *PLUGINFREECUSTOMDATAW)(wchar_t *CustomData);
typedef void (WINAPI *PLUGINFREEFINDDATAW)(HANDLE hPanel,PluginPanelItem *PanelItem,int ItemsNumber);
typedef void (WINAPI *PLUGINFREEVIRTUALFINDDATAW)(HANDLE hPanel,PluginPanelItem *PanelItem,int ItemsNumber);
typedef int  (WINAPI *PLUGINGETCUSTOMDATAW)(const wchar_t *FilePath, wchar_t **CustomData);
typedef int  (WINAPI *PLUGINGETFILESW)(HANDLE hPanel,PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,DWORD OpMode);
typedef int  (WINAPI *PLUGINGETFINDDATAW)(HANDLE hPanel,PluginPanelItem **pPanelItem,int *pItemsNumber,DWORD OpMode);
typedef int  (WINAPI *PLUGINGETLINKTARGETW)(HANDLE hPanel, PluginPanelItem *PanelItem, wchar_t *Target, size_t TargetSize, DWORD OpMode);
typedef void (WINAPI *PLUGINGETOPENPLUGININFOW)(HANDLE hPanel,OpenPluginInfo *Info);
typedef void (WINAPI *PLUGINGETPLUGININFOW)(PluginInfo *Info);
typedef int  (WINAPI *PLUGINGETVIRTUALFINDDATAW)(HANDLE hPanel,PluginPanelItem **pPanelItem,int *pItemsNumber,const wchar_t *Path);
typedef int  (WINAPI *PLUGINMAKEDIRECTORYW)(HANDLE hPanel,const wchar_t **Name,DWORD OpMode);
typedef int  (WINAPI *PLUGINMAYEXITFARW)();
typedef int  (WINAPI *PLUGINMINFARVERSIONW)();
typedef HANDLE (WINAPI *PLUGINOPENFILEPLUGINW)(const wchar_t *Name,const unsigned char *Data,int DataSize,DWORD OpMode);
typedef HANDLE (WINAPI *PLUGINOPENPLUGINW)(int OpenFrom,INT_PTR Item);
typedef int  (WINAPI *PLUGINPROCESSCONSOLEINPUTW)(INPUT_RECORD *Rec);
typedef int  (WINAPI *PLUGINPROCESSDIALOGEVENTW)(int Event,void *Param);
typedef int  (WINAPI *PLUGINPROCESSEDITOREVENTW)(int Event,void *Param);
typedef int  (WINAPI *PLUGINPROCESSEDITOREVENTV3W)(const ProcessEditorEventInfo *Info);
typedef int  (WINAPI *PLUGINPROCESSEDITORINPUTW)(const INPUT_RECORD *Rec);
typedef int  (WINAPI *PLUGINPROCESSEVENTW)(HANDLE hPanel,int Event,void *Param);
typedef int  (WINAPI *PLUGINPROCESSHOSTFILEW)(HANDLE hPanel,PluginPanelItem *PanelItem,int ItemsNumber,DWORD OpMode);
typedef int  (WINAPI *PLUGINPROCESSKEYW)(HANDLE hPanel,int Key,unsigned int ControlState);
typedef int  (WINAPI *PLUGINPROCESSSYNCHROEVENTW)(int Event,void *Param);
typedef int  (WINAPI *PLUGINPROCESSVIEWEREVENTW)(int Event,void *Param); //* $ 27.09.2000 SVS -  События во вьювере
typedef int  (WINAPI *PLUGINPUTFILESW)(HANDLE hPanel,PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t *SrcPath,DWORD OpMode);
typedef int  (WINAPI *PLUGINSETDIRECTORYW)(HANDLE hPanel,const wchar_t *Dir,DWORD OpMode);
typedef int  (WINAPI *PLUGINSETFINDLISTW)(HANDLE hPanel,const PluginPanelItem *PanelItem,int ItemsNumber);
typedef void (WINAPI *PLUGINSETSTARTUPINFOW)(const PluginStartupInfo *Info);


class PluginW: public Plugin
{
private:
	PLUGINANALYSEW               pAnalyseW;
	PLUGINCLOSEANALYSEW          pCloseAnalyseW;
	PLUGINCLOSEPLUGINW           pClosePluginW;
	PLUGINCOMPAREW               pCompareW;
	PLUGINCONFIGUREV3W           pConfigureV3W;
	PLUGINCONFIGUREW             pConfigureW;
	PLUGINDELETEFILESW           pDeleteFilesW;
	PLUGINEXITFARW               pExitFARW;
	PLUGINFREECUSTOMDATAW        pFreeCustomDataW;
	PLUGINFREEFINDDATAW          pFreeFindDataW;
	PLUGINFREEVIRTUALFINDDATAW   pFreeVirtualFindDataW;
	PLUGINGETCUSTOMDATAW         pGetCustomDataW;
	PLUGINGETFILESW              pGetFilesW;
	PLUGINGETFINDDATAW           pGetFindDataW;
	PLUGINGETLINKTARGETW         pGetLinkTargetW;
	PLUGINGETOPENPLUGININFOW     pGetOpenPluginInfoW;
	PLUGINGETPLUGININFOW         pGetPluginInfoW;
	PLUGINGETVIRTUALFINDDATAW    pGetVirtualFindDataW;
	PLUGINMAKEDIRECTORYW         pMakeDirectoryW;
	PLUGINMAYEXITFARW            pMayExitFARW;
	PLUGINMINFARVERSIONW         pMinFarVersionW;
	PLUGINOPENFILEPLUGINW        pOpenFilePluginW;
	PLUGINOPENPLUGINW            pOpenPluginW;
	PLUGINPROCESSCONSOLEINPUTW   pProcessConsoleInputW;
	PLUGINPROCESSDIALOGEVENTW    pProcessDialogEventW;
	PLUGINPROCESSEDITOREVENTW    pProcessEditorEventW;
	PLUGINPROCESSEDITOREVENTV3W  pProcessEditorEventV3W;
	PLUGINPROCESSEDITORINPUTW    pProcessEditorInputW;
	PLUGINPROCESSEVENTW          pProcessEventW;
	PLUGINPROCESSHOSTFILEW       pProcessHostFileW;
	PLUGINPROCESSKEYW            pProcessKeyW;
	PLUGINPROCESSSYNCHROEVENTW   pProcessSynchroEventW;
	PLUGINPROCESSVIEWEREVENTW    pProcessViewerEventW;
	PLUGINPUTFILESW              pPutFilesW;
	PLUGINSETDIRECTORYW          pSetDirectoryW;
	PLUGINSETFINDLISTW           pSetFindListW;
	PLUGINSETSTARTUPINFOW        pSetStartupInfoW;

public:

	PluginW(PluginManager *owner,
			const FARString &strModuleName,
			const std::string &settingsName,
			const std::string &moduleID);
	~PluginW() override;

	const wchar_t* GetMsg(int nID) { return Lang.GetMsgWide(nID); }

	bool  CheckWorkFlags(DWORD flags) override { return WorkFlags.Check(flags); }
	void  CloseLang() override { Lang.Close(); }
	const FARString& GetModuleName() const override { return m_strModuleName; }
	const char* GetSettingsName() override { return m_strSettingsName.c_str(); }
	DWORD GetWorkFlags() override { return WorkFlags.Flags; }
	bool  InitLang(const wchar_t *Path) override { return Lang.Init(Path,true); }
	bool  IsOemPlugin() override {return false;}
	bool  IsPanelPlugin() override;
	bool  Load() override;
	bool  LoadFromCache() override;
	bool  SaveToCache() override;
	int   Unload(bool bExitFAR) override;

	bool HasAnalyse()               override { return pAnalyseW != nullptr; }
	bool HasCloseAnalyse()          override { return pCloseAnalyseW != nullptr; }
	bool HasClosePlugin()           override { return pClosePluginW != nullptr; }
	bool HasCompare()               override { return pCompareW != nullptr; }
	bool HasConfigure()             override { return pConfigureW != nullptr; }
	bool HasConfigureV3()           override { return pConfigureV3W != nullptr; }
	bool HasDeleteFiles()           override { return pDeleteFilesW != nullptr; }
	bool HasExitFAR()               override { return pExitFARW != nullptr; }
	bool HasFreeCustomData()        override { return pFreeCustomDataW != nullptr; }
	bool HasFreeFindData()          override { return pFreeFindDataW != nullptr; }
	bool HasFreeVirtualFindData()   override { return pFreeVirtualFindDataW != nullptr; }
	bool HasGetCustomData()         override { return pGetCustomDataW != nullptr; }
	bool HasGetFiles()              override { return pGetFilesW != nullptr; }
	bool HasGetFindData()           override { return pGetFindDataW != nullptr; }
	bool HasGetLinkTarget()         override { return pGetLinkTargetW != nullptr; }
	bool HasGetOpenPluginInfo()     override { return pGetOpenPluginInfoW != nullptr; }
	bool HasGetPluginInfo()         override { return pGetPluginInfoW != nullptr; }
	bool HasGetVirtualFindData()    override { return pGetVirtualFindDataW != nullptr; }
	bool HasMakeDirectory()         override { return pMakeDirectoryW != nullptr; }
	bool HasMayExitFAR()            override { return pMayExitFARW != nullptr; }
	bool HasMinFarVersion()         override { return pMinFarVersionW != nullptr; }
	bool HasOpenFilePlugin()        override { return pOpenFilePluginW != nullptr; }
	bool HasOpenPlugin()            override { return pOpenPluginW != nullptr; }
	bool HasProcessConsoleInput()   override { return pProcessConsoleInputW != nullptr; }
	bool HasProcessDialogEvent()    override { return pProcessDialogEventW != nullptr; }
	bool HasProcessEditorEvent()    override { return pProcessEditorEventW != nullptr; }
	bool HasProcessEditorEventV3()  override { return pProcessEditorEventV3W != nullptr; }
	bool HasProcessEditorInput()    override { return pProcessEditorInputW != nullptr; }
	bool HasProcessEvent()          override { return pProcessEventW != nullptr; }
	bool HasProcessHostFile()       override { return pProcessHostFileW != nullptr; }
	bool HasProcessKey()            override { return pProcessKeyW != nullptr; }
	bool HasProcessSynchroEvent()   override { return pProcessSynchroEventW != nullptr; }
	bool HasProcessViewerEvent()    override { return pProcessViewerEventW != nullptr; }
	bool HasPutFiles()              override { return pPutFilesW != nullptr; }
	bool HasSetDirectory()          override { return pSetDirectoryW != nullptr; }
	bool HasSetFindList()           override { return pSetFindListW != nullptr; }
	bool HasSetStartupInfo()        override { return pSetStartupInfoW != nullptr; }

public:
	HANDLE Analyse(const AnalyseInfo *Info) override;
	bool   CheckMinFarVersion() override;
	void   CloseAnalyse(const CloseAnalyseInfo *Info) override;
	void   ClosePanel(HANDLE hPanel) override;
	int    Compare(HANDLE hPanel, const PluginPanelItem *Item1, const PluginPanelItem *Item2, DWORD Mode) override;
	int    Configure(int MenuItem) override;
	int    ConfigureV3(const ConfigureInfo *Info) override;
	int    DeleteFiles(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber, DWORD OpMode) override;
	void   ExitFAR() override;
	void   FreeCustomData(wchar_t *CustomData) override;
	void   FreeFindData(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber) override;
	void   FreeVirtualFindData(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber) override;
	int    GetCustomData(const wchar_t *FilePath, wchar_t **CustomData) override;
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
	int    ProcessConsoleInput(INPUT_RECORD *D) override;
	int    ProcessDialogEvent(int Event, void *Param) override;
	int    ProcessEditorEvent(int Event, void *Param) override;
	int    ProcessEditorEventV3(const ProcessEditorEventInfo *Info) override;
	int    ProcessEditorInput(const INPUT_RECORD *D) override;
	int    ProcessEvent(HANDLE hPanel, int Event, void *Param) override;
	int    ProcessHostFile(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber, DWORD OpMode) override;
	int    ProcessKey(HANDLE hPanel, int Key, unsigned int dwControlState) override;
	int    ProcessSynchroEvent(int Event, void *Param) override;
	int    ProcessViewerEvent(int Event, void *Param) override;
	int    PutFiles(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber, bool Move, DWORD OpMode) override;
	int    SetDirectory(HANDLE hPanel, const wchar_t *Dir, DWORD OpMode) override;
	int    SetFindList(HANDLE hPanel, const PluginPanelItem *PanelItem, int ItemsNumber) override;
	bool   SetStartupInfo() override;

private:
	void ClearExports();
};
