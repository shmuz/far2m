#pragma once

/*
dialog.hpp

Класс диалога Dialog.

Предназначен для отображения модальных диалогов.
Является производным от класса Frame.
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

#include <optional>
#include <farplug-wide.h>
#include "bitflags.hpp"
#include "chgmmode.hpp"
#include "CriticalSections.hpp"
#include "frame.hpp"
#include "vmenu.hpp"

class History;
class Editor;
class DlgEdit;

// Флаги текущего режима диалога
enum DIALOG_MODES
{
	DMODE_INITOBJECTS   = 0x00000001,		// элементы инициализарованы?
	DMODE_CREATEOBJECTS = 0x00000002,		// объекты (Edit,...) созданы?
	DMODE_WARNINGSTYLE  = 0x00000004,		// Warning Dialog Style?
	DMODE_DRAGGED       = 0x00000008,		// диалог двигается?
	DMODE_ISCANMOVE     = 0x00000010,		// можно ли двигать диалог?
	DMODE_ALTDRAGGED    = 0x00000020,		// диалог двигается по Alt-Стрелка?
	DMODE_SMALLDIALOG   = 0x00000040,		// "короткий диалог"
	DMODE_DRAWING       = 0x00001000,		// диалог рисуется?
	DMODE_KEY           = 0x00002000,		// Идет посылка клавиш?
	DMODE_SHOW          = 0x00004000,		// Диалог виден?
	DMODE_MOUSEEVENT    = 0x00008000,		// Нужно посылать MouseMove в обработчик?
	DMODE_RESIZED       = 0x00010000,		//
	DMODE_ENDLOOP       = 0x00020000,		// Конец цикла обработки диалога?
	DMODE_BEGINLOOP     = 0x00040000,		// Начало цикла обработки диалога?
	// DMODE_OWNSITEMS           =0x00080000, // если TRUE, Dialog освобождает список Item в деструкторе
	DMODE_NODRAWSHADOW     = 0x00100000,	// не рисовать тень?
	DMODE_NODRAWPANEL      = 0x00200000,	// не рисовать подложку?
	DMODE_FULLSHADOW       = 0x00400000,
	DMODE_NOPLUGINS        = 0x00800000,
	DMODE_KEEPCONSOLETITLE = 0x10000000,	// не изменять заголовок консоли
	DMODE_CLICKOUTSIDE     = 0x20000000,	// было нажатие мыши вне диалога?
	DMODE_MSGINTERNAL      = 0x40000000,	// Внутренняя Message?
	DMODE_OLDSTYLE         = 0x80000000,	// Диалог в старом (до 1.70) стиле
};

// #define DIMODE_REDRAW       0x00000001 // требуется принудительная прорисовка итема?

#define MakeDialogItemsEx(Data, Item)	\
	DialogItemEx Item[ARRAYSIZE(Data)] {};	\
	DataToItemEx(Data, Item, ARRAYSIZE(Data));

// Структура, описывающая автоматизацию для DIF_AUTOMATION
// на первом этапе - примитивная - выставление флагов у элементов для CheckBox
struct DialogItemAutomation
{
	int Target;			// Для этого элемента...
	DWORD Flags[3][2];	// ...выставить вот эти флаги
						// [0] - Unchecked, [1] - Checked, [2] - 3Checked
						// [][0] - Set, [][1] - Skip
};

struct DlgUserCursor
{
	COORD Pos {-1, -1};
	bool  Visible {};
	DWORD Size { static_cast<DWORD>(-1) };

	DlgUserCursor() = default;
};

/*
Описывает один элемент диалога - внутренне представление.
Для плагинов это FarDialogItem (за исключением ObjPtr)
*/
struct DialogItemEx
{
	int Type;
	int X1, Y1, X2, Y2;
	int Focus;
	union
	{
		DWORD_PTR Reserved;
		int Selected;
		FarList *ListItems;
		int ListPos;
		CHAR_INFO *VBuf;
	};
	FARString strHistory;
	FARString strMask;

	uint64_t customColor[DLG_ITEM_MAX_CUST_COLORS];

	DWORD Flags;
	int DefaultButton;

	FARString strData;
	size_t nMaxLength;

	int ID;
	BitFlags IFlags;
	std::vector<DialogItemAutomation> Auto;
	DWORD_PTR UserData;		// ассоциированные данные

	// прочее
	void *ObjPtr;
	VMenu *ListPtr;
	DlgUserCursor *UCData;

	int SelStart;
	int SelEnd;

	DialogItemEx() = default;
	DialogItemEx(const DialogItemEx &Other) = default;
	DialogItemEx &operator=(const DialogItemEx &Other) = default;
	void Clear() { *this = DialogItemEx{}; }

	void Indent(int Delta)
	{
		X1 += Delta;
		X2 += Delta;
	}

	void AddAutomation(int id, FarDialogItemFlags UncheckedSet, FarDialogItemFlags UncheckedSkip,
			FarDialogItemFlags CheckedSet, FarDialogItemFlags CheckedSkip,
			FarDialogItemFlags Checked3Set, FarDialogItemFlags Checked3Skip)
	{
		auto &A = Auto.emplace_back();
		A.Target = id;
		A.Flags[0][0] = UncheckedSet;
		A.Flags[0][1] = UncheckedSkip;
		A.Flags[1][0] = CheckedSet;
		A.Flags[1][1] = CheckedSkip;
		A.Flags[2][0] = Checked3Set;
		A.Flags[2][1] = Checked3Skip;
	}

	void     CopyToItemSmall(FarDialogItem *Item) const;
	bool     CopyToPluginItem(FarDialogItem *Item, bool shortMode) const;
	size_t   GetDlgItem(FarDialogItem *Item) const;
	DlgEdit* GetEdit() const { return static_cast<DlgEdit*>(ObjPtr); }
	DlgEdit* GetEdit() { return static_cast<DlgEdit*>(ObjPtr); }
	size_t   GetStringAndSize(FARString &ItemString) const;
	bool     HasDropDownArrow() const;
	bool     IsFocusable() const;
	bool     IsHorizontalSeparator() const;
	bool     IsVerticalSeparator() const;
	bool     SetFromPluginItem(const FarDialogItem *Item, bool shortMode);
};

/*
Описывает один элемент диалога - для сокращения объемов
Структура аналогичена структуре InitDialogItem (см. "Far PlugRinG
Russian Help Encyclopedia of Developer")
*/

struct DialogDataEx
{
	WORD Type;
	int X1, Y1, X2, Y2;
	union
	{
		DWORD_PTR Reserved;
		unsigned int Selected;
		const wchar_t *History;
		const wchar_t *Mask;
		FarList *ListItems;
		int ListPos;
		CHAR_INFO *VBuf;
	};
	DWORD Flags;
	const wchar_t *Data;
};

class DlgEdit;
class ConsoleTitle;

class Dialog : public Frame
{
	friend class DlgEdit;
	friend class History;
	friend LONG_PTR WINAPI DefDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
	friend LONG_PTR SendDlgMessageSynched(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);

private:
	ChangeMacroArea Cma;
	INT_PTR PluginNumber;		// Номер плагина, для формирования HelpTopic
	int FocusPos;						// всегда известно какой элемент в фокусе
	int PrevFocusPos;				// всегда известно какой элемент был в фокусе
	int IsEnableRedraw;			// Разрешена перерисовка диалога? ( > 0 - разрешена)
	int InCtlColorDlgItem;
	BitFlags DialogMode;		// Флаги текущего режима диалога

	LONG_PTR DataDialog;		// Данные, специфические для конкретного экземпляра диалога
								//           (первоначально здесь параметр, переданный в конструктор)

	std::vector<DialogItemEx> Item;	// массив элементов диалога
	DialogItemEx *pSaveItemEx;	// пользовательский массив элементов диалога

	ConsoleTitle *OldTitle;		// предыдущий заголовок

	FARWINDOWPROC RealDlgProc;	// функция обработки диалога

	// переменные для перемещения диалога
	int OldX1, OldX2, OldY1, OldY2;

	FARString HelpTopic;

	volatile bool DropDownOpened;	// Содержит статус комбобокса и хистори: TRUE - открыт, FALSE - закрыт.

	CriticalSection CS;

	int RealWidth, RealHeight;

	std::optional<GUID> Id;
	int AltState, CtrlState, ShiftState;

private:
	void Init(FARWINDOWPROC DlgProc, LONG_PTR InitParam);
	void DisplayObject() override;
	void DeleteDialogObjects();
	int LenStrItem(int ID, const wchar_t *lpwszStr = nullptr);

	void ShowDialog(int ID = -1);	// ID=-1 - отрисовать весь диалог

	DWORD CtlColorDlgItem(int ItemPos, uint64_t *ItemColor);
	/*
		$ 28.07.2000 SVS
		+ Изменяет фокус ввода между двумя элементами.
		Вынесен отдельно для того, чтобы обработать DMSG_KILLFOCUS & DMSG_SETFOCUS
	*/
	void ChangeFocus2(int SetFocusPos);

	int ChangeFocus(int FocusPos, int Step, bool SkipGroup);
	bool SelectFromEditHistory(DialogItemEx &CurItem, DlgEdit *EditLine, const wchar_t *HistoryName,
			FARString &strStr);
	int SelectFromComboBox(DialogItemEx &CurItem, DlgEdit *EditLine, VMenu *List);
	int AddToEditHistory(const wchar_t *AddStr, const wchar_t *HistoryName);

	void ProcessLastHistory(DialogItemEx &CurItem, int MsgIndex);	// обработка DIF_USELASTHISTORY

	bool ProcessHighlighting(FarKey Key, int FocusPos, bool Translate);
	int CheckHighlights(WORD Chr, int StartPos = 0);

	void SelectOnEntry(int Pos, bool Selected);

	void CheckDialogCoord();

	// возвращает заголовок диалога (текст первого текста или фрейма)
	const wchar_t *GetDialogTitle();

	bool GetItemRect(int ID, SMALL_RECT &Rect);
	bool SetItemRect(int ID, const SMALL_RECT *Rect);

	/*
		$ 23.06.2001 KM
		+ Функции программного открытия/закрытия комбобокса и хистори
		и получения статуса открытости/закрытости комбобокса и хистори.
	*/
	void SetDropDownOpened(bool Status) { DropDownOpened = Status; }
	bool GetDropDownOpened() const { return DropDownOpened; }

	void ProcessCenterGroup();
	int ProcessRadioButton(int);

	int InitDialogObjects(int ID = -1);

	bool ProcessOpenComboBox(int Type, DialogItemEx &CurItem, int CurFocusPos);
	bool ProcessMoveDialog(DWORD Key);

	bool Do_ProcessTab(bool Next);
	bool Do_ProcessNextCtrl(bool Next, bool IsRedraw = true);

	bool MoveToCtrlHorizontal(bool right); // move focus to right or left dialog item.
	bool MoveToCtrlVertical(bool up); // move focus to up or down dialog item.

	bool Do_ProcessFirstCtrl();
	bool Do_ProcessSpace();
	void SetComboBoxPos(DialogItemEx *Item = nullptr);

	LONG_PTR CallDlgProc(int nMsg, int Param1, LONG_PTR Param2);
	LONG_PTR DefDlgProc(int Msg, int Param1, LONG_PTR Param2);
	LONG_PTR DlgProc(int Msg, int Param1, LONG_PTR Param2);
	LONG_PTR SendDlgMessage(int Msg, int Param1, LONG_PTR Param2);
	LONG_PTR SendDlgMessageSynched(int Msg, int Param1, LONG_PTR Param2);

	void ProcessKey(FarKey Key, int ItemPos);

public:
	Dialog(DialogItemEx *SrcItem, unsigned SrcItemCount, FARWINDOWPROC DlgProc = nullptr,
			LONG_PTR InitParam = 0);
	Dialog(FarDialogItem *SrcItem, unsigned SrcItemCount, FARWINDOWPROC DlgProc = nullptr,
			LONG_PTR InitParam = 0);
	~Dialog() override;

public:
	int FastHide() override;
	int GetType() const override { return MODALTYPE_DIALOG; }
	const wchar_t *GetTypeName() const override { return L"[Dialog]"; };
	int GetTypeAndName(FARString &strType, FARString &strName) override;
	void Hide() override;
	void OnChangeFocus(bool focus) override;
	bool ProcessEvents() const override;
	int ProcessKey(FarKey Key) override;
	int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent) override;
	void ResizeConsole() override;
	void SetExitCode(int Code) override;
	void SetPosition(int X1, int Y1, int X2, int Y2) override;
	void Show() override;
	int64_t VMProcess(int OpCode, void *vParam = nullptr, int64_t iParam = 0) override;

	bool CheckDialogMode(DWORD flags) const { return DialogMode.Check(flags); }
	void FastShow() { ShowDialog(); }
	void GetDialogObjectsData();
	void SetDialogMode(DWORD flags) { DialogMode.Set(flags); }

	void AdjustEditPos(int dx, int dy);
	bool IsMoving() const { return DialogMode.Check(DMODE_DRAGGED); }
	void SetModeMoving(bool IsMoving) { DialogMode.Change(DMODE_ISCANMOVE, IsMoving); }
	bool GetModeMoving() const { return DialogMode.Check(DMODE_ISCANMOVE); }
	void SetDialogData(LONG_PTR NewDataDialog);
	LONG_PTR GetDialogData() const { return DataDialog; };

	void InitDialog();
	void Process();
	void SetPluginNumber(INT_PTR NewPluginNumber) { PluginNumber = NewPluginNumber; }

	void SetHelp(const wchar_t *Topic);
	void ShowHelp();
	bool Done() const { return DialogMode.Check(DMODE_ENDLOOP); }
	void ClearDone();
	void CloseDialog();

	// For MACRO
	const DialogItemEx *GetAllItem() const { return Item.data(); };
	int ItemCount() const { return Item.size(); };	// количество элементов диалога
	int GetDlgFocusPos() const { return FocusPos; };

	void SetAutomation(int IDParent, int id, FarDialogItemFlags UncheckedSet,
			FarDialogItemFlags UncheckedSkip, FarDialogItemFlags CheckedSet, FarDialogItemFlags CheckedSkip,
			FarDialogItemFlags Checked3Set = DIF_NONE, FarDialogItemFlags Checked3Skip = DIF_NONE);

	bool IsInited();
	void SetId(const GUID &Guid) { Id = Guid; }
	bool IsRedrawEnabled() const { return IsEnableRedraw > 0; }
	Editor* GetMemoEdit(int Pos = -1) const;
};

LONG_PTR WINAPI SendDlgMessage(HANDLE hDlg, int Msg, int Param1 = 0, LONG_PTR Param2 = 0);

LONG_PTR WINAPI DefDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);

bool IsKeyHighlighted(const wchar_t *Str, FarKey Key, bool Translate, int AmpPos = -1);

void DataToItemEx(const DialogDataEx *Data, DialogItemEx *Item, int Count);

extern const wchar_t *fmtSavedDialogHistory;
