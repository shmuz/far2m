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
#include "pick_color.hpp"
#include "wrap.cpp"
#include <KeyFileHelper.h>

static const char *szCache_Preload = "Preload";
static const char *szCache_Preopen = "Preopen";
static const char *szCache_SysID = "SysID";

static const char *szCache_Author = "Author";
static const char *szCache_Description = "Description";
static const char *szCache_Title = "Title";
static const char *szCache_Version = "Version";

#define MAKE_ACCESSORS(member_name) \
	[](PluginA* self) -> void* { return reinterpret_cast<void*>(self->member_name); }, \
	[](PluginA* self, void* ptr) { self->member_name = reinterpret_cast<decltype(self->member_name)>(ptr); }

const PluginA::PluginExportEntry PluginA::PLUGIN_EXPORTS[] = {
	{"ClosePlugin",         false, MAKE_ACCESSORS(pClosePlugin)},
	{"Compare",             false, MAKE_ACCESSORS(pCompare)},
	{"Configure",           true , MAKE_ACCESSORS(pConfigure)},
	{"DeleteFiles",         false, MAKE_ACCESSORS(pDeleteFiles)},
	{"ExitFAR",             false, MAKE_ACCESSORS(pExitFAR)},
	{"FreeFindData",        false, MAKE_ACCESSORS(pFreeFindData)},
	{"FreeVirtualFindData", false, MAKE_ACCESSORS(pFreeVirtualFindData)},
	{"GetFiles",            true,  MAKE_ACCESSORS(pGetFiles)},
	{"GetFindData",         false, MAKE_ACCESSORS(pGetFindData)},
	{"GetOpenPluginInfo",   false, MAKE_ACCESSORS(pGetOpenPluginInfo)},
	{"GetPluginInfo",       false, MAKE_ACCESSORS(pGetPluginInfo)},
	{"GetVirtualFindData",  false, MAKE_ACCESSORS(pGetVirtualFindData)},
	{"MakeDirectory",       false, MAKE_ACCESSORS(pMakeDirectory)},
	{"MayExitFAR",          false, MAKE_ACCESSORS(pMayExitFAR)},
	{"MinFarVersion",       false, MAKE_ACCESSORS(pMinFarVersion)},
	{"OpenFilePlugin",      true,  MAKE_ACCESSORS(pOpenFilePlugin)},
	{"OpenPlugin",          true,  MAKE_ACCESSORS(pOpenPlugin)},
	{"ProcessDialogEvent",  true,  MAKE_ACCESSORS(pProcessDialogEvent)},
	{"ProcessEditorEvent",  true,  MAKE_ACCESSORS(pProcessEditorEvent)},
	{"ProcessEditorInput",  true,  MAKE_ACCESSORS(pProcessEditorInput)},
	{"ProcessEvent",        false, MAKE_ACCESSORS(pProcessEvent)},
	{"ProcessHostFile",     true,  MAKE_ACCESSORS(pProcessHostFile)},
	{"ProcessKey",          false, MAKE_ACCESSORS(pProcessKey)},
	{"ProcessViewerEvent",  true,  MAKE_ACCESSORS(pProcessViewerEvent)},
	{"PutFiles",            false, MAKE_ACCESSORS(pPutFiles)},
	{"SetDirectory",        false, MAKE_ACCESSORS(pSetDirectory)},
	{"SetFindList",         true,  MAKE_ACCESSORS(pSetFindList)},
	{"SetStartupInfo",      false, MAKE_ACCESSORS(pSetStartupInfo)},
};


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

	// Load cached exports
	for (const auto& entry : PLUGIN_EXPORTS)
	{
		if (entry.cached)
		{
			void* ptr = nullptr;
			load_ptr(kfh, entry.export_name, ptr);
			entry.setter(this, ptr);
		}
	}

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
		fprintf(stderr, "%s: stat('%s') error %d\n",
			__FUNCTION__, module.c_str(), errno);
		return false;
	}

	kfh.SetString(GetSettingsName(), "Module", module.c_str());

	PluginInfo Info;
	GetPluginInfo(&Info);

	kfh.SetInt(GetSettingsName(), szCache_Preopen, ((Info.Flags & PF_PREOPEN) != 0));

	if ((Info.Flags & PF_PRELOAD) != 0)
	{
		kfh.SetInt(GetSettingsName(), szCache_Preload, 1);
		WorkFlags.Set(PIWF_PRELOADED);
		return true;
	}
	WorkFlags.Clear(PIWF_PRELOADED);

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

	// Save cached exports
	for (const auto& entry : PLUGIN_EXPORTS)
	{
		if (entry.cached)
		{
			void* ptr = entry.getter(this);
			kfh.SetUInt(GetSettingsName(), entry.export_name, ptr ? 1 : 0);
		}
	}

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

	// Load all exports from module
	for (const auto& entry : PLUGIN_EXPORTS)
	{
		void* ptr = nullptr;
		GetModuleFN(ptr, entry.export_name);
		entry.setter(this, ptr);
	}

	if (CheckMinFarVersion())
	{
		if (SetStartupInfo())
		{
			SaveToCache();
			return true;
		}
	}

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
		StartupInfo.ColorDialog = FarColorDialogA;
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

bool PluginA::SetStartupInfo()
{
	if (pSetStartupInfo)
	{
		oldfar::PluginStartupInfo _info;
		oldfar::FarStandardFunctions _fsf;

		CreatePluginStartupInfoA(this, &_info, &_fsf);
		ExecuteStruct es(EXCEPT_SETSTARTUPINFO);
		EXECUTE_FUNCTION(pSetStartupInfo(&_info), es);
	}

	return true;
}

bool PluginA::CheckMinFarVersion()
{
	if (pMinFarVersion)
	{
		ExecuteStruct es(EXCEPT_MINFARVERSION);
		EXECUTE_FUNCTION_EX(pMinFarVersion(), es);

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

HANDLE PluginA::OpenPlugin(int OpenFrom, const void *Item)
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
			Item = ItemA;
		}

		EXECUTE_FUNCTION_EX(pOpenPlugin(OpenFrom, (INT_PTR)Item), es);

		if (ItemA) free(ItemA);

		hResult = es.hResult;
	}

//	delete ChPriority;

	return hResult;
}

//////////////////////////////////

HANDLE PluginA::OpenFilePlugin(
    const wchar_t *Name,
    const unsigned char *Data,
    int DataSize,
    DWORD OpMode)
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


int PluginA::SetFindList(HANDLE hPanel, const PluginPanelItem *PanelItem, int ItemsNumber)
{
	BOOL bResult = FALSE;

	if (pSetFindList)
	{
		ExecuteStruct es(EXCEPT_SETFINDLIST);
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		EXECUTE_FUNCTION_BOOL(pSetFindList(hPanel, PanelItemA, ItemsNumber), es);
		FreePanelItemA(PanelItemA,ItemsNumber);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::ProcessEditorInput(const INPUT_RECORD *D)
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

		EXECUTE_FUNCTION_BOOL(pProcessEditorInput(Ptr), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::ProcessEditorEvent(int Event, void *Param)
{
	if (Load() && pProcessEditorEvent)
	{
		ExecuteStruct es(EXCEPT_PROCESSEDITOREVENT);
		EXECUTE_FUNCTION_EX(pProcessEditorEvent(Event, Param), es);
		(void)es; // supress 'set but not used' warning
	}

	return 0; //oops!
}

int PluginA::ProcessViewerEvent(int Event, void *Param)
{
	if (Load() && pProcessViewerEvent)
	{
		ExecuteStruct es(EXCEPT_PROCESSVIEWEREVENT);
		EXECUTE_FUNCTION_EX(pProcessViewerEvent(Event, Param), es);
		(void)es; // supress 'set but not used' warning
	}

	return 0; //oops, again!
}

int PluginA::ProcessDialogEvent(int Event, void *Param)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessDialogEvent)
	{
		ExecuteStruct es(EXCEPT_PROCESSDIALOGEVENT);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pProcessDialogEvent(Event, Param), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::GetVirtualFindData(
    HANDLE hPanel,
    PluginPanelItem **pPanelItem,
    int *pItemsNumber,
    const wchar_t *Path)
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
		EXECUTE_FUNCTION_BOOL(pGetVirtualFindData(hPanel, &pVFDPanelItemA, pItemsNumber, PathA), es);
		bResult = es.bResult;
		delete[] PathA;

		if (bResult && *pItemsNumber)
		{
			ConvertPanelItemA(pVFDPanelItemA, pPanelItem, *pItemsNumber);
		}
	}

	return bResult;
}


void PluginA::FreeVirtualFindData(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber)
{
	FreeUnicodePanelItem(PanelItem, ItemsNumber);

	if (pFreeVirtualFindData && pVFDPanelItemA)
	{
		ExecuteStruct es(EXCEPT_FREEVIRTUALFINDDATA);
		EXECUTE_FUNCTION(pFreeVirtualFindData(hPanel, pVFDPanelItemA, ItemsNumber), es);
		pVFDPanelItemA = nullptr;
		(void)es; // supress 'set but not used' warning
	}
}

bool PluginA::GetLinkTarget(HANDLE hPanel, PluginPanelItem *PanelItem, FARString &result, DWORD OpMode)
{
	return false;
}

int PluginA::GetFiles(
    HANDLE hPanel,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    bool Move,
    const wchar_t **DestPath,
    DWORD OpMode)
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
		EXECUTE_FUNCTION_EX(pGetFiles(hPanel, PanelItemA, ItemsNumber, Move, DestA, OpMode), es);
		static wchar_t DestW[oldfar::NM];
		PZ_to_PWZ(DestA,DestW,ARRAYSIZE(DestW));
		*DestPath=DestW;
		FreePanelItemA(PanelItemA,ItemsNumber);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginA::PutFiles(
    HANDLE hPanel,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    bool Move,
    DWORD OpMode)
{
	int nResult = -1;

	if (pPutFiles)
	{
		ExecuteStruct es(EXCEPT_PUTFILES);
		es.nDefaultResult = -1;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		EXECUTE_FUNCTION_EX(pPutFiles(hPanel, PanelItemA, ItemsNumber, Move, OpMode), es);
		FreePanelItemA(PanelItemA,ItemsNumber);
		nResult = (int)es.nResult;
	}

	return nResult;
}

int PluginA::DeleteFiles(
    HANDLE hPanel,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    DWORD OpMode)
{
	BOOL bResult = FALSE;

	if (pDeleteFiles)
	{
		ExecuteStruct es(EXCEPT_DELETEFILES);
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		EXECUTE_FUNCTION_BOOL(pDeleteFiles(hPanel, PanelItemA, ItemsNumber, OpMode), es);
		FreePanelItemA(PanelItemA,ItemsNumber);
		bResult = es.bResult;
	}

	return bResult;
}


int PluginA::MakeDirectory(HANDLE hPanel, const wchar_t **Name, DWORD OpMode)
{
	int nResult = -1;

	if (pMakeDirectory)
	{
		ExecuteStruct es(EXCEPT_MAKEDIRECTORY);
		es.nDefaultResult = -1;
		char NameA[oldfar::NM];
		PWZ_to_PZ(*Name,NameA,sizeof(NameA));
		EXECUTE_FUNCTION_EX(pMakeDirectory(hPanel, NameA, OpMode), es);
		static wchar_t NameW[oldfar::NM];
		PZ_to_PWZ(NameA,NameW,ARRAYSIZE(NameW));
		*Name=NameW;
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginA::ProcessHostFile(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber, DWORD OpMode)
{
	BOOL bResult = FALSE;

	if (pProcessHostFile)
	{
		ExecuteStruct es(EXCEPT_PROCESSHOSTFILE);
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem,PanelItemA,ItemsNumber);
		EXECUTE_FUNCTION_BOOL(pProcessHostFile(hPanel, PanelItemA, ItemsNumber, OpMode), es);
		FreePanelItemA(PanelItemA,ItemsNumber);
		bResult = es.bResult;
	}

	return bResult;
}


int PluginA::ProcessEvent(HANDLE hPanel, int Event, void *Param)
{
	BOOL bResult = FALSE;

	if (pProcessEvent)
	{
		ExecuteStruct es(EXCEPT_PROCESSEVENT);
		es.bDefaultResult = FALSE;
		void *ParamA = Param;

		if (Param && (Event == FE_COMMAND || Event == FE_CHANGEVIEWMODE))
			ParamA = (PVOID)UnicodeToAnsi((const wchar_t *)Param);

		EXECUTE_FUNCTION_BOOL(pProcessEvent(hPanel, Event, ParamA), es);

		if (ParamA && (Event == FE_COMMAND || Event == FE_CHANGEVIEWMODE))
			free(ParamA);

		bResult = es.bResult;
	}

	return bResult;
}


int PluginA::Compare(HANDLE hPanel, const PluginPanelItem *Item1, const PluginPanelItem *Item2, DWORD Mode)
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
		EXECUTE_FUNCTION_EX(pCompare(hPanel, Item1A, Item2A, Mode), es);
		FreePanelItemA(Item1A,1);
		FreePanelItemA(Item2A,1);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginA::GetFindData(HANDLE hPanel, PluginPanelItem **pPanelItem, int *pItemsNumber, DWORD OpMode)
{
	BOOL bResult = FALSE;

	if (pGetFindData)
	{
		ExecuteStruct es(EXCEPT_GETFINDDATA);
		es.bDefaultResult = FALSE;
		pFDPanelItemA = nullptr;
		EXECUTE_FUNCTION_BOOL(pGetFindData(hPanel, &pFDPanelItemA, pItemsNumber, OpMode), es);
		bResult = es.bResult;

		if (bResult && *pItemsNumber)
		{
			ConvertPanelItemA(pFDPanelItemA, pPanelItem, *pItemsNumber);
		}
	}

	return bResult;
}


void PluginA::FreeFindData(HANDLE hPanel, PluginPanelItem *PanelItem, int ItemsNumber)
{
	FreeUnicodePanelItem(PanelItem, ItemsNumber);

	if (pFreeFindData && pFDPanelItemA)
	{
		ExecuteStruct es(EXCEPT_FREEFINDDATA);
		EXECUTE_FUNCTION(pFreeFindData(hPanel, pFDPanelItemA, ItemsNumber), es);
		pFDPanelItemA = nullptr;
		(void)es; // supress 'set but not used' warning
	}
}

int PluginA::ProcessKey(HANDLE hPanel, int Key, unsigned int dwControlState)
{
	BOOL bResult = FALSE;

	if (pProcessKey)
	{
		ExecuteStruct es(EXCEPT_PROCESSKEY);
		es.bDefaultResult = TRUE; // do not pass this key to far on exception
		EXECUTE_FUNCTION_BOOL(pProcessKey(hPanel, Key, dwControlState), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginA::ClosePanel(HANDLE hPanel)
{
	if (pClosePlugin)
	{
		ExecuteStruct es(EXCEPT_CLOSEPLUGIN);
		EXECUTE_FUNCTION(pClosePlugin(hPanel), es);
		(void)es; // supress 'set but not used' warning
	}

	FreeOpenPluginInfo();
	//	m_pManager->m_pCurrentPlugin = (Plugin*)-1;
}


int PluginA::SetDirectory(HANDLE hPanel, const wchar_t *Dir, DWORD OpMode)
{
	BOOL bResult = FALSE;

	if (pSetDirectory)
	{
		ExecuteStruct es(EXCEPT_SETDIRECTORY);
		es.bDefaultResult = FALSE;
		char *DirA = UnicodeToAnsi(Dir);
		EXECUTE_FUNCTION_BOOL(pSetDirectory(hPanel, DirA, OpMode), es);

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
		ConvertKeyBarTitlesA(Src.KeyBar, (KeyBarTitles*)OPI.KeyBar, Src.StructSize>=sizeof(oldfar::OpenPluginInfo));
	}

	if (Src.ShortcutData)
		OPI.ShortcutData = AnsiToUnicode(Src.ShortcutData);

	*Dest=OPI;
}

void PluginA::GetOpenPluginInfo(HANDLE hPanel, OpenPluginInfo *pInfo)
{
//	m_pManager->m_pCurrentPlugin = this;
	pInfo->StructSize = sizeof(OpenPluginInfo);

	if (pGetOpenPluginInfo)
	{
		ExecuteStruct es(EXCEPT_GETOPENPLUGININFO);
		oldfar::OpenPluginInfo InfoA{};
		EXECUTE_FUNCTION(pGetOpenPluginInfo(hPanel, &InfoA), es);
		ConvertOpenPluginInfo(InfoA,pInfo);
		(void)es; // supress 'set but not used' warning
	}
}


int PluginA::Configure(int MenuItem)
{
	BOOL bResult = FALSE;

	if (Load() && pConfigure)
	{
		ExecuteStruct es(EXCEPT_CONFIGURE);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pConfigure(MenuItem), es);
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
		oldfar::PluginInfo InfoA { sizeof(InfoA) };
		EXECUTE_FUNCTION(pGetPluginInfo(&InfoA), es);

		ConvertPluginInfo(InfoA, pi);
		return true;
	}

	return false;
}

bool PluginA::MayExitFAR()
{
	if (pMayExitFAR)
	{
		ExecuteStruct es(EXCEPT_MAYEXITFAR);
		es.bDefaultResult = 1;
		EXECUTE_FUNCTION_BOOL(pMayExitFAR(), es);
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
	for (const auto& entry : PLUGIN_EXPORTS)
		entry.setter(this, nullptr);
}
