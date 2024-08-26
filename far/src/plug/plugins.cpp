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
#include "processname.hpp"
#include "interf.hpp"
#include "filelist.hpp"
#include "message.hpp"
#include "SafeMMap.hpp"
#include "HotkeyLetterDialog.hpp"
#include "InterThreadCall.hpp"
#include "DlgGuid.hpp"
#include <KeyFileHelper.h>
#include "DialogBuilder.hpp"
#include <crc64.h>
#include <assert.h>

const char *FmtDiskMenuStringD = "DiskMenuString%d";
const char *FmtPluginMenuStringD = "PluginMenuString%d";
const char *FmtPluginConfigStringD = "PluginConfigString%d";
const char *SettingsSection = "Settings";
const wchar_t *PluginsFolderName = L"Plugins";

static int _cdecl PluginsSort(const void *el1,const void *el2);

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


PluginManager::PluginManager():
	PluginsData(nullptr),
	PluginsCount(0),
	OemPluginsCount(0),
	CurEditor(nullptr),
	CurViewer(nullptr)
{
}

PluginManager::~PluginManager()
{
	Plugin *pLuaMacro=nullptr; //to be deleted last

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
	if(PluginsData)
	{
		free(PluginsData);
	}
}

bool PluginManager::AddPlugin(Plugin *pPlugin)
{
	Plugin **NewPluginsData=(Plugin**)realloc(PluginsData,sizeof(*PluginsData)*(PluginsCount+1));

	if (!NewPluginsData)
		return false;

	PluginsData = NewPluginsData;
	PluginsData[PluginsCount]=pPlugin;
	PluginsCount++;
	if(pPlugin->IsOemPlugin())
	{
		OemPluginsCount++;
	}
	return true;
}

bool PluginManager::RemovePlugin(Plugin *pPlugin)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		if (PluginsData[i] == pPlugin)
		{
			SysIdMap.erase(pPlugin->SysID);
			if(pPlugin->IsOemPlugin())
			{
				OemPluginsCount--;
			}
			delete pPlugin;
			memmove(&PluginsData[i], &PluginsData[i+1], (PluginsCount-i-1)*sizeof(Plugin*));
			PluginsCount--;
			return true;
		}
	}

	return false;
}


Plugin* PluginManager::LoadPlugin(
    const FARString &strModuleName,
    bool UncachedLoad
)
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

bool PluginManager::LoadPluginExternal(const wchar_t *lpwszModuleName, bool LoadToMem)
{
	return LoadPluginExternalV3(lpwszModuleName, LoadToMem) != nullptr;
}

Plugin* PluginManager::LoadPluginExternalV3(const wchar_t *lpwszModuleName, bool LoadToMem)
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
		Frame *frame;

		if ((frame = FrameManager->GetBottomFrame()) )
			frame->Unlock();

		if (Flags.Check(PSIF_DIALOG))   // BugZ#52 exception handling for floating point incorrect
		{
			Flags.Clear(PSIF_DIALOG);
			FrameManager->DeleteFrame();
			FrameManager->Commit();
		}

		bool bPanelPlugin = pPlugin->IsPanelPlugin();

		if (dwException != (DWORD)-1)
			nResult = pPlugin->Unload(true);
		else
			nResult = pPlugin->Unload(false);

		if (bPanelPlugin /*&& bUpdatePanels*/)
		{
			CtrlObject->Cp()->ActivePanel->SetCurDir(L".",true);
			Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
			ActivePanel->Update(UPDATE_KEEP_SELECTION);
			ActivePanel->Redraw();
			Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);
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
	int nResult = FALSE;
	Plugin *pPlugin = FindPlugin(lpwszModuleName);

	if (pPlugin)
	{
		nResult = pPlugin->Unload(true);
		RemovePlugin(pPlugin);
	}

	return nResult;
}

int PluginManager::UnloadPluginExternalV3(Plugin* pPlugin)
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

Plugin *PluginManager::FindPlugin(Plugin *pPlugin)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		if (pPlugin == PluginsData[i])
			return pPlugin;
	}
	return nullptr;
}

Plugin *PluginManager::GetPlugin(int PluginNumber)
{
	if (PluginNumber < PluginsCount && PluginNumber >= 0)
		return PluginsData[PluginNumber];

	return nullptr;
}

void PluginManager::LoadPlugins()
{
	Flags.Clear(PSIF_PLUGINSLOADDED);

	if (Opt.LoadPlug.PluginsCacheOnly)  // $ 01.09.2000 tran  '/co' switch
	{
		LoadPluginsFromCache();
	}
	else if (Opt.LoadPlug.MainPluginDir
			|| !Opt.LoadPlug.strCustomPluginsPath.IsEmpty()
			|| (Opt.LoadPlug.PluginsPersonal && !Opt.LoadPlug.strPersonalPluginsPath.IsEmpty()))
	{
		ScanTree ScTree(FALSE,TRUE,Opt.LoadPlug.ScanSymlinks);
		UserDefinedList PluginPathList(ULF_UNIQUE | ULF_CASESENSITIVE, L':', L';');  // хранение списка каталогов
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
					&& !Opt.LoadPlug.strPersonalPluginsPath.IsEmpty()
					&& !(Opt.Policies.DisabledOptions & FFPOL_PERSONALPATH))
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
		for (size_t PPLI = 0; nullptr!=(NamePtr=PluginPathList.Get(PPLI)); ++PPLI)
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

	Flags.Set(PSIF_PLUGINSLOADDED);

	far_qsort(PluginsData, PluginsCount, sizeof(*PluginsData), PluginsSort);
}

/* $ 01.09.2000 tran
   Load cache only plugins  - '/co' switch */
void PluginManager::LoadPluginsFromCache()
{
	KeyFileReadHelper kfh(PluginsIni());
	const std::vector<std::string> &sections = kfh.EnumSections();
	FARString strModuleName;
	for (const auto &s : sections)
	{
		if (s != SettingsSection)
		{
			const std::string &module = kfh.GetString(s, "Module");
			if (!module.empty()) {
				strModuleName = module;
				LoadPlugin(strModuleName, false);
			}
		}
	}
}

int _cdecl PluginsSort(const void *el1,const void *el2)
{
	Plugin *Plugin1=*((Plugin**)el1);
	Plugin *Plugin2=*((Plugin**)el2);
	return (StrCmpI(PointToName(Plugin1->GetModuleName()),PointToName(Plugin2->GetModuleName())));
}

PHPTR PluginManager::OpenFilePlugin(
    const wchar_t *Name,
    int OpMode,
    OPENFILEPLUGINTYPE Type,
    Plugin *pDesiredPlugin
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	ConsoleTitle ct(Opt.ShowCheckingFile ? Msg::CheckingFileInPlugin.CPtr() : nullptr);
	PHPTR hResult = nullptr;
	PHPTR pResult = nullptr;
	std::vector<PanelHandle> items;
	FARString strFullName;

	if (Name)
	{
		ConvertNameToFull(Name,strFullName);
		Name = strFullName;
	}

	bool ShowMenu = Opt.PluginConfirm.OpenFilePlugin == BSTATE_3STATE ?
			!(Type == OFP_NORMAL || Type == OFP_SEARCH) : Opt.PluginConfirm.OpenFilePlugin != 0;
	if (Type==OFP_ALTERNATIVE) OpMode|= OPM_PGDN;
	if (Type==OFP_COMMANDS) OpMode|= OPM_COMMANDS;

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

		if(Name && !smm)
		{
			try
			{
				smm.reset(new SafeMMap(Wide2MB(Name).c_str(), SafeMMap::M_READ, Opt.PluginMaxReadData));
			}
			catch (std::exception &e)
			{
				fprintf(stderr, "PluginManager::OpenFilePlugin: %s\n", e.what());

				if(!OpMode)
				{
					Message(MSG_WARNING|MSG_ERRORTYPE, 1, MB2Wide(e.what()).c_str(),
						Msg::OpenPluginCannotOpenFile, Name, Msg::Ok);
				}
				break;
			}
		}

		if (pPlugin->HasOpenFilePlugin())
		{
			if (Opt.ShowCheckingFile)
				ct.Set(L"%ls - [%ls]...", Msg::CheckingFileInPlugin.CPtr(), PointToName(pPlugin->GetModuleName()));

			HANDLE hPlugin = pPlugin->OpenFilePlugin(Name,
				smm ? (const unsigned char *)smm->View() : nullptr,
				smm ? (DWORD)smm->Length() : 0, OpMode);

			if (hPlugin == PANEL_STOP)   //сразу на выход, плагин решил нагло обработать все сам (Autorun/PictureView)!!!
			{
				hResult = PHPTR_STOP;
				break;
			}

			if (hPlugin != INVALID_HANDLE_VALUE)
			{
				items.emplace_back(hPlugin, pPlugin);
			}
		}
		else
		{
			AnalyseData AData;
			AData.FileName = Name;
			AData.Buffer = smm ? smm->View() : nullptr;
			AData.BufferSize = smm ? smm->Length() : 0;
			AData.OpMode = OpMode;

			if (pPlugin->Analyse(&AData))
			{
				items.emplace_back(INVALID_HANDLE_VALUE, pPlugin);
			}
		}

		if (!items.empty() && !ShowMenu)
			break;
	}

	if (!items.empty() && (hResult != PHPTR_STOP))
	{
		bool OnlyOne = (items.size() == 1) && !(Name && Opt.PluginConfirm.OpenFilePlugin &&
			Opt.PluginConfirm.StandardAssociation && Opt.PluginConfirm.EvenIfOnlyOnePlugin);

		if(!OnlyOne && ShowMenu)
		{
			VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY-4);
			menu.SetPosition(-1, -1, 0, 0);
			menu.SetHelp(L"ChoosePluginMenu");
			menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
			MenuItemEx mitem;

			for (size_t i = 0; i < items.size(); i++)
			{
				PanelHandle *handle = &items[i];
				mitem.Clear();
				mitem.strName = PointToName(handle->pPlugin->GetModuleName());
				//NB: here is really should be used sizeof(handle), not sizeof(*handle)
				//cuz sizeof(void *) has special meaning in SetUserData!
				menu.SetUserData(handle, sizeof(handle), menu.AddItem(&mitem));
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
				pResult = (PanelHandle*)menu.GetUserData(nullptr, 0);
		}
		else
		{
			pResult = &items.front();
		}

		if (pResult && pResult->hPanel == INVALID_HANDLE_VALUE)
		{
			HANDLE h = pResult->pPlugin->OpenPlugin(OPEN_ANALYSE, reinterpret_cast<INT_PTR>(Name));

			if (h != INVALID_HANDLE_VALUE)
				pResult->hPanel = h;
			else
				pResult = nullptr;
		}
	}

	for (const auto& PH: items)
	{
		if (&PH != pResult && PH.hPanel != INVALID_HANDLE_VALUE)
			PH.pPlugin->ClosePlugin(PH.hPanel);
	}

	if (pResult)
	{
		hResult = new PanelHandle(pResult->hPanel, pResult->pPlugin);
	}

	return hResult;
}

PHPTR PluginManager::OpenFindListPlugin(const PluginPanelItem *PanelItem, int ItemsNumber)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	PanelHandle *pResult = nullptr;
	std::vector<PanelHandle> items;
	Plugin *pPlugin=nullptr;

	for (int i = 0; i < PluginsCount; i++)
	{
		pPlugin = PluginsData[i];

		if (!pPlugin->HasSetFindList())
			continue;

		HANDLE hPlugin = pPlugin->OpenPlugin(OPEN_FINDLIST, 0);

		if (hPlugin != INVALID_HANDLE_VALUE)
		{
			items.emplace_back(hPlugin, pPlugin);
		}

		if (!items.empty() && !Opt.PluginConfirm.SetFindList)
			break;
	}

	if (!items.empty())
	{
		if (items.size() > 1)
		{
			VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY-4);
			menu.SetPosition(-1, -1, 0, 0);
			menu.SetHelp(L"ChoosePluginMenu");
			menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
			MenuItemEx mitem;

			for (size_t i=0; i<items.size(); i++)
			{
				PanelHandle *handle = &items[i];
				mitem.Clear();
				mitem.strName=PointToName(handle->pPlugin->GetModuleName());
				menu.AddItem(&mitem);
			}

			menu.Show();

			while (!menu.Done())
			{
				menu.ReadInput();
				menu.ProcessInput();
			}

			int ExitCode=menu.GetExitCode();

			if (ExitCode>=0)
			{
				pResult = &items[ExitCode];
			}
		}
		else
		{
			pResult = &items.front();
		}
	}

	if (pResult)
	{
		if (!pResult->pPlugin->SetFindList(pResult->hPanel, PanelItem, ItemsNumber))
		{
			pResult=nullptr;
		}
	}

	for (size_t i=0; i<items.size(); i++)
	{
		PanelHandle *handle = &items[i];

		if (handle!=pResult)
		{
			if (handle->hPanel!=INVALID_HANDLE_VALUE)
				handle->pPlugin->ClosePlugin(handle->hPanel);
		}
	}

	if (pResult)
	{
		pResult = new PanelHandle(pResult->hPanel, pResult->pPlugin);
	}

	return pResult;
}


void PluginManager::ClosePanel(PHPTR ph)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	const auto RefCnt = ph->RefCnt;
	assert(RefCnt > 0);
	ph->RefCnt = RefCnt - 1;
	if (RefCnt == 1) {
		ph->pPlugin->ClosePlugin(ph->hPanel);
		delete ph;
	}
}

void PluginManager::RetainPanel(PHPTR ph)
{
	const auto RefCnt = ph->RefCnt;
	assert(RefCnt > 0);
	ph->RefCnt = RefCnt + 1;
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
	int nResult = 0;

	if (CtrlObject->Plugins.CurEditor)
	{
		if (Event == EE_REDRAW)
		{
			CtrlObject->Plugins.CurEditor->AutoDeleteColors();
		}

		for (int i = 0; i < PluginsCount; i++)
		{
			Plugin *pPlugin = PluginsData[i];

			if (pPlugin->HasProcessEditorEvent())
				nResult = pPlugin->ProcessEditorEvent(Event, Param);
		}
	}

	return nResult;
}


int PluginManager::ProcessViewerEvent(int Event, void *Param)
{
	int nResult = 0;

	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessViewerEvent())
			nResult = pPlugin->ProcessViewerEvent(Event, Param);
	}

	return nResult;
}

int PluginManager::ProcessDialogEvent(int Event, void *Param)
{
	for (int i=0; i<PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessDialogEvent() && pPlugin->ProcessDialogEvent(Event,Param))
			return TRUE;
	}

	return FALSE;
}

int PluginManager::GetFindData(
    PHPTR ph,
    PluginPanelItem **pPanelData,
    int *pItemsNumber,
    int OpMode
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	*pItemsNumber = 0;
	return ph->pPlugin->GetFindData(ph->hPanel, pPanelData, pItemsNumber, OpMode);
}


void PluginManager::FreeFindData(
    PHPTR ph,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	ph->pPlugin->FreeFindData(ph->hPanel, PanelItem, ItemsNumber);
}


int PluginManager::GetVirtualFindData(
    PHPTR ph,
    PluginPanelItem **pPanelData,
    int *pItemsNumber,
    const wchar_t *Path
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	*pItemsNumber=0;
	return ph->pPlugin->GetVirtualFindData(ph->hPanel, pPanelData, pItemsNumber, Path);
}


void PluginManager::FreeVirtualFindData(
    PHPTR ph,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	return ph->pPlugin->FreeVirtualFindData(ph->hPanel, PanelItem, ItemsNumber);
}


int PluginManager::SetDirectory(
    PHPTR ph,
    const wchar_t *Dir,
    int OpMode
)
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


int PluginManager::GetFile(
    PHPTR ph,
    PluginPanelItem *PanelItem,
    const wchar_t *DestPath,
    FARString &strResultName,
    int OpMode
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SaveScreen *SaveScr=nullptr;
	int Found=FALSE;
	KeepUserScreen=FALSE;

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
			Found=TRUE;
	}

	ReadUserBackgound(SaveScr);
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
	KeepUserScreen=FALSE;
	int Code = ph->pPlugin->DeleteFiles(ph->hPanel, PanelItem, ItemsNumber, OpMode);

	if (Code)
		ReadUserBackgound(&SaveScr); //???

	return Code;
}


int PluginManager::MakeDirectory(
    PHPTR ph,
    const wchar_t **Name,
    int OpMode
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen=FALSE;
	int Code = ph->pPlugin->MakeDirectory(ph->hPanel, Name, OpMode);

	if (Code != -1)   //???BUGBUG
		ReadUserBackgound(&SaveScr);

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
	KeepUserScreen=FALSE;
	int Code = ph->pPlugin->ProcessHostFile(ph->hPanel, PanelItem, ItemsNumber, OpMode);

	if (Code)   //BUGBUG
		ReadUserBackgound(&SaveScr);

	return Code;
}

bool PluginManager::GetLinkTarget(PHPTR ph, PluginPanelItem *PanelItem, FARString &result, int OpMode)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	return ph->pPlugin->GetLinkTarget(ph->hPanel, PanelItem, result, OpMode);
}

int PluginManager::GetFiles(
    PHPTR ph,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    const wchar_t **DestPath,
    int OpMode
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	return ph->pPlugin->GetFiles(ph->hPanel, PanelItem, ItemsNumber, Move, DestPath, OpMode);
}


int PluginManager::PutFiles(
    PHPTR ph,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    int OpMode
)
{
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen=FALSE;
	int Code = ph->pPlugin->PutFiles(ph->hPanel, PanelItem, ItemsNumber, Move, OpMode);

	if (Code)   //BUGBUG
		ReadUserBackgound(&SaveScr);

	return Code;
}

void PluginManager::GetOpenPluginInfo(
    PHPTR ph,
    OpenPluginInfo *Info
)
{
	if (!Info)
		return;

	memset(Info, 0, sizeof(*Info));
	ph->pPlugin->GetOpenPluginInfo(ph->hPanel, Info);

	if (!Info->CurDir)  //хмм...
		Info->CurDir = L"";

	if ((Info->Flags & OPIF_REALNAMES) && (CtrlObject->Cp()->ActivePanel->GetPluginHandle() == ph) && *Info->CurDir && !IsNetworkServerPath(Info->CurDir))
		apiSetCurrentDirectory(Info->CurDir, false);
}


int PluginManager::ProcessKey(
    PHPTR ph,
    int Key,
    unsigned int ControlState
)
{
	return ph->pPlugin->ProcessKey(ph->hPanel, Key, ControlState);
}


int PluginManager::ProcessEvent(
    PHPTR ph,
    int Event,
    void *Param
)
{
	return ph->pPlugin->ProcessEvent(ph->hPanel, Event, Param);
}


int PluginManager::Compare(
    PHPTR ph,
    const PluginPanelItem *Item1,
    const PluginPanelItem *Item2,
    unsigned int Mode
)
{
	return ph->pPlugin->Compare(ph->hPanel, Item1, Item2, Mode);
}

void PluginManager::ConfigureCurrent(Plugin *pPlugin, int INum, const GUID *Guid)
{
	int Result = FALSE;
	if (pPlugin->IsLuamacro()) {
		Guid = Guid ? Guid : &FarGuid;
		ConfigureInfo Info = { sizeof(ConfigureInfo), Guid };
		Result = dynamic_cast<PluginW*>(pPlugin)->ConfigureV3(&Info);
	}
	else {
		Result = pPlugin->Configure(INum);
	}

	if (Result)
	{
		int PMode[2];
		PMode[0]=CtrlObject->Cp()->LeftPanel->GetMode();
		PMode[1]=CtrlObject->Cp()->RightPanel->GetMode();

		for (size_t I=0; I < ARRAYSIZE(PMode); ++I)
		{
			if (PMode[I] == PLUGIN_PANEL)
			{
				Panel *pPanel=(I?CtrlObject->Cp()->RightPanel:CtrlObject->Cp()->LeftPanel);
				pPanel->Update(UPDATE_KEEP_SELECTION);
				pPanel->SetViewMode(pPanel->GetViewMode());
				pPanel->Redraw();
			}
		}
		pPlugin->SaveToCache();
	}
}

struct PluginMenuItemData
{
	Plugin *pPlugin;
	int nItem;
	GUID Guid;
};

/* $ 29.05.2001 IS
   ! При настройке "параметров внешних модулей" закрывать окно с их
     списком только при нажатии на ESC
*/

bool PluginManager::CheckIfHotkeyPresent(MENUTYPE MenuType)
{
	bool IsConfig = (MenuType == MTYPE_CONFIGSMENU);
	const char *Fmt = IsConfig ? FmtPluginConfigStringD : FmtPluginMenuStringD;

	for (int I=0; I<PluginsCount; I++)
	{
		Plugin *pPlugin = PluginsData[I];
		PluginInfo Info{};
		bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED);
		if (!bCached && !pPlugin->GetPluginInfo(&Info))
		{
			continue;
		}

		for (int J = 0; ; ++J)
		{
			if (bCached)
			{
				KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
				if (!kfh.HasKey(StrPrintf(Fmt, J)))
					break;
			}
			else if (J >= (IsConfig ? Info.PluginConfigStringsNumber:Info.PluginMenuStringsNumber))
			{
				break;
			}

			FARString strHotKey;
			const GUID *Guid = pPlugin->IsLuamacro() ? MenuItemGuids(MenuType, &Info) + J : nullptr;
			GetPluginHotKey(pPlugin, J, Guid, MenuType, strHotKey);
			if (!strHotKey.IsEmpty())
			{
				return true;
			}
		}
	}
	return false;
}

void PluginManager::Configure(int StartPos)
{
	// Полиция 4 - Параметры внешних модулей
	if (Opt.Policies.DisabledOptions&FFPOL_MAINMENUPLUGINS)
		return;

	{
		ChangeMacroArea Cma(MACROAREA_MENU);
		VMenu PluginList(Msg::PluginConfigTitle,nullptr,0,ScrY-4);
		PluginList.SetFlags(VMENU_WRAPMODE);
		PluginList.SetHelp(L"PluginsConfig");
		PluginList.SetId(PluginsConfigMenuId);

		for (;;)
		{
			BOOL NeedUpdateItems=TRUE;
			int MenuItemNumber=0;

			Cma.SetPrevArea(); // for plugins: set the right macro area in GetPluginInfo()
			bool HotKeysPresent = CheckIfHotkeyPresent(MTYPE_CONFIGSMENU);
			Cma.SetCurArea();

			if (NeedUpdateItems)
			{
				PluginList.ClearDone();
				PluginList.DeleteItems();
				PluginList.SetPosition(-1,-1,0,0);
				MenuItemNumber=0;
				LoadIfCacheAbsent();
				FARString strHotKey, strValue, strName;
				PluginInfo Info{};

				Cma.SetPrevArea(); // for plugins: set the right macro area in GetPluginInfo()
				for (int I=0; I<PluginsCount; I++)
				{
					Plugin *pPlugin = PluginsData[I];
					bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED)?true:false;

					if (!bCached && !pPlugin->GetPluginInfo(&Info))
					{
						continue;
					}

					for (int J=0; ; J++)
					{
						if (bCached)
						{
							KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
							const std::string &key = StrPrintf(FmtPluginConfigStringD, J);
							if (!kfh.HasKey(key))
								break;

							strName = kfh.GetString(key, "");
						}
						else
						{
							if (J >= Info.PluginConfigStringsNumber)
								break;

							strName = Info.PluginConfigStrings[J];
						}

						const GUID *Guid = pPlugin->IsLuamacro() ? Info.PluginConfigGuids + J : nullptr;
						GetPluginHotKey(pPlugin, J, Guid, MTYPE_CONFIGSMENU, strHotKey);
						MenuItemEx ListItem;
						ListItem.Clear();

						if (pPlugin->IsOemPlugin())
							ListItem.Flags=LIF_CHECKED|L'A';

						if (!HotKeysPresent)
							ListItem.strName = strName;
						else if (!strHotKey.IsEmpty())
							ListItem.strName.Format(L"&%lc%ls  %ls",strHotKey.At(0),(strHotKey.At(0)==L'&'?L"&":L""), strName.CPtr());
						else
							ListItem.strName.Format(L"   %ls", strName.CPtr());

						//ListItem.SetSelect(MenuItemNumber++ == StartPos);
						MenuItemNumber++;
						PluginMenuItemData item;
						item.pPlugin = pPlugin;
						item.nItem = J;
						if (pPlugin->IsLuamacro() && Info.PluginConfigGuids) {
							item.Guid = Info.PluginConfigGuids[J];
						}
						PluginList.SetUserData(&item, sizeof(PluginMenuItemData),PluginList.AddItem(&ListItem));
					}
				}
				Cma.SetCurArea();

				PluginList.AssignHighlights(FALSE);
				PluginList.SetBottomTitle(Msg::PluginHotKeyBottom);
				PluginList.ClearDone();
				PluginList.SortItems(0,HotKeysPresent?3:0);
				PluginList.SetSelectPos(StartPos,1);
				NeedUpdateItems=FALSE;
			}

			FARString strPluginModuleName;
			PluginList.Show();

			while (!PluginList.Done())
			{
				FarKey Key=PluginList.ReadInput();
				int SelPos=PluginList.GetSelectPos();
				PluginMenuItemData *item = (PluginMenuItemData*)PluginList.GetUserData(nullptr,0,SelPos);

				switch (Key)
				{
					case KEY_SHIFTF1:
						if (item)
						{
							strPluginModuleName = item->pPlugin->GetModuleName();
							if (!FarShowHelp(strPluginModuleName,L"Config",FHELP_SELFHELP|FHELP_NOSHOWERROR) &&
											!FarShowHelp(strPluginModuleName,L"Configure",FHELP_SELFHELP|FHELP_NOSHOWERROR))
							{
								FarShowHelp(strPluginModuleName,nullptr,FHELP_SELFHELP|FHELP_NOSHOWERROR);
							}
						}
						break;

					case KEY_F3:
						if (item)
						{
							ShowPluginInfo(item->pPlugin);
						}
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
								NeedUpdateItems=TRUE;
								StartPos=SelPos;
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
				StartPos=PluginList.Modal::GetExitCode();
				PluginList.Hide();

				if (StartPos<0)
					break;

				PluginMenuItemData *item = (PluginMenuItemData*)PluginList.GetUserData(nullptr,0,StartPos);
				ConfigureCurrent(item->pPlugin, item->nItem, &item->Guid);
			}
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

	int MenuItemNumber=0;
	int Editor = ModalType==MODALTYPE_EDITOR,
	             Viewer = ModalType==MODALTYPE_VIEWER,
	                      Dialog = ModalType==MODALTYPE_DIALOG;
	PluginMenuItemData item;
	{
		ChangeMacroArea Cma(MACROAREA_MENU);
		VMenu PluginList(Msg::PluginCommandsMenuTitle,nullptr,0,ScrY-4);
		PluginList.SetFlags(VMENU_WRAPMODE);
		PluginList.SetHelp(L"PluginCommands");
		PluginList.SetId(PluginsMenuId);
		BOOL NeedUpdateItems=TRUE;
		BOOL Done=FALSE;

		while (!Done)
		{
			Cma.SetPrevArea(); // for plugins: set the right macro area in GetPluginInfo()
			bool HotKeysPresent = CheckIfHotkeyPresent(MTYPE_COMMANDSMENU);
			Cma.SetCurArea();

			if (NeedUpdateItems)
			{
				PluginList.ClearDone();
				PluginList.DeleteItems();
				PluginList.SetPosition(-1,-1,0,0);
				LoadIfCacheAbsent();
				FARString strHotKey, strValue, strName;
				PluginInfo Info{};
				KeyFileReadHelper kfh(PluginsIni());

				Cma.SetPrevArea(); // for plugins: set the right macro area in GetPluginInfo()
				for (int I=0; I<PluginsCount; I++)
				{
					Plugin *pPlugin = PluginsData[I];
					bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED)?true:false;
					int IFlags;

					if (bCached)
					{
						IFlags = kfh.GetUInt(pPlugin->GetSettingsName(), "Flags",0);
					}
					else
					{
						if (!pPlugin->GetPluginInfo(&Info))
							continue;

						IFlags = Info.Flags;
					}

					if ((Editor && !(IFlags & PF_EDITOR)) ||
					        (Viewer && !(IFlags & PF_VIEWER)) ||
					        (Dialog && !(IFlags & PF_DIALOG)) ||
					        (!Editor && !Viewer && !Dialog && (IFlags & PF_DISABLEPANELS)))
						continue;

					for (int J=0; ; J++)
					{
						if (bCached)
						{
							const std::string &key = StrPrintf(FmtPluginMenuStringD, J);
							if (!kfh.HasKey(pPlugin->GetSettingsName(), key))
								break;
							strName = kfh.GetString(pPlugin->GetSettingsName(), key, "");
						}
						else
						{
							if (J >= Info.PluginMenuStringsNumber)
								break;

							strName = Info.PluginMenuStrings[J];
						}

						const GUID *Guid = pPlugin->IsLuamacro() ? Info.PluginMenuGuids + J : nullptr;
						GetPluginHotKey(pPlugin, J, Guid, MTYPE_COMMANDSMENU, strHotKey);
						MenuItemEx ListItem;
						ListItem.Clear();

						if (pPlugin->IsOemPlugin())
							ListItem.Flags=LIF_CHECKED|L'A';

						if (!HotKeysPresent)
							ListItem.strName = strName;
						else if (!strHotKey.IsEmpty())
							ListItem.strName.Format(L"&%lc%ls  %ls",strHotKey.At(0),(strHotKey.At(0)==L'&'?L"&":L""), strName.CPtr());
						else
							ListItem.strName.Format(L"   %ls", strName.CPtr());

						//ListItem.SetSelect(MenuItemNumber++ == StartPos);
						MenuItemNumber++;
						PluginMenuItemData item {};
						item.pPlugin = pPlugin;
						item.nItem = J;
						if (pPlugin->IsLuamacro() && Info.PluginMenuGuids) {
							item.Guid = Info.PluginMenuGuids[J];
						}
						PluginList.SetUserData(&item, sizeof(PluginMenuItemData),PluginList.AddItem(&ListItem));
					}
				}
				Cma.SetCurArea();

				PluginList.AssignHighlights(FALSE);
				PluginList.SetBottomTitle(Msg::PluginHotKeyBottom);
				PluginList.SortItems(0,HotKeysPresent?3:0);
				PluginList.SetSelectPos(StartPos,1);
				NeedUpdateItems=FALSE;
			}

			PluginList.Show();

			while (!PluginList.Done())
			{
				FarKey Key=PluginList.ReadInput();
				int SelPos=PluginList.GetSelectPos();
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
						{
							ShowPluginInfo(item->pPlugin);
						}
						break;

					case KEY_F4:
						if (item && PluginList.GetItemCount() > 0 && SelPos < MenuItemNumber)
						{
							FARString strName00;
							int nOffset = HotKeysPresent?3:0;
							strName00 = PluginList.GetItemPtr()->strName.CPtr()+nOffset;
							RemoveExternalSpaces(strName00);

							if (SetHotKeyDialog(strName00, item->pPlugin, item->nItem, &item->Guid, MTYPE_COMMANDSMENU))
							{
								PluginList.Hide();
								NeedUpdateItems=TRUE;
								StartPos=SelPos;
								PluginList.SetExitCode(SelPos);
								PluginList.Show();
							}
						}
						break;

					case KEY_ALTSHIFTF9:
						PluginList.Hide();
						NeedUpdateItems=TRUE;
						StartPos=SelPos;
						PluginList.SetExitCode(SelPos);
						Configure();
						PluginList.Show();
						break;

					case KEY_SHIFTF9:
						if (item && PluginList.GetItemCount() > 0 && SelPos < MenuItemNumber)
						{
							NeedUpdateItems=TRUE;
							StartPos=SelPos;

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

		int ExitCode=PluginList.Modal::GetExitCode();
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
	INT_PTR Item = item.nItem;
	if (item.pPlugin->IsLuamacro()) {
		Item = (INT_PTR)&item.Guid;
	}
	OpenDlgPluginData pd {};

	if (Editor)
	{
		OpenCode=OPEN_EDITOR;
	}
	else if (Viewer)
	{
		OpenCode=OPEN_VIEWER;
	}
	else if (Dialog)
	{
		OpenCode=OPEN_DIALOG;
		pd.hDlg=(HANDLE)FrameManager->GetCurrentFrame();
		if (item.pPlugin->IsLuamacro())
			pd.ItemGuid = item.Guid;
		else
			pd.ItemNumber = item.nItem;

		Item = (INT_PTR)&pd;
	}

	PHPTR hPlugin=OpenPlugin(item.pPlugin,OpenCode,Item);

	if (hPlugin && !Editor && !Viewer && !Dialog)
	{
		if (ActivePanel->ProcessPluginEvent(FE_CLOSE,nullptr))
		{
			ClosePanel(hPlugin);
			return FALSE;
		}

		Panel *NewPanel=CtrlObject->Cp()->ChangePanel(ActivePanel,FILE_PANEL,TRUE,TRUE);
		NewPanel->SetPluginMode(hPlugin,L"",true);
		NewPanel->Update(0);
		NewPanel->Show();
	}

	// restore title for old plugins only.
	if (item.pPlugin->IsOemPlugin() && Editor && CurEditor)
	{
		CurEditor->SetPluginTitle(nullptr);
	}

	return TRUE;
}

std::string PluginManager::GetHotKeySettingName(Plugin *pPlugin, int ItemNumber, const GUID *Guid, MENUTYPE MenuType)
{
	if (pPlugin->IsLuamacro())
	{
		const std::string &strGuid = GuidToString(*Guid);
		std::string out = StrPrintf("luamacro:%s#%s", HotKeyType(MenuType), strGuid.c_str());
		return out;
	}
	std::string out = pPlugin->GetSettingsName();
	out+= StrPrintf(":%s#%d", HotKeyType(MenuType), ItemNumber);
	return out;
}

void PluginManager::GetPluginHotKey(Plugin *pPlugin, int ItemNumber, const GUID *Guid, MENUTYPE MenuType, FARString &strHotKey)
{
	KeyFileReadSection kfh(PluginsIni(), SettingsSection);
	strHotKey = kfh.GetString(GetHotKeySettingName(pPlugin, ItemNumber, Guid, MenuType));
}

bool PluginManager::SetHotKeyDialog(
		const wchar_t *DlgPluginTitle,    // имя плагина
		Plugin *pPlugin,                  // ключ, откуда берем значение в state.ini/Settings
		int ItemNumber,                   // +
		const GUID *Guid,                 // +
		MENUTYPE MenuType)                // +
{
	const std::string &SettingName = GetHotKeySettingName(pPlugin, ItemNumber, Guid, MenuType);
	KeyFileHelper kfh(PluginsIni());
	const auto &Setting = kfh.GetString(SettingsSection, SettingName, L"");
	WCHAR Letter[2] = {Setting.empty() ? 0 : Setting[0], 0};
	if (!HotkeyLetterDialog(Msg::PluginHotKeyTitle, DlgPluginTitle, Letter[0]))
		return false;

	if (Letter[0])
		kfh.SetString(SettingsSection, SettingName, Letter);
	else
		kfh.RemoveKey(SettingsSection, SettingName);
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

	FARString strHotKey;
	if (!pPlugin->IsLuamacro()) {
		GetPluginHotKey(pPlugin, PluginItem, nullptr, MTYPE_DISKSMENU, strHotKey);
		PluginHotkey = strHotKey.At(0);
	}

	if (pPlugin->CheckWorkFlags(PIWF_CACHED))
	{
		KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
		strPluginText = kfh.GetString( StrPrintf(FmtDiskMenuStringD, PluginItem), "" );
		return !strPluginText.IsEmpty();
	}

	PluginInfo Info;

	if (pPlugin->GetPluginInfo(&Info) && Info.DiskMenuStringsNumber > PluginItem)
	{
		if (pPlugin->IsLuamacro()) {
			Guid = Info.DiskMenuGuids[PluginItem];
			GetPluginHotKey(pPlugin, PluginItem, &Guid, MTYPE_DISKSMENU, strHotKey);
			PluginHotkey = strHotKey.At(0);
		}
		strPluginText = Info.DiskMenuStrings[PluginItem];
		return true;
	}

	return false;
}

int PluginManager::UseFarCommand(PHPTR ph,int CommandType)
{
	OpenPluginInfo Info;
	GetOpenPluginInfo(ph,&Info);

	if (!(Info.Flags & OPIF_REALNAMES))
		return FALSE;

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

	return TRUE;
}


void PluginManager::ReloadLanguage()
{
	Plugin *PData;

	for (int I=0; I<PluginsCount; I++)
	{
		PData = PluginsData[I];
		PData->CloseLang();
	}

	DiscardCache();
}


void PluginManager::DiscardCache()
{
	for (int I=0; I<PluginsCount; I++)
	{
		Plugin *pPlugin = PluginsData[I];
		pPlugin->Load();
	}

	KeyFileHelper kfh(PluginsIni());
	const std::vector<std::string> &sections = kfh.EnumSections();
	for (const auto &s : sections)
	{
		if (s != SettingsSection)
			kfh.RemoveSection(s);
	}
}


void PluginManager::LoadIfCacheAbsent()
{
	struct stat st;
	if (stat(PluginsIni(), &st) == -1)
	{
		for (int I=0; I<PluginsCount; I++)
		{
			Plugin *pPlugin = PluginsData[I];
			pPlugin->Load();
		}
	}
}

//template parameters must have external linkage
struct PluginData
{
	Plugin *pPlugin;
	DWORD PluginFlags;

	PluginData() : pPlugin(nullptr), PluginFlags(0) {}
	PluginData(Plugin *plugin, DWORD flags) : pPlugin(plugin), PluginFlags(flags) {}
};

int PluginManager::ProcessCommandLine(const wchar_t *CommandParam,Panel *Target)
{
	size_t PrefixLength=0;
	FARString strCommand=CommandParam;
	UnquoteExternal(strCommand);
	RemoveLeadingSpaces(strCommand);

	for (;;)
	{
		wchar_t Ch=strCommand.At(PrefixLength);

		if (!Ch || IsSpace(Ch) || Ch==L'/' || PrefixLength>64)
			return FALSE;

		if (Ch==L':' && PrefixLength>0)
			break;

		PrefixLength++;
	}

	LoadIfCacheAbsent();
	FARString strPrefix(strCommand,PrefixLength);
	FARString strPluginPrefix;
	std::vector<PluginData> items;

	for (int I=0; I<PluginsCount; I++)
	{
		int PluginFlags=0;

		if (PluginsData[I]->CheckWorkFlags(PIWF_CACHED))
		{
			KeyFileReadSection kfh(PluginsIni(), PluginsData[I]->GetSettingsName());
			strPluginPrefix = kfh.GetString("CommandPrefix", "");
			PluginFlags = kfh.GetUInt("Flags", 0);
		}
		else
		{
			PluginInfo Info;

			if (PluginsData[I]->GetPluginInfo(&Info))
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
		PrefixLength=strPrefix.GetLength();

		for (;;)
		{
			const wchar_t *PrEnd = wcschr(PrStart, L':');
			size_t Len=PrEnd ? (PrEnd-PrStart):StrLength(PrStart);

			if (Len<PrefixLength)Len=PrefixLength;

			if (!StrCmpNI(strPrefix, PrStart, (int)Len))
			{
				if (PluginsData[I]->Load() && PluginsData[I]->HasOpenPlugin())
				{
					items.emplace_back(PluginsData[I], PluginFlags);
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
		return FALSE;

	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
	Panel *CurPanel=(Target)?Target:ActivePanel;

	if (CurPanel->ProcessPluginEvent(FE_CLOSE,nullptr))
		return FALSE;

	PluginData* PData=nullptr;

	if (items.size() > 1)
	{
		VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY-4);
		menu.SetPosition(-1, -1, 0, 0);
		menu.SetHelp(L"ChoosePluginMenu");
		menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
		MenuItemEx mitem;

		for (size_t i=0; i < items.size(); i++)
		{
			mitem.Clear();
			mitem.strName=PointToName(items[i].pPlugin->GetModuleName());
			menu.AddItem(&mitem);
		}

		menu.Show();

		while (!menu.Done())
		{
			menu.ReadInput();
			menu.ProcessInput();
		}

		int ExitCode=menu.GetExitCode();

		if (ExitCode>=0)
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
		FARString strPluginCommand=strCommand.CPtr()+(PData->PluginFlags & PF_FULLCMDLINE ? 0:PrefixLength+1);
		RemoveTrailingSpaces(strPluginCommand);
		PHPTR hPlugin=OpenPlugin(PData->pPlugin,OPEN_COMMANDLINE,(INT_PTR)strPluginCommand.CPtr()); //BUGBUG

		if (hPlugin)
		{
			Panel *NewPanel=CtrlObject->Cp()->ChangePanel(CurPanel,FILE_PANEL,TRUE,TRUE);
			NewPanel->SetPluginMode(hPlugin,L"",!Target || Target == ActivePanel);
			NewPanel->Update(0);
			NewPanel->Show();
		}
	}

	return TRUE;
}


void PluginManager::ReadUserBackgound(SaveScreen *SaveScr)
{
	FilePanels *FPanel=CtrlObject->Cp();
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


/* $ 27.09.2000 SVS
  Функция CallPlugin - найти плагин по ID и запустить
  в зачаточном состоянии!
*/
int PluginManager::CallPlugin(DWORD SysID, int OpenFrom, void *Data, void **Ret)
{
	Plugin *pPlugin = FindPlugin(SysID);
	if ( !(pPlugin && pPlugin->HasOpenPlugin()) )
		return FALSE;

	if (Ret)
		*Ret = nullptr;

	if (OpenFrom == OPEN_LUAMACRO)
		return pPlugin->OpenPlugin(OpenFrom, (INT_PTR)Data) != nullptr;

	PHPTR PluginPanel = OpenPlugin(pPlugin,OpenFrom,(INT_PTR)Data);
	if (!PluginPanel)
		return TRUE;

	bool process = false;

	if (OpenFrom == OPEN_FROMMACRO)
	{
		if (reinterpret_cast<UINT_PTR>(PluginPanel->hPanel) >= 0x10000)
		{
			FarMacroCall *fmc = reinterpret_cast<FarMacroCall*>(PluginPanel->hPanel);
			if (fmc->Count > 0 && fmc->Values[0].Type == FMVT_PANEL)
			{
				process = true;
				PluginPanel->hPanel = fmc->Values[0].Pointer;
				if (fmc->Callback)
					fmc->Callback(fmc->CallbackData, fmc->Values, fmc->Count);
			}
		}

		if (!process)
		{
			if (Ret)
				*Ret = PluginPanel->hPanel;
			delete PluginPanel;
			return TRUE;
		}
	}
	else
	{
		process = OpenFrom == OPEN_PLUGINSMENU || OpenFrom == OPEN_FILEPANEL;
	}

	if (process)
	{
		int CurFocus=CtrlObject->Cp()->ActivePanel->GetFocus();
		Panel *NewPanel=CtrlObject->Cp()->ChangePanel(CtrlObject->Cp()->ActivePanel,FILE_PANEL,TRUE,TRUE);
		NewPanel->SetPluginMode(PluginPanel,L"",CurFocus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible());

		if (OpenFrom != OPEN_FROMMACRO)
		{
			if (Data && *(const wchar_t *)Data)
				SetDirectory(PluginPanel,(const wchar_t *)Data,0);
		}
		else
		{
			NewPanel->Update(0);
			NewPanel->Show();
		}
	}

	if (Ret && (OpenFrom == OPEN_FROMMACRO) && process)
		*Ret = reinterpret_cast<void*>(1);

	return TRUE;
}

// поддержка макрофункций Plugin.Menu, Plugin.Command, Plugin.Config
bool PluginManager::CallPluginItem(DWORD SysID, CallPluginInfo *Data)
{
	auto IsLuamacro = (SysID == SYSID_LUAMACRO);
	auto Result = false;

	Frame *TopFrame = FrameManager->GetTopModal();
	const auto curType = TopFrame->GetType();

	if (curType==MODALTYPE_DIALOG && reinterpret_cast<Dialog*>(TopFrame)->CheckDialogMode(DMODE_NOPLUGINS))
		return false;

	const auto IsEditor = curType == MODALTYPE_EDITOR;
	const auto IsViewer = curType == MODALTYPE_VIEWER;
	const auto IsDialog = curType == MODALTYPE_DIALOG;

	if (Data->CallFlags & CPT_CHECKONLY)
	{
		Data->pPlugin = FindPlugin(SysID);
		if (!Data->pPlugin || !Data->pPlugin->Load())
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

			if (IsLuamacro ? !Data->pPlugin->HasConfigureV3() : !Data->pPlugin->HasConfigure())
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
			if (IsLuamacro ? !Data->ItemUuid : !Data->ItemNumber) // 0 means "not specified"
			{
				if (MenuItemsCount == 1)
				{
					Data->FoundItemNumber = 0;
					if (IsLuamacro) {
						Data->FoundUuid = Guids[0];
					}
					ItemFound = true;
				}
			}
			else
			{
				if (!IsLuamacro) {
					if (Data->ItemNumber <= MenuItemsCount) {
						Data->FoundItemNumber = Data->ItemNumber - 1; // 1-based on the user side
						ItemFound = true;
					}
				}
				else {
					for (int ii=0; ii < MenuItemsCount; ii++) {
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
			INT_PTR Item = Data->FoundItemNumber;
			if (IsLuamacro) {
				Item = reinterpret_cast<INT_PTR>(&Data->FoundUuid);
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
				if (!IsLuamacro) {
					pd.ItemNumber = Data->FoundItemNumber;
				}
				else {
					pd.ItemGuid = Data->FoundUuid;
				}
				pd.hDlg = reinterpret_cast<Dialog*>(TopFrame);
				Item = reinterpret_cast<intptr_t>(&pd);
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
			hPlugin = OpenPlugin(Data->pPlugin, OPEN_COMMANDLINE, reinterpret_cast<INT_PTR>(command.CPtr()));
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

		const auto NewPanel = CtrlObject->Cp()->ChangePanel(ActivePanel, FILE_PANEL, TRUE, TRUE);
		NewPanel->SetPluginMode(hPlugin, {}, true);
		NewPanel->Update(0);
		NewPanel->Show();
	}

	// restore title for old plugins only.
#ifndef NO_WRAPPER
	if (Data->pPlugin->IsOemPlugin() && IsEditor && CurEditor)
	{
		CurEditor->SetPluginTitle(nullptr);
	}
#endif // NO_WRAPPER

	return Result;
}

Plugin *PluginManager::FindPlugin(DWORD SysId)
{
	return SysIdMap.count(SysId) ? SysIdMap[SysId] : nullptr;
}

PHPTR PluginManager::OpenPlugin(Plugin *pPlugin,int OpenFrom,INT_PTR Item)
{
	HANDLE hPlugin = pPlugin->OpenPlugin(OpenFrom, Item);

	if (hPlugin != INVALID_HANDLE_VALUE)
	{
		return new PanelHandle(hPlugin, pPlugin);
	}

	return nullptr;
}

void PluginManager::GetCustomData(FileListItem *ListItem)
{
	FARString FilePath(NTPath(ListItem->strName).Get());

	for (int i=0; i<PluginsCount; i++)
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
	for (int i=0; i<PluginsCount; i++)
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

void PluginManager::BackroundTaskStarted(const wchar_t *Info)
{
	{
		std::lock_guard<std::mutex> lock(BgTasks);
		auto ir = BgTasks.emplace(Info, 0);
		ir.first->second++;
		fprintf(stderr, "PluginManager::BackroundTaskStarted('%ls') - count=%d\n", Info, ir.first->second);
	}

	InterThreadCallAsync(std::bind(OnBackgroundTasksChangedSynched));
}

void PluginManager::BackroundTaskFinished(const wchar_t *Info)
{
	{
		std::lock_guard<std::mutex> lock(BgTasks);
		auto it = BgTasks.find(Info);
		if (it == BgTasks.end())
		{
			fprintf(stderr, "PluginManager::BackroundTaskFinished('%ls') - no such task!\n", Info);
			return;
		}

		it->second--;
		fprintf(stderr, "PluginManager::BackroundTaskFinished('%ls') - count=%d\n", Info, it->second);
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
	for (int J=0; ; J++)
	{
		const std::string& key = StrPrintf(Fmt, J);
		if (!kfh.HasKey(key))
			break;
		Items.emplace_back(kfh.GetString(key, ""));
	}
}

static char* BufReserve(char*& Buf, size_t Count, size_t& Rest, size_t& Size)
{
	char* Res = nullptr;

	if (Buf)
	{
		if (Rest >= Count)
		{
			Res = Buf;
			Buf += Count;
			Rest -= Count;
		}
		else
		{
			Buf += Rest;
			Rest = 0;
		}
	}

	Size += Count;
	return Res;
}


static wchar_t* StrToBuf(const FARString& Str, char*& Buf, size_t& Rest, size_t& Size)
{
	const auto Count = (Str.GetLength() + 1) * sizeof(wchar_t);
	const auto Res = reinterpret_cast<wchar_t*>(BufReserve(Buf, Count, Rest, Size));
	if (Res)
	{
		wcscpy(Res, Str.CPtr());
	}
	return Res;
}

static void ItemsToBuf(const wchar_t* const* &Strings, int& Count,
	const std::vector<FARString>& NamesArray, char*& Buf, size_t& Rest, size_t& Size)
{
	Count = NamesArray.size();
	Strings = nullptr;

	if (Count)
	{
		const auto Items = reinterpret_cast<wchar_t**>(BufReserve(Buf, Count * sizeof(wchar_t*), Rest, Size));
		Strings = Items;

		for (int i = 0; i < Count; ++i)
		{
			wchar_t* pStr = StrToBuf(NamesArray[i], Buf, Rest, Size);
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

	// if(IsPluginUnloaded(pPlugin)) return 0;
	FARString Prefix;
	DWORD Flags=0, SysID=0;
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
			for (int i=0; i<Info.PluginMenuStringsNumber; i++)
				MenuItems.emplace_back(Info.PluginMenuStrings[i]);

			for (int i=0; i<Info.DiskMenuStringsNumber; i++)
				DiskItems.emplace_back(Info.DiskMenuStrings[i]);

			for (int i=0; i<Info.PluginConfigStringsNumber; i++)
				ConfItems.emplace_back(Info.PluginConfigStrings[i]);
		}
	}

	struct
	{
		FarGetPluginInformation fgpi;
		PluginInfo PInfo;
		GlobalInfo GInfo;
	} Temp;
	char* Buffer = nullptr;
	size_t Rest = 0;
	size_t Size = sizeof(Temp);

	if (pInfo && BufferSize >= Size)
	{
		Rest = BufferSize - Size;
		Buffer = reinterpret_cast<char*>(pInfo) + Size;
	}
	else
	{
		pInfo = &Temp.fgpi;
	}

	pInfo->PInfo = reinterpret_cast<PluginInfo*>(pInfo+1);
	pInfo->GInfo = reinterpret_cast<GlobalInfo*>(pInfo->PInfo+1);
	pInfo->ModuleName = StrToBuf(pPlugin->GetModuleName(), Buffer, Rest, Size);

	pInfo->Flags = 0;

	if (pPlugin->IsLoaded())
	{
		pInfo->Flags |= FPF_LOADED;
	}
#ifndef NO_WRAPPER
	if (pPlugin->IsOemPlugin())
	{
		pInfo->Flags |= FPF_ANSI;
	}
#endif // NO_WRAPPER

	pInfo->GInfo->StructSize = sizeof(GlobalInfo);
	pInfo->GInfo->SysID = SysID;
	pInfo->GInfo->Version = pPlugin->m_PlugVersion;
	pInfo->GInfo->Title = StrToBuf(pPlugin->strTitle, Buffer, Rest, Size);
	pInfo->GInfo->Description = StrToBuf(pPlugin->strDescription, Buffer, Rest, Size);
	pInfo->GInfo->Author = StrToBuf(pPlugin->strAuthor, Buffer, Rest, Size);

	pInfo->PInfo->StructSize = sizeof(PluginInfo);
	pInfo->PInfo->Flags = Flags;
	pInfo->PInfo->SysID = SysID;
	pInfo->PInfo->CommandPrefix = StrToBuf(Prefix, Buffer, Rest, Size);

	ItemsToBuf(pInfo->PInfo->PluginMenuStrings, pInfo->PInfo->PluginMenuStringsNumber, MenuItems, Buffer, Rest, Size);
	ItemsToBuf(pInfo->PInfo->DiskMenuStrings, pInfo->PInfo->DiskMenuStringsNumber, DiskItems, Buffer, Rest, Size);
	ItemsToBuf(pInfo->PInfo->PluginConfigStrings, pInfo->PInfo->PluginConfigStringsNumber, ConfItems, Buffer, Rest, Size);

	return Size;
}

void PluginManager::ShowPluginInfo(Plugin* pPlugin)
{
	const auto strPluginSysId = FARString().Format(L"0x%08X", pPlugin->GetSysID());
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

	Builder.AddText(Msg::MPluginSysID);
	Builder.AddConstEditField(strPluginSysId, Width);

	Builder.AddText(Msg::MPluginPrefix);
	Builder.AddConstEditField(strPluginPrefix, Width);

	Builder.AddOK();
	Builder.ShowDialog();
}
