/*
findfile.cpp

Поиск (Alt-F7)
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

#include "findfile.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "fileview.hpp"
#include "fileedit.hpp"
#include "filelist.hpp"
#include "cmdline.hpp"
#include "chgprior.hpp"
#include "namelist.hpp"
#include "scantree.hpp"
#include "manager.hpp"
#include "scrbuf.hpp"
#include "CFileMask.hpp"
#include "filefilter.hpp"
#include "syslog.hpp"
// #include "localOEM.hpp"
#include "codepage.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "farcolors.hpp"
#include "message.hpp"
#include "delete.hpp"
#include "datetime.hpp"
#include "drivemix.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "mix.hpp"
#include "constitle.hpp"
#include "DlgGuid.hpp"
#include "console.hpp"
#include "wakeful.hpp"
#include "panelmix.hpp"
#include "setattr.hpp"
#include "udlist.hpp"
#include "InterThreadCall.hpp"
#include "ThreadedWorkQueue.h"
#include "MountInfo.h"
#include <atomic>

constexpr DWORD LIST_INDEX_NONE = std::numeric_limits<DWORD>::max();

static bool AnySetFindList = false;

// Список найденных файлов. Индекс из списка хранится в меню.
struct FINDLIST
{
	FAR_FIND_DATA_EX FindData;
	size_t ArcIndex;
	DWORD Used;
};

// Список архивов. Если файл найден в архиве, то FindList->ArcIndex указывает сюда.
struct ARCLIST
{
	FARString strArcName;
	PHPTR hPlugin = nullptr;    // Plugin handle
	DWORD Flags   = 0;          // OpenPluginInfo.Flags
	FARString strRootPath;      // Root path in plugin after opening.
};

static struct InterThreadData
{
private:
	CriticalSection DataCS;
	size_t FindFileArcIndex;
	int Percent;
	int LastFoundNumber;
	int FileCount;
	int DirCount;

	std::vector<FINDLIST> FindList;
	std::vector<ARCLIST> ArcList;
	FARString strFindMessage;

public:
	void Init()
	{
		CriticalSectionLock Lock(DataCS);
		FindFileArcIndex = LIST_INDEX_NONE;
		Percent = 0;
		LastFoundNumber = 0;
		FileCount = 0;
		DirCount = 0;
		FindList.clear();
		ArcList.clear();
		strFindMessage.Clear();
	}

	size_t GetFindFileArcIndex()
	{
		CriticalSectionLock Lock(DataCS);
		return FindFileArcIndex;
	}
	void SetFindFileArcIndex(size_t Value)
	{
		CriticalSectionLock Lock(DataCS);
		FindFileArcIndex = Value;
	}

	int GetPercent()
	{
		CriticalSectionLock Lock(DataCS);
		return Percent;
	}
	void SetPercent(int Value)
	{
		CriticalSectionLock Lock(DataCS);
		Percent = Value;
	}

	int GetLastFoundNumber()
	{
		CriticalSectionLock Lock(DataCS);
		return LastFoundNumber;
	}
	void SetLastFoundNumber(int Value)
	{
		CriticalSectionLock Lock(DataCS);
		LastFoundNumber = Value;
	}

	int GetFileCount()
	{
		CriticalSectionLock Lock(DataCS);
		return FileCount;
	}
	void SetFileCount(int Value)
	{
		CriticalSectionLock Lock(DataCS);
		FileCount = Value;
	}

	int GetDirCount()
	{
		CriticalSectionLock Lock(DataCS);
		return DirCount;
	}
	void SetDirCount(int Value)
	{
		CriticalSectionLock Lock(DataCS);
		DirCount = Value;
	}

	size_t GetFindListCount()
	{
		CriticalSectionLock Lock(DataCS);
		return FindList.size();
	}

	void GetFindMessage(FARString &To)
	{
		CriticalSectionLock Lock(DataCS);
		To = strFindMessage;
	}

	void SetFindMessage(const FARString &From)
	{
		CriticalSectionLock Lock(DataCS);
		strFindMessage = From;
	}

	void GetFindListItem(size_t index, FINDLIST &Item)
	{
		CriticalSectionLock Lock(DataCS);
		Item = FindList[index];
	}

	void SetFindListItem(size_t index, const FINDLIST &Item)
	{
		CriticalSectionLock Lock(DataCS);
		FindList[index] = Item;
	}

	void GetArcListItem(size_t index, ARCLIST &Item)
	{
		CriticalSectionLock Lock(DataCS);
		Item = ArcList[index];
	}

	void SetArcListItem(size_t index, const ARCLIST &Item)
	{
		CriticalSectionLock Lock(DataCS);
		ArcList[index] = Item;
	}

	void ClearAllLists()
	{
		CriticalSectionLock Lock(DataCS);
		FindFileArcIndex = LIST_INDEX_NONE;

		FindList.clear();
		ArcList.clear();
	}

	size_t AddArcListItem(const wchar_t *ArcName, PHPTR hPlugin, DWORD dwFlags, const wchar_t *RootPath)
	{
		CriticalSectionLock Lock(DataCS);
		try {
			ArcList.emplace_back();
			auto &back = ArcList.back();
			back.strArcName = ArcName;
			back.hPlugin = hPlugin;
			back.Flags = dwFlags;
			back.strRootPath = RootPath;
			AddEndSlash(back.strRootPath);

		} catch (std::exception &ex) {
			fprintf(stderr, "AddArcListItem[%lu]: %s\n", (unsigned long)ArcList.size(), ex.what());
			return LIST_INDEX_NONE;
		}

		return ArcList.size() - 1;
	}

	size_t AddFindListItem(const FAR_FIND_DATA_EX &FindData)
	{
		CriticalSectionLock Lock(DataCS);
		try {
			FindList.emplace_back();
			auto &back = FindList.back();
			back.FindData = FindData;
			back.ArcIndex = LIST_INDEX_NONE;
			back.Used = 0;

		} catch (std::exception &ex) {
			fprintf(stderr, "AddFindListItem[%lu]: %s\n", (unsigned long)FindList.size(), ex.what());
			return LIST_INDEX_NONE;
		}

		return FindList.size() - 1;
	}

} itd;

enum
{
	FIND_EXIT_NONE,
	FIND_EXIT_SEARCHAGAIN,
	FIND_EXIT_GOTO,
	FIND_EXIT_PANEL
};

struct Vars
{
	Vars() {}

	~Vars() {}

	// Используются для отправки файлов на временную панель.
	// индекс текущего элемента в списке и флаг для отправки.
	DWORD FindExitIndex;
	bool FindFoldersChanged;
	bool SearchFromChanged;
	bool FindPositionChanged;
	bool Finalized;
	bool PluginMode;

	void Clear()
	{
		FindExitIndex = LIST_INDEX_NONE;
		FindFoldersChanged = false;
		SearchFromChanged = false;
		FindPositionChanged = false;
		Finalized = false;
		PluginMode = false;
	}
};

static FARString strFindMask, strFindStr;
static int SearchMode, CmpCase, WholeWords, SearchInArchives, SearchHex;

static FARString strLastDirName;
static FARString strPluginSearchPath;

static std::unique_ptr<ThreadedWorkQueue> pWorkQueue;
static std::unique_ptr<MountInfo> pMountInfo;
// static CriticalSection PluginCS;

class PluginLocker
{
	static bool s_locked;
	PluginLocker(const PluginLocker &) = delete;

	bool TryLock()
	{
		if (s_locked)
			return false;

		s_locked = true;
		return true;
	}

public:
	PluginLocker() { WAIT_FOR_AND_DISPATCH_INTER_THREAD_CALLS(TryLock()); }

	~PluginLocker()
	{
		InterThreadLockAndWake itlw;
		s_locked = false;
	}
};

bool PluginLocker::s_locked = false;

static std::atomic<bool> PauseFlag{false}, StopFlag{false};

static bool UseFilter = false;
static UINT CodePage = CP_AUTODETECT;
static UINT64 SearchInFirst = 0;

static int codePagesCount;

struct CodePageInfo
{
	UINT CodePage;
	UINT MaxCharSize;
	wchar_t LastSymbol;
	bool WordFound;
} *codePages;

static unsigned char *hexFindString;
static size_t hexFindStringSize;
static wchar_t *findString, *findStringBuffer;

static size_t *skipCharsTable;
static int favoriteCodePages = 0;

static bool InFileSearchInited = false;

static CFileMask FileMaskForFindFile;

static FileFilter *Filter;

enum ADVANCEDDLG
{
	AD_DOUBLEBOX,
	AD_TEXT_SEARCHFIRST,
	AD_EDIT_SEARCHFIRST,
	AD_CHECKBOX_FINDALTERNATESTREAMS,
	AD_SEPARATOR1,
	AD_TEXT_COLUMNSFORMAT,
	AD_EDIT_COLUMNSFORMAT,
	AD_TEXT_COLUMNSWIDTH,
	AD_EDIT_COLUMNSWIDTH,
	AD_SEPARATOR2,
	AD_BUTTON_OK,
	AD_BUTTON_CANCEL,
};

enum FINDASKDLG
{
	FAD_DOUBLEBOX,
	FAD_TEXT_MASK,
	FAD_EDIT_MASK,
	FAD_SEPARATOR0,
	FAD_TEXT_TEXTHEX,
	FAD_EDIT_TEXT,
	FAD_EDIT_HEX,
	FAD_TEXT_CP,
	FAD_COMBOBOX_CP,
	FAD_SEPARATOR1,
	FAD_CHECKBOX_CASEMASK,
	FAD_CHECKBOX_CASE,
	FAD_CHECKBOX_WHOLEWORDS,
	FAD_CHECKBOX_HEX,
	FAD_CHECKBOX_FILTER,
	FAD_CHECKBOX_ARC,
	FAD_CHECKBOX_DIRS,
	FAD_CHECKBOX_LINKS,
	FAD_SEPARATOR_2,
	FAD_SEPARATOR_3,
	FAD_TEXT_WHERE,
	FAD_COMBOBOX_WHERE,
	FAD_SEPARATOR_4,
	FAD_BUTTON_FIND,
	FAD_BUTTON_DRIVE,
	FAD_BUTTON_FILTER,
	FAD_BUTTON_ADVANCED,
	FAD_BUTTON_CANCEL,
};

enum FINDASKDLGCOMBO
{
	FADC_ALLDISKS,
	FADC_ALLBUTNET,
	FADC_PATH,
	FADC_ROOT,
	FADC_FROMCURRENT,
	FADC_INCURRENT,
	FADC_SELECTED,
};

enum FINDDLG
{
	FD_DOUBLEBOX,
	FD_LISTBOX,
	FD_SEPARATOR1,
	FD_TEXT_STATUS,
	FD_TEXT_STATUS_PERCENTS,
	FD_SEPARATOR2,
	FD_BUTTON_NEW,
	FD_BUTTON_GOTO,
	FD_BUTTON_VIEW,
	FD_BUTTON_EDIT,
	FD_BUTTON_PANEL,
	FD_BUTTON_STOP,
};

static void InitInFileSearch()
{
	if (!InFileSearchInited && !strFindStr.IsEmpty()) {
		size_t findStringCount = strFindStr.GetLength();

		if (!SearchHex) {
			// Формируем строку поиска
			if (!CmpCase) {
				findStringBuffer = (wchar_t *)malloc(2 * findStringCount * sizeof(wchar_t));
				findString = findStringBuffer;

				for (size_t index = 0; index < strFindStr.GetLength(); index++) {
					wchar_t ch = strFindStr[index];

					if (WINPORT(IsCharLower)(ch)) {
						findString[index] = Upper(ch);
						findString[index + findStringCount] = ch;
					} else {
						findString[index] = ch;
						findString[index + findStringCount] = Lower(ch);
					}
				}
			} else
				findString = strFindStr.GetBuffer();

			// Инициализируем данные для алгоритма поиска
			skipCharsTable = (size_t *)malloc((MAX_VKEY_CODE + 1) * sizeof(size_t));

			for (size_t index = 0; index < MAX_VKEY_CODE + 1; index++)
				skipCharsTable[index] = findStringCount;

			for (size_t index = 0; index < findStringCount - 1; index++)
				skipCharsTable[findString[index]] = findStringCount - 1 - index;

			if (!CmpCase)
				for (size_t index = 0; index < findStringCount - 1; index++)
					skipCharsTable[findString[index + findStringCount]] = findStringCount - 1 - index;

			// Формируем список кодовых страниц
			if (CodePage == CP_AUTODETECT) {
				// FARString codePageName;
				bool hasSelected = false;
				// Проверяем наличие выбранных страниц символов
				ConfigReader cfg_reader(FavoriteCodePagesKey);
				const auto &codepages = cfg_reader.EnumKeys();
				for (const auto &cp : codepages) {
					int selectType = cfg_reader.GetInt(cp, 0);
					if (selectType & CPST_FIND) {
						hasSelected = true;
						// codePageName = cp;
						break;
					}
				}

				// Добавляем стандартные таблицы символов
				if (!hasSelected) {
					codePagesCount = StandardCPCount;
					codePages = (CodePageInfo *)malloc(codePagesCount * sizeof(CodePageInfo));
					codePages[0].CodePage = WINPORT(GetOEMCP)();
					codePages[1].CodePage = WINPORT(GetACP)();
					codePages[2].CodePage = CP_KOI8R;
					codePages[3].CodePage = CP_UTF7;
					codePages[4].CodePage = CP_UTF8;
					codePages[5].CodePage = CP_UTF16LE;
					codePages[6].CodePage = CP_UTF16BE;
#if (__WCHAR_MAX__ > 0xffff)
					codePages[7].CodePage = CP_UTF32LE;
					codePages[8].CodePage = CP_UTF32BE;
#endif
				} else {
					codePagesCount = 0;
					codePages = nullptr;
				}

				// Добавляем стандартные таблицы символов
				for (const auto &cp : codepages) {
					int selectType = cfg_reader.GetInt(cp, 0);
					if (selectType & (hasSelected ? CPST_FIND : CPST_FAVORITE)) {
						UINT codePage = atoi(cp.c_str());

						// Проверяем дубли
						if (!hasSelected) {
							bool isDouble = false;

							for (int j = 0; j < StandardCPCount; j++)
								if (codePage == codePages[j].CodePage) {
									isDouble = true;
									break;
								}

							if (isDouble)
								continue;
						}

						codePages = (CodePageInfo *)realloc((void *)codePages,
								++codePagesCount * sizeof(CodePageInfo));
						codePages[codePagesCount - 1].CodePage = codePage;
					}
				}
			} else {
				codePagesCount = 1;
				codePages = (CodePageInfo *)malloc(codePagesCount * sizeof(CodePageInfo));
				codePages[0].CodePage = CodePage;
			}

			for (int index = 0; index < codePagesCount; index++) {
				CodePageInfo *cp = codePages + index;

				if (IsFullWideCodePage(cp->CodePage)) {
					cp->MaxCharSize = sizeof(wchar_t);
				} else {
					CPINFO cpi;

					if (!WINPORT(GetCPInfo)(cp->CodePage, &cpi))
						cpi.MaxCharSize = 0;    // Считаем, что ошибка и потом такие таблицы в поиске пропускаем

					cp->MaxCharSize = cpi.MaxCharSize;
				}

				cp->LastSymbol = 0;
				cp->WordFound = false;
			}
		} else {
			// Формируем hex-строку для поиска
			hexFindStringSize = 0;

			if (SearchHex) {
				bool flag = false;
				hexFindString = (unsigned char *)malloc((findStringCount - findStringCount / 3 + 1) / 2);

				for (size_t index = 0; index < strFindStr.GetLength(); index++) {
					wchar_t symbol = strFindStr.At(index);
					BYTE offset = 0;

					if (symbol >= L'a' && symbol <= L'f')
						offset = 87;
					else if (symbol >= L'A' && symbol <= L'F')
						offset = 55;
					else if (symbol >= L'0' && symbol <= L'9')
						offset = 48;
					else
						continue;

					if (!flag)
						hexFindString[hexFindStringSize++] = ((BYTE)symbol - offset) << 4;
					else
						hexFindString[hexFindStringSize - 1]|= ((BYTE)symbol - offset);

					flag = !flag;
				}
			}

			// Инициализируем данные для алгоритма поиска
			skipCharsTable = (size_t *)malloc((255 + 1) * sizeof(size_t));

			for (size_t index = 0; index < 255 + 1; index++)
				skipCharsTable[index] = hexFindStringSize;

			for (size_t index = 0; index < (size_t)hexFindStringSize - 1; index++)
				skipCharsTable[hexFindString[index]] = hexFindStringSize - 1 - index;
		}

		InFileSearchInited = true;
	}
}

static void ReleaseInFileSearch()
{
	if (InFileSearchInited && !strFindStr.IsEmpty()) {
		if (skipCharsTable) {
			free(skipCharsTable);
			skipCharsTable = nullptr;
		}

		if (codePages) {
			free(codePages);
			codePages = nullptr;
		}

		if (findStringBuffer) {
			free(findStringBuffer);
			findStringBuffer = nullptr;
		}

		if (hexFindString) {
			free(hexFindString);
			hexFindString = nullptr;
		}

		InFileSearchInited = false;
	}
}

// Проверяем символ на принадлежность разделителям слов
static bool IsWordDiv(const wchar_t symbol)
{
	// Так же разделителем является конец строки и пробельные символы
	return !symbol || IsSpace(symbol) || IsEol(symbol) || IsWordDiv(Opt.strWordDiv, symbol);
}

static void SetPluginDirectory(const wchar_t *DirName, PHPTR hPlugin, bool UpdatePanel = false)
{
	if (DirName && *DirName) {
		FARString strName(DirName);
		wchar_t *DirPtr = strName.GetBuffer();
		wchar_t *NamePtr = (wchar_t *)PointToName(DirPtr);

		if (NamePtr != DirPtr) {
			*(NamePtr - 1) = 0;
			// force plugin to update its file list (that can be empty at this time)
			// if not done SetDirectory may fail
			{
				int FileCount = 0;
				PluginPanelItem *PanelData = nullptr;

				if (CtrlObject->Plugins.GetFindData(hPlugin, &PanelData, &FileCount, OPM_SILENT)) {
					CtrlObject->Plugins.FreeFindData(hPlugin, PanelData, FileCount);
				}
			}

			if (*DirPtr) {
				if (*DirPtr != GOOD_SLASH)    // fix #182
					CtrlObject->Plugins.SetDirectory(hPlugin, L"/", OPM_SILENT);

				CtrlObject->Plugins.SetDirectory(hPlugin, DirPtr, OPM_SILENT);
			} else {
				CtrlObject->Plugins.SetDirectory(hPlugin, L"/", OPM_SILENT);
			}
		}

		// Отрисуем панель при необходимости.
		if (UpdatePanel) {
			CtrlObject->Cp()->ActivePanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->ActivePanel->GoToFile(NamePtr);
			CtrlObject->Cp()->ActivePanel->Show();
		}

		// strName.ReleaseBuffer(); Не надо. Строка все равно удаляется, лишний вызов StrLength.
	}
}

static LONG_PTR WINAPI AdvancedDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_CLOSE:

			if (Param1 == AD_BUTTON_OK) {
				LPCWSTR Data = reinterpret_cast<LPCWSTR>(
						SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, AD_EDIT_SEARCHFIRST, 0));

				if (Data && *Data && !CheckFileSizeStringFormat(Data)) {
					Message(MSG_WARNING, 1, Msg::FindFileAdvancedTitle, Msg::BadFileSizeFormat, Msg::Ok);
					return FALSE;
				}
			}

			break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static void AdvancedDialog()
{
	DialogDataEx AdvancedDlgData[] = {
			{DI_DOUBLEBOX, 3, 1,  52, 12, {},                                 0,                             Msg::FindFileAdvancedTitle         },
			{DI_TEXT,      5, 2,  0,  2,  {},                                 0,                             Msg::FindFileSearchFirst           },
			{DI_EDIT,      5, 3,  50, 3,  {},                                 0,                             Opt.FindOpt.strSearchInFirstSize   },
			{DI_CHECKBOX,  5, 4,  0,  4,  {Opt.FindOpt.FindAlternateStreams}, 0,                             Msg::FindAlternateStreams          },
			{DI_TEXT,      3, 5,  0,  5,  {},                                 DIF_SEPARATOR,                 L""                                },
			{DI_TEXT,      5, 6,  0,  6,  {},                                 0,                             Msg::FindAlternateModeTypes        },
			{DI_EDIT,      5, 7,  35, 7,  {},                                 0,                             Opt.FindOpt.strSearchOutFormat     },
			{DI_TEXT,      5, 8,  0,  8,  {},                                 0,                             Msg::FindAlternateModeWidths       },
			{DI_EDIT,      5, 9,  35, 9,  {},                                 0,                             Opt.FindOpt.strSearchOutFormatWidth},
			{DI_TEXT,      3, 10, 0,  10, {},                                 DIF_SEPARATOR,                 L""                                },
			{DI_BUTTON,    0, 11, 0,  11, {},                                 DIF_DEFAULT | DIF_CENTERGROUP, Msg::Ok                            },
			{DI_BUTTON,    0, 11, 0,  11, {},                                 DIF_CENTERGROUP,               Msg::Cancel                        }
    };
	MakeDialogItemsEx(AdvancedDlgData, AdvancedDlg);
	Dialog Dlg(AdvancedDlg, ARRAYSIZE(AdvancedDlg), AdvancedDlgProc);
	Dlg.SetHelp(L"FindFileAdvanced");
	Dlg.SetPosition(-1, -1, 52 + 4, 7 + 7);
	Dlg.Process();
	int ExitCode = Dlg.GetExitCode();

	if (ExitCode == AD_BUTTON_OK) {
		Opt.FindOpt.strSearchInFirstSize = AdvancedDlg[AD_EDIT_SEARCHFIRST].strData;
		SearchInFirst = ConvertFileSizeString(Opt.FindOpt.strSearchInFirstSize);

		Opt.FindOpt.strSearchOutFormat = AdvancedDlg[AD_EDIT_COLUMNSFORMAT].strData;
		Opt.FindOpt.strSearchOutFormatWidth = AdvancedDlg[AD_EDIT_COLUMNSWIDTH].strData;

		Opt.FindOpt.OutColumns.clear();

		if (!Opt.FindOpt.strSearchOutFormat.IsEmpty()) {
			if (Opt.FindOpt.strSearchOutFormatWidth.IsEmpty())
				Opt.FindOpt.strSearchOutFormatWidth = L"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";

			TextToViewSettings(Opt.FindOpt.strSearchOutFormat.CPtr(),
					Opt.FindOpt.strSearchOutFormatWidth.CPtr(), Opt.FindOpt.OutColumns);
		}

		Opt.FindOpt.FindAlternateStreams =
				(AdvancedDlg[AD_CHECKBOX_FINDALTERNATESTREAMS].Selected == BSTATE_CHECKED);
	}
}

static LONG_PTR WINAPI MainDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	Vars *v = reinterpret_cast<Vars *>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
	switch (Msg) {
		case DN_INITDIALOG: {
			bool Hex = (SendDlgMessage(hDlg, DM_GETCHECK, FAD_CHECKBOX_HEX, 0) == BSTATE_CHECKED);
			SendDlgMessage(hDlg, DM_SHOWITEM, FAD_EDIT_TEXT, !Hex);
			SendDlgMessage(hDlg, DM_SHOWITEM, FAD_EDIT_HEX, Hex);
			SendDlgMessage(hDlg, DM_ENABLE, FAD_TEXT_CP, !Hex);
			SendDlgMessage(hDlg, DM_ENABLE, FAD_COMBOBOX_CP, !Hex);
			SendDlgMessage(hDlg, DM_ENABLE, FAD_CHECKBOX_CASE, !Hex);
			SendDlgMessage(hDlg, DM_ENABLE, FAD_CHECKBOX_WHOLEWORDS, !Hex);
			SendDlgMessage(hDlg, DM_ENABLE, FAD_CHECKBOX_DIRS, !Hex);
			SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, FAD_EDIT_TEXT, 1);
			SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, FAD_EDIT_HEX, 1);
			SendDlgMessage(hDlg, DM_SETTEXTPTR, FAD_TEXT_TEXTHEX,
					(LONG_PTR)(Hex ? Msg::FindFileHex : Msg::FindFileText).CPtr());
			SendDlgMessage(hDlg, DM_SETTEXTPTR, FAD_TEXT_CP, (LONG_PTR)Msg::FindFileCodePage.CPtr());
			SendDlgMessage(hDlg, DM_SETCOMBOBOXEVENT, FAD_COMBOBOX_CP, CBET_KEY);
			FarListTitles Titles = {0, nullptr, 0, Msg::FindFileCodePageBottom};
			SendDlgMessage(hDlg, DM_LISTSETTITLES, FAD_COMBOBOX_CP, reinterpret_cast<LONG_PTR>(&Titles));
			// Установка запомненных ранее параметров
			CodePage = Opt.FindCodePage;
			favoriteCodePages = FillCodePagesList(hDlg, FAD_COMBOBOX_CP, CodePage, false, true);
			// Текущее значение в списке выбора кодовых страниц в общем случае может не совпадать с CodePage,
			// так что получаем CodePage из списка выбора
			FarListPos Position;
			SendDlgMessage(hDlg, DM_LISTGETCURPOS, FAD_COMBOBOX_CP, (LONG_PTR)&Position);
			FarListGetItem Item = {Position.SelectPos, {}};
			SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_CP, (LONG_PTR)&Item);
			CodePage = (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, FAD_COMBOBOX_CP, Position.SelectPos);
			return TRUE;
		}
		case DN_CLOSE: {
			switch (Param1) {
				case FAD_BUTTON_FIND: {
					LPCWSTR Mask = (LPCWSTR)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, FAD_EDIT_MASK, 0);

					if (!Mask || !*Mask)
						Mask = L"*";

					return FileMaskForFindFile.Set(Mask, 0);
				}
				case FAD_BUTTON_DRIVE: {
					IsRedrawFramesInProcess++;
					CtrlObject->Cp()->ActivePanel->ChangeDisk();
					// Ну что ж, раз пошла такая пьянка рефрешить фреймы
					// будем таким способом.
					// FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
					FrameManager->ResizeAllFrame();
					IsRedrawFramesInProcess--;
					FARString strSearchFromRoot(Msg::SearchFromRootFolder);
					FarListGetItem item = {FADC_ROOT, {}};
					SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_WHERE, (LONG_PTR)&item);
					item.Item.Text = strSearchFromRoot;
					SendDlgMessage(hDlg, DM_LISTUPDATE, FAD_COMBOBOX_WHERE, (LONG_PTR)&item);
					v->PluginMode = CtrlObject->Cp()->ActivePanel->GetMode() == PLUGIN_PANEL;
					SendDlgMessage(hDlg, DM_ENABLE, FAD_CHECKBOX_DIRS, v->PluginMode ? FALSE : TRUE);
					item.ItemIndex = FADC_ALLDISKS;
					SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_WHERE, (LONG_PTR)&item);

					if (v->PluginMode)
						item.Item.Flags|= LIF_GRAYED;
					else
						item.Item.Flags&= ~LIF_GRAYED;

					SendDlgMessage(hDlg, DM_LISTUPDATE, FAD_COMBOBOX_WHERE, (LONG_PTR)&item);
					item.ItemIndex = FADC_ALLBUTNET;
					SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_WHERE, (LONG_PTR)&item);

					if (v->PluginMode)
						item.Item.Flags|= LIF_GRAYED;
					else
						item.Item.Flags&= ~LIF_GRAYED;

					SendDlgMessage(hDlg, DM_LISTUPDATE, FAD_COMBOBOX_WHERE, (LONG_PTR)&item);
				} break;
				case FAD_BUTTON_FILTER:
					Filter->FilterEdit();
					break;
				case FAD_BUTTON_ADVANCED:
					AdvancedDialog();
					break;
				case -2:
				case -1:
				case FAD_BUTTON_CANCEL:
					return TRUE;
			}

			return FALSE;
		}
		case DN_BTNCLICK: {
			switch (Param1) {
				case FAD_CHECKBOX_DIRS: {
					v->FindFoldersChanged = true;
				} break;

				case FAD_CHECKBOX_HEX: {
					SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
					FARString strDataStr;
					Transform(strDataStr,
							(LPCWSTR)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR,
									Param2 ? FAD_EDIT_TEXT : FAD_EDIT_HEX, 0),
							Param2 ? L'X' : L'S');
					SendDlgMessage(hDlg, DM_SETTEXTPTR, Param2 ? FAD_EDIT_HEX : FAD_EDIT_TEXT,
							(LONG_PTR)strDataStr.CPtr());
					SendDlgMessage(hDlg, DM_SHOWITEM, FAD_EDIT_TEXT, !Param2);
					SendDlgMessage(hDlg, DM_SHOWITEM, FAD_EDIT_HEX, Param2);
					SendDlgMessage(hDlg, DM_ENABLE, FAD_TEXT_CP, !Param2);
					SendDlgMessage(hDlg, DM_ENABLE, FAD_COMBOBOX_CP, !Param2);
					SendDlgMessage(hDlg, DM_ENABLE, FAD_CHECKBOX_CASE, !Param2);
					SendDlgMessage(hDlg, DM_ENABLE, FAD_CHECKBOX_WHOLEWORDS, !Param2);
					SendDlgMessage(hDlg, DM_ENABLE, FAD_CHECKBOX_DIRS, !Param2);
					SendDlgMessage(hDlg, DM_SETTEXTPTR, FAD_TEXT_TEXTHEX,
							(LONG_PTR)(Param2 ? Msg::FindFileHex : Msg::FindFileText).CPtr());

					if (strDataStr.GetLength() > 0) {
						int UnchangeFlag = (int)SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, FAD_EDIT_TEXT, -1);
						SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, FAD_EDIT_HEX, UnchangeFlag);
					}

					SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
				} break;
			}

			break;
		}
		case DM_KEY: {
			switch (Param1) {
				case FAD_COMBOBOX_CP: {
					switch (Param2) {
						case KEY_INS:
						case KEY_NUMPAD0:
						case KEY_SPACE: {
							// Обработка установки/снятия флажков для стандартных и любимых таблиц символов
							// Получаем текущую позицию в выпадающем списке таблиц символов
							FarListPos Position;
							SendDlgMessage(hDlg, DM_LISTGETCURPOS, FAD_COMBOBOX_CP, (LONG_PTR)&Position);
							// Получаем номер выбранной таблицы символов
							FarListGetItem Item = {Position.SelectPos, {}};
							SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_CP, (LONG_PTR)&Item);
							UINT SelectedCodePage = (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA,
									FAD_COMBOBOX_CP, Position.SelectPos);
							// Разрешаем отмечать только стандартные и любимые таблицы символов
							int FavoritesIndex = 2 + StandardCPCount + 2;

							if (Position.SelectPos > 1
									&& Position.SelectPos < FavoritesIndex
													+ (favoriteCodePages ? favoriteCodePages + 1 : 0)) {
								// Преобразуем номер таблицы символов к строке
								const std::string &strCodePageName = ToDec(SelectedCodePage);
								//  Получаем текущее состояние флага в реестре
								int SelectType =
										ConfigReader(FavoriteCodePagesKey).GetInt(strCodePageName, 0);

								// Отмечаем/разотмечаем таблицу символов
								if (Item.Item.Flags & LIF_CHECKED) {
									// Для стандартных таблиц символов просто удаляем значение из реестра, для
									// любимых же оставляем в реестре флаг, что таблица символов любимая
									ConfigWriter cfg_writer(FavoriteCodePagesKey);
									if (SelectType & CPST_FAVORITE)
										cfg_writer.SetInt(strCodePageName, CPST_FAVORITE);
									else
										cfg_writer.RemoveKey(strCodePageName);

									Item.Item.Flags&= ~LIF_CHECKED;
								} else {
									ConfigWriter(FavoriteCodePagesKey)
											.SetInt(strCodePageName,
													CPST_FIND
															| (SelectType & CPST_FAVORITE ? CPST_FAVORITE
																						  : 0));
									Item.Item.Flags|= LIF_CHECKED;
								}

								// Обновляем текущий элемент в выпадающем списке
								SendDlgMessage(hDlg, DM_LISTUPDATE, FAD_COMBOBOX_CP, (LONG_PTR)&Item);

								if (Position.SelectPos < FavoritesIndex
												+ (favoriteCodePages ? favoriteCodePages + 1 : 0) - 2) {
									FarListPos Pos = {Position.SelectPos + 1, Position.TopPos};
									SendDlgMessage(hDlg, DM_LISTSETCURPOS, FAD_COMBOBOX_CP,
											reinterpret_cast<LONG_PTR>(&Pos));
								}

								// Обрабатываем случай, когда таблица символов может присутствовать, как в стандартных, так и в любимых,
								// т.е. выбор/снятие флага автоматически происходит у обоих элементов
								bool bStandardCodePage = Position.SelectPos < FavoritesIndex;

								for (int Index = bStandardCodePage ? FavoritesIndex : 0; Index
										< (bStandardCodePage ? FavoritesIndex + favoriteCodePages
															 : FavoritesIndex);
										Index++) {
									// Получаем элемент таблицы символов
									FarListGetItem CheckItem = {Index, {}};
									SendDlgMessage(hDlg, DM_LISTGETITEM, FAD_COMBOBOX_CP,
											(LONG_PTR)&CheckItem);

									// Обрабатываем только таблицы символов
									if (!(CheckItem.Item.Flags & LIF_SEPARATOR)) {
										if (SelectedCodePage
												== (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, FAD_COMBOBOX_CP,
														Index)) {
											if (Item.Item.Flags & LIF_CHECKED)
												CheckItem.Item.Flags|= LIF_CHECKED;
											else
												CheckItem.Item.Flags&= ~LIF_CHECKED;

											SendDlgMessage(hDlg, DM_LISTUPDATE, FAD_COMBOBOX_CP,
													(LONG_PTR)&CheckItem);
											break;
										}
									}
								}
							}
						} break;
					}
				} break;
			}

			break;
		}
		case DN_EDITCHANGE: {
			FarDialogItem &Item = *reinterpret_cast<FarDialogItem *>(Param2);

			switch (Param1) {
				case FAD_EDIT_TEXT: {
					// Строка "Содержащих текст"
					if (!v->FindFoldersChanged) {
						BOOL Checked = (Item.PtrData && *Item.PtrData) ? FALSE : Opt.FindOpt.FindFolders;
						SendDlgMessage(hDlg, DM_SETCHECK, FAD_CHECKBOX_DIRS,
								Checked ? BSTATE_CHECKED : BSTATE_UNCHECKED);
					}

					return TRUE;
				} break;

				case FAD_COMBOBOX_CP: {
					// Получаем выбранную в выпадающем списке таблицу символов
					CodePage = (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, FAD_COMBOBOX_CP,
							SendDlgMessage(hDlg, DM_LISTGETCURPOS, FAD_COMBOBOX_CP, 0));
				}
					return TRUE;
				case FAD_COMBOBOX_WHERE: {
					v->SearchFromChanged = true;
				}
					return TRUE;
			}
		}
		case DN_HOTKEY: {
			if (Param1 == FAD_TEXT_TEXTHEX) {
				bool Hex = (SendDlgMessage(hDlg, DM_GETCHECK, FAD_CHECKBOX_HEX, 0) == BSTATE_CHECKED);
				SendDlgMessage(hDlg, DM_SETFOCUS, Hex ? FAD_EDIT_HEX : FAD_EDIT_TEXT, 0);
				return FALSE;
			}
		}
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool GetPluginFile(size_t ArcIndex, const FAR_FIND_DATA_EX &FindData, const wchar_t *DestPath,
		FARString &strResultName)
{
	_ALGO(CleverSysLog clv(L"FindFiles::GetPluginFile()"));
	ARCLIST ArcItem;
	itd.GetArcListItem(ArcIndex, ArcItem);
	OpenPluginInfo Info;
	CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin, &Info);
	FARString strSaveDir = NullToEmpty(Info.CurDir);
	AddEndSlash(strSaveDir);
	CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin, L"/", OPM_SILENT);
	// SetPluginDirectory(ArcList[ArcIndex]->strRootPath,hPlugin);
	SetPluginDirectory(FindData.strFileName, ArcItem.hPlugin);
	const wchar_t *lpFileNameToFind = PointToName(FindData.strFileName);
	PluginPanelItem *pItems;
	int nItemsNumber;
	bool nResult = false;

	if (CtrlObject->Plugins.GetFindData(ArcItem.hPlugin, &pItems, &nItemsNumber, OPM_SILENT)) {
		for (int i = 0; i < nItemsNumber; i++) {
			PluginPanelItem Item = pItems[i];
			Item.FindData.lpwszFileName =
					const_cast<LPWSTR>(PointToName(NullToEmpty(pItems[i].FindData.lpwszFileName)));

			if (!StrCmp(lpFileNameToFind, Item.FindData.lpwszFileName)) {
				nResult = CtrlObject->Plugins.GetFile(ArcItem.hPlugin, &Item, DestPath, strResultName, OPM_SILENT);
				break;
			}
		}

		CtrlObject->Plugins.FreeFindData(ArcItem.hPlugin, pItems, nItemsNumber);
	}

	CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin, L"/", OPM_SILENT);
	SetPluginDirectory(strSaveDir, ArcItem.hPlugin);
	return nResult;
}

// Алгоритма Бойера-Мура-Хорспула поиска подстроки (Unicode версия)

#define WC16(wc) ((wc)&0xffff)    // workaround since buffers were designed for 16bit wchar_t

static int FindStringBMH(const wchar_t *searchBuffer, size_t searchBufferCount)
{
	size_t findStringCount = strFindStr.GetLength();
	const wchar_t *buffer = searchBuffer;
	const wchar_t *findStringLower = CmpCase ? nullptr : findString + findStringCount;
	size_t lastBufferChar = findStringCount - 1;
	while (searchBufferCount >= findStringCount) {
		for (size_t index = lastBufferChar; WC16(buffer[index]) == WC16(findString[index])
				|| (findStringLower && WC16(buffer[index]) == WC16(findStringLower[index]));
				index--) {
			if (!index)
				return static_cast<int>(buffer - searchBuffer);
		}

		size_t offset = skipCharsTable[WC16(buffer[lastBufferChar])];
		searchBufferCount-= offset;
		buffer+= offset;
	}

	return -1;
}

// Алгоритма Бойера-Мура-Хорспула поиска подстроки (Char версия)
static int FindStringBMH(const unsigned char *searchBuffer, size_t searchBufferCount)
{
	const unsigned char *buffer = searchBuffer;
	size_t lastBufferChar = hexFindStringSize - 1;

	while (searchBufferCount >= hexFindStringSize) {
		for (size_t index = lastBufferChar; buffer[index] == hexFindString[index]; index--)
			if (!index)
				return static_cast<int>(buffer - searchBuffer);

		size_t offset = skipCharsTable[buffer[lastBufferChar]];
		searchBufferCount-= offset;
		buffer+= offset;
	}

	return -1;
}

static bool ScanFile(const wchar_t *Name)
{
#define RETURN(r)                                                                                            \
	{                                                                                                        \
		result = (r);                                                                                        \
		goto exit;                                                                                           \
	}
#define CONTINUE(r)                                                                                          \
	{                                                                                                        \
		if ((r) || cpIndex == codePagesCount - 1)                                                            \
			RETURN(r) else continue;                                                                         \
	}
	// Длина строки поиска
	const size_t findStringCount = strFindStr.GetLength();

	// Если строки поиска пустая, то считаем, что мы всегда что-нибудь найдём
	if (findStringCount == 0)
		return true;

	File file;
	// Открываем файл
	if (!file.Open(Name, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
				FILE_FLAG_SEQUENTIAL_SCAN))
		return false;

	char readBufferA[0x10000] __attribute__((aligned(0x1000)));
	wchar_t readBuffer[ARRAYSIZE(readBufferA)] __attribute__((aligned(0x1000)));

	// Количество считанных из файла байт
	DWORD readBlockSize = 0;
	// Количество прочитанных из файла байт
	uint64_t alreadyRead = 0;
	// Смещение на которое мы отступили при переходе между блоками
	int offset = 0;

	if (SearchHex)
		offset = (int)hexFindStringSize - 1;

	UINT64 FileSize = 0;
	file.GetSize(FileSize);

	if (SearchInFirst) {
		FileSize = Min(SearchInFirst, FileSize);
	}

	UINT LastPercents = 0;

	// Результат поиска
	bool result = false;

	// Основной цикл чтения из файла

	while (!StopFlag
			&& file.Read(readBufferA,
					(!SearchInFirst || alreadyRead + sizeof(readBufferA) <= SearchInFirst)
							? sizeof(readBufferA)
							: static_cast<DWORD>(SearchInFirst - alreadyRead),
					&readBlockSize)) {
		UINT Percents = static_cast<UINT>(FileSize ? alreadyRead * 100 / FileSize : 0);
		if (Percents != LastPercents) {
			itd.SetPercent(Percents);
			LastPercents = Percents;
		}

		// Увеличиваем счётчик прочитанных байт
		alreadyRead+= readBlockSize;

		// Для hex и обыкновенного поиска разные ветки
		if (SearchHex) {
			// Выходим, если ничего не прочитали или прочитали мало
			if (!readBlockSize || readBlockSize < hexFindStringSize)
				RETURN(false)

			// Ищем
			if (FindStringBMH((const unsigned char *)readBufferA, readBlockSize) != -1)
				RETURN(true)
		} else {
			for (int cpIndex = 0; cpIndex < codePagesCount; cpIndex++) {
				// Информация о кодовой странице
				CodePageInfo *cpi = codePages + cpIndex;

				// Пропускаем ошибочные кодовые страницы
				if (!cpi->MaxCharSize)
					CONTINUE(false)

				// Если начало файла очищаем информацию о поиске по словам
				if (WholeWords && alreadyRead == readBlockSize) {
					cpi->WordFound = false;
					cpi->LastSymbol = 0;
				}

				// Если ничего не прочитали
				if (!readBlockSize)
					// Если поиск по словам и в конце предыдущего блока было что-то найдено,
					// то считаем, что нашли то, что нужно
					CONTINUE(WholeWords && cpi->WordFound)

				// Выходим, если прочитали меньше размера строки поиска и нет поиска по словам
				if (readBlockSize < findStringCount && !(WholeWords && cpi->WordFound))
					CONTINUE(FALSE)

				// Количество символов в выходном буфере
				unsigned int bufferCount;

				// Буфер для поиска
				wchar_t *buffer;

				// Перегоняем буфер в wchar_t
				if (IsFullWideCodePage(cpi->CodePage)) {
					// Вычисляем размер буфера в wchar_t
					bufferCount = readBlockSize / sizeof(wchar_t);

					// Выходим, если размер буфера меньше длины строки посика
					if (bufferCount < findStringCount)
						CONTINUE(false)

					// Копируем буфер чтения в буфер сравнения
					// todo
					if (cpi->CodePage == CP_WIDE_BE) {
						WideReverse((const wchar_t *)readBufferA, readBuffer, bufferCount);
						buffer = readBuffer;
					} else {
						buffer = (wchar_t *)readBufferA;
					}
				} else {
					// Конвертируем буфер чтения из кодировки поиска в wchar_t
					bufferCount = WINPORT(MultiByteToWideChar)(cpi->CodePage, 0, (char *)readBufferA,
							readBlockSize, readBuffer, ARRAYSIZE(readBuffer));

					// Выходим, если нам не удалось сконвертировать строку
					if (!bufferCount)
						CONTINUE(false)

					// Если прочитали меньше размера строки поиска и поиска по словам, то проверяем
					// первый символ блока на разделитель и выходим
					// Если у нас поиск по словам и в конце предыдущего блока было вхождение
					if (WholeWords && cpi->WordFound) {
						// Если конец файла, то считаем, что есть разделитель в конце
						if (findStringCount - 1 >= bufferCount)
							RETURN(true)
						// Проверяем первый символ текущего блока с учётом обратного смещения, которое делается
						// при переходе между блоками
						cpi->LastSymbol = readBuffer[findStringCount - 1];

						if (IsWordDiv(cpi->LastSymbol))
							RETURN(true)

						// Если размер буфера меньше размера слова, то выходим
						if (readBlockSize < findStringCount)
							CONTINUE(false)
					}

					// Устанавливаем буфер сравнения
					buffer = readBuffer;
				}

				unsigned int index = 0;

				do {
					// Ищем подстроку в буфере и возвращаем индекс её начала в случае успеха
					int foundIndex = FindStringBMH(buffer + index, bufferCount - index);

					// Если подстрока не найдена идём на следующий шаг
					if (foundIndex == -1)
						break;

					// Если подстрока найдена и отключен поиск по словам, то считаем что всё хорошо
					if (!WholeWords)
						RETURN(TRUE)

					// Устанавливаем позицию в исходном буфере
					index+= foundIndex;

					// Если идёт поиск по словам, то делаем соответвующие проверки
					bool firstWordDiv = false;

					// Если мы находимся в начале блока
					if (!index) {
						// Если мы находимся в начале файла, то считаем, что разделитель есть
						// Если мы находимся в начале блока, то проверяем является
						// или нет последний символ предыдущего блока разделителем
						if (alreadyRead == readBlockSize || IsWordDiv(cpi->LastSymbol))
							firstWordDiv = true;
					} else {
						// Проверяем является или нет предыдущий найденному символ блока разделителем
						cpi->LastSymbol = buffer[index - 1];

						if (IsWordDiv(cpi->LastSymbol))
							firstWordDiv = true;
					}

					// Проверяем разделитель в конце, только если найден разделитель в начале
					if (firstWordDiv) {
						// Если блок выбран не до конца
						if (index + findStringCount != bufferCount) {
							// Проверяем является или нет последующий за найденным символ блока разделителем
							cpi->LastSymbol = buffer[index + findStringCount];

							if (IsWordDiv(cpi->LastSymbol))
								RETURN(true)
						} else
							cpi->WordFound = true;
					}
				} while (++index <= bufferCount - findStringCount);

				// Выходим, если мы вышли за пределы количества байт разрешённых для поиска
				if (SearchInFirst && SearchInFirst >= alreadyRead)
					CONTINUE(false)
				// Запоминаем последний символ блока
				cpi->LastSymbol = buffer[bufferCount - 1];
			}

			// Получаем смещение на которое мы отступили при переходе между блоками
			offset = (int)((CodePage == CP_AUTODETECT ? sizeof(wchar_t) : codePages->MaxCharSize)
					* (findStringCount - 1));
		}

		// Если мы потенциально прочитали не весь файл
		if (readBlockSize == sizeof(readBufferA)) {
			// Отступаем назад на длину слова поиска минус 1
			if (!file.SetPointer(-1 * offset, nullptr, FILE_CURRENT))
				RETURN(FALSE)
			alreadyRead-= offset;
		}
	}

exit:
	// Закрываем хэндл файла
	file.Close();
	// Возвращаем результат
	return (result);
#undef CONTINUE
#undef RETURN
}

static void
AddMenuRecord(HANDLE hDlg, const wchar_t *FullName, const FAR_FIND_DATA_EX &FindData, size_t ArcIndex);

struct ScanFileWorkItem : IThreadedWorkItem
{
	ScanFileWorkItem(HANDLE hDlg, FARString &FileToScan, FARString &FileToReport, bool RemoveTemp,
			const FAR_FIND_DATA_EX &FindData, size_t ArcIndex)
		:
		_hDlg(hDlg),
		_FileToScan(FileToScan),
		_FileToReport(FileToReport),
		_RemoveTemp(RemoveTemp),
		_FindData(FindData),
		_ArcIndex(ArcIndex)
	{}

	virtual ~ScanFileWorkItem()
	{
		if (_RemoveTemp)
			DeleteFileWithFolder(_FileToScan);

		if (_Result)
			AddMenuRecord(_hDlg, _FileToReport, _FindData, _ArcIndex);
	}

	// invoked within worker thread, so make sure no FARString copied within this function
	// as it has not thread-safe (but fast!) reference counters implementation
	virtual void WorkProc()
	{
		SudoClientRegion scr;
		SudoSilentQueryRegion ssqr;
		_Result = ScanFile(_FileToScan);
	}

private:
	HANDLE _hDlg;
	FARString _FileToScan, _FileToReport;
	bool _RemoveTemp;
	FAR_FIND_DATA_EX _FindData;
	size_t _ArcIndex;

	bool _Result = false;
};

static void AnalyzeFileItem(HANDLE hDlg, PluginPanelItem *FileItem, const wchar_t *FileName,
		const FAR_FIND_DATA_EX &FindData)
{
	// Если включен режим поиска содержимого, тогда в поиск включаем только обычные файлы
	if ((FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE)) != 0
			&& (SearchHex || !strFindStr.IsEmpty()))
		return;
	if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && !Opt.FindOpt.FindFolders)
		return;

	if (!FileMaskForFindFile.Compare(FileName, Opt.FindOpt.FindCaseSensitiveFileMask))
		return;

	size_t ArcIndex = itd.GetFindFileArcIndex();

	FARString FileToReport = FileName;
	if (ArcIndex != LIST_INDEX_NONE) {
		FileToReport.Insert(0, strPluginSearchPath);
	}

	if (strFindStr.IsEmpty()) {
		AddMenuRecord(hDlg, FileToReport, FindData, ArcIndex);
		return;
	}

	bool RemoveTemp = false;

	PHPTR hPlugin = nullptr;
	if (ArcIndex != LIST_INDEX_NONE) {
		ARCLIST ArcItem;
		itd.GetArcListItem(ArcIndex, ArcItem);
		hPlugin = ArcItem.hPlugin;
	}

	FARString FileToScan;
	if (hPlugin) {
		SCOPED_ACTION(PluginLocker);
		if (!CtrlObject->Plugins.UseFarCommand(hPlugin, PLUGIN_FARGETFILES)) {
			FARString strTempDir;
			FarMkTempEx(strTempDir);
			apiCreateDirectory(strTempDir, nullptr);

			bool GetFileResult = false;
			GetFileResult = CtrlObject->Plugins.GetFile(hPlugin, FileItem, strTempDir, FileToScan,
										OPM_SILENT | OPM_FIND);
			if (!GetFileResult) {
				apiRemoveDirectory(strTempDir);
				return;
			}
			RemoveTemp = true;
		} else
			FileToScan = FileToReport;
	} else
		FileToScan = FileName;

	if (pMountInfo->IsMultiThreadFriendly(FileToScan.GetMB())) {
		ScanFileWorkItem *wi = new (std::nothrow)
				ScanFileWorkItem(hDlg, FileToScan, FileToReport, RemoveTemp, FindData, ArcIndex);
		if (wi) {    // do file contents scan and following logic asynchronously
			pWorkQueue->Queue(wi);
			return;
		}
	}

	// fallback to synchronous logic
	ScanFileWorkItem(hDlg, FileToScan, FileToReport, RemoveTemp, FindData, ArcIndex).WorkProc();
}

static void AnalyzeFileItem(HANDLE hDlg, PluginPanelItem *FileItem, const wchar_t *FileName,
		const FAR_FIND_DATA &FindData)
{
	FAR_FIND_DATA_EX fdata{};
	apiFindDataToDataEx(&FindData, &fdata);
	AnalyzeFileItem(hDlg, FileItem, FileName, fdata);
}

class FindDlg_TempFileHolder : public TempFileUploadHolder
{
	size_t ArcIndex;
	FAR_FIND_DATA_EX FindData;

	virtual bool UploadTempFile()
	{
		SCOPED_ACTION(PluginLocker);
		ARCLIST ArcItem;
		itd.GetArcListItem(ArcIndex, ArcItem);
		bool ClosePlugin = false;

		FARString strSaveDir;

		if (ArcItem.hPlugin == nullptr) {
			FARString strFindArcName = ArcItem.strArcName;
			int SavePluginsOutput = DisablePluginsOutput;
			DisablePluginsOutput = TRUE;
			ArcItem.hPlugin = CtrlObject->Plugins.OpenFilePlugin(strFindArcName, OPM_FIND, OFP_SEARCH);
			DisablePluginsOutput = SavePluginsOutput;
			if (ArcItem.hPlugin == PHPTR_STOP || ArcItem.hPlugin == nullptr) {
				ArcItem.hPlugin = nullptr;
				fprintf(stderr, "OnEditedFileSaved: can't open plugins '%ls'\n", strFindArcName.CPtr());
				return false;
			}
			ClosePlugin = true;
		} else {
			OpenPluginInfo Info;
			CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin, &Info);

			strSaveDir = NullToEmpty(Info.CurDir);
			AddEndSlash(strSaveDir);
		}
		CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin, L"/", OPM_SILENT);
		// SetPluginDirectory(ArcList[ArcIndex]->strRootPath,hPlugin);
		SetPluginDirectory(FindData.strFileName, ArcItem.hPlugin);
		//		const wchar_t *lpFileNameToFind = PointToName(FindData.strFileName);
		//		PluginPanelItem *pItems;
		//		int nItemsNumber;
		//		bool nResult=false;
		PluginPanelItem PanelItem;
		//		FARString strSaveCurDir;
		//		apiGetCurrentDirectory(strSaveCurDir);

		//		FARString strTempName = FileName;

		bool out = false;
		if (FileList::FileNameToPluginItem(GetPathName(), &PanelItem)) {
			out = (CtrlObject->Plugins.PutFiles(ArcItem.hPlugin, &PanelItem, 1, FALSE, OPM_EDIT) != 0);

			if (!out) {
				Message(MSG_WARNING, 1, Msg::Error, Msg::CannotSaveFile, Msg::TextSavedToTemp, GetPathName(),
						Msg::Ok);
			}
		}

		//		FarChDir(strSaveCurDir);

		if (ClosePlugin) {
			CtrlObject->Plugins.ClosePanel(ArcItem.hPlugin);
		} else {
			CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin, L"/", OPM_SILENT);
			SetPluginDirectory(strSaveDir, ArcItem.hPlugin);
		}

		return out;
	}

public:
	FindDlg_TempFileHolder(FARString strTempName_, size_t ArcIndex_, const FAR_FIND_DATA_EX &FindData_)
		:
		TempFileUploadHolder(strTempName_), ArcIndex(ArcIndex_), FindData(FindData_)
	{}
};

static LONG_PTR WINAPI FindDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	Vars *v = reinterpret_cast<Vars *>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
	Dialog *Dlg = reinterpret_cast<Dialog *>(hDlg);
	VMenu *ListBox = Dlg->GetAllItem()[FD_LISTBOX]->ListPtr;

	static bool Recurse = false;
	static DWORD ShowTime = 0;

	if (!v->Finalized && !Recurse) {
		Recurse = true;
		DWORD Time = WINPORT(GetTickCount)();
		if (Time - ShowTime > RedrawTimeout) {
			ShowTime = Time;
			if (!StopFlag) {
				FARString strDataStr;
				strDataStr.Format(Msg::FindFound, itd.GetFileCount(), itd.GetDirCount());
				SendDlgMessage(hDlg, DM_SETTEXTPTR, 2, (LONG_PTR)strDataStr.CPtr());

				FARString strSearchStr;

				if (!strFindStr.IsEmpty()) {
					FARString strFStr(strFindStr);
					TruncStrFromEnd(strFStr, 10);
					FARString strTemp(L" \"");
					strTemp+= strFStr+= "\"";
					strSearchStr.Format(Msg::FindSearchingIn, strTemp.CPtr());
				} else
					strSearchStr.Format(Msg::FindSearchingIn, L"");

				FARString strFM;
				itd.GetFindMessage(strFM);
				SMALL_RECT Rect;
				SendDlgMessage(hDlg, DM_GETITEMPOSITION, FD_TEXT_STATUS, reinterpret_cast<LONG_PTR>(&Rect));
				TruncStrFromCenter(strFM,
						Rect.Right - Rect.Left + 1 - static_cast<int>(strSearchStr.GetLength()) - 1);
				strDataStr = strSearchStr + L" " + strFM;
				SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_TEXT_STATUS,
						reinterpret_cast<LONG_PTR>(strDataStr.CPtr()));

				strDataStr.Format(L"%3d%%", itd.GetPercent());
				SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_TEXT_STATUS_PERCENTS,
						reinterpret_cast<LONG_PTR>(strDataStr.CPtr()));

				if (itd.GetLastFoundNumber()) {
					itd.SetLastFoundNumber(0);

					if (ListBox->UpdateRequired())
						SendDlgMessage(hDlg, DM_SHOWITEM, 1, 1);
				}
			}
		}
		Recurse = false;
	}

	if (!v->Finalized && StopFlag) {
		v->Finalized = true;
		FARString strMessage;
		strMessage.Format(Msg::FindDone, itd.GetFileCount(), itd.GetDirCount());
		SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
		SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_SEPARATOR1, reinterpret_cast<LONG_PTR>(L""));
		SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_TEXT_STATUS, reinterpret_cast<LONG_PTR>(strMessage.CPtr()));
		SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_TEXT_STATUS_PERCENTS, reinterpret_cast<LONG_PTR>(L""));
		SendDlgMessage(hDlg, DM_SETTEXTPTR, FD_BUTTON_STOP,
				reinterpret_cast<LONG_PTR>(Msg::FindCancel.CPtr()));
		SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
		ConsoleTitle::SetFarTitle(strMessage);
		if (Opt.NotifOpt.OnFileOperation) {
			DisplayNotification(Msg::FileOperationComplete, strMessage);
		}
	}

	switch (Msg) {
		case DN_DRAWDIALOGDONE: {
			DefDlgProc(hDlg, Msg, Param1, Param2);

			// Переместим фокус на кнопку [Go To]
			if ((itd.GetDirCount() || itd.GetFileCount()) && !v->FindPositionChanged) {
				v->FindPositionChanged = true;
				SendDlgMessage(hDlg, DM_SETFOCUS, FD_BUTTON_GOTO, 0);
			}
			return TRUE;
		} break;

		case DN_KEY: {
			switch (Param2) {
				case KEY_ESC:
				case KEY_F10: {
					if (!StopFlag) {
						PauseFlag = true;
						bool LocalRes = true;
						if (Opt.Confirm.Esc)
							LocalRes = AbortMessage() != 0;
						PauseFlag = false;
						if (LocalRes) {
							StopFlag = true;
						}
						return TRUE;
					}
				} break;

				case KEY_CTRLALTSHIFTPRESS:
				case KEY_ALTF9:
				case KEY_F11:
				case KEY_CTRLW: {
					FrameManager->ProcessKey((DWORD)Param2);
					return TRUE;
				} break;

				case KEY_RIGHT:
				case KEY_NUMPAD6:
				case KEY_TAB: {
					if (Param1 == FD_BUTTON_STOP) {
						v->FindPositionChanged = true;
						SendDlgMessage(hDlg, DM_SETFOCUS, FD_BUTTON_NEW, 0);
						return TRUE;
					}
				} break;

				case KEY_LEFT:
				case KEY_NUMPAD4:
				case KEY_SHIFTTAB: {
					if (Param1 == FD_BUTTON_NEW) {
						v->FindPositionChanged = true;
						SendDlgMessage(hDlg, DM_SETFOCUS, FD_BUTTON_STOP, 0);
						return TRUE;
					}
				} break;

				case KEY_UP:
				case KEY_DOWN:
				case KEY_NUMPAD8:
				case KEY_NUMPAD2:
				case KEY_PGUP:
				case KEY_PGDN:
				case KEY_NUMPAD9:
				case KEY_NUMPAD3:
				case KEY_HOME:
				case KEY_END:
				case KEY_NUMPAD7:
				case KEY_NUMPAD1:
				case KEY_MSWHEEL_UP:
				case KEY_MSWHEEL_DOWN:
				case KEY_ALTLEFT:
				case KEY_ALT | KEY_NUMPAD4:
				case KEY_MSWHEEL_LEFT:
				case KEY_ALTRIGHT:
				case KEY_ALT | KEY_NUMPAD6:
				case KEY_MSWHEEL_RIGHT:
				case KEY_ALTSHIFTLEFT:
				case KEY_ALT | KEY_SHIFT | KEY_NUMPAD4:
				case KEY_ALTSHIFTRIGHT:
				case KEY_ALT | KEY_SHIFT | KEY_NUMPAD6:
				case KEY_ALTHOME:
				case KEY_ALT | KEY_NUMPAD7:
				case KEY_ALTEND:
				case KEY_ALT | KEY_NUMPAD1: {
					ListBox->ProcessKey((int)Param2);
					return TRUE;
				} break;

					/*
					case KEY_CTRLA:
					{
						if (!ListBox->GetItemCount())
						{
							return TRUE;
						}

						size_t ItemIndex = reinterpret_cast<size_t>(ListBox->GetUserData(nullptr,0));

						FINDLIST FindItem;
						itd.GetFindListItem(ItemIndex, FindItem);

						if (ShellSetFileAttributes(NULL,FindItem.FindData.strFileName))
						{
							itd.SetFindListItem(ItemIndex, FindItem);
							SendDlgMessage(hDlg,DM_REDRAW,0,0);
						}
						return TRUE;
					}
					*/

				case KEY_F3:
				case KEY_NUMPAD5:
				case KEY_SHIFTNUMPAD5:
				case KEY_F4: {
					if (!ListBox->GetItemCount()) {
						return TRUE;
					}

					size_t ItemIndex = reinterpret_cast<size_t>(ListBox->GetUserData(nullptr, 0));
					bool RemoveTemp = false;
					// Плагины надо закрывать, если открыли.
					bool ClosePlugin = false;
					FARString strSearchFileName;
					FARString strTempDir;

					FINDLIST FindItem;
					itd.GetFindListItem(ItemIndex, FindItem);
					if (FindItem.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						return TRUE;
					}

					// FindFileArcIndex нельзя здесь использовать
					// Он может быть уже другой.
					if (FindItem.ArcIndex != LIST_INDEX_NONE) {
						ARCLIST ArcItem;
						itd.GetArcListItem(FindItem.ArcIndex, ArcItem);

						if (!(ArcItem.Flags & OPIF_REALNAMES)) {
							FARString strFindArcName = ArcItem.strArcName;
							if (ArcItem.hPlugin == nullptr) {
								int SavePluginsOutput = DisablePluginsOutput;
								DisablePluginsOutput = TRUE;
								{
									SCOPED_ACTION(PluginLocker);
									ArcItem.hPlugin = CtrlObject->Plugins.OpenFilePlugin(strFindArcName,
											OPM_FIND, OFP_SEARCH);
								}
								itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
								DisablePluginsOutput = SavePluginsOutput;

								if (ArcItem.hPlugin == PHPTR_STOP || ArcItem.hPlugin == nullptr) {
									ArcItem.hPlugin = nullptr;
									itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
									return TRUE;
								}

								ClosePlugin = true;
							}
							FarMkTempEx(strTempDir);
							apiCreateDirectory(strTempDir, nullptr);
							SCOPED_ACTION(PluginLocker);
							bool bGet = GetPluginFile(FindItem.ArcIndex, FindItem.FindData, strTempDir,
									strSearchFileName);
							itd.SetFindListItem(ItemIndex, FindItem);
							if (!bGet) {
								fprintf(stderr, "%s: GetPluginFile failed for '%ls'\n", __FUNCTION__,
										FindItem.FindData.strFileName.CPtr());
								apiRemoveDirectory(strTempDir);

								if (ClosePlugin) {
									CtrlObject->Plugins.ClosePanel(ArcItem.hPlugin);
									ArcItem.hPlugin = nullptr;
									itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
								}
								return FALSE;
							} else {
								if (ClosePlugin) {
									CtrlObject->Plugins.ClosePanel(ArcItem.hPlugin);
									ArcItem.hPlugin = nullptr;
									itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
								}
							}
							RemoveTemp = true;
						} else
							strSearchFileName = FindItem.FindData.strFileName;
					} else
						strSearchFileName = FindItem.FindData.strFileName;

					DWORD FileAttr = apiGetFileAttributes(strSearchFileName);

					if (FileAttr != INVALID_FILE_ATTRIBUTES) {
						FARString strOldTitle;
						Console.GetTitle(strOldTitle);

						if (Param2 == KEY_F3 || Param2 == KEY_NUMPAD5 || Param2 == KEY_SHIFTNUMPAD5) {
							int ListSize = ListBox->GetItemCount();
							NamesList ViewList;

							// Возьмем все файлы, которые имеют реальные имена...
							if (Opt.FindOpt.CollectFiles) {
								for (int I = 0; I < ListSize; I++) {
									FINDLIST FindItem;
									itd.GetFindListItem(reinterpret_cast<size_t>(
																ListBox->GetUserData(nullptr, 0, I)),
											FindItem);

									bool RealNames = true;
									if (FindItem.ArcIndex != LIST_INDEX_NONE) {
										ARCLIST ArcItem;
										itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
										if (!(ArcItem.Flags & OPIF_REALNAMES)) {
											RealNames = false;
										}
									}

									if (RealNames) {
										if (!FindItem.FindData.strFileName.IsEmpty()
												&& !(FindItem.FindData.dwFileAttributes
														& FILE_ATTRIBUTE_DIRECTORY))
											ViewList.AddName(FindItem.FindData.strFileName);
									}
								}

								FARString strCurDir = FindItem.FindData.strFileName;
								ViewList.SetCurName(strCurDir);
							}

							SendDlgMessage(hDlg, DM_SHOWDIALOG, FALSE, 0);
							SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
							{
								FileViewer ShellViewer(strSearchFileName, FALSE, FALSE, FALSE, -1, nullptr,
										(FindItem.ArcIndex != LIST_INDEX_NONE)
												? nullptr
												: (Opt.FindOpt.CollectFiles ? &ViewList : nullptr));
								ShellViewer.SetDynamicallyBorn(false);
								ShellViewer.SetEnableF6(TRUE);

								// FindFileArcIndex нельзя здесь использовать
								// Он может быть уже другой.
								if (FindItem.ArcIndex != LIST_INDEX_NONE) {
									ARCLIST ArcItem;
									itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
									if (!(ArcItem.Flags & OPIF_REALNAMES)) {
										ShellViewer.SetSaveToSaveAs(true);
									}
								}
								FrameManager->ExecuteModalEV(true);
							}
							SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
							SendDlgMessage(hDlg, DM_SHOWDIALOG, TRUE, 0);
						} else {
							SendDlgMessage(hDlg, DM_SHOWDIALOG, FALSE, 0);
							SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
							{
								/* $ 14.08.2002 VVM
								  ! Пока-что запретим из поиска переключаться в активный редактор.
									К сожалению, манагер на это не способен сейчас
															int FramePos=FrameManager->FindFrameByFile(MODALTYPE_EDITOR,SearchFileName);
															int SwitchTo=FALSE;
															if (FramePos!=-1)
															{
																if (!(*FrameManager)[FramePos]->GetCanLoseFocus(true) ||
																	Opt.Confirm.AllowReedit)
																{
																	char MsgFullFileName[NM];
																	far_strncpy(MsgFullFileName,SearchFileName,sizeof(MsgFullFileName));
																	int MsgCode=Message(0,2,Msg::FindFileTitle,
																				TruncPathStr(MsgFullFileName,ScrX-16),
																				Msg::AskReload,
																				Msg::Current,Msg::NewOpen);
																	if (!MsgCode)
																	{
																		SwitchTo=TRUE;
																	}
																	else if (MsgCode==1)
																	{
																		SwitchTo=FALSE;
																	}
																	else
																	{
																		SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
																		SendDlgMessage(hDlg,DM_SHOWDIALOG,TRUE,0);
																		return TRUE;
																	}
																}
																else
																{
																	SwitchTo=TRUE;
																}
															}
															if (SwitchTo)
															{
																(*FrameManager)[FramePos]->SetCanLoseFocus(false);
																(*FrameManager)[FramePos]->SetDynamicallyBorn(false);
																FrameManager->ActivateFrame(FramePos);
																FrameManager->ExecuteModalEV ();
																// FrameManager->ExecuteNonModal();
																// заставляем рефрешиться экран
																FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
															}
															else
								*/
								{
									std::shared_ptr<FindDlg_TempFileHolder> TFH;
									FileEditor ShellEditor(strSearchFileName, CP_AUTODETECT, 0);
									ShellEditor.SetDynamicallyBorn(false);
									ShellEditor.SetEnableF6(TRUE);

									// FindFileArcIndex нельзя здесь использовать
									// Он может быть уже другой.
									if (FindItem.ArcIndex != LIST_INDEX_NONE) {
										ARCLIST Item;
										itd.GetArcListItem(FindItem.ArcIndex, Item);
										if (0 == (Item.Flags & OPIF_REALNAMES)) { // see https://github.com/elfmz/far2l/issues/2223
											TFH = std::make_shared<FindDlg_TempFileHolder>(strSearchFileName,
													FindItem.ArcIndex, FindItem.FindData);
											ShellEditor.SetFileHolder(TFH);
										}
									}
									FrameManager->ExecuteModalEV(false);
									if (TFH) {
										TFH->CheckForChanges();
									}
									// заставляем рефрешиться экран
									FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
								}
							}
							SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
							SendDlgMessage(hDlg, DM_SHOWDIALOG, TRUE, 0);
						}
						Console.SetTitle(strOldTitle);
					}
					if (RemoveTemp) {
						DeleteFileWithFolder(strSearchFileName);
					}
					return TRUE;
				} break;
			}
		} break;

		case DN_BTNCLICK: {
			v->FindPositionChanged = true;
			switch (Param1) {
				case FD_BUTTON_NEW: {
					StopFlag = true;
					return FALSE;
				} break;

				case FD_BUTTON_STOP: {
					if (!StopFlag) {
						StopFlag = true;
						return TRUE;
					} else {
						return FALSE;
					}
				} break;

				case FD_BUTTON_VIEW: {
					FindDlgProc(hDlg, DN_KEY, FD_LISTBOX, KEY_F3);
					return TRUE;
				} break;

				case FD_BUTTON_EDIT: {
					FindDlgProc(hDlg, DN_KEY, FD_LISTBOX, KEY_F4);
					return TRUE;
				} break;

				case FD_BUTTON_GOTO:
				case FD_BUTTON_PANEL: {
					// Переход и посыл на панель будем делать не в диалоге, а после окончания поиска.
					// Иначе возможна ситуация, когда мы ищем на панели, потом ее грохаем и создаем новую
					// (а поиск-то идет!) и в результате ФАР трапается.
					if (!ListBox->GetItemCount()) {
						return TRUE;
					}
					v->FindExitIndex =
							static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(ListBox->GetUserData(nullptr, 0)));
					return FALSE;
				} break;
			}
		} break;

		case DN_CLOSE: {
			BOOL Result = TRUE;
			if (Param1 == FD_LISTBOX) {
				if (ListBox->GetItemCount()) {
					FindDlgProc(hDlg, DN_BTNCLICK, FD_BUTTON_GOTO, 0);    // emulates a [ Go to ] button pressing;
				} else {
					Result = FALSE;
				}
			}
			if (Result) {
				StopFlag = true;
			}
			return Result;
		} break;

		case DN_RESIZECONSOLE: {
			PCOORD pCoord = reinterpret_cast<PCOORD>(Param2);
			SMALL_RECT DlgRect;
			SendDlgMessage(hDlg, DM_GETDLGRECT, 0, reinterpret_cast<LONG_PTR>(&DlgRect));
			int DlgWidth = DlgRect.Right - DlgRect.Left + 1;
			int DlgHeight = DlgRect.Bottom - DlgRect.Top + 1;
			int IncX = pCoord->X - DlgWidth - 2;
			int IncY = pCoord->Y - DlgHeight - 2;
			SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);

			for (int i = 0; i <= FD_BUTTON_STOP; i++) {
				SendDlgMessage(hDlg, DM_SHOWITEM, i, FALSE);
			}

			if ((IncX > 0) || (IncY > 0)) {
				pCoord->X = DlgWidth + (IncX > 0 ? IncX : 0);
				pCoord->Y = DlgHeight + (IncY > 0 ? IncY : 0);
				SendDlgMessage(hDlg, DM_RESIZEDIALOG, 0, reinterpret_cast<LONG_PTR>(pCoord));
			}

			DlgWidth+= IncX;
			DlgHeight+= IncY;

			for (int i = 0; i < FD_SEPARATOR1; i++) {
				SMALL_RECT rect;
				SendDlgMessage(hDlg, DM_GETITEMPOSITION, i, reinterpret_cast<LONG_PTR>(&rect));
				rect.Right+= IncX;
				rect.Bottom+= IncY;
				SendDlgMessage(hDlg, DM_SETITEMPOSITION, i, reinterpret_cast<LONG_PTR>(&rect));
			}

			for (int i = FD_SEPARATOR1; i <= FD_BUTTON_STOP; i++) {
				SMALL_RECT rect;
				SendDlgMessage(hDlg, DM_GETITEMPOSITION, i, reinterpret_cast<LONG_PTR>(&rect));

				if (i == FD_TEXT_STATUS) {
					rect.Right+= IncX;
				} else if (i == FD_TEXT_STATUS_PERCENTS) {
					rect.Right+= IncX;
					rect.Left+= IncX;
				}

				rect.Top+= IncY;
				SendDlgMessage(hDlg, DM_SETITEMPOSITION, i, reinterpret_cast<LONG_PTR>(&rect));
			}

			if ((IncX <= 0) || (IncY <= 0)) {
				pCoord->X = DlgWidth;
				pCoord->Y = DlgHeight;
				SendDlgMessage(hDlg, DM_RESIZEDIALOG, 0, reinterpret_cast<LONG_PTR>(pCoord));
			}

			for (int i = 0; i <= FD_BUTTON_STOP; i++) {
				SendDlgMessage(hDlg, DM_SHOWITEM, i, TRUE);
			}

			SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
			return TRUE;
		} break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static void
AddMenuRecord(HANDLE hDlg, const wchar_t *FullName, const FAR_FIND_DATA_EX &FindData, size_t ArcIndex)
{
	if (!hDlg)
		return;

	VMenu *ListBox = reinterpret_cast<Dialog *>(hDlg)->GetAllItem()[FD_LISTBOX]->ListPtr;

	if (!ListBox->GetItemCount()) {
		SendDlgMessage(hDlg, DM_ENABLE, FD_BUTTON_GOTO, TRUE);
		SendDlgMessage(hDlg, DM_ENABLE, FD_BUTTON_VIEW, TRUE);
		SendDlgMessage(hDlg, DM_ENABLE, FD_BUTTON_EDIT, TRUE);
		if (AnySetFindList) {
			SendDlgMessage(hDlg, DM_ENABLE, FD_BUTTON_PANEL, TRUE);
		}
		SendDlgMessage(hDlg, DM_ENABLE, FD_LISTBOX, TRUE);
	}

	MenuItemEx ListItem = {};

	FormatString MenuText;

	FARString strDateStr, strTimeStr;
	const wchar_t *DisplayName = FindData.strFileName;

	MenuText << L' ';

	for (size_t Count = 0; Count < Opt.FindOpt.OutColumns.size(); ++Count) {
		const Column &CurColumn = Opt.FindOpt.OutColumns[Count];
		unsigned int CurColumnType = CurColumn.Type & 0xFF;

		switch (CurColumnType) {
			case DIZ_COLUMN:
			case OWNER_COLUMN:
			case GROUP_COLUMN: {
				// пропускаем, не реализовано
				break;
			}
			case NAME_COLUMN: {
				// даже если указали, пропускаем, т.к. поле имени обязательное и идет в конце.
				break;
			}

			case ATTR_COLUMN: {
				MenuText << FormatStr_Attribute(FindData.dwFileAttributes, FindData.dwUnixMode)
						 << BoxSymbols[BS_V1];
				break;
			}
			case SIZE_COLUMN:
			case PHYSICAL_COLUMN:
			case NUMLINK_COLUMN: {
				MenuText << FormatStr_Size(FindData.nFileSize, FindData.nPhysicalSize, DisplayName,
						FindData.dwFileAttributes, 0, CurColumnType, CurColumn.Type, CurColumn.Width);

				MenuText << BoxSymbols[BS_V1];
				break;
			}

			case DATE_COLUMN:
			case TIME_COLUMN:
			case WDATE_COLUMN:
			case ADATE_COLUMN:
			case CDATE_COLUMN:
			case CHDATE_COLUMN: {
				const FILETIME *FileTime;
				switch (CurColumnType) {
					case CDATE_COLUMN:
						FileTime = &FindData.ftCreationTime;
						break;
					case ADATE_COLUMN:
						FileTime = &FindData.ftLastAccessTime;
						break;
					case CHDATE_COLUMN:
						FileTime = &FindData.ftChangeTime;
						break;
					case DATE_COLUMN:
					case TIME_COLUMN:
					case WDATE_COLUMN:
					default:
						FileTime = &FindData.ftLastWriteTime;
						break;
				}

				MenuText << FormatStr_DateTime(FileTime, CurColumnType, CurColumn.Type, CurColumn.Width)
						 << BoxSymbols[BS_V1];
				break;
			}
		}
	}

	// В плагинах принудительно поставим указатель в имени на имя
	// для корректного его отображения в списке, отбросив путь,
	// т.к. некоторые плагины возвращают имя вместе с полным путём,
	// к примеру временная панель.

	const wchar_t *DisplayName0 = DisplayName;
	if (ArcIndex != LIST_INDEX_NONE)    // itd.GetFindFileArcIndex()
		DisplayName0 = PointToName(DisplayName0);
	MenuText << DisplayName0;

	FARString strPathName = FullName;
	{
		size_t pos;

		if (FindLastSlash(pos, strPathName))
			strPathName.Truncate(pos);
		else
			strPathName.Clear();
	}
	AddEndSlash(strPathName);

	if (StrCmp(strPathName, strLastDirName)) {
		if (ListBox->GetItemCount()) {
			ListItem.Flags|= LIF_SEPARATOR;
			ListBox->AddItem(&ListItem);
			ListItem.Flags&= ~LIF_SEPARATOR;
		}

		strLastDirName = strPathName;

		if (ArcIndex != LIST_INDEX_NONE)    // itd.GetFindFileArcIndex()
		{
			ARCLIST ArcItem;
			itd.GetArcListItem(ArcIndex, ArcItem);    // itd.GetFindFileArcIndex()
			if (!(ArcItem.Flags & OPIF_REALNAMES) && !ArcItem.strArcName.IsEmpty()) {
				FARString strArcPathName = ArcItem.strArcName;
				strArcPathName+= L":";

				if (!IsSlash(strPathName.At(0)))
					AddEndSlash(strArcPathName);

				strArcPathName+= (!StrCmp(strPathName, L"./") ? L"/" : strPathName.CPtr());
				strPathName = strArcPathName;
			}
		}
		ListItem.strName = strPathName;
		size_t ItemIndex = itd.AddFindListItem(FindData);

		if (ItemIndex != LIST_INDEX_NONE) {
			// Сбросим данные в FindData. Они там от файла
			FINDLIST FindItem;
			itd.GetFindListItem(ItemIndex, FindItem);
			FindItem.FindData.Clear();
			// Используем LastDirName, т.к. PathName уже может быть искажена
			FindItem.FindData.strFileName = strLastDirName;
			// Used=0 - Имя не попадёт во временную панель.
			FindItem.Used = 0;
			// Поставим атрибут у каталога, что-бы он не был файлом :)
			FindItem.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

			// size_t ArcIndex=itd.GetFindFileArcIndex();
			if (ArcIndex != LIST_INDEX_NONE) {
				FindItem.ArcIndex = ArcIndex;
			}
			itd.SetFindListItem(ItemIndex, FindItem);
			ListBox->SetUserData((void *)(DWORD_PTR)ItemIndex, sizeof(ItemIndex),
					ListBox->AddItem(&ListItem));
		}
	}

	size_t ItemIndex = itd.AddFindListItem(FindData);

	if (ItemIndex != LIST_INDEX_NONE) {
		FINDLIST FindItem;
		itd.GetFindListItem(ItemIndex, FindItem);
		FindItem.FindData.strFileName = FullName;
		FindItem.Used = 1;
		// size_t ArcIndex=itd.GetFindFileArcIndex();
		if (ArcIndex != LIST_INDEX_NONE)
			FindItem.ArcIndex = ArcIndex;
		itd.SetFindListItem(ItemIndex, FindItem);
	}

	ListItem.strName = MenuText.strValue();
	int ListPos = ListBox->AddItem(&ListItem);
	ListBox->SetUserData((void *)(DWORD_PTR)ItemIndex, sizeof(ItemIndex), ListPos);

	// Выделим как положено - в списке.
	int FC = itd.GetFileCount(), DC = itd.GetDirCount(), LF = itd.GetLastFoundNumber();
	if (!FC && !DC) {
		ListBox->SetSelectPos(ListPos, -1);
	}

	if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		DC++;
	} else {
		FC++;
	}

	LF++;

	itd.SetFileCount(FC);
	itd.SetDirCount(DC);
	itd.SetLastFoundNumber(LF);
}

static void DoPreparePluginList(HANDLE hDlg);

static void ArchiveSearch(HANDLE hDlg, const wchar_t *ArcName)
{
	_ALGO(CleverSysLog clv(L"FindFiles::ArchiveSearch()"));
	_ALGO(SysLog(L"ArcName='%ls'", (ArcName ? ArcName : L"nullptr")));

	FARString strArcName = ArcName;
	PHPTR hArc;
	{
		SCOPED_ACTION(PluginLocker);
		int SavePluginsOutput = DisablePluginsOutput;
		DisablePluginsOutput = TRUE;
		hArc = CtrlObject->Plugins.OpenFilePlugin(strArcName, OPM_FIND, OFP_SEARCH);
		DisablePluginsOutput = SavePluginsOutput;
	}

	if (hArc == PHPTR_STOP) {
		StopFlag = true;
		_ALGO(SysLog(L"return: hArc==(HANDLE)-2"));
		return;
	}

	if (hArc == nullptr) {
		_ALGO(SysLog(L"return: hArc==nullptr"));
		return;
	}

	int SaveSearchMode = SearchMode;
	size_t SaveArcIndex = itd.GetFindFileArcIndex();
	{
		SearchMode = FINDAREA_FROM_CURRENT;
		OpenPluginInfo Info;
		{
			SCOPED_ACTION(PluginLocker);
			int SavePluginsOutput = DisablePluginsOutput;
			DisablePluginsOutput = TRUE;
			CtrlObject->Plugins.GetOpenPluginInfo(hArc, &Info);
			itd.SetFindFileArcIndex(itd.AddArcListItem(ArcName, hArc, Info.Flags, Info.CurDir));
			DisablePluginsOutput = SavePluginsOutput;
		}

		// Запомним каталог перед поиском в архиве. И если ничего не нашли - не рисуем его снова.
		{
			FARString strSaveDirName, strSaveSearchPath;
			size_t SaveListCount = itd.GetFindListCount();
			// Запомним пути поиска в плагине, они могут измениться.
			strSaveSearchPath = strPluginSearchPath;
			strSaveDirName = strLastDirName;
			strLastDirName.Clear();
			DoPreparePluginList(hDlg);
			strPluginSearchPath = strSaveSearchPath;
			ARCLIST ArcItem;
			itd.GetArcListItem(itd.GetFindFileArcIndex(), ArcItem);
			{
				SCOPED_ACTION(PluginLocker);
				CtrlObject->Plugins.ClosePanel(ArcItem.hPlugin);
			}
			ArcItem.hPlugin = nullptr;
			itd.SetArcListItem(itd.GetFindFileArcIndex(), ArcItem);

			if (SaveListCount == itd.GetFindListCount())
				strLastDirName = strSaveDirName;
		}

		//		DisablePluginsOutput=SavePluginsOutput;
	}
	itd.SetFindFileArcIndex(SaveArcIndex);
	SearchMode = SaveSearchMode;
}

static void DoScanTree(HANDLE hDlg, FARString &strRoot)
{
	ScanTree ScTree(FALSE, !(SearchMode == FINDAREA_CURRENT_ONLY || SearchMode == FINDAREA_INPATH),
			Opt.FindOpt.FindSymLinks);
	FARString strSelName;
	DWORD FileAttr;

	if (SearchMode == FINDAREA_SELECTED)
		CtrlObject->Cp()->ActivePanel->GetSelNameCompat(nullptr, FileAttr);

	while (!StopFlag) {
		FARString strCurRoot;

		if (SearchMode == FINDAREA_SELECTED) {
			if (!CtrlObject->Cp()->ActivePanel->GetSelNameCompat(&strSelName, FileAttr))
				break;

			if (!(FileAttr & FILE_ATTRIBUTE_DIRECTORY) || TestParentFolderName(strSelName)
					|| !StrCmp(strSelName, L"."))
				continue;

			strCurRoot = strRoot;
			AddEndSlash(strCurRoot);
			strCurRoot+= strSelName;
		} else {
			strCurRoot = strRoot;
		}

		ScTree.SetFindPath(strCurRoot, L"*");
		itd.SetFindMessage(strCurRoot);
		FAR_FIND_DATA_EX FindData;
		FARString strFullName;

		while (!StopFlag && ScTree.GetNextName(&FindData, strFullName)) {
			// WINPORT(Sleep)(0);
			while (PauseFlag)
				WINPORT(Sleep)(10);

			bool bContinue = false;

			while (!StopFlag) {
				if (UseFilter) {
					enumFileInFilterType foundType;

					if (!Filter->FileInFilter(FindData, &foundType)) {
						// сюда заходим, если не попали в фильтр или попали в Exclude-фильтр
						if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
								&& foundType == FIFT_EXCLUDE)
							ScTree.SkipDir();    // скипаем только по Exclude-фильтру, т.к. глубже тоже нужно просмотреть

						{
							bContinue = true;
							break;
						}
					}
				}

				if (((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strFindStr.IsEmpty())
						|| (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
								&& !strFindStr.IsEmpty())) {
					itd.SetFindMessage(strFullName);
				}

				if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || Opt.FindOpt.FindSymLinks) {
					AnalyzeFileItem(hDlg, nullptr, strFullName, FindData);
					// AddMenuRecord(hDlg,strFullStreamName, FindData);
				}

				break;
			}

			if (bContinue) {
				continue;
			}

			if (SearchInArchives)
				ArchiveSearch(hDlg, strFullName);
		}

		if (SearchMode != FINDAREA_SELECTED)
			break;
	}
}

static void ScanPluginTree(HANDLE hDlg, PHPTR hPlugin, DWORD Flags, int &RecurseLevel)
{
	PluginPanelItem *PanelData = nullptr;
	int ItemCount = 0;
	bool GetFindDataResult = false;
	{
		if (!StopFlag) {
			SCOPED_ACTION(PluginLocker);
			GetFindDataResult =
					CtrlObject->Plugins.GetFindData(hPlugin, &PanelData, &ItemCount, OPM_FIND) != FALSE;
		}
	}
	if (!GetFindDataResult) {
		return;
	}

	RecurseLevel++;

	if (SearchMode != FINDAREA_SELECTED || RecurseLevel != 1) {
		for (int I = 0; I < ItemCount && !StopFlag; I++) {
			// WINPORT(Sleep)(0);
			while (PauseFlag)
				WINPORT(Sleep)(10);

			PluginPanelItem *CurPanelItem = PanelData + I;
			FARString strCurName = CurPanelItem->FindData.lpwszFileName;

			if (!StrCmp(strCurName, L".") || TestParentFolderName(strCurName))
				continue;

			if (!UseFilter || Filter->FileInFilter(CurPanelItem->FindData)) {
				if (((CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							&& strFindStr.IsEmpty())
						|| (!(CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
								&& !strFindStr.IsEmpty())) {
					itd.SetFindMessage(strPluginSearchPath + strCurName);
				}

				AnalyzeFileItem(hDlg, CurPanelItem, strCurName, CurPanelItem->FindData);

				if (SearchInArchives && hPlugin && (Flags & OPIF_REALNAMES))
					ArchiveSearch(hDlg, strPluginSearchPath + strCurName);
			}
		}
	}

	if (SearchMode != FINDAREA_CURRENT_ONLY) {
		for (int I = 0; I < ItemCount && !StopFlag; I++) {
			PluginPanelItem *CurPanelItem = PanelData + I;
			FARString strCurName = CurPanelItem->FindData.lpwszFileName;

			if ((CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					&& StrCmp(strCurName, L".") && !TestParentFolderName(strCurName)
					&& (!UseFilter || Filter->FileInFilter(CurPanelItem->FindData))
					&& (SearchMode != FINDAREA_SELECTED || RecurseLevel != 1
							|| CtrlObject->Cp()->ActivePanel->IsSelected(strCurName))) {
				bool SetDirectoryResult = false;
				{
					SCOPED_ACTION(PluginLocker);
					SetDirectoryResult =
							CtrlObject->Plugins.SetDirectory(hPlugin, strCurName, OPM_FIND) != FALSE;
				}
				if (SetDirectoryResult) {
					strPluginSearchPath+= strCurName;
					strPluginSearchPath+= L"/";
					ScanPluginTree(hDlg, hPlugin, Flags, RecurseLevel);

					size_t pos = 0;
					if (strPluginSearchPath.RPos(pos, GOOD_SLASH))
						strPluginSearchPath.Truncate(pos);

					if (strPluginSearchPath.RPos(pos, GOOD_SLASH))
						strPluginSearchPath.Truncate(pos + 1);
					else
						strPluginSearchPath.Clear();

					bool SetDirectoryResult = false;
					{
						SCOPED_ACTION(PluginLocker);
						SetDirectoryResult =
								CtrlObject->Plugins.SetDirectory(hPlugin, L"..", OPM_FIND) != FALSE;
					}
					if (!SetDirectoryResult) {
						StopFlag = true;
					}
				}
			}
		}
	}

	{
		SCOPED_ACTION(PluginLocker);
		CtrlObject->Plugins.FreeFindData(hPlugin, PanelData, ItemCount);
	}
	RecurseLevel--;
}

static void DoPrepareFileList(HANDLE hDlg)
{
	ThreadedWorkQueuePtrScope wqs(pWorkQueue);
	FARString strRoot = CtrlObject->CmdLine->GetCurDir();

	UserDefinedList List(ULF_UNIQUE, L";");

	if (SearchMode == FINDAREA_INPATH) {
		FARString strPathEnv;
		apiGetEnvironmentVariable(L"PATH", strPathEnv);
		size_t pos;
		while (strPathEnv.Pos(pos, L':')) {
			strPathEnv.Replace(pos, 1, L';');
		}
		List.Set(strPathEnv);
	} else if (SearchMode == FINDAREA_ROOT) {
		strRoot = L"/";
		List.Set(strRoot);
	} else if (SearchMode == FINDAREA_ALL || SearchMode == FINDAREA_ALL_BUTNETWORK) {
		List.AddItem(L"/");
	} else {
		List.Set(strRoot);
	}

	const wchar_t *pwRoot;
	for (size_t LI = 0; (pwRoot = List.Get(LI)) != nullptr; ++LI) {
		strRoot = pwRoot;
		DoScanTree(hDlg, strRoot);
	}
}

static void DoPreparePluginList(HANDLE hDlg)
{
	ThreadedWorkQueuePtrScope wqs(pWorkQueue);
	ARCLIST ArcItem;
	itd.GetArcListItem(itd.GetFindFileArcIndex(), ArcItem);
	OpenPluginInfo Info;
	FARString strSaveDir;
	{
		SCOPED_ACTION(PluginLocker);
		CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin, &Info);
		strSaveDir = Info.CurDir;
		if (SearchMode == FINDAREA_ROOT || SearchMode == FINDAREA_ALL || SearchMode == FINDAREA_ALL_BUTNETWORK
				|| SearchMode == FINDAREA_INPATH) {
			CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin, L"/", OPM_FIND);
			CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin, &Info);
		}
	}

	strPluginSearchPath = Info.CurDir;

	if (!strPluginSearchPath.IsEmpty())
		AddEndSlash(strPluginSearchPath);

	int RecurseLevel = 0;
	ScanPluginTree(hDlg, ArcItem.hPlugin, ArcItem.Flags, RecurseLevel);

	if (SearchMode == FINDAREA_ROOT || SearchMode == FINDAREA_ALL || SearchMode == FINDAREA_ALL_BUTNETWORK
			|| SearchMode == FINDAREA_INPATH) {
		SCOPED_ACTION(PluginLocker);
		CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin, strSaveDir, OPM_FIND);
	}
}

class FindFileThread : public Threaded
{
	bool PluginMode;
	HANDLE hDlg;
	bool bDone = false;

public:
	using Threaded::StartThread;

	bool CheckForDone()
	{
		if (!bDone)
			return false;

		WaitThread();
		return true;
	}

	FindFileThread(bool PluginMode_, HANDLE hDlg_)
		:
		PluginMode(PluginMode_), hDlg(hDlg_)
	{}

	virtual void *ThreadProc()
	{
		InitInFileSearch();
		{
			SudoClientRegion scr;
			DWORD msec = GetProcessUptimeMSec();
			pMountInfo.reset(new MountInfo);
			if (PluginMode) {
				DoPreparePluginList(hDlg);
			} else {
				DoPrepareFileList(hDlg);
			}
			msec = GetProcessUptimeMSec() - msec;
			fprintf(stderr, "FindFiles complete in %u msec\n", msec);
			itd.SetPercent(0);
			StopFlag = true;
			pMountInfo.reset();
		}
		ReleaseInFileSearch();

		InterThreadLockAndWake itlw;
		bDone = true;
		return nullptr;
	}
};

static bool FindFilesProcess(Vars &v)
{
	_ALGO(CleverSysLog clv(L"FindFiles::FindFilesProcess()"));
	// Если используется фильтр операций, то во время поиска сообщаем об этом
	FARString strTitle(Msg::FindFileTitle);
	FARString strSearchStr;

	itd.Init();

	if (!strFindMask.IsEmpty()) {
		strTitle+= L": ";
		strTitle+= strFindMask;

		if (UseFilter) {
			strTitle+= L" (";
			strTitle+= Msg::FindUsingFilter;
			strTitle+= L")";
		}
	} else {
		if (UseFilter) {
			strTitle+= L" (";
			strTitle+= Msg::FindUsingFilter;
			strTitle+= L")";
		}
	}

	if (!strFindStr.IsEmpty()) {
		FARString strFStr = strFindStr;
		TruncStrFromEnd(strFStr, 10);
		InsertQuote(strFStr);
		FARString strTemp = L" ";
		strTemp+= strFStr;
		strSearchStr.Format(Msg::FindSearchingIn, strTemp.CPtr());
	} else {
		strSearchStr.Format(Msg::FindSearchingIn, L"");
	}

	int DlgWidth = ScrX + 1 - 2;
	int DlgHeight = ScrY + 1 - 2;
	DialogDataEx FindDlgData[] = {
			{DI_DOUBLEBOX, 3,                     1,                      (short)(DlgWidth - 4),                               (short)(DlgHeight - 2), {}, DIF_SHOWAMPERSAND,
             strTitle                                                                                                                                                                                                 },
			{DI_LISTBOX,   4,                     2,                      (short)(DlgWidth - 5),                               (short)(DlgHeight - 7), {}, DIF_LISTNOBOX | DIF_DISABLE,
             L""                                                                                                                                                                                                      },
			{DI_TEXT,      0,                     (short)(DlgHeight - 6), 0,                                                   (short)(DlgHeight - 6), {}, DIF_SEPARATOR2,                          L""               },
			{DI_TEXT,      5,                     (short)(DlgHeight - 5), (short)(DlgWidth - (strFindStr.IsEmpty() ? 6 : 12)),
             (short)(DlgHeight - 5),                                                                                                                   {}, DIF_SHOWAMPERSAND,                       strSearchStr      },
			{DI_TEXT,      (short)(DlgWidth - 9), (short)(DlgHeight - 5), (short)(DlgWidth - 6),
             (short)(DlgHeight - 5),                                                                                                                   {}, (strFindStr.IsEmpty() ? DIF_HIDDEN : 0), L""               },
			{DI_TEXT,      0,                     (short)(DlgHeight - 4), 0,                                                   (short)(DlgHeight - 4), {}, DIF_SEPARATOR,                           L""               },
			{DI_BUTTON,    0,                     (short)(DlgHeight - 3), 0,                                                   (short)(DlgHeight - 3), {},
             DIF_FOCUS | DIF_DEFAULT | DIF_CENTERGROUP,                                                                                                                                             Msg::FindNewSearch},
			{DI_BUTTON,    0,                     (short)(DlgHeight - 3), 0,                                                   (short)(DlgHeight - 3), {},
             DIF_CENTERGROUP | DIF_DISABLE,                                                                                                                                                         Msg::FindGoTo     },
			{DI_BUTTON,    0,                     (short)(DlgHeight - 3), 0,                                                   (short)(DlgHeight - 3), {},
             DIF_CENTERGROUP | DIF_DISABLE,                                                                                                                                                         Msg::FindView     },
			{DI_BUTTON,    0,                     (short)(DlgHeight - 3), 0,                                                   (short)(DlgHeight - 3), {},
             DIF_CENTERGROUP | DIF_DISABLE,                                                                                                                                                         Msg::FindEdit     },
			{DI_BUTTON,    0,                     (short)(DlgHeight - 3), 0,                                                   (short)(DlgHeight - 3), {},
             DIF_CENTERGROUP | DIF_DISABLE,                                                                                                                                                         Msg::FindPanel    },
			{DI_BUTTON,    0,                     (short)(DlgHeight - 3), 0,                                                   (short)(DlgHeight - 3), {}, DIF_CENTERGROUP,
             Msg::FindStop                                                                                                                                                                                            }
    };
	MakeDialogItemsEx(FindDlgData, FindDlg);
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);

	if (v.PluginMode) {
		Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
		PHPTR hPlugin = ActivePanel->GetPluginHandle();
		OpenPluginInfo Info;
		{
			SCOPED_ACTION(PluginLocker);
			CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);
			itd.SetFindFileArcIndex(itd.AddArcListItem(Info.HostFile, hPlugin, Info.Flags, Info.CurDir));
		}

		if (itd.GetFindFileArcIndex() == LIST_INDEX_NONE)
			return false;

		if (!(Info.Flags & OPIF_REALNAMES)) {
			FindDlg[FD_BUTTON_PANEL].Type = DI_TEXT;
			FindDlg[FD_BUTTON_PANEL].strData.Clear();
		}
	}

	AnySetFindList = false;
	{
		SCOPED_ACTION(PluginLocker);
		for (int i = 0; i < CtrlObject->Plugins.GetPluginsCount(); i++) {
			if (CtrlObject->Plugins.GetPlugin(i)->HasSetFindList()) {
				AnySetFindList = true;
				break;
			}
		}
	}

	if (!AnySetFindList) {
		FindDlg[FD_BUTTON_PANEL].Flags|= DIF_DISABLE;
	}

	Dialog Dlg = Dialog(FindDlg, ARRAYSIZE(FindDlg), FindDlgProc, reinterpret_cast<LONG_PTR>(&v));
	//  pDlg->SetDynamicallyBorn();
	Dlg.SetHelp(L"FindFileResult");
	Dlg.SetId(FindFileResultId);
	Dlg.SetPosition(-1, -1, DlgWidth, DlgHeight);
	// Надо бы показать диалог, а то инициализация элементов запаздывает
	// иногда при поиске и первые элементы не добавляются
	Dlg.InitDialog();
	Dlg.Show();

	strLastDirName.Clear();

	FindFileThread fft(v.PluginMode, reinterpret_cast<HANDLE>(&Dlg));
	if (fft.StartThread()) {
		SCOPED_ACTION(wakeful);
		Dlg.Process();
		WAIT_FOR_AND_DISPATCH_INTER_THREAD_CALLS(fft.CheckForDone());

		PauseFlag = false;
		StopFlag = false;

		switch (Dlg.GetExitCode()) {
			case FD_BUTTON_NEW: {
				return true;
			}

			case FD_BUTTON_PANEL:
				// Отработаем переброску на временную панель
				{
					size_t ListSize = itd.GetFindListCount();
					PluginPanelItem *PanelItems = new (std::nothrow) PluginPanelItem[ListSize];

					if (!PanelItems)
						ListSize = 0;

					int ItemsNumber = 0;

					for (size_t i = 0; i < ListSize; i++) {
						FINDLIST FindItem;
						itd.GetFindListItem(i, FindItem);
						if (!FindItem.FindData.strFileName.IsEmpty() && FindItem.Used)
						// Добавляем всегда, если имя задано
						{
							// Для плагинов с виртуальными именами заменим имя файла на имя архива.
							// панель сама уберет лишние дубли.
							bool IsArchive = false;
							if (FindItem.ArcIndex != LIST_INDEX_NONE) {
								ARCLIST ArcItem;
								itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
								if (!(ArcItem.Flags & OPIF_REALNAMES)) {
									IsArchive = true;
								}
							}
							// Добавляем только файлы или имена архивов или папки когда просили
							if (IsArchive || (Opt.FindOpt.FindFolders && !SearchHex)
									|| !(FindItem.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
								if (IsArchive) {
									ARCLIST ArcItem;
									itd.GetArcListItem(FindItem.ArcIndex, ArcItem);
									FindItem.FindData.strFileName = ArcItem.strArcName;
									itd.SetFindListItem(i, FindItem);
								}
								PluginPanelItem *pi = &PanelItems[ItemsNumber++];
								memset(pi, 0, sizeof(*pi));
								apiFindDataExToData(&FindItem.FindData, &pi->FindData);

								if (IsArchive)
									pi->FindData.dwFileAttributes = 0;

								if (pi->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
									DeleteEndSlash(pi->FindData.lpwszFileName);
								}
							}
						}
					}

					PHPTR hNewPlugin = CtrlObject->Plugins.OpenFindListPlugin(PanelItems, ItemsNumber);

					if (hNewPlugin) {
						Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
						Panel *NewPanel = CtrlObject->Cp()->ChangePanel(ActivePanel, FILE_PANEL, TRUE, TRUE);
						NewPanel->SetPluginMode(hNewPlugin, L"", true);
						NewPanel->SetVisible(TRUE);
						NewPanel->Update(0);
						// if (FindExitIndex != LIST_INDEX_NONE)
						// NewPanel->GoToFile(FindList[FindExitIndex].FindData.cFileName);
						NewPanel->Show();
					}

					for (int i = 0; i < ItemsNumber; i++)
						apiFreeFindData(&PanelItems[i].FindData);

					delete[] PanelItems;
					break;
				}
			case FD_BUTTON_GOTO:
			case FD_LISTBOX: {
				FINDLIST FindItem;
				itd.GetFindListItem(v.FindExitIndex, FindItem);
				FARString strFileName = FindItem.FindData.strFileName;
				Panel *FindPanel = CtrlObject->Cp()->ActivePanel;

				if (FindItem.ArcIndex != LIST_INDEX_NONE) {
					ARCLIST ArcItem;
					itd.GetArcListItem(FindItem.ArcIndex, ArcItem);

					if (ArcItem.hPlugin == nullptr) {
						FARString strArcName = ArcItem.strArcName;

						if (FindPanel->GetType() != FILE_PANEL) {
							FindPanel = CtrlObject->Cp()->ChangePanel(FindPanel, FILE_PANEL, TRUE, TRUE);
						}

						FARString strArcPath = strArcName;
						CutToSlash(strArcPath);
						FindPanel->SetCurDir(strArcPath, true);
						ArcItem.hPlugin =
								((FileList *)FindPanel)->OpenFilePlugin(strArcName, FALSE, OFP_SEARCH);
						if (ArcItem.hPlugin == PHPTR_STOP)
							ArcItem.hPlugin = nullptr;
						itd.SetArcListItem(FindItem.ArcIndex, ArcItem);
					}

					if (ArcItem.hPlugin) {
						SCOPED_ACTION(PluginLocker);
						OpenPluginInfo Info;
						CtrlObject->Plugins.GetOpenPluginInfo(ArcItem.hPlugin, &Info);
						if (SearchMode == FINDAREA_ROOT || SearchMode == FINDAREA_ALL
								|| SearchMode == FINDAREA_ALL_BUTNETWORK || SearchMode == FINDAREA_INPATH)
							CtrlObject->Plugins.SetDirectory(ArcItem.hPlugin, L"/", 0);

						SetPluginDirectory(strFileName, ArcItem.hPlugin, TRUE);
					}
				} else {
					FARString strSetName;
					size_t Length = strFileName.GetLength();

					if (!Length)
						break;

					if (Length > 1 && IsSlash(strFileName.At(Length - 1)))
						strFileName.Truncate(Length - 1);

					if ((apiGetFileAttributes(strFileName) == INVALID_FILE_ATTRIBUTES)
							&& !ErrnoSaver().IsAccessDenied())
						break;

					const wchar_t *NamePtr = PointToName(strFileName);
					strSetName = NamePtr;

					strFileName.Truncate(NamePtr - strFileName.CPtr());
					Length = strFileName.GetLength();

					if (Length > 1 && IsSlash(strFileName.At(Length - 1)))
						strFileName.Truncate(Length - 1);

					if (strFileName.IsEmpty())
						break;

					if (FindPanel->GetType() != FILE_PANEL
							&& CtrlObject->Cp()->GetAnotherPanel(FindPanel)->GetType() == FILE_PANEL)
						FindPanel = CtrlObject->Cp()->GetAnotherPanel(FindPanel);

					if ((FindPanel->GetType() != FILE_PANEL) || (FindPanel->GetMode() != NORMAL_PANEL))
					// Сменим панель на обычную файловую...
					{
						FindPanel = CtrlObject->Cp()->ChangePanel(FindPanel, FILE_PANEL, TRUE, TRUE);
						FindPanel->SetVisible(TRUE);
						FindPanel->Update(0);
					}

					// ! Не меняем каталог, если мы уже в нем находимся.
					// Тем самым добиваемся того, что выделение с элементов панели не сбрасывается.
					FARString strDirTmp;
					FindPanel->GetCurDir(strDirTmp);
					Length = strDirTmp.GetLength();

					if (Length > 1 && IsSlash(strDirTmp.At(Length - 1)))
						strDirTmp.Truncate(Length - 1);

					if (StrCmp(strFileName, strDirTmp))
						FindPanel->SetCurDir(strFileName, true);
					if (!strSetName.IsEmpty())
						FindPanel->GoToFile(strSetName);

					FindPanel->Show();
					FindPanel->SetFocus();
				}
				break;
			}
		}
	}
	return false;
}

FindFiles::FindFiles()
{
	_ALGO(CleverSysLog clv(L"FindFiles::FindFiles()"));
	static FARString strLastFindMask = L"*", strLastFindStr;
	// Статическая структура и статические переменные
	static FARString strSearchFromRoot;
	static int LastCmpCase = 0, LastWholeWords = 0, LastSearchInArchives = 0, LastSearchHex = 0;
	// Создадим объект фильтра
	Filter = new FileFilter(CtrlObject->Cp()->ActivePanel, FFT_FINDFILE);
	CmpCase = LastCmpCase;
	WholeWords = LastWholeWords;
	SearchInArchives = LastSearchInArchives;
	SearchHex = LastSearchHex;
	SearchMode = Opt.FindOpt.FileSearchMode;
	UseFilter = Opt.FindOpt.UseFilter;
	strFindMask = strLastFindMask;
	strFindStr = strLastFindStr;
	strSearchFromRoot = Msg::SearchFromRootFolder;

	Vars v;
	do {
		v.Clear();
		itd.ClearAllLists();
		Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
		v.PluginMode = ActivePanel->GetMode() == PLUGIN_PANEL && ActivePanel->IsVisible();
		strSearchFromRoot = Msg::SearchFromRootFolder;
		const wchar_t *MasksHistoryName = L"Masks", *TextHistoryName = L"SearchText";
		const wchar_t *HexMask = L"HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH";
		const wchar_t VSeparator[] = {BoxSymbols[BS_T_H1V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1],
				BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_B_H1V1], 0};
		struct DialogDataEx FindAskDlgData[] = {
			{DI_DOUBLEBOX, 3,  1,  74, 18, {}, 0, Msg::FindFileTitle},
			{DI_TEXT,      5,  2,  0,  2,  {}, 0, Msg::FindFileMasks},
			{DI_EDIT,      5,  3,  72, 3,  {(DWORD_PTR)MasksHistoryName}, DIF_FOCUS | DIF_HISTORY | DIF_USELASTHISTORY,L""},
			{DI_TEXT,      3,  4,  0,  5,  {}, DIF_SEPARATOR, L""},
			{DI_TEXT,      5,  5,  0,  6,  {}, 0, L""},
			{DI_EDIT,      5,  6,  72, 6,  {(DWORD_PTR)TextHistoryName}, DIF_HISTORY, L""},
			{DI_FIXEDIT,   5,  6,  72, 6,  {(DWORD_PTR)HexMask}, DIF_MASKEDIT, L""},
			{DI_TEXT,      5,  7,  0,  7,  {}, 0, L""},
			{DI_COMBOBOX,  5,  8,  72, 8,  {}, DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND, L""},
			{DI_TEXT,      3,  9,  0,  9,  {}, DIF_SEPARATOR, L""},
			{DI_CHECKBOX,  5,  10, 0,  10, {}, 0, Msg::FindFileCaseFileMask},
			{DI_CHECKBOX,  5,  11, 0,  11, {}, 0, Msg::FindFileCase},
			{DI_CHECKBOX,  5,  12, 0,  12, {}, 0, Msg::FindFileWholeWords},
			{DI_CHECKBOX,  5,  13, 0,  13, {}, 0, Msg::SearchForHex},
			{DI_CHECKBOX,  40, 10, 0,  10, {UseFilter ? BSTATE_CHECKED : BSTATE_UNCHECKED}, DIF_AUTOMATION, Msg::FindUseFilter},
			{DI_CHECKBOX,  40, 11, 0,  11, {}, 0, Msg::FindArchives},
			{DI_CHECKBOX,  40, 12, 0,  12, {}, 0, Msg::FindFolders},
			{DI_CHECKBOX,  40, 13, 0,  13, {}, 0, Msg::FindSymLinks},
			{DI_TEXT,      3,  14, 0,  14, {}, DIF_SEPARATOR, L""},
			{DI_VTEXT,     38, 9,  0,   9, {}, DIF_BOXCOLOR, VSeparator},
			{DI_TEXT,      5,  15, 37, 15, {}, 0, Msg::SearchWhere},
			{DI_COMBOBOX,  38, 15, 72, 15, {}, DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND, L""},
			{DI_TEXT,      3,  16, 0,  16, {}, DIF_SEPARATOR, L""},
			{DI_BUTTON,    0,  17, 0,  17, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::FindFileFind},
			{DI_BUTTON,    0,  17, 0,  17, {}, DIF_CENTERGROUP, Msg::FindFileDrive},
			{DI_BUTTON,    0,  17, 0,  17, {}, DIF_CENTERGROUP | DIF_AUTOMATION | (UseFilter ? 0 : DIF_DISABLE), Msg::FindFileSetFilter},
			{DI_BUTTON,    0,  17, 0,  17, {}, DIF_CENTERGROUP, Msg::FindFileAdvanced },
			{DI_BUTTON,    0,  17, 0,  17, {}, DIF_CENTERGROUP, Msg::Cancel}
		};
		MakeDialogItemsEx(FindAskDlgData, FindAskDlg);

		if (strFindStr.IsEmpty())
			FindAskDlg[FAD_CHECKBOX_DIRS].Selected = Opt.FindOpt.FindFolders;

		FarListItem li[] = {
			{0, Msg::SearchAllDisks,      {}},
			{0, Msg::SearchAllButNetwork, {}},
			{0, Msg::SearchInPATH,        {}},
			{0, strSearchFromRoot,        {}},
			{0, Msg::SearchFromCurrent,   {}},
			{0, Msg::SearchInCurrent,     {}},
			{0, Msg::SearchInSelected,    {}},
		};
		li[FADC_ALLDISKS + SearchMode].Flags|= LIF_SELECTED;
		FarList l = {ARRAYSIZE(li), li};
		FindAskDlg[FAD_COMBOBOX_WHERE].ListItems = &l;

		if (v.PluginMode) {
			SCOPED_ACTION(PluginLocker);
			OpenPluginInfo Info;
			CtrlObject->Plugins.GetOpenPluginInfo(ActivePanel->GetPluginHandle(), &Info);

			if (!(Info.Flags & OPIF_REALNAMES))
				FindAskDlg[FAD_CHECKBOX_ARC].Flags|= DIF_DISABLE;

			if (FADC_ALLDISKS + SearchMode == FADC_ALLDISKS || FADC_ALLDISKS + SearchMode == FADC_ALLBUTNET) {
				li[FADC_ALLDISKS].Flags = 0;
				li[FADC_ALLBUTNET].Flags = 0;
				li[FADC_ROOT].Flags|= LIF_SELECTED;
			}

			li[FADC_ALLDISKS].Flags|= LIF_GRAYED;
			li[FADC_ALLBUTNET].Flags|= LIF_GRAYED;
			FindAskDlg[FAD_CHECKBOX_LINKS].Selected = 0;
			FindAskDlg[FAD_CHECKBOX_LINKS].Flags|= DIF_DISABLE;
		} else
			FindAskDlg[FAD_CHECKBOX_LINKS].Selected = Opt.FindOpt.FindSymLinks;

		if (!(FindAskDlg[FAD_CHECKBOX_ARC].Flags & DIF_DISABLE))
			FindAskDlg[FAD_CHECKBOX_ARC].Selected = SearchInArchives;

		FindAskDlg[FAD_EDIT_MASK].strData = strFindMask;
		FindAskDlg[FAD_CHECKBOX_CASEMASK].Selected = Opt.FindOpt.FindCaseSensitiveFileMask;

		if (SearchHex)
			FindAskDlg[FAD_EDIT_HEX].strData = strFindStr;
		else
			FindAskDlg[FAD_EDIT_TEXT].strData = strFindStr;

		FindAskDlg[FAD_CHECKBOX_CASE].Selected = CmpCase;
		FindAskDlg[FAD_CHECKBOX_WHOLEWORDS].Selected = WholeWords;
		FindAskDlg[FAD_CHECKBOX_HEX].Selected = SearchHex;
		int ExitCode;
		Dialog Dlg(FindAskDlg, ARRAYSIZE(FindAskDlg), MainDlgProc, reinterpret_cast<LONG_PTR>(&v));
		Dlg.SetAutomation(FAD_CHECKBOX_FILTER, FAD_BUTTON_FILTER, DIF_DISABLE, DIF_NONE, DIF_NONE,
				DIF_DISABLE);
		Dlg.SetHelp(L"FindFile");
		Dlg.SetId(FindFileId);
		Dlg.SetPosition(-1, -1, 78, 20);
		Dlg.Process();
		ExitCode = Dlg.GetExitCode();
		// Рефреш текущему времени для фильтра сразу после выхода из диалога
		Filter->UpdateCurrentTime();

		if (ExitCode != FAD_BUTTON_FIND) {
			return;
		}

		Opt.FindOpt.FindCaseSensitiveFileMask = (FindAskDlg[FAD_CHECKBOX_CASEMASK].Selected == BSTATE_CHECKED);

		Opt.FindCodePage = CodePage;
		CmpCase = FindAskDlg[FAD_CHECKBOX_CASE].Selected;
		WholeWords = FindAskDlg[FAD_CHECKBOX_WHOLEWORDS].Selected;
		SearchHex = FindAskDlg[FAD_CHECKBOX_HEX].Selected;
		SearchInArchives = FindAskDlg[FAD_CHECKBOX_ARC].Selected;

		if (v.FindFoldersChanged) {
			Opt.FindOpt.FindFolders = (FindAskDlg[FAD_CHECKBOX_DIRS].Selected == BSTATE_CHECKED);
		}

		if (!v.PluginMode) {
			Opt.FindOpt.FindSymLinks = (FindAskDlg[FAD_CHECKBOX_LINKS].Selected == BSTATE_CHECKED);
		}

		UseFilter = (FindAskDlg[FAD_CHECKBOX_FILTER].Selected == BSTATE_CHECKED);
		Opt.FindOpt.UseFilter = UseFilter;
		strFindMask = !FindAskDlg[FAD_EDIT_MASK].strData.IsEmpty() ? FindAskDlg[FAD_EDIT_MASK].strData : L"*";

		if (SearchHex) {
			strFindStr = FindAskDlg[FAD_EDIT_HEX].strData;
			RemoveTrailingSpaces(strFindStr);
		} else
			strFindStr = FindAskDlg[FAD_EDIT_TEXT].strData;

		if (!strFindStr.IsEmpty()) {
			strGlobalSearchString = strFindStr;
			GlobalSearchCase = CmpCase;
			GlobalSearchWholeWords = WholeWords;
			GlobalSearchHex = SearchHex;
		}

		switch (FindAskDlg[FAD_COMBOBOX_WHERE].ListPos) {
			case FADC_ALLDISKS:
				SearchMode = FINDAREA_ALL;
				break;
			case FADC_ALLBUTNET:
				SearchMode = FINDAREA_ALL_BUTNETWORK;
				break;
			case FADC_PATH:
				SearchMode = FINDAREA_INPATH;
				break;
			case FADC_ROOT:
				SearchMode = FINDAREA_ROOT;
				break;
			case FADC_FROMCURRENT:
				SearchMode = FINDAREA_FROM_CURRENT;
				break;
			case FADC_INCURRENT:
				SearchMode = FINDAREA_CURRENT_ONLY;
				break;
			case FADC_SELECTED:
				SearchMode = FINDAREA_SELECTED;
				break;
		}

		if (v.SearchFromChanged) {
			Opt.FindOpt.FileSearchMode = SearchMode;
		}

		LastCmpCase = CmpCase;
		LastWholeWords = WholeWords;
		LastSearchHex = SearchHex;
		LastSearchInArchives = SearchInArchives;
		strLastFindMask = strFindMask;
		strLastFindStr = strFindStr;

		if (!strFindStr.IsEmpty())
			Editor::SetReplaceMode(FALSE);
	} while (FindFilesProcess(v));
	CtrlObject->Cp()->ActivePanel->SetTitle();
}

FindFiles::~FindFiles()
{
	itd.ClearAllLists();
	ScrBuf.ResetShadow();

	if (Filter) {
		delete Filter;
	}
}
