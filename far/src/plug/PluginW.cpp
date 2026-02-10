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

#include <dlfcn.h>

#include "plugins.hpp"
#include "plugapi.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "scantree.hpp"
#include "chgprior.hpp"
#include "constitle.hpp"
#include "cmdline.hpp"
#include "filepanels.hpp"
#include "fileattr.hpp"
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
#include "PluginW.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "xlat.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "processname.hpp"
#include "mix.hpp"
#include "interf.hpp"
#include "execute.hpp"
#include "flink.hpp"
#include "fileowner.hpp"
#include "datetime.hpp"

#include <string>
#include <list>
#include <vector>
#include <KeyFileHelper.h>

extern const wchar_t *PluginsFolderName;

static const char *szCache_Preload = "Preload";
static const char *szCache_Preopen = "Preopen";
static const char *szCache_SysID = "SysID";

static const char *szCache_Author = "Author";
static const char *szCache_Description = "Description";
static const char *szCache_Title = "Title";
static const char *szCache_UseMenuGuids = "UseMenuGuids";
static const char *szCache_Version = "Version";

static const char szCache_Analyse[] = "AnalyseW";
static const char szCache_Configure[] = "ConfigureW";
static const char szCache_ConfigureV3[] = "ConfigureV3W";
static const char szCache_GetCustomData[] = "GetCustomDataW";
static const char szCache_GetFiles[] = "GetFilesW";
static const char szCache_OpenFilePlugin[] = "OpenFilePluginW";
static const char szCache_OpenPlugin[] = "OpenPluginW";
static const char szCache_ProcessConsoleInput[] = "ProcessConsoleInputW";
static const char szCache_ProcessDialogEvent[] = "ProcessDialogEventW";
static const char szCache_ProcessEditorEvent[] = "ProcessEditorEventW";
static const char szCache_ProcessEditorEventV3[] = "ProcessEditorEventV3W";
static const char szCache_ProcessEditorInput[] = "ProcessEditorInputW";
static const char szCache_ProcessHostFile[] = "ProcessHostFileW";
static const char szCache_ProcessSynchroEvent[] = "ProcessSynchroEventW";
static const char szCache_ProcessViewerEvent[] = "ProcessViewerEventW";
static const char szCache_SetFindList[] = "SetFindListW";

static const char NFMP_Analyse[] = "AnalyseW";
static const char NFMP_CloseAnalyse[] = "CloseAnalyseW";
static const char NFMP_ClosePlugin[] = "ClosePluginW";
static const char NFMP_Compare[] = "CompareW";
static const char NFMP_Configure[] = "ConfigureW";
static const char NFMP_ConfigureV3[] = "ConfigureV3W";
static const char NFMP_DeleteFiles[] = "DeleteFilesW";
static const char NFMP_ExitFAR[] = "ExitFARW";
static const char NFMP_FreeCustomData[] = "FreeCustomDataW";
static const char NFMP_FreeFindData[] = "FreeFindDataW";
static const char NFMP_FreeVirtualFindData[] = "FreeVirtualFindDataW";
static const char NFMP_GetCustomData[] = "GetCustomDataW";
static const char NFMP_GetFiles[] = "GetFilesW";
static const char NFMP_GetFindData[] = "GetFindDataW";
static const char NFMP_GetLinkTarget[] = "GetLinkTargetW";
static const char NFMP_GetMinFarVersion[] = "GetMinFarVersionW";
static const char NFMP_GetOpenPluginInfo[] = "GetOpenPluginInfoW";
static const char NFMP_GetPluginInfo[] = "GetPluginInfoW";
static const char NFMP_GetVirtualFindData[] = "GetVirtualFindDataW";
static const char NFMP_MakeDirectory[] = "MakeDirectoryW";
static const char NFMP_MayExitFAR[] = "MayExitFARW";
static const char NFMP_OpenFilePlugin[] = "OpenFilePluginW";
static const char NFMP_OpenPlugin[] = "OpenPluginW";
static const char NFMP_ProcessConsoleInput[] = "ProcessConsoleInputW";
static const char NFMP_ProcessDialogEvent[] = "ProcessDialogEventW";
static const char NFMP_ProcessEditorEvent[] = "ProcessEditorEventW";
static const char NFMP_ProcessEditorEventV3[] = "ProcessEditorEventV3W";
static const char NFMP_ProcessEditorInput[] = "ProcessEditorInputW";
static const char NFMP_ProcessEvent[] = "ProcessEventW";
static const char NFMP_ProcessHostFile[] = "ProcessHostFileW";
static const char NFMP_ProcessKey[] = "ProcessKeyW";
static const char NFMP_ProcessSynchroEvent[] = "ProcessSynchroEventW";
static const char NFMP_ProcessViewerEvent[] = "ProcessViewerEventW";
static const char NFMP_PutFiles[] = "PutFilesW";
static const char NFMP_SetDirectory[] = "SetDirectoryW";
static const char NFMP_SetFindList[] = "SetFindListW";
static const char NFMP_SetStartupInfo[] = "SetStartupInfoW";


static size_t WINAPI FarKeyToName(FarKey Key,wchar_t *KeyText,size_t Size)
{
	FARString strKT;

	if (!KeyToText(Key,strKT))
		return 0;

	const size_t len = strKT.GetLength();

	if (Size && KeyText)
	{
		size_t minLen = std::min(len, Size - 1);
		wmemcpy(KeyText, strKT.CPtr(), minLen);
		KeyText[minLen] = 0;
	}
	else if (KeyText) *KeyText = 0;

	return (len+1);
}

static BOOL WINAPI FarNameToInputRecord(const wchar_t *Name,INPUT_RECORD* Rec)
{
	if (Name)
	{
		int VirtKey, ControlState;
		auto Key = KeyNameToKey(Name);
		return Key != KEY_INVALID && TranslateKeyToVK(Key,VirtKey,ControlState,Rec);
	}
	return FALSE;
}

FarKey WINAPI KeyNameToKeyW(const wchar_t *Name)
{
	return Name ? KeyNameToKey(Name) : KEY_INVALID;
}

PluginW::PluginW(PluginManager *owner, const FARString &strModuleName,
					const std::string &settingsName, const std::string &moduleID)
	:
	Plugin(owner, strModuleName, settingsName, moduleID)
{
	ClearExports();
}

PluginW::~PluginW()
{
}

bool PluginW::LoadFromCache()
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
	bUseMenuGuids = kfh.GetUInt(szCache_UseMenuGuids, 0) != 0;

	pAnalyseW = (PLUGINANALYSEW)(INT_PTR)kfh.GetUInt(szCache_Analyse, 0);
	pConfigureV3W = (PLUGINCONFIGUREV3W)(INT_PTR)kfh.GetUInt(szCache_ConfigureV3, 0);
	pConfigureW = (PLUGINCONFIGUREW)(INT_PTR)kfh.GetUInt(szCache_Configure, 0);
	pGetCustomDataW = (PLUGINGETCUSTOMDATAW)(INT_PTR)kfh.GetUInt(szCache_GetCustomData, 0);
	pGetFilesW = (PLUGINGETFILESW)(INT_PTR)kfh.GetUInt(szCache_GetFiles, 0);
	pOpenFilePluginW = (PLUGINOPENFILEPLUGINW)(INT_PTR)kfh.GetUInt(szCache_OpenFilePlugin, 0);
	pOpenPluginW = (PLUGINOPENPLUGINW)(INT_PTR)kfh.GetUInt(szCache_OpenPlugin, 0);
	pProcessConsoleInputW = (PLUGINPROCESSCONSOLEINPUTW)(INT_PTR)kfh.GetUInt(szCache_ProcessConsoleInput, 0);
	pProcessDialogEventW = (PLUGINPROCESSDIALOGEVENTW)(INT_PTR)kfh.GetUInt(szCache_ProcessDialogEvent, 0);
	pProcessEditorEventW = (PLUGINPROCESSEDITOREVENTW)(INT_PTR)kfh.GetUInt(szCache_ProcessEditorEvent, 0);
	pProcessEditorEventV3W = (PLUGINPROCESSEDITOREVENTV3W)(INT_PTR)kfh.GetUInt(szCache_ProcessEditorEventV3, 0);
	pProcessEditorInputW = (PLUGINPROCESSEDITORINPUTW)(INT_PTR)kfh.GetUInt(szCache_ProcessEditorInput, 0);
	pProcessHostFileW = (PLUGINPROCESSHOSTFILEW)(INT_PTR)kfh.GetUInt(szCache_ProcessHostFile, 0);
	pProcessSynchroEventW = (PLUGINPROCESSSYNCHROEVENTW)(INT_PTR)kfh.GetUInt(szCache_ProcessSynchroEvent, 0);
	pProcessViewerEventW = (PLUGINPROCESSVIEWEREVENTW)(INT_PTR)kfh.GetUInt(szCache_ProcessViewerEvent, 0);
	pSetFindListW = (PLUGINSETFINDLISTW)(INT_PTR)kfh.GetUInt(szCache_SetFindList, 0);

	WorkFlags.Set(PIWF_CACHED); //too much "cached" flags

	if (kfh.GetInt(szCache_Preopen) != 0)
		OpenModule();

	return true;
}

bool PluginW::SaveToCache()
{
	KeyFileHelper kfh(PluginsIni());
	kfh.RemoveSection(GetSettingsName());

	struct stat st{};
	const std::string &module = m_strModuleName.GetMB();
	if (stat(module.c_str(), &st) == -1)
	{
		fprintf(stderr, "%s: stat('%s') error %d\n",
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
		if (UseMenuGuids())
			kfh.SetString(GetSettingsName(), StrPrintf(FmtDiskMenuGuidD, i),
				GuidToString(Info.DiskMenuGuids[i]));
	}

	for (int i = 0; i < Info.PluginMenuStringsNumber; i++)
	{
		kfh.SetString(GetSettingsName(), StrPrintf(FmtPluginMenuStringD, i),
				Info.PluginMenuStrings[i]);
		if (UseMenuGuids())
			kfh.SetString(GetSettingsName(), StrPrintf(FmtPluginMenuGuidD, i),
				GuidToString(Info.PluginMenuGuids[i]));
	}

	for (int i = 0; i < Info.PluginConfigStringsNumber; i++)
	{
		kfh.SetString(GetSettingsName(), StrPrintf(FmtPluginConfigStringD, i),
				Info.PluginConfigStrings[i]);
		if (UseMenuGuids())
			kfh.SetString(GetSettingsName(), StrPrintf(FmtPluginConfigGuidD, i),
				GuidToString(Info.PluginConfigGuids[i]));
	}

	kfh.SetString(GetSettingsName(), "CommandPrefix", Info.CommandPrefix);
	kfh.SetUInt(GetSettingsName(), "Flags", Info.Flags);
	kfh.SetUInt(GetSettingsName(), szCache_SysID, SysID);

	kfh.SetUInt(GetSettingsName(), szCache_Analyse,             pAnalyseW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ConfigureV3,         pConfigureV3W ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_Configure,           pConfigureW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_GetCustomData,       pGetCustomDataW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_GetFiles,            pGetFilesW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_OpenFilePlugin,      pOpenFilePluginW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_OpenPlugin,          pOpenPluginW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessConsoleInput, pProcessConsoleInputW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessDialogEvent,  pProcessDialogEventW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessEditorEvent,  pProcessEditorEventW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessEditorEventV3, pProcessEditorEventV3W ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessEditorInput,  pProcessEditorInputW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessHostFile,     pProcessHostFileW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessSynchroEvent, pProcessSynchroEventW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessViewerEvent,  pProcessViewerEventW ? 1:0);
	kfh.SetUInt(GetSettingsName(), szCache_SetFindList,         pSetFindListW ? 1:0);

	kfh.SetString(GetSettingsName(), szCache_Author, strAuthor);
	kfh.SetString(GetSettingsName(), szCache_Description, strDescription);
	kfh.SetString(GetSettingsName(), szCache_Title, strTitle);
	kfh.SetUInt(GetSettingsName(),   szCache_UseMenuGuids, bUseMenuGuids ? 1:0);
	kfh.SetBytes(GetSettingsName(),  szCache_Version, (unsigned char*)&m_PlugVersion, sizeof(m_PlugVersion), 1);

	return true;
}

bool PluginW::Load()
{
	if (m_Loaded)
		return true;

	if (!OpenModule() || !GetGlobalInfo())
		return false;

	m_Loaded = true;

	WorkFlags.Clear(PIWF_CACHED);

	GetModuleFN(pAnalyseW, NFMP_Analyse);
	GetModuleFN(pCloseAnalyseW, NFMP_CloseAnalyse);
	GetModuleFN(pClosePluginW, NFMP_ClosePlugin);
	GetModuleFN(pCompareW, NFMP_Compare);
	GetModuleFN(pConfigureV3W, NFMP_ConfigureV3);
	GetModuleFN(pConfigureW, NFMP_Configure);
	GetModuleFN(pDeleteFilesW, NFMP_DeleteFiles);
	GetModuleFN(pExitFARW, NFMP_ExitFAR);
	GetModuleFN(pFreeCustomDataW, NFMP_FreeCustomData);
	GetModuleFN(pFreeFindDataW, NFMP_FreeFindData);
	GetModuleFN(pFreeVirtualFindDataW, NFMP_FreeVirtualFindData);
	GetModuleFN(pGetCustomDataW, NFMP_GetCustomData);
	GetModuleFN(pGetFilesW, NFMP_GetFiles);
	GetModuleFN(pGetFindDataW, NFMP_GetFindData);
	GetModuleFN(pGetLinkTargetW, NFMP_GetLinkTarget);
	GetModuleFN(pGetOpenPluginInfoW, NFMP_GetOpenPluginInfo);
	GetModuleFN(pGetPluginInfoW, NFMP_GetPluginInfo);
	GetModuleFN(pGetVirtualFindDataW, NFMP_GetVirtualFindData);
	GetModuleFN(pMakeDirectoryW, NFMP_MakeDirectory);
	GetModuleFN(pMayExitFARW, NFMP_MayExitFAR);
	GetModuleFN(pMinFarVersionW,NFMP_GetMinFarVersion);
	GetModuleFN(pOpenFilePluginW, NFMP_OpenFilePlugin);
	GetModuleFN(pOpenPluginW, NFMP_OpenPlugin);
	GetModuleFN(pProcessConsoleInputW, NFMP_ProcessConsoleInput);
	GetModuleFN(pProcessDialogEventW, NFMP_ProcessDialogEvent);
	GetModuleFN(pProcessEditorEventW, NFMP_ProcessEditorEvent);
	GetModuleFN(pProcessEditorEventV3W, NFMP_ProcessEditorEventV3);
	GetModuleFN(pProcessEditorInputW, NFMP_ProcessEditorInput);
	GetModuleFN(pProcessEventW, NFMP_ProcessEvent);
	GetModuleFN(pProcessHostFileW, NFMP_ProcessHostFile);
	GetModuleFN(pProcessKeyW, NFMP_ProcessKey);
	GetModuleFN(pProcessSynchroEventW, NFMP_ProcessSynchroEvent);
	GetModuleFN(pProcessViewerEventW, NFMP_ProcessViewerEvent);
	GetModuleFN(pPutFilesW, NFMP_PutFiles);
	GetModuleFN(pSetDirectoryW, NFMP_SetDirectory);
	GetModuleFN(pSetFindListW, NFMP_SetFindList);
	GetModuleFN(pSetStartupInfoW, NFMP_SetStartupInfo);

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

static int WINAPI farExecuteW(const wchar_t *CmdStr, unsigned int flags)
{
	return farExecuteA(Wide2MB(CmdStr).c_str(), flags);
}

static int WINAPI farExecuteLibraryW(const wchar_t *Library, const wchar_t *Symbol, const wchar_t *CmdStr, unsigned int flags)
{
	return farExecuteLibraryA(Wide2MB(Library).c_str(), Wide2MB(Symbol).c_str(), Wide2MB(CmdStr).c_str(), flags);
}

static void farDisplayNotificationW(const wchar_t *action, const wchar_t *object)
{
	DisplayNotification(action, object);
}

static int farDispatchInterThreadCallsW()
{
	return DispatchInterThreadCalls();
}

static void WINAPI farBackgroundTaskW(const wchar_t *Info, BOOL Started)
{
	if (Started)
		CtrlObject->Plugins.BackgroundTaskStarted(Info);
	else
		CtrlObject->Plugins.BackgroundTaskFinished(Info);
}

static size_t WINAPI farStrCellsCount(const wchar_t *Str, size_t CharsCount)
{
	return StrCellsCount(Str, CharsCount);
}

static size_t WINAPI farStrSizeOfCells(const wchar_t *Str, size_t CharsCount, size_t *CellsCount, BOOL RoundUp)
{
	return StrSizeOfCells(Str, CharsCount, *CellsCount, RoundUp != FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int WINAPI farESetFileMode(const wchar_t *Name, DWORD Mode, int SkipMode)
{
	return ESetFileMode(Name, Mode, SkipMode);
}

static int WINAPI farESetFileTime(const wchar_t *Name, FILETIME *AccessTime, FILETIME *ModifyTime, DWORD FileAttr, int SkipMode)
{
	return ESetFileTime(Name, AccessTime, ModifyTime, FileAttr, SkipMode);
}

static int WINAPI farESetFileGroup(const wchar_t *Name, const wchar_t *Group, int SkipMode)
{
	return ESetFileGroup(Name, Group, SkipMode);
}

static int WINAPI farESetFileOwner(const wchar_t *Name, const wchar_t *Owner, int SkipMode)
{
	return ESetFileOwner(Name, Owner, SkipMode);
}

static const char *WINAPI farOwnerNameByID(uid_t id)
{
	return OwnerNameByID(id);
}

static const char *WINAPI farGroupNameByID(uid_t id)
{
	return GroupNameByID(id);
}

static BOOL farGetFindData(const wchar_t *lpwszFileName, WIN32_FIND_DATAW *FindDataW)
{
	FAR_FIND_DATA_EX FindDataEx;

	if (!apiGetFindDataForExactPathName(lpwszFileName, FindDataEx))
		return FALSE;

	if (FindDataEx.strFileName.GetLength() >= MAX_NAME)
		return FALSE;

	FindDataW->ftCreationTime = FindDataEx.ftCreationTime;
	FindDataW->ftLastAccessTime = FindDataEx.ftLastAccessTime;
	FindDataW->ftLastWriteTime = FindDataEx.ftLastWriteTime;

	FindDataW->UnixOwner = FindDataEx.UnixOwner;
	FindDataW->UnixGroup = FindDataEx.UnixGroup;
	FindDataW->UnixDevice = FindDataEx.UnixDevice;
	FindDataW->UnixNode = FindDataEx.UnixNode;
	FindDataW->dwFileAttributes = FindDataEx.dwFileAttributes;
	FindDataW->nFileSize = FindDataEx.nFileSize;
	FindDataW->dwUnixMode = FindDataEx.dwUnixMode;
	FindDataW->nHardLinks = FindDataEx.nHardLinks;
	FindDataW->nBlockSize = FindDataEx.nBlockSize;

	memcpy(FindDataW->cFileName, FindDataEx.strFileName.GetBuffer(), sizeof(WCHAR) * (FindDataEx.strFileName.GetLength() + 1) );

	return TRUE;
}

static int WINAPI farGetDateFormat() {return GetDateFormat();}
static wchar_t WINAPI farGetDateSeparator() {return GetDateSeparator();}
static wchar_t WINAPI farGetTimeSeparator() {return GetTimeSeparator();}
static wchar_t WINAPI farGetDecimalSeparator() {return GetDecimalSeparator();}


static const MacroPrivateInfo MacroInfo
{
	sizeof(MacroPrivateInfo),
	farCallFar,
};

static BOOL LoadLuafar()
{
#if !defined(USELUA)
	return FALSE;
#elif defined(__ANDROID__)
	return TRUE;
#else

	// 1. Load Lua
#ifdef __APPLE__
	const char *ext = ".dylib";
	const char *paths[] = { "", "/opt/homebrew/lib/", nullptr };
#else
	const char *ext = ".so";
	const char *paths[] = { "", "/usr/local/lib/", "/usr/lib/", nullptr };
#endif

	const char *names[] = { "libluajit-5.1", "liblua5.1" }; // luajit is first by default
	const auto str = getenv("FARPLAINLUA");
	if (str && (0 == strcmp(str, "1") || 0 == strcasecmp(str, "yes")))
		std::swap(names[0], names[1]);

	void *handle = nullptr;
	for (size_t i=0; i < ARRAYSIZE(names) && !handle; i++)
	{
		for (const char **path=paths; *path && !handle; path++)
		{
			const auto strName = std::string(*path) + names[i] + ext;
			handle = dlopen(strName.c_str(), RTLD_LAZY|RTLD_GLOBAL);
		}
	}
	if (!handle)
	{
		Message(MSG_WARNING, 1, Msg::Error, L"Neither LuaJIT nor Lua5.1 library was found", Msg::Ok);
		return FALSE;
	}

	// 2. Load LuaFAR
	FARString strLuaFar = g_strFarPath + PluginsFolderName + L"/luafar/luafar.so";
	TranslateFarString<TranslateInstallPath_Share2Lib>(strLuaFar);
	if (!dlopen(strLuaFar.GetMB().c_str(), RTLD_LAZY|RTLD_GLOBAL))
	{
		Message(MSG_WARNING, 1, Msg::Error, L"Cannot load luafar.so", Msg::Ok);
		return FALSE;
	}
	return TRUE;
#endif // #if defined(USELUA)
}

void CreatePluginStartupInfo(Plugin *pPlugin, PluginStartupInfo *PSI, FarStandardFunctions *FSF)
{
	static PluginStartupInfo StartupInfo{};
	static FarStandardFunctions StandardFunctions{};

	// заполняем структуру StandardFunctions один раз!!!
	if (!StandardFunctions.StructSize)
	{
		StandardFunctions.StructSize = sizeof(StandardFunctions);
		StandardFunctions.AddEndSlash = AddEndSlash;
		StandardFunctions.atoi64 = FarAtoi64;
		StandardFunctions.atoi = FarAtoi;
		StandardFunctions.BackgroundTask = farBackgroundTaskW;
		StandardFunctions.BoxSymbols = BoxSymbols;
		StandardFunctions.bsearch = FarBsearch;
		StandardFunctions.ConvertPath = farConvertPath;
		StandardFunctions.CopyToClipboard = CopyToClipboard;
		StandardFunctions.DeleteBuffer = DeleteBuffer;
		StandardFunctions.DetectCodePage = farDetectCodePage;
		StandardFunctions.DispatchInterThreadCalls = farDispatchInterThreadCallsW;
		StandardFunctions.DisplayNotification = farDisplayNotificationW;
		StandardFunctions.Execute = farExecuteW;
		StandardFunctions.ExecuteLibrary = farExecuteLibraryW;
		StandardFunctions.FarInputRecordToKey = InputRecordToKey;
		StandardFunctions.FarKeyToName = FarKeyToName;
		StandardFunctions.FarNameToInputRecord = FarNameToInputRecord;
		StandardFunctions.FarNameToKey = KeyNameToKeyW;
		StandardFunctions.FarRecursiveSearch = FarRecursiveSearch;
		StandardFunctions.FormatFileSize = farFormatFileSize;
		StandardFunctions.GetCurrentDirectory = farGetCurrentDirectory;
		StandardFunctions.GetNumberOfLinks = GetNumberOfLinks;
		StandardFunctions.GetPathRoot = farGetPathRoot;
		StandardFunctions.GetReparsePointInfo = farGetReparsePointInfo;
		StandardFunctions.itoa64 = FarItoa64;
		StandardFunctions.itoa = FarItoa;
		StandardFunctions.LIsAlpha = farIsAlpha;
		StandardFunctions.LIsAlphanum = farIsAlphaNum;
		StandardFunctions.LIsLower = farIsLower;
		StandardFunctions.LIsUpper = farIsUpper;
		StandardFunctions.LLowerBuf = farLowerBuf;
		StandardFunctions.LLower = farLower;
		StandardFunctions.LStrcmp = farStrCmp;
		StandardFunctions.LStricmp = farStrCmpI;
		StandardFunctions.LStrlwr = farStrLower;
		StandardFunctions.LStrncmp = farStrCmpN;
		StandardFunctions.LStrnicmp = farStrCmpNI;
		StandardFunctions.LStrupr = farStrUpper;
		StandardFunctions.LTrim = RemoveLeadingSpaces;
		StandardFunctions.LUpperBuf = farUpperBuf;
		StandardFunctions.LUpper = farUpper;
		StandardFunctions.MkLink = FarMkLink;
		StandardFunctions.MkTemp = FarMkTemp;
		StandardFunctions.PasteFromClipboard = PasteFromClipboard;
		StandardFunctions.PointToName = PointToName;
		StandardFunctions.ProcessName = ProcessName;
		StandardFunctions.qsortex = FarQsortEx;
		StandardFunctions.qsort = FarQsort;
		StandardFunctions.QuoteSpaceOnly = QuoteSpaceOnly;
		StandardFunctions.RTrim = RemoveTrailingSpaces;
		StandardFunctions.snprintf = swprintf;
		StandardFunctions.sscanf = swscanf;
		StandardFunctions.StrCellsCount = farStrCellsCount;
		StandardFunctions.StrSizeOfCells = farStrSizeOfCells;
		StandardFunctions.Trim = RemoveExternalSpaces;
		StandardFunctions.TruncPathStr = TruncPathStr;
		StandardFunctions.TruncStr = TruncStr;
		StandardFunctions.Unquote = Unquote;
		StandardFunctions.VTEnumBackground = farAPIVTEnumBackground;
		StandardFunctions.VTLogExport = farAPIVTLogExportW;
		StandardFunctions.XLat = Xlat;

		StandardFunctions.ESetFileGroup = farESetFileGroup;
		StandardFunctions.ESetFileMode = farESetFileMode;
		StandardFunctions.ESetFileOwner = farESetFileOwner;
		StandardFunctions.ESetFileTime = farESetFileTime;
		StandardFunctions.GetDateFormat = farGetDateFormat;
		StandardFunctions.GetDateSeparator = farGetDateSeparator;
		StandardFunctions.GetDecimalSeparator = farGetDecimalSeparator;
		StandardFunctions.GetFileGroup = farGetFileGroup;
		StandardFunctions.GetFileOwner = farGetFileOwner;
		StandardFunctions.GetFindData = farGetFindData;
		StandardFunctions.GetTimeSeparator = farGetTimeSeparator;
		StandardFunctions.GroupNameByID = farGroupNameByID;
		StandardFunctions.OwnerNameByID = farOwnerNameByID;
	}

	if (!StartupInfo.StructSize)
	{
		StartupInfo.StructSize = sizeof(StartupInfo);
		StartupInfo.AdvControlAsync = FarAdvControlAsync;
		StartupInfo.AdvControl = FarAdvControl;
		StartupInfo.CmpName = FarCmpName;
		StartupInfo.ColorDialog = FarColorDialog;
		StartupInfo.ColorDialogV2 = FarColorDialogV2;
		StartupInfo.Control = FarControl;
		StartupInfo.DefDlgProc = FarDefDlgProc;
		StartupInfo.DialogFree = FarDialogFree;
		StartupInfo.DialogInit = FarDialogInit;
		StartupInfo.DialogInitV3 = FarDialogInitV3;
		StartupInfo.DialogRun = FarDialogRun;
		StartupInfo.EditorControl = FarEditorControl;
		StartupInfo.EditorControlV2 = FarEditorControlV2;
		StartupInfo.Editor = FarEditor;
		StartupInfo.FileFilterControl = farFileFilterControl;
		StartupInfo.FillText = FarFillText;
		StartupInfo.FreeDirList = FarFreeDirList;
		StartupInfo.FreePluginDirList = FarFreePluginDirList;
		StartupInfo.FreeScreen = FarFreeScreen;
		StartupInfo.GetDirList = FarGetDirList;
		StartupInfo.GetMsg = FarGetMsgFn;
		StartupInfo.GetPluginDirList = FarGetPluginDirList;
		StartupInfo.InputBox = FarInputBox;
		StartupInfo.InputBoxV3 = FarInputBoxV3;
		StartupInfo.MacroControl = farMacroControl;
		StartupInfo.Menu = FarMenuFn;
		StartupInfo.MenuV2 = FarMenuV2Fn;
		StartupInfo.Message = FarMessageFn;
		StartupInfo.MessageV3 = FarMessageV3Fn;
		StartupInfo.PluginsControl = farPluginsControl;
		StartupInfo.PluginsControlV3 = farPluginsControlV3;
		StartupInfo.RegExpControl = farRegExpControl;
		StartupInfo.RestoreScreen = FarRestoreScreen;
		StartupInfo.SaveScreen = FarSaveScreen;
		StartupInfo.SendDlgMessage = FarSendDlgMessage;
		StartupInfo.ShowHelp = FarShowHelp;
		StartupInfo.Text = FarText;
		StartupInfo.TextV2 = FarTextV2;
		StartupInfo.ViewerControl = FarViewerControl;
		StartupInfo.ViewerControlV2 = FarViewerControlV2;
		StartupInfo.Viewer = FarViewer;

		StartupInfo.LuafarLoaded = LoadLuafar();
	}

	*PSI = StartupInfo;
	*FSF = StandardFunctions;
	PSI->FSF = FSF;
	PSI->RootKey = L"";
	PSI->ModuleNumber = (INT_PTR)pPlugin;
	PSI->ModuleName = pPlugin->GetModuleName().CPtr();
	if (pPlugin->IsLuamacro()) {
		PSI->Private = &MacroInfo;
	}
}

bool PluginW::SetStartupInfo(bool &bUnloaded)
{
	if (pSetStartupInfoW)
	{
		PluginStartupInfo _info;
		FarStandardFunctions _fsf;
		CreatePluginStartupInfo(this, &_info, &_fsf);
		ExecuteStruct es(EXCEPT_SETSTARTUPINFO);
		EXECUTE_FUNCTION(pSetStartupInfoW(&_info), es);

		if (es.bUnloaded)
		{
			bUnloaded = true;
			return false;
		}
	}

	return true;
}

bool PluginW::CheckMinFarVersion(bool &bUnloaded)
{
	if (pMinFarVersionW)
	{
		ExecuteStruct es(EXCEPT_MINFARVERSION);
		EXECUTE_FUNCTION_EX(pMinFarVersionW(), es);

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

int PluginW::Unload(bool bExitFAR)
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

bool PluginW::IsPanelPlugin()
{
	return pSetFindListW ||
	       pGetFindDataW ||
	       pGetVirtualFindDataW ||
	       pSetDirectoryW ||
	       pGetFilesW ||
	       pPutFilesW ||
	       pDeleteFilesW ||
	       pMakeDirectoryW ||
	       pProcessHostFileW ||
	       pProcessKeyW ||
	       pProcessEventW ||
	       pCompareW ||
	       pGetOpenPluginInfoW ||
	       pFreeFindDataW ||
	       pFreeVirtualFindDataW ||
	       pClosePluginW;
}

HANDLE PluginW::Analyse(const AnalyseInfo *Info)
{
	if (Load() && pAnalyseW)
	{
		ExecuteStruct es(EXCEPT_ANALYSE);
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		es.hResult = INVALID_HANDLE_VALUE;
		EXECUTE_FUNCTION_EX(pAnalyseW(Info), es);
		return es.hResult;
	}
	return INVALID_HANDLE_VALUE;
}

void PluginW::CloseAnalyse(const CloseAnalyseInfo *Info)
{
	if (Load() && pCloseAnalyseW)
	{
		ExecuteStruct es(EXCEPT_CLOSEANALYSE);
		EXECUTE_FUNCTION(pCloseAnalyseW(Info), es);
	}
}

HANDLE PluginW::OpenPlugin(int OpenFrom, const void *Item)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);

	g_strDirToSet.Clear();

	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (Load() && pOpenPluginW)
	{
		ExecuteStruct es(EXCEPT_OPENPLUGIN);
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		es.hResult = INVALID_HANDLE_VALUE;
		EXECUTE_FUNCTION_EX(pOpenPluginW(OpenFrom, (INT_PTR)Item), es);
		hResult = es.hResult;
	}

	return hResult;
}

HANDLE PluginW::OpenFilePlugin(
    const wchar_t *Name,
    const unsigned char *Data,
    int DataSize,
    int OpMode
)
{
	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (Load() && pOpenFilePluginW)
	{
		ExecuteStruct es(EXCEPT_OPENFILEPLUGIN);
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		EXECUTE_FUNCTION_EX(pOpenFilePluginW(Name, Data, DataSize, OpMode), es);
		hResult = es.hResult;
	}

	return hResult;
}


int PluginW::SetFindList(
    HANDLE hPlugin,
    const PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	BOOL bResult = FALSE;

	if (pSetFindListW)
	{
		ExecuteStruct es(EXCEPT_SETFINDLIST);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pSetFindListW(hPlugin, PanelItem, ItemsNumber), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginW::ProcessEditorInput(const INPUT_RECORD *D)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessEditorInputW)
	{
		ExecuteStruct es(EXCEPT_PROCESSEDITORINPUT);
		es.bDefaultResult = TRUE; //(TRUE) treat the result as a completed request on exception!
		EXECUTE_FUNCTION_BOOL(pProcessEditorInputW(D), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginW::ProcessEditorEvent(int Event, void *Param)
{
	if (Load() && pProcessEditorEventW)
	{
		ExecuteStruct es(EXCEPT_PROCESSEDITOREVENT);
		EXECUTE_FUNCTION_EX(pProcessEditorEventW(Event, Param), es);
	}

	return 0;
}

int PluginW::ProcessEditorEventV3(const ProcessEditorEventInfo *Info)
{
	if (Load() && pProcessEditorEventV3W)
	{
		ExecuteStruct es(EXCEPT_PROCESSEDITOREVENTV3);
		EXECUTE_FUNCTION_EX(pProcessEditorEventV3W(Info), es);
	}

	return 0;
}

int PluginW::ProcessViewerEvent(int Event, void *Param)
{
	if (Load() && pProcessViewerEventW)
	{
		ExecuteStruct es(EXCEPT_PROCESSVIEWEREVENT);
		EXECUTE_FUNCTION_EX(pProcessViewerEventW(Event, Param), es);
	}

	return 0; //oops, again!
}

int PluginW::ProcessDialogEvent(int Event, void *Param)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessDialogEventW)
	{
		ExecuteStruct es(EXCEPT_PROCESSDIALOGEVENT);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pProcessDialogEventW(Event, Param), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginW::ProcessSynchroEvent(int Event, void *Param)
{
	if (Load() && pProcessSynchroEventW)
	{
		ExecuteStruct es(EXCEPT_PROCESSSYNCHROEVENT);
		EXECUTE_FUNCTION_EX(pProcessSynchroEventW(Event, Param), es);
	}

	return 0; //oops, again!
}

int PluginW::GetVirtualFindData(
    HANDLE hPlugin,
    PluginPanelItem **pPanelItem,
    int *pItemsNumber,
    const wchar_t *Path
)
{
	BOOL bResult = FALSE;

	if (pGetVirtualFindDataW)
	{
		ExecuteStruct es(EXCEPT_GETVIRTUALFINDDATA);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pGetVirtualFindDataW(hPlugin, pPanelItem, pItemsNumber, Path), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginW::FreeVirtualFindData(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	if (pFreeVirtualFindDataW)
	{
		ExecuteStruct es(EXCEPT_FREEVIRTUALFINDDATA);
		EXECUTE_FUNCTION(pFreeVirtualFindDataW(hPlugin, PanelItem, ItemsNumber), es);
	}
}

bool PluginW::GetLinkTarget(HANDLE hPlugin, PluginPanelItem *PanelItem, FARString &result, int OpMode)
{
	if (!pGetLinkTargetW) {
		return false;
	}
	ExecuteStruct es(EXCEPT_GETLINKTARGET);
	es.nDefaultResult = -1;
	wchar_t buf[MAX_PATH + 1] = {0};
	EXECUTE_FUNCTION_EX(pGetLinkTargetW(hPlugin, PanelItem, buf, ARRAYSIZE(buf), OpMode), es);
	if (es.nResult <= 0 || size_t(es.nResult) > ARRAYSIZE(buf)) {
		return false;
	}
	result = buf;
	return true;
}

int PluginW::GetFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    const wchar_t **DestPath,
    int OpMode
)
{
	int nResult = -1;

	if (pGetFilesW)
	{
		ExecuteStruct es(EXCEPT_GETFILES);
		es.nDefaultResult = -1;
		EXECUTE_FUNCTION_EX(pGetFilesW(hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginW::PutFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    int OpMode
)
{
	int nResult = -1;

	if (pPutFilesW)
	{
		ExecuteStruct es(EXCEPT_PUTFILES);
		es.nDefaultResult = -1;
		static FARString strCurrentDirectory;
		apiGetCurrentDirectory(strCurrentDirectory);
		EXECUTE_FUNCTION_EX(pPutFilesW(hPlugin, PanelItem, ItemsNumber, Move, strCurrentDirectory, OpMode), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}

int PluginW::DeleteFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pDeleteFilesW)
	{
		ExecuteStruct es(EXCEPT_DELETEFILES);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pDeleteFilesW(hPlugin, PanelItem, ItemsNumber, OpMode), es);
		bResult = (int)es.bResult;
	}

	return bResult;
}


int PluginW::MakeDirectory(HANDLE hPlugin, const wchar_t **Name, int OpMode)
{
	int nResult = -1;

	if (pMakeDirectoryW)
	{
		ExecuteStruct es(EXCEPT_MAKEDIRECTORY);
		es.nDefaultResult = -1;
		EXECUTE_FUNCTION_EX(pMakeDirectoryW(hPlugin, Name, OpMode), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginW::ProcessHostFile(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pProcessHostFileW)
	{
		ExecuteStruct es(EXCEPT_PROCESSHOSTFILE);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pProcessHostFileW(hPlugin, PanelItem, ItemsNumber, OpMode), es);
		bResult = es.bResult;
	}

	return bResult;
}


int PluginW::ProcessEvent(HANDLE hPlugin, int Event, void *Param)
{
	BOOL bResult = FALSE;

	if (pProcessEventW)
	{
		ExecuteStruct es(EXCEPT_PROCESSEVENT);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pProcessEventW(hPlugin, Event, Param), es);
		bResult = es.bResult;
	}

	return bResult;
}


int PluginW::Compare(
    HANDLE hPlugin,
    const PluginPanelItem *Item1,
    const PluginPanelItem *Item2,
    DWORD Mode
)
{
	int nResult = -2;

	if (pCompareW)
	{
		ExecuteStruct es(EXCEPT_COMPARE);
		es.nDefaultResult = -2;
		EXECUTE_FUNCTION_EX(pCompareW(hPlugin, Item1, Item2, Mode), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginW::GetFindData(
    HANDLE hPlugin,
    PluginPanelItem **pPanelItem,
    int *pItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pGetFindDataW)
	{
		ExecuteStruct es(EXCEPT_GETFINDDATA);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pGetFindDataW(hPlugin, pPanelItem, pItemsNumber, OpMode), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginW::FreeFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber)
{
	if (pFreeFindDataW)
	{
		ExecuteStruct es(EXCEPT_FREEFINDDATA);
		EXECUTE_FUNCTION(pFreeFindDataW(hPlugin, PanelItem, ItemsNumber), es);
	}
}

int PluginW::ProcessKey(HANDLE hPlugin, int Key, unsigned int dwControlState)
{
	BOOL bResult = FALSE;

	if (pProcessKeyW)
	{
		ExecuteStruct es(EXCEPT_PROCESSKEY);
		es.bDefaultResult = TRUE; // do not pass this key to far on exception
		EXECUTE_FUNCTION_BOOL(pProcessKeyW(hPlugin, Key, dwControlState), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginW::ClosePlugin(HANDLE hPlugin)
{
	if (pClosePluginW)
	{
		ExecuteStruct es(EXCEPT_CLOSEPLUGIN);
		EXECUTE_FUNCTION(pClosePluginW(hPlugin), es);
	}

//	m_pManager->m_pCurrentPlugin = (Plugin*)-1;
}


int PluginW::SetDirectory(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
	BOOL bResult = FALSE;

	if (pSetDirectoryW)
	{
		ExecuteStruct es(EXCEPT_SETDIRECTORY);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pSetDirectoryW(hPlugin, Dir, OpMode), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginW::GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *pInfo)
{
//	m_pManager->m_pCurrentPlugin = this;
	pInfo->StructSize = sizeof(OpenPluginInfo);

	if (pGetOpenPluginInfoW)
	{
		ExecuteStruct es(EXCEPT_GETOPENPLUGININFO);
		EXECUTE_FUNCTION(pGetOpenPluginInfoW(hPlugin, pInfo), es);
	}
}


int PluginW::Configure(int MenuItem)
{
	BOOL bResult = FALSE;

	if (Load() && pConfigureW)
	{
		ExecuteStruct es(EXCEPT_CONFIGURE);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pConfigureW(MenuItem), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginW::ConfigureV3(const ConfigureInfo *Info)
{
	BOOL bResult = FALSE;

	if (Load() && pConfigureV3W)
	{
		ExecuteStruct es(EXCEPT_CONFIGUREV3);
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_BOOL(pConfigureV3W(Info), es);
		bResult = es.bResult;
	}
	return bResult;
}


bool PluginW::GetPluginInfo(PluginInfo *pi)
{
	memset(pi, 0, sizeof(PluginInfo));

	if (pGetPluginInfoW)
	{
		ExecuteStruct es(EXCEPT_GETPLUGININFO);
		EXECUTE_FUNCTION(pGetPluginInfoW(pi), es);

		if (!es.bUnloaded)
		{
			if (pi->SysID == 0) // prevent erasing SysID that may be already set by GetGlobalInfoW()
				pi->SysID = SysID;
			return true;
		}
	}

	return false;
}

int PluginW::GetCustomData(const wchar_t *FilePath, wchar_t **CustomData)
{
	if (Load() && pGetCustomDataW)
	{
		ExecuteStruct es(EXCEPT_GETCUSTOMDATA);
		es.bDefaultResult = 0;
		es.bResult = 0;
		EXECUTE_FUNCTION_BOOL(pGetCustomDataW(FilePath, CustomData), es);
		return es.bResult;
	}

	return 0;
}

void PluginW::FreeCustomData(wchar_t *CustomData)
{
	if (Load() && pFreeCustomDataW)
	{
		ExecuteStruct es(EXCEPT_FREECUSTOMDATA);
		EXECUTE_FUNCTION(pFreeCustomDataW(CustomData), es);
	}
}

bool PluginW::MayExitFAR()
{
	if (pMayExitFARW)
	{
		ExecuteStruct es(EXCEPT_MAYEXITFAR);
		es.bDefaultResult = 1;
		EXECUTE_FUNCTION_BOOL(pMayExitFARW(), es);
		return es.bResult;
	}

	return true;
}

void PluginW::ExitFAR()
{
	if (pExitFARW)
	{
		ExecuteStruct es(EXCEPT_EXITFAR);
		EXECUTE_FUNCTION(pExitFARW(), es);
	}
}

int PluginW::ProcessConsoleInput(INPUT_RECORD *D)
{
	int bResult = 0;

	if (Load() && pProcessConsoleInputW)
	{
		ExecuteStruct es(EXCEPT_PROCESSCONSOLEINPUT);
		es.bDefaultResult = 0;
		EXECUTE_FUNCTION_BOOL(pProcessConsoleInputW(D), es);
		bResult = es.bResult;
	}

	return bResult;
}

void PluginW::ClearExports()
{
	pAnalyseW = nullptr;
	pCloseAnalyseW = nullptr;
	pClosePluginW = nullptr;
	pCompareW = nullptr;
	pConfigureV3W = nullptr;
	pConfigureW = nullptr;
	pDeleteFilesW = nullptr;
	pExitFARW = nullptr;
	pFreeCustomDataW = nullptr;
	pFreeFindDataW = nullptr;
	pFreeVirtualFindDataW = nullptr;
	pGetCustomDataW = nullptr;
	pGetFilesW = nullptr;
	pGetFindDataW = nullptr;
	pGetGlobalInfoW = nullptr;
	pGetLinkTargetW = nullptr;
	pGetOpenPluginInfoW = nullptr;
	pGetPluginInfoW = nullptr;
	pGetVirtualFindDataW = nullptr;
	pMakeDirectoryW = nullptr;
	pMayExitFARW = nullptr;
	pMinFarVersionW = nullptr;
	pOpenFilePluginW = nullptr;
	pOpenPluginW = nullptr;
	pProcessConsoleInputW = nullptr;
	pProcessDialogEventW = nullptr;
	pProcessEditorEventW = nullptr;
	pProcessEditorEventV3W = nullptr;
	pProcessEditorInputW = nullptr;
	pProcessEventW = nullptr;
	pProcessHostFileW = nullptr;
	pProcessKeyW = nullptr;
	pProcessSynchroEventW = nullptr;
	pProcessViewerEventW = nullptr;
	pPutFilesW = nullptr;
	pSetDirectoryW = nullptr;
	pSetFindListW = nullptr;
	pSetStartupInfoW = nullptr;
}

