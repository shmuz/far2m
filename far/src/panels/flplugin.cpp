/*
flplugin.cpp

Файловая панель - работа с плагинами
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

#include "config.hpp"
#include "ctrlobj.hpp"
#include "datetime.hpp"
#include "delete.hpp"
#include "dirmix.hpp"
#include "filelist.hpp"
#include "filepanels.hpp"
#include "history.hpp"
#include "manager.hpp"
#include "mix.hpp"
#include "panelmix.hpp"
#include "pathmix.hpp"
#include "sizer.hpp"
#include "syslog.hpp"

/*
   В стеке ФАРова панель не хранится - только плагиновые!
*/

void FileList::PushPlugin(const wchar_t *HostFile)
{
	PluginsListItem *stItem = new PluginsListItem;
	stItem->hPlugin = hPlugin;
	stItem->strHostFile = HostFile;
	stItem->strPrevOriginalCurDir = strOriginalCurDir;
	strOriginalCurDir = strCurDir;
	stItem->Modified = FALSE;
	stItem->PrevViewMode = ViewMode;
	stItem->PrevSortMode = SortMode;
	stItem->PrevSortOrder = SortOrder;
	stItem->PrevNumericSort = NumericSort;
	stItem->PrevCaseSensitiveSort = CaseSensitiveSort;
	stItem->PrevViewSettings = ViewSettings;
	stItem->PrevDirectoriesFirst = DirectoriesFirst;
	PluginsList.push_back(stItem);
}

int FileList::PopPlugin(int EnableRestoreViewMode)
{
	OpenPluginInfo Info{};

	if (PluginsList.empty()) {
		PanelMode = NORMAL_PANEL;
		return FALSE;
	}

	// указатель на плагин, с которого уходим
	PluginsListItem *PStack = PluginsList.back();

	// закрываем текущий плагин.
	PluginsList.pop_back();
	CtrlObject->Plugins.ClosePanel(hPlugin);

	if (!PluginsList.empty()) {
		hPlugin = PluginsList.back()->hPlugin;
		strOriginalCurDir = PStack->strPrevOriginalCurDir;

		if (EnableRestoreViewMode) {
			SetViewMode(PStack->PrevViewMode);
			SortMode = PStack->PrevSortMode;
			NumericSort = PStack->PrevNumericSort;
			CaseSensitiveSort = PStack->PrevCaseSensitiveSort;
			SortOrder = PStack->PrevSortOrder;
			DirectoriesFirst = PStack->PrevDirectoriesFirst;
		}

		if (PStack->Modified) {
			PluginPanelItem PanelItem{};
			FARString strSaveDir;
			apiGetCurrentDirectory(strSaveDir);

			if (FileNameToPluginItem(PStack->strHostFile, &PanelItem)) {
				CtrlObject->Plugins.PutFiles(hPlugin, &PanelItem, 1, FALSE, 0);
			} else {
				PanelItem.FindData.lpwszFileName = wcsdup(PointToName(PStack->strHostFile));
				CtrlObject->Plugins.DeleteFiles(hPlugin, &PanelItem, 1, 0);
				free(PanelItem.FindData.lpwszFileName);
			}

			FarChDir(strSaveDir);
		}

		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

		if (!(Info.Flags & OPIF_REALNAMES)) {
			DeleteFileWithFolder(PStack->strHostFile);    // удаление файла от предыдущего плагина
		}
	} else {
		hPlugin = nullptr;
		PanelMode = NORMAL_PANEL;

		if (EnableRestoreViewMode) {
			SetViewMode(PStack->PrevViewMode);
			SortMode = PStack->PrevSortMode;
			NumericSort = PStack->PrevNumericSort;
			CaseSensitiveSort = PStack->PrevCaseSensitiveSort;
			SortOrder = PStack->PrevSortOrder;
			DirectoriesFirst = PStack->PrevDirectoriesFirst;
		}
	}

	delete PStack;

	if (EnableRestoreViewMode)
		CtrlObject->Cp()->RedrawKeyBar();

	return TRUE;
}

int FileList::FileNameToPluginItem(const wchar_t *Name, PluginPanelItem *pi)
{
	FARString strTempDir = Name;

	if (!CutToSlash(strTempDir, true))
		return FALSE;

	FarChDir(strTempDir);
	memset(pi, 0, sizeof(*pi));
	FAR_FIND_DATA_EX fdata;

	if (apiGetFindDataEx(Name, fdata)) {
		apiFindDataExToData(&fdata, &pi->FindData);
		return TRUE;
	}

	return FALSE;
}

void FileList::FileListToPluginItem(const FileListItem *fi, PluginPanelItem *pi)
{
	pi->FindData.lpwszFileName = wcsdup(fi->strName);
	pi->FindData.nFileSize = fi->FileSize;
	pi->FindData.nPhysicalSize = fi->PhysicalSize;
	pi->FindData.dwFileAttributes = fi->FileAttr;
	pi->FindData.ftLastWriteTime = fi->WriteTime;
	pi->FindData.ftCreationTime = fi->CreationTime;
	pi->FindData.ftLastAccessTime = fi->AccessTime;
	pi->NumberOfLinks = fi->NumberOfLinks;
	pi->Flags = fi->UserFlags;

	if (fi->Selected)
		pi->Flags|= PPIF_SELECTED;

	pi->CustomColumnData = fi->CustomColumnData;
	pi->CustomColumnNumber = fi->CustomColumnNumber;
	pi->Description = fi->DizText;    // BUGBUG???

	if (fi->UserData && (fi->UserFlags & PPIF_USERDATA)) {
		DWORD Size = *(DWORD *)fi->UserData;
		pi->UserData = (DWORD_PTR)malloc(Size);
		memcpy((void *)pi->UserData, (void *)fi->UserData, Size);
	} else
		pi->UserData = fi->UserData;

	pi->CRC32 = fi->CRC32;
	pi->Reserved[0] = pi->Reserved[1] = 0;
	pi->Owner = fi->strOwner.IsEmpty() ? nullptr : (wchar_t *)fi->strOwner.CPtr();
	pi->Group = fi->strGroup.IsEmpty() ? nullptr : (wchar_t *)fi->strGroup.CPtr();
}

void FileList::FreePluginPanelItem(PluginPanelItem *pi)
{
	apiFreeFindData(&pi->FindData);

	if (pi->UserData && (pi->Flags & PPIF_USERDATA))
		free((void *)pi->UserData);
}

size_t FileList::FileListToPluginItem2(const FileListItem *fi, PluginPanelItem *pi)
{
	PluginPanelItem Item, *pAll = &Item;
	Sizer sizer(pi, SIZE_MAX);
	if (sizer.AddObject<PluginPanelItem>())
		pAll = pi;

	pAll->FindData.lpwszFileName = sizer.AddFARString(fi->strName);
	pAll->CustomColumnNumber =
			sizer.AddStrArray(pAll->CustomColumnData, fi->CustomColumnData, fi->CustomColumnNumber);
	pAll->Description = sizer.AddWString(fi->DizText);
	pAll->Owner = fi->strOwner.IsEmpty() ? nullptr : sizer.AddFARString(fi->strOwner);
	pAll->Group = fi->strGroup.IsEmpty() ? nullptr : sizer.AddFARString(fi->strGroup);

	if (fi->UserData && (fi->UserFlags & PPIF_USERDATA)) {
		DWORD *pUserData = (DWORD*)fi->UserData;
		pAll->UserData = (DWORD_PTR)sizer.AddBytes(*pUserData, pUserData, alignof(max_align_t));
	}
	else
		pAll->UserData = fi->UserData;

	if (pi) {
		pAll->FindData.nFileSize = fi->FileSize;
		pAll->FindData.nPhysicalSize = fi->PhysicalSize;
		pAll->FindData.dwFileAttributes = fi->FileAttr;
		pAll->FindData.dwUnixMode = fi->FileMode;
		pAll->FindData.ftLastWriteTime = fi->WriteTime;
		pAll->FindData.ftCreationTime = fi->CreationTime;
		pAll->FindData.ftLastAccessTime = fi->AccessTime;
		pAll->NumberOfLinks = fi->NumberOfLinks;
		pAll->Flags = fi->UserFlags | (fi->Selected ? PPIF_SELECTED : 0);
		pAll->CRC32 = fi->CRC32;
		pAll->Reserved[0] = pAll->Reserved[1] = 0;
	}

	return sizer.GetSize();
}

void FileList::PluginToFileListItem(const PluginPanelItem *pi, FileListItem *fi)
{
	fi->strName = pi->FindData.lpwszFileName;
	fi->strOwner = pi->Owner;
	fi->strGroup = pi->Group;

	if (pi->Description) {
		fi->DizText = new wchar_t[StrLength(pi->Description) + 1];
		wcscpy(fi->DizText, pi->Description);
		fi->DeleteDiz = true;
	} else
		fi->DizText = nullptr;

	fi->FileSize = pi->FindData.nFileSize;
	fi->PhysicalSize = pi->FindData.nPhysicalSize;
	fi->FileAttr = pi->FindData.dwFileAttributes;
	fi->FileMode = pi->FindData.dwUnixMode;
	fi->WriteTime = pi->FindData.ftLastWriteTime;
	fi->CreationTime = pi->FindData.ftCreationTime;
	fi->AccessTime = pi->FindData.ftLastAccessTime;
	fi->ChangeTime.dwHighDateTime = 0;
	fi->ChangeTime.dwLowDateTime = 0;
	fi->NumberOfLinks = pi->NumberOfLinks;
	fi->UserFlags = pi->Flags;

	if (pi->UserData && (pi->Flags & PPIF_USERDATA)) {
		DWORD Size = *(DWORD *)pi->UserData;
		fi->UserData = (DWORD_PTR)malloc(Size);
		memcpy((void *)fi->UserData, (void *)pi->UserData, Size);
	} else
		fi->UserData = pi->UserData;

	if (pi->CustomColumnNumber > 0) {
		fi->CustomColumnData = new wchar_t *[pi->CustomColumnNumber];

		for (int I = 0; I < pi->CustomColumnNumber; I++)
			if (pi->CustomColumnData && pi->CustomColumnData[I]) {
				fi->CustomColumnData[I] = new wchar_t[StrLength(pi->CustomColumnData[I]) + 1];
				wcscpy(fi->CustomColumnData[I], pi->CustomColumnData[I]);
			} else {
				fi->CustomColumnData[I] = new wchar_t[1];
				fi->CustomColumnData[I][0] = 0;
			}
	}

	fi->CustomColumnNumber = pi->CustomColumnNumber;
	fi->CRC32 = pi->CRC32;
}

PHPTR FileList::OpenPluginForFile(const wchar_t *FileName, DWORD FileAttr, OPENFILEPLUGINTYPE Type)
{
	PHPTR Result = nullptr;
	if (FileName && *FileName && !(FileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		SetCurPath();
		_ALGO(SysLog(L"close AnotherPanel file"));
		CtrlObject->Cp()->GetAnotherPanel(this)->CloseFile();
		_ALGO(SysLog(L"call Plugins.OpenFilePlugin {"));
		Result = CtrlObject->Plugins.OpenFilePlugin(FileName, OPM_NONE, Type);
		_ALGO(SysLog(L"}"));
	}
	return Result;
}

void FileList::CreatePluginItemList(bool AddTwoDot)
{
	SelItems.clear();
	SelItems.reserve(SelFileCount + 1);

	if (!ListData)
		return;

	long SaveSelPosition = GetSelPosition;
	long OldLastSelPosition = LastSelPosition;
	FARString strSelName;
	DWORD FileAttr;

	GetSelNameCompat(nullptr, FileAttr);

	while (GetSelNameCompat(&strSelName, FileAttr))
	{
		if ((!(FileAttr & FILE_ATTRIBUTE_DIRECTORY) || !TestParentFolderName(strSelName))
				&& LastSelPosition >= 0 && LastSelPosition < FileCount)
		{
			SelItems.emplace_back();
			FileListToPluginItem(ListData[LastSelPosition], &SelItems.back());
		}
	}

	if (AddTwoDot && SelItems.empty() && (FileAttr & FILE_ATTRIBUTE_DIRECTORY))    // это про ".."
	{
		SelItems.emplace_back();
		FileListToPluginItem(ListData[0], &SelItems.back());
	}

	LastSelPosition = OldLastSelPosition;
	GetSelPosition = SaveSelPosition;
}

void FileList::DeletePluginItemList()
{
	for (auto& item: SelItems) {
		FreePluginPanelItem(&item);
	}
	SelItems.clear();
}

void FileList::PluginDelete()
{
	_ALGO(CleverSysLog clv(L"FileList::PluginDelete()"));
	SaveSelection();
	CreatePluginItemList();

	if (!SelItems.empty()) {
		if (CtrlObject->Plugins.DeleteFiles(hPlugin, SelItems.data(), SelItems.size(), 0)) {
			SetPluginModified();
			PutDizToPlugin(this, TRUE, FALSE, nullptr, &Diz);
		}

		DeletePluginItemList();
		Update(UPDATE_KEEP_SELECTION);
		Redraw();
		Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
		AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
		AnotherPanel->Redraw();
	}
}

void FileList::PutDizToPlugin(FileList *DestPanel, int Delete,
		int Move, DizList *SrcDiz, DizList *DestDiz)
{
	_ALGO(CleverSysLog clv(L"FileList::PutDizToPlugin()"));
	OpenPluginInfo Info;
	CtrlObject->Plugins.GetOpenPluginInfo(DestPanel->hPlugin, &Info);

	if (DestPanel->strPluginDizName.IsEmpty() && Info.DescrFilesNumber > 0)
		DestPanel->strPluginDizName = Info.DescrFiles[0];

	if (((Opt.Diz.UpdateMode == DIZ_UPDATE_IF_DISPLAYED && IsDizDisplayed())
				|| Opt.Diz.UpdateMode == DIZ_UPDATE_ALWAYS)
			&& !DestPanel->strPluginDizName.IsEmpty()
			&& (!Info.HostFile || !*Info.HostFile || DestPanel->GetModalMode()
					|| apiGetFileAttributes(Info.HostFile) != INVALID_FILE_ATTRIBUTES)) {
		CtrlObject->Cp()->LeftPanel->ReadDiz();
		CtrlObject->Cp()->RightPanel->ReadDiz();

		if (DestPanel->GetModalMode())
			DestPanel->ReadDiz();

		int DizPresent = FALSE;

		for (const auto &item: SelItems)
			if (item.Flags & PPIF_PROCESSDESCR) {
				FARString strName = item.FindData.lpwszFileName;
				int Code;

				if (Delete)
					Code = DestDiz->DeleteDiz(strName);
				else {
					Code = SrcDiz->CopyDiz(strName, strName, DestDiz);

					if (Code && Move)
						SrcDiz->DeleteDiz(strName);
				}

				if (Code)
					DizPresent = TRUE;
			}

		if (DizPresent) {
			FARString strTempDir;

			if (FarMkTempEx(strTempDir) && apiCreateDirectory(strTempDir, nullptr)) {
				FARString strSaveDir;
				apiGetCurrentDirectory(strSaveDir);
				FARString strDizName = strTempDir + L"/" + DestPanel->strPluginDizName;
				DestDiz->Flush(L"", strDizName);

				if (Move)
					SrcDiz->Flush(L"");

				PluginPanelItem PanelItem;

				if (FileNameToPluginItem(strDizName, &PanelItem))
					CtrlObject->Plugins.PutFiles(DestPanel->hPlugin, &PanelItem, 1, FALSE,
							OPM_SILENT | OPM_DESCR);
				else if (Delete) {
					PluginPanelItem pi{};
					pi.FindData.lpwszFileName = wcsdup(DestPanel->strPluginDizName);
					CtrlObject->Plugins.DeleteFiles(DestPanel->hPlugin, &pi, 1, OPM_SILENT);
					free(pi.FindData.lpwszFileName);
				}

				FarChDir(strSaveDir);
				DeleteFileWithFolder(strDizName);
			}
		}
	}
}

void FileList::PluginGetFiles(const wchar_t **DestPath, int Move)
{
	_ALGO(CleverSysLog clv(L"FileList::PluginGetFiles()"));
	SaveSelection();
	CreatePluginItemList();

	if (!SelItems.empty()) {
		int GetCode = CtrlObject->Plugins.GetFiles(hPlugin, SelItems.data(), SelItems.size(), Move, DestPath, 0);

		if ((Opt.Diz.UpdateMode == DIZ_UPDATE_IF_DISPLAYED && IsDizDisplayed())
				|| Opt.Diz.UpdateMode == DIZ_UPDATE_ALWAYS) {
			DizList DestDiz;
			int DizFound = FALSE;

			for (const auto& item: SelItems)
				if (item.Flags & PPIF_PROCESSDESCR) {
					if (!DizFound) {
						CtrlObject->Cp()->LeftPanel->ReadDiz();
						CtrlObject->Cp()->RightPanel->ReadDiz();
						DestDiz.Read(*DestPath);
						DizFound = TRUE;
					}

					FARString strName = item.FindData.lpwszFileName;
					CopyDiz(strName, strName, &DestDiz);
				}

			DestDiz.Flush(*DestPath);
		}

		if (GetCode == 1) {
			if (!ReturnCurrentFile)
				ClearSelection();

			if (Move) {
				SetPluginModified();
				PutDizToPlugin(this, TRUE, FALSE, nullptr, &Diz);
			}
		} else if (!ReturnCurrentFile)
			PluginClearSelection();

		DeletePluginItemList();
		Update(UPDATE_KEEP_SELECTION);
		Redraw();
		Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
		AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
		AnotherPanel->Redraw();
	}
}

void FileList::PluginToPluginFiles(int Move)
{
	_ALGO(CleverSysLog clv(L"FileList::PluginToPluginFiles()"));
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	FARString strTempDir;

	if (AnotherPanel->GetMode() != PLUGIN_PANEL)
		return;

	FileList *AnotherFilePanel = (FileList *)AnotherPanel;

	if (!FarMkTempEx(strTempDir))
		return;

	SaveSelection();
	apiCreateDirectory(strTempDir, nullptr);
	CreatePluginItemList();

	if (!SelItems.empty()) {
		const wchar_t *lpwszTempDir = strTempDir;
		int PutCode =
				CtrlObject->Plugins.GetFiles(hPlugin, SelItems.data(), SelItems.size(), FALSE, &lpwszTempDir, OPM_SILENT);
		strTempDir = lpwszTempDir;

		if (PutCode == 1 || PutCode == 2) {
			FARString strSaveDir;
			apiGetCurrentDirectory(strSaveDir);
			FarChDir(strTempDir);
			PutCode = CtrlObject->Plugins.PutFiles(AnotherFilePanel->hPlugin, SelItems.data(), SelItems.size(), FALSE, 0);

			if (PutCode == 1 || PutCode == 2) {
				if (!ReturnCurrentFile)
					ClearSelection();

				AnotherPanel->SetPluginModified();
				PutDizToPlugin(AnotherFilePanel, FALSE, FALSE, &Diz, &AnotherFilePanel->Diz);

				if (Move)
					if (CtrlObject->Plugins.DeleteFiles(hPlugin, SelItems.data(), SelItems.size(), OPM_SILENT)) {
						SetPluginModified();
						PutDizToPlugin(this, TRUE, FALSE, nullptr, &Diz);
					}
			} else if (!ReturnCurrentFile)
				PluginClearSelection();

			FarChDir(strSaveDir);
		}

		DeleteDirTree(strTempDir);
		DeletePluginItemList();
		Update(UPDATE_KEEP_SELECTION);
		Redraw();

		if (PanelMode == PLUGIN_PANEL)
			AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
		else
			AnotherPanel->Update(UPDATE_KEEP_SELECTION);

		AnotherPanel->Redraw();
	}
}

void FileList::PluginHostGetFiles()
{
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	FARString strDestPath;
	FARString strSelName;
	DWORD FileAttr;
	SaveSelection();
	GetSelNameCompat(nullptr, FileAttr);

	if (!GetSelNameCompat(&strSelName, FileAttr))
		return;

	AnotherPanel->GetCurDir(strDestPath);

	if (((!AnotherPanel->IsVisible() || AnotherPanel->GetType() != FILE_PANEL) && !SelFileCount)
			|| strDestPath.IsEmpty()) {
		strDestPath = PointToName(strSelName);
		// SVS: А зачем здесь велся поиск точки с начала?
		size_t pos;

		if (strDestPath.RPos(pos, L'.'))
			strDestPath.Truncate(pos);
	}

	int OpMode = OPM_TOPLEVEL, ExitLoop = FALSE;
	GetSelNameCompat(nullptr, FileAttr);

	while (!ExitLoop && GetSelNameCompat(&strSelName, FileAttr)) {
		PHPTR hCurPlugin;

		if ((hCurPlugin = OpenPluginForFile(strSelName, FileAttr, OFP_EXTRACT)) != nullptr
				&& hCurPlugin != PHPTR_STOP) {
			PluginPanelItem *ItemList;
			int ItemNumber;
			_ALGO(SysLog(L"call Plugins.GetFindData()"));

			if (CtrlObject->Plugins.GetFindData(hCurPlugin, &ItemList, &ItemNumber, 0)) {
				_ALGO(SysLog(L"call Plugins.GetFiles()"));
				const wchar_t *lpwszDestPath = strDestPath;
				ExitLoop = CtrlObject->Plugins.GetFiles(hCurPlugin, ItemList, ItemNumber, FALSE,
								   &lpwszDestPath, OpMode)
						!= 1;
				strDestPath = lpwszDestPath;

				if (!ExitLoop) {
					_ALGO(SysLog(L"call ClearLastGetSelection()"));
					ClearLastGetSelection();
				}

				_ALGO(SysLog(L"call Plugins.FreeFindData()"));
				CtrlObject->Plugins.FreeFindData(hCurPlugin, ItemList, ItemNumber);
				OpMode|= OPM_SILENT;
			}

			_ALGO(SysLog(L"call Plugins.ClosePlugin"));
			CtrlObject->Plugins.ClosePanel(hCurPlugin);
		}
	}

	Update(UPDATE_KEEP_SELECTION);
	Redraw();
	AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
	AnotherPanel->Redraw();
}

void FileList::PluginPutFilesToNew()
{
	_ALGO(CleverSysLog clv(L"FileList::PluginPutFilesToNew()"));
	//_ALGO(SysLog(L"FileName='%ls'",(FileName?FileName:"(nullptr)")));
	_ALGO(SysLog(L"call Plugins.OpenFilePlugin(nullptr, 0)"));
	PHPTR hNewPlugin = CtrlObject->Plugins.OpenFilePlugin(nullptr, OPM_NONE, OFP_CREATE);

	if (hNewPlugin && hNewPlugin != PHPTR_STOP) {
		_ALGO(SysLog(L"Create: FileList TmpPanel, FileCount=%d", FileCount));
		FileList TmpPanel;
		TmpPanel.SetPluginMode(hNewPlugin, L"");    // SendOnFocus??? true???
		TmpPanel.SetModalMode(TRUE);
		int PrevFileCount = FileCount;
		/* $ 12.04.2002 IS
		   Если PluginPutFilesToAnother вернула число, отличное от 2, то нужно
		   попробовать установить курсор на созданный файл.
		*/
		int rc = PluginPutFilesToAnother(FALSE, &TmpPanel);

		if (rc != 2 && FileCount == PrevFileCount + 1) {
			int LastPos = 0;
			/* Место, где вычисляются координаты вновь созданного файла
			   Позиционирование происходит на файл с максимальной датой
			   создания файла. Посему, если какой-то злобный буратино поимел
			   в текущем каталоге файло с датой создания поболее текущей,
			   то корректного позиционирования не произойдет!
			*/
			FileListItem *PtrListData, *PtrLastPos = nullptr;

			for (int i = 0; i < FileCount; i++) {
				PtrListData = ListData[i];
				if ((PtrListData->FileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
					if (PtrLastPos) {
						if (FileTimeDifference(&PtrListData->CreationTime, &PtrLastPos->CreationTime) > 0) {
							LastPos = i;
							PtrLastPos = PtrListData;
						}
					} else {
						LastPos = i;
						PtrLastPos = PtrListData;
					}
				}
			}

			if (PtrLastPos) {
				CurFile = LastPos;
				Redraw();
			}
		}
	}
}

/* $ 12.04.2002 IS
	 PluginPutFilesToAnother теперь int - возвращает то, что возвращает
	 PutFiles:
	 -1 - прервано пользовтелем
	  0 - неудача
	  1 - удача
	  2 - удача, курсор принудительно установлен на файл и заново его
		  устанавливать не нужно (см. PluginPutFilesToNew)
*/
int FileList::PluginPutFilesToAnother(int Move, Panel *AnotherPanel)
{
	if (AnotherPanel->GetMode() != PLUGIN_PANEL)
		return 0;

	FileList *AnotherFilePanel = (FileList *)AnotherPanel;
	int PutCode = 0;
	SaveSelection();
	CreatePluginItemList();

	if (!SelItems.empty()) {
		SetCurPath();
		_ALGO(SysLog(L"call Plugins.PutFiles"));
		PutCode = CtrlObject->Plugins.PutFiles(AnotherFilePanel->hPlugin, SelItems.data(), SelItems.size(), Move, 0);

		if (PutCode == 1 || PutCode == 2) {
			if (!ReturnCurrentFile) {
				_ALGO(SysLog(L"call ClearSelection()"));
				ClearSelection();
			}

			_ALGO(SysLog(L"call PutDizToPlugin"));
			PutDizToPlugin(AnotherFilePanel, FALSE, Move, &Diz, &AnotherFilePanel->Diz);
			AnotherPanel->SetPluginModified();
		} else if (!ReturnCurrentFile)
			PluginClearSelection();

		_ALGO(SysLog(L"call DeletePluginItemList"));
		DeletePluginItemList();
		Update(UPDATE_KEEP_SELECTION);
		Redraw();

		if (AnotherPanel == CtrlObject->Cp()->GetAnotherPanel(this)) {
			AnotherPanel->Update(UPDATE_KEEP_SELECTION);
			AnotherPanel->Redraw();
		}
	}

	return PutCode;
}

void FileList::GetOpenPluginInfo(OpenPluginInfo *Info)
{
	_ALGO(CleverSysLog clv(L"FileList::GetOpenPluginInfo()"));
	//_ALGO(SysLog(L"FileName='%ls'",(FileName?FileName:"(nullptr)")));
	memset(Info, 0, sizeof(*Info));

	if (PanelMode == PLUGIN_PANEL)
		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, Info);
}

/*
   Функция для вызова команды "Архивные команды" (Shift-F3)
*/
void FileList::ProcessHostFile()
{
	_ALGO(CleverSysLog clv(L"FileList::ProcessHostFile()"));

	//_ALGO(SysLog(L"FileName='%ls'",(FileName?FileName:"(nullptr)")));
	if (FileCount > 0 && SetCurPath()) {
		int Done = FALSE;
		SaveSelection();

		if (PanelMode == PLUGIN_PANEL && !PluginsList.back()->strHostFile.IsEmpty()) {
			_ALGO(SysLog(L"call CreatePluginItemList"));
			CreatePluginItemList();
			_ALGO(SysLog(L"call Plugins.ProcessHostFile"));
			Done = CtrlObject->Plugins.ProcessHostFile(hPlugin, SelItems.data(), SelItems.size(), 0);

			if (Done)
				SetPluginModified();
			else {
				if (!ReturnCurrentFile)
					PluginClearSelection();

				Redraw();
			}

			_ALGO(SysLog(L"call DeletePluginItemList"));
			DeletePluginItemList();

			if (Done)
				ClearSelection();
		} else {
			int SCount = GetRealSelCount();

			if (SCount > 0) {
				for (int I = 0; I < FileCount; ++I) {
					if (ListData[I]->Selected) {
						Done = ProcessOneHostFile(I);

						if (Done == 1)
							Select(ListData[I], 0);
						else if (Done == -1)
							continue;
						else          // Если ЭТО убрать, то... будем жать ESC до потере пулься
							break;    //
					}
				}

				if (SelectedFirst)
					SortFileList(true);
			} else {
				if ((Done = ProcessOneHostFile(CurFile)) == 1)
					ClearSelection();
			}
		}

		if (Done) {
			Update(UPDATE_KEEP_SELECTION);
			Redraw();
			Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
			AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
			AnotherPanel->Redraw();
		}
	}
}

/*
  Обработка одного хост-файла.
  Return:
	-1 - Этот файл никаким плагином не поддержан
	 0 - Плагин вернул FALSE
	 1 - Плагин вернул TRUE
*/
int FileList::ProcessOneHostFile(int Idx)
{
	_ALGO(CleverSysLog clv(L"FileList::ProcessOneHostFile()"));
	int Done = -1;
	_ALGO(SysLog(L"call OpenPluginForFile([Idx=%d] '%ls')", Idx, ListData[Idx]->strName.CPtr()));
	FARString strName = ListData[Idx]->strName;
	PHPTR hNewPlugin = OpenPluginForFile(strName, ListData[Idx]->FileAttr, OFP_COMMANDS);

	if (hNewPlugin && hNewPlugin != PHPTR_STOP) {
		PluginPanelItem *ItemList;
		int ItemNumber;
		_ALGO(SysLog(L"call Plugins.GetFindData"));

		if (CtrlObject->Plugins.GetFindData(hNewPlugin, &ItemList, &ItemNumber, OPM_TOPLEVEL)) {
			_ALGO(SysLog(L"call Plugins.ProcessHostFile"));
			Done = CtrlObject->Plugins.ProcessHostFile(hNewPlugin, ItemList, ItemNumber, OPM_TOPLEVEL);
			_ALGO(SysLog(L"call Plugins.FreeFindData"));
			CtrlObject->Plugins.FreeFindData(hNewPlugin, ItemList, ItemNumber);
		}

		_ALGO(SysLog(L"call Plugins.ClosePanel"));
		CtrlObject->Plugins.ClosePanel(hNewPlugin);
	}

	return Done;
}

void FileList::SetPluginMode(PHPTR PanHandle, const wchar_t *PluginFile, bool SendOnFocus)
{
	FrameManager->FolderChanged();
	if (PanelMode != PLUGIN_PANEL) {
		CtrlObject->FolderHistory->AddToHistory(strCurDir);
	}

	hPlugin = PanHandle;
	PushPlugin(PluginFile);
	PanelMode = PLUGIN_PANEL;

	if (SendOnFocus)
		SetFocus();

	OpenPluginInfo Info;
	CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

	if (Info.StartPanelMode)
		SetViewMode(VIEW_0 + Info.StartPanelMode - L'0');

	CtrlObject->Cp()->RedrawKeyBar();

	if (Info.StartSortMode) {
		SortMode = Info.StartSortMode - (SM_UNSORTED - UNSORTED);
		SortOrder = Info.StartSortOrder ? -1 : 1;
	}

	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->GetType() != FILE_PANEL) {
		AnotherPanel->Update(UPDATE_KEEP_SELECTION);
		AnotherPanel->Redraw();
	}
}

void FileList::PluginGetPanelInfo(PanelInfo &Info)
{
	CorrectPosition();
	Info.CurrentItem = CurFile;
	Info.TopPanelItem = CurTopFile;
	Info.ItemsNumber = FileCount;
	Info.SelectedItemsNumber = ListData ? GetSelCount() : 0;
}

size_t FileList::PluginGetPanelItem(int ItemNumber, PluginPanelItem *Item)
{
	size_t result = 0;

	if (ListData && ItemNumber < FileCount) {
		result = FileListToPluginItem2(ListData[ItemNumber], Item);
	}

	return result;
}

size_t FileList::PluginGetSelectedPanelItem(int ItemNumber, PluginPanelItem *Item)
{
	size_t result = 0;

	if (ListData && ItemNumber < FileCount) {
		if (ItemNumber == CacheSelIndex) {
			result = FileListToPluginItem2(ListData[CacheSelPos], Item);
		} else {
			if (ItemNumber < CacheSelIndex)
				CacheSelIndex = -1;

			int CurSel = CacheSelIndex, StartValue = CacheSelIndex >= 0 ? CacheSelPos + 1 : 0;

			for (int i = StartValue; i < FileCount; i++) {
				if (ListData[i]->Selected)
					CurSel++;

				if (CurSel == ItemNumber) {
					result = FileListToPluginItem2(ListData[i], Item);
					CacheSelIndex = ItemNumber;
					CacheSelPos = i;
					break;
				}
			}

			if (CurSel == -1 && !ItemNumber) {
				result = FileListToPluginItem2(ListData[CurFile], Item);
				CacheSelIndex = -1;
			}
		}
	}

	return result;
}

void FileList::PluginGetColumnTypesAndWidths(FARString &strColumnTypes, FARString &strColumnWidths)
{
	ViewSettingsToText(ViewSettings.PanelColumns, strColumnTypes, strColumnWidths);
}

void FileList::PluginBeginSelection()
{
	SaveSelection();
}

void FileList::PluginSetSelection(int ItemNumber, bool Selection)
{
	Select(ListData[ItemNumber], Selection);
}

void FileList::PluginClearSelection(int SelectedItemNumber)
{
	if (ListData && SelectedItemNumber < FileCount) {
		if (SelectedItemNumber <= CacheSelClearIndex) {
			CacheSelClearIndex = -1;
		}

		int CurSel = CacheSelClearIndex, StartValue = CacheSelClearIndex >= 0 ? CacheSelClearPos + 1 : 0;

		for (int i = StartValue; i < FileCount; i++) {
			if (ListData[i]->Selected) {
				CurSel++;
			}

			if (CurSel == SelectedItemNumber) {
				Select(ListData[i], FALSE);
				CacheSelClearIndex = SelectedItemNumber;
				CacheSelClearPos = i;
				break;
			}
		}
	}
}

void FileList::PluginEndSelection()
{
	if (SelectedFirst) {
		SortFileList(true);
	}
}

void FileList::ProcessPluginCommand()
{
	_ALGO(CleverSysLog clv(L"FileList::ProcessPluginCommand"));
	_ALGO(SysLog(L"PanelMode=%ls", (PanelMode == PLUGIN_PANEL ? "PLUGIN_PANEL" : "NORMAL_PANEL")));
	int Command = PluginCommand;
	PluginCommand = -1;

	if (PanelMode == PLUGIN_PANEL)
		switch (Command) {
			case FCTL_CLOSEPLUGIN:
				_ALGO(SysLog(L"Command=FCTL_CLOSEPLUGIN"));
				SetCurDir(strPluginParam, true);

				if (strPluginParam.IsEmpty())
					Update(UPDATE_KEEP_SELECTION);

				Redraw();
				break;
		}
}

void FileList::SetPluginModified()
{
	if (!PluginsList.empty() && PluginsList.back()) {
		PluginsList.back()->Modified = TRUE;
	}
}

PHPTR FileList::GetPluginHandle() const
{
	return (hPlugin);
}

int FileList::ProcessPluginEvent(int Event, void *Param)
{
	if (PanelMode == PLUGIN_PANEL)
		return (CtrlObject->Plugins.ProcessEvent(hPlugin, Event, Param));

	return FALSE;
}

void FileList::PluginClearSelection()
{
	SaveSelection();
	int FileNumber = 0;

	for (const auto &item: SelItems) {
		if (!(item.Flags & PPIF_SELECTED)) {
			while (StrCmp(item.FindData.lpwszFileName, ListData[FileNumber]->strName))
				if (++FileNumber >= FileCount)
					return;

			Select(ListData[FileNumber++], 0);
		}
	}
}
