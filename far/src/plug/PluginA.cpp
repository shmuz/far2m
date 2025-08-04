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

#include "headers.hpp"
#include <list>

#include "plugins.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "codepage.hpp"
#include "scantree.hpp"
#include "chgprior.hpp"
#include "constitle.hpp"
#include "cmdline.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "fileedit.hpp"
#include "RefreshFrameManager.hpp"
#include "InterThreadCall.hpp"
#include "plclass.hpp"
#include "PluginA.hpp"
#include "plugapi.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "clipboard.hpp"
#include "xlat.hpp"
#include "fileowner.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "processname.hpp"
#include "mix.hpp"
#include "execute.hpp"
#include "flink.hpp"
#include "ConfigRW.hpp"
#include "wrap.cpp"
#include <KeyFileHelper.h>

static const char *szCache_Preload = "Preload";
static const char *szCache_Preopen = "Preopen";
static const char *szCache_SysID = "SysID";

static const char *szCache_Author = "Author";
static const char *szCache_Description = "Description";
static const char *szCache_Title = "Title";
static const char *szCache_Version = "Version";

static const char szCache_Configure[] = "Configure";
static const char szCache_GetFiles[] = "GetFiles";
static const char szCache_OpenFilePlugin[] = "OpenFilePlugin";
static const char szCache_OpenPlugin[] = "OpenPlugin";
static const char szCache_ProcessDialogEvent[] = "ProcessDialogEvent";
static const char szCache_ProcessEditorEvent[] = "ProcessEditorEvent";
static const char szCache_ProcessEditorInput[] = "ProcessEditorInput";
static const char szCache_ProcessHostFile[] = "ProcessHostFile";
static const char szCache_ProcessViewerEvent[] = "ProcessViewerEvent";
static const char szCache_SetFindList[] = "SetFindList";

static const char NFMP_ClosePlugin[] = "ClosePlugin";
static const char NFMP_Compare[] = "Compare";
static const char NFMP_Configure[] = "Configure";
static const char NFMP_DeleteFiles[] = "DeleteFiles";
static const char NFMP_ExitFAR[] = "ExitFAR";
static const char NFMP_FreeFindData[] = "FreeFindData";
static const char NFMP_FreeVirtualFindData[] = "FreeVirtualFindData";
static const char NFMP_GetFiles[] = "GetFiles";
static const char NFMP_GetFindData[] = "GetFindData";
static const char NFMP_GetMinFarVersion[] = "GetMinFarVersion";
static const char NFMP_GetOpenPluginInfo[] = "GetOpenPluginInfo";
static const char NFMP_GetPluginInfo[] = "GetPluginInfo";
static const char NFMP_GetVirtualFindData[] = "GetVirtualFindData";
static const char NFMP_MakeDirectory[] = "MakeDirectory";
static const char NFMP_MayExitFAR[] = "MayExitFAR";
static const char NFMP_OpenFilePlugin[] = "OpenFilePlugin";
static const char NFMP_OpenPlugin[] = "OpenPlugin";
static const char NFMP_ProcessDialogEvent[] = "ProcessDialogEvent";
static const char NFMP_ProcessEditorEvent[] = "ProcessEditorEvent";
static const char NFMP_ProcessEditorInput[] = "ProcessEditorInput";
static const char NFMP_ProcessEvent[] = "ProcessEvent";
static const char NFMP_ProcessHostFile[] = "ProcessHostFile";
static const char NFMP_ProcessKey[] = "ProcessKey";
static const char NFMP_ProcessViewerEvent[] = "ProcessViewerEvent";
static const char NFMP_PutFiles[] = "PutFiles";
static const char NFMP_SetDirectory[] = "SetDirectory";
static const char NFMP_SetFindList[] = "SetFindList";
static const char NFMP_SetStartupInfo[] = "SetStartupInfo";


static void CheckScreenLock()
{
	if (ScrBuf.GetLockCount() > 0 && !CtrlObject->Macro.PeekKey())
	{
		ScrBuf.SetLockCount(0);
		ScrBuf.Flush();
	}
}



PluginA::PluginA(PluginManager *owner, const FARString &strModuleName,
					const std::string &settingsName, const std::string &moduleID)
	:
	Plugin(owner, strModuleName, settingsName, moduleID),
	pFDPanelItemA(nullptr),
	pVFDPanelItemA(nullptr)
	//more initialization here!!!
{
	ClearExports();
	memset(&PI,0,sizeof(PI));
	memset(&OPI,0,sizeof(OPI));
}

PluginA::~PluginA()
{
	FreePluginInfo();
	FreeOpenPluginInfo();
}


bool PluginA::LoadFromCache()
{
	KeyFileReadSection kfh(PluginsIni(), GetSettingsName());

	if (!kfh.SectionLoaded())
		return false;

	//PF_PRELOAD plugin, skip cache
	if (kfh.GetInt(szCache_Preload) != 0)
		return Load();

	//одинаковые ли бинарники?
	if (kfh.GetString("ID") != m_strModuleID)
		return false;

	SysID = kfh.GetUInt(szCache_SysID, 0);
	if (SysID && CtrlObject->Plugins.FindPlugin(SysID))
	{
		SysID = 0;
		return false;
	}

	if (kfh.GetBytes((unsigned char*)&m_PlugVersion, sizeof(m_PlugVersion), szCache_Version) != sizeof(m_PlugVersion))
		memset(&m_PlugVersion, 0, sizeof(m_PlugVersion));

	strAuthor = kfh.GetString(szCache_Author);
	strDescription = kfh.GetString(szCache_Description);
	strTitle = kfh.GetString(szCache_Title);

	pConfigure = (PLUGINCONFIGURE)(INT_PTR)kfh.GetUInt(szCache_Configure, 0);
	pGetFiles = (PLUGINGETFILES)(INT_PTR)kfh.GetUInt(szCache_GetFiles, 0);
	pOpenFilePlugin = (PLUGINOPENFILEPLUGIN)(INT_PTR)kfh.GetUInt(szCache_OpenFilePlugin, 0);
	pOpenPlugin = (PLUGINOPENPLUGIN)(INT_PTR)kfh.GetUInt(szCache_OpenPlugin, 0);
	pProcessDialogEvent = (PLUGINPROCESSDIALOGEVENT)(INT_PTR)kfh.GetUInt(szCache_ProcessDialogEvent, 0);
	pProcessEditorEvent = (PLUGINPROCESSEDITOREVENT)(INT_PTR)kfh.GetUInt(szCache_ProcessEditorEvent, 0);
	pProcessEditorInput = (PLUGINPROCESSEDITORINPUT)(INT_PTR)kfh.GetUInt(szCache_ProcessEditorInput, 0);
	pProcessHostFile = (PLUGINPROCESSHOSTFILE)(INT_PTR)kfh.GetUInt(szCache_ProcessHostFile, 0);
	pProcessViewerEvent = (PLUGINPROCESSVIEWEREVENT)(INT_PTR)kfh.GetUInt(szCache_ProcessViewerEvent, 0);
	pSetFindList = (PLUGINSETFINDLIST)(INT_PTR)kfh.GetUInt(szCache_SetFindList, 0);

	WorkFlags.Set(PIWF_CACHED); //too much "cached" flags

	if (kfh.GetInt(szCache_Preopen) != 0)
		OpenModule();

	return true;
}

bool PluginA::SaveToCache()
{
	KeyFileHelper kfh(PluginsIni());
	kfh.RemoveSection(GetSettingsName());

	const std::string &module = m_strModuleName.GetMB();

	struct stat st{};
	if (stat(module.c_str(), &st) == -1)
	{
		fprintf(stderr, "%s: stat('%s') error %u\n",
			__FUNCTION__, module.c_str(), errno);
		return false;
	}

	kfh.SetString(GetSettingsName(), "Module", module.c_str());

	PluginInfo Info{};
	GetPluginInfo(&Info);
	SysID = Info.SysID; //LAME!!!

	kfh.SetInt(GetSettingsName(), szCache_Preopen, ((Info.Flags & PF_PREOPEN) != 0));

	if ((Info.Flags & PF_PRELOAD) != 0)
	{
		kfh.SetInt(GetSettingsName(), szCache_Preload, 1);
		WorkFlags.Change(PIWF_PRELOADED, TRUE);
		return true;
	}
	WorkFlags.Change(PIWF_PRELOADED, FALSE);

	kfh.SetString(GetSettingsName(), "ID", m_strModuleID.c_str());

	for (int i = 0; i < Info.DiskMenuStringsNumber; i++)
	{
		kfh.SetString(GetSettingsName(), StrPrintf(FmtDiskMenuStringD, i), Info.DiskMenuStrings[i]);
	}

	for (int i = 0; i < Info.PluginMenuStringsNumber; i++)
	{
		kfh.SetString(GetSettingsName(), StrPrintf(FmtPluginMenuStringD, i),
				Info.PluginMenuStrings[i]);
	}

	for (int i = 0; i < Info.PluginConfigStringsNumber; i++)
	{
		kfh.SetString(GetSettingsName(), StrPrintf(FmtPluginConfigStringD, i),
				Info.PluginConfigStrings[i]);
	}

	kfh.SetString(GetSettingsName(), "CommandPrefix", Info.CommandPrefix);
	kfh.SetUInt(GetSettingsName(), "Flags", Info.Flags);
	kfh.SetUInt(GetSettingsName(), szCache_SysID, SysID);

	kfh.SetUInt(GetSettingsName(), szCache_Configure,          pConfigure ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_GetFiles,           pGetFiles ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_OpenFilePlugin,     pOpenFilePlugin ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_OpenPlugin,         pOpenPlugin ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessDialogEvent, pProcessDialogEvent ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessEditorEvent, pProcessEditorEvent ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessEditorInput, pProcessEditorInput ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessHostFile,    pProcessHostFile ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessViewerEvent, pProcessViewerEvent ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_SetFindList,        pSetFindList ? 1:0);

	kfh.SetString(GetSettingsName(), szCache_Author, strAuthor);
	kfh.SetString(GetSettingsName(), szCache_Description, strDescription);
	kfh.SetString(GetSettingsName(), szCache_Title, strTitle);
	kfh.SetBytes (GetSettingsName(), szCache_Version, (unsigned char*)&m_PlugVersion, sizeof(m_PlugVersion), 1);

	return true;
}

bool PluginA::Load()
{
	if (m_Loaded)
		return true;

	if (!OpenModule() || !GetGlobalInfo())
		return false;

	m_Loaded = true;

	WorkFlags.Clear(PIWF_CACHED);

	GetModuleFN(pClosePlugin, NFMP_ClosePlugin);
	GetModuleFN(pCompare, NFMP_Compare);
	GetModuleFN(pConfigure, NFMP_Configure);
	GetModuleFN(pDeleteFiles, NFMP_DeleteFiles);
	GetModuleFN(pExitFAR, NFMP_ExitFAR);
	GetModuleFN(pFreeFindData, NFMP_FreeFindData);
	GetModuleFN(pFreeVirtualFindData, NFMP_FreeVirtualFindData);
	GetModuleFN(pGetFiles, NFMP_GetFiles);
	GetModuleFN(pGetFindData, NFMP_GetFindData);
	GetModuleFN(pGetOpenPluginInfo, NFMP_GetOpenPluginInfo);
	GetModuleFN(pGetPluginInfo, NFMP_GetPluginInfo);
	GetModuleFN(pGetVirtualFindData, NFMP_GetVirtualFindData);
	GetModuleFN(pMakeDirectory, NFMP_MakeDirectory);
	GetModuleFN(pMayExitFAR, NFMP_MayExitFAR);
	GetModuleFN(pMinFarVersion, NFMP_GetMinFarVersion);
	GetModuleFN(pOpenFilePlugin, NFMP_OpenFilePlugin);
	GetModuleFN(pOpenPlugin, NFMP_OpenPlugin);
	GetModuleFN(pProcessDialogEvent, NFMP_ProcessDialogEvent);
	GetModuleFN(pProcessEditorEvent, NFMP_ProcessEditorEvent);
	GetModuleFN(pProcessEditorInput, NFMP_ProcessEditorInput);
	GetModuleFN(pProcessEvent, NFMP_ProcessEvent);
	GetModuleFN(pProcessHostFile, NFMP_ProcessHostFile);
	GetModuleFN(pProcessKey, NFMP_ProcessKey);
	GetModuleFN(pProcessViewerEvent, NFMP_ProcessViewerEvent);
	GetModuleFN(pPutFiles, NFMP_PutFiles);
	GetModuleFN(pSetDirectory, NFMP_SetDirectory);
	GetModuleFN(pSetFindList, NFMP_SetFindList);
	GetModuleFN(pSetStartupInfo, NFMP_SetStartupInfo);

	bool bUnloaded = false;

	if (CheckMinFarVersion(bUnloaded))
	{
		if (SetStartupInfo(bUnloaded))
		{
			SaveToCache();
			return true;
		}
	}

	if (!bUnloaded)
		Unload();

	//чтоб не пытаться загрузить опять а то ошибка будет постоянно показываться.
	WorkFlags.Set(PIWF_DONTLOADAGAIN);

	return false;
}


static void farDisplayNotificationA(const char *action, const char *object)
{
	DisplayNotification(action, object);
}

static int farDispatchInterThreadCallsA()
{
	return DispatchInterThreadCalls();
}

static void WINAPI farBackgroundTaskA(const char *Info, BOOL Started)
{
	if (Started)
		CtrlObject->Plugins.BackgroundTaskStarted(MB2Wide(Info).c_str());
	else
		CtrlObject->Plugins.BackgroundTaskFinished(MB2Wide(Info).c_str());
}

static size_t WINAPI farStrCellsCountA(const char *Str, size_t CharsCount)
{
	std::wstring ws;
	MB2Wide(Str, CharsCount, ws);
	return StrCellsCount(ws.c_str(), ws.size());
}

static size_t WINAPI farStrSizeOfCellsA(const char *Str, size_t CharsCount, size_t *CellsCount, BOOL RoundUp)
{
	std::wstring ws;
	MB2Wide(Str, CharsCount, ws);
	size_t cnt = StrSizeOfCells(ws.c_str(), ws.size(), *CellsCount, RoundUp != FALSE);
	ws.resize(cnt);
	return StrWide2MB(ws).size();
}

static void CreatePluginStartupInfoA(PluginA *pPlugin, oldfar::PluginStartupInfo *PSI, oldfar::FarStandardFunctions *FSF)
{
	static oldfar::PluginStartupInfo StartupInfo{};
	static oldfar::FarStandardFunctions StandardFunctions{};

	// заполняем структуру StandardFunctions один раз!!!
	if (!StandardFunctions.StructSize)
	{
		StandardFunctions.StructSize = sizeof(StandardFunctions);
		StandardFunctions.AddEndSlash = AddEndSlashA;
		StandardFunctions.atoi64 = FarAtoi64A;
		StandardFunctions.atoi = FarAtoiA;
		StandardFunctions.BackgroundTask = farBackgroundTaskA;
		StandardFunctions.bsearch = FarBsearch;
		StandardFunctions.ConvertNameToReal = ConvertNameToRealA;
		StandardFunctions.CopyToClipboard = CopyToClipboardA;
		StandardFunctions.DeleteBuffer = DeleteBufferA;
		StandardFunctions.DispatchInterThreadCalls = farDispatchInterThreadCallsA;
		StandardFunctions.DisplayNotification = farDisplayNotificationA;
		StandardFunctions.Execute = farExecuteA;
		StandardFunctions.ExecuteLibrary = farExecuteLibraryA;
		StandardFunctions.ExpandEnvironmentStr = ExpandEnvironmentStrA;
		StandardFunctions.FarInputRecordToKey = InputRecordToKeyA;
		StandardFunctions.FarKeyToName = FarKeyToNameA;
		StandardFunctions.FarNameToKey = KeyNameToKeyA;
		StandardFunctions.FarRecursiveSearch = FarRecursiveSearchA;
		StandardFunctions.GetFileGroup = GetFileGroupA;
		StandardFunctions.GetFileOwner = GetFileOwnerA;
		StandardFunctions.GetNumberOfLinks = GetNumberOfLinksA;
		StandardFunctions.GetPathRoot = GetPathRootA;
		StandardFunctions.GetReparsePointInfo = FarGetReparsePointInfoA;
		StandardFunctions.itoa64 = FarItoa64A;
		StandardFunctions.itoa = FarItoaA;
		StandardFunctions.LTrim = RemoveLeadingSpacesA;
		StandardFunctions.MkLink = FarMkLinkA;
		StandardFunctions.MkTemp = FarMkTempA;
		StandardFunctions.PasteFromClipboard = PasteFromClipboardA;
		StandardFunctions.PointToName = PointToNameA;
		StandardFunctions.ProcessName = ProcessNameA;
		StandardFunctions.qsortex = FarQsortEx;
		StandardFunctions.qsort = FarQsort;
		StandardFunctions.QuoteSpaceOnly = QuoteSpaceOnlyA;
		StandardFunctions.RTrim = RemoveTrailingSpacesA;
		StandardFunctions.snprintf = snprintf;
		StandardFunctions.sprintf = sprintf;
		StandardFunctions.sscanf = sscanf;
		StandardFunctions.StrCellsCount = farStrCellsCountA;
		StandardFunctions.StrSizeOfCells = farStrSizeOfCellsA;
		StandardFunctions.Trim = RemoveExternalSpacesA;
		StandardFunctions.TruncPathStr = TruncPathStrA;
		StandardFunctions.TruncStr = TruncStrA;
		StandardFunctions.Unquote = UnquoteA;
		StandardFunctions.VTEnumBackground = farAPIVTEnumBackground;
		StandardFunctions.VTLogExport = farAPIVTLogExportA;
		StandardFunctions.XLat = XlatA;
	}

	if (!StartupInfo.StructSize)
	{
		StartupInfo.StructSize = sizeof(StartupInfo);
		StartupInfo.AdvControl = FarAdvControlA;
		StartupInfo.CharTable = FarCharTableA;
		StartupInfo.CmpName = FarCmpNameA;
		StartupInfo.Control = FarControlA;
		StartupInfo.DefDlgProc = FarDefDlgProcA;
		StartupInfo.DialogEx = FarDialogExA;
		StartupInfo.Dialog = FarDialogFnA;
		StartupInfo.EditorControl = FarEditorControlA;
		StartupInfo.Editor = FarEditorA;
		StartupInfo.FreeDirList = FarFreeDirListA;
		StartupInfo.GetDirList = FarGetDirListA;
		StartupInfo.GetMsg = FarGetMsgFnA;
		StartupInfo.GetPluginDirList = FarGetPluginDirListA;
		StartupInfo.InputBox = FarInputBoxA;
		StartupInfo.Menu = FarMenuFnA;
		StartupInfo.Message = FarMessageFnA;
		StartupInfo.RestoreScreen = FarRestoreScreen;
		StartupInfo.SaveScreen = FarSaveScreen;
		StartupInfo.SendDlgMessage = FarSendDlgMessageA;
		StartupInfo.ShowHelp = FarShowHelpA;
		StartupInfo.Text = FarTextA;
		StartupInfo.ViewerControl = FarViewerControlA;
		StartupInfo.Viewer = FarViewerA;
	}

	*PSI = StartupInfo;
	*FSF = StandardFunctions;
	PSI->ModuleNumber = (INT_PTR)pPlugin;
	PSI->FSF = FSF;
	pPlugin->GetModuleName().GetCharString(PSI->ModuleName,sizeof(PSI->ModuleName));
	PSI->RootKey = "";
}

bool PluginA::SetStartupInfo(bool &bUnloaded)
{
	if (pSetStartupInfo)
	{
		oldfar::PluginStartupInfo _info;
		oldfar::FarStandardFunctions _fsf;

		CreatePluginStartupInfoA(this, &_info, &_fsf);
		ExecuteStruct es(EXCEPT_SETSTARTUPINFO);
		EXECUTE_FUNCTION(pSetStartupInfo(&_info), es);

		if (es.bUnloaded)
		{
			bUnloaded = true;
			return false;
		}
	}

	return true;
}

bool PluginA::CheckMinFarVersion(bool &bUnloaded)
{
	if (pMinFarVersion)
	{
		ExecuteStruct es(EXCEPT_MINFARVERSION);
		EXECUTE_FUNCTION_EX(pMinFarVersion(), es);

		if (es.bUnloaded)
		{
			bUnloaded = true;
			return false;
		}

		DWORD FVer = (DWORD)es.nResult;

		if (FVer > FAR_VERSION)
		{
			ShowMessageAboutIllegalPluginVersion(m_strModuleName,FVer);
			return false;
		}
	}

	return true;
}

int PluginA::Unload(bool bExitFAR)
{
	int nResult = TRUE;

	if (bExitFAR)
		ExitFAR();

	if (!WorkFlags.Check(PIWF_CACHED))
		ClearExports();

	CloseModule();

	m_Loaded = false;
	return nResult;
}

bool PluginA::IsPanelPlugin()
{
	return pSetFindList ||
	       pGetFindData ||
	       pGetVirtualFindData ||
	       pSetDirectory ||
	       pGetFiles ||
	       pPutFiles ||
	       pDeleteFiles ||
	       pMakeDirectory ||
	       pProcessHostFile ||
	       pProcessKey ||
	       pProcessEvent ||
	       pCompare ||
	       pGetOpenPluginInfo ||
	       pFreeFindData ||
	       pFreeVirtualFindData ||
	       pClosePlugin;
}

HANDLE PluginA::OpenPlugin(int OpenFrom, INT_PTR Item)
{
	//ChangePriority *ChPriority = new ChangePriority(THREAD_PRIORITY_NORMAL);

	CheckScreenLock(); //??

	{
//		FARString strCurDir;
//		CtrlObject->CmdLine->GetCurDir(strCurDir);
//		FarChDir(strCurDir);
		g_strDirToSet.Clear();
	}

	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (Load() && pOpenPlugin)
	{
		ExecuteStruct es(EXCEPT_OPENPLUGIN);
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		es.hResult = INVALID_HANDLE_VALUE;
		char *ItemA = nullptr;

		if (Item && (OpenFrom == OPEN_COMMANDLINE  || OpenFrom == OPEN_SHORTCUT))
		{
			ItemA = UnicodeToAnsi((const wchar_t *)Item);
			Item = (INT_PTR)ItemA;
		}

		EXECUTE_FUNCTION_EX(pOpenPlugin(OpenFrom,Item), es);

		if (ItemA) free(ItemA);

		hResult = es.hResult;
		/*    CtrlObject->Macro.SetRedrawEditor(TRUE); //BUGBUG

		    if ( !es.bUnloaded )
		    {

		      if(OpenFrom == OPEN_EDITOR &&
		         !CtrlObject->Macro.IsExecuting() &&
		         CtrlObject->Plugins.CurEditor &&
		         CtrlObject->Plugins.CurEditor->IsVisible() )
		      {
		        CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW,EEREDRAW_CHANGE);
		        CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW,EEREDRAW_ALL);
		        CtrlObject->Plugins.CurEditor->Show();
		      }
		      if (hInternal!=INVALID_HANDLE_VALUE)
		      {
		        PanelHandle *hPlugin=new PanelHandle;
		        hPlugin->InternalHandle=es.hResult;
		        hPlugin->PluginNumber=(INT_PTR)this;
		        return((HANDLE)hPlugin);
		      }
		      else
		        if ( !g_strDirToSet.IsEmpty() )
		        {
							CtrlObject->Cp()->ActivePanel->SetCurDir(g_strDirToSet,true);
		          CtrlObject->Cp()->ActivePanel->Redraw();
		        }
		    } */
	}

//	delete ChPriority;

	return hResult;
}

//////////////////////////////////

HANDLE PluginA::OpenFilePlugin(
    const wchar_t *Name,
    const unsigned char *Data,
    int DataSize,
    int OpMode
)
{
	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (Load() && pOpenFilePlugin)
	{
		ExecuteStruct es(EXCEPT_OPENFILEPLUGIN);
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		char *NameA = nullptr;

		if (Name)
			NameA = UnicodeToAnsi(Name);

		EXECUTE_FUNCTION_EX(pOpenFilePlugin(NameA, Data, DataSize, OpMode), es);

		if (NameA) free(NameA);

		hResult = es.hResult;
	}

	return hResult;
}


int PluginA::SetFindList(
    HANDLE hPlugin,
    const PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	BOOL bResult = FALSE;

	if (pSetFindList)
	{
		ExecuteStruct es(EXCEPT_SETFINDLIST);
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		EXECUTE_FUNCTION_EX(pSetFindList(hPlugin, PanelItemA, ItemsNumber), es);
		FreePanelItemA(PanelItemA,ItemsNumber);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::ProcessEditorInput(
    const INPUT_RECORD *D
)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessEditorInput)
	{
		ExecuteStruct es(EXCEPT_PROCESSEDITORINPUT);
		es.bDefaultResult = TRUE; //(TRUE) treat the result as a completed request on exception!
		const INPUT_RECORD *Ptr=D;
		INPUT_RECORD OemRecord;

		if (Ptr->EventType==KEY_EVENT)
		{
			OemRecord=*D;
			int r = WINPORT(WideCharToMultiByte)(CP_UTF8, 0,  &D->Event.KeyEvent.uChar.UnicodeChar,
					1, &OemRecord.Event.KeyEvent.uChar.AsciiChar,1, nullptr, nullptr);
			if (r<0) fprintf(stderr, "PluginA::ProcessEditorInput: convert failed\n");
			//CharToOemBuff(&D->Event.KeyEvent.uChar.UnicodeChar,&OemRecord.Event.KeyEvent.uChar.AsciiChar,1);
			Ptr=&OemRecord;
		}

		EXECUTE_FUNCTION_EX(pProcessEditorInput(Ptr), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::ProcessEditorEvent(
    int Event,
    void *Param
)
{
	if (Load() && pProcessEditorEvent)
	{
		ExecuteStruct es(EXCEPT_PROCESSEDITOREVENT);
		EXECUTE_FUNCTION_EX(pProcessEditorEvent(Event, Param), es);
		(void)es; // supress 'set but not used' warning
	}

	return 0; //oops!
}

int PluginA::ProcessViewerEvent(
    int Event,
    void *Param
)
{
	if (Load() && pProcessViewerEvent)
	{
		ExecuteStruct es(EXCEPT_PROCESSVIEWEREVENT);
		EXECUTE_FUNCTION_EX(pProcessViewerEvent(Event, Param), es);
		(void)es; // supress 'set but not used' warning
	}

	return 0; //oops, again!
}

int PluginA::ProcessDialogEvent(
    int Event,
    void *Param
)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessDialogEvent)
	{
		ExecuteStruct es(EXCEPT_PROCESSDIALOGEVENT);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pProcessDialogEvent(Event, Param), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::GetVirtualFindData(
    HANDLE hPlugin,
    PluginPanelItem **pPanelItem,
    int *pItemsNumber,
    const wchar_t *Path
)
{
	BOOL bResult = FALSE;

	if (pGetVirtualFindData)
	{
		ExecuteStruct es(EXCEPT_GETVIRTUALFINDDATA);
		es.bDefaultResult = FALSE;
		pVFDPanelItemA = nullptr;
		size_t Size=StrLength(Path)+1;
		LPSTR PathA=new char[Size * 4];
		PWZ_to_PZ(Path,PathA, Size * 4);
		EXECUTE_FUNCTION_EX(pGetVirtualFindData(hPlugin, &pVFDPanelItemA, pItemsNumber, PathA), es);
		bResult = es.bResult;
		delete[] PathA;

		if (bResult && *pItemsNumber)
		{
			ConvertPanelItemA(pVFDPanelItemA, pPanelItem, *pItemsNumber);
		}
	}

	return bResult;
}


void PluginA::FreeVirtualFindData(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	FreeUnicodePanelItem(PanelItem, ItemsNumber);

	if (pFreeVirtualFindData && pVFDPanelItemA)
	{
		ExecuteStruct es(EXCEPT_FREEVIRTUALFINDDATA);
		EXECUTE_FUNCTION(pFreeVirtualFindData(hPlugin, pVFDPanelItemA, ItemsNumber), es);
		pVFDPanelItemA = nullptr;
		(void)es; // supress 'set but not used' warning
	}
}

bool PluginA::GetLinkTarget(HANDLE hPlugin, PluginPanelItem *PanelItem, FARString &result, int OpMode)
{
	return false;
}

int PluginA::GetFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    const wchar_t **DestPath,
    int OpMode
)
{
	int nResult = -1;

	if (pGetFiles)
	{
		ExecuteStruct es(EXCEPT_GETFILES);
		es.nDefaultResult = -1;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		char DestA[oldfar::NM];
		PWZ_to_PZ(*DestPath,DestA,sizeof(DestA));
		EXECUTE_FUNCTION_EX(pGetFiles(hPlugin, PanelItemA, ItemsNumber, Move, DestA, OpMode), es);
		static wchar_t DestW[oldfar::NM];
		PZ_to_PWZ(DestA,DestW,ARRAYSIZE(DestW));
		*DestPath=DestW;
		FreePanelItemA(PanelItemA,ItemsNumber);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginA::PutFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    int OpMode
)
{
	int nResult = -1;

	if (pPutFiles)
	{
		ExecuteStruct es(EXCEPT_PUTFILES);
		es.nDefaultResult = -1;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		EXECUTE_FUNCTION_EX(pPutFiles(hPlugin, PanelItemA, ItemsNumber, Move, OpMode), es);
		FreePanelItemA(PanelItemA,ItemsNumber);
		nResult = (int)es.nResult;
	}

	return nResult;
}

int PluginA::DeleteFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pDeleteFiles)
	{
		ExecuteStruct es(EXCEPT_DELETEFILES);
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		EXECUTE_FUNCTION_EX(pDeleteFiles(hPlugin, PanelItemA, ItemsNumber, OpMode), es);
		FreePanelItemA(PanelItemA,ItemsNumber);
		bResult = (int)es.bResult;
	}

	return bResult;
}


int PluginA::MakeDirectory(
    HANDLE hPlugin,
    const wchar_t **Name,
    int OpMode
)
{
	int nResult = -1;

	if (pMakeDirectory)
	{
		ExecuteStruct es(EXCEPT_MAKEDIRECTORY);
		es.nDefaultResult = -1;
		char NameA[oldfar::NM];
		PWZ_to_PZ(*Name,NameA,sizeof(NameA));
		EXECUTE_FUNCTION_EX(pMakeDirectory(hPlugin, NameA, OpMode), es);
		static wchar_t NameW[oldfar::NM];
		PZ_to_PWZ(NameA,NameW,ARRAYSIZE(NameW));
		*Name=NameW;
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginA::ProcessHostFile(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pProcessHostFile)
	{
		ExecuteStruct es(EXCEPT_PROCESSHOSTFILE);
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		EXECUTE_FUNCTION_EX(pProcessHostFile(hPlugin, PanelItemA, ItemsNumber, OpMode), es);
		FreePanelItemA(PanelItemA,ItemsNumber);
		bResult = es.bResult;
	}

	return bResult;
}


int PluginA::ProcessEvent(
    HANDLE hPlugin,
    int Event,
    void *Param
)
{
	BOOL bResult = FALSE;

	if (pProcessEvent)
	{
		ExecuteStruct es(EXCEPT_PROCESSEVENT);
		es.bDefaultResult = FALSE;
		void *ParamA = Param;

		if (Param && (Event == FE_COMMAND || Event == FE_CHANGEVIEWMODE))
			ParamA = (PVOID)UnicodeToAnsi((const wchar_t *)Param);

		EXECUTE_FUNCTION_EX(pProcessEvent(hPlugin, Event, ParamA), es);

		if (ParamA && (Event == FE_COMMAND || Event == FE_CHANGEVIEWMODE))
			free(ParamA);

		bResult = es.bResult;
	}

	return bResult;
}


int PluginA::Compare(
    HANDLE hPlugin,
    const PluginPanelItem *Item1,
    const PluginPanelItem *Item2,
    DWORD Mode
)
{
	int nResult = -2;

	if (pCompare)
	{
		ExecuteStruct es(EXCEPT_COMPARE);
		es.nDefaultResult = -2;
		oldfar::PluginPanelItem *Item1A = nullptr;
		oldfar::PluginPanelItem *Item2A = nullptr;
		ConvertPanelItemsArrayToAnsi(Item1,Item1A,1);
		ConvertPanelItemsArrayToAnsi(Item2,Item2A,1);
		EXECUTE_FUNCTION_EX(pCompare(hPlugin, Item1A, Item2A, Mode), es);
		FreePanelItemA(Item1A,1);
		FreePanelItemA(Item2A,1);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginA::GetFindData(
    HANDLE hPlugin,
    PluginPanelItem **pPanelItem,
    int *pItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pGetFindData)
	{
		ExecuteStruct es(EXCEPT_GETFINDDATA);
		es.bDefaultResult = FALSE;
		pFDPanelItemA = nullptr;
		EXECUTE_FUNCTION_EX(pGetFindData(hPlugin, &pFDPanelItemA, pItemsNumber, OpMode), es);
		bResult = es.bResult;

		if (bResult && *pItemsNumber)
		{
			ConvertPanelItemA(pFDPanelItemA, pPanelItem, *pItemsNumber);
		}
	}

	return bResult;
}


void PluginA::FreeFindData(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	FreeUnicodePanelItem(PanelItem, ItemsNumber);

	if (pFreeFindData && pFDPanelItemA)
	{
		ExecuteStruct es(EXCEPT_FREEFINDDATA);
		EXECUTE_FUNCTION(pFreeFindData(hPlugin, pFDPanelItemA, ItemsNumber), es);
		pFDPanelItemA = nullptr;
		(void)es; // supress 'set but not used' warning
	}
}

int PluginA::ProcessKey(
    HANDLE hPlugin,
    int Key,
    unsigned int dwControlState
)
{
	BOOL bResult = FALSE;

	if (pProcessKey)
	{
		ExecuteStruct es(EXCEPT_PROCESSKEY);
		es.bDefaultResult = TRUE; // do not pass this key to far on exception
		EXECUTE_FUNCTION_EX(pProcessKey(hPlugin, Key, dwControlState), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginA::ClosePlugin(
    HANDLE hPlugin
)
{
	if (pClosePlugin)
	{
		ExecuteStruct es(EXCEPT_CLOSEPLUGIN);
		EXECUTE_FUNCTION(pClosePlugin(hPlugin), es);
		(void)es; // supress 'set but not used' warning
	}

	FreeOpenPluginInfo();
	//	m_pManager->m_pCurrentPlugin = (Plugin*)-1;
}


int PluginA::SetDirectory(
    HANDLE hPlugin,
    const wchar_t *Dir,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pSetDirectory)
	{
		ExecuteStruct es(EXCEPT_SETDIRECTORY);
		es.bDefaultResult = FALSE;
		char *DirA = UnicodeToAnsi(Dir);
		EXECUTE_FUNCTION_EX(pSetDirectory(hPlugin, DirA, OpMode), es);

		if (DirA) free(DirA);

		bResult = es.bResult;
	}

	return bResult;
}

void PluginA::FreeOpenPluginInfo()
{
	if (OPI.CurDir)
		free((void *)OPI.CurDir);

	if (OPI.HostFile)
		free((void *)OPI.HostFile);

	if (OPI.Format)
		free((void *)OPI.Format);

	if (OPI.PanelTitle)
		free((void *)OPI.PanelTitle);

	if (OPI.InfoLines && OPI.InfoLinesNumber)
	{
		FreeUnicodeInfoPanelLines((InfoPanelLine*)OPI.InfoLines,OPI.InfoLinesNumber);
	}

	if (OPI.DescrFiles)
	{
		FreeArrayUnicode((wchar_t**)OPI.DescrFiles);
	}

	if (OPI.PanelModesArray)
	{
		FreeUnicodePanelModes((PanelMode*)OPI.PanelModesArray, OPI.PanelModesNumber);
	}

	if (OPI.KeyBar)
	{
		FreeUnicodeKeyBarTitles((KeyBarTitles*)OPI.KeyBar);
		free((void *)OPI.KeyBar);
	}

	if (OPI.ShortcutData)
		free((void *)OPI.ShortcutData);

	memset(&OPI,0,sizeof(OPI));
}

void PluginA::ConvertOpenPluginInfo(oldfar::OpenPluginInfo &Src, OpenPluginInfo *Dest)
{
	FreeOpenPluginInfo();
	OPI.StructSize = sizeof(OPI);
	OPI.Flags = Src.Flags;

	if (Src.CurDir)
		OPI.CurDir = AnsiToUnicode(Src.CurDir);

	if (Src.HostFile)
		OPI.HostFile = AnsiToUnicode(Src.HostFile);

	if (Src.Format)
		OPI.Format = AnsiToUnicode(Src.Format);

	if (Src.PanelTitle)
		OPI.PanelTitle = AnsiToUnicode(Src.PanelTitle);

	if (Src.InfoLines && Src.InfoLinesNumber)
	{
		ConvertInfoPanelLinesA(Src.InfoLines, (InfoPanelLine**)&OPI.InfoLines, Src.InfoLinesNumber);
		OPI.InfoLinesNumber = Src.InfoLinesNumber;
	}

	if (Src.DescrFiles && Src.DescrFilesNumber)
	{
		OPI.DescrFiles = ArrayAnsiToUnicode((char**)Src.DescrFiles, Src.DescrFilesNumber);
		OPI.DescrFilesNumber = Src.DescrFilesNumber;
	}

	if (Src.PanelModesArray && Src.PanelModesNumber)
	{
		ConvertPanelModesA(Src.PanelModesArray, (PanelMode**)&OPI.PanelModesArray, Src.PanelModesNumber);
		OPI.PanelModesNumber	= Src.PanelModesNumber;
		OPI.StartPanelMode		= Src.StartPanelMode;
		OPI.StartSortMode			= Src.StartSortMode;
		OPI.StartSortOrder		= Src.StartSortOrder;
	}

	if (Src.KeyBar)
	{
		OPI.KeyBar=(KeyBarTitles*) malloc(sizeof(KeyBarTitles));
		ConvertKeyBarTitlesA(Src.KeyBar, (KeyBarTitles*)OPI.KeyBar, Src.StructSize>=(int)sizeof(oldfar::OpenPluginInfo));
	}

	if (Src.ShortcutData)
		OPI.ShortcutData = AnsiToUnicode(Src.ShortcutData);

	*Dest=OPI;
}

void PluginA::GetOpenPluginInfo(
    HANDLE hPlugin,
    OpenPluginInfo *pInfo
)
{
//	m_pManager->m_pCurrentPlugin = this;
	pInfo->StructSize = sizeof(OpenPluginInfo);

	if (pGetOpenPluginInfo)
	{
		ExecuteStruct es(EXCEPT_GETOPENPLUGININFO);
		oldfar::OpenPluginInfo InfoA{};
		EXECUTE_FUNCTION(pGetOpenPluginInfo(hPlugin, &InfoA), es);
		ConvertOpenPluginInfo(InfoA,pInfo);
		(void)es; // supress 'set but not used' warning
	}
}


int PluginA::Configure(
    int MenuItem
)
{
	BOOL bResult = FALSE;

	if (Load() && pConfigure)
	{
		ExecuteStruct es(EXCEPT_CONFIGURE);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pConfigure(MenuItem), es);
		bResult = es.bResult;
	}

	return bResult;
}

void PluginA::FreePluginInfo()
{
	if (PI.DiskMenuStringsNumber)
	{
		for (int i=0; i<PI.DiskMenuStringsNumber; i++)
			free((void *)PI.DiskMenuStrings[i]);

		free((void *)PI.DiskMenuStrings);
	}

	if (PI.PluginMenuStringsNumber)
	{
		for (int i=0; i<PI.PluginMenuStringsNumber; i++)
			free((void *)PI.PluginMenuStrings[i]);

		free((void *)PI.PluginMenuStrings);
	}

	if (PI.PluginConfigStringsNumber)
	{
		for (int i=0; i<PI.PluginConfigStringsNumber; i++)
			free((void *)PI.PluginConfigStrings[i]);

		free((void *)PI.PluginConfigStrings);
	}

	if (PI.CommandPrefix)
		free((void *)PI.CommandPrefix);

	memset(&PI,0,sizeof(PI));
}

void PluginA::ConvertPluginInfo(oldfar::PluginInfo &Src, PluginInfo *Dest)
{
	FreePluginInfo();
	PI.StructSize = sizeof(PI);
	PI.Flags = Src.Flags;
	PI.SysID = Src.SysID;

	if (Src.DiskMenuStringsNumber)
	{
		wchar_t **p = (wchar_t **) malloc(Src.DiskMenuStringsNumber*sizeof(wchar_t*));

		for (int i=0; i<Src.DiskMenuStringsNumber; i++)
			p[i] = AnsiToUnicode(Src.DiskMenuStrings[i]);

		PI.DiskMenuStrings = p;
		PI.DiskMenuStringsNumber = Src.DiskMenuStringsNumber;
	}

	if (Src.PluginMenuStringsNumber)
	{
		wchar_t **p = (wchar_t **) malloc(Src.PluginMenuStringsNumber*sizeof(wchar_t*));

		for (int i=0; i<Src.PluginMenuStringsNumber; i++)
			p[i] = AnsiToUnicode(Src.PluginMenuStrings[i]);

		PI.PluginMenuStrings = p;
		PI.PluginMenuStringsNumber = Src.PluginMenuStringsNumber;
	}

	if (Src.PluginConfigStringsNumber)
	{
		wchar_t **p = (wchar_t **) malloc(Src.PluginConfigStringsNumber*sizeof(wchar_t*));

		for (int i=0; i<Src.PluginConfigStringsNumber; i++)
			p[i] = AnsiToUnicode(Src.PluginConfigStrings[i]);

		PI.PluginConfigStrings = p;
		PI.PluginConfigStringsNumber = Src.PluginConfigStringsNumber;
	}

	if (Src.CommandPrefix)
		PI.CommandPrefix = AnsiToUnicode(Src.CommandPrefix);

	*Dest=PI;
}

bool PluginA::GetPluginInfo(PluginInfo *pi)
{
	memset(pi, 0, sizeof(PluginInfo));

	if (pGetPluginInfo)
	{
		ExecuteStruct es(EXCEPT_GETPLUGININFO);
		oldfar::PluginInfo InfoA{};
		EXECUTE_FUNCTION(pGetPluginInfo(&InfoA), es);

		if (!es.bUnloaded)
		{
			ConvertPluginInfo(InfoA, pi);
			if (pi->SysID == 0) // prevent erasing SysID that may be already set by GetGlobalInfoW()
				pi->SysID = SysID;
			return true;
		}
	}

	return false;
}

bool PluginA::MayExitFAR()
{
	if (pMayExitFAR)
	{
		ExecuteStruct es(EXCEPT_MAYEXITFAR);
		es.bDefaultResult = 1;
		EXECUTE_FUNCTION_EX(pMayExitFAR(), es);
		return es.bResult;
	}

	return true;
}

void PluginA::ExitFAR()
{
	if (pExitFAR)
	{
		ExecuteStruct es(EXCEPT_EXITFAR);
		EXECUTE_FUNCTION(pExitFAR(), es);
		(void)es; // supress 'set but not used' warning
	}
}

void PluginA::ClearExports()
{
	pClosePlugin = nullptr;
	pCompare = nullptr;
	pConfigure = nullptr;
	pDeleteFiles = nullptr;
	pExitFAR = nullptr;
	pFreeFindData = nullptr;
	pFreeVirtualFindData = nullptr;
	pGetFiles = nullptr;
	pGetFindData = nullptr;
	pGetGlobalInfoW = nullptr;
	pGetOpenPluginInfo = nullptr;
	pGetPluginInfo = nullptr;
	pGetVirtualFindData = nullptr;
	pMakeDirectory = nullptr;
	pMayExitFAR = nullptr;
	pMinFarVersion = nullptr;
	pOpenFilePlugin = nullptr;
	pOpenPlugin = nullptr;
	pProcessDialogEvent = nullptr;
	pProcessEditorEvent = nullptr;
	pProcessEditorInput = nullptr;
	pProcessEvent = nullptr;
	pProcessHostFile = nullptr;
	pProcessKey = nullptr;
	pProcessViewerEvent = nullptr;
	pPutFiles = nullptr;
	pSetDirectory = nullptr;
	pSetFindList = nullptr;
	pSetStartupInfo = nullptr;
}
