/*
flupdate.cpp

Файловая панель - чтение имен файлов
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

#include "filelist.hpp"
#include "colors.hpp"
#include "lang.hpp"
#include "filepanels.hpp"
#include "cmdline.hpp"
#include "filefilter.hpp"
#include "hilight.hpp"
#include "ctrlobj.hpp"
#include "manager.hpp"
#include "TPreRedrawFunc.hpp"
#include "syslog.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "config.hpp"
#include "fileowner.hpp"
#include "delete.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "mix.hpp"
#include "panelctype.hpp"

// Флаги для ReadDiz()
enum ReadDizFlags
{
	RDF_NO_UPDATE = 0x00000001UL,
};

void FileList::Update(int Mode)
{
	_ALGO(CleverSysLog clv(L"FileList::Update"));
	_ALGO(SysLog(L"(Mode=[%d/0x%08X] %ls)", Mode, Mode,
			(Mode == UPDATE_KEEP_SELECTION ? L"UPDATE_KEEP_SELECTION" : L"")));

	if (EnableUpdate)
		switch (PanelMode) {
			case NORMAL_PANEL:
				ReadFileNames(Mode & UPDATE_KEEP_SELECTION, Mode & UPDATE_IGNORE_VISIBLE,
						Mode & UPDATE_DRAW_MESSAGE, Mode & UPDATE_CAN_BE_ANNOYING);
				break;
			case PLUGIN_PANEL: {
				OpenPluginInfo Info;
				CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);
				ProcessPluginCommand();

				if (PanelMode != PLUGIN_PANEL)
					ReadFileNames(Mode & UPDATE_KEEP_SELECTION, Mode & UPDATE_IGNORE_VISIBLE,
							Mode & UPDATE_DRAW_MESSAGE, Mode & UPDATE_CAN_BE_ANNOYING);
				else if ((Info.Flags & OPIF_REALNAMES)
						|| CtrlObject->Cp()->GetAnotherPanel(this)->GetMode() == PLUGIN_PANEL
						|| !(Mode & UPDATE_SECONDARY))
					UpdatePlugin(Mode & UPDATE_KEEP_SELECTION, Mode & UPDATE_IGNORE_VISIBLE);
			}
				ProcessPluginCommand();
				break;
		}

	LastUpdateTime = GetProcessUptimeMSec();
}

void FileList::UpdateIfRequired()
{
	if (UpdateRequired && !UpdateDisabled) {
		UpdateRequired = false;
		Update(UpdateRequiredMode | UPDATE_IGNORE_VISIBLE);
	}
}

void ReadFileNamesMsg(const wchar_t *Msg)
{
	Message(0, 0, Msg::ReadingTitleFiles, Msg);
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Param1 = (void *)Msg;
	PreRedraw.SetParam(preRedrawItem.Param);
}

static void PR_ReadFileNamesMsg()
{
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	ReadFileNamesMsg((wchar_t *)preRedrawItem.Param.Param1);
}

// ЭТО ЕСТЬ УЗКОЕ МЕСТО ДЛЯ СКОРОСТНЫХ ХАРАКТЕРИСТИК Far Manager
// при считывании дирректории

void FileList::ReadFileNames(int KeepSelection, int IgnoreVisible, int DrawMessage, int CanBeAnnoying)
{
	SCOPED_ACTION(TPreRedrawFuncGuard)(PR_ReadFileNamesMsg);

	strOriginalCurDir = strCurDir;

	if (!IsVisible() && !IgnoreVisible) {
		UpdateRequired = true;
		UpdateRequiredMode = KeepSelection;
		return;
	}

	UpdateRequired = false;
	AccessTimeUpdateRequired = false;
	DizRead = false;
	FAR_FIND_DATA_EX fdata;
	FileListItem *CurPtr = nullptr, **OldData = nullptr;
	FARString strCurName, strNextCurName;
	int OldFileCount = 0;
	CloseChangeNotification();

	if (this != CtrlObject->Cp()->LeftPanel && this != CtrlObject->Cp()->RightPanel)
		return;

	SudoClientRegion sdc_rgn;

	FARString strSaveDir;
	apiGetCurrentDirectory(strSaveDir);
	{
		if (!SetCurPath()) {
			if (!WinPortTesting())
				FlushInputBuffer();    // Очистим буффер ввода, т.к. мы уже можем быть в другом месте...
			return;
		}
	}
	SortGroupsRead = FALSE;

	if (GetFocus())
		CtrlObject->CmdLine->SetCurDir(strCurDir);

	LastCurFile = -1;
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	AnotherPanel->QViewDelTempName();
	int PrevSelFileCount = SelFileCount;
	SelFileCount = 0;
	SelFileSize = 0;
	TotalFileCount = 0;
	TotalFileSize = 0;
	CacheSelIndex = -1;
	CacheSelClearIndex = -1;
	MarkLM = 0;

	if (Opt.ShowPanelFree) {
		uint64_t TotalSize, TotalFree;

		if (!apiGetDiskSize(strCurDir, &TotalSize, &TotalFree, &FreeDiskSize))
			FreeDiskSize = 0;
	}

	if (FileCount > 0) {
		strCurName = ListData[CurFile]->strName;

		if (ListData[CurFile]->Selected) {
			for (int i = CurFile + 1; i < FileCount; i++) {
				CurPtr = ListData[i];

				if (!CurPtr->Selected) {
					strNextCurName = CurPtr->strName;
					break;
				}
			}
		}
	}

	if (KeepSelection || PrevSelFileCount > 0) {
		OldData = ListData;
		OldFileCount = FileCount;
	} else
		DeleteListData(ListData, FileCount);

	ListData = nullptr;
	SymlinksCache.clear();

	int ReadOwners = IsColumnDisplayed(OWNER_COLUMN);
	int ReadGroups = IsColumnDisplayed(GROUP_COLUMN);
	FARString strComputerName;

	WINPORT(SetLastError)(ERROR_SUCCESS);
	int AllocatedCount = 0;
	FileListItem *NewPtr;
	// сформируем заголовок вне цикла
	wchar_t Title[2048];
	int TitleLength = Min((int)X2 - X1 - 1, (int)(ARRAYSIZE(Title)) - 1);
	// wmemset(Title,0x0CD,TitleLength); //BUGBUG
	// Title[TitleLength]=0;
	MakeSeparator(TitleLength, Title, 9, nullptr);
	BOOL IsShowTitle = FALSE;
	// BOOL NeedHighlight=Opt.Highlight && PanelMode != PLUGIN_PANEL;

	if (!Filter)
		Filter = new FileFilter(this, FFT_PANEL);

	// Рефреш текущему времени для фильтра перед началом операции
	Filter->UpdateCurrentTime();
	CtrlObject->HiFiles->UpdateCurrentTime();
	bool bCurDirRoot = IsLocalRootPath(strCurDir) || IsLocalPrefixRootPath(strCurDir)
			|| IsLocalVolumeRootPath(strCurDir);

	FileCount = 0;
	// BUGBUG!!! // что это?
	::FindFile Find(L"*", true,
			CanBeAnnoying ? FIND_FILE_FLAG_NO_CUR_UP
						  : FIND_FILE_FLAG_NO_CUR_UP | FIND_FILE_FLAG_NOT_ANNOYING);
	DWORD FindErrorCode = ERROR_SUCCESS;
	bool UseFilter = Filter->IsEnabledOnPanel();
	bool ReadCustomData = IsColumnDisplayed(CUSTOM_COLUMN0) != 0;

	CachedFileOwnerLookup cached_owners;
	CachedFileGroupLookup cached_groups;

	DWORD StartTime = WINPORT(GetTickCount)();

	while (Find.Get(fdata)) {
		FindErrorCode = WINPORT(GetLastError)();

		if ((Opt.ShowHidden || !(fdata.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
				&& (!UseFilter || Filter->FileInFilter(fdata))) {
			if (FileCount >= AllocatedCount) {
				AllocatedCount = AllocatedCount + 256 + AllocatedCount / 4;
				FileListItem **pTemp;

				if (!(pTemp = (FileListItem **)realloc(ListData, AllocatedCount * sizeof(*ListData))))
					break;

				ListData = pTemp;
			}

			ListData[FileCount] = new FileListItem;
			NewPtr = ListData[FileCount];
			NewPtr->FileAttr = fdata.dwFileAttributes;
			NewPtr->FileMode = fdata.dwUnixMode;
			NewPtr->CreationTime = fdata.ftCreationTime;
			NewPtr->AccessTime = fdata.ftLastAccessTime;
			NewPtr->WriteTime = fdata.ftLastWriteTime;
			NewPtr->ChangeTime = fdata.ftChangeTime;
			NewPtr->FileSize = fdata.nFileSize;
			NewPtr->PhysicalSize = fdata.nPhysicalSize;
			NewPtr->strName = std::move(fdata.strFileName);
			NewPtr->Position = FileCount++;
			NewPtr->NumberOfLinks = fdata.nHardLinks;

			if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 || Opt.ScanJunction) {
					TotalFileSize += NewPtr->FileSize;
				}
			}

			NewPtr->SortGroup = DEFAULT_SORT_GROUP;

			if (ReadOwners || ReadGroups) {
				SudoSilentQueryRegion ssqr(!CanBeAnnoying);

				if (ReadOwners)
					NewPtr->strOwner = cached_owners.Lookup(fdata.UnixOwner);

				if (ReadGroups)
					NewPtr->strGroup = cached_groups.Lookup(fdata.UnixGroup);
			}

			if (ReadCustomData)
				CtrlObject->Plugins.GetCustomData(NewPtr);

			// if (NeedHighlight)
			//	CtrlObject->HiFiles->GetHiColor(&NewPtr,1);

			if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				TotalFileCount++;

			// memcpy(ListData+FileCount,&NewPtr,sizeof(NewPtr));
			//      FileCount++;

			DWORD CurTime = WINPORT(GetTickCount)();
			if (CurTime - StartTime > RedrawTimeout) {
				StartTime = CurTime;
				if (IsVisible()) {
					FARString strReadMsg;

					if (!IsShowTitle) {
						if (!DrawMessage) {
							Text(X1 + 1, Y1, FarColorToReal(COL_PANELBOX), Title);
							IsShowTitle = TRUE;
							SetFarColor(Focus ? COL_PANELSELECTEDTITLE : COL_PANELTITLE);
						}
					}

					strReadMsg.Format(Msg::ReadingFiles, FileCount);

					if (DrawMessage) {
						ReadFileNamesMsg(strReadMsg);
					} else {
						TruncStr(strReadMsg, TitleLength - 2);
						int MsgLength = (int)strReadMsg.GetLength();
						GotoXY(X1 + 1 + (TitleLength - MsgLength - 1) / 2, Y1);
						FS << L" " << strReadMsg << L" ";
					}
				}

				if (CheckForEsc()) {
					break;
				}
			}
		}
	}

	if (!(FindErrorCode == ERROR_SUCCESS || FindErrorCode == ERROR_NO_MORE_FILES
				|| FindErrorCode == ERROR_FILE_NOT_FOUND))
		Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, Msg::ReadFolderError, Msg::Ok);
	/*
	int NetRoot=FALSE;
	if (strCurDir.At(0)==GOOD_SLASH && strCurDir.At(1)==GOOD_SLASH)
	{
		const wchar_t *ChPtr=wcschr(strCurDir.CPtr()+2,'/');
		if (!ChPtr || !wcschr(ChPtr+1,L'/'))
			NetRoot=TRUE;
	}
	*/

	// пока кусок закомментим, возможно он даже и не пригодится.
	if (!bCurDirRoot)    // && !NetRoot)
	{
		if (FileCount >= AllocatedCount) {
			FileListItem **pTemp;

			if ((pTemp = (FileListItem **)realloc(ListData, (FileCount + 1) * sizeof(*ListData))))
				ListData = pTemp;
		}

		if (ListData) {
			FARString TwoDotsOwner, TwoDotsGroup;
			if (ReadOwners) {
				GetFileOwner(strComputerName, strCurDir, TwoDotsOwner);
			}

			if (ReadGroups) {
				GetFileGroup(strComputerName, strCurDir, TwoDotsGroup);
			}

			FILETIME TwoDotsTimes[4] = {};
			if (apiGetFindDataForExactPathName(strCurDir, fdata)) {
				TwoDotsTimes[0] = fdata.ftCreationTime;
				TwoDotsTimes[1] = fdata.ftLastAccessTime;
				TwoDotsTimes[2] = fdata.ftLastWriteTime;
				TwoDotsTimes[3] = fdata.ftChangeTime;
			}

			AddParentPoint(FileCount, TwoDotsTimes, TwoDotsOwner, TwoDotsGroup);

			// if (NeedHighlight)
			//	CtrlObject->HiFiles->GetHiColor(&ListData[FileCount],1);

			FileCount++;
		}
	}

	if (IsColumnDisplayed(DIZ_COLUMN))
		ReadDiz();

	if (AnotherPanel->GetMode() == PLUGIN_PANEL) {
		PHPTR hAnotherPlugin = AnotherPanel->GetPluginHandle();
		PluginPanelItem *PanelData = nullptr;
		FARString strPath;
		int PanelCount = 0;
		strPath = strCurDir;
		AddEndSlash(strPath);

		if (CtrlObject->Plugins.GetVirtualFindData(hAnotherPlugin, &PanelData, &PanelCount, strPath)) {
			FileListItem **pTemp;

			if ((pTemp = (FileListItem **)realloc(ListData, (FileCount + PanelCount) * sizeof(*ListData)))) {
				ListData = pTemp;

				for (int i = 0; i < PanelCount; i++) {
					CurPtr = ListData[FileCount + i];
					FAR_FIND_DATA &fdata = PanelData[i].FindData;
					PluginToFileListItem(&PanelData[i], CurPtr);
					CurPtr->Position = FileCount;
					TotalFileSize+= fdata.nFileSize;
					CurPtr->PrevSelected = CurPtr->Selected = false;
					CurPtr->ShowFolderSize = 0;
					CurPtr->SortGroup = CtrlObject->HiFiles->GetGroup(CurPtr);

					if (!TestParentFolderName(fdata.lpwszFileName)
							&& !(CurPtr->FileAttr & FILE_ATTRIBUTE_DIRECTORY))
						TotalFileCount++;
				}

				// цветовую боевую раскраску в самом конце, за один раз
				// CtrlObject->HiFiles->GetHiColor(&ListData[FileCount],PanelCount);
				FileCount+= PanelCount;
			}

			CtrlObject->Plugins.FreeVirtualFindData(hAnotherPlugin, PanelData, PanelCount);
		}
	}

	if (Opt.Highlight && FileCount)
		CtrlObject->HiFiles->GetHiColor(&ListData[0], FileCount, false, &MarkLM);

	CreateChangeNotification(FALSE);
	CorrectPosition();

	if (KeepSelection || PrevSelFileCount > 0) {
		MoveSelection(ListData, FileCount, OldData, OldFileCount);
		DeleteListData(OldData, OldFileCount);
	}

	if (SortGroups)
		ReadSortGroups(false);

	if (!KeepSelection && PrevSelFileCount > 0) {
		SaveSelection();
		ClearSelection();
	}

	SortFileList(false);

	if (CurFile >= FileCount || StrCmp(ListData[CurFile]->strName, strCurName))
		if (!GoToFile(strCurName) && !strNextCurName.IsEmpty())
			GoToFile(strNextCurName);

	/* $ 13.02.2002 DJ
		SetTitle() - только если мы текущий фрейм!
	*/
	if (CtrlObject->Cp() == FrameManager->GetCurrentFrame())
		SetTitle();

	FarChDir(strSaveDir);    //???
}

/*$ 22.06.2001 SKV
  Добавлен параметр для вызова после исполнения команды.
*/
int FileList::UpdateIfChanged(int UpdateMode)
{
	//_SVS(SysLog(L"CurDir='%ls' Opt.AutoUpdateLimit=%d <= FileCount=%d",CurDir,Opt.AutoUpdateLimit,FileCount));
	if (!Opt.AutoUpdateLimit || static_cast<DWORD>(FileCount) <= Opt.AutoUpdateLimit) {
		/* $ 19.12.2001 VVM
		  ! Сменим приоритеты. При Force обновление всегда! */
		if ((IsVisible() && (GetProcessUptimeMSec() - LastUpdateTime > 2000))
				|| (UpdateMode != UIC_UPDATE_NORMAL)) {
			if (UpdateMode == UIC_UPDATE_NORMAL)
				ProcessPluginEvent(FE_IDLE, nullptr);

			/* $ 24.12.2002 VVM
			  ! Поменяем логику обновления панелей. */
			if (    // Нормальная панель, на ней установлено уведомление и есть сигнал
					(PanelMode == NORMAL_PANEL && ListChange && ListChange->Check()) ||
					// Или Нормальная панель, но нет уведомления и мы попросили обновить через UPDATE_FORCE
					(PanelMode == NORMAL_PANEL && !ListChange && UpdateMode == UIC_UPDATE_FORCE) ||
					// Или плагинная панель и обновляем через UPDATE_FORCE
					(PanelMode != NORMAL_PANEL && UpdateMode == UIC_UPDATE_FORCE)) {
				Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

				if (AnotherPanel->GetType() == INFO_PANEL) {
					AnotherPanel->Update(UPDATE_KEEP_SELECTION);

					if (UpdateMode == UIC_UPDATE_NORMAL)
						AnotherPanel->Redraw();
				}

				Update(UPDATE_KEEP_SELECTION);

				if (UpdateMode == UIC_UPDATE_NORMAL)
					Show();

				return TRUE;
			}
		}
	}

	return FALSE;
}

void FileList::CreateChangeNotification(int CheckTree)
{
	wchar_t RootDir[4] = L" :/";
	DWORD DriveType = DRIVE_REMOTE;
	CloseChangeNotification();

	if (IsLocalPath(strCurDir)) {
		RootDir[0] = strCurDir.At(0);
		DriveType = FAR_GetDriveType(RootDir);
	}

	if (Opt.AutoUpdateRemoteDrive || (DriveType != DRIVE_REMOTE)) {
		ListChange.reset();
		ListChange.reset(IFSNotify_Create(strCurDir.GetMB(), CheckTree != FALSE, FSNW_NAMES_AND_STATS));
	}
}

void FileList::CloseChangeNotification()
{
	ListChange.reset();
}

static int _cdecl SortSearchList(const void *el1, const void *el2)
{
	FileListItem **SPtr1 = (FileListItem **)el1, **SPtr2 = (FileListItem **)el2;
	return StrCmp(SPtr1[0]->strName, SPtr2[0]->strName);
}

void FileList::MoveSelection(FileListItem **ListData, long FileCount, FileListItem **OldData,
		long OldFileCount)
{
	FileListItem **OldPtr;
	SelFileCount = 0;
	SelFileSize = 0;
	CacheSelIndex = -1;
	CacheSelClearIndex = -1;
	far_qsort(OldData, OldFileCount, sizeof(*OldData), SortSearchList);

	while (FileCount--) {
		OldPtr = (FileListItem **)bsearch(ListData, (const void *)OldData, OldFileCount, sizeof(*ListData),
				SortSearchList);

		if (OldPtr) {
			if (OldPtr[0]->ShowFolderSize) {
				ListData[0]->ShowFolderSize = 2;
				ListData[0]->FileSize = OldPtr[0]->FileSize;
				ListData[0]->PhysicalSize = OldPtr[0]->PhysicalSize;
			}

			Select(ListData[0], OldPtr[0]->Selected);
			ListData[0]->PrevSelected = OldPtr[0]->PrevSelected;
		}

		ListData++;
	}
}

void FileList::UpdatePlugin(int KeepSelection, int IgnoreVisible)
{
	_ALGO(CleverSysLog clv(L"FileList::UpdatePlugin"));
	_ALGO(SysLog(L"(KeepSelection=%d, IgnoreVisible=%d)", KeepSelection, IgnoreVisible));

	if (!IsVisible() && !IgnoreVisible) {
		UpdateRequired = true;
		UpdateRequiredMode = KeepSelection;
		return;
	}

	DizRead = false;
	FileListItem *CurPtr, **OldData = nullptr;
	FARString strCurName, strNextCurName;
	int OldFileCount = 0;
	CloseChangeNotification();
	LastCurFile = -1;
	OpenPluginInfo Info;
	CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

	if (Opt.ShowPanelFree && (Info.Flags & OPIF_REALNAMES)) {
		uint64_t TotalSize, TotalFree;

		if (!apiGetDiskSize(strCurDir, &TotalSize, &TotalFree, &FreeDiskSize))
			FreeDiskSize = 0;
	}

	PluginPanelItem *PanelData = nullptr;
	int PluginFileCount;

	if (!CtrlObject->Plugins.GetFindData(hPlugin, &PanelData, &PluginFileCount, 0)) {
		DeleteListData(ListData, FileCount);
		SymlinksCache.clear();
		PopPlugin(TRUE);
		Update(KeepSelection);

		// WARP> явный хак, но очень способствует - восстанавливает позицию на панели при ошибке чтения архива.
		if (!PrevDataList.empty())
			GoToFile(PrevDataList.back()->strPrevName);

		return;
	}

	int PrevSelFileCount = SelFileCount;
	SelFileCount = 0;
	SelFileSize = 0;
	TotalFileCount = 0;
	TotalFileSize = 0;
	CacheSelIndex = -1;
	CacheSelClearIndex = -1;
	MarkLM = 0;

	strPluginDizName.Clear();

	if (FileCount > 0) {
		CurPtr = ListData[CurFile];
		strCurName = CurPtr->strName;

		if (CurPtr->Selected) {
			for (int i = CurFile + 1; i < FileCount; i++) {
				CurPtr = ListData[i];

				if (!CurPtr->Selected) {
					strNextCurName = CurPtr->strName;
					break;
				}
			}
		}
	} else if (Info.Flags & OPIF_ADDDOTS) {
		strCurName = L"..";
	}

	if (KeepSelection || PrevSelFileCount > 0) {
		OldData = ListData;
		OldFileCount = FileCount;
	} else {
		DeleteListData(ListData, FileCount);
	}

	SymlinksCache.clear();
	FileCount = PluginFileCount;
	ListData = (FileListItem **)malloc(sizeof(FileListItem *) * (FileCount + 1));

	if (!ListData) {
		FileCount = 0;
		return;
	}

	if (!Filter)
		Filter = new FileFilter(this, FFT_PANEL);

	// Рефреш текущему времени для фильтра перед началом операции
	Filter->UpdateCurrentTime();
	CtrlObject->HiFiles->UpdateCurrentTime();
	int DotsPresent = FALSE;
	int FileListCount = 0;
	bool UseFilter = Filter->IsEnabledOnPanel();

	for (int i = 0; i < FileCount; i++) {
		ListData[FileListCount] = new FileListItem;
		FileListItem *CurListData = ListData[FileListCount];

		if (UseFilter && (Info.Flags & OPIF_USEFILTER))
			// if (!(CurPanelData->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			if (!Filter->FileInFilter(PanelData[i].FindData))
				continue;

		if (!Opt.ShowHidden
				&& (PanelData[i].FindData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
			continue;

		// memset(CurListData,0,sizeof(*CurListData));
		PluginToFileListItem(&PanelData[i], CurListData);
		CurListData->Position = i;

		if ((Info.Flags & OPIF_USESORTGROUPS) /* && !(CurListData->FileAttr & FILE_ATTRIBUTE_DIRECTORY)*/)
			CurListData->SortGroup = CtrlObject->HiFiles->GetGroup(CurListData);
		else
			CurListData->SortGroup = DEFAULT_SORT_GROUP;

		if (!CurListData->DizText) {
			CurListData->DeleteDiz = false;
			// CurListData->DizText=nullptr;
		}

		if (TestParentFolderName(CurListData->strName)) {
			DotsPresent = TRUE;
			CurListData->FileAttr|= FILE_ATTRIBUTE_DIRECTORY;
		} else if (!(CurListData->FileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
			TotalFileCount++;
		}

		TotalFileSize+= CurListData->FileSize;
		FileListCount++;
	}

	if ((Info.Flags & OPIF_USEHIGHLIGHTING) || (Info.Flags & OPIF_USEATTRHIGHLIGHTING))
		CtrlObject->HiFiles->GetHiColor(ListData, FileListCount,
				(Info.Flags & OPIF_USEATTRHIGHLIGHTING) != 0, &MarkLM);

	FileCount = FileListCount;

	if ((Info.Flags & OPIF_ADDDOTS) && !DotsPresent) {
		FileListItem *CurPtr = AddParentPoint(FileCount);

		if ((Info.Flags & OPIF_USEHIGHLIGHTING) || (Info.Flags & OPIF_USEATTRHIGHLIGHTING))
			CtrlObject->HiFiles->GetHiColor(&CurPtr, 1, (Info.Flags & OPIF_USEATTRHIGHLIGHTING) != 0, &MarkLM );

		if (Info.HostFile && *Info.HostFile) {
			FAR_FIND_DATA_EX FindData;

			if (apiGetFindDataForExactPathName(Info.HostFile, FindData)) {
				CurPtr->WriteTime = FindData.ftLastWriteTime;
				CurPtr->CreationTime = FindData.ftCreationTime;
				CurPtr->AccessTime = FindData.ftLastAccessTime;
				CurPtr->ChangeTime = FindData.ftChangeTime;
			}
		}

		FileCount++;
	}

	if (CurFile >= FileCount)
		CurFile = FileCount ? FileCount - 1 : 0;

	/* $ 25.02.2001 VVM
		! Не считывать повторно список файлов с панели плагина */
	if (IsColumnDisplayed(DIZ_COLUMN))
		ReadDiz(PanelData, PluginFileCount, RDF_NO_UPDATE);

	CorrectPosition();
	CtrlObject->Plugins.FreeFindData(hPlugin, PanelData, PluginFileCount);

	if (KeepSelection || PrevSelFileCount > 0) {
		MoveSelection(ListData, FileCount, OldData, OldFileCount);
		DeleteListData(OldData, OldFileCount);
	}

	if (!KeepSelection && PrevSelFileCount > 0) {
		SaveSelection();
		ClearSelection();
	}

	SortFileList(false);

	if (CurFile >= FileCount || StrCmp(ListData[CurFile]->strName, strCurName))
		if (!GoToFile(strCurName) && !strNextCurName.IsEmpty())
			GoToFile(strNextCurName);

	SetTitle();
}

void FileList::ReadDiz(PluginPanelItem *ItemList, int ItemLength, DWORD dwFlags)
{
	if (DizRead)
		return;

	DizRead = true;
	Diz.Reset();

	if (PanelMode == NORMAL_PANEL) {
		Diz.Read(strCurDir);
	} else {
		PluginPanelItem *PanelData = nullptr;
		int PluginFileCount = 0;
		OpenPluginInfo Info;
		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

		if (!Info.DescrFilesNumber)
			return;

		int GetCode = TRUE;

		/* $ 25.02.2001 VVM
			+ Обработка флага RDF_NO_UPDATE */
		if (!ItemList && !(dwFlags & RDF_NO_UPDATE)) {
			GetCode = CtrlObject->Plugins.GetFindData(hPlugin, &PanelData, &PluginFileCount, 0);
		} else {
			PanelData = ItemList;
			PluginFileCount = ItemLength;
		}

		if (GetCode) {
			for (int I = 0; I < Info.DescrFilesNumber; I++) {
				PluginPanelItem *CurPanelData = PanelData;

				for (int J = 0; J < PluginFileCount; J++, CurPanelData++) {
					FARString strFileName = CurPanelData->FindData.lpwszFileName;

					if (!StrCmp(strFileName, Info.DescrFiles[I])) {
						FARString strTempDir, strDizName;

						if (FarMkTempEx(strTempDir) && apiCreateDirectory(strTempDir, nullptr)) {
							if (CtrlObject->Plugins.GetFile(hPlugin, CurPanelData, strTempDir, strDizName,
										OPM_SILENT | OPM_VIEW | OPM_QUICKVIEW | OPM_DESCR)) {
								strPluginDizName = Info.DescrFiles[I];
								Diz.Read(L"", strDizName);
								DeleteFileWithFolder(strDizName);
								I = Info.DescrFilesNumber;
								break;
							}

							apiRemoveDirectory(strTempDir);
							// ViewPanel->ShowFile(nullptr,FALSE,nullptr);
						}
					}
				}
			}

			/* $ 25.02.2001 VVM
				+ Обработка флага RDF_NO_UPDATE */
			if (!ItemList && !(dwFlags & RDF_NO_UPDATE))
				CtrlObject->Plugins.FreeFindData(hPlugin, PanelData, PluginFileCount);
		}
	}

	for (int I = 0; I < FileCount; I++) {
		if (!ListData[I]->DizText) {
			ListData[I]->DeleteDiz = false;
			ListData[I]->DizText = (wchar_t *)Diz.GetDizTextAddr(ListData[I]->strName, ListData[I]->FileSize);
		}
	}
}

void FileList::ReadSortGroups(bool UpdateFilterCurrentTime)
{
	if (!SortGroupsRead) {
		if (UpdateFilterCurrentTime) {
			CtrlObject->HiFiles->UpdateCurrentTime();
		}

		SortGroupsRead = TRUE;

		for (int i = 0; i < FileCount; i++) {
			ListData[i]->SortGroup = CtrlObject->HiFiles->GetGroup(ListData[i]);
		}
	}
}

// Обнулить текущий CurPtr и занести предопределенные данные для каталога ".."
FileListItem *FileList::AddParentPoint(long CurFilePos, FILETIME *Times, FARString Owner, FARString Group)
{
	FileListItem *CurPtr = new FileListItem;
	ListData[FileCount] = CurPtr;
	CurPtr->FileAttr = FILE_ATTRIBUTE_DIRECTORY;
	CurPtr->FileMode = S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
	CurPtr->strName = L"..";

	if (Times) {
		CurPtr->CreationTime = Times[0];
		CurPtr->AccessTime = Times[1];
		CurPtr->WriteTime = Times[2];
		CurPtr->ChangeTime = Times[3];
	}

	CurPtr->strOwner = Owner;
	CurPtr->strGroup = Group;
	CurPtr->Position = CurFilePos;
	return CurPtr;
}
