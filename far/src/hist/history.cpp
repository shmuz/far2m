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

#include "history.hpp"
#include "keys.hpp"
#include "vmenu.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "config.hpp"
#include "ConfigRW.hpp"
#include "strmix.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include "ctrlobj.hpp"
#include "DlgGuid.hpp"
#include <crc64.h>
#include "FileMasksProcessor.hpp"

History::History(enumHISTORYTYPE TypeHistory, size_t HistoryCount, const std::string &RegKey,
		const int *EnableSave, bool SaveType)
	:
	mTypeHistory(TypeHistory),
	mHistoryCount(HistoryCount),
	mSaveType(SaveType),
	mStrRegKey(RegKey),
	mEnableAdd(true),
	mKeepSelectedPos(false),
	mEnableSave(EnableSave),
	mRemoveDups(1),
	mCurrentItem(mHistoryList.end())
{
	if (*mEnableSave)
		ReadHistory();
}

static bool IsAllowedForHistory(const wchar_t *Str)
{
	FileMasksProcessor fmp;
	return !(fmp.Set(Opt.AutoComplete.Exceptions.CPtr(), FMF_ADDASTERISK) && fmp.Compare(Str, true));
}

/*
   SaveForbid - принудительно запретить запись добавляемой строки.
				Используется на панели плагина
*/
void History::AddToHistoryExtra(const wchar_t *Str, const wchar_t *Extra, int Type, const wchar_t *Prefix,
		bool SaveForbid)
{
	if (!mEnableAdd)
		return;

	if (CtrlObject->Macro.IsExecuting() && CtrlObject->Macro.IsHistoryDisabled(Type))
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

void History::AddToHistoryLocal(const wchar_t *Str, const wchar_t *Extra, const wchar_t *Prefix, int Type)
{
	if (!Str)
		return;

	HistoryRecord AddRecord;

	if (mTypeHistory == HISTORYTYPE_FOLDER && Prefix && *Prefix) {
		AddRecord.strName = Prefix;
		AddRecord.strName+= L":";
	}

	AddRecord.strName+= Str;
	AddRecord.Type = Type;
	if (Extra) {
		AddRecord.strExtra = Extra;
	}
	ReplaceStrings(AddRecord.strName, L"\n", L"\r");

	if (mRemoveDups)    // удалять дубликаты?
	{
		for (auto Item = mHistoryList.begin(); Item != mHistoryList.end(); Item++) {
			if (EqualType(AddRecord.Type, Item->Type)) {
				if ((mRemoveDups == 1 && !StrCmp(AddRecord.strName, Item->strName))
						|| (mRemoveDups == 2 && !StrCmpI(AddRecord.strName, Item->strName))) {
					AddRecord.Lock = Item->Lock;
					mHistoryList.erase(Item);
					break;
				}
			}
		}
	}

	for (auto Item = mHistoryList.begin();
			Item != mHistoryList.end() && mHistoryList.size() >= mHistoryCount;) {
		if (!Item->Lock)
			Item = mHistoryList.erase(Item);
		else
			Item++;
	}

	WINPORT(GetSystemTimeAsFileTime)(&AddRecord.Timestamp);    // in UTC
	mHistoryList.push_back(std::move(AddRecord));
	ResetPosition();
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

bool History::SaveHistory()
{
	if (!*mEnableSave)
		return true;

	if (mHistoryList.empty()) {
		ConfigWriter(mStrRegKey).RemoveSection();
		return true;
	}

	// for dialogs, locked items should show first (be last in the list)
	if (mTypeHistory == HISTORYTYPE_DIALOG) {
		auto LastItem = --mHistoryList.end();
		for (auto Item = mHistoryList.begin(); Item != mHistoryList.end();) {
			const auto tmp = Item;

			Item++;

			if (tmp->Lock)
				mHistoryList.splice(mHistoryList.cend(), mHistoryList, tmp);

			if (tmp == LastItem)
				break;
		}
	}

	bool ret = false;
	try {
		bool HasExtras = false;
		std::wstring strTypes, strLines, strLocks, strExtras;
		std::vector<FILETIME> vTimes;
		int Position = -1;
		size_t i = mHistoryList.size();

		for (auto Item = --mHistoryList.cend(); Item != mHistoryList.cend(); Item--) {
			AppendWithLFSeparator(strLines, Item->strName, i == mHistoryList.size());
			AppendWithLFSeparator(strExtras, Item->strExtra, i == mHistoryList.size());
			if (!Item->strExtra.IsEmpty()) {
				HasExtras = true;
			}

			if (mSaveType)
				strTypes+= L'0' + Item->Type;

			strLocks+= L'0' + Item->Lock;
			vTimes.emplace_back(Item->Timestamp);

			--i;

			if (Item == mCurrentItem)
				Position = static_cast<int>(i);
		}

		ConfigWriter cfg_writer(mStrRegKey);
		cfg_writer.SetString("Lines", strLines.c_str());
		if (HasExtras) {
			cfg_writer.SetString("Extras", strExtras.c_str());
		} else {
			cfg_writer.RemoveKey("Extras");
		}
		if (mSaveType) {
			cfg_writer.SetString("Types", strTypes.c_str());
		}
		cfg_writer.SetString("Locks", strLocks.c_str());
		cfg_writer.SetBytes("Times", (const unsigned char *)&vTimes[0], vTimes.size() * sizeof(FILETIME));
		cfg_writer.SetInt("Position", Position);

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
	if (!cfg_reader.HasSection())
		return false;

	if (!cfg_reader.GetString(strStr, "Lines", L""))
		return false;

	// last item is first in config
	size_t p;
	if (strStr.Pos(p, L'\n'))
		strStr.Remove(p, strStr.GetLength() - p);

	return true;
}

bool History::ReadHistory()
{
	FARString strLines, strExtras, strLocks, strTypes;
	std::vector<unsigned char> vTimes;

	ConfigReader cfg_reader(mStrRegKey);

	if (!cfg_reader.GetString(strLines, "Lines", L""))
		return false;

	int Position = cfg_reader.GetInt("Position", -1);
	cfg_reader.GetBytes(vTimes, "Times");
	cfg_reader.GetString(strLocks, "Locks", L"");
	cfg_reader.GetString(strTypes, "Types", L"");
	cfg_reader.GetString(strExtras, "Extras", L"");

	size_t LinesPos = 0, TypesPos = 0, LocksPos = 0, TimePos = 0, ExtrasPos = 0;
	for (size_t Count=0; LinesPos < strLines.GetLength() && Count < mHistoryCount; Count++) {
		size_t LineEnd, ExtraEnd;
		if (!strLines.Pos(LineEnd, L'\n', LinesPos))
			LineEnd = strLines.GetLength();

		if (!strExtras.Pos(ExtraEnd, L'\n', ExtrasPos))
			ExtraEnd = strExtras.GetLength();

		mHistoryList.push_front(HistoryRecord());
		auto &AddRecord = mHistoryList.front();
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
			mCurrentItem = mHistoryList.begin();
	}

	mLoadedStat = cfg_reader.LoadedSectionStat();

	return true;
}

void History::SyncChanges()
{
	const struct stat &CurrentStat = ConfigReader::SavedSectionStat(mStrRegKey);
	if (mLoadedStat.st_ino != CurrentStat.st_ino || mLoadedStat.st_size != CurrentStat.st_size
			|| mLoadedStat.st_mtime != CurrentStat.st_mtime) {
		fprintf(stderr, "History::SyncChanges: %s\n", mStrRegKey.c_str());
		mHistoryList.clear();
		mCurrentItem = mHistoryList.end();
		ReadHistory();
	}
}

const wchar_t *History::GetTitle(int Type)
{
	switch (Type) {
		case 0:    // вьювер
			return Msg::HistoryView;
		case 1:    // обычное открытие в редакторе
		case 4:    // открытие с локом
			return Msg::HistoryEdit;
		case 2:    // external - без ожидания
		case 3:    // external - AlwaysWaitFinish
			return Msg::HistoryExt;
	}

	return L"";
}

const wchar_t *History::GetDelTitle()
{
	switch (mTypeHistory) {
		case HISTORYTYPE_CMD:
		case HISTORYTYPE_DIALOG:
			return Msg::HistoryTitle;
		case HISTORYTYPE_FOLDER:
			return Msg::FolderHistoryTitle;
		default:
			return Msg::ViewHistoryTitle;
	}
}

int History::Select(const wchar_t *Title, const wchar_t *HelpTopic, FARString &strStr, int &Type)
{
	int Height = ScrY - 8;
	VMenu HistoryMenu(Title, nullptr, 0, Height);
	HistoryMenu.SetFlags(VMENU_SHOWAMPERSAND | VMENU_WRAPMODE);
	switch (mTypeHistory) {
		case HISTORYTYPE_CMD:
			HistoryMenu.SetId(HistoryCmdId);
			break;
		case HISTORYTYPE_FOLDER:
			HistoryMenu.SetId(HistoryFolderId);
			break;
		case HISTORYTYPE_VIEW:
			HistoryMenu.SetId(HistoryEditViewId);
			break;
		default:
			break;
	}

	if (HelpTopic)
		HistoryMenu.SetHelp(HelpTopic);

	HistoryMenu.SetPosition(-1, -1, 0, 0);
	return ProcessMenu(strStr, Title, HistoryMenu, Height, Type, nullptr);
}

int History::Select(VMenu &HistoryMenu, int Height, Dialog *Dlg, FARString &strStr)
{
	int Type = 0;
	return ProcessMenu(strStr, nullptr, HistoryMenu, Height, Type, Dlg);
}

/*
 Return:
  -1 - Error???
   0 - Esc
   1 - Enter
   2 - Shift-Enter
   3 - Ctrl-Enter
   4 - F3
   5 - F4
   6 - Ctrl-Shift-Enter
   7 - Ctrl-Alt-Enter
*/
int History::ProcessMenu(FARString &strStr, const wchar_t *Title, VMenu &HistoryMenu, int Height, int &Type,
		Dialog *Dlg)
{
	MenuItemEx MenuItem;
	auto SelectedRecord = mHistoryList.end();
	FarListPos Pos = {0, 0};
	int Code = -1;
	int RetCode = 1;
	bool Done = false;
	bool SetUpMenuPos = false;

	SyncChanges();
	if (mTypeHistory == HISTORYTYPE_DIALOG && mHistoryList.empty())
		return 0;

	std::vector<Iter> IterVector(mHistoryList.size());
	while (!Done) {
		int IterIndex = 0;
		bool IsUpdate = false;
		HistoryMenu.DeleteItems();
		HistoryMenu.Modal::ClearDone();
		HistoryMenu.SetBottomTitle(Msg::HistoryFooter);

		// заполнение пунктов меню
		for (auto Item = mTypeHistory == HISTORYTYPE_DIALOG ? --mHistoryList.end() : mHistoryList.begin();
				Item != mHistoryList.end();
				mTypeHistory == HISTORYTYPE_DIALOG ? --Item : ++Item) {
			FARString strRecord;

			if (mTypeHistory == HISTORYTYPE_VIEW) {
				strRecord+= GetTitle(Item->Type);
				strRecord+= L":";
				strRecord+= (Item->Type == 4 ? L"-" : L" ");
			}

			/*
				TODO: возможно здесь! или выше....
				char Date[16],Time[16], OutStr[32];
				ConvertDate(Item->Timestamp,Date,Time,5,TRUE,FALSE,TRUE,TRUE);
				а дальше
				strRecord += дату и время
			*/
			strRecord+= Item->strName;

			MenuItem.Clear();
			MenuItem.strName = strRecord;
			MenuItem.SetCheck(Item->Lock ? 1 : 0);

			if (!SetUpMenuPos)
				MenuItem.SetSelect(mCurrentItem == Item
						|| (mCurrentItem == mHistoryList.end() && Item == --mHistoryList.end()));

			// NB: here is really should be used sizeof(Item), not sizeof(*Item)
			// cuz sizeof(void *) has special meaning in SetUserData!
			IterVector[IterIndex] = Item;
			HistoryMenu.SetUserData(IterVector.data() + IterIndex, sizeof(void *),
					HistoryMenu.AddItem(&MenuItem));
			IterIndex++;
		}

		if (mTypeHistory == HISTORYTYPE_DIALOG)
			Dlg->SetComboBoxPos();
		else
			HistoryMenu.SetPosition(-1, -1, 0, 0);

		if (SetUpMenuPos) {
			Pos.SelectPos =
					Pos.SelectPos < (int)mHistoryList.size() ? Pos.SelectPos : (int)mHistoryList.size() - 1;
			Pos.TopPos = Min(Pos.TopPos, HistoryMenu.GetItemCount() - Height);
			HistoryMenu.SetSelectPos(&Pos);
			SetUpMenuPos = false;
		}

		/*BUGBUG???
			if (mTypeHistory == HISTORYTYPE_DIALOG)
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
			if (mTypeHistory == HISTORYTYPE_DIALOG && (!Dlg->GetDropDownOpened() || mHistoryList.empty())) {
				HistoryMenu.ProcessKey(KEY_ESC);
				continue;
			}

			FarKey Key = HistoryMenu.ReadInput();

			if (mTypeHistory == HISTORYTYPE_DIALOG && Key == KEY_TAB)    // Tab в списке хистори диалогов - аналог Enter
			{
				HistoryMenu.ProcessKey(KEY_ENTER);
				continue;
			}

			HistoryMenu.GetSelectPos(&Pos);
			Iter *userdata = (Iter *)HistoryMenu.GetUserData(nullptr, sizeof(void *), Pos.SelectPos);
			auto CurrentRecord = userdata ? *userdata : mHistoryList.end();

			switch (Key) {
				case KEY_CTRLR:    // обновить с удалением недоступных
				{
					if (mTypeHistory == HISTORYTYPE_FOLDER || mTypeHistory == HISTORYTYPE_VIEW) {
						bool ModifiedHistory = false;

						for (auto Item = mHistoryList.begin(); Item != mHistoryList.end();) {
							// залоченные не трогаем
							if (!Item->Lock
									&& (apiGetFileAttributes(Item->strName) == INVALID_FILE_ATTRIBUTES)) {
								Item = mHistoryList.erase(Item);    // убить запись из истории
								ModifiedHistory = true;
							} else
								Item++;
						}

						if (ModifiedHistory)    // избавляемся от лишних телодвижений
						{
							SaveHistory();      // сохранить
							HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
							HistoryMenu.SetUpdateRequired(TRUE);
							IsUpdate = true;
						}

						ResetPosition();
					}

					break;
				}
				case KEY_CTRLSHIFTNUMENTER:
				case KEY_CTRLNUMENTER:
				case KEY_SHIFTNUMENTER:
				case KEY_CTRLSHIFTENTER:
				case KEY_CTRLENTER:
				case KEY_SHIFTENTER:
				case KEY_CTRLALTENTER:
				case KEY_CTRLALTNUMENTER: {
					if (mTypeHistory == HISTORYTYPE_DIALOG)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done = true;
					RetCode = Key == KEY_CTRLALTENTER || Key == KEY_CTRLALTNUMENTER
							? 7
							: (Key == KEY_CTRLSHIFTENTER || Key == KEY_CTRLSHIFTNUMENTER
											? 6
											: (Key == KEY_SHIFTENTER || Key == KEY_SHIFTNUMENTER ? 2 : 3));
					break;
				}
				case KEY_F3:
				case KEY_F4:
				case KEY_NUMPAD5:
				case KEY_SHIFTNUMPAD5: {
					if (mTypeHistory == HISTORYTYPE_DIALOG || mTypeHistory == HISTORYTYPE_CMD
							|| mTypeHistory == HISTORYTYPE_FOLDER)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done = true;
					RetCode = (Key == KEY_F4 ? 5 : 4);
					break;
				}
				// $ 09.04.2001 SVS - Фича - копирование из истории строки в Clipboard
				case KEY_CTRLC:
				case KEY_CTRLINS:
				case KEY_CTRLNUMPAD0: {
					if (CurrentRecord != mHistoryList.end())
						CopyToClipboard(CurrentRecord->strName);

					break;
				}
				// Lock/Unlock
				case KEY_INS:
				case KEY_NUMPAD0: {
					if (HistoryMenu.GetItemCount() /* > 1*/) {
						mCurrentItem = CurrentRecord;
						mCurrentItem->Lock = mCurrentItem->Lock ? false : true;
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
					if (HistoryMenu.GetItemCount() /* > 1*/) {
						if (!CurrentRecord->Lock) {
							HistoryMenu.Hide();
							mHistoryList.erase(CurrentRecord);
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
						for (auto Item = mHistoryList.begin(); Item != mHistoryList.end();) {
							if (Item->Lock)    // залоченные не трогаем
								Item++;
							else
								Item = mHistoryList.erase(Item);
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
				default:
					HistoryMenu.ProcessInput();
					break;
			}
		}

		if (IsUpdate)
			continue;

		Done = true;
		Code = HistoryMenu.Modal::GetExitCode();

		if (Code >= 0) {
			SelectedRecord = *(Iter *)HistoryMenu.GetUserData(nullptr, sizeof(Iter *), Code);

			if (SelectedRecord == mHistoryList.end())
				return -1;

			// BUGBUG: eliminate those magic numbers!
			if (SelectedRecord->Type != 2 && SelectedRecord->Type != 3    // ignore external
					&& RetCode != 3
					&& ((mTypeHistory == HISTORYTYPE_FOLDER && !SelectedRecord->Type)
							|| mTypeHistory == HISTORYTYPE_VIEW)
					&& apiGetFileAttributes(SelectedRecord->strName) == INVALID_FILE_ATTRIBUTES)
			{
				WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);

				if (SelectedRecord->Type == 1 && mTypeHistory == HISTORYTYPE_VIEW)    // Edit? тогда спросим и если надо создадим
				{
					if (!Message(MSG_WARNING | MSG_ERRORTYPE, 2, Title, SelectedRecord->strName,
								Msg::ViewHistoryIsCreate, Msg::HYes, Msg::HNo))
						break;
				} else {
					Message(MSG_WARNING | MSG_ERRORTYPE, 1, Title, SelectedRecord->strName, Msg::Ok);
				}

				Done = false;
				SetUpMenuPos = true;
				HistoryMenu.Modal::SetExitCode(Pos.SelectPos = Code);
				continue;
			}
		}
	}

	if (Code < 0 || SelectedRecord == mHistoryList.end())
		return 0;

	if (mKeepSelectedPos) {
		mCurrentItem = SelectedRecord;
	}

	strStr = SelectedRecord->strName;

	if (RetCode < 4 || RetCode == 6 || RetCode == 7) {
		Type = SelectedRecord->Type;
	} else {
		Type = RetCode - 4;

		if (Type == 1 && SelectedRecord->Type == 4)
			Type = 4;

		RetCode = 1;
	}

	return RetCode;
}

void History::GetPrev(FARString &strStr)
{
	mCurrentItem--;

	if (mCurrentItem == mHistoryList.end()) {
		SyncChanges();
		mCurrentItem = mHistoryList.begin();
	}

	if (mCurrentItem != mHistoryList.end())
		strStr = mCurrentItem->strName;
	else
		strStr.Clear();
}

void History::GetNext(FARString &strStr)
{
	if (mCurrentItem != mHistoryList.end())
		mCurrentItem++;
	else
		SyncChanges();

	if (mCurrentItem != mHistoryList.end())
		strStr = mCurrentItem->strName;
	else
		strStr.Clear();
}

bool History::DeleteMatching(FARString &strStr)
{
	SyncChanges();

	auto Item = mCurrentItem;
	for (Item--; Item != mCurrentItem; Item--) {
		if (Item == mHistoryList.end() || Item->Lock)
			continue;

		if (Item->strName == strStr) {
			mHistoryList.erase(Item);
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

	auto Item = mCurrentItem;
	for (Item--; Item != mCurrentItem; Item--) {
		if (Item == mHistoryList.end())
			continue;

		if (!StrCmpNI(strStr, Item->strName, Length) && StrCmp(strStr, Item->strName)) {
			if (bAppend)
				strStr+= &Item->strName[Length];
			else
				strStr = Item->strName;

			mCurrentItem = Item;
			return true;
		}
	}

	return false;
}

bool History::GetAllSimilar(VMenu &HistoryMenu, const wchar_t *Str)
{
	SyncChanges();
	int Length = StrLength(Str);
	for (auto Item = --mHistoryList.end(); Item != mHistoryList.end(); Item--) {
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

void History::SetAddMode(bool EnableAdd, int RemoveDups, bool KeepSelectedPos)
{
	mEnableAdd = EnableAdd;
	mRemoveDups = RemoveDups;
	mKeepSelectedPos = KeepSelectedPos;
}

bool History::EqualType(int Type1, int Type2)
{
	return (Type1 == Type2)
			|| (mTypeHistory == HISTORYTYPE_VIEW
					&& ((Type1 == 4 && Type2 == 1) || (Type1 == 1 && Type2 == 4)));
}
