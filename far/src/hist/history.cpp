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

static uint64_t RegKey2ID(const FARString &str)
{
	const std::string &s = str.GetMB();
	return crc64(0, (const unsigned char *)s.c_str(), s.size());
}

History::History(enumHISTORYTYPE TypeHistory, size_t HistoryCount, const std::string &RegKey, const int *EnableSave, bool SaveType):
	strRegKey(RegKey),
	EnableAdd(true),
	KeepSelectedPos(false),
	SaveType(SaveType),
	RemoveDups(1),
	TypeHistory(TypeHistory),
	HistoryCount(HistoryCount),
	EnableSave(EnableSave),
	CurrentItem(HistoryList.end())
{
	if (*EnableSave)
		ReadHistory();
}

History::~History()
{
}

static bool IsAllowedForHistory(const wchar_t *Str)
{
	FileMasksProcessor fmp;
	return !(fmp.Set(Opt.AutoComplete.Exceptions.CPtr(), FMF_ADDASTERISK) && fmp.Compare(Str,true));
}


/*
   SaveForbid - принудительно запретить запись добавляемой строки.
                Используется на панели плагина
*/
void History::AddToHistoryExtra(const wchar_t *Str, const wchar_t *Extra, int Type, const wchar_t *Prefix, bool SaveForbid)
{
	if (!EnableAdd)
		return;

	if (CtrlObject->Macro.IsExecuting() && CtrlObject->Macro.IsHistoryDisabled(Type))
		return;

	if (!IsAllowedForHistory(Str)) {
		fprintf(stderr, "AddToHistory - disallowed: '%ls'\n", Str);
		return;
	}

	SyncChanges();
	AddToHistoryLocal(Str, Extra, Prefix, Type);

	if (*EnableSave && !SaveForbid)
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

	if (TypeHistory == HISTORYTYPE_FOLDER && Prefix && *Prefix)
	{
		AddRecord.strName = Prefix;
		AddRecord.strName += L":";
	}

	AddRecord.strName += Str;
	AddRecord.Type=Type;
	if (Extra) {
		AddRecord.strExtra = Extra;
	}

	if (RemoveDups) // удалять дубликаты?
	{
		for (auto HistoryItem=HistoryList.begin(); HistoryItem!=HistoryList.end(); HistoryItem++)
		{
			if (EqualType(AddRecord.Type,HistoryItem->Type))
			{
				if ((RemoveDups==1 && !StrCmp(AddRecord.strName,HistoryItem->strName)) ||
				        (RemoveDups==2 && !StrCmpI(AddRecord.strName,HistoryItem->strName)))
				{
					AddRecord.Lock=HistoryItem->Lock;
					HistoryList.erase(HistoryItem);
					break;
				}
			}
		}
	}

	if (HistoryList.size()>=HistoryCount)
	{
		for (auto HistoryItem=HistoryList.begin(); HistoryItem!=HistoryList.end() && HistoryList.size()>=HistoryCount; )
		{
			if (!HistoryItem->Lock)
				HistoryItem = HistoryList.erase(HistoryItem);
			else
				HistoryItem++;
		}
	}

	WINPORT(GetSystemTimeAsFileTime)(&AddRecord.Timestamp); // in UTC
	HistoryList.push_back(std::move(AddRecord));
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
	if (!*EnableSave)
		return true;

	if (HistoryList.empty())
	{
		ConfigWriter(strRegKey).RemoveSection();
		return true;
	}

	//for dialogs, locked items should show first (be last in the list)
	if (TypeHistory == HISTORYTYPE_DIALOG)
	{
		auto LastItem = --HistoryList.end();
		for (auto HistoryItem=HistoryList.begin(); HistoryItem!=HistoryList.end(); )
		{
			const auto tmp = HistoryItem;

			HistoryItem++;

			if (tmp->Lock)
				HistoryList.splice(HistoryList.cend(), HistoryList, tmp);

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
		size_t i = HistoryList.size();

		for (auto HistoryItem = --HistoryList.cend(); HistoryItem!=HistoryList.cend(); HistoryItem--)
		{
			AppendWithLFSeparator(strLines, HistoryItem->strName, i == HistoryList.size());
			AppendWithLFSeparator(strExtras, HistoryItem->strExtra, i == HistoryList.size());
			if (!HistoryItem->strExtra.IsEmpty()) {
				HasExtras = true;
			}

			if (SaveType)
				strTypes+= L'0' + HistoryItem->Type;

			strLocks+= L'0' + HistoryItem->Lock;
			vTimes.emplace_back(HistoryItem->Timestamp);

			--i;

			if (HistoryItem == CurrentItem)
				Position = static_cast<int>(i);
		}

		ConfigWriter cfg_writer(strRegKey);
		cfg_writer.SetString("Lines", strLines.c_str());
		if (HasExtras) {
			cfg_writer.SetString("Extras", strExtras.c_str());
		} else {
			cfg_writer.RemoveKey("Extras");
		}
		if (SaveType) {
			cfg_writer.SetString("Types", strTypes.c_str());
		}
		cfg_writer.SetString("Locks", strLocks.c_str());
		cfg_writer.SetBytes("Times", (const unsigned char *)&vTimes[0], vTimes.size() * sizeof(FILETIME));
		cfg_writer.SetInt("Position", Position);

		ret = cfg_writer.Save();
		if (ret) {
			LoadedStat = ConfigReader::SavedSectionStat(strRegKey);
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

bool History::ReadHistory(bool bOnlyLines)
{
	int Position = -1;
	FARString strLines, strExtras, strLocks, strTypes;
	std::vector<unsigned char> vTimes;

	ConfigReader cfg_reader(strRegKey);

	if (!cfg_reader.GetString(strLines, "Lines", L""))
		return false;

	if (!bOnlyLines)
	{
		Position = cfg_reader.GetInt("Position", Position);
		cfg_reader.GetBytes(vTimes, "Times");
		cfg_reader.GetString(strLocks, "Locks", L"");
		cfg_reader.GetString(strTypes, "Types", L"");
		cfg_reader.GetString(strExtras, "Extras", L"");
	}

	size_t StrPos = 0, LinesPos = 0, TypesPos = 0, LocksPos = 0, TimePos = 0, ExtrasPos = 0;
	while (LinesPos < strLines.GetLength() && StrPos < HistoryCount)
	{
		size_t LineEnd, ExtraEnd;
		if (!strLines.Pos(LineEnd, L'\n', LinesPos))
			LineEnd = strLines.GetLength();

		if (!strExtras.Pos(ExtraEnd, L'\n', ExtrasPos))
			ExtraEnd = strExtras.GetLength();

		HistoryList.push_front(HistoryRecord());
		auto& AddRecord = HistoryList.front();
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
			TimePos += sizeof(FILETIME);
		}

		if ((int)StrPos == Position)
			CurrentItem = HistoryList.begin();
	}

	LoadedStat = cfg_reader.LoadedSectionStat();

	return true;
}

void History::SyncChanges()
{
	const struct stat &CurrentStat = ConfigReader::SavedSectionStat(strRegKey);
	if (LoadedStat.st_ino != CurrentStat.st_ino
			|| LoadedStat.st_size != CurrentStat.st_size
			|| LoadedStat.st_mtime != CurrentStat.st_mtime) {
		fprintf(stderr, "History::SyncChanges: %s\n", strRegKey.c_str());
		HistoryList.clear();
		CurrentItem = HistoryList.end();
		ReadHistory();
	}
}

const wchar_t *History::GetTitle(int Type)
{
	switch (Type)
	{
		case 0: // вьювер
			return Msg::HistoryView;
		case 1: // обычное открытие в редакторе
		case 4: // открытие с локом
			return Msg::HistoryEdit;
		case 2: // external - без ожидания
		case 3: // external - AlwaysWaitFinish
			return Msg::HistoryExt;
	}

	return L"";
}

int History::Select(const wchar_t *Title, const wchar_t *HelpTopic, FARString &strStr, int &Type)
{
	int Height=ScrY-8;
	VMenu HistoryMenu(Title,nullptr,0,Height);
	HistoryMenu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
	switch(TypeHistory)
	{
		case HISTORYTYPE_CMD:     HistoryMenu.SetId(HistoryCmdId); break;
		case HISTORYTYPE_FOLDER:  HistoryMenu.SetId(HistoryFolderId); break;
		case HISTORYTYPE_VIEW:    HistoryMenu.SetId(HistoryEditViewId); break;
		default:                  break;
	}

	if (HelpTopic)
		HistoryMenu.SetHelp(HelpTopic);

	HistoryMenu.SetPosition(-1,-1,0,0);
	if (Opt.AutoHighlightHistory)
		HistoryMenu.AssignHighlights(TRUE);
	return ProcessMenu(strStr, Title, HistoryMenu, Height, Type, nullptr);
}

int History::Select(VMenu &HistoryMenu, int Height, Dialog *Dlg, FARString &strStr)
{
	int Type=0;
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
int History::ProcessMenu(FARString &strStr, const wchar_t *Title, VMenu &HistoryMenu, int Height, int &Type, Dialog *Dlg)
{
	MenuItemEx MenuItem;
	auto SelectedRecord = HistoryList.end();
	FarListPos Pos={0,0};
	int Code=-1;
	int RetCode=1;
	bool Done=false;
	bool SetUpMenuPos=false;

	SyncChanges();
	if (TypeHistory == HISTORYTYPE_DIALOG && HistoryList.empty())
		return 0;

	std::vector<Iter> IterVector(HistoryList.size());
	while (!Done)
	{
		int IterIndex=0;
		bool IsUpdate=false;
		HistoryMenu.DeleteItems();
		HistoryMenu.Modal::ClearDone();
		HistoryMenu.SetBottomTitle(Msg::HistoryFooter);

		// заполнение пунктов меню
		for (auto HistoryItem=TypeHistory==HISTORYTYPE_DIALOG ? --HistoryList.end():HistoryList.begin();
			HistoryItem != HistoryList.end();
			TypeHistory==HISTORYTYPE_DIALOG ? --HistoryItem : ++HistoryItem)
		{
			FARString strRecord;

			if (TypeHistory == HISTORYTYPE_VIEW)
			{
				strRecord += GetTitle(HistoryItem->Type);
				strRecord += L":";
				strRecord += (HistoryItem->Type==4?L"-":L" ");
			}

			/*
				TODO: возможно здесь! или выше....
				char Date[16],Time[16], OutStr[32];
				ConvertDate(HistoryItem->Timestamp,Date,Time,5,TRUE,FALSE,TRUE,TRUE);
				а дальше
				strRecord += дату и время
			*/
			strRecord += HistoryItem->strName;

			if (TypeHistory != HISTORYTYPE_DIALOG)
				ReplaceStrings(strRecord, L"&",L"&&", -1);

			MenuItem.Clear();
			MenuItem.strName = strRecord;
			MenuItem.SetCheck(HistoryItem->Lock?1:0);

			if (!SetUpMenuPos)
				MenuItem.SetSelect(CurrentItem==HistoryItem || (CurrentItem==HistoryList.end() && HistoryItem==--HistoryList.end()));

			//NB: here is really should be used sizeof(HistoryItem), not sizeof(*HistoryItem)
			//cuz sizeof(void *) has special meaning in SetUserData!
			IterVector[IterIndex] = HistoryItem;
			HistoryMenu.SetUserData(IterVector.data()+IterIndex,sizeof(void*),HistoryMenu.AddItem(&MenuItem));
			IterIndex++;
		}

		if (TypeHistory == HISTORYTYPE_DIALOG)
			Dlg->SetComboBoxPos();
		else
			HistoryMenu.SetPosition(-1,-1,0,0);

		if (SetUpMenuPos)
		{
			Pos.SelectPos=Pos.SelectPos < (int)HistoryList.size() ? Pos.SelectPos : (int)HistoryList.size()-1;
			Pos.TopPos=Min(Pos.TopPos,HistoryMenu.GetItemCount()-Height);
			HistoryMenu.SetSelectPos(&Pos);
			SetUpMenuPos=false;
		}

		/*BUGBUG???
			if (TypeHistory == HISTORYTYPE_DIALOG)
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

		while (!HistoryMenu.Done())
		{
			if (TypeHistory == HISTORYTYPE_DIALOG && (!Dlg->GetDropDownOpened() || HistoryList.empty()))
			{
				HistoryMenu.ProcessKey(KEY_ESC);
				continue;
			}

			FarKey Key=HistoryMenu.ReadInput();

			if (TypeHistory == HISTORYTYPE_DIALOG && Key==KEY_TAB) // Tab в списке хистори диалогов - аналог Enter
			{
				HistoryMenu.ProcessKey(KEY_ENTER);
				continue;
			}

			HistoryMenu.GetSelectPos(&Pos);
			Iter *userdata = (Iter*) HistoryMenu.GetUserData(nullptr,sizeof(void*),Pos.SelectPos);
			auto CurrentRecord = userdata ? *userdata : HistoryList.end();

			switch (Key)
			{
				case KEY_CTRLR: // обновить с удалением недоступных
				{
					if (TypeHistory == HISTORYTYPE_FOLDER || TypeHistory == HISTORYTYPE_VIEW)
					{
						bool ModifiedHistory=false;

						for (auto HistoryItem=HistoryList.begin(); HistoryItem!=HistoryList.end(); HistoryItem++)
						{
							if (HistoryItem->Lock) // залоченные не трогаем
								continue;

							// убить запись из истории
							if (apiGetFileAttributes(HistoryItem->strName) == INVALID_FILE_ATTRIBUTES)
							{
								HistoryItem = --HistoryList.erase(HistoryItem);
								ModifiedHistory=true;
							}
						}

						if (ModifiedHistory) // избавляемся от лишних телодвижений
						{
							SaveHistory(); // сохранить
							HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
							HistoryMenu.SetUpdateRequired(TRUE);
							IsUpdate=true;
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
				case KEY_CTRLALTNUMENTER:
				{
					if (TypeHistory == HISTORYTYPE_DIALOG)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done=true;
					RetCode = Key==KEY_CTRLALTENTER||Key==KEY_CTRLALTNUMENTER?7:(Key==KEY_CTRLSHIFTENTER||Key==KEY_CTRLSHIFTNUMENTER?6:(Key==KEY_SHIFTENTER||Key==KEY_SHIFTNUMENTER?2:3));
					break;
				}
				case KEY_F3:
				case KEY_F4:
				case KEY_NUMPAD5:  case KEY_SHIFTNUMPAD5:
				{
					if (TypeHistory == HISTORYTYPE_DIALOG || TypeHistory == HISTORYTYPE_CMD || TypeHistory == HISTORYTYPE_FOLDER)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done=true;
					RetCode=(Key==KEY_F4? 5 : 4);
					break;
				}
				// $ 09.04.2001 SVS - Фича - копирование из истории строки в Clipboard
				case KEY_CTRLC:
				case KEY_CTRLINS:  case KEY_CTRLNUMPAD0:
				{
					if (CurrentRecord != HistoryList.end())
						CopyToClipboard(CurrentRecord->strName);

					break;
				}
				// Lock/Unlock
				case KEY_INS:
				case KEY_NUMPAD0:
				{
					if (HistoryMenu.GetItemCount()/* > 1*/)
					{
						CurrentItem=CurrentRecord;
						CurrentItem->Lock=CurrentItem->Lock?false:true;
						HistoryMenu.Hide();
						ResetPosition();
						SaveHistory();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate=true;
						SetUpMenuPos=true;
					}

					break;
				}
				case KEY_SHIFTNUMDEL:
				case KEY_SHIFTDEL:
				{
					if (HistoryMenu.GetItemCount()/* > 1*/)
					{
						if (!CurrentRecord->Lock)
						{
							HistoryMenu.Hide();
							HistoryList.erase(CurrentRecord);
							ResetPosition();
							SaveHistory();
							HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
							HistoryMenu.SetUpdateRequired(TRUE);
							IsUpdate=true;
							SetUpMenuPos=true;
						}
					}

					break;
				}
				case KEY_NUMDEL:
				case KEY_DEL:
				{
					if (HistoryMenu.GetItemCount()/* > 1*/ &&
					        (!Opt.Confirm.HistoryClear ||
					         (Opt.Confirm.HistoryClear &&
					          !Message(MSG_WARNING,2,
					                      ((TypeHistory==HISTORYTYPE_CMD || TypeHistory==HISTORYTYPE_DIALOG ? Msg::HistoryTitle
					                       : (TypeHistory==HISTORYTYPE_FOLDER ? Msg::FolderHistoryTitle : Msg::ViewHistoryTitle))),
					                  Msg::HistoryClear,
					                  Msg::Clear,Msg::Cancel))))
					{
						for (auto HistoryItem=HistoryList.begin(); HistoryItem!=HistoryList.end(); )
						{
							if (HistoryItem->Lock) // залоченные не трогаем
								HistoryItem++;
							else
								HistoryItem = HistoryList.erase(HistoryItem);
						}

						ResetPosition();
						HistoryMenu.Hide();
						SaveHistory();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate=true;
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

		Done=true;
		Code=HistoryMenu.Modal::GetExitCode();

		if (Code >= 0)
		{
			SelectedRecord = *(Iter *)HistoryMenu.GetUserData(nullptr,sizeof(Iter *),Code);

			if (SelectedRecord == HistoryList.end())
				return -1;

			//BUGUBUG: eliminate those magic numbers!
			if (SelectedRecord->Type != 2 && SelectedRecord->Type != 3 // ignore external
				&& RetCode != 3 && ((TypeHistory == HISTORYTYPE_FOLDER && !SelectedRecord->Type) || TypeHistory == HISTORYTYPE_VIEW) && apiGetFileAttributes(SelectedRecord->strName) == INVALID_FILE_ATTRIBUTES)
			{
				WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);

				if (SelectedRecord->Type == 1 && TypeHistory == HISTORYTYPE_VIEW) // Edit? тогда спросим и если надо создадим
				{
					if (!Message(MSG_WARNING|MSG_ERRORTYPE,2,Title,SelectedRecord->strName,Msg::ViewHistoryIsCreate,Msg::HYes,Msg::HNo))
						break;
				}
				else
				{
					Message(MSG_WARNING|MSG_ERRORTYPE,1,Title,SelectedRecord->strName,Msg::Ok);
				}

				Done=false;
				SetUpMenuPos=true;
				HistoryMenu.Modal::SetExitCode(Pos.SelectPos=Code);
				continue;
			}
		}
	}

	if (Code < 0 || SelectedRecord==HistoryList.end())
		return 0;

	if (KeepSelectedPos)
	{
		CurrentItem = SelectedRecord;
	}

	strStr = SelectedRecord->strName;

	if (RetCode < 4 || RetCode == 6 || RetCode == 7)
	{
		Type=SelectedRecord->Type;
	}
	else
	{
		Type=RetCode-4;

		if (Type == 1 && SelectedRecord->Type == 4)
			Type=4;

		RetCode=1;
	}

	return RetCode;
}

void History::GetPrev(FARString &strStr)
{
	CurrentItem--;

	if (CurrentItem == HistoryList.end()) {
		SyncChanges();
		CurrentItem=HistoryList.begin();
	}

	if (CurrentItem != HistoryList.end())
		strStr = CurrentItem->strName;
	else
		strStr.Clear();
}


void History::GetNext(FARString &strStr)
{
	if (CurrentItem != HistoryList.end())
		CurrentItem++;
	else
		SyncChanges();

	if (CurrentItem != HistoryList.end())
		strStr = CurrentItem->strName;
	else
		strStr.Clear();
}

bool History::DeleteMatching(FARString &strStr)
{
	SyncChanges();

	auto HistoryItem = CurrentItem;
	for (HistoryItem--; HistoryItem != CurrentItem; HistoryItem--)
	{
		if (HistoryItem==HistoryList.end() || HistoryItem->Lock)
			continue;

		if (HistoryItem->strName == strStr)
		{
			HistoryList.erase(HistoryItem);
			SaveHistory();
			return true;
		}
	}

	return false;
}

bool History::GetSimilar(FARString &strStr, int LastCmdPartLength, bool bAppend)
{
	SyncChanges();
	int Length=(int)strStr.GetLength();

	if (LastCmdPartLength!=-1 && LastCmdPartLength<Length)
		Length=LastCmdPartLength;

	if (LastCmdPartLength==-1)
	{
		ResetPosition();
	}

	auto HistoryItem = CurrentItem;
	for (HistoryItem--; HistoryItem != CurrentItem; HistoryItem--)
	{
		if (HistoryItem == HistoryList.end())
			continue;

		if (!StrCmpNI(strStr,HistoryItem->strName,Length) && StrCmp(strStr,HistoryItem->strName))
		{
			if (bAppend)
				strStr += &HistoryItem->strName[Length];
			else
				strStr = HistoryItem->strName;

			CurrentItem = HistoryItem;
			return true;
		}
	}

	return false;
}

bool History::GetAllSimilar(VMenu &HistoryMenu,const wchar_t *Str)
{
	SyncChanges();
	int Length=StrLength(Str);
	for (auto HistoryItem=--HistoryList.end(); HistoryItem!=HistoryList.end(); HistoryItem--)
	{
		if (!StrCmpNI(Str,HistoryItem->strName,Length) && StrCmp(Str,HistoryItem->strName) && IsAllowedForHistory(HistoryItem->strName.CPtr()))
		{
			HistoryMenu.AddItem(HistoryItem->strName);
		}
	}
	return false;
}

void History::SetAddMode(bool EnableAdd, int RemoveDups, bool KeepSelectedPos)
{
	History::EnableAdd=EnableAdd;
	History::RemoveDups=RemoveDups;
	History::KeepSelectedPos=KeepSelectedPos;
}

bool History::EqualType(int Type1, int Type2)
{
	return (Type1 == Type2) ||
		(TypeHistory == HISTORYTYPE_VIEW && ((Type1==4 && Type2==1) || (Type1==1 && Type2==4)));
}

