/*
plugins.cpp

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

#include "headers.hpp"

#include "plugins.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "scantree.hpp"
#include "chgprior.hpp"
#include "chgmmode.hpp"
#include "constitle.hpp"
#include "cmdline.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "udlist.hpp"
#include "fileedit.hpp"
#include "RefreshFrameManager.hpp"
#include "plugapi.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "filelist.hpp"
#include "message.hpp"
#include "SafeMMap.hpp"
#include "HotkeyLetterDialog.hpp"
#include "InterThreadCall.hpp"
#include "DlgGuid.hpp"
#include <KeyFileHelper.h>
#include "DialogBuilder.hpp"
#include <assert.h>

const char *FmtDiskMenuStringD = "DiskMenuString%d";
const char *FmtPluginMenuStringD = "PluginMenuString%d";
const char *FmtPluginConfigStringD = "PluginConfigString%d";
const char *FmtDiskMenuGuidD = "DiskMenuGuid%d";
const char *FmtPluginMenuGuidD = "PluginMenuGuid%d";
const char *FmtPluginConfigGuidD = "PluginConfigGuid%d";
const wchar_t *PluginsFolderName = L"Plugins";

static const char *HotkeysSection = "Settings"; //don't change (used with both old and new far2m's)

enum PluginType
{
	NOT_PLUGIN = 0,
	WIDE_PLUGIN,
	MULTIBYTE_PLUGIN
};

static const char *HotKeyType(int Type)
{
	switch(Type)
	{
		default:
		case MTYPE_COMMANDSMENU: return "Hotkey";
		case MTYPE_CONFIGSMENU:  return "ConfHotkey";
		case MTYPE_DISKSMENU:    return "DriveMenuHotkey";
	}
}

static const GUID *MenuItemGuids(int Type, const PluginInfo *Info)
{
	switch(Type)
	{
		default:
		case MTYPE_COMMANDSMENU: return Info->PluginMenuGuids;
		case MTYPE_CONFIGSMENU:  return Info->PluginConfigGuids;
		case MTYPE_DISKSMENU:    return Info->DiskMenuGuids;
	}
}

const char *PluginsIni()
{
	static std::string s_out(InMyConfig("plugins/state.ini"));
	return s_out.c_str();
}

// Return string used as ini file key that represents given
// plugin object file. To reduce overhead encode less meaningful
// components like file path and extension as CRC suffix, leaded
// by actual plugin file name.
// If plugins resided in a path that is nested under g_strFarPath
// then dismiss g_strFarPath from CRC to make result invariant to
// whole package relocation.
static std::string PluginSettingsName(const FARString &strModuleName)
{
	std::string pathname;

	const size_t FarPathLength = g_strFarPath.GetLength();
	if (FarPathLength < strModuleName.GetLength()
	  && !StrCmpNI(strModuleName, g_strFarPath, (int)FarPathLength))
	{
		Wide2MB(strModuleName.CPtr() + FarPathLength, pathname);
	}
	else
	{
		Wide2MB(strModuleName.CPtr(), pathname);
	}

	FilePathHashSuffix(pathname);

	return pathname;
}

static PluginType PluginTypeByExtension(const wchar_t *lpModuleName)
{
	const wchar_t *ext = wcsrchr(lpModuleName, L'.');
	if (ext) {
		if (wcscmp(ext, L".far-plug-wide")==0)
			return WIDE_PLUGIN;
		if (wcscmp(ext, L".far-plug-mb")==0)
			return MULTIBYTE_PLUGIN;
	}

	return NOT_PLUGIN;
}

static int _cdecl PluginsSort(const void *el1,const void *el2)
{
	Plugin *Plugin1=*((Plugin**)el1);
	Plugin *Plugin2=*((Plugin**)el2);
	return StrCmp(PointToName(Plugin1->GetModuleName()),PointToName(Plugin2->GetModuleName()));
}

PluginManager::PluginManager():
	PluginsData(nullptr),
	PluginsCount(0),
	CurEditor(nullptr),
	CurViewer(nullptr)
{
}

PluginManager::~PluginManager()
{
	Plugin *pLuaMacro = nullptr; //to be deleted last

	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];
		if (pPlugin->IsLuamacro())
			pLuaMacro = pPlugin;
		else
		{
			pPlugin->Unload(true);
			delete pPlugin;
		}
	}
	if (pLuaMacro)
	{
		pLuaMacro->Unload(true);
		delete pLuaMacro;
	}
	free(PluginsData);
}

bool PluginManager::AddPlugin(Plugin *pPlugin)
{
	Plugin **NewPluginsData=(Plugin**)realloc(PluginsData,sizeof(*PluginsData)*(PluginsCount+1));

	if (!NewPluginsData)
		return false;

	PluginsData = NewPluginsData;
	PluginsData[PluginsCount]=pPlugin;
	PluginsCount++;
	return true;
}

bool PluginManager::RemovePlugin(Plugin *pPlugin)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		if (PluginsData[i] == pPlugin)
		{
			SysIdMap.erase(pPlugin->SysID);
			delete pPlugin;
			memmove(&PluginsData[i], &PluginsData[i+1], (PluginsCount-i-1)*sizeof(Plugin*));
			PluginsCount--;
			return true;
		}
	}

	return false;
}


Plugin* PluginManager::LoadPlugin(const FARString &strModuleName, bool UncachedLoad)
{
	const PluginType PlType = PluginTypeByExtension(strModuleName);

	if (PlType == NOT_PLUGIN)
		return nullptr;

	struct stat st{};
	if (stat(strModuleName.GetMB().c_str(), &st) == -1)
	{
		fprintf(stderr, "%s: stat error %u for '%ls'\n",
			__FUNCTION__, errno, strModuleName.CPtr());
		return nullptr;
	}

	const std::string &SettingsName = PluginSettingsName(strModuleName);
	const std::string &ModuleID = StrPrintf("%llx.%llx.%llx.%llx",
							(unsigned long long)st.st_ino, (unsigned long long)st.st_size,
							(unsigned long long)st.st_mtime, (unsigned long long)st.st_ctime);

	Plugin *pPlugin = nullptr;

	switch (PlType)
	{
		case WIDE_PLUGIN:
			pPlugin = new(std::nothrow) PluginW(this, strModuleName, SettingsName, ModuleID);
			break;
		case MULTIBYTE_PLUGIN:
			pPlugin = new(std::nothrow) PluginA(this, strModuleName, SettingsName, ModuleID);
			break;
		default:
			break;
	}

	if (!pPlugin)
		return nullptr;

	if (!AddPlugin(pPlugin))
	{
		delete pPlugin;
		return nullptr;
	}

	bool bResult = false;

	if (!UncachedLoad)
	{
		bResult = pPlugin->LoadFromCache();
		fprintf(stderr, "%s: cache %s for '%ls'\n",
			__FUNCTION__, bResult ? "hit" : "miss", strModuleName.CPtr());
	}

	if (!bResult && !Opt.LoadPlug.PluginsCacheOnly)
	{
		bResult = pPlugin->Load();

		if (!bResult)
			RemovePlugin(pPlugin);
	}

	if (bResult && pPlugin->SysID) {
		SysIdMap.emplace(pPlugin->SysID, pPlugin);
	}

	return bResult ? pPlugin : nullptr;
}

bool PluginManager::CacheForget(const wchar_t *lpwszModuleName)
{
	KeyFileHelper kfh(PluginsIni());
	const std::string &SettingsName = PluginSettingsName(lpwszModuleName);
	if (!kfh.RemoveSection(SettingsName))
	{
		fprintf(stderr, "%s: nothing to forget for '%ls'\n", __FUNCTION__, lpwszModuleName);
		return false;
	}
	fprintf(stderr, "%s: forgotten - '%ls'\n", __FUNCTION__, lpwszModuleName);
	return true;
}

Plugin* PluginManager::LoadPluginExternal(const wchar_t *lpwszModuleName, bool LoadToMem)
{
	Plugin *pPlugin = FindPlugin(lpwszModuleName);

	if (pPlugin)
	{
		if (LoadToMem && !pPlugin->Load())
		{
			RemovePlugin(pPlugin);
			return nullptr;
		}
	}
	else
	{
		pPlugin = LoadPlugin(lpwszModuleName, LoadToMem);
		if (pPlugin)
			far_qsort(PluginsData, PluginsCount, sizeof(*PluginsData), PluginsSort);
	}
	return pPlugin;
}

int PluginManager::UnloadPlugin(Plugin *pPlugin, DWORD dwException, bool bRemove)
{
	int nResult = FALSE;

	if (pPlugin && (dwException != EXCEPT_EXITFAR))   //схитрим, если упали в EXITFAR, не полезем в рекурсию, мы и так в Unload
	{
		if (auto frame = FrameManager->GetBottomFrame())
			frame->Unlock();

		if (Flags.Check(PSIF_DIALOG))   // BugZ#52 exception handling for floating point incorrect
		{
			Flags.Clear(PSIF_DIALOG);
			FrameManager->DeleteFrame();
			FrameManager->Commit();
		}

		bool bPanelPlugin = pPlugin->IsPanelPlugin();

		nResult = pPlugin->Unload(dwException != (DWORD)-1);

		if (bPanelPlugin /*&& bUpdatePanels*/)
		{
			CtrlObject->Cp()->ActivePanel->SetCurDir(L".",true);
			Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
			ActivePanel->Update(UPDATE_KEEP_SELECTION);
			ActivePanel->Redraw();
			Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(ActivePanel);
			AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
			AnotherPanel->Redraw();
		}

		if (bRemove)
			RemovePlugin(pPlugin);
	}

	return nResult;
}

int PluginManager::UnloadPluginExternal(const wchar_t *lpwszModuleName)
{
//BUGBUG нужны проверки на легальность выгрузки
	Plugin *pPlugin = FindPlugin(lpwszModuleName);

	if (pPlugin)
	{
		int nResult = pPlugin->Unload(true);
		RemovePlugin(pPlugin);
		return nResult;
	}
	return FALSE;
}

int PluginManager::UnloadPluginExternal(Plugin* pPlugin)
{
	if (FindPlugin(pPlugin))
	{
		int nResult = pPlugin->Unload(true);
		RemovePlugin(pPlugin);
		return nResult;
	}
	return FALSE;
}

Plugin *PluginManager::FindPlugin(const wchar_t *lpwszModuleName)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (!StrCmp(lpwszModuleName, pPlugin->GetModuleName()))
			return pPlugin;
	}

	return nullptr;
}

bool PluginManager::FindPlugin(Plugin *pPlugin)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		if (pPlugin == PluginsData[i])
			return true;
	}
	return false;
}

Plugin *PluginManager::GetPlugin(int PluginNumber)
{
	if (PluginNumber < PluginsCount && PluginNumber >= 0)
		return PluginsData[PluginNumber];

	return nullptr;
}

void PluginManager::LoadPlugins()
{
	Flags.Clear(PSIF_PLUGINSLOADED);

	if (Opt.LoadPlug.PluginsCacheOnly)  // $ 01.09.2000 tran  '/co' switch
	{
		LoadPluginsFromCache();
	}
	else if (Opt.LoadPlug.MainPluginDir
			|| !Opt.LoadPlug.strCustomPluginsPath.IsEmpty()
			|| (Opt.LoadPlug.PluginsPersonal && !Opt.LoadPlug.strPersonalPluginsPath.IsEmpty()))
	{
		ScanTree ScTree(FALSE,TRUE,Opt.LoadPlug.ScanSymlinks);
		UserDefinedList PluginPathList(ULF_UNIQUE | ULF_CASESENSITIVE, L":");  // хранение списка каталогов
		FARString strPluginsDir;
		FARString strFullName;
		FAR_FIND_DATA_EX FindData;

		// сначала подготовим список
		if (Opt.LoadPlug.MainPluginDir) // только основные и персональные?
		{
			strPluginsDir = g_strFarPath + PluginsFolderName;
			PluginPathList.AddItem(strPluginsDir);

			if (TranslateFarString<TranslateInstallPath_Share2Lib>(strPluginsDir)) {
				PluginPathList.AddItem(strPluginsDir);
			}

			// ...а персональные есть?
			if (Opt.LoadPlug.PluginsPersonal
					&& !Opt.LoadPlug.strPersonalPluginsPath.IsEmpty())
			{
				PluginPathList.AddItem(Opt.LoadPlug.strPersonalPluginsPath);
			}
		}
		else if (!Opt.LoadPlug.strCustomPluginsPath.IsEmpty())  // только "заказные" пути?
		{
			PluginPathList.AddItem(Opt.LoadPlug.strCustomPluginsPath);
		}

		const wchar_t *NamePtr;

		// теперь пройдемся по всему ранее собранному списку
		for (size_t I = 0; (NamePtr = PluginPathList.Get(I)); ++I)
		{
			// расширяем значение пути
			apiExpandEnvironmentStrings(NamePtr,strFullName);
			Unquote(strFullName); //??? здесь ХЗ

			if (!IsAbsolutePath(strFullName))
			{
				strPluginsDir = g_strFarPath;
				strPluginsDir += strFullName;
				strFullName = strPluginsDir;
			}

			// Получим реальное значение полного длинного пути
			ConvertNameToFull(strFullName,strFullName);
			strPluginsDir = strFullName;

			if (strPluginsDir.IsEmpty())  // Хмм... а нужно ли ЭТО условие после такой модернизации алгоритма загрузки?
				continue;

			// ставим на поток очередной путь из списка...
			ScTree.SetFindPath(strPluginsDir,L"*.far-plug-*", 0);

			// ...и пройдемся по нему
			while (ScTree.GetNextName(&FindData,strFullName))
			{
				if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					// this will check filename extension
					LoadPlugin(strFullName, false);
				}
			} // end while
		}
	}

	Flags.Set(PSIF_PLUGINSLOADED);

	far_qsort(PluginsData, PluginsCount, sizeof(*PluginsData), PluginsSort);
}

/* $ 01.09.2000 tran
   Load cache only plugins  - '/co' switch */
void PluginManager::LoadPluginsFromCache()
{
	KeyFileReadHelper kfh(PluginsIni());
	FARString strModuleName;
	for (const auto &s : kfh.EnumSections())
	{
		if (s != HotkeysSection)
		{
			const std::string &module = kfh.GetString(s, "Module");
			if (!module.empty()) {
				strModuleName = module;
				LoadPlugin(strModuleName, false);
			}
		}
	}
}

PHPTR PluginManager::OpenFilePlugin(const wchar_t *FileName, int OpMode, OPENFILEPLUGINTYPE Type,
		Plugin *pDesiredPlugin)
{
	struct CallResult
	{
		HANDLE Handle;
		Plugin *pPlugin;
		bool FromAnalyse;

		CallResult(HANDLE aHandle, Plugin *aPlugin, bool aFromAnalyse)
			: Handle(aHandle), pPlugin(aPlugin), FromAnalyse(aFromAnalyse) {}
	};

	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	ConsoleTitle ct(Opt.ShowCheckingFile ? Msg::CheckingFileInPlugin.CPtr() : nullptr);
	PHPTR hResult = nullptr;
	CallResult *pCallResult = nullptr;
	std::vector<CallResult> Results;
	FARString strFullName;

	OpMode |= (Type == OFP_ALTERNATIVE) ? OPM_PGDN : (Type == OFP_COMMANDS) ? OPM_COMMANDS : 0;
	AnalyseInfo AnInfo { sizeof(AnalyseInfo), nullptr ,nullptr, 0, OpMode };

	if (FileName)
	{
		ConvertNameToFull(FileName,strFullName);
		FileName = strFullName;
		AnInfo.FileName = FileName;
	}

	bool ShowMenu = Opt.PluginConfirm.OpenFilePlugin == BSTATE_3STATE
			? !(Type == OFP_NORMAL || Type == OFP_SEARCH)
			: Opt.PluginConfirm.OpenFilePlugin != 0;

	Plugin *pPlugin = nullptr;
	std::unique_ptr<SafeMMap> smm;

	for (int i = 0; i < PluginsCount; i++)
	{
		pPlugin = PluginsData[i];
		if (pDesiredPlugin != nullptr && pDesiredPlugin != pPlugin)
			continue;

		if (!pPlugin->HasOpenFilePlugin() && !(pPlugin->HasAnalyse() && pPlugin->HasOpenPlugin()))
			continue;

		if ((Type == OFP_EXTRACT && !pPlugin->HasGetFiles()) ||
			(Type == OFP_COMMANDS && !pPlugin->HasProcessHostFile()))
		{
			continue;
		}

		if(FileName && !smm)
		{
			try
			{
				smm.reset(new SafeMMap(Wide2MB(FileName).c_str(), SafeMMap::M_READ, Opt.PluginMaxReadData));
				AnInfo.Buffer = smm->View();
				AnInfo.BufferSize = smm->Length();
			}
			catch (std::exception &e)
			{
				fprintf(stderr, "PluginManager::OpenFilePlugin: %s\n", e.what());

				if (OpMode == OPM_NONE)
				{
					Message(MSG_WARNING|MSG_ERRORTYPE, 1, MB2Wide(e.what()).c_str(),
						Msg::OpenPluginCannotOpenFile, FileName, Msg::Ok);
				}
				break;
			}
		}

		if (pPlugin->HasOpenFilePlugin())
		{
			if (Opt.ShowCheckingFile)
				ct.Set(L"%ls - [%ls]...", Msg::CheckingFileInPlugin.CPtr(), PointToName(pPlugin->GetModuleName()));

			HANDLE Handle = pPlugin->OpenFilePlugin(FileName,
				smm ? (const unsigned char *)smm->View() : nullptr,
				smm ? (DWORD)smm->Length() : 0, OpMode);

			if (Handle == PANEL_STOP)   //сразу на выход, плагин решил нагло обработать все сам (Autorun/PictureView)!!!
			{
				hResult = PHPTR_STOP;
				break;
			}

			if (Handle != INVALID_HANDLE_VALUE)
			{
				Results.emplace_back(Handle, pPlugin, false);
			}
		}
		else
		{
			AnalyseInfo copyInfo = AnInfo;
			HANDLE Handle = pPlugin->Analyse(&copyInfo);
			if (Handle != INVALID_HANDLE_VALUE)
			{
				Results.emplace_back(Handle, pPlugin, true);
			}
		}

		if (!Results.empty() && !ShowMenu)
			break;
	}

	if (!Results.empty() && (hResult != PHPTR_STOP))
	{
		bool OnlyOne = (Results.size() == 1) && !(FileName && Opt.PluginConfirm.OpenFilePlugin &&
			Opt.PluginConfirm.StandardAssociation && Opt.PluginConfirm.EvenIfOnlyOnePlugin);

		if(!OnlyOne && ShowMenu)
		{
			VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY-4);
			menu.SetPosition(-1, -1, 0, 0);
			menu.SetHelp(L"ChoosePluginMenu");
			menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
			MenuItemEx mitem;

			for (const auto &res: Results)
			{
				mitem.Clear();
				mitem.strName = PointToName(res.pPlugin->GetModuleName());
				//NB: here is really should be used sizeof(handle), not sizeof(*handle)
				//cuz sizeof(void *) has special meaning in SetUserData!
				menu.SetUserData(&res, sizeof(&res), menu.AddItem(&mitem));
			}

			if (Opt.PluginConfirm.StandardAssociation && Type == OFP_NORMAL)
			{
				mitem.Clear();
				mitem.Flags |= MIF_SEPARATOR;
				menu.AddItem(&mitem);
				mitem.Clear();
				mitem.strName = Msg::MenuPluginStdAssociation;
				menu.AddItem(&mitem);
			}

			menu.Show();

			while (!menu.Done())
			{
				menu.ReadInput();
				menu.ProcessInput();
			}

			if (menu.GetExitCode() == -1)
				hResult = PHPTR_STOP;
			else
				pCallResult = (CallResult*)menu.GetUserData(nullptr, 0);
		}
		else
		{
			pCallResult = &Results.front();
		}

		if (pCallResult && pCallResult->FromAnalyse)
		{
			AnalyseInfo copyInfo = AnInfo;
			OpenAnalyseInfo oaInfo { sizeof(oaInfo), &copyInfo, pCallResult->Handle };
			HANDLE h = pCallResult->pPlugin->OpenPlugin(OPEN_ANALYSE, &oaInfo);

			if (h != INVALID_HANDLE_VALUE)
				pCallResult->Handle = h;
			else
				pCallResult = nullptr;
		}
	}

	for (const auto& res: Results)
	{
		if (&res != pCallResult)
		{
			if (res.FromAnalyse)
			{
				 if (res.pPlugin->HasCloseAnalyse())
				 {
					 CloseAnalyseInfo Info { sizeof(Info), res.Handle };
					 res.pPlugin->CloseAnalyse(&Info);
				 }
			}
			else
				res.pPlugin->ClosePlugin(res.Handle);
		}
	}

	if (pCallResult)
	{
		hResult = new PanelHandle(pCallResult->Handle, pCallResult->pPlugin);
	}

	return hResult;
}

PHPTR PluginManager::OpenFindListPlugin(const PluginPanelItem *PanelItems, int ItemsNumber)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	PanelHandle *pResult = nullptr;
	std::vector<PanelHandle> panels;

	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasSetFindList())
		{
			HANDLE hPlugin = pPlugin->OpenPlugin(OPEN_FINDLIST, nullptr);

			if (hPlugin != INVALID_HANDLE_VALUE)
			{
				panels.emplace_back(hPlugin, pPlugin);
				if (!Opt.PluginConfirm.SetFindList)
					break;
			}
		}
	}

	if (panels.size() > 1)
	{
		VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY-4);
		menu.SetPosition(-1, -1, 0, 0);
		menu.SetHelp(L"ChoosePluginMenu");
		menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);

		for (const auto &ph: panels)
		{
			MenuItemEx mitem;
			mitem.strName = PointToName(ph.pPlugin->GetModuleName());
			menu.AddItem(&mitem);
		}

		menu.Show();

		while (!menu.Done())
		{
			menu.ReadInput();
			menu.ProcessInput();
		}

		int ExitCode = menu.GetExitCode();
		if (ExitCode >= 0)
		{
			pResult = &panels[ExitCode];
		}
	}
	else if (panels.size() == 1)
	{
		pResult = &panels.front();
	}

	if (pResult)
	{
		if (!pResult->pPlugin->SetFindList(pResult->hPanel, PanelItems, ItemsNumber))
			pResult = nullptr;
	}

	for (const auto &ph: panels)
	{
		if (&ph != pResult && ph.hPanel != INVALID_HANDLE_VALUE)
			ph.pPlugin->ClosePlugin(ph.hPanel);
	}

	return pResult ? new PanelHandle(pResult->hPanel, pResult->pPlugin) : nullptr;
}

void PluginManager::ClosePanel(PHPTR ph)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	assert(ph->RefCnt > 0);
	if (--ph->RefCnt == 0) {
		ph->pPlugin->ClosePlugin(ph->hPanel);
		delete ph;
	}
}

void PluginManager::RetainPanel(PHPTR ph)
{
	assert(ph->RefCnt > 0);
	++ph->RefCnt;
}

FARString PluginManager::GetPluginModuleName(PHPTR ph)
{
	return ph->pPlugin->GetModuleName();
}

int PluginManager::ProcessEditorInput(INPUT_RECORD *Rec)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessEditorInput() && pPlugin->ProcessEditorInput(Rec))
			return TRUE;
	}

	return FALSE;
}

int PluginManager::ProcessEditorEvent(int Event,void *Param)
{
	if (CurEditor)
	{
		if (Event == EE_REDRAW)
		{
			CurEditor->AutoDeleteColors();
		}

		for (int i = 0; i < PluginsCount; i++)
		{
			Plugin *pPlugin = PluginsData[i];

			// The return value is currently ignored
			if (pPlugin->HasProcessEditorEvent())
				pPlugin->ProcessEditorEvent(Event, Param);
		}
	}

	return 0;
}


int PluginManager::ProcessViewerEvent(int Event, void *Param)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		// The return value is currently ignored
		if (pPlugin->HasProcessViewerEvent())
			pPlugin->ProcessViewerEvent(Event, Param);
	}

	return 0;
}

int PluginManager::ProcessDialogEvent(int Event, void *Param)
{
	for (int i = 0; i<PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessDialogEvent() && pPlugin->ProcessDialogEvent(Event,Param))
			return TRUE;
	}

	return FALSE;
}

int PluginManager::GetFindData(PHPTR ph, PluginPanelItem **pItems, int *pItemsNumber, int OpMode)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	*pItemsNumber = 0;
	return ph->pPlugin->GetFindData(ph->hPanel, pItems, pItemsNumber, OpMode);
}


void PluginManager::FreeFindData(PHPTR ph, PluginPanelItem *pItems, int ItemsNumber)
{
	ph->pPlugin->FreeFindData(ph->hPanel, pItems, ItemsNumber);
}


int PluginManager::GetVirtualFindData(
		PHPTR ph, PluginPanelItem **pItems, int *pItemsNumber, const wchar_t *Path)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	*pItemsNumber = 0;
	return ph->pPlugin->GetVirtualFindData(ph->hPanel, pItems, pItemsNumber, Path);
}


void PluginManager::FreeVirtualFindData(PHPTR ph, PluginPanelItem *PanelItem, int ItemsNumber)
{
	return ph->pPlugin->FreeVirtualFindData(ph->hPanel, PanelItem, ItemsNumber);
}


int PluginManager::SetDirectory(PHPTR ph, const wchar_t *Dir, int OpMode)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	return ph->pPlugin->SetDirectory(ph->hPanel, Dir, OpMode);
}


int PluginManager::ProcessConsoleInput(INPUT_RECORD *Rec)
{
	bool InputChanged = false;

	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];
		if (!pPlugin->HasProcessConsoleInput())
			continue;

		int Result = pPlugin->ProcessConsoleInput(Rec);
		if (Result == 1)
		{
			return Result;
		}
		else if (Result == 2)
		{
			InputChanged = true;
		}
	}

	return InputChanged ? 2 : 0;
}


bool PluginManager::GetFile(
		PHPTR ph,
		PluginPanelItem *PanelItem,
		const wchar_t *DestPath,
		FARString &strResultName,
		int OpMode)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SaveScreen *SaveScr = nullptr;
	bool Found = false;
	KeepUserScreen = FALSE;

	if (!(OpMode & OPM_FIND))
		SaveScr = new SaveScreen; //???

	UndoGlobalSaveScrPtr UndSaveScr(SaveScr);
	int GetCode = ph->pPlugin->GetFiles(ph->hPanel, PanelItem, 1, 0, &DestPath, OpMode);
	FARString strFindPath;
	strFindPath = DestPath;
	AddEndSlash(strFindPath);
	strFindPath += L"*";
	FAR_FIND_DATA_EX fdata;
	FindFile Find(strFindPath);
	bool Done = true;
	while(Find.Get(fdata))
	{
		if(!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			Done = false;
			break;
		}
	}

	if (!Done)
	{
		strResultName = DestPath;
		AddEndSlash(strResultName);
		strResultName += fdata.strFileName;

		if (GetCode!=1)
		{
			apiSetFileAttributes(strResultName,FILE_ATTRIBUTE_NORMAL);
			apiDeleteFile(strResultName); //BUGBUG
		}
		else
			Found = true;
	}

	ReadUserBackground(SaveScr);
	delete SaveScr;
	return Found;
}


int PluginManager::DeleteFiles(
    PHPTR ph,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen = FALSE;
	int Code = ph->pPlugin->DeleteFiles(ph->hPanel, PanelItem, ItemsNumber, OpMode);

	if (Code)
		ReadUserBackground(&SaveScr); //???

	return Code;
}


int PluginManager::MakeDirectory(PHPTR ph, const wchar_t **Name, int OpMode)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen = FALSE;
	int Code = ph->pPlugin->MakeDirectory(ph->hPanel, Name, OpMode);

	if (Code != -1)   //???BUGBUG
		ReadUserBackground(&SaveScr);

	return Code;
}


int PluginManager::ProcessHostFile(
    PHPTR ph,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen = FALSE;
	int Code = ph->pPlugin->ProcessHostFile(ph->hPanel, PanelItem, ItemsNumber, OpMode);

	if (Code)   //BUGBUG
		ReadUserBackground(&SaveScr);

	return Code;
}

bool PluginManager::GetLinkTarget(PHPTR ph, PluginPanelItem *PanelItem, FARString &result, int OpMode)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	return ph->pPlugin->GetLinkTarget(ph->hPanel, PanelItem, result, OpMode);
}

int PluginManager::GetFiles(
		PHPTR ph,
		PluginPanelItem *PanelItems,
		int ItemsNumber,
		int Move,
		const wchar_t **DestPath,
		int OpMode)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	return ph->pPlugin->GetFiles(ph->hPanel, PanelItems, ItemsNumber, Move, DestPath, OpMode);
}


int PluginManager::PutFiles(
		PHPTR ph,
		PluginPanelItem *PanelItems,
		int ItemsNumber,
		int Move,
		int OpMode)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen = FALSE;
	int Code = ph->pPlugin->PutFiles(ph->hPanel, PanelItems, ItemsNumber, Move, OpMode);

	if (Code)   //BUGBUG
		ReadUserBackground(&SaveScr);

	return Code;
}

void PluginManager::GetOpenPluginInfo(PHPTR ph, OpenPluginInfo *Info)
{
	*Info = { sizeof(*Info) };
	ph->pPlugin->GetOpenPluginInfo(ph->hPanel, Info);

	if ((Info->Flags & OPIF_REALNAMES)
			&& (CtrlObject->Cp()->ActivePanel->GetPluginHandle() == ph)
			&& Info->CurDir && *Info->CurDir && !IsNetworkServerPath(Info->CurDir))
	{
		apiSetCurrentDirectory(Info->CurDir, false);
	}
}

int PluginManager::ProcessKey(PHPTR ph, int Key, unsigned int ControlState)
{
	return ph->pPlugin->ProcessKey(ph->hPanel, Key, ControlState);
}

int PluginManager::ProcessEvent(PHPTR ph, int Event, void *Param)
{
	return ph->pPlugin->ProcessEvent(ph->hPanel, Event, Param);
}

int PluginManager::Compare(PHPTR ph, const PluginPanelItem *Item1, const PluginPanelItem *Item2,
		unsigned int Mode)
{
	return ph->pPlugin->Compare(ph->hPanel, Item1, Item2, Mode);
}

void PluginManager::ConfigureCurrent(Plugin *pPlugin, int INum, const GUID *Guid)
{
	int Result = FALSE;
	if (pPlugin->HasConfigureV3()) {
		if (pPlugin->UseMenuGuids()) {
			Guid = Guid ? Guid : &FarGuid;
			ConfigureInfo Info = { sizeof(ConfigureInfo), Guid };
			Result = dynamic_cast<PluginW*>(pPlugin)->ConfigureV3(&Info);
		}
	}
	else {
		Result = pPlugin->Configure(INum);
	}

	if (Result) {
		auto panels = { CtrlObject->Cp()->LeftPanel, CtrlObject->Cp()->RightPanel };
		for (auto panel: panels) {
			if (panel->GetMode() == PLUGIN_PANEL) {
				panel->Update(UPDATE_KEEP_SELECTION);
				panel->SetViewMode(panel->GetViewMode());
				panel->Redraw();
			}
		}
		pPlugin->SaveToCache();
	}
}

struct PluginMenuItemData
{
	Plugin *pPlugin;
	intptr_t nItem;
	GUID Guid;
	wchar_t HotKey;
};

struct TmpItemData
{
	PluginMenuItemData Item;
	FARString Name;
	TmpItemData(const PluginMenuItemData &aItem, const FARString &aName) : Item(aItem), Name(aName) {}
};

static std::string GetHotKeySettingName(Plugin *pPlugin, int ItemNumber, const GUID *Guid, MENUTYPE MenuType)
{
	if (pPlugin->UseMenuGuids())
	{
		const std::string &strGuid = GuidToString(*Guid);
		std::string out = StrPrintf("%08X:%s#%s", pPlugin->GetSysID(), HotKeyType(MenuType), strGuid.c_str());
		return out;
	}
	std::string out = pPlugin->GetSettingsName();
	out+= StrPrintf(":%s#%d", HotKeyType(MenuType), ItemNumber);
	return out;
}

static void GetPluginHotKey(Plugin *pPlugin, int ItemNumber, const GUID *Guid, MENUTYPE MenuType, FARString &strHotKey)
{
	KeyFileReadSection kfh(PluginsIni(), HotkeysSection);
	strHotKey = kfh.GetString(GetHotKeySettingName(pPlugin, ItemNumber, Guid, MenuType));
}

static void FillPluginList(VMenu &PluginList, const std::vector<TmpItemData> &TmpItems, bool HotKeysPresent)
{
	for (const auto &tmp: TmpItems)
	{
		MenuItemEx ListItem;
		if (tmp.Item.pPlugin->IsOemPlugin())
			ListItem.Flags = LIF_CHECKED|L'A';

		if (!HotKeysPresent)
			ListItem.strName = tmp.Name;
		else if (tmp.Item.HotKey)
			ListItem.strName.Format(L"&%lc%ls  %ls",
					tmp.Item.HotKey,
					(tmp.Item.HotKey == L'&' ? L"&" : L""),
					tmp.Name.CPtr());
		else
			ListItem.strName.Format(L"   %ls", tmp.Name.CPtr());

		PluginList.SetUserData(&tmp.Item, sizeof(tmp.Item), PluginList.AddItem(&ListItem));
	}
}

/* $ 29.05.2001 IS
   ! При настройке "параметров внешних модулей" закрывать окно с их
     списком только при нажатии на ESC
*/
void PluginManager::Configure(int StartPos)
{
	ChangeMacroArea Cma(MACROAREA_MENU);
	VMenu PluginList(Msg::PluginConfigTitle,nullptr,0,ScrY-4);
	PluginList.SetFlags(VMENU_WRAPMODE);
	PluginList.SetHelp(L"PluginsConfig");
	PluginList.SetId(PluginsConfigMenuId);

	for (;;)
	{
		bool NeedUpdateItems = true;
		int MenuItemNumber = 0;
		bool HotKeysPresent = false;

		if (NeedUpdateItems)
		{
			PluginList.ClearDone();
			PluginList.DeleteItems();
			PluginList.SetPosition(-1,-1,0,0);
			LoadIfCacheAbsent();
			FARString strName;
			PluginInfo Info{};
			std::vector<TmpItemData> TmpItems;

			Cma.SetPrevArea(); // for plugins: set the right macro area in GetPluginInfo()
			for (int I = 0; I<PluginsCount; I++)
			{
				Plugin *pPlugin = PluginsData[I];
				bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED);

				if (!bCached && !pPlugin->GetPluginInfo(&Info))
					continue;

				KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
				for (int J = 0; ; J++)
				{
					PluginMenuItemData item { .pPlugin=pPlugin, .nItem=J };
					if (bCached)
					{
						const std::string &key = StrPrintf(FmtPluginConfigStringD, J);
						if (kfh.HasKey(key))
							strName = kfh.GetString(key, "");
						else
							break;

						if (pPlugin->UseMenuGuids())
						{
							const std::string &keyGuid = StrPrintf(FmtPluginConfigGuidD, J);
							if (!kfh.HasKey(keyGuid))
								break;

							const auto &strGuid = kfh.GetString(keyGuid, L"");
							if (!StrToGuid(strGuid.c_str(), item.Guid))
								break;
						}
					}
					else if (J < Info.PluginConfigStringsNumber)
					{
						strName = Info.PluginConfigStrings[J];
						if (pPlugin->UseMenuGuids())
							item.Guid = Info.PluginConfigGuids[J];
					}
					else
						break;

					FARString strHotKey;
					GetPluginHotKey(pPlugin, J, &item.Guid, MTYPE_CONFIGSMENU, strHotKey);
					if (!strHotKey.IsEmpty()) {
						HotKeysPresent = true;
						item.HotKey = strHotKey[0];
					}
					TmpItems.emplace_back(item, strName);
				}
			}
			Cma.SetCurArea();
			MenuItemNumber = TmpItems.size();
			FillPluginList(PluginList, TmpItems, HotKeysPresent);

			PluginList.AssignHighlights(FALSE);
			PluginList.SetBottomTitle(Msg::PluginHotKeyBottom);
			PluginList.ClearDone();
			PluginList.SortItems(0, HotKeysPresent ? 3 : 0);
			PluginList.SetSelectPos(StartPos, 1);
			NeedUpdateItems = false;
		}

		PluginList.Show();

		while (!PluginList.Done())
		{
			FarKey Key = PluginList.ReadInput();
			int SelPos = PluginList.GetSelectPos();
			PluginMenuItemData *item = (PluginMenuItemData*)PluginList.GetUserData(nullptr,0,SelPos);

			switch (Key)
			{
				case KEY_SHIFTF1:
					if (item)
					{
						const FARString &strModuleName = item->pPlugin->GetModuleName();
						const wchar_t *topics[] = { L"Config", L"Configure", nullptr };
						for (auto topic: topics)
						{
							if (FarShowHelp(strModuleName, topic, FHELP_SELFHELP|FHELP_NOSHOWERROR))
								break;
						}
					}
					break;

				case KEY_F3:
					if (item)
						ShowPluginInfo(item->pPlugin, item->nItem, item->Guid);
					break;

				case KEY_F4:
					if (item && PluginList.GetItemCount() > 0 && SelPos<MenuItemNumber)
					{
						FARString strName00;
						int nOffset = HotKeysPresent?3:0;
						strName00 = PluginList.GetItemPtr()->strName.CPtr()+nOffset;
						RemoveExternalSpaces(strName00);

						if (SetHotKeyDialog(strName00, item->pPlugin, item->nItem, &item->Guid, MTYPE_CONFIGSMENU))
						{
							PluginList.Hide();
							NeedUpdateItems = true;
							StartPos = SelPos;
							PluginList.SetExitCode(SelPos);
							PluginList.Show();
						}
					}
					break;

				default:
					PluginList.ProcessInput();
					break;
			}
		}

		if (!NeedUpdateItems)
		{
			StartPos = PluginList.Modal::GetExitCode();
			PluginList.Hide();

			if (StartPos<0)
				break;

			PluginMenuItemData *item = (PluginMenuItemData*)PluginList.GetUserData(nullptr,0,StartPos);
			ConfigureCurrent(item->pPlugin, item->nItem, &item->Guid);
		}
	}
}

int PluginManager::CommandsMenu(int ModalType,int StartPos,const wchar_t *HistoryName)
{
	if(ModalType == MODALTYPE_DIALOG)
	{
		if(reinterpret_cast<Dialog*>(FrameManager->GetCurrentFrame())->CheckDialogMode(DMODE_NOPLUGINS))
		{
			return 0;
		}
	}

	int MenuItemNumber = 0;
	bool IsEditor = ModalType == MODALTYPE_EDITOR;
	bool IsViewer = ModalType == MODALTYPE_VIEWER;
	bool IsDialog = ModalType == MODALTYPE_DIALOG;
	PluginMenuItemData item;
	{
		ChangeMacroArea Cma(MACROAREA_MENU);
		VMenu PluginList(Msg::PluginCommandsMenuTitle,nullptr,0,ScrY-4);
		PluginList.SetFlags(VMENU_WRAPMODE);
		PluginList.SetHelp(L"PluginCommands");
		PluginList.SetId(PluginsMenuId);
		bool NeedUpdateItems = true;

		for (;;)
		{
			bool HotKeysPresent = false;

			if (NeedUpdateItems)
			{
				PluginList.ClearDone();
				PluginList.DeleteItems();
				PluginList.SetPosition(-1,-1,0,0);
				LoadIfCacheAbsent();
				FARString strName;
				PluginInfo Info{};
				std::vector<TmpItemData> TmpItems;

				Cma.SetPrevArea(); // for plugins: set the right macro area in GetPluginInfo()
				for (int I = 0; I<PluginsCount; I++)
				{
					Plugin *pPlugin = PluginsData[I];
					bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED);
					KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
					int IFlags;

					if (bCached)
						IFlags = kfh.GetUInt("Flags", 0);
					else if (pPlugin->GetPluginInfo(&Info))
						IFlags = Info.Flags;
					else
						continue;

					if ((IsEditor && !(IFlags & PF_EDITOR)) ||
							(IsViewer && !(IFlags & PF_VIEWER)) ||
							(IsDialog && !(IFlags & PF_DIALOG)) ||
							(!IsEditor && !IsViewer && !IsDialog && (IFlags & PF_DISABLEPANELS)))
						continue;

					for (int J = 0; ; J++)
					{
						PluginMenuItemData item { .pPlugin=pPlugin, .nItem=J };
						if (bCached)
						{
							const std::string &key = StrPrintf(FmtPluginMenuStringD, J);
							if (kfh.HasKey(key))
								strName = kfh.GetString(key, "");
							else
								break;

							if (pPlugin->UseMenuGuids())
							{
								const std::string &keyGuid = StrPrintf(FmtPluginMenuGuidD, J);
								if (!kfh.HasKey(keyGuid))
									break;

								const auto &strGuid = kfh.GetString(keyGuid, L"");
								if (!StrToGuid(strGuid.c_str(), item.Guid))
									break;
							}
						}
						else if (J < Info.PluginMenuStringsNumber)
						{
							strName = Info.PluginMenuStrings[J];
							if (pPlugin->UseMenuGuids())
								item.Guid = Info.PluginMenuGuids[J];
						}
						else
							break;

						FARString strHotKey;
						GetPluginHotKey(pPlugin, J, &item.Guid, MTYPE_COMMANDSMENU, strHotKey);
						if (!strHotKey.IsEmpty()) {
							HotKeysPresent = true;
							item.HotKey = strHotKey[0];
						}
						TmpItems.emplace_back(item, strName);
					}
				}
				Cma.SetCurArea();
				MenuItemNumber = TmpItems.size();
				FillPluginList(PluginList, TmpItems, HotKeysPresent);

				PluginList.AssignHighlights(FALSE);
				PluginList.SetBottomTitle(Msg::PluginHotKeyBottom);
				PluginList.SortItems(0, HotKeysPresent ? 3 : 0);
				PluginList.SetSelectPos(StartPos, 1);
				NeedUpdateItems = false;
			}

			PluginList.Show();

			while (!PluginList.Done())
			{
				FarKey Key = PluginList.ReadInput();
				int SelPos = PluginList.GetSelectPos();
				PluginMenuItemData *item = (PluginMenuItemData*)PluginList.GetUserData(nullptr,0,SelPos);

				switch (Key)
				{
					case KEY_SHIFTF1:
						// Вызываем нужный топик, который передали в CommandsMenu()
						if (item)
							FarShowHelp(item->pPlugin->GetModuleName(),HistoryName,FHELP_SELFHELP|FHELP_NOSHOWERROR|FHELP_USECONTENTS);
						break;

					case KEY_F3:
						if (item)
							ShowPluginInfo(item->pPlugin, item->nItem, item->Guid);
						break;

					case KEY_F4:
						if (item && PluginList.GetItemCount() > 0 && SelPos < MenuItemNumber)
						{
							int nOffset = HotKeysPresent ? 3 : 0;
							FARString strName00 = PluginList.GetItemPtr()->strName.CPtr() + nOffset;
							RemoveExternalSpaces(strName00);

							if (SetHotKeyDialog(strName00, item->pPlugin, item->nItem, &item->Guid, MTYPE_COMMANDSMENU))
							{
								PluginList.Hide();
								NeedUpdateItems = true;
								StartPos = SelPos;
								PluginList.SetExitCode(SelPos);
								PluginList.Show();
							}
						}
						break;

					case KEY_ALTSHIFTF9:
						PluginList.Hide();
						NeedUpdateItems = true;
						StartPos = SelPos;
						PluginList.SetExitCode(SelPos);
						Configure();
						PluginList.Show();
						break;

					case KEY_SHIFTF9:
						if (item && PluginList.GetItemCount() > 0 && SelPos < MenuItemNumber)
						{
							NeedUpdateItems = true;
							StartPos = SelPos;

							if (item->pPlugin->HasConfigure() || item->pPlugin->HasConfigureV3())
								ConfigureCurrent(item->pPlugin, item->nItem, &item->Guid);

							PluginList.SetExitCode(SelPos);
							PluginList.Show();
						}
						break;

					default:
						PluginList.ProcessInput();
						break;
				}
			}

			if (!NeedUpdateItems && PluginList.Done())
				break;
		}

		int ExitCode = PluginList.Modal::GetExitCode();
		PluginList.Hide();

		if (ExitCode<0)
		{
			return FALSE;
		}

		ScrBuf.Flush();
		item = *(PluginMenuItemData*)PluginList.GetUserData(nullptr,0,ExitCode);
	}

	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
	int OpenCode = OPEN_PLUGINSMENU;
	const void *Item = (void*)item.nItem;
	if (item.pPlugin->UseMenuGuids()) {
		Item = &item.Guid;
	}
	OpenDlgPluginData pd {};

	if (IsEditor)
	{
		OpenCode = OPEN_EDITOR;
	}
	else if (IsViewer)
	{
		OpenCode = OPEN_VIEWER;
	}
	else if (IsDialog)
	{
		OpenCode = OPEN_DIALOG;
		pd.hDlg=(HANDLE)FrameManager->GetCurrentFrame();
		if (item.pPlugin->UseMenuGuids())
			pd.ItemGuid = item.Guid;
		else
			pd.ItemNumber = item.nItem;

		Item = &pd;
	}

	PHPTR hPlugin = OpenPlugin(item.pPlugin, OpenCode, Item);

	if (hPlugin && !IsEditor && !IsViewer && !IsDialog)
	{
		if (ActivePanel->ProcessPluginEvent(FE_CLOSE,nullptr))
		{
			ClosePanel(hPlugin);
			return FALSE;
		}

		Panel *NewPanel = CtrlObject->Cp()->ChangePanel(ActivePanel,FILE_PANEL,true,true);
		NewPanel->SetPluginMode(hPlugin,L"",true);
		NewPanel->Update(0);
		NewPanel->Show();
	}

	// restore title for old plugins only.
	if (item.pPlugin->IsOemPlugin() && IsEditor && CurEditor)
	{
		CurEditor->SetPluginTitle(nullptr);
	}

	return TRUE;
}

bool PluginManager::SetHotKeyDialog(
		const wchar_t *DlgPluginTitle,
		Plugin *pPlugin,
		int ItemNumber,
		const GUID *Guid,
		MENUTYPE MenuType)
{
	const std::string &SettingName = GetHotKeySettingName(pPlugin, ItemNumber, Guid, MenuType);
	KeyFileHelper kfh(PluginsIni());
	const auto &Setting = kfh.GetString(HotkeysSection, SettingName, L"");
	wchar_t Letter[2] = {Setting.empty() ? 0 : Setting[0], 0};
	if (!HotkeyLetterDialog(Msg::PluginHotKeyTitle, DlgPluginTitle, Letter[0]))
		return false;

	if (Letter[0])
		kfh.SetString(HotkeysSection, SettingName, Letter);
	else
		kfh.RemoveKey(HotkeysSection, SettingName);
	return true;
}

bool PluginManager::GetDiskMenuItem(
		Plugin *pPlugin,
		int PluginItem,
		wchar_t& PluginHotkey,
		FARString &strPluginText,
		GUID &Guid
)
{
	LoadIfCacheAbsent();

	if (pPlugin->CheckWorkFlags(PIWF_CACHED))
	{
		KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
		const std::string &key = StrPrintf(FmtDiskMenuStringD, PluginItem);
		if (kfh.HasKey(key))
			strPluginText = kfh.GetString(key, "");
		else
			return false;

		if (pPlugin->UseMenuGuids())
		{
			const std::string &keyGuid = StrPrintf(FmtDiskMenuGuidD, PluginItem);
			if (!kfh.HasKey(keyGuid))
				return false;

			const auto &strGuid = kfh.GetString(keyGuid, L"");
			if (!StrToGuid(strGuid.c_str(), Guid))
				return false;
		}
	}
	else
	{
		PluginInfo Info {};
		if (!pPlugin->GetPluginInfo(&Info) || (PluginItem >= Info.DiskMenuStringsNumber))
			return false;

		strPluginText = Info.DiskMenuStrings[PluginItem];
		if (pPlugin->UseMenuGuids())
			Guid = Info.DiskMenuGuids[PluginItem];
	}

	FARString strHotKey;
	GetPluginHotKey(pPlugin, PluginItem, &Guid, MTYPE_DISKSMENU, strHotKey);
	PluginHotkey = strHotKey.At(0);
	return !strPluginText.IsEmpty();
}

bool PluginManager::UseFarCommand(PHPTR ph,int CommandType)
{
	OpenPluginInfo Info;
	GetOpenPluginInfo(ph,&Info);

	if (!(Info.Flags & OPIF_REALNAMES))
		return false;

	switch (CommandType)
	{
		case PLUGIN_FARGETFILE:
		case PLUGIN_FARGETFILES:
			return(!ph->pPlugin->HasGetFiles() || (Info.Flags & OPIF_EXTERNALGET));
		case PLUGIN_FARPUTFILES:
			return(!ph->pPlugin->HasPutFiles() || (Info.Flags & OPIF_EXTERNALPUT));
		case PLUGIN_FARDELETEFILES:
			return(!ph->pPlugin->HasDeleteFiles() || (Info.Flags & OPIF_EXTERNALDELETE));
		case PLUGIN_FARMAKEDIRECTORY:
			return(!ph->pPlugin->HasMakeDirectory() || (Info.Flags & OPIF_EXTERNALMKDIR));
	}

	return true;
}


void PluginManager::ReloadLanguage()
{
	for (int I = 0; I<PluginsCount; I++)
	{
		PluginsData[I]->CloseLang();
	}

	DiscardCache();
}


void PluginManager::DiscardCache()
{
	for (int I = 0; I<PluginsCount; I++)
	{
		PluginsData[I]->Load();
	}

	KeyFileHelper kfh(PluginsIni());
	for (const auto &s : kfh.EnumSections())
	{
		if (s != HotkeysSection)
			kfh.RemoveSection(s);
	}
}


void PluginManager::LoadIfCacheAbsent()
{
	struct stat st;
	if (stat(PluginsIni(), &st) == -1)
	{
		for (int I = 0; I<PluginsCount; I++)
		{
			PluginsData[I]->Load();
		}
	}
}

bool PluginManager::ProcessCommandLine(const wchar_t *CommandParam, Panel *Target)
{
	struct PluginData
	{
		Plugin *pPlugin;
		DWORD Flags;

		PluginData(Plugin *aPlugin, DWORD aFlags) : pPlugin(aPlugin), Flags(aFlags) {}
	};

	size_t PrefixLength = 0;
	FARString strCommand = CommandParam;
	UnquoteExternal(strCommand);
	RemoveLeadingSpaces(strCommand);

	for (;;)
	{
		wchar_t Ch = strCommand.At(PrefixLength);

		if (!Ch || IsSpace(Ch) || Ch == L'/' || PrefixLength>64)
			return false;

		if (Ch == L':' && PrefixLength>0)
			break;

		PrefixLength++;
	}

	LoadIfCacheAbsent();
	FARString strPrefix(strCommand,PrefixLength);
	FARString strPluginPrefix;
	std::vector<PluginData> items;

	for (int I = 0; I<PluginsCount; I++)
	{
		Plugin *pPlugin = PluginsData[I];
		int PluginFlags = 0;

		if (pPlugin->CheckWorkFlags(PIWF_CACHED))
		{
			KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
			strPluginPrefix = kfh.GetString("CommandPrefix", "");
			PluginFlags = kfh.GetUInt("Flags", 0);
		}
		else
		{
			PluginInfo Info;

			if (pPlugin->GetPluginInfo(&Info))
			{
				strPluginPrefix = Info.CommandPrefix;
				PluginFlags = Info.Flags;
			}
			else
				continue;
		}

		if (strPluginPrefix.IsEmpty())
			continue;

		const wchar_t *PrStart = strPluginPrefix;
		PrefixLength = strPrefix.GetLength();

		for (;;)
		{
			const wchar_t *PrEnd = wcschr(PrStart, L':');
			size_t Len = PrEnd ? (PrEnd - PrStart) : StrLength(PrStart);

			if (Len < PrefixLength) Len = PrefixLength;

			if (!StrCmpNI(strPrefix, PrStart, (int)Len))
			{
				if (pPlugin->Load() && pPlugin->HasOpenPlugin())
				{
					items.emplace_back(pPlugin, PluginFlags);
					break;
				}
			}

			if (!PrEnd)
				break;

			PrStart = ++PrEnd;
		}

		if (!items.empty() && !Opt.PluginConfirm.Prefix)
			break;
	}

	if (items.empty())
		return false;

	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
	Panel *CurPanel = Target ? Target : ActivePanel;

	if (CurPanel->ProcessPluginEvent(FE_CLOSE,nullptr))
		return false;

	PluginData* PData = nullptr;

	if (items.size() > 1)
	{
		VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY-4);
		menu.SetPosition(-1, -1, 0, 0);
		menu.SetHelp(L"ChoosePluginMenu");
		menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
		MenuItemEx mitem;

		for (size_t i = 0; i < items.size(); i++)
		{
			mitem.Clear();
			mitem.strName = PointToName(items[i].pPlugin->GetModuleName());
			menu.AddItem(&mitem);
		}

		menu.Show();

		while (!menu.Done())
		{
			menu.ReadInput();
			menu.ProcessInput();
		}

		int ExitCode = menu.GetExitCode();

		if (ExitCode >= 0)
		{
			PData = &items[ExitCode];
		}
	}
	else
	{
		PData = &items.front();
	}

	if (PData)
	{
		CtrlObject->CmdLine->SetString(L"");
		FARString strPluginCommand = strCommand.CPtr() + (PData->Flags & PF_FULLCMDLINE ? 0:PrefixLength+1);
		RemoveTrailingSpaces(strPluginCommand);
		PHPTR hPlugin = OpenPlugin(PData->pPlugin, OPEN_COMMANDLINE, strPluginCommand.CPtr());

		if (hPlugin)
		{
			Panel *NewPanel = CtrlObject->Cp()->ChangePanel(CurPanel,FILE_PANEL,true,true);
			NewPanel->SetPluginMode(hPlugin,L"",!Target || Target == ActivePanel);
			NewPanel->Update(0);
			NewPanel->Show();
		}
	}

	return true;
}


void PluginManager::ReadUserBackground(SaveScreen *SaveScr)
{
	FilePanels *FPanel = CtrlObject->Cp();
	FPanel->LeftPanel->ProcessingPluginCommand++;
	FPanel->RightPanel->ProcessingPluginCommand++;

	if (KeepUserScreen)
	{
		if (SaveScr)
			SaveScr->Discard();

		SCOPED_ACTION(RedrawDesktop);
	}

	FPanel->LeftPanel->ProcessingPluginCommand--;
	FPanel->RightPanel->ProcessingPluginCommand--;
}


bool PluginManager::CallMacroPlugin(OpenMacroPluginInfo *Info)
{
#ifdef USELUA
	Plugin *pPlugin = FindPlugin(SYSID_LUAMACRO);
	return pPlugin && pPlugin->OpenPlugin(OPEN_LUAMACRO, Info);
#else
	return false;
#endif
}

void* PluginManager::CallPluginFromMacro(DWORD SysID, OpenMacroInfo *Info)
{
	Plugin *pPlugin = FindPlugin(SysID);
	if ( !(pPlugin && pPlugin->HasOpenPlugin()) )
		return nullptr;

	PHPTR PluginPanel = OpenPlugin(pPlugin, OPEN_FROMMACRO, Info);

	if (PluginPanel)
	{
		if (IsPointer(PluginPanel->hPanel) && PluginPanel->hPanel != INVALID_HANDLE_VALUE)
		{
			FarMacroCall *fmc = reinterpret_cast<FarMacroCall*>(PluginPanel->hPanel);
			if (fmc->Count > 0 && fmc->Values[0].Type == FMVT_PANEL)
			{
				PluginPanel->hPanel = fmc->Values[0].Pointer;
				if (fmc->Callback)
					fmc->Callback(fmc->CallbackData, fmc->Values, fmc->Count);

				int CurFocus = CtrlObject->Cp()->ActivePanel->GetFocus();
				Panel *NewPanel = CtrlObject->Cp()->ChangePanel(CtrlObject->Cp()->ActivePanel,FILE_PANEL,true,true);
				bool SendOnFocus = CurFocus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible();
				NewPanel->SetPluginMode(PluginPanel, L"", SendOnFocus);
				NewPanel->Update(0);
				NewPanel->Show();
				return reinterpret_cast<void*>(1);
			}
		}

		void *hPanel = PluginPanel->hPanel;
		delete PluginPanel;
		return hPanel;
	}

	return nullptr;
}

/* $ 27.09.2000 SVS
  Функция CallPlugin - найти плагин по ID и запустить
  в зачаточном состоянии!
*/
bool PluginManager::CallPlugin(DWORD SysID, int OpenFrom, void *Data)
{
	Plugin *pPlugin = FindPlugin(SysID);
	if ( !(pPlugin && pPlugin->HasOpenPlugin()) )
		return false;

	PHPTR PluginPanel = OpenPlugin(pPlugin, OpenFrom, Data);
	if (PluginPanel && (OpenFrom == OPEN_PLUGINSMENU || OpenFrom == OPEN_FILEPANEL))
	{
		int CurFocus = CtrlObject->Cp()->ActivePanel->GetFocus();
		Panel *NewPanel = CtrlObject->Cp()->ChangePanel(CtrlObject->Cp()->ActivePanel,FILE_PANEL,true,true);
		bool SendOnFocus = CurFocus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible();
		NewPanel->SetPluginMode(PluginPanel, L"", SendOnFocus);

		if (Data && *(const wchar_t *)Data)
			SetDirectory(PluginPanel,(const wchar_t *)Data,0);
	}

	return true;
}

// поддержка макрофункций Plugin.Menu, Plugin.Command, Plugin.Config
bool PluginManager::CallPluginItem(DWORD SysID, CallPluginInfo *Data)
{
	auto Result = false;

	Frame *TopFrame = FrameManager->GetTopModal();
	const auto curType = TopFrame->GetType();

	if (curType == MODALTYPE_DIALOG && reinterpret_cast<Dialog*>(TopFrame)->CheckDialogMode(DMODE_NOPLUGINS))
		return false;

	const auto IsEditor = curType == MODALTYPE_EDITOR;
	const auto IsViewer = curType == MODALTYPE_VIEWER;
	const auto IsDialog = curType == MODALTYPE_DIALOG;

	Plugin *pPlugin = FindPlugin(SysID);
	bool UseMenuGuids = pPlugin && pPlugin->UseMenuGuids();

	if (Data->CallFlags & CPT_CHECKONLY)
	{
		Data->pPlugin = pPlugin;
		if (!pPlugin || !pPlugin->Load())
			return false;

		// Разрешен ли вызов данного типа в текущей области (предварительная проверка)
		switch (Data->CallFlags & CPT_MASK)
		{
		case CPT_MENU:
			if (!Data->pPlugin->HasOpenPlugin())
				return false;
			break;

		case CPT_CONFIGURE:
			//TODO: Автокомплит не влияет?
			if (curType!=MODALTYPE_PANELS)
				return false;

			if (UseMenuGuids ? !Data->pPlugin->HasConfigureV3() : !Data->pPlugin->HasConfigure())
				return false;
			break;

		case CPT_CMDLINE:
			//TODO: Автокомплит не влияет?
			if (curType!=MODALTYPE_PANELS)
				return false;

			//TODO: OpenPanel или OpenFilePlugin?
			if (!Data->pPlugin->HasOpenPlugin())
				return false;
			break;

		default:
			break;
		}

		PluginInfo Info{sizeof(Info)};
		if (!Data->pPlugin->GetPluginInfo(&Info))
			return false;

		const auto IFlags = Info.Flags;
		int MenuItemsCount = 0;

		// Разрешен ли вызов данного типа в текущей области
		switch (Data->CallFlags & CPT_MASK)
		{
		case CPT_MENU:
			if (
					(IsEditor && !(IFlags & PF_EDITOR)) ||
					(IsViewer && !(IFlags & PF_VIEWER)) ||
					(IsDialog && !(IFlags & PF_DIALOG)) ||
					(!IsEditor && !IsViewer && !IsDialog && (IFlags & PF_DISABLEPANELS)))
				return false;

			MenuItemsCount = Info.PluginMenuStringsNumber;
			break;

		case CPT_CONFIGURE:
			MenuItemsCount = Info.PluginConfigStringsNumber;
			break;

		case CPT_CMDLINE:
			if (!Info.CommandPrefix || !*Info.CommandPrefix)
				return false;
			break;

		default:
			break;
		}

		if ((Data->CallFlags & CPT_MASK)==CPT_MENU || (Data->CallFlags & CPT_MASK)==CPT_CONFIGURE)
		{
			int Type = (Data->CallFlags & CPT_MASK)==CPT_MENU ? MTYPE_COMMANDSMENU : MTYPE_CONFIGSMENU;
			const GUID *Guids = MenuItemGuids(Type, &Info);

			auto ItemFound = false;
			if (UseMenuGuids ? !Data->ItemUuid : !Data->ItemNumber) // 0 means "not specified"
			{
				if (MenuItemsCount == 1)
				{
					Data->FoundItemNumber = 0;
					if (UseMenuGuids) {
						Data->FoundUuid = Guids[0];
					}
					ItemFound = true;
				}
			}
			else
			{
				if (!UseMenuGuids) {
					if (Data->ItemNumber <= MenuItemsCount) {
						Data->FoundItemNumber = Data->ItemNumber - 1; // 1-based on the user side
						ItemFound = true;
					}
				}
				else {
					for (int ii = 0; ii < MenuItemsCount; ii++) {
						if (!memcmp(Data->ItemUuid, &Guids[ii], sizeof(GUID))) {
							Data->FoundUuid = *Data->ItemUuid;
							Data->FoundItemNumber = ii;
							ItemFound = true;
							break;
						}
					}
				}
			}
			if (!ItemFound)
				return false;
		}

		return true;
	}

	if (!Data->pPlugin)
		return false;

	PHPTR hPlugin = nullptr;
	Panel* ActivePanel = CtrlObject->Cp()->ActivePanel;

	switch (Data->CallFlags & CPT_MASK)
	{
	case CPT_MENU:
		{
			auto OpenCode = OPEN_PLUGINSMENU;
			void *Item = (void*)Data->FoundItemNumber;
			if (UseMenuGuids) {
				Item = &Data->FoundUuid;
			}
			OpenDlgPluginData pd { sizeof(pd) };

			if (IsEditor)
			{
				OpenCode = OPEN_EDITOR;
			}
			else if (IsViewer)
			{
				OpenCode = OPEN_VIEWER;
			}
			else if (IsDialog)
			{
				OpenCode = OPEN_DIALOG;
				if (!UseMenuGuids) {
					pd.ItemNumber = Data->FoundItemNumber;
				}
				else {
					pd.ItemGuid = Data->FoundUuid;
				}
				pd.hDlg = reinterpret_cast<Dialog*>(TopFrame);
				Item = &pd;
			}

			hPlugin = OpenPlugin(Data->pPlugin, OpenCode, Item);
			Result = true;
		}
		break;

	case CPT_CONFIGURE:
		ConfigureCurrent(Data->pPlugin, Data->FoundItemNumber, &Data->FoundUuid);
		return true;

	case CPT_CMDLINE:
		{
			const FARString command = Data->Command; // Нужна копия строки
			hPlugin = OpenPlugin(Data->pPlugin, OPEN_COMMANDLINE, command.CPtr());
			Result = true;
		}
		break;

	default:
		break;
	}

	if (hPlugin && !IsEditor && !IsViewer && !IsDialog)
	{
		//BUGBUG: Закрытие панели? Нужно ли оно?
		//BUGBUG: В ProcessCommandLine зовется перед Open, а в CPT_MENU - после
		if (ActivePanel->ProcessPluginEvent(FE_CLOSE, nullptr))
		{
			ClosePanel(hPlugin);
			return false;
		}

		const auto NewPanel = CtrlObject->Cp()->ChangePanel(ActivePanel, FILE_PANEL, true, true);
		NewPanel->SetPluginMode(hPlugin, {}, true);
		NewPanel->Update(0);
		NewPanel->Show();
	}

	// restore title for old plugins only.
	if (Data->pPlugin->IsOemPlugin() && IsEditor && CurEditor)
	{
		CurEditor->SetPluginTitle(nullptr);
	}

	return Result;
}

Plugin *PluginManager::FindPlugin(DWORD SysId)
{
	return SysIdMap.count(SysId) ? SysIdMap[SysId] : nullptr;
}

PHPTR PluginManager::OpenPlugin(Plugin *pPlugin, int OpenFrom, const void *Item)
{
	HANDLE hPanel = pPlugin->OpenPlugin(OpenFrom, Item);

	bool Creating = (OpenFrom == OPEN_FROMMACRO) ?
		(hPanel != nullptr) : (hPanel != INVALID_HANDLE_VALUE);

	return Creating ? new PanelHandle(hPanel, pPlugin) : nullptr;
}

void PluginManager::GetCustomData(FileListItem *ListItem)
{
	FARString FilePath(NTPath(ListItem->strName).Get());

	for (int i = 0; i<PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		wchar_t *CustomData = nullptr;

		if (pPlugin->HasGetCustomData() && pPlugin->GetCustomData(FilePath.CPtr(), &CustomData))
		{
			if (!ListItem->strCustomData.IsEmpty())
				ListItem->strCustomData += L" ";
			ListItem->strCustomData += CustomData;

			if (pPlugin->HasFreeCustomData())
				pPlugin->FreeCustomData(CustomData);
		}
	}
}

bool PluginManager::MayExitFar()
{
	for (int i = 0; i<PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasMayExitFAR() && !pPlugin->MayExitFAR())
			return false;
	}

	return true;
}

static void OnBackgroundTasksChangedSynched()
{
	if (FrameManager)
		FrameManager->RefreshFrame();
}

void PluginManager::BackgroundTaskStarted(const wchar_t *Info)
{
	{
		std::lock_guard<std::mutex> lock(BgTasks);
		auto ir = BgTasks.emplace(Info, 0);
		ir.first->second++;
		fprintf(stderr, "PluginManager::BackgroundTaskStarted('%ls') - count=%d\n", Info, ir.first->second);
	}

	InterThreadCallAsync(std::bind(OnBackgroundTasksChangedSynched));
}

void PluginManager::BackgroundTaskFinished(const wchar_t *Info)
{
	{
		std::lock_guard<std::mutex> lock(BgTasks);
		auto it = BgTasks.find(Info);
		if (it == BgTasks.end())
		{
			fprintf(stderr, "PluginManager::BackgroundTaskFinished('%ls') - no such task!\n", Info);
			return;
		}

		it->second--;
		fprintf(stderr, "PluginManager::BackgroundTaskFinished('%ls') - count=%d\n", Info, it->second);
		if (it->second == 0)
			BgTasks.erase(it);
	}

	InterThreadCallAsync(std::bind(OnBackgroundTasksChangedSynched));
}

bool PluginManager::HasBackgroundTasks()
{
	std::lock_guard<std::mutex> lock(BgTasks);
	return !BgTasks.empty();
}

std::map<std::wstring, unsigned int> PluginManager::BackgroundTasks()
{
	std::lock_guard<std::mutex> lock(BgTasks);
	return BgTasks;
}

static void ReadCache(KeyFileReadSection& kfh, const char *Fmt, std::vector<FARString>& Items)
{
	for (int J = 0; ; J++)
	{
		const std::string& key = StrPrintf(Fmt, J);
		if (!kfh.HasKey(key))
			break;
		Items.emplace_back(kfh.GetString(key, ""));
	}
}

class Sizer {
private:
	char  *mBuf;
	size_t mRest;
	size_t mSize;

public:
	Sizer(char *aBuf, size_t aRest, size_t aSize) : mBuf(aBuf), mRest(aRest), mSize(aSize) {}

public:
	void* BufReserve(size_t Count);
	wchar_t* StrToBuf(const FARString& Str);
	void ItemsToBuf(const wchar_t* const* &Strings, int& Count, const std::vector<FARString>& NamesArray);
	size_t GetSize() const { return mSize; }
};

void* Sizer::BufReserve(size_t Count)
{
	void* Res = nullptr;

	if (mBuf)
	{
		if (mRest >= Count)
		{
			Res = mBuf;
			mBuf += Count;
			mRest -= Count;
		}
		else
		{
			mBuf += mRest;
			mRest = 0;
		}
	}

	mSize += Count;
	return Res;
}

wchar_t* Sizer::StrToBuf(const FARString& Str)
{
	const auto Count = (Str.GetLength() + 1) * sizeof(wchar_t);
	const auto Res = reinterpret_cast<wchar_t*> (BufReserve(Count));
	if (Res)
	{
		wcscpy(Res, Str.CPtr());
	}
	return Res;
}

void Sizer::ItemsToBuf(const wchar_t* const* &Strings, int& Count, const std::vector<FARString>& NamesArray)
{
	Count = NamesArray.size();
	Strings = nullptr;

	if (Count)
	{
		const auto Items = reinterpret_cast<wchar_t**>(BufReserve(Count * sizeof(wchar_t*)));
		Strings = Items;

		for (int i = 0; i < Count; ++i)
		{
			wchar_t* pStr = StrToBuf(NamesArray[i]);
			if (Items)
			{
				Items[i] = pStr;
			}
		}
	}
}

size_t PluginManager::GetPluginInformation(Plugin *pPlugin, FarGetPluginInformation *pInfo, size_t BufferSize)
{
	if (!FindPlugin(pPlugin))
		return 0;

	FARString Prefix;
	DWORD Flags = 0, SysID = 0;
	std::vector<FARString> MenuItems, DiskItems, ConfItems;

	if (pPlugin->CheckWorkFlags(PIWF_CACHED))
	{
		KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
		Prefix = kfh.GetString("CommandPrefix", "");
		Flags = kfh.GetUInt("Flags", 0);
		SysID = kfh.GetUInt("SysID", 0);
		ReadCache(kfh, FmtPluginMenuStringD, MenuItems);
		ReadCache(kfh, FmtDiskMenuStringD, DiskItems);
		ReadCache(kfh, FmtPluginConfigStringD, ConfItems);
	}
	else
	{
		PluginInfo Info = {sizeof(Info)};
		if (pPlugin->GetPluginInfo(&Info))
		{
			Prefix = NullToEmpty(Info.CommandPrefix);
			Flags = Info.Flags;
			SysID = Info.SysID;
			for (int i = 0; i<Info.PluginMenuStringsNumber; i++)
				MenuItems.emplace_back(Info.PluginMenuStrings[i]);

			for (int i = 0; i<Info.DiskMenuStringsNumber; i++)
				DiskItems.emplace_back(Info.DiskMenuStrings[i]);

			for (int i = 0; i<Info.PluginConfigStringsNumber; i++)
				ConfItems.emplace_back(Info.PluginConfigStrings[i]);
		}
	}

	struct
	{
		FarGetPluginInformation fgpi;
		PluginInfo PInfo;
		GlobalInfo GInfo;
	} Temp;

	char  *x_Buffer = nullptr;
	size_t x_Rest = 0;
	size_t x_Size = sizeof(Temp);

	if (pInfo && BufferSize >= x_Size)
	{
		x_Rest = BufferSize - x_Size;
		x_Buffer = reinterpret_cast<char*>(pInfo) + x_Size;
	}
	else
	{
		pInfo = &Temp.fgpi;
	}
	Sizer sizer(x_Buffer, x_Rest, x_Size);

	pInfo->PInfo = reinterpret_cast<PluginInfo*>(pInfo+1);
	pInfo->GInfo = reinterpret_cast<GlobalInfo*>(pInfo->PInfo+1);
	pInfo->ModuleName = sizer.StrToBuf(pPlugin->GetModuleName());

	pInfo->Flags = 0;

	if (pPlugin->IsLoaded())
	{
		pInfo->Flags |= FPF_LOADED;
	}

	if (pPlugin->IsOemPlugin())
	{
		pInfo->Flags |= FPF_ANSI;
	}

	pInfo->GInfo->StructSize = sizeof(GlobalInfo);
	pInfo->GInfo->SysID = SysID;
	pInfo->GInfo->Version = pPlugin->m_PlugVersion;
	pInfo->GInfo->Title = sizer.StrToBuf(pPlugin->strTitle);
	pInfo->GInfo->Description = sizer.StrToBuf(pPlugin->strDescription);
	pInfo->GInfo->Author = sizer.StrToBuf(pPlugin->strAuthor);
	pInfo->GInfo->UseMenuGuids = pPlugin->UseMenuGuids() ? 1 : 0;

	pInfo->PInfo->StructSize = sizeof(PluginInfo);
	pInfo->PInfo->Flags = Flags;
	pInfo->PInfo->SysID = SysID;
	pInfo->PInfo->CommandPrefix = sizer.StrToBuf(Prefix);

	sizer.ItemsToBuf(pInfo->PInfo->PluginMenuStrings, pInfo->PInfo->PluginMenuStringsNumber, MenuItems);
	sizer.ItemsToBuf(pInfo->PInfo->DiskMenuStrings, pInfo->PInfo->DiskMenuStringsNumber, DiskItems);
	sizer.ItemsToBuf(pInfo->PInfo->PluginConfigStrings, pInfo->PInfo->PluginConfigStringsNumber, ConfItems);

	return sizer.GetSize();
}

void PluginManager::ShowPluginInfo(Plugin *pPlugin, int nItem, const GUID &Guid)
{
	const auto strPluginId = FARString().Format(L"0x%08X", pPlugin->GetSysID());
	FARString strPluginPrefix;
	if (pPlugin->CheckWorkFlags(PIWF_CACHED))
	{
		KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
		if (kfh.SectionLoaded())
			strPluginPrefix = kfh.GetString("CommandPrefix");
	}
	else
	{
		PluginInfo Info = {sizeof(Info)};
		if (pPlugin->GetPluginInfo(&Info))
			strPluginPrefix = NullToEmpty(Info.CommandPrefix);
	}
	const int Width = 36;
	DialogBuilder Builder(Msg::MPluginInformation, L"ShowPluginInfo");
	Builder.SetId(PluginInformationId);

	auto SetEditData = [&] (const FARString &str)
	{
		Builder.AddConstEditField(str.IsEmpty() ? Msg::MDataNotAvailable:str, Width);
	};

	Builder.AddText(Msg::MPluginModuleTitle);
	SetEditData(pPlugin->strTitle);

	Builder.AddText(Msg::MPluginDescription);
	SetEditData(pPlugin->strDescription);

	Builder.AddText(Msg::MPluginAuthor);
	SetEditData(pPlugin->strAuthor);

	Builder.AddText(Msg::MPluginVersion);
	const auto& Ver = pPlugin->m_PlugVersion;
	Builder.AddConstEditField(FARString().Format(L"%u.%u.%u.%u", Ver.Major,Ver.Minor,Ver.Revision,Ver.Build), Width);

	Builder.AddText(Msg::MPluginModulePath);
	Builder.AddConstEditField(pPlugin->GetModuleName(), Width);

	Builder.AddText(Msg::MPluginID);
	Builder.AddConstEditField(strPluginId, Width);

	if (pPlugin->UseMenuGuids())
	{
		Builder.AddText(Msg::MPluginItemUUID);
		Builder.AddConstEditField(GuidToString(Guid), Width);
	}
	else
	{
		Builder.AddText(Msg::MPluginItemNumber);
		Builder.AddConstEditField(FARString().Format(L"%d", nItem), Width);
	}

	Builder.AddText(Msg::MPluginPrefix);
	Builder.AddConstEditField(strPluginPrefix, Width);

	Builder.AddOK();
	Builder.ShowDialog();
}
