/*
history.cpp

История (Alt-F8, Alt-F11, Alt-F12)
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

#include "clipboard.hpp"
#include "config.hpp"
#include "ConfigRW.hpp"
#include "ctrlobj.hpp"
#include "datetime.hpp"
#include "dialog.hpp"
#include "DlgGuid.hpp"
#include "FileMasksProcessor.hpp"
#include "history.hpp"
#include "interf.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "vmenu.hpp"

static const char NKeyExtras[]   = "Extras";
static const char NKeyLastItem[] = "LastItem";
static const char NKeyLines[]    = "Lines";
static const char NKeyLocks[]    = "Locks";
static const char NKeyPosition[] = "Position";
static const char NKeyTimes[]    = "Times";
static const char NKeyTypes[]    = "Types";

static const wchar_t *GetNamePrefix(int Type)
{
	switch (Type) {
		case HR_VIEWER:             // вьювер
			return Msg::HistoryView;
		case HR_EDITOR:             // обычное открытие в редакторе
		case HR_EDITOR_RO:          // открытие с локом
			return Msg::HistoryEdit;
		case HR_EXTERNAL:           // external - без ожидания
		case HR_EXTERNAL_WAIT:      // external - AlwaysWaitFinish
			return Msg::HistoryExt;
	}

	return L"";
}

static void AppendWithLFSeparator(std::wstring &str, const FARString &ap, bool first)
{
	if (!first) {
		str+= L'\n';
	}
	size_t p = str.size();
	str.append(ap.CPtr(), ap.GetLength());
	for (; p < str.size(); ++p) {
		if (str[p] == L'\n') {
			str[p] = L'\r';
		}
	}
}

static bool IsSameDay(const FILETIME &ft1, const FILETIME &ft2)
{
	SYSTEMTIME st1, st2;
	FILETIME lt;

	WINPORT(FileTimeToLocalFileTime)(&ft1, &lt);
	WINPORT(FileTimeToSystemTime)(&lt, &st1);

	WINPORT(FileTimeToLocalFileTime)(&ft2, &lt);
	WINPORT(FileTimeToSystemTime)(&lt, &st2);

	return st1.wDay == st2.wDay && st1.wMonth == st2.wMonth && st1.wYear == st2.wYear;
}

History::History(enumHISTORYTYPE TypeHistory, size_t HistoryCount, const std::string &RegKey,
		const int *EnableSave, bool SaveType)
	:
	mHistoryType(TypeHistory),
	mMaxCount(HistoryCount),
	mSaveType(SaveType),
	mStrRegKey(RegKey),
	mEnableAdd(true),
	mKeepSelectedPos(false),
	mEnableSave(EnableSave),
	mRemoveDups(HRD_CASESENS),
	mIterCommon(mList.end()),
	mIterCmdLine(mList.end())
{
	if (*mEnableSave)
		ReadHistory();
}

void History::ResetPosition()
{
	mIterCommon = mList.end();
	mIterCmdLine = mList.end();
}

bool History::IsAllowedForHistory(const wchar_t *Str) const
{
	if (mHistoryType == HISTORYTYPE_CMD) {
		FileMasksProcessor fmp;
		return !(fmp.Set(Opt.AutoComplete.Exceptions.CPtr(), FMF_ADDASTERISK) && fmp.Compare(Str, true));
	}
	return true;
}

/*
   SaveForbid - принудительно запретить запись добавляемой строки.
				Используется на панели плагина
*/
void History::AddToHistoryExtra(const wchar_t *Str, const wchar_t *Extra, int Type,
		const wchar_t *Prefix, bool SaveForbid)
{
	if (!mEnableAdd)
		return;

	if (CtrlObject->Macro.IsExecuting() && CtrlObject->Macro.IsHistoryDisabled(mHistoryType))
		return;

	if (!IsAllowedForHistory(Str)) {
		fprintf(stderr, "AddToHistory - disallowed: '%ls'\n", Str);
		return;
	}

	SyncChanges();
	AddToHistoryLocal(Str, Extra, Prefix, Type);

	if (*mEnableSave && !SaveForbid)
		SaveHistory();
}

void History::AddToHistory(const wchar_t *Str, int Type, const wchar_t *Prefix, bool SaveForbid)
{
	AddToHistoryExtra(Str, nullptr, Type, Prefix, SaveForbid);
}

void History::AddToHistoryLocal(const wchar_t *Str, const wchar_t *Extra, const wchar_t *Prefix,
		int Type)
{
	HistoryRecord AddRecord;

	if (mHistoryType == HISTORYTYPE_FOLDER && Prefix && *Prefix) {
		AddRecord.strName = Prefix;
		AddRecord.strName+= L":";
	}

	AddRecord.strName+= Str;
	AddRecord.Type = Type;
	if (Extra) {
		AddRecord.strExtra = Extra;
	}
	ReplaceStrings(AddRecord.strName, L"\n", L"\r");

	if (mRemoveDups != HRD_NOREMOVE) // удалять дубликаты?
	{
		auto Cmp = (mRemoveDups == HRD_CASESENS) ? StrCmp : StrCmpI;
		for (auto Item = mList.begin(); Item != mList.end(); Item++) {
			if (EqualType(AddRecord.Type, Item->Type)) {
				if (!Cmp(AddRecord.strName, Item->strName)) {
					AddRecord.Lock = Item->Lock;
					mList.erase(Item);
					break;
				}
			}
		}
	}

	for (auto Item = mList.begin(); Item != mList.end() && mList.size() >= mMaxCount; ) {
		if (!Item->Lock)
			Item = mList.erase(Item);
		else
			Item++;
	}

	WINPORT(GetSystemTimeAsFileTime)(&AddRecord.Timestamp);    // in UTC
	mList.push_back(std::move(AddRecord));
	ResetPosition();
}

bool History::SaveHistory()
{
	if (!*mEnableSave)
		return true;

	if (mList.empty()) {
		ConfigWriter(mStrRegKey).RemoveSection();
		return true;
	}

	// for dialogs, locked items should show first (be last in the list)
	if (mHistoryType == HISTORYTYPE_DIALOG) {
		std::stable_partition(mList.begin(), mList.end(), [](const auto &x) { return !x.Lock; });
	}

	bool ret = false;
	try {
		bool HasExtras = false;
		std::wstring strTypes, strLines, strLocks, strExtras;
		std::vector<FILETIME> vTimes;
		int Position = -1;
		bool first = true;
		FARString strLastItem;
		FILETIME timeLastItem {};

		for (auto It = mList.crbegin(); It != mList.crend(); ++It, first=false) {
			AppendWithLFSeparator(strLines, It->strName, first);
			AppendWithLFSeparator(strExtras, It->strExtra, first);
			if (!It->strExtra.IsEmpty())
				HasExtras = true;

			if (mSaveType)
				strTypes+= L'0' + It->Type;

			strLocks+= (It->Lock ? L'1' : L'0');
			vTimes.emplace_back(It->Timestamp);

			if (WINPORT(CompareFileTime)(&It->Timestamp, &timeLastItem) > 0) {
				timeLastItem = It->Timestamp;
				strLastItem = It->strName;
			}
		}

		ConfigWriter cfg_writer(mStrRegKey);
		cfg_writer.SetString(NKeyLines, strLines.c_str());
		if (HasExtras) {
			cfg_writer.SetString(NKeyExtras, strExtras.c_str());
		} else {
			cfg_writer.RemoveKey(NKeyExtras);
		}
		if (mSaveType) {
			cfg_writer.SetString(NKeyTypes, strTypes.c_str());
		}
		cfg_writer.SetString(NKeyLocks, strLocks.c_str());
		cfg_writer.SetBytes(NKeyTimes, (const unsigned char *)&vTimes[0], vTimes.size() * sizeof(FILETIME));
		cfg_writer.SetInt(NKeyPosition, Position);
		cfg_writer.SetString(NKeyLastItem, strLastItem);

		ret = cfg_writer.Save();
		if (ret) {
			mLoadedStat = ConfigReader::SavedSectionStat(mStrRegKey);
		}

	} catch (std::exception &e) {
	}

	return ret;
}

bool History::ReadLastItem(const char *RegKey, FARString &strStr)
{
	strStr.Clear();
	ConfigReader cfg_reader(RegKey);
	return cfg_reader.HasSection() && cfg_reader.GetString(strStr, NKeyLastItem, L"");
}

bool History::ReadHistory()
{
	FARString strLines, strExtras, strLocks, strTypes;
	std::vector<unsigned char> vTimes;

	ConfigReader cfg_reader(mStrRegKey);

	if (!cfg_reader.GetString(strLines, NKeyLines, L""))
		return false;

	int Position = cfg_reader.GetInt(NKeyPosition, -1);
	cfg_reader.GetBytes(vTimes, NKeyTimes);
	cfg_reader.GetString(strLocks, NKeyLocks, L"");
	cfg_reader.GetString(strTypes, NKeyTypes, L"");
	cfg_reader.GetString(strExtras, NKeyExtras, L"");

	size_t LinesPos = 0, TypesPos = 0, LocksPos = 0, TimePos = 0, ExtrasPos = 0;
	for (size_t Count=0; LinesPos < strLines.GetLength() && Count < mMaxCount; Count++) {
		size_t LineEnd, ExtraEnd;
		if (!strLines.Pos(LineEnd, L'\n', LinesPos))
			LineEnd = strLines.GetLength();

		if (!strExtras.Pos(ExtraEnd, L'\n', ExtrasPos))
			ExtraEnd = strExtras.GetLength();

		mList.push_front(HistoryRecord());
		auto &AddRecord = mList.front();
		AddRecord.strName = strLines.SubStr(LinesPos, LineEnd - LinesPos);
		LinesPos = LineEnd + 1;
		AddRecord.strExtra = strExtras.SubStr(ExtrasPos, ExtraEnd - ExtrasPos);
		ExtrasPos = ExtraEnd + 1;

		if (TypesPos < strTypes.GetLength()) {
			if (iswdigit(strTypes[TypesPos])) {
				AddRecord.Type = strTypes[TypesPos] - L'0';
			}
			++TypesPos;
		}

		if (LocksPos < strLocks.GetLength()) {
			if (iswdigit(strLocks[LocksPos])) {
				AddRecord.Lock = (strLocks[LocksPos] != L'0');
			}
			++LocksPos;
		}

		if (TimePos + sizeof(FILETIME) <= vTimes.size()) {
			AddRecord.Timestamp = *(const FILETIME *)(vTimes.data() + TimePos);
			TimePos+= sizeof(FILETIME);
		}

		if ((int)Count == Position)
			mIterCommon = mList.begin();
	}

	mLoadedStat = cfg_reader.LoadedSectionStat();

	return true;
}

void History::SyncChanges()
{
	const struct stat &CurrentStat = ConfigReader::SavedSectionStat(mStrRegKey);
	if (mLoadedStat.st_ino != CurrentStat.st_ino || mLoadedStat.st_size != CurrentStat.st_size
			|| mLoadedStat.st_mtime != CurrentStat.st_mtime)
	{
		fprintf(stderr, "History::SyncChanges: %s\n", mStrRegKey.c_str());
		mList.clear();
		ResetPosition();
		ReadHistory();
	}
}

const wchar_t *History::GetDelTitle() const
{
	switch (mHistoryType) {
		case HISTORYTYPE_CMD:
		case HISTORYTYPE_DIALOG:
			return Msg::HistoryTitle;
		case HISTORYTYPE_FOLDER:
			return Msg::FolderHistoryTitle;
		default:
			return Msg::ViewHistoryTitle;
	}
}

int History::Select(FARString &strOut, int &TypeOut)
{
	const wchar_t *Title, *HelpTopic;
	const GUID *Guid;

	switch (mHistoryType) {
		case HISTORYTYPE_CMD:
			Title = Msg::HistoryTitle;
			HelpTopic = L"History";
			Guid = &HistoryCmdId;
			break;

		case HISTORYTYPE_FOLDER:
			Title = Msg::FolderHistoryTitle;
			HelpTopic = L"HistoryFolders";
			Guid = &HistoryFolderId;
			break;

		case HISTORYTYPE_VIEW:
			Title = Msg::ViewHistoryTitle;
			HelpTopic = L"HistoryViews";
			Guid = &HistoryEditViewId;
			break;

		default:
			return HRT_CANCEL;
	}

	const int Height = ScrY - 8;
	VMenu HistoryMenu(Title, nullptr, 0, Height);
	HistoryMenu.SetFlags(VMENU_SHOWAMPERSAND | VMENU_WRAPMODE);
	HistoryMenu.SetHelp(HelpTopic);
	HistoryMenu.SetId(*Guid);
	HistoryMenu.SetPosition(-1, -1, 0, 0);

	return ProcessMenu(HistoryMenu, Title, Height, strOut, TypeOut, nullptr);
}

int History::Select(VMenu &HistoryMenu, Dialog *Dlg, FARString &strOut)
{
	int TypeOut = HR_DEFAULT;
	return ProcessMenu(HistoryMenu, nullptr, Opt.Dialogs.CBoxMaxHeight, strOut, TypeOut, Dlg);
}

int History::ProcessMenu(VMenu &HistoryMenu, const wchar_t *Title, int Height, FARString &strOut,
		int &TypeOut, Dialog *Dlg)
{
	MenuItemEx MenuItem;
	auto SelectedRecord = mList.end();
	FarListPos Pos = {0, 0};
	int MenuExitCode = -1;
	int RetCode = HRT_ENTER;
	bool SetUpMenuPos = false;
	int DeltaMenuPos = -1;
	std::vector<Iter> IterVector;

	SyncChanges();
	if (mHistoryType == HISTORYTYPE_DIALOG && mList.empty())
		return HRT_CANCEL;

	mIterCommon = mList.end();
	for (bool Done = false; !Done; ) {
		IterVector.clear();
		IterVector.reserve(mList.size());

		bool IsUpdate = false;
		FILETIME CurTimestamp {};
		HistoryMenu.DeleteItems();
		HistoryMenu.Modal::ClearDone();
		if (Title) {
			HistoryMenu.SetTitle(FARString().Format(L"%ls (%lu)", Title, (unsigned long)mList.size()));
		}

		// заполнение пунктов меню
		for (auto Item = mHistoryType == HISTORYTYPE_DIALOG ? --mList.end() : mList.begin();
				Item != mList.end();
				mHistoryType == HISTORYTYPE_DIALOG ? --Item : ++Item)
		{
			if (Opt.HistoryShowDates && !IsSameDay(Item->Timestamp, CurTimestamp)) {
				FARString strDate, strTime;
				ConvertDate(Item->Timestamp, strDate, strTime, 0);
				MenuItem.Clear();
				MenuItem.strName = strDate;
				MenuItem.Flags = LIF_SEPARATOR;
				HistoryMenu.AddItem(&MenuItem);
				CurTimestamp = Item->Timestamp;
			}

			FARString strRecord;

			if (mHistoryType == HISTORYTYPE_VIEW) {
				strRecord+= GetNamePrefix(Item->Type);
				strRecord+= L":";
				strRecord+= (Item->Type == HR_EDITOR_RO ? L"-" : L" ");
			}

			strRecord+= Item->strName;

			MenuItem.Clear();
			MenuItem.strName = strRecord;
			MenuItem.SetCheck(Item->Lock ? 1 : 0);

			if (!SetUpMenuPos) {
				if (mIterCommon == Item || (mIterCommon == mList.end() && Item == --mList.end())) {
					Pos.SelectPos = HistoryMenu.GetItemCount();
					MenuItem.SetSelect(TRUE);
				}
			}

			// NB: VMenu just copies userdata pointers, no memory allocation takes place
			HistoryMenu.SetUserData(reinterpret_cast<void*>(IterVector.size()), sizeof(void*),
					HistoryMenu.AddItem(&MenuItem));
			IterVector.push_back(Item);
		}

		if (mHistoryType == HISTORYTYPE_DIALOG)
			Dlg->SetComboBoxPos();
		else
			HistoryMenu.SetPosition(-1, -1, 0, 0);

		if (SetUpMenuPos) {
			const auto ItemCount = HistoryMenu.GetItemCount();
			Pos.SelectPos = ItemCount ? Min(Pos.SelectPos, ItemCount - 1) : 0;
			Pos.TopPos = Min(Pos.TopPos, ItemCount - Height);
			HistoryMenu.SetSelectPos(&Pos);
			SetUpMenuPos = false;
		}
		else if (DeltaMenuPos >= 0) {
			Pos.TopPos = Min(Pos.SelectPos - DeltaMenuPos, HistoryMenu.GetItemCount() - Height);
			HistoryMenu.SetSelectPos(&Pos);
			DeltaMenuPos = -1;
		}

		/*BUGBUG???
			if (mHistoryType == HISTORYTYPE_DIALOG)
			{
					//  Перед отрисовкой спросим об изменении цветовых атрибутов
					BYTE RealColors[VMENU_COLOR_COUNT];
					FarListColors ListColors={0};
					ListColors.ColorCount=VMENU_COLOR_COUNT;
					ListColors.Colors=RealColors;
					HistoryMenu.GetColors(&ListColors);
					if(DlgProc((HANDLE)this,DN_CTLCOLORDLGLIST,CurItem->ID,(LONG_PTR)&ListColors))
						HistoryMenu.SetColors(&ListColors);
				}
		*/
		HistoryMenu.Show();

		while (!HistoryMenu.Done()) {
			if (mHistoryType == HISTORYTYPE_DIALOG && (!Dlg->GetDropDownOpened() || mList.empty())) {
				HistoryMenu.ProcessKey(KEY_ESC);
				continue;
			}

			FarKey Key = HistoryMenu.ReadInput();

			if (mHistoryType == HISTORYTYPE_DIALOG && Key == KEY_TAB) // Tab в списке хистори диалогов - аналог Enter
			{
				HistoryMenu.ProcessKey(KEY_ENTER);
				continue;
			}

			HistoryMenu.GetSelectPos(&Pos);
			auto IterIndex = reinterpret_cast<uintptr_t>
					(HistoryMenu.GetUserData(nullptr, sizeof(void*), Pos.SelectPos));
			auto CurrentIter = IterVector.empty() ? mList.end() : IterVector[IterIndex];

			switch (Key) {
				case KEY_CTRLR:    // обновить с удалением недоступных
					if (mHistoryType == HISTORYTYPE_FOLDER || mHistoryType == HISTORYTYPE_VIEW)
					{
						int DelCount = 0;

						for (auto &Item: mList) {
							Item.Marked = !Item.Lock && apiGetFileAttributes(Item.strName) == INVALID_FILE_ATTRIBUTES;
							if (Item.Marked)
								++DelCount;
						}

						if (DelCount && 0 == Message(MSG_WARNING, 2, Title,
								FARString().Format(Msg::HistoryRefreshConfirm, DelCount), Msg::Ok, Msg::Cancel))
						{
							for (auto Item = mList.begin(); Item != mList.end(); ) {
								if (Item->Marked)
									Item = mList.erase(Item);
								else
									++Item;
							}
							SaveHistory();
							HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
							HistoryMenu.SetUpdateRequired(TRUE);
							IsUpdate = true;
							ResetPosition();
						}
					}
					break;

				case KEY_CTRLSHIFTNUMENTER:
				case KEY_CTRLNUMENTER:
				case KEY_SHIFTNUMENTER:
				case KEY_CTRLSHIFTENTER:
				case KEY_CTRLENTER:
				case KEY_SHIFTENTER:
				case KEY_CTRLALTENTER:
				case KEY_CTRLALTNUMENTER: {
					if (mHistoryType == HISTORYTYPE_DIALOG)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done = true;
					RetCode = (Key == KEY_CTRLALTENTER || Key == KEY_CTRLALTNUMENTER) ? HRT_CTRLALTENTER
							: (Key == KEY_CTRLSHIFTENTER || Key == KEY_CTRLSHIFTNUMENTER) ? HRT_CTRLSHIFTENTER
							: (Key == KEY_SHIFTENTER || Key == KEY_SHIFTNUMENTER) ? HRT_SHIFTENTER : HRT_CTRLENTER;
					break;
				}

				case KEY_F3:
				case KEY_F4:
				case KEY_NUMPAD5:
				case KEY_SHIFTNUMPAD5:
					if (mHistoryType == HISTORYTYPE_VIEW) {
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						Done = true;
						RetCode = (Key == KEY_F4) ? HRT_F4 : HRT_F3;
					}
					break;

				// $ 09.04.2001 SVS - Фича - копирование из истории строки в Clipboard
				case KEY_CTRLC:
				case KEY_CTRLINS:
				case KEY_CTRLNUMPAD0: {
					if (CurrentIter != mList.end())
						CopyToClipboard(CurrentIter->strName);

					break;
				}

				// Lock/Unlock
				case KEY_INS:
				case KEY_NUMPAD0: {
					if (HistoryMenu.GetItemCount() /* > 1*/) {
						mIterCommon = CurrentIter;
						mIterCommon->Lock = !mIterCommon->Lock;
						HistoryMenu.Hide();
						ResetPosition();
						SaveHistory();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate = true;
						SetUpMenuPos = true;
					}

					break;
				}

				case KEY_SHIFTNUMDEL:
				case KEY_SHIFTDEL: {
					if (HistoryMenu.GetShowItemCount() /* > 1*/) {
						if (!CurrentIter->Lock) {
							HistoryMenu.Hide();
							mList.erase(CurrentIter);
							ResetPosition();
							SaveHistory();
							HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
							HistoryMenu.SetUpdateRequired(TRUE);
							IsUpdate = true;
							SetUpMenuPos = true;
						}
					}

					break;
				}

				case KEY_NUMDEL:
				case KEY_DEL: {
					if (HistoryMenu.GetItemCount() &&
							(!Opt.Confirm.HistoryClear ||
									!Message(MSG_WARNING, 2, GetDelTitle(), Msg::HistoryClear, Msg::Clear, Msg::Cancel)))
					{
						for (auto Item = mList.begin(); Item != mList.end();) {
							if (Item->Lock)    // залоченные не трогаем
								Item++;
							else
								Item = mList.erase(Item);
						}

						ResetPosition();
						HistoryMenu.Hide();
						SaveHistory();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate = true;
					}

					break;
				}

				case KEY_CTRLT:
					Opt.HistoryShowDates = !Opt.HistoryShowDates;
					mIterCommon = CurrentIter;
					DeltaMenuPos = Pos.SelectPos - Pos.TopPos;
					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					HistoryMenu.SetUpdateRequired(TRUE);
					IsUpdate = true;
					break;

				default:
					HistoryMenu.ProcessInput();
					break;
			}
		}

		if (IsUpdate)
			continue;

		Done = true;
		MenuExitCode = HistoryMenu.Modal::GetExitCode();

		if (MenuExitCode >= 0)
		{
			auto IterIndex = reinterpret_cast<uintptr_t>
					(HistoryMenu.GetUserData(nullptr, sizeof(void*), Pos.SelectPos));
			SelectedRecord = IterVector[IterIndex];

			if (SelectedRecord == mList.end())
				return HRT_CANCEL;

			const auto HR = SelectedRecord->Type;
			if (HR != HR_EXTERNAL && HR != HR_EXTERNAL_WAIT // ignore external
					&& RetCode != HRT_CTRLENTER
					&& (mHistoryType == HISTORYTYPE_VIEW || (mHistoryType == HISTORYTYPE_FOLDER && HR == HR_DEFAULT))
					&& apiGetFileAttributes(SelectedRecord->strName) == INVALID_FILE_ATTRIBUTES)
			{
				WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);

				if (mHistoryType == HISTORYTYPE_VIEW && HR == HR_EDITOR) // Edit? тогда спросим и если надо создадим
				{
					if (!Message(MSG_WARNING | MSG_ERRORTYPE, 2, Title, SelectedRecord->strName,
								Msg::ViewHistoryIsCreate, Msg::HYes, Msg::HNo))
						break;
				}
				else
					Message(MSG_WARNING | MSG_ERRORTYPE, 1, Title, SelectedRecord->strName, Msg::Ok);

				Done = false;
				SetUpMenuPos = true;
				HistoryMenu.Modal::SetExitCode(Pos.SelectPos = MenuExitCode);
				continue;
			}
		}
	}

	if (MenuExitCode < 0 || SelectedRecord == mList.end())
		return HRT_CANCEL;

	if (mKeepSelectedPos) {
		mIterCommon = SelectedRecord;
	}

	strOut = SelectedRecord->strName;

	switch(RetCode) {
		case HRT_CANCEL:
			break;

		case HRT_ENTER:
		case HRT_SHIFTENTER:
		case HRT_CTRLENTER:
		case HRT_CTRLSHIFTENTER:
		case HRT_CTRLALTENTER:
			TypeOut = SelectedRecord->Type;
			break;

		case HRT_F3:
			TypeOut = HR_VIEWER;
			RetCode = HRT_ENTER;
			break;

		case HRT_F4:
			TypeOut = (SelectedRecord->Type == HR_EDITOR_RO) ? HR_EDITOR_RO : HR_EDITOR;
			RetCode = HRT_ENTER;
			break;
	}

	return RetCode;
}

void History::GetPrev(FARString &strStr)
{
	mIterCmdLine--;

	if (mIterCmdLine == mList.end()) {
		SyncChanges();
		mIterCmdLine = mList.begin();
	}

	if (mIterCmdLine != mList.end())
		strStr = mIterCmdLine->strName;
	else
		strStr.Clear();
}

void History::GetNext(FARString &strStr)
{
	if (mIterCmdLine != mList.end())
		mIterCmdLine++;
	else
		SyncChanges();

	if (mIterCmdLine != mList.end())
		strStr = mIterCmdLine->strName;
	else
		strStr.Clear();
}

bool History::DeleteMatching(FARString &strStr)
{
	SyncChanges();

	auto Item = mIterCommon;
	for (Item--; Item != mIterCommon; Item--) {
		if (Item == mList.end() || Item->Lock)
			continue;

		if (Item->strName == strStr) {
			mList.erase(Item);
			SaveHistory();
			return true;
		}
	}

	return false;
}

bool History::GetSimilar(FARString &strStr, int LastCmdPartLength, bool bAppend)
{
	SyncChanges();
	int Length = (int)strStr.GetLength();

	if (LastCmdPartLength != -1 && LastCmdPartLength < Length)
		Length = LastCmdPartLength;

	if (LastCmdPartLength == -1) {
		ResetPosition();
	}

	auto Item = mIterCommon;
	for (Item--; Item != mIterCommon; Item--) {
		if (Item == mList.end())
			continue;

		if (!StrCmpNI(strStr, Item->strName, Length) && StrCmp(strStr, Item->strName)) {
			if (bAppend)
				strStr+= &Item->strName[Length];
			else
				strStr = Item->strName;

			mIterCommon = Item;
			return true;
		}
	}

	return false;
}

bool History::GetAllSimilar(VMenu &HistoryMenu, const wchar_t *Str)
{
	SyncChanges();
	int Length = StrLength(Str);
	for (auto Item = mList.rbegin(); Item != mList.rend(); Item++) {
		if (!StrCmpNI(Str, Item->strName, Length)
			&& StrCmp(Str, Item->strName)
			&& IsAllowedForHistory(Item->strName)
			&& HistoryMenu.FindItem(0, Item->strName, LIFIND_EXACTMATCH | LIFIND_KEEPAMPERSAND) < 0)
						// after #2241 history may have duplicate names
		{
			HistoryMenu.AddItem(Item->strName);
		}
	}
	return false;
}

void History::SetAddMode(bool EnableAdd, history_remove_dups RemoveDups, bool KeepSelectedPos)
{
	mEnableAdd = EnableAdd;
	mRemoveDups = RemoveDups;
	mKeepSelectedPos = KeepSelectedPos;
}

bool History::EqualType(int Type1, int Type2) const
{
	return (Type1 == Type2) || (mHistoryType == HISTORYTYPE_VIEW &&
		((Type1 == HR_EDITOR_RO && Type2 == HR_EDITOR) || (Type1 == HR_EDITOR && Type2 == HR_EDITOR_RO)));
}
