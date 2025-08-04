/*
usermenu.cpp

User menu и есть
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

#include "lang.hpp"
#include "keys.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "cmdline.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "fileedit.hpp"
#include "ctrlobj.hpp"
#include "manager.hpp"
#include "constitle.hpp"
#include "message.hpp"
#include "usermenu.hpp"
#include "filetype.hpp"
#include "fnparce.hpp"
#include "execute.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "filestr.hpp"
#include "mix.hpp"
#include "savescr.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "cache.hpp"
#include "DlgGuid.hpp"

static std::unique_ptr<ConfigReader> s_cfg_reader;

// Коды выхода из меню (Exit codes)
enum
{
	EC_CLOSE_LEVEL      = -1,    // Выйти из меню на один уровень вверх
	EC_CLOSE_MENU       = -2,    // Выйти из меню по SHIFT+F10
	EC_PARENT_MENU      = -3,    // Показать меню родительского каталога
	EC_MAIN_MENU        = -4,    // Показать главное меню
	EC_COMMAND_SELECTED = -5,    // Выбрана команда - закрыть меню и обновить папку
};

static bool SetToParentPath(FARString &Path)
{
	return CutToSlash(Path, true) && !Path.IsEmpty();
}

static int PrepareHotKey(FARString &strHotKey)
{
	int FuncNum = 0;

	if (strHotKey.GetLength() > 1) {
		// если хоткей больше 1 символа, считаем это случаем "F?", причем при кривизне всегда будет "F1"
		FuncNum = _wtoi(strHotKey.CPtr() + 1);

		if (FuncNum < 1 || FuncNum > 24) {
			FuncNum = 1;
			strHotKey = L"F1";
		}
	} else {
		// при наличии "&" продублируем
		if (strHotKey.At(0) == L'&')
			strHotKey+= L"&";
	}

	return FuncNum;
}

static void
MenuRegToFile(const wchar_t *MenuKey, File &MenuFile, CachedWrite &CW, bool SingleItemMenu = false)
{
	for (int i = 0;; i++) {
		const std::string &strItemKey = StrPrintf("%ls/Item%d", MenuKey, i);
		s_cfg_reader->SelectSection(strItemKey);
		FARString strLabel;
		if (!s_cfg_reader->GetString(strLabel, "Label", L"")) {
			break;
		}

		FARString strHotKey = s_cfg_reader->GetString("HotKey", L"");
		bool SubMenu = s_cfg_reader->GetInt("Submenu", 0) != 0;
		CW.Write(strHotKey.CPtr(), static_cast<DWORD>(strHotKey.GetLength() * sizeof(WCHAR)));
		CW.Write(L":  ", 3 * sizeof(WCHAR));
		CW.Write(strLabel.CPtr(), static_cast<DWORD>(strLabel.GetLength() * sizeof(WCHAR)));
		CW.Write(L"\r\n", 2 * sizeof(WCHAR));

		if (SubMenu) {
			CW.Write(L"{\r\n", 3 * sizeof(WCHAR));
			MenuRegToFile(FARString(strItemKey), MenuFile, CW, false);
			CW.Write(L"}\r\n", 3 * sizeof(WCHAR));
		} else {
			for (int i = 0;; i++) {
				FARString strCommand;
				if (!s_cfg_reader->GetString(strCommand, StrPrintf("Command%d", i), L"")) {
					break;
				}

				CW.Write(L"    ", 4 * sizeof(WCHAR));
				CW.Write(strCommand.CPtr(), static_cast<DWORD>(strCommand.GetLength() * sizeof(WCHAR)));
				CW.Write(L"\r\n", 2 * sizeof(WCHAR));
			}
		}
	}
}

void MenuFileToReg(const wchar_t *MenuKey, File &MenuFile, GetFileString &GetStr, bool SingleItemMenu = false,
		UINT MenuCP = CP_WIDE_LE)
{
	INT64 Pos = 0;
	MenuFile.GetPointer(Pos);
	if (!Pos) {
		if (!GetFileFormat(MenuFile, MenuCP))
			MenuCP = CP_UTF8;
	}

	LPWSTR MenuStr = nullptr;
	int MenuStrLength = 0;
	int KeyNumber = -1, CommandNumber = 0;

	while (GetStr.GetString(&MenuStr, MenuCP, MenuStrLength)) {
		FARString strItemKey;

		if (!SingleItemMenu)
			strItemKey.Format(L"%ls/Item%d", MenuKey, KeyNumber);
		else
			strItemKey = MenuKey;

		RemoveTrailingSpaces(MenuStr);

		if (!*MenuStr)
			continue;

		if (*MenuStr == L'{' && KeyNumber >= 0) {
			MenuFileToReg(strItemKey, MenuFile, GetStr, false, MenuCP);
			continue;
		}

		if (*MenuStr == L'}')
			break;

		if (!IsSpace(*MenuStr)) {
			wchar_t *ChPtr = nullptr;

			if (!(ChPtr = wcschr(MenuStr, L':')))
				continue;

			if (!SingleItemMenu) {
				strItemKey.Format(L"%ls/Item%d", MenuKey, ++KeyNumber);
			} else {
				strItemKey = MenuKey;
				++KeyNumber;
			}

			*ChPtr = 0;
			FARString strHotKey = MenuStr;
			FARString strLabel = ChPtr + 1;
			RemoveLeadingSpaces(strLabel);
			bool SubMenu = (GetStr.PeekString(&MenuStr, MenuCP, MenuStrLength) && *MenuStr == L'{');

			// Support for old 1.x separator format
			if (MenuCP == CP_OEMCP && strHotKey == L"-" && strLabel.IsEmpty()) {
				strHotKey+= L"-";
			}

			ConfigWriter cfg_writer(strItemKey.GetMB());
			cfg_writer.SetString("HotKey", strHotKey);
			cfg_writer.SetString("Label", strLabel);
			cfg_writer.SetInt("Submenu", SubMenu);
			// CloseSameRegKey();
			CommandNumber = 0;
		} else {
			if (KeyNumber >= 0) {
				RemoveLeadingSpaces(MenuStr);
				const std::string &strLineName = StrPrintf("Command%d", CommandNumber);
				++CommandNumber;
				ConfigWriter(strItemKey.GetMB()).SetString(strLineName, MenuStr);
			}
		}

		ConfigReaderScope::Update(s_cfg_reader);

		SingleItemMenu = false;
	}
}

// ChooseMenuType: true - выбор типа меню (основное или локальное),
//                 false - зависит от наличия FarMenu.Ini в текущем каталоге
UserMenu::UserMenu(bool ChooseMenuType, bool FromAnyFile, const wchar_t *FileName)
	:
	grs(s_cfg_reader)
{
	MenuFromAnyFile = FromAnyFile && *FileName;
	ProcessUserMenu(ChooseMenuType, FileName);
}

UserMenu::~UserMenu() {}

void UserMenu::ProcessUserMenu(bool ChooseMenuType, const wchar_t *MenuFileName)
{
	const wchar_t *const LocalMenuFileName = L"FarMenu.ini";
	MenuMode = MM_LOCAL;
	MenuModified = MenuNeedRefresh = false;

	// Путь к текущему каталогу с файлом LocalMenuFileName
	FARString strMenuFilePath = CtrlObject->CmdLine->GetCurDir();

	if (ChooseMenuType) {
		switch (Message(0, 3, Msg::UserMenuTitle, Msg::ChooseMenuType, Msg::ChooseMenuMain,
				Msg::ChooseMenuLocal, Msg::Cancel)) {
			case 0:
				MenuMode = MM_FAR;
				strMenuFilePath = g_strFarPath;
				break;
			case 1:
				break;
			default:
				return;
		}
	}

	FARString strLocalMenuKey;
	strLocalMenuKey.Format(L"UserMenu/LocalMenu%lu", (unsigned long)GetProcessUptimeMSec());
	{
		ConfigWriter(strLocalMenuKey.GetMB()).RemoveSection();
	}
	ConfigReaderScope::Update(s_cfg_reader);

	// основной цикл обработки
	bool FirstRun = true;
	int ExitCode = 0;

	while ((ExitCode != EC_CLOSE_LEVEL) && (ExitCode != EC_CLOSE_MENU) && (ExitCode != EC_COMMAND_SELECTED)) {
		FARString strMenuFileFullPath;
		if (MenuMode == MM_LOCAL && MenuFromAnyFile) {
			strMenuFileFullPath = MenuFileName;
		} else {
			strMenuFileFullPath = strMenuFilePath;
			AddEndSlash(strMenuFileFullPath);
			strMenuFileFullPath+= LocalMenuFileName;
		}

		if (MenuMode == MM_LOCAL || MenuMode == MM_FAR) {
			// Пытаемся открыть файл
			File MenuFile;
			bool FileOpened = PathCanHoldRegularFile(strMenuFilePath)
					&& MenuFile.Open(strMenuFileFullPath, GENERIC_READ, FILE_SHARE_READ, nullptr,
							OPEN_EXISTING);
			if (FileOpened) {
				// сливаем содержимое в реестр "на запасной путь" и оттуда будем пользовать
				GetFileString GetStr(MenuFile);
				MenuFileToReg(strLocalMenuKey, MenuFile, GetStr);
				MenuFile.Close();
			} else {
				// Файл не открылся. Смотрим дальше.
				if (MenuMode == MM_FAR) {
					MenuMode = MM_USER;
				} else    // MM_LOCAL
				{
					if (!ChooseMenuType && !MenuFromAnyFile) {
						if (!FirstRun && SetToParentPath(strMenuFilePath))
							continue;    // подымаемся выше...

						FirstRun = false;
						MenuMode = MM_FAR;
						strMenuFilePath = g_strFarPath;
						continue;
					}
				}
			}
		}

		FARMACROAREA PrevMacroArea = CtrlObject->Macro.GetArea();
		int _CurrentFrame = FrameManager->GetCurrentFrame()->GetType();
		CtrlObject->Macro.SetArea(MACROAREA_USERMENU);

		// вызываем меню
		FARString strRootMenuKey = (MenuMode == MM_USER) ? L"UserMenu/MainMenu" : strLocalMenuKey;
		ExitCode = ProcessSingleMenu(strRootMenuKey, 0, strRootMenuKey);

		if (_CurrentFrame == FrameManager->GetCurrentFrame()->GetType())    //???
			CtrlObject->Macro.SetArea(PrevMacroArea);

		if (MenuMode == MM_LOCAL || MenuMode == MM_FAR) {
			// ...запишем изменения обратно в файл
			if (MenuModified) {
				DWORD FileAttr = apiGetFileAttributes(strMenuFileFullPath);

				if (FileAttr != INVALID_FILE_ATTRIBUTES) {
					if (FileAttr & FILE_ATTRIBUTE_READONLY) {
						int AskOverwrite = Message(MSG_WARNING, 2, Msg::UserMenuTitle, LocalMenuFileName,
								Msg::EditRO, Msg::EditOvr, Msg::Yes, Msg::No);

						if (!AskOverwrite)
							apiSetFileAttributes(strMenuFileFullPath, FileAttr & ~FILE_ATTRIBUTE_READONLY);
					}

					if (FileAttr & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
						apiSetFileAttributes(strMenuFileFullPath, FILE_ATTRIBUTE_NORMAL);
				}

				File MenuFile;
				// Don't use CreationDisposition=CREATE_ALWAYS here - it kills alternate streams
				if (MenuFile.Open(strMenuFileFullPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
							FileAttr == INVALID_FILE_ATTRIBUTES ? CREATE_NEW : TRUNCATE_EXISTING)) {
					CachedWrite CW(MenuFile);
					WCHAR Data = SIGN_WIDE_LE;
					CW.Write(&Data, sizeof(WCHAR));
					MenuRegToFile(strLocalMenuKey, MenuFile, CW);
					CW.Flush();
					UINT64 Size = 0;
					MenuFile.GetSize(Size);
					MenuFile.Close();

					// если файл FarMenu.ini пуст, то удалим его
					if (Size <= 4)    // 4 for BOM
					{
						apiDeleteFile(strMenuFileFullPath);
					} else if (FileAttr != INVALID_FILE_ATTRIBUTES) {
						apiSetFileAttributes(strMenuFileFullPath, FileAttr);
					}
				}
			}

			// ...почистим реестр.
			{
				ConfigWriter(strLocalMenuKey.GetMB()).RemoveSection();
			}
			ConfigReaderScope::Update(s_cfg_reader);
		}

		// что было после вызова меню?
		switch (ExitCode) {
			// Показать меню родительского каталога
			case EC_PARENT_MENU: {
				if (MenuMode == MM_LOCAL) {
					if (SetToParentPath(strMenuFilePath))
						continue;

					MenuMode = MM_FAR;
					strMenuFilePath = g_strFarPath;
				} else {
					MenuMode = MM_USER;
				}

				break;
			}
			// Показать главное меню
			case EC_MAIN_MENU: {
				// Shift+F2 переключает тип меню в цикле
				switch (MenuMode) {
					case MM_LOCAL:
						MenuMode = MM_FAR;
						strMenuFilePath = g_strFarPath;
						break;

					case MM_FAR:
						MenuMode = MM_USER;
						break;

					default:    // MM_USER
						MenuMode = MM_LOCAL;
						strMenuFilePath = CtrlObject->CmdLine->GetCurDir();
						break;
				}

				break;
			}
		}
	}

	if (FrameManager->IsPanelsActive() && (ExitCode == EC_COMMAND_SELECTED || MenuModified))
		ShellUpdatePanels(CtrlObject->Cp()->ActivePanel, FALSE);
}

// заполнение меню
static int
FillUserMenu(VMenu &UserMenu, const wchar_t *MenuKey, int MenuPos, int *FuncPos, const wchar_t *Name)
{
	UserMenu.DeleteItems();
	int NumLines = 0;

	for (NumLines = 0;; NumLines++) {
		s_cfg_reader->SelectSectionFmt("%ls/Item%d", MenuKey, NumLines);
		if (!s_cfg_reader->HasSection())
			break;

		MenuItemEx UserMenuItem;
		UserMenuItem.Clear();
		FARString strHotKey = s_cfg_reader->GetString("HotKey", L"");
		FARString strLabel = s_cfg_reader->GetString("Label", L"");
		int FuncNum = 0;

		// сепаратором является случай, когда хоткей == "--"
		if (!StrCmp(strHotKey, L"--")) {
			UserMenuItem.Flags|= LIF_SEPARATOR;
			UserMenuItem.Flags&= ~LIF_SELECTED;
			UserMenuItem.strName = strLabel;

			if (NumLines == MenuPos) {
				MenuPos++;
			}
		} else {
			SubstFileName(strLabel, Name, nullptr, nullptr, TRUE);
			apiExpandEnvironmentStrings(strLabel, strLabel);
			FuncNum = PrepareHotKey(strHotKey);
			int Offset = strHotKey.At(0) == L'&' ? 5 : 4;
			FormatString FString;
			FString << ((!strHotKey.IsEmpty() && !FuncNum) ? L"&" : L"") << fmt::LeftAlign()
					<< fmt::Size(Offset) << strHotKey;
			UserMenuItem.strName = std::move(FString.strValue());
			UserMenuItem.strName+= strLabel;

			if (s_cfg_reader->GetInt("Submenu", 0) != 0) {
				UserMenuItem.Flags|= MIF_SUBMENU;
			}

			UserMenuItem.SetSelect(NumLines == MenuPos);
			UserMenuItem.Flags &= ~LIF_SEPARATOR;
		}

		int ItemPos = UserMenu.AddItem(&UserMenuItem);

		if (FuncNum > 0) {
			FuncPos[FuncNum - 1] = ItemPos;
		}
	}

#if 0
	// Extra empty item
	MenuItemEx UserMenuItem;
	UserMenuItem.Clear();
	UserMenuItem.SetSelect(NumLines == MenuPos);
	UserMenu.AddItem(&UserMenuItem);
#endif
	return NumLines;
}

// обработка единичного меню
int UserMenu::ProcessSingleMenu(const wchar_t *MenuKey, int MenuPos, const wchar_t *MenuRootKey,
		const wchar_t *Title)
{
	MenuItemEx UserMenuItem;

	for (;;) {
		UserMenuItem.Clear();
		int NumLine = 0, ExitCode, FuncPos[24];

		// очистка F-хоткеев
		for (size_t I = 0; I < ARRAYSIZE(FuncPos); I++)
			FuncPos[I] = -1;

		FARString strName;
		CtrlObject->Cp()->ActivePanel->GetCurName(strName);
		/* $ 24.07.2000 VVM + При показе главного меню в заголовок добавляет тип - FAR/Registry */
		FARString strMenuTitle;

		if (Title && *Title)
			strMenuTitle = Title;
		else {
			switch (MenuMode) {
				case MM_LOCAL:
					strMenuTitle = Msg::LocalMenuTitle;
					if (MenuFromAnyFile)
						strMenuTitle+= L" *";
					break;

				case MM_FAR:
					strMenuTitle = Msg::MainMenuTitle;
					strMenuTitle+= L" (" + Msg::MainMenuFAR + L")";
					break;

				default:    // MM_USER
					strMenuTitle = Msg::MainMenuTitle;
					strMenuTitle+= L" (" + Msg::MainMenuUser + L")";
					break;
			}
		}

		{
			VMenu UserMenu(strMenuTitle, nullptr, 0, ScrY - 4);
			UserMenu.SetFlags(VMENU_WRAPMODE);
			UserMenu.SetHelp(L"UserMenu");
			UserMenu.SetPosition(-1, -1, 0, 0);
			UserMenu.SetBottomTitle(Msg::MainMenuBottomTitle);
			MenuNeedRefresh = true;

			while (!UserMenu.Done()) {
				if (MenuNeedRefresh) {
					UserMenu.Hide();    // спрячем
					// "изнасилуем" (перезаполним :-)
					NumLine = FillUserMenu(UserMenu, MenuKey, MenuPos, FuncPos, strName);
					// заставим манагер менюхи корректно отрисовать ширину и
					// высоту, а заодно и скорректировать вертикальные позиции
					UserMenu.SetPosition(-1, -1, -1, -1);
					UserMenu.Show();
					MenuNeedRefresh = false;
				}

				FarKey Key = UserMenu.ReadInput();
				MenuPos = UserMenu.GetSelectPos();

				if (Key >= KEY_F1 && Key <= KEY_F24) {
					int FuncItemPos;

					if ((FuncItemPos = FuncPos[Key - KEY_F1]) != -1) {
						UserMenu.Modal::SetExitCode(FuncItemPos);
						continue;
					}
				} else if (Key == L' ')    // исключаем пробел из "хоткеев"!
					continue;

				switch (Key) {
						/* $ 24.08.2001 VVM + Стрелки вправо/влево открывают/закрывают подменю соответственно */
					case KEY_RIGHT:
					case KEY_NUMPAD6:
					case KEY_MSWHEEL_RIGHT:
						s_cfg_reader->SelectSectionFmt("%ls/Item%d", MenuKey, MenuPos);
						if (s_cfg_reader->GetInt("Submenu", 0) != 0)
							UserMenu.SetExitCode(MenuPos);

						break;

					case KEY_LEFT:
					case KEY_NUMPAD4:
					case KEY_MSWHEEL_LEFT:
						if (Title && *Title)
							UserMenu.SetExitCode(-1);

						break;

					case KEY_NUMDEL:
					case KEY_DEL:
						if (NumLine && MenuPos > -1 && MenuPos < NumLine)
							DeleteMenuRecord(MenuKey, MenuPos);

						break;
					case KEY_INS:
					case KEY_F4:
					case KEY_SHIFTF4:
					case KEY_NUMPAD0: {
						bool bInsertNew = (Key == KEY_INS || Key == KEY_NUMPAD0);

						if (!bInsertNew && (MenuPos >= NumLine || MenuPos < 0))
							break;
						if (bInsertNew && MenuPos < 0)
							MenuPos = 0;

						EditMenu(MenuKey, MenuPos, NumLine, bInsertNew);
					}
						break;
					case KEY_CTRLUP:
					case KEY_CTRLDOWN: {
						int Pos = UserMenu.GetSelectPos();

						if (Pos != UserMenu.GetItemCount()) {
							if (!(Key == KEY_CTRLUP && !Pos)
									&& !(Key == KEY_CTRLDOWN && Pos == UserMenu.GetItemCount() - 1)) {
								MenuPos = Pos + (Key == KEY_CTRLUP ? -1 : +1);
								MoveMenuItem(MenuKey, Pos, MenuPos);
							}
						}
						break;
					}

					// case KEY_ALTSHIFTF4:  // редактировать только текущий пункт (если субменю - то все субменю)
					case KEY_CTRLF4:    // редактировать все меню
					{
						(*FrameManager)[0]->Unlock();
						FARString strMenuFileName;
						File MenuFile;
						if (!FarMkTempEx(strMenuFileName)
								|| (!MenuFile.Open(strMenuFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
										CREATE_ALWAYS))) {
							break;
						}

						FARString strCurrentKey;

						if (Key == KEY_ALTSHIFTF4)
							strCurrentKey.Format(L"%ls/Item%d", MenuKey, MenuPos);
						else
							strCurrentKey = MenuRootKey;
						CachedWrite CW(MenuFile);
						WCHAR Data = SIGN_WIDE_LE;
						CW.Write(&Data, sizeof(WCHAR));
						MenuRegToFile(strCurrentKey, MenuFile, CW, Key == KEY_ALTSHIFTF4);
						CW.Flush();
						MenuNeedRefresh = true;
						MenuFile.Close();
						{
							ConsoleTitle *OldTitle = new ConsoleTitle;
							FARString strFileName = strMenuFileName;
							FileEditor ShellEditor(strFileName, CP_WIDE_LE, FFILEEDIT_DISABLEHISTORY, -1, -1,
									nullptr);
							delete OldTitle;
							ShellEditor.SetDynamicallyBorn(false);
							FrameManager->ExecuteModalEV(false);

							if (!ShellEditor.IsFileChanged()
									|| (!MenuFile.Open(strMenuFileName, GENERIC_READ, FILE_SHARE_READ,
											nullptr, OPEN_EXISTING))) {
								apiDeleteFile(strMenuFileName);

								if (Key == KEY_ALTSHIFTF4)    // для текущего пункта меню закрывать не надо
									break;

								return 0;
							}
						}
						{
							ConfigWriter(strCurrentKey.GetMB()).RemoveSection();
						}
						ConfigReaderScope::Update(s_cfg_reader);
						GetFileString GetStr(MenuFile);
						MenuFileToReg(strCurrentKey, MenuFile, GetStr, Key == KEY_ALTSHIFTF4);
						MenuFile.Close();
						apiDeleteFile(strMenuFileName);
						MenuModified = true;
						UserMenu.Hide();

						if (Key == KEY_ALTSHIFTF4)    // для текущего пункта меню закрывать не надо
							break;

						return 0;    // Закрыть меню
					}

					case KEY_SHIFTF10:
						return (EC_CLOSE_MENU);

					case KEY_SHIFTF2:    // Показать главное меню
						return (EC_MAIN_MENU);

					case KEY_BS:    // Показать меню из родительского каталога только в MM_LOCAL режиме
						if (MenuMode == MM_LOCAL && !MenuFromAnyFile)
							return (EC_PARENT_MENU);

						break;

					default:
						UserMenu.ProcessInput();
						if (Key == KEY_F1)
							MenuNeedRefresh = true;

						break;
				}    // switch(Key)
			}        // while (!UserMenu.Done())

			ExitCode = UserMenu.Modal::GetExitCode();
		}

		if (ExitCode < 0 || ExitCode >= NumLine)
			return (EC_CLOSE_LEVEL);    //  вверх на один уровень

		FARString strCurrentKey;
		strCurrentKey.Format(L"%ls/Item%d", MenuKey, ExitCode);
		s_cfg_reader->SelectSection(strCurrentKey);
		int SubMenu = s_cfg_reader->GetInt("Submenu", 0);

		if (SubMenu) {
			/* $ 20.08.2001 VVM + При вложенных меню показывает заголовки предыдущих */
			FARString strSubMenuKey, strSubMenuTitle, strSubMenuLabel;
			strSubMenuKey.Format(L"%ls/Item%d", MenuKey, ExitCode);
			s_cfg_reader->SelectSection(strSubMenuKey);
			if (s_cfg_reader->GetString(strSubMenuLabel, "Label", L"")) {
				SubstFileName(strSubMenuLabel, strName, nullptr, nullptr, TRUE);
				apiExpandEnvironmentStrings(strSubMenuLabel, strSubMenuLabel);
				size_t pos;

				if (strSubMenuLabel.Pos(pos, L'&'))
					strSubMenuLabel.LShift(1, pos);

				if (Title && *Title) {
					strSubMenuTitle = Title;
					strSubMenuTitle+= L" -> ";
					strSubMenuTitle+= strSubMenuLabel;
				} else
					strSubMenuTitle = strSubMenuLabel;
			}

			/* $ 14.07.2000 VVM ! Если закрыли подменю, то остаться. Инече передать управление выше */
			MenuPos = ProcessSingleMenu(strSubMenuKey, 0, MenuRootKey, strSubMenuTitle);

			if (MenuPos != EC_CLOSE_LEVEL)
				return (MenuPos);

			MenuPos = ExitCode;
			continue;
		}

		int CurLine = 0;
		FARString strCmdLineDir = CtrlObject->CmdLine->GetCurDir();
		FARString strOldCmdLine;
		CtrlObject->CmdLine->GetString(strOldCmdLine);
		int OldCmdLineCurPos = CtrlObject->CmdLine->GetCurPos();
		int OldCmdLineLeftPos = CtrlObject->CmdLine->GetLeftPos();
		int OldCmdLineSelStart, OldCmdLineSelEnd;
		CtrlObject->CmdLine->GetSelection(OldCmdLineSelStart, OldCmdLineSelEnd);
		CtrlObject->CmdLine->LockUpdatePanel(TRUE);

		// Цикл исполнения команд меню (CommandX)
		for (;;) {
			FormatString strLineName;
			FARString strCommand, strListName, strAnotherListName;
			strLineName << L"Command" << CurLine;

			s_cfg_reader->SelectSection(strCurrentKey);
			if (!s_cfg_reader->GetString(strCommand, FARString(strLineName).GetMB(), L""))
				break;

			if ((StrCmpNI(strCommand, L"REM", 3) || !IsSpaceOrEos(strCommand.At(3)))
					&& StrCmpNI(strCommand, L"::", 2)) {
				/*
				  Осталось корректно обработать ситуацию, например:
				  if exist !#!\!^!.! far:edit < diff -c -p !#!\!^!.! !\!.!
				  Т.е. сначала "вычислить" кусок "if exist !#!\!^!.!", ну а если
				  выполнится, то делать дальше.
				  Или еще пример,
				  if exist ..\a.bat D:\FAR\170\DIFF.MY\mkdiff.bat !?&Номер патча?!
				  ЭТО выполняется всегда, т.к. парсинг всей строки идет, а надо
				  проверить фазу "if exist ..\a.bat", а уж потом делать выводы...
				*/
				SubstFileName(strCommand, strName, &strListName, &strAnotherListName, FALSE, strCmdLineDir);
				bool ListFileUsed = !strListName.IsEmpty() || !strAnotherListName.IsEmpty();

				RemoveExternalSpaces(strCommand);

				if (!strCommand.IsEmpty()) {
					bool isSilent = false;

					if (strCommand.At(0) == L'@') {
						strCommand.LShift(1);
						isSilent = true;
					}

					// TODO: Ахтунг. В режиме isSilent имеем проблемы с командами, которые выводят что-то на экран
					//       Здесь необходимо переделка, например, перед исполнением подсунуть временный экранный
					//       буфер, а потом его содержимое подсунуть в ScreenBuf...

					if (!isSilent) {
						CtrlObject->CmdLine->ExecString(strCommand, FALSE, 0, 0, ListFileUsed);
					} else {
						SaveScreen SaveScr;
						CtrlObject->Cp()->LeftPanel->CloseFile();
						CtrlObject->Cp()->RightPanel->CloseFile();
						Execute(strCommand, 0, 0, ListFileUsed, true);
					}
				}
			}    // strCommand != "REM"

			if (!strListName.IsEmpty())
				QueueDeleteOnClose(strListName);

			if (!strAnotherListName.IsEmpty())
				QueueDeleteOnClose(strAnotherListName);

			CurLine++;
		}    // for (;;)

		CtrlObject->CmdLine->LockUpdatePanel(FALSE);

		if (!strOldCmdLine.IsEmpty())    // восстановим сохраненную командную строку
		{
			CtrlObject->CmdLine->SetString(strOldCmdLine, FrameManager->IsPanelsActive());
			CtrlObject->CmdLine->SetCurPos(OldCmdLineCurPos, OldCmdLineLeftPos);
			CtrlObject->CmdLine->Select(OldCmdLineSelStart, OldCmdLineSelEnd);
		}

		return (EC_COMMAND_SELECTED);
	}
}

enum EditMenuItems
{
	EM_DOUBLEBOX,
	EM_HOTKEY_TEXT,
	EM_HOTKEY_EDIT,
	EM_LABEL_TEXT,
	EM_LABEL_EDIT,
	EM_SEPARATOR1,
	EM_COMMANDS_TEXT,
	EM_EDITLINE_0,
	EM_EDITLINE_1,
	EM_EDITLINE_2,
	EM_EDITLINE_3,
	EM_EDITLINE_4,
	EM_EDITLINE_5,
	EM_EDITLINE_6,
	EM_EDITLINE_7,
	EM_EDITLINE_8,
	EM_EDITLINE_9,
	EM_SEPARATOR2,
	EM_BUTTON_OK,
	EM_BUTTON_CANCEL,
};

LONG_PTR WINAPI EditMenuDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_CLOSE:

			if (Param1 == EM_BUTTON_OK) {
				BOOL Result = TRUE;
				LPCWSTR HotKey = reinterpret_cast<LPCWSTR>(
						SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, EM_HOTKEY_EDIT, 0));
				LPCWSTR Label =
						reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, EM_LABEL_EDIT, 0));
				int FocusPos = -1;

				if (StrCmp(HotKey, L"--")) {
					if (!*Label) {
						FocusPos = EM_LABEL_EDIT;
					} else if (StrLength(HotKey) > 1) {
						FocusPos = EM_HOTKEY_EDIT;

						if (Upper(*HotKey) == L'F') {
							int FuncNum = _wtoi(HotKey + 1);

							if (FuncNum > 0 && FuncNum < 25)
								FocusPos = -1;
						}
					}
				}

				if (FocusPos != -1) {
					Message(MSG_WARNING, 1, Msg::UserMenuTitle,
							((*Label ? Msg::UserMenuInvalidInputHotKey : Msg::UserMenuInvalidInputLabel)),
							Msg::Ok);
					SendDlgMessage(hDlg, DM_SETFOCUS, FocusPos, 0);
					Result = FALSE;
				}

				return Result;
			}

			break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool UserMenu::EditMenu(const wchar_t *MenuKey, int EditPos, int TotalRecords, bool Create)
{
	bool Result = false;
	FormatString strItemKey;
	strItemKey << MenuKey << L"/Item" << EditPos;
	MenuNeedRefresh = true;
	bool SubMenu = false, Continue = true;

	s_cfg_reader->SelectSection(strItemKey);

	if (Create) {
		switch (Message(0, 2, &AskInsertMenuOrCommandId, Msg::UserMenuTitle, Msg::AskInsertMenuOrCommand,
				Msg::MenuInsertCommand, Msg::MenuInsertMenu)) {
			case -1:
			case -2:
				Continue = false;
			case 1:
				SubMenu = true;
		}
	} else {
		SubMenu = s_cfg_reader->GetInt("Submenu", 0) != 0;
	}

	if (Continue) {
		const int DLG_X = 76, DLG_Y = SubMenu ? 10 : 22;
		DWORD State = SubMenu ? DIF_HIDDEN | DIF_DISABLE : 0;
		DialogDataEx EditDlgData[] = {
				{DI_DOUBLEBOX, 3, 1,                  DLG_X - 4, (short)(DLG_Y - 2), {}, 0,
                 (SubMenu ? Msg::EditSubmenuTitle : Msg::EditMenuTitle)                                                                          },
				{DI_TEXT,      5, 2,                  0,         2,                  {}, 0,                                 Msg::EditMenuHotKey  },
				{DI_FIXEDIT,   5, 3,                  7,         3,                  {}, DIF_FOCUS,                         L""                  },
				{DI_TEXT,      5, 4,                  0,         4,                  {}, 0,                                 Msg::EditMenuLabel   },
                {DI_EDIT,      5, 5,                  DLG_X - 6, 5,                  {}, 0,                                 L""                  },

				{DI_TEXT,      3, 6,                  0,         6,                  {}, DIF_SEPARATOR | State,             L""                  },
				{DI_TEXT,      5, 7,                  0,         7,                  {}, State,                             Msg::EditMenuCommands},
				{DI_EDIT,      5, 8,                  DLG_X - 6, 8,                  {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 9,                  DLG_X - 6, 9,                  {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 10,                 DLG_X - 6, 10,                 {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 11,                 DLG_X - 6, 11,                 {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 12,                 DLG_X - 6, 12,                 {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 13,                 DLG_X - 6, 13,                 {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 14,                 DLG_X - 6, 14,                 {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 15,                 DLG_X - 6, 15,                 {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 16,                 DLG_X - 6, 16,                 {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },
				{DI_EDIT,      5, 17,                 DLG_X - 6, 17,                 {}, DIF_EDITPATH | DIF_EDITOR | State, L""                  },

				{DI_TEXT,      3, (short)(DLG_Y - 4), 0,         (short)(DLG_Y - 4), {}, DIF_SEPARATOR,                     L""                  },
				{DI_BUTTON,    0, (short)(DLG_Y - 3), 0,         (short)(DLG_Y - 3), {}, DIF_DEFAULT | DIF_CENTERGROUP,
                 Msg::Ok                                                                                                                         },
				{DI_BUTTON,    0, (short)(DLG_Y - 3), 0,         (short)(DLG_Y - 3), {}, DIF_CENTERGROUP,                   Msg::Cancel          }
        };
		MakeDialogItemsEx(EditDlgData, EditDlg);
		enum
		{
			DI_EDIT_COUNT = EM_SEPARATOR2 - EM_COMMANDS_TEXT - 1
		};

		if (!Create) {
			EditDlg[EM_HOTKEY_EDIT].strData = s_cfg_reader->GetString("HotKey", L"");
			EditDlg[EM_LABEL_EDIT].strData = s_cfg_reader->GetString("Label", L"");
			int CommandNumber = 0;

			while (CommandNumber < DI_EDIT_COUNT) {
				FARString strCommand;
				if (!s_cfg_reader->GetString(strCommand, StrPrintf("Command%d", CommandNumber), L""))
					break;

				EditDlg[EM_EDITLINE_0 + CommandNumber].strData = strCommand;
				CommandNumber++;
			}
		}

		Dialog Dlg(EditDlg, ARRAYSIZE(EditDlg), EditMenuDlgProc);
		Dlg.SetHelp(L"UserMenu");
		Dlg.SetPosition(-1, -1, DLG_X, DLG_Y);
		Dlg.SetId(EditUserMenuId);
		Dlg.Process();

		if (Dlg.GetExitCode() == EM_BUTTON_OK) {
			MenuModified = true;
			{
				ConfigWriter cfg_writer;
				cfg_writer.SelectSectionFmt("%ls/Item%u", MenuKey, (unsigned int)EditPos);

				if (Create) {
					cfg_writer.ReserveIndexedSection(StrPrintf("%ls/Item", MenuKey).c_str(),
							(unsigned int)EditPos);
				}

				cfg_writer.SetString("HotKey", EditDlg[EM_HOTKEY_EDIT].strData.CPtr());
				cfg_writer.SetString("Label", EditDlg[EM_LABEL_EDIT].strData.CPtr());
				cfg_writer.SetInt("Submenu", SubMenu ? 1 : 0);

				if (!SubMenu) {
					int CommandNumber = 0;

					for (int i = 0; i < DI_EDIT_COUNT; i++)
						if (!EditDlg[i + EM_EDITLINE_0].strData.IsEmpty())
							CommandNumber = i + 1;

					for (int i = 0; i < DI_EDIT_COUNT; i++) {
						const std::string &strCommandName = StrPrintf("Command%d", i);

						if (i >= CommandNumber)
							cfg_writer.RemoveKey(strCommandName);
						else
							cfg_writer.SetString(strCommandName, EditDlg[i + EM_EDITLINE_0].strData.CPtr());
					}
				}
			}

			ConfigReaderScope::Update(s_cfg_reader);
			Result = true;
		}
	}

	return Result;
}

int UserMenu::DeleteMenuRecord(const wchar_t *MenuKey, int DeletePos)
{
	FormatString strRegKey;
	strRegKey << MenuKey << L"/Item" << DeletePos;
	s_cfg_reader->SelectSection(strRegKey);
	FARString strRecText = s_cfg_reader->GetString("Label", L"");
	int SubMenu = s_cfg_reader->GetInt("Submenu", 0);
	FARString strItemName = strRecText;
	InsertQuote(strItemName);

	if (0
			!= Message(MSG_WARNING, 2, Msg::UserMenuTitle,
					SubMenu ? Msg::AskDeleteSubMenuItem : Msg::AskDeleteMenuItem, strItemName, Msg::Delete,
					Msg::Cancel)) {
		return FALSE;
	}

	MenuModified = MenuNeedRefresh = true;
	strRegKey.Clear();
	strRegKey << MenuKey << L"/Item";
	FARString DefragPrefix(strRegKey);
	strRegKey << DeletePos;
	{
		ConfigWriter cfg_writer(FARString(strRegKey).GetMB());
		cfg_writer.RemoveSection();
		cfg_writer.DefragIndexedSections(DefragPrefix.GetMB().c_str());
	}
	ConfigReaderScope::Update(s_cfg_reader);
	return TRUE;
}

bool UserMenu::MoveMenuItem(const wchar_t *MenuKey, int Pos, int NewPos)
{
	if (Pos != NewPos) {
		ConfigWriter().MoveIndexedSection(StrPrintf("%ls/Item", MenuKey).c_str(), Pos, NewPos);
		MenuModified = MenuNeedRefresh = true;
	}
	ConfigReaderScope::Update(s_cfg_reader);
	return true;
}
