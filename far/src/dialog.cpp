/*
dialog.cpp

Класс диалога
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

#include "dialog.hpp"
#include "lang.hpp"
#include "keyboard.hpp"
#include "macroopcode.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "chgprior.hpp"
#include "vmenu.hpp"
#include "dlgedit.hpp"
#include "help.hpp"
#include "scrbuf.hpp"
#include "manager.hpp"
#include "savescr.hpp"
#include "constitle.hpp"
#include "lockscrn.hpp"
#include "TPreRedrawFunc.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "farcolors.hpp"
#include "message.hpp"
#include "strmix.hpp"
#include "history.hpp"
#include "InterThreadCall.hpp"
#include "DlgGuid.hpp"
#include "sizer.hpp"
#include <VT256ColorTable.h>
#include <cwctype>
#include <atomic>

#define FarIsEditNoCombobox(Type) \
	((Type) == DI_EDIT || (Type) == DI_FIXEDIT || (Type) == DI_PSWEDIT || (Type) == DI_MEMOEDIT)

#define VTEXT_ADN_SEPARATORS 1

enum DLGEDITLINEFLAGS
{
	DLGEDITLINE_CLEARSELONKILLFOCUS = 0x00000001,		// управляет выделением блока при потере фокуса ввода
	DLGEDITLINE_SELALLGOTFOCUS      = 0x00000002,		// управляет выделением блока при получении фокуса ввода
	DLGEDITLINE_NOTSELONGOTFOCUS    = 0x00000004,		// не восстанавливать выделение строки редактирования при получении фокуса ввода
	DLGEDITLINE_NEWSELONGOTFOCUS    = 0x00000008,		// управляет процессом выделения блока при получении фокуса
	DLGEDITLINE_GOTOEOLGOTFOCUS     = 0x00000010,		// при получении фокуса ввода переместить курсор в конец строки
};

enum DLGITEMINTERNALFLAGS
{
	DLGIIF_LISTREACTIONFOCUS   = 0x00000001,		// MouseReaction для фокусного элемента
	DLGIIF_LISTREACTIONNOFOCUS = 0x00000002,		// MouseReaction для не фокусного элемента

	DLGIIF_COMBOBOXNOREDRAWEDIT = 0x00000008,		// не прорисовывать строку редактирования при изменениях в комбо
	DLGIIF_COMBOBOXEVENTKEY     = 0x00000010,		// посылать события клавиатуры в диалоговую проц. для открытого комбобокса
	DLGIIF_COMBOBOXEVENTMOUSE   = 0x00000020,		// посылать события мыши в диалоговую проц. для открытого комбобокса
};

const wchar_t *fmtSavedDialogHistory = L"SavedDialogHistory/";

//////////////////////////////////////////////////////////////////////////
bool DialogItemEx::IsFocusable() const
{
	switch (Type)
	{
		case DI_EDIT:
		case DI_FIXEDIT:
		case DI_PSWEDIT:
		case DI_COMBOBOX:
		case DI_MEMOEDIT:
		case DI_BUTTON:
		case DI_CHECKBOX:
		case DI_RADIOBUTTON:
		case DI_LISTBOX:
		case DI_USERCONTROL:
			return !(Flags & (DIF_NOFOCUS | DIF_DISABLE | DIF_HIDDEN));
		default:
			return false;
	}
}

/**
 * check if dialog item is horizontal separator.
*/
bool DialogItemEx::IsHorizontalSeparator() const
{
	return (Type == DI_SINGLEBOX || Type == DI_DOUBLEBOX ||
		(Type == DI_TEXT && (Flags & (DIF_SEPARATOR | DIF_SEPARATOR2 | DIF_SEPARATORUSER))));
}

/**
 * check if dialog item is vertical separator.
*/
bool DialogItemEx::IsVerticalSeparator() const
{
	return (Type == DI_SINGLEBOX || Type == DI_DOUBLEBOX ||
		(Type == DI_VTEXT && (Flags & (DIF_SEPARATOR | DIF_SEPARATOR2 | DIF_SEPARATORUSER))));
}

bool IsKeyHighlighted(const wchar_t *Str, FarKey Key, bool Translate, int AmpPos)
{
	if (AmpPos == -1) {
		if (!(Str = wcschr(Str, L'&')))
			return false;

		AmpPos = 1;
	} else {
		if (AmpPos >= StrLength(Str))
			return false;

		Str+= AmpPos;
		AmpPos = 0;

		if (Str[AmpPos] == L'&')
			AmpPos++;
	}

	wchar_t UpperStrKey = Upper(Str[AmpPos]);

	if (WCHAR_IS_VALID(Key)) {
		return UpperStrKey == Upper(Key) || (Translate && KeyToKeyLayoutCompare(Upper(Key), UpperStrKey));
	}

	if (Key & KEY_ALT) {
		uint32_t AltKey = Key & (~KEY_ALT);

		if (WCHAR_IS_VALID(AltKey)) {
			if (iswdigit(AltKey) != 0)
				return (AltKey == (uint32_t)UpperStrKey);

			if (AltKey > L' ')
			//         (AltKey=='-'  || AltKey=='/' || AltKey==','  || AltKey=='.' ||
			//          AltKey=='\\' || AltKey=='=' || AltKey=='['  || AltKey==']' ||
			//          AltKey==':'  || AltKey=='"' || AltKey=='~'))
			{
				return (UpperStrKey == Upper(AltKey)
						|| (Translate && KeyToKeyLayoutCompare(AltKey, UpperStrKey)));
			}
		}
	}

	return false;
}

void DialogItemEx::ToDialogItemEx(DialogItemEx *pDest) const
{
	*pDest = *this;
	pDest->nMaxLength = 0;
}

void DialogItemEx::ConvertItemSmall(FarDialogItem *Item) const
{
	Item->Type = Type;
	Item->X1 = X1;
	Item->Y1 = Y1;
	Item->X2 = X2;
	Item->Y2 = Y2;
	Item->Focus = Focus;
	Item->Flags = Flags;
	Item->DefaultButton = DefaultButton;
	Item->MaxLen = nMaxLength;
	Item->PtrData = nullptr;

	Item->Param.History = nullptr;
	if (Type == DI_LISTBOX || Type == DI_COMBOBOX)
		Item->Param.ListPos = ListPtr ? ListPtr->GetSelectPos() : 0;
	else if ((Type == DI_EDIT || Type == DI_FIXEDIT) && Flags & DIF_HISTORY)
		Item->Param.History = strHistory;
	else if (Type == DI_FIXEDIT && Flags & DIF_MASKEDIT)
		Item->Param.Mask = strMask;
	else
		Item->Param.Reserved = Reserved;
}

size_t DialogItemEx::StringAndSize(FARString &ItemString) const
{
	DlgEdit *EditPtr;
	if (FarIsEdit(Type) && (EditPtr = GetEdit()) != nullptr)
		EditPtr->GetString(ItemString);
	else
		ItemString = strData;

	size_t sz = ItemString.GetLength();

	if (sz > nMaxLength && nMaxLength > 0)
		sz = nMaxLength;

	return sz;
}

bool DialogItemEx::ConvertFromPlugin(const FarDialogItem *Item, bool Short)
{
	if (!Item)
		return false;

	X1 = Item->X1;
	Y1 = Item->Y1;
	X2 = Max(Item->X1, Item->X2);
	Y2 = Max(Item->Y1, Item->Y2);
	Focus = Item->Focus;
	Reserved = 0;

	if ((Item->Type == DI_EDIT || Item->Type == DI_FIXEDIT) && (Item->Flags & DIF_HISTORY))
		strHistory = Item->Param.History;
	else if (Item->Type == DI_FIXEDIT && Item->Flags & DIF_MASKEDIT)
		strMask = Item->Param.Mask;
	else
		Reserved = Item->Param.Reserved;

	Flags = Item->Flags;
	DefaultButton = Item->DefaultButton;
	Type = Item->Type;

	if (!Short) {
		strData = Item->PtrData;
		nMaxLength = Item->MaxLen;

		if (nMaxLength > 0)
			strData.Truncate(nMaxLength);
	}

	ListItems = Item->Param.ListItems;

	if ((Type == DI_COMBOBOX || Type == DI_LISTBOX) && !IsPtr(ListItems))
		ListItems = nullptr;

	return true;
}

bool DialogItemEx::ConvertToPlugin(FarDialogItem *Item, bool Short) const
{
	if (!Item)
		return false;

	ConvertItemSmall(Item);

	if (!Short) {
		FARString str;
		size_t sz = StringAndSize(str);

		wchar_t *p = (wchar_t *)malloc((sz + 1) * sizeof(wchar_t));
		Item->PtrData = p;

		if (!p)		// TODO: may be needed message?
			return false;

		wmemcpy(p, str.CPtr(), sz);
		p[sz] = L'\0';
	}

	return true;
}

size_t DialogItemEx::ConvertItemEx2(FarDialogItem *Item) const
{
	FarDialogItem LocalItem, *pAll = &LocalItem;
	Sizer sizer(Item, SIZE_MAX);
	if (sizer.AddObject<FarDialogItem>())
		pAll = Item;

	if (Item)
		ConvertItemSmall(pAll); // place here, because it sets pAll->PtrData to nullptr

	FARString str;
	StringAndSize(str);
	pAll->PtrData = sizer.AddFARString(str);

	if (Type == DI_LISTBOX || Type == DI_COMBOBOX) {
		FarList LocalFarList, *pFarList = &LocalFarList;
		pAll->Param.ListItems = pFarList;
		if (auto ptr = sizer.AddObject<FarList>())
			pAll->Param.ListItems = pFarList = ptr;

		auto Menu = ListPtr;
		int Count = Menu->GetItemCount();
		pFarList->ItemsNumber = Count;
		pFarList->Items = sizer.AddObject<FarListItem>(Count);
		if (pFarList->Items) {
			for (int i=0; i < Count; i++)
				Menu->MenuItem2FarList(Menu->GetItemPtr(i), &pFarList->Items[i]);
		}
	}

	return sizer.GetSize();
}

void DataToItemEx(const DialogDataEx *Data, DialogItemEx *Item, int Count)
{
	if (!Item || !Data)
		return;

	for (int i = 0; i < Count; i++)
	{
		const auto &Src = Data[i];
		auto &Trg = Item[i];
		const int Type = Src.Type, Flags = Src.Flags;

		Trg.Clear();
		Trg.ID = i;
		Trg.Type = Type;
		Trg.X1 = Src.X1;
		Trg.Y1 = Src.Y1;
		Trg.X2 = Max(Src.X1, Src.X2);
		Trg.Y2 = Max(Src.Y1, Src.Y2);

		Trg.Focus = (Type != DI_SINGLEBOX) && (Type != DI_DOUBLEBOX) && (Flags & DIF_FOCUS);

		if ((Type == DI_EDIT || Type == DI_FIXEDIT) && (Flags & DIF_HISTORY))
			Trg.strHistory = Src.History;
		else if (Type == DI_FIXEDIT && (Flags & DIF_MASKEDIT))
			Trg.strMask = Src.Mask;
		else
			Trg.Reserved = Src.Reserved;

		Trg.Flags = Flags;
		Trg.DefaultButton = (Type != DI_TEXT) && (Type != DI_VTEXT) && (Flags & DIF_DEFAULT);
		Trg.SelStart = -1;

		if (!IsPtr(Src.Data))	// awful
			Trg.strData = FarLangMsg{(int)(DWORD_PTR)Src.Data};
		else
			Trg.strData = Src.Data;
	}
}

Dialog::Dialog(DialogItemEx *SrcItem,		// Набор элементов диалога
		unsigned SrcItemCount,				// Количество элементов
		FARWINDOWPROC aDlgProc,				// Диалоговая процедура
		LONG_PTR InitParam)				// Ассоцированные с диалогом данные
	:
	Cma(MACROAREA_DIALOG), AltState(0), CtrlState(0), ShiftState(0)
{
	Item.reserve(SrcItemCount);

	for (unsigned i = 0; i < SrcItemCount; i++) {
		Item.emplace_back();
		SrcItem[i].ToDialogItemEx(&Item.back());
	}

	pSaveItemEx = SrcItem;
	Init(aDlgProc, InitParam);
}

Dialog::Dialog(FarDialogItem *SrcItem,		// Набор элементов диалога
		unsigned SrcItemCount,			// Количество элементов
		FARWINDOWPROC aDlgProc,			// Диалоговая процедура
		LONG_PTR InitParam)					// Ассоцированные с диалогом данные
	:
	Cma(MACROAREA_DIALOG)
{
	Item.reserve(SrcItemCount);

	for (unsigned i = 0; i < SrcItemCount; i++) {
		Item.emplace_back();
		// BUGBUG add error check
		Item.back().ConvertFromPlugin(&SrcItem[i], false);
	}

	pSaveItemEx = nullptr;
	Init(aDlgProc, InitParam);
}

void Dialog::Init(FARWINDOWPROC aDlgProc,	// Диалоговая процедура
		LONG_PTR aInitParam)					// Ассоцированные с диалогом данные
{
	SetMacroArea(MACROAREA_DIALOG);
	SetDynamicallyBorn(false);				// $OT: По умолчанию все диалоги создаются статически
	CanLoseFocus = false;
	// Номер плагина, вызвавшего диалог (-1 = Main)
	PluginNumber = -1;
	DataDialog = aInitParam;
	DialogMode.Set(DMODE_ISCANMOVE);
	SetDropDownOpened(false);
	IsEnableRedraw = 1;
	InCtlColorDlgItem = 0;
	FocusPos = -1;
	PrevFocusPos = -1;
	AltState = CtrlState = ShiftState = 0;

	if (!aDlgProc)		// функция должна быть всегда!!!
	{
		aDlgProc = ::DefDlgProc;
		// знать диалог в старом стиле - учтем этот факт!
		DialogMode.Set(DMODE_OLDSTYLE);
	}

	RealDlgProc = aDlgProc;

	//_SVS(SysLog(L"Dialog =%d",CtrlObject->Macro.GetMode()));
	// запоминаем предыдущий заголовок консоли
	OldTitle = new ConsoleTitle;
	IdExist = false;
	Id = {};
}

//////////////////////////////////////////////////////////////////////////
/*
	Public, Virtual:
	Деструктор класса Dialog
*/
Dialog::~Dialog()
{
	_tran(SysLog(L"[%p] Dialog::~Dialog()", this));

	DeleteDialogObjects();

	Hide();
	ScrBuf.Flush();

	delete OldTitle;

	if (!WinPortTesting()) {
		INPUT_RECORD rec;
		PeekInputRecord(&rec);
	}

	_DIALOG(CleverSysLog CL(L"Destroy Dialog"));
}

void Dialog::CheckDialogCoord()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (X1 == -1)		// задано центрирование диалога по горизонтали?
	{					//   X2 при этом = ширине диалога.
		X1 = (ScrX - X2 + 1) / 2;

		if (X1 < 0)		// ширина диалога больше ширины экрана?
			X1 = 0;
		else
			X2 += X1 - 1;
	}

	if (Y1 == -1)		// задано центрирование диалога по вертикали?
	{					//   Y2 при этом = высоте диалога.
		Y1 = (ScrY - Y2 + 1) / 2;

		if (Y1 < 0)
			Y1 = 0;
		else
			Y2 += Y1 - 1;
	}
}

void Dialog::InitDialog()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (!DialogMode.Check(DMODE_INITOBJECTS))		// самодостаточный вариант, когда
	{												// элементы инициализируются при первом вызове.
		CheckDialogCoord();
		int InitFocus = InitDialogObjects();
		auto Result = DlgProc(DN_INITDIALOG, InitFocus, DataDialog);

		if (ExitCode == -1) {
			if (Result) {
				// еще разок, т.к. данные могли быть изменены
				InitFocus = InitDialogObjects();	// InitFocus=????
			}

			if (!DialogMode.Check(DMODE_KEEPCONSOLETITLE))
				ConsoleTitle::SetFarTitle(GetDialogTitle());
		}

		// все объекты проинициализированы!
		DialogMode.Set(DMODE_INITOBJECTS);
		DialogInfo di = {sizeof(di)};

		if (DlgProc(DN_GETDIALOGINFO, 0, reinterpret_cast<LONG_PTR>(&di))) {
			Id = di.Id;
			IdExist = true;
		}

		SetMacroArea(Item[InitFocus].Type == DI_MEMOEDIT ? MACROAREA_MEMOEDIT : MACROAREA_DIALOG);
		DlgProc(DN_GOTFOCUS, InitFocus, 0);
	}
}

//////////////////////////////////////////////////////////////////////////
/*
	Public, Virtual:
	Расчет значений координат окна диалога и вызов функции
	ScreenObject::Show() для вывода диалога на экран.
*/
void Dialog::Show()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	_tran(SysLog(L"[%p] Dialog::Show()", this));

	if (!DialogMode.Check(DMODE_INITOBJECTS))
		return;

	if (!Locked() && DialogMode.Check(DMODE_RESIZED)) {
		PreRedrawItem preRedrawItem = PreRedraw.Peek();

		if (preRedrawItem.PreRedrawFunc)
			preRedrawItem.PreRedrawFunc();
	}

	DialogMode.Clear(DMODE_RESIZED);

	if (Locked())
		return;

	DialogMode.Set(DMODE_SHOW);
	ScreenObject::Show();
}

// Цель перехвата данной функции - управление видимостью...
void Dialog::Hide()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	_tran(SysLog(L"[%p] Dialog::Hide()", this));

	if (!DialogMode.Check(DMODE_INITOBJECTS))
		return;

	DialogMode.Clear(DMODE_SHOW);
	ScreenObject::Hide();
}

//////////////////////////////////////////////////////////////////////////
/*
	Private, Virtual:
	Инициализация объектов и вывод диалога на экран.
*/
void Dialog::DisplayObject()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (DialogMode.Check(DMODE_SHOW)) {
		SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
		ShowDialog();	// "нарисуем" диалог
	}
}

// пересчитать координаты для элементов с DIF_CENTERGROUP
void Dialog::ProcessCenterGroup()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	for (int I = 0; I < ItemCount(); I++) {
		/*
			Последовательно объявленные элементы с флагом DIF_CENTERGROUP
			и одинаковой вертикальной позицией будут отцентрированы в диалоге.
			Их координаты X не важны. Удобно использовать для центрирования
			групп кнопок.
		*/
		if ((Item[I].Flags & DIF_CENTERGROUP)
				&& (I == 0 || (Item[I - 1].Flags & DIF_CENTERGROUP) == 0
						|| Item[I - 1].Y1 != Item[I].Y1)) {
			int Length = 0;

			for (int J = I;
					J < ItemCount() && (Item[J].Flags & DIF_CENTERGROUP) && Item[J].Y1 == Item[I].Y1; J++) {
				Length+= LenStrItem(J);

				if (!Item[J].strData.IsEmpty())
					switch (Item[J].Type) {
						case DI_BUTTON:
							Length++;
							break;
						case DI_CHECKBOX:
						case DI_RADIOBUTTON:
							Length+= 5;
							break;
					}
			}

			if (!Item[I].strData.IsEmpty())
				switch (Item[I].Type) {
					case DI_BUTTON:
						Length--;
						break;
					case DI_CHECKBOX:
					case DI_RADIOBUTTON:
						//						Length-=5;
						break;
				}	// Бля, це ж ботва какая-то

			int StartX = Max(0, (X2 - X1 + 1 - Length) / 2);

			for (int J = I;
					J < ItemCount() && (Item[J].Flags & DIF_CENTERGROUP) && Item[J].Y1 == Item[I].Y1; J++) {
				Item[J].X1 = StartX;
				StartX+= LenStrItem(J);

				if (!Item[J].strData.IsEmpty())
					switch (Item[J].Type) {
						case DI_BUTTON:
							StartX++;
							break;
						case DI_CHECKBOX:
						case DI_RADIOBUTTON:
							StartX+= 5;
							break;
					}

				if (StartX == Item[J].X1)
					Item[J].X2 = StartX;
				else
					Item[J].X2 = StartX - 1;
			}
		}
	}
}

/*
	Public:
	Инициализация элементов диалога.

	InitDialogObjects возвращает ID элемента с фокусом ввода
	Параметр - для выборочной реинициализации элементов. ID = -1 - касаемо всех объектов
*/
int Dialog::InitDialogObjects(int ID)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	int InitItemCount;

	if (ID == -1)		// инициализируем все?
	{
		ID = 0;
		InitItemCount = ItemCount();
	}
	else if (ID < ItemCount())
		InitItemCount = ID + 1;
	else
		return -1;

	// если FocusPos в пределах и элемент задисаблен, то ищем сначала
	if (FocusPos != -1 && FocusPos < ItemCount() && !Item[FocusPos].IsFocusable())
		FocusPos = -1;	// будем искать сначала!

	// предварительный цикл по поводу кнопок
	for (int I = ID; I < InitItemCount; I++) {
		DialogItemEx &CurItem = Item[I];
		DWORD ItemFlags = CurItem.Flags;
		int Type = CurItem.Type;

		if (Type == DI_BUTTON && ItemFlags & DIF_SETSHIELD) {
			CurItem.strData = FARString(L"\x2580\x2584 ") + CurItem.strData;
		}

		/*
			для кнопок не имеющи стиля "Показывает заголовок кнопки без скобок"
			добавим энти самые скобки
		*/
		if (Type == DI_BUTTON && !(ItemFlags & DIF_NOBRACKETS)) {
			LPCWSTR Brackets[] = {L"[ ", L" ]", L"{ ", L" }"};
			int Start = (CurItem.DefaultButton ? 2 : 0);
			if (CurItem.strData.At(0) != *Brackets[Start]) {
				CurItem.strData = Brackets[Start] + CurItem.strData + Brackets[Start + 1];
			}
		}
		// предварительный поик фокуса
		if (FocusPos == -1 && CurItem.IsFocusable() && CurItem.Focus)
			FocusPos = I;		// запомним первый фокусный элемент

		CurItem.Focus = 0;		// сбросим для всех, чтобы не оказалось,
		// что фокусов - как у дурочка фантиков

		// сбросим флаг DIF_CENTERGROUP для редакторов
		switch (Type) {
			case DI_BUTTON:
			case DI_CHECKBOX:
			case DI_RADIOBUTTON:
			case DI_TEXT:
			case DI_VTEXT:	// ????
				break;
			default:

				if (ItemFlags & DIF_CENTERGROUP)
					CurItem.Flags&= ~DIF_CENTERGROUP;
		}
	}

	/*
		Опять про фокус ввода - теперь, если "чудо" забыло выставить
		хотя бы один, то ставим на первый подходящий
	*/
	if (FocusPos == -1) {
		for (int I = 0; I < ItemCount(); I++)		// по всем!!!!
		{
			DialogItemEx &CurItem = Item[I];

			if (CurItem.IsFocusable()) {
				FocusPos = I;
				break;
			}
		}
	}

	if (FocusPos == -1)		// ну ни хрена себе - нет ни одного
	{									// элемента с возможностью фокуса
		FocusPos = 0;					// убится, блин
	}

	// ну вот и добрались до!
	Item[FocusPos].Focus = 1;
	// а теперь все сначала и по полной программе...
	ProcessCenterGroup();	// сначала отцентрируем

	for (int I = ID; I < InitItemCount; I++) {
		DialogItemEx &CurItem = Item[I];
		int Type = CurItem.Type;
		DWORD ItemFlags = CurItem.Flags;

		if (Type == DI_LISTBOX) {
			if (!DialogMode.Check(DMODE_CREATEOBJECTS)) {
				CurItem.ListPtr = new VMenu(nullptr, nullptr, 0, CurItem.Y2 - CurItem.Y1 + 1,
						VMENU_ALWAYSSCROLLBAR | VMENU_LISTBOX, nullptr, this);
			}

			if (CurItem.ListPtr) {
				VMenu *ListPtr = CurItem.ListPtr;
				ListPtr->SetVDialogItemID(I);
				/*
					$ 13.09.2000 SVS
					+ Флаг DIF_LISTNOAMPERSAND. По умолчанию для DI_LISTBOX &
					DI_COMBOBOX выставляется флаг MENU_SHOWAMPERSAND. Этот флаг
					подавляет такое поведение
				*/
				CurItem.IFlags.Set(DLGIIF_LISTREACTIONFOCUS | DLGIIF_LISTREACTIONNOFOCUS);		// всегда!
				ListPtr->ChangeFlags(VMENU_DISABLED, ItemFlags & DIF_DISABLE);
				ListPtr->ChangeFlags(VMENU_SHOWAMPERSAND, !(ItemFlags & DIF_LISTNOAMPERSAND));
				ListPtr->ChangeFlags(VMENU_SHOWNOBOX, ItemFlags & DIF_LISTNOBOX);
				ListPtr->ChangeFlags(VMENU_WRAPMODE, ItemFlags & DIF_LISTWRAPMODE);
				ListPtr->ChangeFlags(VMENU_AUTOHIGHLIGHT, ItemFlags & DIF_LISTAUTOHIGHLIGHT);

				if (ItemFlags & DIF_LISTAUTOHIGHLIGHT)
					ListPtr->AssignHighlights(false);

				ListPtr->SetDialogStyle(DialogMode.Check(DMODE_WARNINGSTYLE));
				ListPtr->SetPosition(X1 + CurItem.X1, Y1 + CurItem.Y1, X1 + CurItem.X2, Y1 + CurItem.Y2);
				ListPtr->SetBoxType(SHORT_SINGLE_BOX);

				// поле FarDialogItem.Data для DI_LISTBOX используется как верхний заголовок листа
				if (!(ItemFlags & DIF_LISTNOBOX) && !DialogMode.Check(DMODE_CREATEOBJECTS)) {
					ListPtr->SetTitle(CurItem.strData);
				}

				// удалим все итемы
				// ListBox->DeleteItems(); //???? А НАДО ЛИ ????
				if (CurItem.ListItems && !DialogMode.Check(DMODE_CREATEOBJECTS)) {
					ListPtr->AddItem(CurItem.ListItems);
				}

				ListPtr->ChangeFlags(VMENU_LISTHASFOCUS, CurItem.Focus);
			}
		}
		// "редакторы" - разговор особый...
		else if (FarIsEdit(Type)) {
			/*
				сбросим флаг DIF_EDITOR для строки ввода, отличной от DI_EDIT,
				DI_FIXEDIT и DI_PSWEDIT
			*/
			if (Type != DI_COMBOBOX)
				if ((ItemFlags & DIF_EDITOR) && Type != DI_EDIT && Type != DI_FIXEDIT && Type != DI_PSWEDIT)
					ItemFlags&= ~DIF_EDITOR;

			if (!DialogMode.Check(DMODE_CREATEOBJECTS)) {
				CurItem.ObjPtr =
						new DlgEdit(this, I, Type == DI_MEMOEDIT ? DLGEDIT_MULTILINE : DLGEDIT_SINGLELINE);

				if (Type == DI_COMBOBOX) {
					CurItem.ListPtr = new VMenu(L"", nullptr, 0, Opt.Dialogs.CBoxMaxHeight,
							VMENU_ALWAYSSCROLLBAR | VMENU_NOTCHANGE, nullptr, this);
					CurItem.ListPtr->SetVDialogItemID(I);
				}

				CurItem.SelStart = -1;
			}

			DlgEdit *DialogEdit = CurItem.GetEdit();
			// Mantis#58 - символ-маска с кодом 0х0А - пропадает
			// DialogEdit->SetDialogParent((Type != DI_COMBOBOX && (ItemFlags & DIF_EDITOR) || (CurItem.Type==DI_PSWEDIT || CurItem.Type==DI_FIXEDIT))?
			//	FEDITLINE_PARENT_SINGLELINE:FEDITLINE_PARENT_MULTILINE);
			DialogEdit->SetDialogParent(
					Type == DI_MEMOEDIT ? FEDITLINE_PARENT_MULTILINE : FEDITLINE_PARENT_SINGLELINE);
			DialogEdit->SetReadOnly(false);

			if (Type == DI_COMBOBOX) {
				if (CurItem.ListPtr) {
					VMenu *ListPtr = CurItem.ListPtr;
					ListPtr->SetBoxType(SHORT_SINGLE_BOX);
					DialogEdit->SetDropDownBox(ItemFlags & DIF_DROPDOWNLIST);
					ListPtr->ChangeFlags(VMENU_WRAPMODE, ItemFlags & DIF_LISTWRAPMODE);
					ListPtr->ChangeFlags(VMENU_DISABLED, ItemFlags & DIF_DISABLE);
					ListPtr->ChangeFlags(VMENU_SHOWAMPERSAND, !(ItemFlags & DIF_LISTNOAMPERSAND));
					ListPtr->ChangeFlags(VMENU_AUTOHIGHLIGHT, ItemFlags & DIF_LISTAUTOHIGHLIGHT);

					if (ItemFlags & DIF_LISTAUTOHIGHLIGHT)
						ListPtr->AssignHighlights(false);

					if (CurItem.ListItems && !DialogMode.Check(DMODE_CREATEOBJECTS))
						ListPtr->AddItem(CurItem.ListItems);

					ListPtr->SetFlags(VMENU_COMBOBOX);
					ListPtr->SetDialogStyle(DialogMode.Check(DMODE_WARNINGSTYLE));
				}
			}

			/*
				$ 15.10.2000 tran
				строка редакторирование должна иметь максимум в 511 символов
				выставляем максимальный размер в том случае, если он еще не выставлен
			*/

			// BUGBUG
			if (DialogEdit->GetMaxLength() == -1)
				DialogEdit->SetMaxLength(CurItem.nMaxLength ? (int)CurItem.nMaxLength : -1);

			DialogEdit->SetPosition(X1 + CurItem.X1, Y1 + CurItem.Y1, X1 + CurItem.X2, Y1 + CurItem.Y2);

			if (CurItem.Type == DI_PSWEDIT) {
				DialogEdit->SetPasswordMode(true);
				// ...Что бы небыло повадно... и для повыщения защиты, т.с.
				ItemFlags&= ~DIF_HISTORY;
			}

			if (Type == DI_FIXEDIT) {
				// DIF_HISTORY имеет более высокий приоритет, чем DIF_MASKEDIT
				if (ItemFlags & DIF_HISTORY)
					ItemFlags&= ~DIF_MASKEDIT;

				/*
					если DI_FIXEDIT, то курсор сразу ставится на замену...
					ай-ай - было недокументированно :-)
				*/
				DialogEdit->SetMaxLength(CurItem.X2 - CurItem.X1 + 1
						+ (CurItem.X2 == CurItem.X1 || !(ItemFlags & DIF_HISTORY) ? 0 : 1));
				DialogEdit->SetOvertypeMode(true);
				/*
					$ 12.08.2000 KM
					Если тип строки ввода DI_FIXEDIT и установлен флаг DIF_MASKEDIT
					и непустой параметр CurItem.Mask, то вызываем новую функцию
					для установки маски в объект DlgEdit.
				*/

				// Маска не должна быть пустой (строка из пробелов не учитывается)!
				if ((ItemFlags & DIF_MASKEDIT) && !CurItem.strMask.IsEmpty()) {
					RemoveExternalSpaces(CurItem.strMask);
					if (!CurItem.strMask.IsEmpty()) {
						DialogEdit->SetInputMask(CurItem.strMask);
					} else {
						ItemFlags&= ~DIF_MASKEDIT;
					}
				}
			} else {

				/*
					"мини-редактор"
					Последовательно определенные поля ввода (edit controls),
					имеющие этот флаг группируются в редактор с возможностью
					вставки и удаления строк
				*/
				if (!(ItemFlags & DIF_EDITOR)) {
					DialogEdit->SetEditBeyondEnd(false);

					if (!DialogMode.Check(DMODE_INITOBJECTS))
						DialogEdit->SetClearFlag(true);
				}
			}

			if (CurItem.Type == DI_COMBOBOX)
				DialogEdit->SetClearFlag(true);

			/*
				$ 01.08.2000 SVS
				Еже ли стоит флаг DIF_USELASTHISTORY и непустая строка ввода,
				то подстанавливаем первое значение из History
			*/
			if (CurItem.Type == DI_EDIT && (ItemFlags & DIF_HISTORY) && (ItemFlags & DIF_USELASTHISTORY))
				ProcessLastHistory(CurItem, -1);

			if (!(ItemFlags & DIF_HISTORY))
				ItemFlags&= ~DIF_MANUALADDHISTORY;	// сбросим нафиг.

			/*
				$ 18.03.2000 SVS
				Если это ComBoBox и данные не установлены, то берем из списка
				при условии, что хоть один из пунктов имеет Selected
			*/

			if (Type == DI_COMBOBOX && CurItem.strData.IsEmpty() && CurItem.ListItems) {
				FarListItem *ListItems = CurItem.ListItems->Items;
				unsigned Length = CurItem.ListItems->ItemsNumber;
				// CurItem.ListPtr->AddItem(CurItem.ListItems);

				for (unsigned J = 0; J < Length; J++) {
					if (ListItems[J].Flags & LIF_SELECTED) {
						const auto Text = NullToEmpty(ListItems[J].Text);

						if (ItemFlags & (DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND))
							HiText2Str(CurItem.strData, Text);
						else
							CurItem.strData = Text;

						break;
					}
				}
			}

			DialogEdit->SetCallbackState(false);
			DialogEdit->SetString(CurItem.strData);
			DialogEdit->SetCallbackState(true);

			if (Type == DI_FIXEDIT)
				DialogEdit->SetCurPos(0);

			if (Type != DI_MEMOEDIT) {
				// Для обычных строк отрубим постоянные блоки
				if (!(ItemFlags & DIF_EDITOR))
					DialogEdit->SetPersistentBlocks(Opt.Dialogs.EditBlock);

				DialogEdit->SetDelRemovesBlocks(Opt.Dialogs.DelRemovesBlocks);
			}

			if (ItemFlags & DIF_READONLY)
				DialogEdit->SetReadOnly(true);
		} else if (Type == DI_USERCONTROL) {
			if (!DialogMode.Check(DMODE_CREATEOBJECTS))
				CurItem.UCData = new DlgUserControl;
		}

		CurItem.Flags = ItemFlags;
	}

	// если будет редактор, то обязательно будет выделен.
	SelectOnEntry(FocusPos, true);
	// все объекты созданы!
	DialogMode.Set(DMODE_CREATEOBJECTS);
	return FocusPos;
}

const wchar_t *Dialog::GetDialogTitle()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	DialogItemEx *CurItemList = nullptr;

	for (int I = 0; I < ItemCount(); I++) {
		auto CurItem = &Item[I];

		// по первому попавшемуся "тексту" установим заголовок консоли!
		if (CurItem->Type == DI_TEXT || CurItem->Type == DI_DOUBLEBOX || CurItem->Type == DI_SINGLEBOX) {
			for (const wchar_t *Ptr = CurItem->strData; *Ptr; Ptr++)
				if (!IsSpace(*Ptr) && !IsEol(*Ptr))
					return Ptr;
		}
		else if (CurItem->Type == DI_LISTBOX && I == 0)
			CurItemList = CurItem;
	}

	if (CurItemList) {
		return CurItemList->ListPtr->GetPtrTitle();
	}

	return nullptr;		//""
}

void Dialog::ProcessLastHistory(DialogItemEx &CurItem, int MsgIndex)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	FARString &strData = CurItem.strData;

	if (strData.IsEmpty()) {
		FARString strRegKey = fmtSavedDialogHistory;
		strRegKey+= CurItem.strHistory;
		History::ReadLastItem(strRegKey.GetMB().c_str(), strData);

		if (MsgIndex != -1) {
			// обработка DM_SETHISTORY => надо пропустить изменение текста через
			// диалоговую функцию
			FarDialogItemData IData;
			IData.PtrData = const_cast<wchar_t *>(strData.CPtr());
			IData.PtrLength = (int)strData.GetLength();
			SendDlgMessage(DM_SETTEXT, MsgIndex, (LONG_PTR)&IData);
		}
	}
}

// Изменение координат и/или размеров итема диалога.
bool Dialog::SetItemRect(int ID, const SMALL_RECT *Rect)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (ID >= ItemCount())
		return FALSE;

	auto Clamp = [](int Val, int Min, int Max) {
		return std::clamp(Val, Min, Max);
	};

	const int Left   = Clamp(Rect->Left, 0, X2-X1);
	const int Top    = Clamp(Rect->Top, 0, Y2-Y1);
	const int Right  = Clamp(Rect->Right, Left, X2-X1);
	const int Bottom = Clamp(Rect->Bottom, Top, Y2-Y1);

	DialogItemEx &CurItem = Item[ID];
	int Type = CurItem.Type;
	CurItem.X1 = Left;
	CurItem.Y1 = Max(0, Top);

	if (FarIsEdit(Type)) {
		DlgEdit *DialogEdit = CurItem.GetEdit();
		CurItem.X2 = Right;
		CurItem.Y2 = (Type == DI_MEMOEDIT) ? Bottom : 0;
		const int edit_bottom = (Type == DI_MEMOEDIT) ? Bottom : Top;
		DialogEdit->SetPosition(X1 + Left, Y1 + Top, X1 + Right, Y1 + edit_bottom);
	}
	else if (Type == DI_LISTBOX) {
		CurItem.X2 = Right;
		CurItem.Y2 = Bottom;
		CurItem.ListPtr->SetPosition(X1 + Left, Y1 + Top, X1 + Right, Y1 + Bottom);
		CurItem.ListPtr->SetMaxHeight(CurItem.Y2 - CurItem.Y1 + 1);
	}

	switch (Type) {
		case DI_TEXT:
			CurItem.X2 = Right;
			CurItem.Y2 = 0;	// ???
			break;
		case DI_VTEXT:
		case DI_DOUBLEBOX:
		case DI_SINGLEBOX:
		case DI_USERCONTROL:
			CurItem.X2 = Right;
			CurItem.Y2 = Bottom;
			break;
	}

	if (DialogMode.Check(DMODE_SHOW)) {
		ShowDialog();
		ScrBuf.Flush();
	}

	return TRUE;
}

bool Dialog::GetItemRect(int I, SMALL_RECT &Rect)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (I >= ItemCount())
		return false;

	DialogItemEx &CurItem = Item[I];
	DWORD ItemFlags = CurItem.Flags;
	int Type = CurItem.Type;
	int Len = 0;
	Rect.Left = CurItem.X1;
	Rect.Top = CurItem.Y1;
	Rect.Right = CurItem.X2;
	Rect.Bottom = CurItem.Y2;

	switch (Type) {
		case DI_COMBOBOX:
		case DI_EDIT:
		case DI_FIXEDIT:
		case DI_PSWEDIT:
		case DI_LISTBOX:
		case DI_MEMOEDIT:
			break;
		default:
			Len = (ItemFlags & DIF_SHOWAMPERSAND)
							? (int)CurItem.strData.CellsCount()
							: HiStrCellsCount(CurItem.strData);
			break;
	}

	switch (Type) {
		case DI_TEXT:

			if (CurItem.X1 == -1)
				Rect.Left = (X2 - X1 + 1 - Len) / 2;

			if (Rect.Left < 0)
				Rect.Left = 0;

			if (CurItem.Y1 == -1)
				Rect.Top = (Y2 - Y1 + 1) / 2;

			if (Rect.Top < 0)
				Rect.Top = 0;

			Rect.Bottom = Rect.Top;

			if (!Rect.Right || Rect.Right == Rect.Left)
				Rect.Right = Rect.Left + Len - (Len ? 1 : 0);

			if (ItemFlags & (DIF_SEPARATOR | DIF_SEPARATOR2)) {
				Rect.Left = (!DialogMode.Check(DMODE_SMALLDIALOG) ? 3 : 0);				//???
				Rect.Right = X2 - X1 - (!DialogMode.Check(DMODE_SMALLDIALOG) ? 5 : 0);	//???
			}

			break;
		case DI_VTEXT:

			if (CurItem.X1 == -1)
				Rect.Left = (X2 - X1 + 1) / 2;

			if (Rect.Left < 0)
				Rect.Left = 0;

			if (CurItem.Y1 == -1)
				Rect.Top = (Y2 - Y1 + 1 - Len) / 2;

			if (Rect.Top < 0)
				Rect.Top = 0;

			Rect.Right = Rect.Left;

			// Rect.bottom=Rect.top+Len;
			if (!Rect.Bottom || Rect.Bottom == Rect.Top)
				Rect.Bottom = Rect.Top + Len - (Len ? 1 : 0);

#if defined(VTEXT_ADN_SEPARATORS)

			if (ItemFlags & (DIF_SEPARATOR | DIF_SEPARATOR2)) {
				Rect.Top = (!DialogMode.Check(DMODE_SMALLDIALOG) ? 1 : 0);					//???
				Rect.Bottom = Y2 - Y1 - (!DialogMode.Check(DMODE_SMALLDIALOG) ? 3 : 0);		//???
				break;
			}

#endif
			break;
		case DI_BUTTON:
			Rect.Bottom = Rect.Top;
			Rect.Right = Rect.Left + Len;
			break;
		case DI_CHECKBOX:
		case DI_RADIOBUTTON:
			Rect.Bottom = Rect.Top;
			Rect.Right = Rect.Left + Len + ((Type == DI_CHECKBOX) ? 4 : (ItemFlags & DIF_MOVESELECT ? 3 : 4));
			break;
		case DI_COMBOBOX:
		case DI_EDIT:
		case DI_FIXEDIT:
		case DI_PSWEDIT:
			Rect.Bottom = Rect.Top;
			break;
	}

	return true;
}

bool DialogItemEx::HasDropDownArrow() const
{
	return (!strHistory.IsEmpty() && (Flags & DIF_HISTORY) && Opt.Dialogs.EditHistory)
			|| (Type == DI_COMBOBOX && ListPtr && ListPtr->GetItemCount() > 0);
}

/*
	Private:
	Получение данных и удаление "редакторов"
*/
void Dialog::DeleteDialogObjects()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	for (const auto &CurItem: Item) {
		switch (CurItem.Type) {
			case DI_EDIT:
			case DI_FIXEDIT:
			case DI_PSWEDIT:
			case DI_COMBOBOX:
			case DI_MEMOEDIT:

				if (auto edit = CurItem.GetEdit())
					delete edit;
				[[fallthrough]];

			case DI_LISTBOX:

				if ((CurItem.Type == DI_COMBOBOX || CurItem.Type == DI_LISTBOX) && CurItem.ListPtr)
					delete CurItem.ListPtr;

				break;
			case DI_USERCONTROL:

				if (CurItem.UCData)
					delete CurItem.UCData;

				break;
		}

		if (CurItem.Flags & DIF_AUTOMATION)
			if (CurItem.AutoPtr)
				free(CurItem.AutoPtr);
	}
}

/*
	Public:
		Сохраняет значение из полей редактирования.
		При установленном флаге DIF_HISTORY, сохраняет данные в реестре.
*/
void Dialog::GetDialogObjectsData()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	int Type;

	for (auto &CurItem: Item) {
		DWORD IFlags = CurItem.Flags;

		switch (Type = CurItem.Type) {
			case DI_MEMOEDIT:
			case DI_EDIT:
			case DI_FIXEDIT:
			case DI_PSWEDIT:
			case DI_COMBOBOX: {
				if (DlgEdit *EditPtr = CurItem.GetEdit()) {
					// подготовим данные, получим данные
					FARString strData;
					EditPtr->GetString(strData);

					if (ExitCode >= 0 && (IFlags & DIF_HISTORY) && !(IFlags & DIF_MANUALADDHISTORY) &&		// при мануале не добавляем
							!CurItem.strHistory.IsEmpty() && Opt.Dialogs.EditHistory) {
						AddToEditHistory(strData, CurItem.strHistory);
					}

					/*
						$ 01.08.2000 SVS
						! В History должно заносится значение (для DIF_EXPAND...) перед
						расширением среды!
					*/

					/*
						$ 05.07.2000 SVS $
						Проверка - этот элемент предполагает расширение переменных среды?
						т.к. функция GetDialogObjectsData() может вызываться самостоятельно
						Но надо проверить!
					*/

					/*
						$ 04.12.2000 SVS
						! Для DI_PSWEDIT и DI_FIXEDIT обработка DIF_EDITEXPAND не нужна
						(DI_FIXEDIT допускается для случая если нету маски)
					*/

					if ((IFlags & DIF_EDITEXPAND) && Type != DI_PSWEDIT && Type != DI_FIXEDIT) {
						apiExpandEnvironmentStrings(strData, strData);
						// как бы грязный хак, нам нужно обновить строку чтоб отдавалась правильная строка
						// для различных DM_* после закрытия диалога, но ни в коем случае нельзя чтоб
						// высылался DN_EDITCHANGE для этого изменения, ибо диалог уже закрыт.
						EditPtr->SetCallbackState(false);
						EditPtr->SetString(strData);
						EditPtr->SetCallbackState(true);
					}

					CurItem.strData = strData;
				}

				break;
			}
			case DI_LISTBOX:
				/*
				if(CurItem.ListPtr)
				{
					CurItem.ListPos=CurItem.ListPtr->GetSelectPos();
					break;
				}
				*/
				break;
		}

		if (Type == DI_COMBOBOX || Type == DI_LISTBOX) {
			CurItem.ListPos = CurItem.ListPtr ? CurItem.ListPtr->GetSelectPos() : 0;
		}
	}
}

// Функция формирования и запроса цветов.
DWORD Dialog::CtlColorDlgItem(int ItemPos, uint64_t *Color)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	const auto &CurItem = Item[ItemPos];
	const int Type = CurItem.Type;
	const int Focus = CurItem.Focus;
	const int Default = CurItem.DefaultButton;
	const DWORD Flags = CurItem.Flags;

	const bool IsWarning = DialogMode.Check(DMODE_WARNINGSTYLE);
	const bool DisabledItem = (Flags & DIF_DISABLE) != 0;

	switch (Type) {
		case DI_SINGLEBOX:
		case DI_DOUBLEBOX: {

			// Title
			Color[0] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGBOXTITLE) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGBOXTITLE));
			// HiText
			Color[1] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGHIGHLIGHTBOXTITLE) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGHIGHLIGHTBOXTITLE));
			// Box
			Color[2] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGBOX) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGBOX));
			break;

/**
			if (Flags & DIF_SETCOLOR)
				Attr = Flags & DIF_COLORMASK;
			else {
				Attr = DialogMode.Check(DMODE_WARNINGSTYLE)
						? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGBOX)
						: (DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGBOX);
			}

			Attr = MAKELONG(MAKEWORD(FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
													? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGBOXTITLE)
													: (DisabledItem ? COL_DIALOGDISABLED
																	: COL_DIALOGBOXTITLE)),		// Title LOBYTE
									FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
													? (DisabledItem ? COL_WARNDIALOGDISABLED
																	: COL_WARNDIALOGHIGHLIGHTBOXTITLE)
													: (DisabledItem ? COL_DIALOGDISABLED
																	: COL_DIALOGHIGHLIGHTBOXTITLE))),		// HiText HIBYTE
					MAKEWORD(FarColorToReal(Attr),															// Box LOBYTE
							0)																				// HIBYTE
			);
			break;
**/
		}
#if defined(VTEXT_ADN_SEPARATORS)
		case DI_VTEXT:
#endif
		case DI_TEXT: {

			Color[0] = FarColorToReal((Flags & DIF_BOXCOLOR)? (IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGBOX) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGBOX)) : (IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGTEXT) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGTEXT)));
			// HiText
			Color[1] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGHIGHLIGHTTEXT) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGHIGHLIGHTTEXT));
			if (Flags & (DIF_SEPARATORUSER|DIF_SEPARATOR|DIF_SEPARATOR2))
			{
				// Box
				Color[2] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGBOX) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGBOX));
			}
			break;

/**
			if (Flags & DIF_SETCOLOR)
				Attr = Flags & DIF_COLORMASK;
			else {
				if (Flags & DIF_BOXCOLOR)
					Attr = DialogMode.Check(DMODE_WARNINGSTYLE)
							? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGBOX)
							: (DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGBOX);
				else
					Attr = DialogMode.Check(DMODE_WARNINGSTYLE)
							? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGTEXT)
							: (DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGTEXT);
			}

			Attr = MAKELONG(MAKEWORD(FarColorToReal(Attr),
									FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
													? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGHIGHLIGHTTEXT)
													: (DisabledItem ? COL_DIALOGDISABLED
																	: COL_DIALOGHIGHLIGHTTEXT)		// HIBYTE HiText
											)),
					((Flags & (DIF_SEPARATORUSER | DIF_SEPARATOR | DIF_SEPARATOR2)) ? (
								MAKEWORD(FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
														? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGBOX)
														: (DisabledItem ? COL_DIALOGDISABLED
																		: COL_DIALOGBOX)	// Box LOBYTE
												),
										0))
																					: 0));
			break;
**/
		}
#if 0
#if !defined(VTEXT_ADN_SEPARATORS)
		case DI_VTEXT: {
			if (Flags & DIF_BOXCOLOR)
				Attr = DialogMode.Check(DMODE_WARNINGSTYLE)
						? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGBOX)
						: (DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGBOX);
			else if (Flags & DIF_SETCOLOR)
				Attr = (Flags & DIF_COLORMASK);
			else
				Attr = (DialogMode.Check(DMODE_WARNINGSTYLE)
								? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGTEXT)
								: (DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGTEXT));

			Attr = MAKEWORD(MAKEWORD(FarColorToReal(Attr), 0), MAKEWORD(0, 0));
			break;
		}
#endif
#endif
		case DI_CHECKBOX:
		case DI_RADIOBUTTON: {

			Color[0] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGTEXT) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGTEXT));
			// HiText
			Color[1] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:COL_WARNDIALOGHIGHLIGHTTEXT) : (DisabledItem?COL_DIALOGDISABLED:COL_DIALOGHIGHLIGHTTEXT));
			break;
/**
			if (Flags & DIF_SETCOLOR)
				Attr = (Flags & DIF_COLORMASK);
			else
				Attr = (DialogMode.Check(DMODE_WARNINGSTYLE)
								? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGTEXT)
								: (DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGTEXT));

			Attr = MAKEWORD(FarColorToReal(Attr),
					FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
									? (DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGHIGHLIGHTTEXT)
									: (DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGHIGHLIGHTTEXT)));		// HiText
			break;
**/
		}
		case DI_BUTTON: {

			if (Focus)
			{
				SetCursorType(false, 10);
				// TEXT
				Color[0] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:(Default?COL_WARNDIALOGSELECTEDDEFAULTBUTTON:COL_WARNDIALOGSELECTEDBUTTON)) : (DisabledItem?COL_DIALOGDISABLED:(Default?COL_DIALOGSELECTEDDEFAULTBUTTON:COL_DIALOGSELECTEDBUTTON)));
				// HiText
				Color[1] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:(Default?COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON:COL_WARNDIALOGHIGHLIGHTSELECTEDBUTTON)) : (DisabledItem?COL_DIALOGDISABLED:(Default?COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON:COL_DIALOGHIGHLIGHTSELECTEDBUTTON)));
			}
			else
			{
				// TEXT
				Color[0] = FarColorToReal(IsWarning?
						(DisabledItem?COL_WARNDIALOGDISABLED:(Default?COL_WARNDIALOGDEFAULTBUTTON:COL_WARNDIALOGBUTTON)):
						(DisabledItem?COL_DIALOGDISABLED:(Default?COL_DIALOGDEFAULTBUTTON:COL_DIALOGBUTTON)));
				// HiText
				Color[1] = FarColorToReal(IsWarning? (DisabledItem?COL_WARNDIALOGDISABLED:(Default?COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON:COL_WARNDIALOGHIGHLIGHTBUTTON)) : (DisabledItem?COL_DIALOGDISABLED:(Default?COL_DIALOGHIGHLIGHTDEFAULTBUTTON:COL_DIALOGHIGHLIGHTBUTTON)));
			}
			break;

/**
			if (Focus) {
				SetCursorType(false, 10);
				Attr = MAKEWORD((Flags & DIF_SETCOLOR)
								? (Flags & DIF_COLORMASK)
								: FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
												? (DisabledItem ? COL_WARNDIALOGDISABLED
																: (Default ? COL_WARNDIALOGSELECTEDDEFAULTBUTTON
																			: COL_WARNDIALOGSELECTEDBUTTON))
												: (DisabledItem ? COL_DIALOGDISABLED
																: (Default ? COL_DIALOGSELECTEDDEFAULTBUTTON
																			: COL_DIALOGSELECTEDBUTTON))),		// TEXT
						FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
										? (DisabledItem ? COL_WARNDIALOGDISABLED
														: (Default ? COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON
																	: COL_WARNDIALOGHIGHLIGHTSELECTEDBUTTON))
										: (DisabledItem ? COL_DIALOGDISABLED
														: (Default ? COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON
																	: COL_DIALOGHIGHLIGHTSELECTEDBUTTON))));	// HiText
			} else {
				Attr = MAKEWORD((Flags & DIF_SETCOLOR)
								? (Flags & DIF_COLORMASK)
								: FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
												? (DisabledItem ? COL_WARNDIALOGDISABLED
																: (Default ? COL_WARNDIALOGDEFAULTBUTTON
																			: COL_WARNDIALOGBUTTON))
												: (DisabledItem ? COL_DIALOGDISABLED
																: (Default ? COL_DIALOGDEFAULTBUTTON
																			: COL_DIALOGBUTTON))),		// TEXT
						FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE)
										? (DisabledItem ? COL_WARNDIALOGDISABLED
														: (Default ? COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON
																	: COL_WARNDIALOGHIGHLIGHTBUTTON))
										: (DisabledItem ? COL_DIALOGDISABLED
														: (Default ? COL_DIALOGHIGHLIGHTDEFAULTBUTTON
																	: COL_DIALOGHIGHLIGHTBUTTON))));	// HiText
			}

			break;
**/
		}
		case DI_EDIT:
		case DI_FIXEDIT:
		case DI_PSWEDIT:
		case DI_COMBOBOX:
		case DI_MEMOEDIT: {

			if (Type == DI_COMBOBOX && (Flags & DIF_DROPDOWNLIST))
			{
				if (IsWarning)
				{
					// Text
					Color[0] = FarColorToReal(DisabledItem? COL_WARNDIALOGEDITDISABLED : Focus? COL_WARNDIALOGEDITSELECTED : COL_WARNDIALOGEDIT);
					// Select
					Color[1] = Color[0];
					// Unchanged
					Color[2] = FarColorToReal(DisabledItem? COL_WARNDIALOGEDITDISABLED : Focus? COL_WARNDIALOGEDITSELECTED : COL_WARNDIALOGEDITUNCHANGED);
					// History
					Color[3] = FarColorToReal(DisabledItem? COL_WARNDIALOGDISABLED : COL_WARNDIALOGTEXT);
				}
				else
				{
					// Text
					Color[0] = FarColorToReal(DisabledItem? COL_DIALOGEDITDISABLED : Focus? COL_DIALOGEDITSELECTED : COL_DIALOGEDIT);
					// Select
					Color[1] = Color[0];
					// Unchanged
					Color[2] = FarColorToReal(DisabledItem? COL_DIALOGEDITDISABLED :  Focus? COL_DIALOGEDITSELECTED : COL_DIALOGEDITUNCHANGED);
					// History
					Color[3] = FarColorToReal(DisabledItem? COL_DIALOGDISABLED : COL_DIALOGTEXT);
				}
			}
			else
			{
				if (IsWarning)
				{
					// Text
					Color[0] = FarColorToReal(DisabledItem? COL_WARNDIALOGEDITDISABLED : Flags & DIF_NOFOCUS? COL_WARNDIALOGEDITUNCHANGED : COL_WARNDIALOGEDIT);
					// Select
					Color[1] = FarColorToReal(DisabledItem? COL_WARNDIALOGEDITDISABLED : COL_WARNDIALOGEDITSELECTED);
					// Unchanged
					Color[2] = FarColorToReal(DisabledItem? COL_WARNDIALOGEDITDISABLED : COL_WARNDIALOGEDITUNCHANGED);
					// History
					Color[3] = FarColorToReal(DisabledItem? COL_WARNDIALOGDISABLED : COL_WARNDIALOGTEXT);
				}
				else
				{
					// Text
					Color[0] = FarColorToReal(DisabledItem? COL_DIALOGEDITDISABLED
							: Type == DI_MEMOEDIT ? COL_EDITORTEXT
							: Flags & DIF_NOFOCUS ? COL_DIALOGEDITUNCHANGED : COL_DIALOGEDIT);
					// Select
					Color[1] = FarColorToReal(DisabledItem? COL_DIALOGEDITDISABLED
							: Type == DI_MEMOEDIT ? COL_EDITORSELECTEDTEXT : COL_DIALOGEDITSELECTED);
					// Unchanged
					Color[2] = FarColorToReal(DisabledItem ? COL_DIALOGEDITDISABLED : COL_DIALOGEDITUNCHANGED);
					// History
					Color[3] = FarColorToReal(DisabledItem? COL_DIALOGDISABLED : COL_DIALOGTEXT);
				}
			}
			break;
/**
			if (Type == DI_COMBOBOX && (Flags & DIF_DROPDOWNLIST)) {
				if (DialogMode.Check(DMODE_WARNINGSTYLE))
					Attr = MAKELONG(MAKEWORD(		// LOWORD
													//  LOLO (Text)
											FarColorToReal(DisabledItem ? COL_WARNDIALOGEDITDISABLED
																		: COL_WARNDIALOGEDIT),
											// LOHI (Select)
											FarColorToReal(DisabledItem ? COL_DIALOGEDITDISABLED
																		: COL_DIALOGEDITSELECTED)),
							MAKEWORD(		// HIWORD
											//  HILO (Unchanged)
									FarColorToReal(DisabledItem ? COL_WARNDIALOGEDITDISABLED
																: COL_DIALOGEDITUNCHANGED),		//???
									// HIHI (History)
									FarColorToReal(
											DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGTEXT)));
				else
					Attr = MAKELONG(MAKEWORD(		// LOWORD
													//  LOLO (Text)
											FarColorToReal(DisabledItem
															? COL_DIALOGEDITDISABLED
															: (!Focus ? COL_DIALOGEDIT
																		: COL_DIALOGEDITSELECTED)),
											// LOHI (Select)
											FarColorToReal(DisabledItem
															? COL_DIALOGEDITDISABLED
															: (!Focus ? COL_DIALOGEDIT
																		: COL_DIALOGEDITSELECTED))),
							MAKEWORD(		// HIWORD
											//  HILO (Unchanged)
									FarColorToReal(DisabledItem ? COL_DIALOGEDITDISABLED
																: COL_DIALOGEDITUNCHANGED),		//???
									// HIHI (History)
									FarColorToReal(DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGTEXT)));
			} else {
				if (DialogMode.Check(DMODE_WARNINGSTYLE))
					Attr = MAKELONG(MAKEWORD(		// LOWORD
													//  LOLO (Text)
											FarColorToReal(DisabledItem
															? COL_WARNDIALOGEDITDISABLED
															: (Flags & DIF_NOFOCUS ? COL_DIALOGEDITUNCHANGED
																				: COL_WARNDIALOGEDIT)),
											// LOHI (Select)
											FarColorToReal(DisabledItem ? COL_DIALOGEDITDISABLED
																		: COL_DIALOGEDITSELECTED)),
							MAKEWORD(		// HIWORD
											//  HILO (Unchanged)
									FarColorToReal(DisabledItem ? COL_WARNDIALOGEDITDISABLED
																: COL_DIALOGEDITUNCHANGED),		//???
									// HIHI (History)
									FarColorToReal(
											DisabledItem ? COL_WARNDIALOGDISABLED : COL_WARNDIALOGTEXT)));
				else
					Attr = MAKELONG(MAKEWORD(		// LOWORD
													//  LOLO (Text)
											FarColorToReal(DisabledItem
															? COL_DIALOGEDITDISABLED
															: (Flags & DIF_NOFOCUS ? COL_DIALOGEDITUNCHANGED
																				: COL_DIALOGEDIT)),
											// LOHI (Select)
											FarColorToReal(DisabledItem ? COL_DIALOGEDITDISABLED
																		: COL_DIALOGEDITSELECTED)),
							MAKEWORD(		// HIWORD
											//  HILO (Unchanged)
									FarColorToReal(DisabledItem ? COL_DIALOGEDITDISABLED
																: COL_DIALOGEDITUNCHANGED),		//???
									// HIHI (History)
									FarColorToReal(DisabledItem ? COL_DIALOGDISABLED : COL_DIALOGTEXT)));
			}

			break;
**/
		}
		case DI_LISTBOX: {
			Item[ItemPos].ListPtr->SetColors(nullptr);
			return 0;
		}
		default: {
			return 0;
		}
	}

	++InCtlColorDlgItem;
	DWORD out = DlgProc(DN_CTLCOLORDLGITEM, ItemPos, (LONG_PTR)Color);
	--InCtlColorDlgItem;
	return out;
}

/*
static void SetColorNormal(DWORD Attr, const std::unique_ptr<DialogItemTrueColors> &TrueColors)
{
	ComposeAndSetColor(Attr & 0xff, TrueColors ? &TrueColors->Normal : nullptr);
}

static void SetColorFrame(DWORD Attr, const std::unique_ptr<DialogItemTrueColors> &TrueColors)
{
	ComposeAndSetColor(LOBYTE(HIWORD(Attr)), TrueColors ? &TrueColors->Frame : nullptr);
}
*/

//////////////////////////////////////////////////////////////////////////
/*
	Private:
		Отрисовка элементов диалога на экране.
*/
void Dialog::ShowDialog(int ID)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (Locked())
		return;

	FARString strStr;
	int X, Y;
	int DrawItemCount;
	uint64_t ItemColor[DLG_ITEM_MAX_CUST_COLORS] = {};

	// Если не разрешена отрисовка, то вываливаем.
	if (IsEnableRedraw < 1 ||						// разрешена прорисовка ?
			ID >= ItemCount() ||					// а номер в рамках дозволенного?
			DialogMode.Check(DMODE_DRAWING) ||		// диалог рисуется?
			!DialogMode.Check(DMODE_SHOW) ||		// если не видим, то и не отрисовываем.
			!DialogMode.Check(DMODE_INITOBJECTS))
		return;

	DialogMode.Set(DMODE_DRAWING);	// диалог рисуется!!!
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);

	if (ID == -1)		// рисуем все?
	{
		// Перед прорисовкой диалога посылаем сообщение в обработчик
		if (!DlgProc(DN_DRAWDIALOG, 0, 0)) {
			DialogMode.Clear(DMODE_DRAWING);	// конец отрисовки диалога!!!
			return;
		}

		// перед прорисовкой подложки окна диалога
		if (!DialogMode.Check(DMODE_NODRAWSHADOW))
			Shadow(DialogMode.Check(DMODE_FULLSHADOW));	// "наводим" тень

		if (!DialogMode.Check(DMODE_NODRAWPANEL)) {

			uint64_t Color[DLG_ITEM_MAX_CUST_COLORS] = {};

			Color[0] = FarColorToReal(DialogMode.Check(DMODE_WARNINGSTYLE) ? COL_WARNDIALOGTEXT:COL_DIALOGTEXT);
			DlgProc(DN_CTLCOLORDIALOG, 0, (LONG_PTR)Color);
			SetScreen(X1, Y1, X2, Y2, L' ', Color[0]);
		}

		ID = 0;
		DrawItemCount = ItemCount();
	} else {
		DrawItemCount = ID + 1;
	}

	/*
		IFlags.Set(DIMODE_REDRAW)
		TODO:
		если рисуется контрол и по Z-order`у он пересекается с
		другим контролом (по координатам), то для "позднего"
		контрола тоже нужна прорисовка.
	*/
	{
		bool CursorVisible = false;
		DWORD CursorSize = 0;

		if (FocusPos != ID) {
			if (Item[FocusPos].Type == DI_USERCONTROL && Item[FocusPos].UCData->CursorPos.X != -1
					&& Item[FocusPos].UCData->CursorPos.Y != -1) {
				CursorVisible = Item[FocusPos].UCData->CursorVisible;
				CursorSize = Item[FocusPos].UCData->CursorSize;
			}
		}

		SetCursorType(CursorVisible, CursorSize);
	}

	for (auto I = ID; I < DrawItemCount; I++) {
		DialogItemEx &CurItem = Item[I];

		if (CurItem.Flags & DIF_HIDDEN)
			continue;

		/*
			$ 28.07.2000 SVS
			Перед прорисовкой каждого элемента посылаем сообщение
			посредством функции SendDlgMessage - в ней делается все!
		*/
		if (!SendDlgMessage(DN_DRAWDLGITEM, I, 0))
			continue;

		int LenText;
		short CX1 = CurItem.X1;
		short CY1 = CurItem.Y1;
		short CX2 = CurItem.X2;
		short CY2 = CurItem.Y2;

		if (CX2 > X2 - X1)
			CX2 = X2 - X1;

		if (CY2 > Y2 - Y1)
			CY2 = Y2 - Y1;

		short CW = CX2 - CX1 + 1;
		short CH = CY2 - CY1 + 1;

		CtlColorDlgItem(I, ItemColor);

		for (size_t g = 0; g < DLG_ITEM_MAX_CUST_COLORS; g++) {
			if (CurItem.customItemColor[g])
				ItemColor[g] = CurItem.customItemColor[g];
		}

		switch (CurItem.Type) {
				/* ***************************************************************** */
			case DI_SINGLEBOX:
			case DI_DOUBLEBOX: {
				bool IsDrawTitle = true;
				GotoXY(X1 + CX1, Y1 + CY1);
//				SetColorFrame(Attr, CurItem.TrueColors);
				SetColor(ItemColor[2]);

				if (CY1 == CY2) {
					DrawLine(CX2 - CX1 + 1, CurItem.Type == DI_SINGLEBOX ? 8 : 9);		//???
				} else if (CX1 == CX2) {
					DrawLine(CY2 - CY1 + 1, CurItem.Type == DI_SINGLEBOX ? 10 : 11);
					IsDrawTitle = false;
				} else {
					Box(X1 + CX1, Y1 + CY1, X1 + CX2, Y1 + CY2, ItemColor[2],
							(CurItem.Type == DI_SINGLEBOX) ? SINGLE_BOX : DOUBLE_BOX);
				}

				if (!CurItem.strData.IsEmpty() && IsDrawTitle) {
					// ! Пусть диалог сам заботится о ширине собственного заголовка.
					strStr = CurItem.strData;
					TruncStrFromEnd(strStr, CW - 2);	// 5 ???
					LenText = LenStrItem(I, strStr);

					if (LenText < CW - 2 && !strStr.Begins(L' ') ) {
						strStr.Insert(0, L' ');
						LenText = LenStrItem(I, strStr);
					}

					if (LenText < CW - 2 && !strStr.Ends(L' ') ) { // пробел после текста заголовка и рамкой
						strStr.Append(L' ');
						LenText = LenStrItem(I, strStr);
					}

					X = X1 + CX1 + (CW - LenText) / 2;

					if ((CurItem.Flags & DIF_LEFTTEXT) && X1 + CX1 + 1 < X)
						X = X1 + CX1 + 1;

//					SetColorNormal(Attr, CurItem.TrueColors);
					SetColor(ItemColor[0]);
					GotoXY(X, Y1 + CY1);

					if (CurItem.Flags & DIF_SHOWAMPERSAND)
						Text(strStr);
					else
						HiText(strStr, ItemColor[1]);

/**
					if (CurItem.Flags & DIF_SHOWAMPERSAND)
						Text(strStr);
					else if (CurItem.TrueColors)
						HiText(strStr, ComposeColor(ItemColor[1] & 0xFF, &CurItem.TrueColors->Hilighted));
					else
						HiText(strStr, ItemColor[1]);
**/

				}

				break;
			}
			/* ***************************************************************** */
			case DI_TEXT: {
				strStr = CurItem.strData;
				LenText = LenStrItem(I, strStr);

				if (!(CurItem.Flags & (DIF_SEPARATORUSER | DIF_SEPARATOR | DIF_SEPARATOR2))
						&& (CurItem.Flags & DIF_CENTERTEXT) && CX1 != -1)
				{
					if (LenText < CX2 - CX1 + 1) // center text if it's length < calculated length
						LenText = LenStrItem(I, CenterStr(strStr, strStr, CX2 - CX1 + 1));
				}

				X = (CX1 == -1 || (CurItem.Flags & (DIF_SEPARATOR | DIF_SEPARATOR2)))
						? (X2 - X1 + 1 - LenText) / 2
						: CX1;
				Y = (CY1 == -1) ? (Y2 - Y1 + 1) / 2 : CY1;

				if (X < 0)
					X = 0;

				if (X1 + X + LenText > X2)
					strStr.TruncateByCells(X2 - X1 - X);

				// нужно ЭТО
				// SetScreen(X1+CX1,Y1+CY1,X1+CX2,Y1+CY2,' ',Attr&0xFF);
				// вместо этого:
				if (CX1 > -1 && CX2 > CX1
						&& !(CurItem.Flags & (DIF_SEPARATORUSER | DIF_SEPARATOR | DIF_SEPARATOR2))) {		// половинчатое решение

					int CntChr = CX2 - CX1 + 1;
//					SetColorNormal(Attr, CurItem.TrueColors);
					SetColor(ItemColor[0]);
					GotoXY(X1 + X, Y1 + Y);

					if (X1 + X + CntChr - 1 > X2)
						CntChr = X2 - (X1 + X) + 1;

					FS << fmt::Cells() << fmt::Expand(CntChr) << L"";

					if (CntChr < LenText)
						strStr.TruncateByCells(CntChr);
				}

///					if (CX1 > -1 && CX2 > CX1 && !(Item.Flags & (DIF_SEPARATORUSER|DIF_SEPARATOR|DIF_SEPARATOR2))) //половинчатое решение
///					{
///						SetScreen({ m_Where.left + CX1, m_Where.top + Y, m_Where.left + CX2, m_Where.top + Y }, L' ', ItemColor[0]);
						/*
						int CntChr=CX2-CX1+1;
						SetColor(ItemColor[0]);
						GotoXY(X1+X, Y1+Y);

						if (X1+X+CntChr-1 > X2)
							CntChr=X2-(X1+X)+1;

						Text(string(CntChr, L' '));

						if (CntChr < LenText)
							strStr.SetLength(CntChr);
						*/
///					}

				if (CurItem.Flags & (DIF_SEPARATORUSER | DIF_SEPARATOR | DIF_SEPARATOR2)) {
//					SetColorFrame(Attr, CurItem.TrueColors);
					SetColor(ItemColor[2]);
					GotoXY(X1
									+ ((CurItem.Flags & DIF_SEPARATORUSER)
													? X
													: (!DialogMode.Check(DMODE_SMALLDIALOG) ? 3 : 0)),
							Y1 + Y);	//????
					ShowUserSeparator((CurItem.Flags & DIF_SEPARATORUSER)
									? X2 - X1 + 1
									: RealWidth - (!DialogMode.Check(DMODE_SMALLDIALOG) ? 6 : 0 /* -1 */),
							(CurItem.Flags & DIF_SEPARATORUSER)
									? 12
									: (CurItem.Flags & DIF_SEPARATOR2 ? 3 : 1),
							CurItem.strMask);
				}

//				SetColorNormal(Attr, CurItem.TrueColors);
				SetColor(ItemColor[0]);
				GotoXY(X1 + X, Y1 + Y);

				if (CurItem.Flags & DIF_SHOWAMPERSAND) {
					// MessageBox(0, strStr, strStr, MB_OK);
					Text(strStr);
				} else {
					// MessageBox(0, strStr, strStr, MB_OK);
					HiText(strStr, ItemColor[1]);
				}

				break;
			}
			/* ***************************************************************** */
			case DI_VTEXT: {
				strStr = CurItem.strData;
				LenText = LenStrItem(I, strStr);

				if (!(CurItem.Flags & (DIF_SEPARATORUSER | DIF_SEPARATOR | DIF_SEPARATOR2))
						&& (CurItem.Flags & DIF_CENTERTEXT) && CY1 != -1)
					LenText = StrLength(CenterStr(strStr, strStr, CY2 - CY1 + 1));

				X = (CX1 == -1) ? (X2 - X1 + 1) / 2 : CX1;
				Y = (CY1 == -1 || (CurItem.Flags & (DIF_SEPARATOR | DIF_SEPARATOR2)))
						? (Y2 - Y1 + 1 - LenText) / 2
						: CY1;

				if (Y < 0)
					Y = 0;

				if ((CY2 <= 0) || (CY2 < CY1))
					CH = LenStrItem(I, strStr);

				if (Y1 + Y + LenText > Y2) {
					int tmpCH = ObjHeight;

					if (CH < ObjHeight)
						tmpCH = CH + 1;

					strStr.TruncateByCells(tmpCH - 1);
				}

				// нужно ЭТО
				// SetScreen(X1+CX1,Y1+CY1,X1+CX2,Y1+CY2,' ',Attr&0xFF);
				// вместо этого:
				if (CY1 > -1 && CY2 > 0 && CY2 > CY1
						&& !(CurItem.Flags & (DIF_SEPARATORUSER | DIF_SEPARATOR | DIF_SEPARATOR2))) {		// половинчатое решение

					int CntChr = CY2 - CY1 + 1;
					SetColor(ItemColor[0]);
//					SetColorNormal(Attr, CurItem.TrueColors);
					GotoXY(X1 + X, Y1 + Y);

					if (Y1 + Y + CntChr - 1 > Y2)
						CntChr = Y2 - (Y1 + Y) + 1;

					vmprintf(L"%*ls", CntChr, L"");
				}

				// нужно ЭТО
				//SetScreen(X1+CX1,Y1+CY1,X1+CX2,Y1+CY2,' ',Attr&0xFF);
				// вместо этого:
///				if (CY1 > -1 && CY2 > CY1 && !(Item.Flags & (DIF_SEPARATORUSER|DIF_SEPARATOR|DIF_SEPARATOR2))) //половинчатое решение
///				{
///					SetScreen({ m_Where.left + X, m_Where.top + CY1, m_Where.left + X, m_Where.top + CY2 }, L' ', ItemColor[0]);
					/*
					int CntChr=CY2-CY1+1;
					SetColor(ItemColor[0]);
					GotoXY(X1+X,Y1+Y);

					if (Y1+Y+CntChr-1 > Y2)
						CntChr=Y2-(Y1+Y)+1;

					vmprintf(L"%*s",CntChr,L"");
					*/
///				}




#if defined(VTEXT_ADN_SEPARATORS)

				if (CurItem.Flags & (DIF_SEPARATORUSER | DIF_SEPARATOR | DIF_SEPARATOR2)) {
//					SetColorFrame(Attr, CurItem.TrueColors);
					SetColor(ItemColor[2]);
					GotoXY(X1 + X,
							Y1
									+ ((CurItem.Flags & DIF_SEPARATORUSER)
													? Y
													: (!DialogMode.Check(DMODE_SMALLDIALOG) ? 1 : 0)));		//????
					ShowUserSeparator((CurItem.Flags & DIF_SEPARATORUSER)
									? Y2 - Y1 + 1
									: RealHeight - (!DialogMode.Check(DMODE_SMALLDIALOG) ? 2 : 0),
							(CurItem.Flags & DIF_SEPARATORUSER)
									? 13
									: (CurItem.Flags & DIF_SEPARATOR2 ? 7 : 5),
							CurItem.strMask);
				}

#endif
				SetColor(ItemColor[0]);
//				SetColorNormal(Attr, CurItem.TrueColors);
				GotoXY(X1 + X, Y1 + Y);

				if (CurItem.Flags & DIF_SHOWAMPERSAND)
					VText(strStr);
				else
					HiText(strStr, ItemColor[1], true);

				break;
			}
			/* ***************************************************************** */
			case DI_CHECKBOX:
			case DI_RADIOBUTTON: {
//				SetColorNormal(Attr, CurItem.TrueColors);
				SetColor(ItemColor[0]);
				GotoXY(X1 + CX1, Y1 + CY1);

				if (CurItem.Type == DI_CHECKBOX) {
					const wchar_t Check[] = {L'[',
							(CurItem.Selected ? (((CurItem.Flags & DIF_3STATE) && CurItem.Selected == 2)
												? *Msg::CheckBox2State
												: L'x')
												: L' '),
							L']', L'\0'};
					strStr = Check;

					if (CurItem.strData.GetLength())
						strStr+= L" ";
				} else {
					wchar_t Dot[] = {L' ', CurItem.Selected ? L'\x2022' : L' ', L' ', L'\0'};

					if (CurItem.Flags & DIF_MOVESELECT) {
						strStr = Dot;
					} else {
						Dot[0] = L'(';
						Dot[2] = L')';
						strStr = Dot;

						if (CurItem.strData.GetLength())
							strStr+= L" ";
					}
				}

				strStr+= CurItem.strData;
				LenText = LenStrItem(I, strStr);

				if (X1 + CX1 + LenText > X2)
					strStr.TruncateByCells(ObjWidth - 1);

				if (CurItem.Flags & DIF_SHOWAMPERSAND)
					Text(strStr);
				else
					HiText(strStr, ItemColor[1]);

				if (CurItem.Focus) {
					// Отключение мигающего курсора при перемещении диалога
					if (!DialogMode.Check(DMODE_DRAGGED))
						SetCursorType(true, -1);

					MoveCursor(X1 + CX1 + 1, Y1 + CY1);
				}

				break;
			}
			/* ***************************************************************** */
			case DI_BUTTON: {
				strStr = CurItem.strData;
				SetColor(ItemColor[0]);
//				SetColorNormal(Attr, CurItem.TrueColors);
				GotoXY(X1 + CX1, Y1 + CY1);

				if (CurItem.Flags & DIF_SHOWAMPERSAND)
					Text(strStr);
				else
					HiText(strStr, ItemColor[1]);
//					HiText(strStr, HIBYTE(LOWORD(Attr)));

				if (CurItem.Flags & DIF_SETSHIELD) {
					int startx = X1 + CX1 + (CurItem.Flags & DIF_NOBRACKETS ? 0 : 2);
					ScrBuf.ApplyColor(startx, Y1 + CY1, startx + 1, Y1 + CY1, 0xE9);
				}
				break;
			}
			/* ***************************************************************** */
			case DI_EDIT:
			case DI_FIXEDIT:
			case DI_PSWEDIT:
			case DI_COMBOBOX:
			case DI_MEMOEDIT: {
				DlgEdit *EditPtr = CurItem.GetEdit();

				if (!EditPtr)
					break;

//				EditPtr->SetObjectColor(Attr & 0xFF, HIBYTE(LOWORD(Attr)), LOBYTE(HIWORD(Attr)));
				EditPtr->SetObjectColor(ItemColor[0],ItemColor[1],ItemColor[2]);

				if (CurItem.Focus) {
					// Отключение мигающего курсора при перемещении диалога
					if (!DialogMode.Check(DMODE_DRAGGED))
						SetCursorType(true, -1);

					EditPtr->Show();
				} else {
					EditPtr->FastShow();
					EditPtr->SetLeftPos(0);
				}

				// Отключение мигающего курсора при перемещении диалога
				if (DialogMode.Check(DMODE_DRAGGED))
					SetCursorType(false, 0);

				if (CurItem.HasDropDownArrow()) {
					int EditX1, EditY1, EditX2, EditY2;
					EditPtr->GetPosition(EditX1, EditY1, EditX2, EditY2);
					// Text((CurItem.Type == DI_COMBOBOX?"\x1F":"\x19"));
					Text(EditX2 + 1, EditY1, ItemColor[3], L"\x2193");
				}

				if (CurItem.Type == DI_COMBOBOX && GetDropDownOpened() && CurItem.ListPtr->IsVisible())		// need redraw VMenu?
				{
					CurItem.ListPtr->Hide();
					CurItem.ListPtr->Show();
				}

				break;
			}
			/* ***************************************************************** */
			case DI_LISTBOX: {
				if (CurItem.ListPtr) {
					// Перед отрисовкой спросим об изменении цветовых атрибутов
					uint64_t RealColors[VMENU_COLOR_COUNT];
					FarListColors ListColors = {0};
					ListColors.ColorCount = VMENU_COLOR_COUNT;
					ListColors.Colors = RealColors;
					CurItem.ListPtr->GetColors(&ListColors);

					if (DlgProc(DN_CTLCOLORDLGLIST, I, (LONG_PTR)&ListColors))
						CurItem.ListPtr->SetColors(&ListColors);

					// Курсор запоминаем...
					bool CursorVisible = false;
					DWORD CursorSize = 0;
					GetCursorType(CursorVisible, CursorSize);
					CurItem.ListPtr->Show();

					// .. а теперь восстановим!
					if (FocusPos != I)
						SetCursorType(CursorVisible, CursorSize);
				}

				break;
			}
			/* 01.08.2000 SVS $ */
			/* ***************************************************************** */
			case DI_USERCONTROL:
				if (CurItem.VBuf != INVALID_HANDLE_VALUE) { // a temporary solution (2025-11-11)
					if (CurItem.Reserved > 0xff) {
						PutText(X1 + CX1, Y1 + CY1, X1 + CX2, Y1 + CY2, CurItem.VBuf);
					} else { // fill with spaces of given attibutes
						CHAR_INFO ci{};
						CI_SET_WCHAR(ci, L' ');
						CI_SET_ATTR(ci, CurItem.Reserved);
						for (auto Y = Y1 + CY1; Y <= Y1 + CY2; ++Y) {
							for (auto X = X1 + CX1; X <= X1 + CX2; ++X) {
								PutText(X, Y, X, Y, &ci);
							}
						}
					}
				}
				// не забудем переместить курсор, если он позиционирован.
				if (FocusPos == I) {
					if (CurItem.UCData->CursorPos.X != -1 && CurItem.UCData->CursorPos.Y != -1) {
						MoveCursor(CurItem.UCData->CursorPos.X + CX1 + X1,
								CurItem.UCData->CursorPos.Y + CY1 + Y1);
						SetCursorType(CurItem.UCData->CursorVisible, CurItem.UCData->CursorSize);
					} else
						SetCursorType(false, -1);
				}


				break;	// уже нарисовали :-)))
				/* ***************************************************************** */
				//.........
		}	// end switch(...
	}		// end for (I=...

	// КОСТЫЛЬ!
	// но работает ;-)
	for (const auto &CurItem: Item) {
		if (CurItem.ListPtr && GetDropDownOpened() && CurItem.ListPtr->IsVisible()) {
			if ((CurItem.Type == DI_COMBOBOX)
					|| ((CurItem.Type == DI_EDIT || CurItem.Type == DI_FIXEDIT)
							&& !(CurItem.Flags & DIF_HIDDEN) && (CurItem.Flags & DIF_HISTORY))) {
				CurItem.ListPtr->Show();
			}
		}
	}

	// Включим индикатор перемещения...
	if (!DialogMode.Check(DMODE_DRAGGED))		// если диалог таскается
	{
		/*
			$ 03.06.2001 KM
			+ При каждой перерисовке диалога, кроме режима перемещения, устанавливаем
			заголовок консоли, в противном случае он не всегда восстанавливался.
		*/
		if (!DialogMode.Check(DMODE_KEEPCONSOLETITLE))
			ConsoleTitle::SetFarTitle(GetDialogTitle());
	}

	DialogMode.Clear(DMODE_DRAWING);	// конец отрисовки диалога!!!
	DialogMode.Set(DMODE_SHOW);			// диалог на экране!

	if (DialogMode.Check(DMODE_DRAGGED)) {
		/*
			- BugZ#813 - DM_RESIZEDIALOG в DN_DRAWDIALOG -> проблема: Ctrl-F5 - отрисовка только полозьев.
			Убираем вызов плагиновго обработчика.
		*/
		// DlgProc(DN_DRAWDIALOGDONE,1,0);
		DefDlgProc(DN_DRAWDIALOGDONE, 1, 0);
	} else
		DlgProc(DN_DRAWDIALOGDONE, 0, 0);
}

int Dialog::LenStrItem(int ID, const wchar_t *lpwszStr)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (!lpwszStr)
		lpwszStr = Item[ID].strData;

	return (Item[ID].Flags & DIF_SHOWAMPERSAND) ? StrZCellsCount(lpwszStr) : HiStrCellsCount(lpwszStr);
}

bool Dialog::ProcessMoveDialog(DWORD Key)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (DialogMode.Check(DMODE_DRAGGED))	// если диалог таскается
	{
		/*
			TODO: Здесь проверить "уже здесь" и не делать лишних движений
			Т.е., если нажали End, то при следующем End ненужно ничего делать! - сравнить координаты !!!
		*/
		int rr = 1;

		// При перемещении диалога повторяем поведение "бормандовых" сред.
		switch (Key) {
			case KEY_CTRLLEFT:
			case KEY_CTRLNUMPAD4:
			case KEY_CTRLHOME:
			case KEY_CTRLNUMPAD7:
			case KEY_HOME:
			case KEY_NUMPAD7:
				rr = Key == KEY_CTRLLEFT || Key == KEY_CTRLNUMPAD4 ? 10 : X1;
				[[fallthrough]];

			case KEY_LEFT:
			case KEY_NUMPAD4:
				Hide();

				for (int i = 0; i < rr; i++)
					if (X2 > 0) {
						X1--;
						X2--;
						AdjustEditPos(-1, 0);
					}

				if (!DialogMode.Check(DMODE_ALTDRAGGED))
					Show();

				break;

			case KEY_CTRLRIGHT:
			case KEY_CTRLNUMPAD6:
			case KEY_CTRLEND:
			case KEY_CTRLNUMPAD1:
			case KEY_END:
			case KEY_NUMPAD1:
				rr = Key == KEY_CTRLRIGHT || Key == KEY_CTRLNUMPAD6 ? 10 : Max(0, ScrX - X2);
				[[fallthrough]];

			case KEY_RIGHT:
			case KEY_NUMPAD6:
				Hide();

				for (int i = 0; i < rr; i++)
					if (X1 < ScrX) {
						X1++;
						X2++;
						AdjustEditPos(1, 0);
					}

				if (!DialogMode.Check(DMODE_ALTDRAGGED))
					Show();

				break;

			case KEY_PGUP:
			case KEY_NUMPAD9:
			case KEY_CTRLPGUP:
			case KEY_CTRLNUMPAD9:
			case KEY_CTRLUP:
			case KEY_CTRLNUMPAD8:
				rr = Key == KEY_CTRLUP || Key == KEY_CTRLNUMPAD8 ? 5 : Y1;
				[[fallthrough]];

			case KEY_UP:
			case KEY_NUMPAD8:
				Hide();

				for (int i = 0; i < rr; i++)
					if (Y2 > 0) {
						Y1--;
						Y2--;
						AdjustEditPos(0, -1);
					}

				if (!DialogMode.Check(DMODE_ALTDRAGGED))
					Show();

				break;

			case KEY_CTRLDOWN:
			case KEY_CTRLNUMPAD2:
			case KEY_CTRLPGDN:
			case KEY_CTRLNUMPAD3:
			case KEY_PGDN:
			case KEY_NUMPAD3:
				rr = Key == KEY_CTRLDOWN || Key == KEY_CTRLNUMPAD2 ? 5 : Max(0, ScrY - Y2);
				[[fallthrough]];

			case KEY_DOWN:
			case KEY_NUMPAD2:
				Hide();

				for (int i = 0; i < rr; i++)
					if (Y1 < ScrY) {
						Y1++;
						Y2++;
						AdjustEditPos(0, 1);
					}

				if (!DialogMode.Check(DMODE_ALTDRAGGED))
					Show();

				break;
			case KEY_NUMENTER:
			case KEY_ENTER:
			case KEY_CTRLF5:
				DialogMode.Clear(DMODE_DRAGGED);	// закончим движение!

				if (!DialogMode.Check(DMODE_ALTDRAGGED)) {
					DlgProc(DN_DRAGGED, 1, 0);
					Show();
				}

				break;
			case KEY_ESC:
				Hide();
				AdjustEditPos(OldX1 - X1, OldY1 - Y1);
				X1 = OldX1;
				X2 = OldX2;
				Y1 = OldY1;
				Y2 = OldY2;
				DialogMode.Clear(DMODE_DRAGGED);

				if (!DialogMode.Check(DMODE_ALTDRAGGED)) {
					DlgProc(DN_DRAGGED, 1, TRUE);
					Show();
				}

				break;
		}

		if (DialogMode.Check(DMODE_ALTDRAGGED)) {
			DialogMode.Clear(DMODE_DRAGGED | DMODE_ALTDRAGGED);
			DlgProc(DN_DRAGGED, 1, 0);
			Show();
		}

		return true;
	}

	if (Key == KEY_CTRLF5 && DialogMode.Check(DMODE_ISCANMOVE)) {
		if (DlgProc(DN_DRAGGED, 0, 0))	// если разрешили перемещать!
		{
			// включаем флаг и запоминаем координаты
			DialogMode.Set(DMODE_DRAGGED);
			OldX1 = X1;
			OldX2 = X2;
			OldY1 = Y1;
			OldY2 = Y2;
			// # GetText(0,0,3,0,LV);
			Show();
		}

		return TRUE;
	}

	return FALSE;
}

int64_t Dialog::VMProcess(int OpCode, void *vParam, int64_t iParam)
{
	switch (OpCode) {
		case MCODE_F_MENU_CHECKHOTKEY:
		case MCODE_F_MENU_GETHOTKEY:
		case MCODE_F_MENU_SELECT:
		case MCODE_F_MENU_GETVALUE:
		case MCODE_F_MENU_ITEMSTATUS:
		case MCODE_V_MENU_VALUE:
		case MCODE_F_MENU_FILTER:
		case MCODE_F_MENU_FILTERSTR: {
			const wchar_t *str = (const wchar_t *)vParam;

			if (GetDropDownOpened() || Item[FocusPos].Type == DI_LISTBOX) {
				if (Item[FocusPos].ListPtr)
					return Item[FocusPos].ListPtr->VMProcess(OpCode, vParam, iParam);
			} else if (OpCode == MCODE_F_MENU_CHECKHOTKEY)
				return CheckHighlights(*str, (int)iParam) + 1;

			return 0;
		}
	}

	switch (OpCode) {
		case MCODE_C_EOF:
		case MCODE_C_BOF:
		case MCODE_C_SELECTED:
		case MCODE_C_EMPTY: {
			if (FarIsEdit(Item[FocusPos].Type)) {
				if (Item[FocusPos].Type == DI_COMBOBOX && GetDropDownOpened())
					return Item[FocusPos].ListPtr->VMProcess(OpCode, vParam, iParam);
				else
					return Item[FocusPos].GetEdit()->VMProcess(OpCode, vParam, iParam);
			} else if (Item[FocusPos].Type == DI_LISTBOX && OpCode != MCODE_C_SELECTED)
				return Item[FocusPos].ListPtr->VMProcess(OpCode, vParam, iParam);

			return 0;
		}
		case MCODE_V_DLGITEMTYPE:
			switch (auto iType = Item[FocusPos].Type) {
				case DI_BUTTON:
				case DI_CHECKBOX:
				case DI_DOUBLEBOX:
				case DI_FIXEDIT:
				case DI_LISTBOX:
				case DI_MEMOEDIT:
				case DI_PSWEDIT:
				case DI_RADIOBUTTON:
				case DI_SINGLEBOX:
				case DI_TEXT:
				case DI_USERCONTROL:
				case DI_VTEXT:
					return iType;
				case DI_COMBOBOX:
				case DI_EDIT:
					return iType | (DropDownOpened ? 0x8000 : 0);
				default:
					return -1;
			}

		case MCODE_V_DLGINFOOWNER: // Dlg.Owner
		{
			if (PluginNumber == -1)
				return 0;
			auto Plug = reinterpret_cast<Plugin*>(PluginNumber);
			return Plug->GetSysID();
		}
		case MCODE_V_DLGITEMCOUNT:		// Dlg.ItemCount()
		{
			return ItemCount();
		}
		case MCODE_V_DLGCURPOS:		// Dlg.CurPos
		{
			return FocusPos + 1;
		}
		case MCODE_V_DLGPREVPOS:    // Dlg.PrevPos
		{
			return PrevFocusPos + 1;
		}
		case MCODE_V_DLGINFOID:		// Dlg.Info.Id
		{
			static FARString strId;
			strId = GuidToString(Id);
			return reinterpret_cast<INT64>(strId.CPtr());
		}
		case MCODE_V_ITEMCOUNT:
		case MCODE_V_CURPOS: {
			switch (Item[FocusPos].Type) {
				case DI_COMBOBOX:

					if (DropDownOpened || (Item[FocusPos].Flags & DIF_DROPDOWNLIST))
						return Item[FocusPos].ListPtr->VMProcess(OpCode, vParam, iParam);

				case DI_EDIT:
				case DI_PSWEDIT:
				case DI_FIXEDIT:
					return Item[FocusPos].GetEdit()->VMProcess(OpCode, vParam, iParam);
				case DI_LISTBOX:
					return Item[FocusPos].ListPtr->VMProcess(OpCode, vParam, iParam);
				case DI_USERCONTROL:

					if (OpCode == MCODE_V_CURPOS)
						return Item[FocusPos].UCData->CursorPos.X;

				case DI_BUTTON:
				case DI_CHECKBOX:
				case DI_RADIOBUTTON:
					return 0;
			}

			return 0;
		}
		case MCODE_F_EDITOR_SEL: {
			if (FarIsEditNoCombobox(Item[FocusPos].Type)
					|| (Item[FocusPos].Type == DI_COMBOBOX
							&& !(DropDownOpened || (Item[FocusPos].Flags & DIF_DROPDOWNLIST)))) {
				return Item[FocusPos].GetEdit()->VMProcess(OpCode, vParam, iParam);
			}

			return 0;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
/*
	Public, Virtual:
		Обработка данных от клавиатуры.
		Перекрывает BaseInput::ProcessKey.
*/
int Dialog::ProcessKey(FarKey Key)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	_DIALOG(CleverSysLog CL(L"Dialog::ProcessKey"));
	_DIALOG(SysLog(L"Param: Key=%ls", _FARKEY_ToName(Key)));
	int I;
	FARString strStr;

	if ((ShiftPressed != ShiftState || CtrlPressed != CtrlState || AltPressed != AltState) && !DialogMode.Check(DMODE_KEY)) {
		ShiftState = ShiftPressed;
		CtrlState = CtrlPressed;
		AltState = AltPressed;
		FarKey fKey = ShiftState ? KEY_SHIFT : 0;
		fKey |= CtrlPressed ? KEY_CTRL : 0;
		fKey |= AltPressed ? KEY_ALT : 0;
		DlgProc(DN_KEY, -1, fKey);
	}
	if (Key == KEY_NONE || Key == KEY_IDLE) {
		DlgProc(DN_ENTERIDLE, 0, 0);	// $ 28.07.2000 SVS Передадим этот факт в обработчик :-)
		return FALSE;
	}

	if (Key == KEY_KILLFOCUS || Key == KEY_GOTFOCUS) {
		DlgProc(DN_ACTIVATEAPP, Key == KEY_GOTFOCUS, 0);
		return FALSE;
	}

	if (ProcessMoveDialog(Key))
		return TRUE;

	// BugZ#488 - Shift=enter
	if (ShiftPressed && (Key == KEY_ENTER || Key == KEY_NUMENTER) && !CtrlObject->Macro.IsExecuting()
			&& Item[FocusPos].Type != DI_BUTTON) {
		Key = Key == KEY_ENTER ? KEY_SHIFTENTER : KEY_SHIFTNUMENTER;
	}

	/*(Key>=KEY_MACRO_BASE && Key <=KEY_MACRO_ENDBASE) ||*/
	if (!(Key >= KEY_OP_BASE && Key <= KEY_OP_ENDBASE) && !DialogMode.Check(DMODE_KEY))
		if (DlgProc(DN_KEY, FocusPos, Key))
			return TRUE;

	if (!DialogMode.Check(DMODE_SHOW))
		return TRUE;

	// А ХЗ, может в этот момент изменилось состояние элемента!
	if (Item[FocusPos].Flags & DIF_HIDDEN)
		return TRUE;

	// небольшая оптимизация
	if (Item[FocusPos].Type == DI_CHECKBOX) {
		if (!(Item[FocusPos].Flags & DIF_3STATE)) {
			if (Key == KEY_MULTIPLY)	// в CheckBox 2-state Gray* не работает!
				Key = KEY_NONE;

			if ((Key == KEY_ADD && !Item[FocusPos].Selected)
					|| (Key == KEY_SUBTRACT && Item[FocusPos].Selected))
				Key = KEY_SPACE;
		}

		// блок else не нужен, т.к. ниже клавиши будут обработаны...
	} else if (Key == KEY_ADD)
		Key = '+';
	else if (Key == KEY_SUBTRACT)
		Key = '-';
	else if (Key == KEY_MULTIPLY)
		Key = '*';

	if (Item[FocusPos].Type == DI_BUTTON && Key == KEY_SPACE)
		Key = KEY_ENTER;

	if (Item[FocusPos].Type == DI_LISTBOX) {
		switch (Key) {
			case KEY_HOME:
			case KEY_NUMPAD7:
			case KEY_LEFT:
			case KEY_NUMPAD4:
			case KEY_END:
			case KEY_NUMPAD1:
			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_UP:
			case KEY_NUMPAD8:
			case KEY_DOWN:
			case KEY_NUMPAD2:
			case KEY_PGUP:
			case KEY_NUMPAD9:
			case KEY_PGDN:
			case KEY_NUMPAD3:
			case KEY_MSWHEEL_UP:
			case KEY_MSWHEEL_DOWN:
			case KEY_MSWHEEL_LEFT:
			case KEY_MSWHEEL_RIGHT:
			case KEY_NUMENTER:
			case KEY_ENTER:
				VMenu *List = Item[FocusPos].ListPtr;
				int CurListPos = List->GetSelectPos();
				auto CheckedListItem = List->GetCheck(-1);
				List->ProcessKey(Key);
				int NewListPos = List->GetSelectPos();

				if (NewListPos != CurListPos && !DlgProc(DN_LISTCHANGE, FocusPos, NewListPos)) {
					if (!DialogMode.Check(DMODE_SHOW))
						return TRUE;

					List->SetCheck(CheckedListItem, CurListPos);

					if (DialogMode.Check(DMODE_SHOW) && !(Item[FocusPos].Flags & DIF_HIDDEN))
						ShowDialog(FocusPos);	// FocusPos
				}

				if (!(Key == KEY_ENTER || Key == KEY_NUMENTER) || (Item[FocusPos].Flags & DIF_LISTNOCLOSE))
					return TRUE;
		}
	}

	switch (Key) {
		case KEY_F1:
		{
			// Перед выводом диалога посылаем сообщение в обработчик
			//   и если вернули что надо, то выводим подсказку
			auto Topic = (const wchar_t*) DlgProc(DN_HELP, FocusPos, (LONG_PTR)HelpTopic.CPtr());
			if (!Help::MkTopic(PluginNumber, Topic, strStr).IsEmpty()) {
				Help Hlp(strStr);
			}
			return TRUE;
		}

		case KEY_ESC:
		case KEY_BREAK:
		case KEY_F10:
			ExitCode = (Key == KEY_BREAK) ? -2 : -1;
			CloseDialog();
			return TRUE;

		case KEY_HOME:
		case KEY_NUMPAD7:

			if (Item[FocusPos].Type == DI_USERCONTROL)		// для user-типа вываливаем
				return TRUE;

			return Do_ProcessFirstCtrl();

		case KEY_TAB:
		case KEY_SHIFTTAB:
			return Do_ProcessTab(Key == KEY_TAB);

		case KEY_SPACE:
			return Do_ProcessSpace();

		case KEY_CTRLNUMENTER:
		case KEY_CTRLENTER: {
			for (I = 0; I < ItemCount(); I++)
				if (Item[I].DefaultButton) {
					if (Item[I].Flags & DIF_DISABLE) {
						// ProcessKey(KEY_DOWN); // на твой вкус :-)
						return TRUE;
					}

					if (!FarIsEdit(Item[I].Type))
						Item[I].Selected = 1;

					ExitCode = I;
					/* $ 18.05.2001 DJ */
					CloseDialog();
					/* DJ $ */
					return TRUE;
				}

			if (!DialogMode.Check(DMODE_OLDSTYLE)) {
				DialogMode.Clear(DMODE_ENDLOOP);	// только если есть
				return TRUE;						// делать больше не чего
			}
		}
		[[fallthrough]];

		case KEY_NUMENTER:
		case KEY_ENTER: {
			if (Item[FocusPos].Type == DI_MEMOEDIT && Item[FocusPos].GetEdit()) {
				Item[FocusPos].GetEdit()->ProcessKey(Key);
				ShowDialog();
				return TRUE;
			}
			if (FarIsEditNoCombobox(Item[FocusPos].Type)
					&& (Item[FocusPos].Flags & DIF_EDITOR) && !(Item[FocusPos].Flags & DIF_READONLY))
			{
				int EditorLastPos;

				for (EditorLastPos = I = FocusPos; I < ItemCount(); I++)
					if (FarIsEdit(Item[I].Type) && (Item[I].Flags & DIF_EDITOR))
						EditorLastPos = I;
					else
						break;

				if (Item[EditorLastPos].GetEdit()->GetLength())
					return TRUE;

				for (I = EditorLastPos; I > FocusPos; I--) {
					int CurPos;

					if (I == FocusPos + 1)
						CurPos = Item[I - 1].GetEdit()->GetCurPos();
					else
						CurPos = 0;

					Item[I - 1].GetEdit()->GetString(strStr);
					int Length = (int)strStr.GetLength();
					Item[I].GetEdit()->SetString(CurPos >= Length ? L"" : strStr.CPtr() + CurPos);

					if (CurPos < Length)
						strStr.Truncate(CurPos);

					Item[I].GetEdit()->SetCurPos(0);
					Item[I-1].GetEdit()->SetString(strStr);
				}

				if (EditorLastPos > FocusPos) {
					Item[FocusPos].GetEdit()->SetCurPos(0);
					Do_ProcessNextCtrl(false, false);
				}

				ShowDialog();
				return TRUE;
			}
			else if (Item[FocusPos].Type == DI_BUTTON) {
				Item[FocusPos].Selected = 1;

				// сообщение - "Кнокна кликнута"
				if (SendDlgMessage(DN_BTNCLICK, FocusPos, 0))
					return TRUE;

				if (Item[FocusPos].Flags & DIF_BTNNOCLOSE)
					return TRUE;

				ExitCode = FocusPos;
				CloseDialog();
				return TRUE;
			}
			else {
				ExitCode = -1;

				for (I = 0; I < ItemCount(); I++) {
					if (Item[I].DefaultButton && !(Item[I].Flags & DIF_BTNNOCLOSE)) {
						if (Item[I].Flags & DIF_DISABLE) {
							// ProcessKey(KEY_DOWN); // на твой вкус :-)
							return TRUE;
						}

						//						if (!(FarIsEdit(Item[I].Type) || Item[I].Type == DI_CHECKBOX || Item[I].Type == DI_RADIOBUTTON))
						//							Item[I].Selected=1;
						ExitCode = I;
						break;
					}
				}
			}

			if (ExitCode == -1)
				ExitCode = FocusPos;

			CloseDialog();
			return TRUE;
		}

		/*
			3-х уровневое состояние
			Для чекбокса сюда попадем только в случае, если контрол
			имеет флаг DIF_3STATE
		*/
		case KEY_ADD:
		case KEY_SUBTRACT:
		case KEY_MULTIPLY:

			if (Item[FocusPos].Type == DI_CHECKBOX) {
				const int CHKState = (Key == KEY_ADD) ? 1 : (Key == KEY_SUBTRACT) ? 0 : 2;

				if (Item[FocusPos].Selected != CHKState) {
					if (SendDlgMessage(DN_BTNCLICK, FocusPos, CHKState)) {
						Item[FocusPos].Selected = CHKState;
						ShowDialog();
					}
				}
			}

			return TRUE;

		case KEY_LEFT:
		case KEY_NUMPAD4:
		case KEY_SHIFTNUMPAD4:
		case KEY_MSWHEEL_LEFT:
		case KEY_RIGHT:
		case KEY_NUMPAD6:
		case KEY_SHIFTNUMPAD6:
		case KEY_MSWHEEL_RIGHT: {
			if (Item[FocusPos].Type == DI_USERCONTROL)		// для user-типа вываливаем
				return TRUE;

			if (FarIsEdit(Item[FocusPos].Type)) {
				Item[FocusPos].GetEdit()->ProcessKey(Key);
				return TRUE;
			} else {
				return MoveToCtrlHorizontal(Key == KEY_RIGHT || Key == KEY_NUMPAD6);
			}
		}

		case KEY_UP:
		case KEY_NUMPAD8:
		case KEY_DOWN:
		case KEY_NUMPAD2: {
			if (Item[FocusPos].Type == DI_USERCONTROL)		// для user-типа вываливаем
				return TRUE;

			if (Item[FocusPos].Type == DI_MEMOEDIT && Item[FocusPos].GetEdit()) {
				Item[FocusPos].GetEdit()->ProcessKey(Key);
				ShowDialog();
				return TRUE;
			}

			return MoveToCtrlVertical(Key == KEY_UP || Key == KEY_NUMPAD8);
		}

		// $ 27.04.2001 VVM - Обработка колеса мышки
		case KEY_MSWHEEL_UP:
		case KEY_MSWHEEL_DOWN:
		case KEY_CTRLUP:
		case KEY_CTRLNUMPAD8:
		case KEY_CTRLDOWN:
		case KEY_CTRLNUMPAD2:
			if (Item[FocusPos].Type == DI_MEMOEDIT) {
				Item[FocusPos].GetEdit()->ProcessKey(Key);
				ShowDialog(FocusPos);
				return TRUE;
			}
			return ProcessOpenComboBox(Item[FocusPos].Type, Item[FocusPos], FocusPos);

		case KEY_F5:
			if (Item[FocusPos].Type == DI_MEMOEDIT) {
				Item[FocusPos].GetEdit()->ToggleShowWhiteSpace();
				ShowDialog(FocusPos);
				return TRUE;
			}
			break;

		case KEY_F11:
			if (!CheckDialogMode(DMODE_NOPLUGINS)) {
				return FrameManager->ProcessKey(Key);
			}
			break;

		// ЭТО перед default предпоследний!!!
		case KEY_END:
		case KEY_NUMPAD1:
			if (Item[FocusPos].Type == DI_USERCONTROL)		// для user-типа вываливаем
				return TRUE;

			if (FarIsEdit(Item[FocusPos].Type)) {
				Item[FocusPos].GetEdit()->ProcessKey(Key);
				return TRUE;
			}
			[[fallthrough]];

		// ЭТО перед default последний!!!
		case KEY_PGDN:
		case KEY_NUMPAD3:
			if (Item[FocusPos].Type == DI_USERCONTROL)		// для user-типа вываливаем
				return TRUE;

			if (Item[FocusPos].Type == DI_MEMOEDIT) {
				Item[FocusPos].GetEdit()->ProcessKey(Key);
				return TRUE;
			}

			if (Item[FocusPos].Flags & DIF_EDITOR)
				{} // для DIF_EDITOR будет обработано ниже [[fallthrough]]
			else {
				for (I = 0; I < ItemCount(); I++) {
					if (Item[I].DefaultButton) {
						ChangeFocus2(I);
						ShowDialog();
						return TRUE;
					}
				}
				return TRUE;
			}
			[[fallthrough]];

		default: {
			if (Item[FocusPos].Type == DI_LISTBOX) {
				VMenu *List = Item[FocusPos].ListPtr;
				int CurListPos = List->GetSelectPos();
				auto CheckedListItem = List->GetCheck(-1);
				List->ProcessKey(Key);
				int NewListPos = List->GetSelectPos();

				if (NewListPos != CurListPos && !DlgProc(DN_LISTCHANGE, FocusPos, NewListPos)) {
					if (!DialogMode.Check(DMODE_SHOW))
						return TRUE;

					List->SetCheck(CheckedListItem, CurListPos);

					if (DialogMode.Check(DMODE_SHOW) && !(Item[FocusPos].Flags & DIF_HIDDEN))
						ShowDialog(FocusPos);	// FocusPos
				}

				return TRUE;
			}

			if (FarIsEdit(Item[FocusPos].Type)) {
				DlgEdit *edt = Item[FocusPos].GetEdit();

				if (Key == KEY_CTRLL) { // исключим смену режима RO для поля ввода с клавиатуры
					return TRUE;
				}
				else if (Key == KEY_CTRLA || Key == KEY_CTRLU) {
					edt->ProcessKey(Key);
					return TRUE;
				}
				else if ((Item[FocusPos].Flags & DIF_EDITOR) && !(Item[FocusPos].Flags & DIF_READONLY)) {
					switch (Key) {
						case KEY_BS: {
							int CurPos = edt->GetCurPos();

							// В начале строки????
							if (!edt->GetCurPos()) {
								// а "выше" тоже DIF_EDITOR?
								if (FocusPos > 0 && (Item[FocusPos - 1].Flags & DIF_EDITOR)) {
									// добавляем к предыдущему и...
									DlgEdit *edt_1 = Item[FocusPos - 1].GetEdit();
									edt_1->GetString(strStr);
									CurPos = static_cast<int>(strStr.GetLength());
									FARString strAdd;
									edt->GetString(strAdd);
									strStr+= strAdd;
									edt_1->SetString(strStr);

									for (I = FocusPos + 1; I < ItemCount(); I++) {
										if (Item[I].Flags & DIF_EDITOR) {
											if (I > FocusPos) {
												Item[I].GetEdit()->GetString(strStr);
												Item[I - 1].GetEdit()->SetString(strStr);
											}

											Item[I].GetEdit()->SetString(L"");
										} else		// ага, значит FocusPos это есть последний из DIF_EDITOR
										{
											Item[I - 1].GetEdit()->SetString(L"");
											break;
										}
									}

									Do_ProcessNextCtrl(true);
									edt_1->SetCurPos(CurPos);
								}
							} else {
								edt->ProcessKey(Key);
							}

							ShowDialog();
							return TRUE;
						}

						case KEY_CTRLY: {
							for (I = FocusPos; I < ItemCount(); I++)
								if (Item[I].Flags & DIF_EDITOR) {
									if (I > FocusPos) {
										Item[I].GetEdit()->GetString(strStr);
										Item[I - 1].GetEdit()->SetString(strStr);
									}

									Item[I].GetEdit()->SetString(L"");
								} else
									break;

							ShowDialog();
							return TRUE;
						}

						case KEY_NUMDEL:
						case KEY_DEL: {
							/*
								$ 19.07.2000 SVS
								! "...В редакторе команд меню нажмите home shift+end del
								блок не удаляется..."
								DEL у итемов, имеющих DIF_EDITOR, работал без учета
								выделения...
							*/
							if (FocusPos + 1 < ItemCount() && (Item[FocusPos + 1].Flags & DIF_EDITOR)) {
								int CurPos = edt->GetCurPos();
								int Length = edt->GetLength();
								int SelStart, SelEnd;
								edt->GetSelection(SelStart, SelEnd);
								edt->GetString(strStr);

								if (SelStart > -1) {
									FARString strEnd = strStr.CPtr() + SelEnd;
									strStr.Truncate(SelStart);
									strStr+= strEnd;
									edt->SetString(strStr);
									edt->SetCurPos(SelStart);
									ShowDialog();
									return TRUE;
								} else if (CurPos >= Length) {
									DlgEdit *edt_1 = Item[FocusPos + 1].GetEdit();

									/*
										$ 12.09.2000 SVS
										Решаем проблему, если Del нажали в позиции
										большей, чем длина строки
									*/
									if (CurPos > Length) {
										strStr.Append(L' ', CurPos - Length);
									}

									FARString strAdd;
									edt_1->GetString(strAdd);
									edt_1->SetString(strStr + strAdd);
									ProcessKey(KEY_CTRLY);
									edt->SetCurPos(CurPos);
									ShowDialog();
									return TRUE;
								}
							}

							break;
						}

						case KEY_PGDN:
						case KEY_NUMPAD3:
						case KEY_PGUP:
						case KEY_NUMPAD9: {
							int Step = (Key == KEY_PGUP || Key == KEY_NUMPAD9) ? -1 : 1;
							I = FocusPos;

							while (Item[I].Flags & DIF_EDITOR)
								I = ChangeFocus(I, Step, false);

							if (!(Item[I].Flags & DIF_EDITOR))
								I = ChangeFocus(I, -Step, false);

							ChangeFocus2(I);
							ShowDialog();

							return TRUE;
						}
					}
				}

				if (Key == KEY_OP_XLAT && !(Item[FocusPos].Flags & DIF_READONLY)) {
					edt->SetClearFlag(false);
					edt->Xlat();

					// иначе неправильно работает ctrl-end
					edt->strLastStr = edt->GetStringAddr();
					edt->LastPartLength = static_cast<int>(edt->strLastStr.GetLength());

					Redraw();	// Перерисовка должна идти после DN_EDITCHANGE (imho)
					return TRUE;
				}

				if (!(Item[FocusPos].Flags & DIF_READONLY) || IsNavKey(Key)) {
					// "только что ломанулись и начинать выделение с нуля"?
					if ((Opt.Dialogs.EditLine & DLGEDITLINE_NEWSELONGOTFOCUS) && Item[FocusPos].SelStart != -1
							&& PrevFocusPos != FocusPos) {	// && Item[FocusPos].SelEnd)

						edt->Flags().Clear(FEDITLINE_MARKINGBLOCK);
						PrevFocusPos = FocusPos;
					}

					if (edt->ProcessKey(Key)) {
						if (Item[FocusPos].Flags & DIF_READONLY)
							return TRUE;

						if ((Key == KEY_CTRLEND || Key == KEY_CTRLNUMPAD1)
								&& edt->GetCurPos() == edt->GetLength()) {
							if (edt->LastPartLength == -1)
								edt->strLastStr = edt->GetStringAddr();

							strStr = edt->strLastStr;
							int CurCmdPartLength = static_cast<int>(strStr.GetLength());
							edt->HistoryGetSimilar(strStr, edt->LastPartLength);

							if (edt->LastPartLength == -1) {
								edt->strLastStr = edt->GetStringAddr();
								edt->LastPartLength = CurCmdPartLength;
							}
							edt->DisableAC();
							edt->SetString(strStr);
							edt->Select(edt->LastPartLength, static_cast<int>(strStr.GetLength()));
							edt->RevertAC();
							Show();
							return TRUE;
						}

						edt->LastPartLength = -1;

						if (Key == KEY_CTRLSHIFTEND || Key == KEY_CTRLSHIFTNUMPAD1) {
							edt->EnableAC();
							edt->AutoComplete(true, false);
							edt->RevertAC();
						}

						Redraw();	// Перерисовка должна идти после DN_EDITCHANGE (imho)
						return TRUE;
					}
				} else if (!(Key & (KEY_ALT | KEY_RALT)))
					return TRUE;
			}

			if (ProcessHighlighting(Key, FocusPos, false))
				return TRUE;

			return (Opt.XLat.EnableForDialogs && ProcessHighlighting(Key, FocusPos, true));
		}
	}
	return FALSE;
}

void Dialog::ProcessKey(FarKey Key, int ItemPos)
{
	int SavedFocusPos = FocusPos;
	FocusPos = ItemPos;
	ProcessKey(Key);
	if (FocusPos == ItemPos)
		FocusPos = SavedFocusPos;
}

//////////////////////////////////////////////////////////////////////////
/*
	Public, Virtual:
		Обработка данных от "мыши".
		Перекрывает BaseInput::ProcessMouse.
*/
/*
	$ 18.08.2000 SVS
	+ DN_MOUSECLICK
*/
int Dialog::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	int I;
	int MsX, MsY;
	int Type;
	SMALL_RECT Rect;

	if (!DialogMode.Check(DMODE_SHOW))
		return FALSE;

	if (DialogMode.Check(DMODE_MOUSEEVENT)) {
		if (!DlgProc(DN_MOUSEEVENT, 0, (LONG_PTR)MouseEvent))
			return TRUE;
	}

	if (!DialogMode.Check(DMODE_SHOW))
		return FALSE;

	MsX = MouseEvent->dwMousePosition.X;
	MsY = MouseEvent->dwMousePosition.Y;

	for (I = ItemCount() - 1; I != -1; I--) {
		if (Item[I].Flags & (DIF_DISABLE | DIF_HIDDEN))
			continue;

		Type = Item[I].Type;

		if (Type == DI_LISTBOX && MsY >= Y1 + Item[I].Y1 && MsY <= Y1 + Item[I].Y2
				&& MsX >= X1 + Item[I].X1 && MsX <= X1 + Item[I].X2)
		{
			VMenu *List = Item[I].ListPtr;
			int Pos = List->GetSelectPos();
			auto CheckedListItem = List->GetCheck(-1);

			if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
				if (FocusPos != I) {
					ChangeFocus2(I);
					ShowDialog();
				}

				if (MouseEvent->dwEventFlags != DOUBLE_CLICK
						&& !(Item[I].IFlags.Flags
								& (DLGIIF_LISTREACTIONFOCUS | DLGIIF_LISTREACTIONNOFOCUS))) {

					List->ProcessMouse(MouseEvent);
					int NewListPos = List->GetSelectPos();

					if (NewListPos != Pos
							&& !SendDlgMessage(DN_LISTCHANGE, I, (LONG_PTR)NewListPos)) {
						List->SetCheck(CheckedListItem, Pos);

						if (DialogMode.Check(DMODE_SHOW) && !(Item[I].Flags & DIF_HIDDEN))
							ShowDialog(I);	// FocusPos
					} else {
						Pos = NewListPos;
					}
				}
				else if (!SendDlgMessage(DN_MOUSECLICK, I, (LONG_PTR)MouseEvent)) {
					List->ProcessMouse(MouseEvent);
					int NewListPos = List->GetSelectPos();
					int InScroolBar =
							(MsX == X1 + Item[I].X2 && MsY >= Y1 + Item[I].Y1 && MsY <= Y1 + Item[I].Y2)
							&& (List->CheckFlags(VMENU_LISTBOX | VMENU_ALWAYSSCROLLBAR)
									|| Opt.ShowMenuScrollbar);

					if (!InScroolBar &&																	// вне скроллбара и
							NewListPos != Pos &&														// позиция изменилась и
							!SendDlgMessage(DN_LISTCHANGE, I, (LONG_PTR)NewListPos))		// и плагин сказал в морг
					{
						List->SetCheck(CheckedListItem, Pos);

						if (DialogMode.Check(DMODE_SHOW) && !(Item[I].Flags & DIF_HIDDEN))
							ShowDialog(I);	// FocusPos
					} else {
						Pos = NewListPos;

						if (!InScroolBar && !(Item[I].Flags & DIF_LISTNOCLOSE)) {
							ExitCode = I;
							CloseDialog();
							return TRUE;
						}
					}
				}

				return TRUE;
			}
			else {
				if (!MouseEvent->dwButtonState
						|| SendDlgMessage(DN_MOUSECLICK, I, (LONG_PTR)MouseEvent)) {
					if ((I == FocusPos && (Item[I].IFlags.Flags & DLGIIF_LISTREACTIONFOCUS))
							|| (I != FocusPos && (Item[I].IFlags.Flags & DLGIIF_LISTREACTIONNOFOCUS))) {
						List->ProcessMouse(MouseEvent);
						int NewListPos = List->GetSelectPos();

						if (NewListPos != Pos
								&& !SendDlgMessage(DN_LISTCHANGE, I, (LONG_PTR)NewListPos)) {
							List->SetCheck(CheckedListItem, Pos);

							if (DialogMode.Check(DMODE_SHOW) && !(Item[I].Flags & DIF_HIDDEN))
								ShowDialog(I);	// FocusPos
						} else
							Pos = NewListPos;
					}
				}
			}

			return TRUE;
		}
	}

	// Stream of drag events passsed directly to multiline modal editor for mouse selection
	// before general dialog handling ignores it.
	if ((MouseEvent->dwEventFlags & MOUSE_MOVED)
			&& (MouseEvent->dwButtonState
					& (FROM_LEFT_1ST_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED))
			&& FocusPos < ItemCount()
			&& !(Item[FocusPos].Flags & (DIF_DISABLE | DIF_HIDDEN))
			&& Item[FocusPos].Type == DI_MEMOEDIT) {
		DlgEdit *MultilineEditor = Item[FocusPos].GetEdit();
		if (MultilineEditor->ProcessMouse(MouseEvent))
			return TRUE;
	}

	if (MsX < X1 || MsY < Y1 || MsX > X2 || MsY > Y2) {
		if (DialogMode.Check(DMODE_CLICKOUTSIDE)
				&& !DlgProc(DN_MOUSECLICK, -1, (LONG_PTR)MouseEvent)) {
			if (!DialogMode.Check(DMODE_SHOW))
				return FALSE;

			//		if (!(MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) && PrevLButtonPressed && ScreenObject::CaptureMouseObject)
			if (!(MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
					&& (PrevMouseButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
					&& (Opt.Dialogs.MouseButton & DMOUSEBUTTON_LEFT))
				ProcessKey(KEY_ESC);
			//		else if (!(MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED) && PrevRButtonPressed && ScreenObject::CaptureMouseObject)
			else if (!(MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED)
					&& (PrevMouseButtonState & RIGHTMOST_BUTTON_PRESSED)
					&& (Opt.Dialogs.MouseButton & DMOUSEBUTTON_RIGHT))
				ProcessKey(KEY_ENTER);
		}

		if (MouseEvent->dwButtonState)
			DialogMode.Set(DMODE_CLICKOUTSIDE);

		if (!MouseEvent->dwButtonState && FocusPos < ItemCount()
				&& !(Item[FocusPos].Flags & (DIF_DISABLE | DIF_HIDDEN))
				&& FarIsEdit(Item[FocusPos].Type)) {
			DlgEdit *EditLine = Item[FocusPos].GetEdit();
			EditLine->ProcessMouse(MouseEvent);
		}

		// ScreenObject::SetCapture(this);
		return TRUE;
	}

	if (!MouseEvent->dwButtonState) {
		if (FocusPos < ItemCount() && !(Item[FocusPos].Flags & (DIF_DISABLE | DIF_HIDDEN))
				&& FarIsEdit(Item[FocusPos].Type)) {
			DlgEdit *EditLine = Item[FocusPos].GetEdit();
			if (EditLine->ProcessMouse(MouseEvent))
				return TRUE;
		}
		DialogMode.Clear(DMODE_CLICKOUTSIDE);
		//		ScreenObject::SetCapture(nullptr);
		return FALSE;
	}

	if (!MouseEvent->dwEventFlags || MouseEvent->dwEventFlags == DOUBLE_CLICK) {
		// первый цикл - все за исключением рамок.
		for (I = ItemCount() - 1; I != -1; I--) {
			if (Item[I].Flags & (DIF_DISABLE | DIF_HIDDEN))
				continue;

			GetItemRect(I, Rect);
			Rect.Left+= X1;
			Rect.Top+= Y1;
			Rect.Right+= X1;
			Rect.Bottom+= Y1;
			//_D(SysLog(L"? %2d) Rect (%2d,%2d) (%2d,%2d) '%ls'",I,Rect.left,Rect.top,Rect.right,Rect.bottom,Item[I].Data));

			if (MsX >= Rect.Left && MsY >= Rect.Top && MsX <= Rect.Right && MsY <= Rect.Bottom) {
				// для прозрачных :-)
				if (Item[I].Type == DI_SINGLEBOX || Item[I].Type == DI_DOUBLEBOX) {
					// если на рамке, то...
					if (MsX == Rect.Left || MsX == Rect.Right || MsY == Rect.Top || MsY == Rect.Bottom)
					{
						if (DlgProc(DN_MOUSECLICK, I, (LONG_PTR)MouseEvent))
							return TRUE;

						if (!DialogMode.Check(DMODE_SHOW))
							return FALSE;
					} else
						continue;
				}

				if (Item[I].Type == DI_USERCONTROL) {
					// для user-типа подготовим координаты мыши
					MouseEvent->dwMousePosition.X-= Rect.Left;
					MouseEvent->dwMousePosition.Y-= Rect.Top;
				}

				//_SVS(SysLog(L"+ %2d) Rect (%2d,%2d) (%2d,%2d) '%ls' Dbl=%d",I,Rect.left,Rect.top,Rect.right,Rect.bottom,Item[I].Data,MouseEvent->dwEventFlags==DOUBLE_CLICK));
				if (DlgProc(DN_MOUSECLICK, I, (LONG_PTR)MouseEvent))
					return TRUE;

				if (!DialogMode.Check(DMODE_SHOW))
					return TRUE;

				if (Item[I].Type == DI_USERCONTROL) {
					ChangeFocus2(I);
					ShowDialog();
					return TRUE;
				}

				break;
			}
		}

		if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
			for (I = ItemCount() - 1; I != -1; I--) {
				// Исключаем из списка оповещаемых о мыши недоступные элементы
				if (Item[I].Flags & (DIF_DISABLE | DIF_HIDDEN))
					continue;

				Type = Item[I].Type;

				GetItemRect(I, Rect);
				Rect.Left+= X1;
				Rect.Top+= Y1;
				Rect.Right+= X1;
				Rect.Bottom+= Y1;
				if (Item[I].HasDropDownArrow())
					Rect.Right++;

				if (MsX >= Rect.Left && MsY >= Rect.Top && MsX <= Rect.Right && MsY <= Rect.Bottom) {
					/* ********************************************************** */
					if (FarIsEdit(Type)) {
						/*
							$ 15.08.2000 SVS
							+ Сделаем так, чтобы ткнув мышкой в DropDownList
							список раскрывался сам.
							Есть некоторая глюкавость - когда список раскрыт и мы
							мышой переваливаем на другой элемент, то список закрывается
							но перехода реального на указанный элемент диалога не происходит
						*/
						int EditX1, EditY1, EditX2, EditY2;
						DlgEdit *EditLine = Item[I].GetEdit();
						EditLine->GetPosition(EditX1, EditY1, EditX2, EditY2);

						if (MsY == EditY1 && Type == DI_COMBOBOX && (Item[I].Flags & DIF_DROPDOWNLIST)
								&& MsX >= EditX1 && MsX <= EditX2 + 1) {
							EditLine->SetClearFlag(false);

							ChangeFocus2(I);
							ShowDialog();

							ProcessOpenComboBox(Item[I].Type, Item[I], I);

							return TRUE;
						}

						ChangeFocus2(I);

						if (EditLine->ProcessMouse(MouseEvent)) {
							EditLine->SetClearFlag(false);	// а может это делать в самом edit?

							/*
								$ 23.06.2001 KM
								! Оказалось нужно перерисовывать весь диалог иначе
								не снимался признак активности с комбобокса с которго уходим.
							*/
							ShowDialog();	// нужен ли только один контрол или весь диалог?
							return TRUE;
						} else {
							// Проверка на DI_COMBOBOX здесь лишняя. Убрана (KM).
							if (MsX == EditX2 + 1 && MsY == EditY1 && Item[I].HasDropDownArrow()) {
								EditLine->SetClearFlag(false);	// раз уж покусились на, то и...

								ChangeFocus2(I);

								if (!(Item[I].Flags & DIF_HIDDEN))
									ShowDialog(I);

								ProcessOpenComboBox(Item[I].Type, Item[I], I);

								return TRUE;
							}
						}
					}

					/* ********************************************************** */
					if (Type == DI_BUTTON && MsY == Y1 + Item[I].Y1
							&& MsX < X1 + Item[I].X1 + HiStrCellsCount(Item[I].strData)) {
						ChangeFocus2(I);
						ShowDialog();

						while (IsMouseButtonPressed())
							;

						if (MouseX < X1 || MouseX > X1 + Item[I].X1 + HiStrCellsCount(Item[I].strData) + 4
								|| MouseY != Y1 + Item[I].Y1) {
							ChangeFocus2(I);
							ShowDialog();

							return TRUE;
						}

						ProcessKey(KEY_ENTER, I);
						return TRUE;
					}

					/* ********************************************************** */
					if ((Type == DI_CHECKBOX || Type == DI_RADIOBUTTON) && MsY == Y1 + Item[I].Y1
							&& MsX < (X1 + Item[I].X1 + HiStrCellsCount(Item[I].strData) + 4
										- ((Item[I].Flags & DIF_MOVESELECT) != 0))) {
						ChangeFocus2(I);
						ProcessKey(KEY_SPACE, I);
						return TRUE;
					}
				}
			}	// for (I=0;I<ItemCount();I++)

			// ДЛЯ MOUSE-Перемещалки:
			// Сюда попадаем в том случае, если мышь не попала на активные элементы
			//

			if (DialogMode.Check(DMODE_ISCANMOVE)) {
				// DialogMode.Set(DMODE_DRAGGED);
				OldX1 = X1;
				OldX2 = X2;
				OldY1 = Y1;
				OldY2 = Y2;
				// запомним delta места хватания и Left-Top диалогового окна
				MsX = abs(X1 - MouseX);
				MsY = abs(Y1 - MouseY);
				int NeedSendMsg = 0;

				for (;;) {
					DWORD Mb = IsMouseButtonPressed();
					if (Mb == FROM_LEFT_1ST_BUTTON_PRESSED)		// still dragging
					{
						const int NX1 = (MouseX == PrevMouseX) ? X1 : MouseX - MsX;
						const int NY1 = (MouseY == PrevMouseY) ? Y1 : MouseY - MsY;

						const int NX2 = NX1 + (X2 - X1);
						const int NY2 = NY1 + (Y2 - Y1);

						const int AdjX = NX1 - X1;
						const int AdjY = NY1 - Y1;

						// "А был ли мальчик?" (про холостой ход)
						if (AdjX || AdjY) {
							if (!NeedSendMsg)		// тыкс, а уже посылку делали в диалоговую процедуру?
							{
								NeedSendMsg++;

								if (!DlgProc(DN_DRAGGED, 0, 0))	// а может нас обломали?
									break;										// валим отсель...плагин сказал - в морг перемещения

								if (!DialogMode.Check(DMODE_SHOW))
									break;
							}

							// Да, мальчик был. Зачнем...
							{
								SCOPED_ACTION(LockScreen);
								Hide();
								X1 = NX1;
								X2 = NX2;
								Y1 = NY1;
								Y2 = NY2;

								AdjustEditPos(AdjX, AdjY);	//?
								Show();
							}
						}
					} else if (Mb == RIGHTMOST_BUTTON_PRESSED)		// abort
					{
						SCOPED_ACTION(LockScreen);
						Hide();
						AdjustEditPos(OldX1 - X1, OldY1 - Y1);
						X1 = OldX1;
						X2 = OldX2;
						Y1 = OldY1;
						Y2 = OldY2;
						DialogMode.Clear(DMODE_DRAGGED);
						DlgProc(DN_DRAGGED, 1, TRUE);

						if (DialogMode.Check(DMODE_SHOW))
							Show();

						break;
					} else		// release key, drop dialog
					{
						if (OldX1 != X1 || OldX2 != X2 || OldY1 != Y1 || OldY2 != Y2) {
							SCOPED_ACTION(LockScreen);
							DialogMode.Clear(DMODE_DRAGGED);
							DlgProc(DN_DRAGGED, 1, 0);

							if (DialogMode.Check(DMODE_SHOW))
								Show();
						}

						break;
					}
				}	// while (true)
			}
		}
	}

	return FALSE;
}

bool Dialog::ProcessOpenComboBox(int Type, DialogItemEx &CurItem, int CurFocusPos)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	FARString strStr;
	DlgEdit *CurEditLine;

	// для user-типа вываливаем
	if (Type == DI_USERCONTROL)
		return true;

	CurEditLine = CurItem.GetEdit();

	if (FarIsEdit(Type) && (CurItem.Flags & DIF_HISTORY) && Opt.Dialogs.EditHistory
			&& !CurItem.strHistory.IsEmpty() && !(CurItem.Flags & DIF_READONLY)) {
		// Передаем то, что в строке ввода в функцию выбора из истории для выделения нужного пункта в истории.
		CurEditLine->GetString(strStr);
		SelectFromEditHistory(CurItem, CurEditLine, CurItem.strHistory, strStr);
	}
	// $ 18.07.2000 SVS: +обработка DI_COMBOBOX - выбор из списка!
	else if (Type == DI_COMBOBOX && CurItem.ListPtr && !(CurItem.Flags & DIF_READONLY)
			&& CurItem.ListPtr->GetItemCount() > 0)	//??
	{
		SelectFromComboBox(CurItem, CurEditLine, CurItem.ListPtr);
	}

	return true;
}

int Dialog::ProcessRadioButton(int CurRB)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	int PrevRB = CurRB;
	int I;

	for (I = CurRB; I; I--) {
		if (Item[I].Type == DI_RADIOBUTTON && (Item[I].Flags & DIF_GROUP))
			break;

		if (Item[I - 1].Type != DI_RADIOBUTTON)
			break;
	}

	do {
		/*
			$ 28.07.2000 SVS
			При изменении состояния каждого элемента посылаем сообщение
			посредством функции SendDlgMessage - в ней делается все!
		*/
		if (Item[I].Selected) {
			PrevRB = I;
			Item[I].Selected = 0;
		}

		++I;
	} while (I < ItemCount() && Item[I].Type == DI_RADIOBUTTON && !(Item[I].Flags & DIF_GROUP));

	Item[CurRB].Selected = 1;

	/*
		$ 28.07.2000 SVS
		При изменении состояния каждого элемента посылаем сообщение
		посредством функции SendDlgMessage - в ней делается все!
	*/
	if (!SendDlgMessage(DN_BTNCLICK, PrevRB, 0) || !SendDlgMessage(DN_BTNCLICK, CurRB, 1)) {
		// вернем назад, если пользователь не захотел...
		Item[CurRB].Selected = 0;
		Item[PrevRB].Selected = 1;
		return PrevRB;
	}

	return CurRB;
}

bool Dialog::Do_ProcessFirstCtrl()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (FarIsEdit(Item[FocusPos].Type)) {
		Item[FocusPos].GetEdit()->ProcessKey(KEY_HOME);
		return true;
	} else {
		for (int I = 0; I < ItemCount(); I++)
			if (Item[I].IsFocusable()) {
				ChangeFocus2(I);
				ShowDialog();
				break;
			}
	}

	return true;
}

bool Dialog::Do_ProcessNextCtrl(bool Up, bool IsRedraw)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	int OldPos = FocusPos;
	int PrevPos = 0;

	if (FarIsEdit(Item[FocusPos].Type) && (Item[FocusPos].Flags & DIF_EDITOR))
		PrevPos = Item[FocusPos].GetEdit()->GetCurPos();

	int I = ChangeFocus(FocusPos, Up ? -1 : 1, false);
	Item[FocusPos].Focus = 0;
	Item[I].Focus = 1;
	ChangeFocus2(I);

	if (FarIsEdit(Item[I].Type) && (Item[I].Flags & DIF_EDITOR))
		Item[I].GetEdit()->SetCurPos(PrevPos);

	if (Item[FocusPos].Type == DI_RADIOBUTTON && (Item[I].Flags & DIF_MOVESELECT))
		ProcessKey(KEY_SPACE);
	else if (IsRedraw) {
		ShowDialog(OldPos);
		ShowDialog(FocusPos);
	}

	return true;
}

bool Dialog::MoveToCtrlHorizontal(bool right)
{
	int MinDist     = RealWidth;
	int	LeftBorder  = 0;
	int	RightBorder = RealWidth;
	int	Dist        = 0;
	int	MinPos      = 0;

	for (int I = 0; I < ItemCount(); I++) {
		//first, let's find nearest borders
		if (Item[I].IsHorizontalSeparator()) {
			if (Item[I].X1 < Item[FocusPos].X1){
				if (LeftBorder < Item[I].X1) {
					LeftBorder = Item[I].X1;
				}
			} else if (Item[I].X1 > Item[FocusPos].X1) {
				if (RightBorder > Item[I].X1) {
					RightBorder = Item[I].X1;
				}
			}
		}

		//find nearest item _inside_ nearest borders
		if (I != FocusPos && Item[I].IsFocusable() && Item[I].Y1 == Item[FocusPos].Y1) {
			Dist = Item[I].X1 - Item[FocusPos].X1;

			if ((!right && Dist < 0 &&(Item[I].X1 > LeftBorder))
				|| (right && Dist > 0 &&(Item[I].X1 < RightBorder))
			) {
				if (abs(Dist) < MinDist) {
					MinDist = abs(Dist);
					MinPos = I;
				}
			}
		}
	}

	//MinDist still equal to RealWidth,
	//it means current line inside block of items has no focusable controls
	//fallback to Do_ProcessNextCtrl
	if (MinDist < RealWidth) {
		ChangeFocus2(MinPos);

		if (Item[MinPos].Flags & DIF_MOVESELECT)
			Do_ProcessSpace();
		else
			ShowDialog();
	}
	else
		return Do_ProcessNextCtrl(!right);

	return true;
}

bool Dialog::MoveToCtrlVertical(bool up)
{
	int MinDist      = RealHeight;
	int UpperBorder  = 0;
	int BottomBorder = RealHeight;
	int Dist         = 0;
	int MinPos       = 0;

	for (int I = 0; I < ItemCount(); I++) {
		//first, let's find nearest borders
		if (Item[I].IsVerticalSeparator()) {
			if (Item[I].Y1 < Item[FocusPos].Y1){
				if (UpperBorder < Item[I].Y1) {
					UpperBorder = Item[I].Y1;
				}
			} else if (Item[I].Y1 > Item[FocusPos].Y1) {
				if (BottomBorder > Item[I].Y1) {
					BottomBorder = Item[I].Y1;
				}
			}
		}

		//find nearest item _inside_ nearest borders
		if (I != FocusPos && Item[I].IsFocusable() && Item[I].X1 == Item[FocusPos].X1) {
			Dist = Item[I].Y1 - Item[FocusPos].Y1;

			if ((up && Dist < 0 && (Item[I].Y1 > UpperBorder))
				|| (!up && Dist > 0 && (Item[I].Y1 < BottomBorder))
			) {
				if (abs(Dist) < MinDist) {
					MinDist = abs(Dist);
					MinPos = I;
				}
			}
		}
	}

	//current column inside block of items has no focusable controls
	//gap more than one line considered as "native" block separator
	//fallback to Do_ProcessNextCtrl
	if (MinDist < 3) {
		ChangeFocus2(MinPos);

		if (Item[MinPos].Flags & DIF_MOVESELECT)
			Do_ProcessSpace();
		else
			ShowDialog();
	}
	else
		return Do_ProcessNextCtrl(up);

	return true;
}

bool Dialog::Do_ProcessTab(bool Next)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	int I;

	if (ItemCount() > 1) {
		// Must check for DI_EDIT since DIF_EDITOR and DIF_LISTNOAMPERSAND are equal
		if (Item[FocusPos].Type==DI_EDIT && (Item[FocusPos].Flags & DIF_EDITOR)) {
			I = FocusPos;

			while (Item[I].Type==DI_EDIT && (Item[I].Flags & DIF_EDITOR))
				I = ChangeFocus(I, Next ? 1 : -1, true);
		} else {
			I = ChangeFocus(FocusPos, Next ? 1 : -1, true);

			if (!Next)
				while (I > 0
						&& Item[I].Type == DI_EDIT && Item[I - 1].Type == DI_EDIT
						&& (Item[I].Flags & DIF_EDITOR) && (Item[I - 1].Flags & DIF_EDITOR)
						&& !Item[I].GetEdit()->GetLength())
					I--;
		}
	}
	else
		I = FocusPos;

	ChangeFocus2(I);
	ShowDialog();

	return true;
}

bool Dialog::Do_ProcessSpace()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (Item[FocusPos].Type == DI_CHECKBOX) {
		int OldSelected = Item[FocusPos].Selected;

		if (Item[FocusPos].Flags & DIF_3STATE) {
			Item[FocusPos].Selected = (Item[FocusPos].Selected + 1) % 3;
		} else
			Item[FocusPos].Selected = !Item[FocusPos].Selected;

		int OldFocusPos = FocusPos;

		if (!SendDlgMessage(DN_BTNCLICK, FocusPos, Item[FocusPos].Selected))
			Item[OldFocusPos].Selected = OldSelected;

		ShowDialog();
		return true;
	} else if (Item[FocusPos].Type == DI_RADIOBUTTON) {
		FocusPos = ProcessRadioButton(FocusPos);
		ShowDialog();
		return true;
	} else if (FarIsEdit(Item[FocusPos].Type) && !(Item[FocusPos].Flags & DIF_READONLY)) {
		if (Item[FocusPos].GetEdit()->ProcessKey(KEY_SPACE)) {
			Redraw();	// Перерисовка должна идти после DN_EDITCHANGE (imho)
		}

		return true;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
/*
	Private:
	Изменяет фокус ввода (воздействие клавишами
	KEY_TAB, KEY_SHIFTTAB, KEY_UP, KEY_DOWN,
	а так же Alt-HotKey)
*/
/*
	$ 28.07.2000 SVS
	Довесок для сообщений DN_KILLFOCUS & DN_SETFOCUS
*/
/*
	$ 24.08.2000 SVS
	Добавка для DI_USERCONTROL
*/
int Dialog::ChangeFocus(int CurFocusPos, int Step, bool SkipGroup)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	int OrigFocusPos = CurFocusPos;

	for (;;) {
		CurFocusPos += Step;

		if (CurFocusPos < 0) {
			CurFocusPos = ItemCount() - 1;
		}

		if (CurFocusPos >= ItemCount()) {
			CurFocusPos = 0;
		}

		if (Item[CurFocusPos].IsFocusable()) {
			//move straight to selected radio when SkipGroup is true
			if (Item[CurFocusPos].Type != DI_RADIOBUTTON || (SkipGroup && Item[CurFocusPos].Selected))
				break;
		}

		// убираем зацикливание с последующим подвисанием :-)
		if (OrigFocusPos == CurFocusPos)
			break;
	}

	return CurFocusPos;
}

//////////////////////////////////////////////////////////////////////////
/*
	Private:
	Изменяет фокус ввода между двумя элементами.
	Вынесен отдельно с тем, чтобы обработать DN_KILLFOCUS & DM_SETFOCUS
*/
void Dialog::ChangeFocus2(int SetFocusPos)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	int FocusPosNeed = -1;

	if (Item[SetFocusPos].IsFocusable()) {
		if (DialogMode.Check(DMODE_INITOBJECTS)) {
			FocusPosNeed = (int)DlgProc(DN_KILLFOCUS, FocusPos, 0);

			if (!DialogMode.Check(DMODE_SHOW))
				return;
		}

		if (FocusPosNeed >= 0 && FocusPosNeed < ItemCount() && Item[FocusPosNeed].IsFocusable())
			SetFocusPos = FocusPosNeed;

		Item[FocusPos].Focus = 0;

		// "снимать выделение при потере фокуса?"
		if (FarIsEdit(Item[FocusPos].Type)
				&& !(Item[FocusPos].Type == DI_COMBOBOX && (Item[FocusPos].Flags & DIF_DROPDOWNLIST))) {
			DlgEdit *EditPtr = Item[FocusPos].GetEdit();
			EditPtr->GetSelection(Item[FocusPos].SelStart, Item[FocusPos].SelEnd);

			if ((Opt.Dialogs.EditLine & DLGEDITLINE_CLEARSELONKILLFOCUS)) {
				EditPtr->Select(-1, 0);
			}
		}

		Item[SetFocusPos].Focus = 1;

		// "не восстанавливать выделение при получении фокуса?"
		if (FarIsEdit(Item[SetFocusPos].Type)
				&& !(Item[SetFocusPos].Type == DI_COMBOBOX
						&& (Item[SetFocusPos].Flags & DIF_DROPDOWNLIST)))
		{
			DlgEdit *EditPtr = Item[SetFocusPos].GetEdit();

			if (!(Opt.Dialogs.EditLine & DLGEDITLINE_NOTSELONGOTFOCUS)) {
				if (Opt.Dialogs.EditLine & DLGEDITLINE_SELALLGOTFOCUS)
					EditPtr->Select(0, EditPtr->GetStrSize());
				else
					EditPtr->Select(Item[SetFocusPos].SelStart, Item[SetFocusPos].SelEnd);
			} else {
				EditPtr->Select(-1, 0);
			}

			// при получении фокуса ввода переместить курсор в конец строки?
			if (Opt.Dialogs.EditLine & DLGEDITLINE_GOTOEOLGOTFOCUS) {
				EditPtr->SetCurPos(EditPtr->GetStrSize());
			}
		}

		// проинформируем листбокс, есть ли у него фокус
		if (Item[FocusPos].Type == DI_LISTBOX)
			Item[FocusPos].ListPtr->ClearFlags(VMENU_LISTHASFOCUS);

		if (Item[SetFocusPos].Type == DI_LISTBOX)
			Item[SetFocusPos].ListPtr->SetFlags(VMENU_LISTHASFOCUS);

		SelectOnEntry(FocusPos, false);
		SelectOnEntry(SetFocusPos, true);

		PrevFocusPos = FocusPos;
		FocusPos = SetFocusPos;

		SetMacroArea(Item[FocusPos].Type == DI_MEMOEDIT ? MACROAREA_MEMOEDIT : MACROAREA_DIALOG);

		if (DialogMode.Check(DMODE_INITOBJECTS))
			DlgProc(DN_GOTFOCUS, FocusPos, 0);
	}
}

/*
	Функция SelectOnEntry - выделение строки редактирования
	Обработка флага DIF_SELECTONENTRY
*/
void Dialog::SelectOnEntry(int Pos, bool Selected)
{
	// if(!DialogMode.Check(DMODE_SHOW))
	//	return;
	if (FarIsEdit(Item[Pos].Type) && (Item[Pos].Flags & DIF_SELECTONENTRY))
			//		&& PrevFocusPos != -1 && PrevFocusPos != Pos
	{
		if (auto edt = Item[Pos].GetEdit()) {
			if (Selected)
				edt->Select(0, edt->GetLength());
			else
				edt->Select(-1, 0);
		}
	}
}

bool Dialog::SetAutomation(int IDParent, int id, FarDialogItemFlags UncheckedSet,
		FarDialogItemFlags UncheckedSkip, FarDialogItemFlags CheckedSet, FarDialogItemFlags CheckedSkip,
		FarDialogItemFlags Checked3Set, FarDialogItemFlags Checked3Skip)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	bool Ret = false;

	if (IDParent < ItemCount() && (Item[IDParent].Flags & DIF_AUTOMATION) && id < ItemCount()
			&& IDParent != id)		// Сами себя не юзаем!
	{
		Ret = Item[IDParent].AddAutomation(id, UncheckedSet, UncheckedSkip, CheckedSet, CheckedSkip,
				Checked3Set, Checked3Skip);
	}

	return Ret;
}

/*
	Private:
	Заполняем выпадающий список для ComboBox
*/
int Dialog::SelectFromComboBox(DialogItemEx &CurItem,
		DlgEdit *EditLine,		// строка редактирования
		VMenu *ComboBox)		// список строк
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	FARString strStr;
	int Dest, OriginalPos;
	int CurFocusPos = FocusPos;

	SetDropDownOpened(true);	// Установим флаг "открытия" комбобокса.
	DlgProc(DN_DROPDOWNOPENED, FocusPos, 1);
	SetComboBoxPos(&CurItem);
	// Перед отрисовкой спросим об изменении цветовых атрибутов
	uint64_t RealColors[VMENU_COLOR_COUNT];
	FarListColors ListColors = {0};
	ListColors.ColorCount = VMENU_COLOR_COUNT;
	ListColors.Colors = RealColors;
	ComboBox->SetColors(nullptr);
	ComboBox->GetColors(&ListColors);

	if (DlgProc(DN_CTLCOLORDLGLIST, CurItem.ID, (LONG_PTR)&ListColors))
		ComboBox->SetColors(&ListColors);

	// Выставим то, что есть в строке ввода!
	// if(EditLine->GetDropDownBox()) //???
	EditLine->GetString(strStr);

	if (CurItem.Flags & (DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND))
		HiText2Str(strStr, strStr);

	if (!strStr.IsEmpty()) {
		ComboBox->SetSelectPos(ComboBox->FindItem(0, strStr, LIFIND_EXACTMATCH), 1);
	}
	ComboBox->Show();
	OriginalPos = Dest = ComboBox->GetSelectPos();
	CurItem.IFlags.Set(DLGIIF_COMBOBOXNOREDRAWEDIT);

	while (!ComboBox->Done()) {
		if (!GetDropDownOpened()) {
			ComboBox->ProcessKey(KEY_ESC);
			continue;
		}

		INPUT_RECORD ReadRec;
		FarKey Key = ComboBox->ReadInput(&ReadRec);

		if (CurItem.IFlags.Check(DLGIIF_COMBOBOXEVENTKEY) && ReadRec.EventType == KEY_EVENT) {
			if (DlgProc(DN_KEY, FocusPos, Key))
				continue;
		}
		else if (CurItem.IFlags.Check(DLGIIF_COMBOBOXEVENTMOUSE) && ReadRec.EventType == MOUSE_EVENT) {
			if (!DlgProc(DN_MOUSEEVENT, 0, (LONG_PTR)&ReadRec.Event.MouseEvent))
				continue;
		}

		// здесь можно добавить что-то свое, например,
		int I = ComboBox->GetSelectPos();

		if (Key == KEY_TAB)		// Tab в списке - аналог Enter
		{
			ComboBox->ProcessKey(KEY_ENTER);
			continue;	//??
		}

		if (I != Dest) {
			if (!DlgProc(DN_LISTCHANGE, CurFocusPos, I))
				ComboBox->SetSelectPos(Dest, Dest < I ? -1 : 1);	//????
			else
				Dest = I;

#if 0

			// во время навигации по DropDown листу - отобразим ЭТО дело в
			// связанной строке
			// ВНИМАНИЕ!!!
			// Очень медленная реакция!
			if (EditLine->GetDropDownBox())
			{
				MenuItem *CurCBItem=ComboBox->GetItemPtr();
				EditLine->SetString(CurCBItem->Name);
				EditLine->Show();
				//EditLine->FastShow();
			}

#endif
		}

		// обработку multiselect ComboBox
		// ...
		ComboBox->ProcessInput();
	}

	CurItem.IFlags.Clear(DLGIIF_COMBOBOXNOREDRAWEDIT);
	ComboBox->ClearDone();
	ComboBox->Hide();

	if (GetDropDownOpened())	// Закрылся не программным путём?
		Dest = ComboBox->Modal::GetExitCode();
	else
		Dest = -1;

	if (Dest == -1)
		ComboBox->SetSelectPos(OriginalPos, 0);		//????

	SetDropDownOpened(false);						// Установим флаг "закрытия" комбобокса.
	DlgProc(DN_DROPDOWNOPENED, FocusPos, 0);

	if (Dest < 0) {
		Redraw();
		return KEY_ESC;
	}

	// ComboBox->GetUserData(Str,MaxLen,Dest);
	MenuItemEx *ItemPtr = ComboBox->GetItemPtr(Dest);

	if (CurItem.Flags & (DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND)) {
		HiText2Str(strStr, ItemPtr->strName);
		EditLine->SetString(strStr);
	} else
		EditLine->SetString(ItemPtr->strName);

	EditLine->SetLeftPos(0);
	Redraw();
	return KEY_ENTER;
}

//////////////////////////////////////////////////////////////////////////
/*
	Private:
	Заполняем выпадающий список из истории
*/
bool Dialog::SelectFromEditHistory(DialogItemEx &CurItem, DlgEdit *EditLine, const wchar_t *HistoryName,
		FARString &strIStr)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (!EditLine)
		return false;

	FARString strStr;
	int ret = HRT_CANCEL;
	FARString strRegKey = fmtSavedDialogHistory;
	strRegKey+= HistoryName;
	History DlgHist(HISTORYTYPE_DIALOG, Opt.DialogsHistoryCount, strRegKey.GetMB(), &Opt.Dialogs.EditHistory,
			false);
	DlgHist.ResetPosition();
	{
		// создание пустого вертикального меню
		VMenu HistoryMenu(L"", nullptr, 0, Opt.Dialogs.CBoxMaxHeight,
				VMENU_ALWAYSSCROLLBAR | VMENU_COMBOBOX | VMENU_NOTCHANGE);
		HistoryMenu.SetFlags(VMENU_SHOWAMPERSAND);
		HistoryMenu.SetBoxType(SHORT_SINGLE_BOX);
		HistoryMenu.SetId(SelectFromEditHistoryId);

		// запомним (для прорисовки)
		CurItem.ListPtr = &HistoryMenu;
		SetDropDownOpened(true);		// Установим флаг "открытия" комбобокса.
		DlgProc(DN_DROPDOWNOPENED, FocusPos, 1);

		ret = DlgHist.Select(HistoryMenu, this, strStr);

		SetDropDownOpened(false);		// Установим флаг "закрытия" комбобокса.
		DlgProc(DN_DROPDOWNOPENED, FocusPos, 0);
		// забудем (не нужен)
		CurItem.ListPtr = nullptr;
	}

	if (ret != HRT_CANCEL) {
		EditLine->SetString(strStr);
		EditLine->SetLeftPos(0);
		EditLine->SetClearFlag(false);
		Redraw();
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
/*
	Private:
	Работа с историей - добавление и reorder списка
*/
int Dialog::AddToEditHistory(const wchar_t *AddStr, const wchar_t *HistoryName)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	FARString strRegKey = fmtSavedDialogHistory;
	strRegKey+= HistoryName;
	History DlgHist(HISTORYTYPE_DIALOG, Opt.DialogsHistoryCount, strRegKey.GetMB(),
			&Opt.Dialogs.EditHistory, false);
	DlgHist.AddToHistory(AddStr);
	return TRUE;
}

int Dialog::CheckHighlights(WORD CheckSymbol, int StartPos)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (StartPos < 0)
		StartPos = 0;

	for (int I = StartPos; I < ItemCount(); I++) {
		int Type = Item[I].Type;
		DWORD Flags = Item[I].Flags;

		if ((!FarIsEdit(Type) || (Type == DI_COMBOBOX && (Flags & DIF_DROPDOWNLIST)))
				&& !(Flags & (DIF_SHOWAMPERSAND | DIF_DISABLE | DIF_HIDDEN))) {
			const wchar_t *ChPtr = wcschr(Item[I].strData, L'&');

			if (ChPtr) {
				WORD Ch = ChPtr[1];

				if (Ch && Upper(CheckSymbol) == Upper(Ch))
					return I;
			} else if (!CheckSymbol)
				return I;
		}
	}

	return -1;
}

/*
	Private:
	Если жмакнули Alt-???
*/
bool Dialog::ProcessHighlighting(FarKey Key, int FocusPos, bool Translate)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	for (int I = 0; I < ItemCount(); I++) {
		int Type = Item[I].Type;
		DWORD Flags = Item[I].Flags;

		if ((!FarIsEdit(Type) || (Type == DI_COMBOBOX && (Flags & DIF_DROPDOWNLIST)))
				&& !(Flags & (DIF_SHOWAMPERSAND | DIF_DISABLE | DIF_HIDDEN)))
		{
			if (IsKeyHighlighted(Item[I].strData, Key, Translate))
			{
				bool DisableSelect = false;

				// Если ЭТО: DlgEdit(пред контрол) и DI_TEXT в одну строку, то...
				if (I > 0 && Type == DI_TEXT &&										// DI_TEXT
						FarIsEdit(Item[I - 1].Type) &&								// и редактор
						Item[I].Y1 == Item[I - 1].Y1 &&							// и оба в одну строку
						(I + 1 < ItemCount() && Item[I].Y1 != Item[I + 1].Y1))		// ...и следующий контрол в другой строке
				{
					// Сначала сообщим о случившемся факте процедуре обработки диалога, а потом...
					if (!DlgProc(DN_HOTKEY, I, Key))
						break;	// сказали не продолжать обработку...

					// ... если предыдущий контрол задизаблен или невидим, тогда выходим.
					if ((Item[I - 1].Flags & (DIF_DISABLE | DIF_HIDDEN)))	// и не задисаблен
						break;

					I = ChangeFocus(I, -1, false);
					DisableSelect = true;
				}
				else if (Item[I].Type == DI_TEXT || Item[I].Type == DI_VTEXT
						|| Item[I].Type == DI_SINGLEBOX || Item[I].Type == DI_DOUBLEBOX)
				{
					if (I + 1 < ItemCount())		// ...и следующий контрол
					{
						// Сначала сообщим о случившемся факте процедуре обработки диалога, а потом...
						if (!DlgProc(DN_HOTKEY, I, Key))
							break;	// сказали не продолжать обработку...

						// ... если следующий контрол задизаблен или невидим, тогда выходим.
						if (Item[I + 1].Flags & (DIF_DISABLE | DIF_HIDDEN))	// и не задисаблен
							break;

						I = ChangeFocus(I, 1, false);
						DisableSelect = true;
					}
				}

				// Сообщим о случивщемся факте процедуре обработки диалога
				if (!DlgProc(DN_HOTKEY, I, Key))
					break;	// сказали не продолжать обработку...

				ChangeFocus2(I);
				ShowDialog();

				if ((Item[I].Type == DI_CHECKBOX || Item[I].Type == DI_RADIOBUTTON)
						&& (!DisableSelect || (Item[I].Flags & DIF_MOVESELECT)))
				{
					Do_ProcessSpace();
					return true;
				}
				else if (Item[I].Type == DI_BUTTON) {
					ProcessKey(KEY_ENTER, I);
					return true;
				}
				// при ComboBox`е - "вываливаем" последний //????
				else if (Item[I].Type == DI_COMBOBOX) {
					ProcessOpenComboBox(Item[I].Type, Item[I], I);
					// ProcessKey(KEY_CTRLDOWN);
					return true;
				}

				return true;
			}
		}
	}

	return false;
}

/*
	функция подравнивания координат edit классов
*/
void Dialog::AdjustEditPos(int dx, int dy)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (!DialogMode.Check(DMODE_CREATEOBJECTS))
		return;

	ScreenObject *DialogScrObject;

	for (const auto &CurItem: Item) {
		int Type = CurItem.Type;

		if ((CurItem.ObjPtr && FarIsEdit(Type)) || (CurItem.ListPtr && Type == DI_LISTBOX)) {
			if (Type == DI_LISTBOX)
				DialogScrObject = (ScreenObject *)CurItem.ListPtr;
			else
				DialogScrObject = (ScreenObject *)CurItem.ObjPtr;

			int x1, x2, y1, y2;
			DialogScrObject->GetPosition(x1, y1, x2, y2);
			x1 += dx;
			x2 += dx;
			y1 += dy;
			y2 += dy;
			DialogScrObject->SetPosition(x1, y1, x2, y2);
		}
	}

	ProcessCenterGroup();
}

/*
	Работа с доп. данными экземпляра диалога
	Пока простое копирование (присвоение)
*/
void Dialog::SetDialogData(LONG_PTR NewDataDialog)
{
	DataDialog = NewDataDialog;
}

/*
	$ 29.06.2007 yjh\
	При расчётах времён копирования проще/надёжнее учитывать время ожидания
	пользовательских ответов в одном месте (здесь).
	Сброс этой переменной должен осуществляться перед общим началом операции
*/
long WaitUserTime;

/*
	$ 11.08.2000 SVS
	+ Для того, чтобы послать DM_CLOSE нужно переопределить Process
*/
static std::atomic<int> s_in_dialog{0};

void Dialog::Process()
{
//  if(DialogMode.Check(DMODE_SMALLDIALOG))
	SetRestoreScreenMode(true);
	ClearDone();
	InitDialog();

	if (ExitCode == -1) {
		DialogMode.Set(DMODE_BEGINLOOP);

		if (GetCanLoseFocus()) {
			FrameManager->InsertFrame(this);
		}
		else {
			clock_t btm = 0;
			long save = 0;

			if (1 == ++s_in_dialog) {
				btm = GetProcessUptimeMSec();
				save = WaitUserTime;
				WaitUserTime = -1;
			}

			FrameManager->ExecuteModal(this);
			save += (GetProcessUptimeMSec() - btm);

			if (0 == --s_in_dialog)
				WaitUserTime = save;
		}
	}

	if (pSaveItemEx)
		for (int i = 0; i < ItemCount(); i++)
			Item[i].ToDialogItemEx(&pSaveItemEx[i]);
}

void Dialog::CloseDialog()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	GetDialogObjectsData();

	if (DlgProc(DN_CLOSE, ExitCode, 0)) {
		DialogMode.Set(DMODE_ENDLOOP);
		Hide();

		if (DialogMode.Check(DMODE_BEGINLOOP)
				&& (DialogMode.Check(DMODE_MSGINTERNAL) || FrameManager->ManagerStarted()))
		{
			DialogMode.Clear(DMODE_BEGINLOOP);
			FrameManager->DeleteFrame(this);

			if (!GetDynamicallyBorn())  //this condition prevents crash "delete(this)" with non-modal plugin dialogs
			{
				FrameManager->Commit(1); // This fixes issues #28 and #58
			}
		}

		_DIALOG(CleverSysLog CL(L"Close Dialog"));
	}
}

/*
	$ 17.05.2001 DJ
	установка help topic'а и прочие радости, временно перетащенные сюда
	из Modal
*/
void Dialog::SetHelp(const wchar_t *Topic)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (Topic)
		HelpTopic = Topic;
	else
		HelpTopic.Clear();
}

void Dialog::ShowHelp()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (!HelpTopic.IsEmpty())
		Help Hlp(HelpTopic);
}

void Dialog::ClearDone()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	ExitCode = -1;
	DialogMode.Clear(DMODE_ENDLOOP);
}

void Dialog::SetExitCode(int Code)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	ExitCode = Code;
	DialogMode.Set(DMODE_ENDLOOP);
	// CloseDialog();
}

void Dialog::OnChangeFocus(bool focus)
{
	Frame::OnChangeFocus(focus);
	if (GetCanLoseFocus())
		DlgProc(focus ? DN_GOTFOCUS : DN_KILLFOCUS, -1, 0);
}

/*
	$ 19.05.2001 DJ
	возвращаем наше название для меню по F12
*/
int Dialog::GetTypeAndName(FARString &strType, FARString &strName)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	strType = Msg::DialogType;
	strName.Clear();
	const wchar_t *lpwszTitle = GetDialogTitle();

	if (lpwszTitle)
		strName = lpwszTitle;

	return MODALTYPE_DIALOG;
}

int Dialog::FastHide()
{
	return Opt.AllCtrlAltShiftRule & CASR_DIALOG;
}

void Dialog::ResizeConsole()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	DialogMode.Set(DMODE_RESIZED);

	if (IsVisible()) {
		Hide();
	}

	COORD c = {(SHORT)(ScrX + 1), (SHORT)(ScrY + 1)};
	SendDlgMessage(DN_RESIZECONSOLE, 0, reinterpret_cast<LONG_PTR>(&c));

	int x1, y1, x2, y2;
	GetPosition(x1, y1, x2, y2);
	c.X = Min(x1, ScrX - 1);
	c.Y = Min(y1, ScrY - 1);
	if (c.X != x1 || c.Y != y1) {
		c.X = x1;
		c.Y = y1;
		SendDlgMessage(DM_MOVEDIALOG, TRUE, reinterpret_cast<LONG_PTR>(&c));
		SetComboBoxPos();
	}
};

LONG_PTR Dialog::DlgProc(int Msg, int Param1, LONG_PTR Param2)
{
	if (DialogMode.Check(DMODE_ENDLOOP))
		return 0;

	FarDialogEvent de = {this, Msg, Param1, Param2, 0};

	if (!CheckDialogMode(DMODE_NOPLUGINS)) {
		if (CtrlObject->Plugins.ProcessDialogEvent(DE_DLGPROCINIT, &de))
			return de.Result;
	}
	LONG_PTR Result = RealDlgProc(this, Msg, Param1, Param2);
	if (!CheckDialogMode(DMODE_NOPLUGINS)) {
		de.Result = Result;
		if (CtrlObject->Plugins.ProcessDialogEvent(DE_DLGPROCEND, &de))
			return de.Result;
	}
	return Result;
}

//////////////////////////////////////////////////////////////////////////
/*
	$ 28.07.2000 SVS
	функция обработки диалога (по умолчанию)
	Вот именно эта функция и является последним рубежом обработки диалога.
	Т.е. здесь должна быть ВСЯ обработка ВСЕХ сообщений!!!
*/
LONG_PTR Dialog::DefDlgProc(int Msg, int Param1, LONG_PTR Param2)
{
	_DIALOG(CleverSysLog CL(L"Dialog.DefDlgProc()"));
	_DIALOG(SysLog(L"hDlg=%p, Msg=%ls, Param1=%d (0x%08X), Param2=%d (0x%08X)", this, _DLGMSG_ToName(Msg),
			Param1, Param1, Param2, Param2));

	FarDialogEvent de = {this, Msg, Param1, Param2, 0};

	if (!CheckDialogMode(DMODE_NOPLUGINS)) {
		if (CtrlObject->Plugins.ProcessDialogEvent(DE_DEFDLGPROCINIT, &de)) {
			return de.Result;
		}
	}
	SCOPED_ACTION(CriticalSectionLock)(CS);

	switch (Msg) {
		case DN_INITDIALOG:
			return FALSE;		// изменений не было!
		case DM_CLOSE:
			return TRUE;		// согласен с закрытием
		case DN_KILLFOCUS:
			return -1;			// "Согласен с потерей фокуса"
		case DN_GOTFOCUS:
			return 0;			// always 0
		case DN_HELP:
			return Param2;		// что передали, то и...
		case DN_DRAGGED:
			return TRUE;		// согласен с перемещалкой.
		case DN_DRAWDIALOGDONE: {
			if (Param1 == 1)	// Нужно отрисовать "салазки"?
			{
				/*
					$ 03.08.2000 tran
					вывод текста в углу может приводить к ошибкам изображения
					1) когда диалог перемещается в угол
					2) когда диалог перемещается из угла
					сделал вывод красных палочек по углам
				*/
				Text(X1, Y1, 0xCE, L"\\");
				Text(X1, Y2, 0xCE, L"/");
				Text(X2, Y1, 0xCE, L"/");
				Text(X2, Y2, 0xCE, L"\\");
			}

			return TRUE;
		}
		case DN_DRAWDIALOG: {
			return TRUE;
		}
		case DN_CTLCOLORDIALOG:
		case DN_CTLCOLORDLGITEM:
			return Param2;
		case DN_CTLCOLORDLGLIST:
		case DN_ENTERIDLE:
		case DM_GETDIALOGINFO:
			return FALSE;
	}

	// предварительно проверим...
	if (Param1 < 0 || Param1 >= ItemCount() || Item.empty())
		return 0;

	DialogItemEx &CurItem = Item[Param1];

	switch (Msg) {
		case DN_MOUSECLICK:
		case DN_KEY:
		case DM_GETSELECTION:	// Msg=DM_GETSELECTION, Param1=ID, Param2=*EditorSelect
		case DM_SETSELECTION:
			return FALSE;

		case DN_DRAWDLGITEM:
		case DN_HOTKEY:
		case DN_EDITCHANGE:
		case DN_LISTCHANGE:
		case DN_MOUSEEVENT:
			return TRUE;

		case DN_BTNCLICK:
			return CurItem.Type != DI_BUTTON || (CurItem.Flags & DIF_BTNNOCLOSE);
	}

	return 0;
}

LONG_PTR WINAPI DefDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (!hDlg || hDlg == INVALID_HANDLE_VALUE)
		return 0;

	Dialog *Dlg = reinterpret_cast<Dialog*>(hDlg);
	return Dlg->DefDlgProc(Msg, Param1, Param2);
}

LONG_PTR Dialog::CallDlgProc(int nMsg, int nParam1, LONG_PTR nParam2)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	return DlgProc(nMsg, nParam1, nParam2);
}

/*
	$ 28.07.2000 SVS
	Посылка сообщения диалогу
	Некоторые сообщения эта функция обрабатывает сама, не передавая управление
	обработчику диалога.
*/
LONG_PTR Dialog::SendDlgMessageSynched(int Msg, int Param1, LONG_PTR Param2)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	_DIALOG(CleverSysLog CL(L"Dialog.SendDlgMessage()"));
	_DIALOG(SysLog(L"hDlg=%p, Msg=%ls, Param1=%d (0x%08X), Param2=%d (0x%08X)", this, _DLGMSG_ToName(Msg),
			Param1, Param1, Param2, Param2));

	// Сообщения, касаемые только диалога и не затрагивающие элементы
	switch (Msg) {
			/*****************************************************************/
		case DM_RESIZEDIALOG:
			// изменим вызов RESIZE.
			Param1 = -1;
			[[fallthrough]];
			/*****************************************************************/
		case DM_MOVEDIALOG: {
			int W1 = X2 - X1 + 1;
			int H1 = Y2 - Y1 + 1;
			OldX1 = X1;
			OldY1 = Y1;
			OldX2 = X2;
			OldY2 = Y2;

			// переместили
			if (Param1 > 0)		// абсолютно?
			{
				X1 = ((COORD *)Param2)->X;
				Y1 = ((COORD *)Param2)->Y;
				X2 = W1;
				Y2 = H1;
				CheckDialogCoord();
			} else if (!Param1)		// значит относительно
			{
				X1+= ((COORD *)Param2)->X;
				Y1+= ((COORD *)Param2)->Y;
			} else		// Resize, Param2=width/height
			{
				int OldW1 = W1;
				int OldH1 = H1;
				W1 = ((COORD *)Param2)->X;
				H1 = ((COORD *)Param2)->Y;
				RealWidth = W1;
				RealHeight = H1;

				if (W1 < OldW1 || H1 < OldH1) {
					DialogMode.Set(DMODE_DRAWING);
					SMALL_RECT Rect;

					for (int I = 0; I < ItemCount(); I++) {
						DialogItemEx &cItem = Item[I];

						if (cItem.Flags & DIF_HIDDEN)
							continue;

						Rect.Left = cItem.X1;
						Rect.Top = cItem.Y1;

						if (cItem.X2 >= W1) {
							Rect.Right = cItem.X2 - (OldW1 - W1);
							Rect.Bottom = cItem.Y2;
							SetItemRect(I, &Rect);
						}

						if (cItem.Y2 >= H1) {
							Rect.Right = cItem.X2;
							Rect.Bottom = cItem.Y2 - (OldH1 - H1);
							SetItemRect(I, &Rect);
						}
					}

					DialogMode.Clear(DMODE_DRAWING);
				}
			}

			// проверили и скорректировали
			if (X1 + W1 < 0)
				X1 = -W1 + 1;

			if (Y1 + H1 < 0)
				Y1 = -H1 + 1;

			if (X1 > ScrX)
				X1 = ScrX;

			if (Y1 > ScrY)
				Y1 = ScrY;

			X2 = X1 + W1 - 1;
			Y2 = Y1 + H1 - 1;

			if (Param1 > 0)		// абсолютно?
			{
				CheckDialogCoord();
			}

			if (Param1 < 0)		// размер?
			{
				((COORD *)Param2)->X = X2 - X1 + 1;
				((COORD *)Param2)->Y = Y2 - Y1 + 1;
			} else {
				((COORD *)Param2)->X = X1;
				((COORD *)Param2)->Y = Y1;
			}

			bool Visible = IsVisible();	// && DialogMode.Check(DMODE_INITOBJECTS);

			if (Visible)
				Hide();

			// приняли.
			AdjustEditPos(X1 - OldX1, Y1 - OldY1);

			if (Visible)
				Show();	// только если диалог был виден

			return Param2;
		}
		/*****************************************************************/
		case DM_REDRAW: {
			if (DialogMode.Check(DMODE_INITOBJECTS))
				Show();

			return 0;
		}
		/*****************************************************************/
		case DM_ENABLEREDRAW: {
			int Prev = IsEnableRedraw;

			if (Param1 > 0) {
				IsEnableRedraw++;
				if (IsEnableRedraw == 1 && DialogMode.Check(DMODE_INITOBJECTS)) {
					ShowDialog();
					// Show();
					ScrBuf.Flush();
				}
			} else if (Param1 == 0)
				IsEnableRedraw--;

			return Prev;
		}
		/*****************************************************************/
		case DM_SHOWDIALOG: {
			//		if(IsEnableRedraw)
			{
				if (Param1) {
					/*
						$ 20.04.2002 KM
						Залочим прорисовку при прятании диалога, в противном
						случае ОТКУДА менеджер узнает, что отрисовывать
						объект нельзя!
					*/
					if (!IsVisible()) {
						Unlock();
						Show();
					}
				} else {
					if (IsVisible()) {
						Hide();
						Lock();
					}
				}
			}
			return 0;
		}
		/*****************************************************************/
		case DM_SETDLGDATA: {
			LONG_PTR PrewDataDialog = DataDialog;
			DataDialog = Param2;
			return PrewDataDialog;
		}
		/*****************************************************************/
		case DM_GETDLGDATA: {
			return DataDialog;
		}
		/*****************************************************************/
		case DM_KEY: {
			int *KeyArray = (int *)Param2;
			DialogMode.Set(DMODE_KEY);

			for (int I = 0; I < Param1; ++I)
				ProcessKey(KeyArray[I]);

			DialogMode.Clear(DMODE_KEY);
			return 0;
		}
		/*****************************************************************/
		case DM_CLOSE: {
			if (Param1 == -1)
				ExitCode = FocusPos;
			else
				ExitCode = Param1;

			CloseDialog();
			return TRUE;	// согласен с закрытием
		}
		/*****************************************************************/
		case DM_GETDLGRECT: {
			if (Param2) {
				auto rect = reinterpret_cast<SMALL_RECT*>(Param2);
				int x1, y1, x2, y2;
				GetPosition(x1, y1, x2, y2);
				rect->Left = x1;
				rect->Top = y1;
				rect->Right = x2;
				rect->Bottom = y2;
				return TRUE;
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_GETDROPDOWNOPENED:		// Param1=0; Param2=0
		{
			return GetDropDownOpened();
		}
		/*****************************************************************/
		case DM_KILLSAVESCREEN: {
			if (SaveScr)
				SaveScr->Discard();

			if (ShadowSaveScr)
				ShadowSaveScr->Discard();

			return TRUE;
		}
		/*****************************************************************/
		case DM_SETMOUSEEVENTNOTIFY:	// Param1 = 1 on, 0 off, -1 - get
		{
			int State = DialogMode.Check(DMODE_MOUSEEVENT);

			if (Param1 != -1) {
				if (!Param1)
					DialogMode.Clear(DMODE_MOUSEEVENT);
				else
					DialogMode.Set(DMODE_MOUSEEVENT);
			}

			return State;
		}
		/*****************************************************************/
		case DN_RESIZECONSOLE: {
			return CallDlgProc(Msg, Param1, Param2);
		}
		case DM_GETDIALOGINFO: {
			auto Result=FALSE;

			if (Param2)
			{
				DialogInfo *di=reinterpret_cast<DialogInfo*>(Param2);
				if (IdExist)
				{
					if (di->StructSize >= offsetof(DialogInfo, Id)+sizeof(di->Id))
					{
						di->Id=Id;
						Result=TRUE;
					}
				}

				if (di->StructSize >= offsetof(DialogInfo, Owner)+sizeof(di->Owner))
				{
					di->Owner = 0;
					if (PluginNumber != -1)
					{
						auto Plug = reinterpret_cast<Plugin*>(PluginNumber);
						di->Owner = Plug->GetSysID();
					}
				}
			}

			return Result;
		}
		/*****************************************************************/
		// Param1=0, Param2=FarDialogItemData, Ret=size (without '\0')
		case DM_GETDIALOGTITLE: {
			FarDialogItemData *did = (FarDialogItemData *)Param2;
			const wchar_t *strTitleDialog = GetDialogTitle();
			size_t Len = wcslen(strTitleDialog);
			if (did != nullptr) // если здесь nullptr, то это еще один способ получить размер
			{
				if (!did->PtrLength)
					did->PtrLength = Len;
				else if (Len > did->PtrLength)
					Len = did->PtrLength;

				if (did->PtrData)
				{
					wmemcpy(did->PtrData, strTitleDialog, Len);
					did->PtrData[Len] = 0;
				}
			}

			return Len;
		}
	}

	/*****************************************************************/
	if (Msg >= DM_USER) {
		return CallDlgProc(Msg, Param1, Param2);
	}

	/*
		предварительно проверим...
		$ 09.12.2001 DJ
		для DM_USER проверять _не_надо_!
	*/
	if (Param1 >= ItemCount() || Item.empty())
		return 0;

	size_t Len = 0;
	DialogItemEx &CurItem = Item[Param1];
	int Type = CurItem.Type;
	const wchar_t *Ptr = CurItem.strData;

	if (FarIsEdit(Type) && CurItem.GetEdit())
		Ptr = const_cast<const wchar_t *>(CurItem.GetEdit()->GetStringAddr());

	switch (Msg) {
			/*****************************************************************/
		case DM_LISTADD:
		case DM_LISTADDSTR:
		case DM_LISTDELETE:
		case DM_LISTFINDSTRING:
		case DM_LISTGETCURPOS:
		case DM_LISTGETDATA:
		case DM_LISTGETDATASIZE:
		case DM_LISTGETITEM:
		case DM_LISTGETTITLES:
		case DM_LISTINFO:
		case DM_LISTINSERT:
		case DM_LISTSET:
		case DM_LISTSETCURPOS:
		case DM_LISTSETDATA:
		case DM_LISTSETMOUSEREACTION:
		case DM_LISTSETTITLES:
		case DM_LISTSORT:
		case DM_LISTUPDATE:
		case DM_GETCOMBOBOXEVENT:
		case DM_SETCOMBOBOXEVENT:
		{
			if (Type == DI_LISTBOX || Type == DI_COMBOBOX) {
				VMenu *ListBox = CurItem.ListPtr;

				if (ListBox) {
					int Ret = TRUE;

					switch (Msg) {
						case DM_LISTINFO:		// Param1=ID Param2=FarListInfo
						{
							return ListBox->GetVMenuInfo((FarListInfo *)Param2);
						}
						case DM_LISTSORT:		// Param1=ID Param=Direct {0|1}
						{
							ListBox->SortItems((int)Param2);
							break;
						}
						case DM_LISTFINDSTRING:		// Param1=ID Param2=FarListFind
						{
							FarListFind *lf = reinterpret_cast<FarListFind *>(Param2);
							return ListBox->FindItem(lf->StartIndex, lf->Pattern, lf->Flags);
						}
						case DM_LISTADDSTR:		// Param1=ID Param2=String
						{
							Ret = ListBox->AddItem((wchar_t *)Param2);
							break;
						}
						case DM_LISTADD:	// Param1=ID Param2=FarList: ItemsNumber=Count, Items=Src
						{
							FarList *ListItems = (FarList *)Param2;

							if (!ListItems)
								return FALSE;

							Ret = ListBox->AddItem(ListItems);
							break;
						}
						case DM_LISTDELETE:		// Param1=ID Param2=FarListDelete: StartIndex=BeginIndex, Count=количество (<=0 - все!)
						{
							int Count;
							FarListDelete *ListItems = (FarListDelete *)Param2;

							if (!ListItems || (Count = ListItems->Count) <= 0)
								ListBox->DeleteItems();
							else
								ListBox->DeleteItem(ListItems->StartIndex, Count);

							break;
						}
						case DM_LISTINSERT:		// Param1=ID Param2=FarListInsert
						{
							if ((Ret = ListBox->InsertItem((FarListInsert *)Param2)) == -1)
								return -1;

							break;
						}
						case DM_LISTUPDATE:		// Param1=ID Param2=FarListUpdate: Index=Index, Items=Src
						{
							if (Param2 && ListBox->UpdateItem((FarListUpdate *)Param2))
								break;

							return FALSE;
						}
						case DM_LISTGETITEM:	// Param1=ID Param2=FarListGetItem: ItemsNumber=Index, Items=Dest
						{
							FarListGetItem *ListItems = (FarListGetItem *)Param2;

							if (!ListItems)
								return FALSE;

							MenuItemEx *ListMenuItem;

							if ((ListMenuItem = ListBox->GetItemPtr(ListItems->ItemIndex))) {
								// ListItems->ItemIndex=1;
								FarListItem *Item = &ListItems->Item;
								memset(Item, 0, sizeof(FarListItem));
								Item->Flags = ListMenuItem->Flags;
								Item->Text = ListMenuItem->strName;
								/*
								if(ListMenuItem->UserDataSize <= sizeof(DWORD)) //???
									Item->UserData=ListMenuItem->UserData;
								*/
								return TRUE;
							}

							return FALSE;
						}
						case DM_LISTGETDATA:	// Param1=ID Param2=Index
						{
							if (Param2 < ListBox->GetItemCount())
								return (LONG_PTR)ListBox->GetUserData(nullptr, 0, (int)Param2);

							return 0;
						}
						case DM_LISTGETDATASIZE:	// Param1=ID Param2=Index
						{
							if (Param2 < ListBox->GetItemCount())
								return ListBox->GetUserDataSize((int)Param2);

							return 0;
						}
						case DM_LISTSETDATA:	// Param1=ID Param2=FarListItemData
						{
							FarListItemData *ListItems = (FarListItemData *)Param2;

							if (ListItems && ListItems->Index < ListBox->GetItemCount()) {
								Ret = ListBox->SetUserData(ListItems->Data, ListItems->DataSize,
										ListItems->Index);

								if (!Ret && ListBox->GetUserData(nullptr, 0, ListItems->Index))
									Ret = sizeof(DWORD);

								return Ret;
							}

							return 0;
						}
						/*
							$ 02.12.2001 KM
							+ Сообщение для добавления в список строк, с удалением
							уже существующих, т.с. "чистая" установка
						*/
						case DM_LISTSET:	// Param1=ID Param2=FarList: ItemsNumber=Count, Items=Src
						{
							FarList *ListItems = (FarList *)Param2;

							if (!ListItems)
								return FALSE;

							ListBox->DeleteItems();
							Ret = ListBox->AddItem(ListItems);
							break;
						}
						// case DM_LISTINS: // Param1=ID Param2=FarList: ItemsNumber=Index, Items=Dest
						case DM_LISTSETTITLES:		// Param1=ID Param2=FarListTitles
						{
							FarListTitles *ListTitle = (FarListTitles *)Param2;
							ListBox->SetTitle(ListTitle->Title);
							ListBox->SetBottomTitle(ListTitle->Bottom);
							break;					// return TRUE;
						}
						case DM_LISTGETTITLES:		// Param1=ID Param2=FarListTitles
						{
							if (Param2) {
								FarListTitles *ListTitle = (FarListTitles *)Param2;
								FARString strTitle, strBottomTitle;
								ListBox->GetTitle(strTitle);
								ListBox->GetBottomTitle(strBottomTitle);

								if (!strTitle.IsEmpty() || !strBottomTitle.IsEmpty()) {
									if (ListTitle->Title && ListTitle->TitleLen)
										far_wcsncpy((wchar_t *)ListTitle->Title, strTitle,
												ListTitle->TitleLen);
									else
										ListTitle->TitleLen = (int)strTitle.GetLength() + 1;

									if (ListTitle->Bottom && ListTitle->BottomLen)
										far_wcsncpy((wchar_t *)ListTitle->Bottom, strBottomTitle,
												ListTitle->BottomLen);
									else
										ListTitle->BottomLen = (int)strBottomTitle.GetLength() + 1;

									return TRUE;
								}
							}

							return FALSE;
						}
						case DM_LISTGETCURPOS:		// Param1=ID Param2=FarListPos
						{
							return Param2 ? ListBox->GetSelectPos((FarListPos *)Param2)
										: ListBox->GetSelectPos();
						}
						case DM_LISTSETCURPOS:		// Param1=ID Param2=FarListPos Ret: RealPos
						{
							/* 26.06.2001 KM Подадим перед изменением позиции об этом сообщение */
							int CurListPos = ListBox->GetSelectPos();
							Ret = ListBox->SetSelectPos((FarListPos *)Param2);

							if (Ret != CurListPos)
								if (!CallDlgProc(DN_LISTCHANGE, Param1, Ret))
									Ret = ListBox->SetSelectPos(CurListPos, 1);

							break;							// т.к. нужно перерисовать!
						}
						case DM_LISTSETMOUSEREACTION:		// Param1=ID Param2=FARLISTMOUSEREACTIONTYPE Ret=OldSets
						{
							int OldSets = CurItem.IFlags.Flags;

							if (Param2 == LMRT_ONLYFOCUS) {
								CurItem.IFlags.Clear(DLGIIF_LISTREACTIONNOFOCUS);
								CurItem.IFlags.Set(DLGIIF_LISTREACTIONFOCUS);
							} else if (Param2 == LMRT_NEVER) {
								CurItem.IFlags.Clear(DLGIIF_LISTREACTIONNOFOCUS | DLGIIF_LISTREACTIONFOCUS);
								// ListBox->ClearFlags(VMENU_MOUSEREACTION);
							} else {
								CurItem.IFlags.Set(DLGIIF_LISTREACTIONNOFOCUS | DLGIIF_LISTREACTIONFOCUS);
								// ListBox->SetFlags(VMENU_MOUSEREACTION);
							}

							if ((OldSets & (DLGIIF_LISTREACTIONNOFOCUS | DLGIIF_LISTREACTIONFOCUS))
									== (DLGIIF_LISTREACTIONNOFOCUS | DLGIIF_LISTREACTIONFOCUS))
								OldSets = LMRT_ALWAYS;
							else if (!(OldSets & (DLGIIF_LISTREACTIONNOFOCUS | DLGIIF_LISTREACTIONFOCUS)))
								OldSets = LMRT_NEVER;
							else
								OldSets = LMRT_ONLYFOCUS;

							return OldSets;
						}
						case DM_GETCOMBOBOXEVENT:		// Param1=ID Param2=0 Ret=Sets
						{
							return (CurItem.IFlags.Check(DLGIIF_COMBOBOXEVENTKEY) ? CBET_KEY : 0)
									| (CurItem.IFlags.Check(DLGIIF_COMBOBOXEVENTMOUSE) ? CBET_MOUSE : 0);
						}
						case DM_SETCOMBOBOXEVENT:		// Param1=ID Param2=FARCOMBOBOXEVENTTYPE Ret=OldSets
						{
							int OldSets = CurItem.IFlags.Flags;
							CurItem.IFlags.Clear(DLGIIF_COMBOBOXEVENTKEY | DLGIIF_COMBOBOXEVENTMOUSE);

							if (Param2 & CBET_KEY)
								CurItem.IFlags.Set(DLGIIF_COMBOBOXEVENTKEY);

							if (Param2 & CBET_MOUSE)
								CurItem.IFlags.Set(DLGIIF_COMBOBOXEVENTMOUSE);

							return OldSets;
						}
					}

					// уточнение для DI_COMBOBOX - здесь еще и DlgEdit нужно корректно заполнить
					if (!CurItem.IFlags.Check(DLGIIF_COMBOBOXNOREDRAWEDIT) && Type == DI_COMBOBOX
							&& CurItem.ObjPtr) {
						MenuItemEx *ListMenuItem;

						if ((ListMenuItem = ListBox->GetItemPtr(ListBox->GetSelectPos()))) {
							if (CurItem.Flags & (DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND))
								CurItem.GetEdit()->SetHiString(ListMenuItem->strName);
							else
								CurItem.GetEdit()->SetString(ListMenuItem->strName);

							CurItem.GetEdit()->Select(-1, -1);		// снимаем выделение
						}
					}

					if (DialogMode.Check(DMODE_SHOW) && ListBox->UpdateRequired()) {
						ShowDialog(Param1);
						ScrBuf.Flush();
					}

					return Ret;
				}
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_SETHISTORY:		// Param1 = ID, Param2 = LPSTR HistoryName
		{
			if (Type == DI_EDIT || Type == DI_FIXEDIT) {
				if (Param2 && *(const wchar_t *)Param2) {
					CurItem.Flags|= DIF_HISTORY;
					CurItem.strHistory = (const wchar_t *)Param2;

					if (Type == DI_EDIT && (CurItem.Flags & DIF_USELASTHISTORY)) {
						ProcessLastHistory(CurItem, Param1);
					}
				} else {
					CurItem.Flags&= ~DIF_HISTORY;
					CurItem.strHistory.Clear();
				}

				if (DialogMode.Check(DMODE_SHOW)) {
					ShowDialog(Param1);
					ScrBuf.Flush();
				}

				return TRUE;
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_ADDHISTORY: {
			if (Param2 && (Type == DI_EDIT || Type == DI_FIXEDIT) && (CurItem.Flags & DIF_HISTORY)) {
				return AddToEditHistory((const wchar_t *)Param2, CurItem.strHistory);
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_GETCURSORPOS: {
			if (!Param2)
				return FALSE;

			if (FarIsEdit(Type) && CurItem.GetEdit()) {
				DlgEdit *EditPtr = CurItem.GetEdit();
				((COORD *)Param2)->X = EditPtr->GetCurPos();
				((COORD *)Param2)->Y = (Type == DI_MEMOEDIT) ? EditPtr->GetCurRow() : 0;
				return TRUE;
			} else if (Type == DI_USERCONTROL && CurItem.UCData) {
				((COORD *)Param2)->X = CurItem.UCData->CursorPos.X;
				((COORD *)Param2)->Y = CurItem.UCData->CursorPos.Y;
				return TRUE;
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_SETCURSORPOS: {
			if (FarIsEdit(Type) && CurItem.GetEdit() && ((COORD *)Param2)->X >= 0) {
				DlgEdit *EditPtr = CurItem.GetEdit();
				if (Type == DI_MEMOEDIT)
					EditPtr->SetCurPos(((COORD *)Param2)->X, ((COORD *)Param2)->Y);
				else
					EditPtr->SetCurPos(((COORD *)Param2)->X);
				// EditPtr->Show();
				ShowDialog(Param1);
				return TRUE;
			} else if (Type == DI_USERCONTROL && CurItem.UCData) {
				/*
					учтем, что координаты для этого элемента всегда относительные!
					и начинаются с 0,0
				*/
				COORD Coord = *(COORD *)Param2;
				Coord.X+= CurItem.X1;

				if (Coord.X > CurItem.X2)
					Coord.X = CurItem.X2;

				Coord.Y+= CurItem.Y1;

				if (Coord.Y > CurItem.Y2)
					Coord.Y = CurItem.Y2;

				// Запомним
				CurItem.UCData->CursorPos.X = Coord.X - CurItem.X1;
				CurItem.UCData->CursorPos.Y = Coord.Y - CurItem.Y1;

				// переместим если надо
				if (DialogMode.Check(DMODE_SHOW) && FocusPos == Param1) {
					// что-то одно надо убрать :-)
					MoveCursor(Coord.X + X1, Coord.Y + Y1);	// ???
					ShowDialog(Param1);							// ???
				}

				return TRUE;
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_GETEDITPOSITION: {
			if (Param2 && FarIsEdit(Type)) {
				if (Type == DI_MEMOEDIT) {
					EditorSetPosition *esp = (EditorSetPosition *)Param2;
					DlgEdit *EditPtr = CurItem.GetEdit();
					EditorInfo Info { sizeof(Info) };
					EditPtr->GetMemoEdit()->EditorControl(ECTL_GETINFO, &Info);
					esp->CurLine = Info.CurLine;
					esp->CurPos = Info.CurPos;
					esp->CurTabPos = Info.CurTabPos;
					esp->TopScreenLine = Info.TopScreenLine;
					esp->LeftPos = Info.LeftPos;
					esp->Overtype = Info.Overtype;
					return TRUE;
				}
				else {
					EditorSetPosition *esp = (EditorSetPosition *)Param2;
					DlgEdit *EditPtr = CurItem.GetEdit();
					esp->CurLine = 0;
					esp->CurPos = EditPtr->GetCurPos();
					esp->CurTabPos = EditPtr->GetCellCurPos();
					esp->TopScreenLine = 0;
					esp->LeftPos = EditPtr->GetLeftPos();
					esp->Overtype = EditPtr->GetOvertypeMode();
					return TRUE;
				}
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_SETEDITPOSITION: {
			if (Param2 && FarIsEdit(Type)) {
				int Result = TRUE;
				if (Type == DI_MEMOEDIT) {
					auto EditPtr = CurItem.GetEdit();
					Result = EditPtr->GetMemoEdit()->EditorControl(ECTL_SETPOSITION, (void*)Param2);
				}
				else {
					EditorSetPosition *esp = (EditorSetPosition *)Param2;
					auto EditPtr = CurItem.GetEdit();
					if (esp->CurPos >= 0)     EditPtr->SetCurPos(esp->CurPos);
					if (esp->CurTabPos >= 0)  EditPtr->SetCellCurPos(esp->CurTabPos);
					if (esp->LeftPos >= 0)    EditPtr->SetLeftPos(esp->LeftPos);
					if (esp->Overtype >= 0)   EditPtr->SetOvertypeMode(esp->Overtype);
				}
				ShowDialog(Param1);
				ScrBuf.Flush();
				return Result;
			}

			return FALSE;
		}
		/*****************************************************************/
		// Param2=0
		// Return MAKELONG(Visible,Size)
		case DM_GETCURSORSIZE: {
			if (FarIsEdit(Type) && CurItem.GetEdit()) {
				bool Visible;
				DWORD Size;
				CurItem.GetEdit()->GetCursorType(Visible, Size);
				return MAKELONG(Visible, Size);
			} else if (Type == DI_USERCONTROL && CurItem.UCData) {
				return MAKELONG(CurItem.UCData->CursorVisible, CurItem.UCData->CursorSize);
			}

			return FALSE;
		}
		/*****************************************************************/
		// Param2=MAKELONG(Visible,Size)
		// Return MAKELONG(OldVisible,OldSize)
		case DM_SETCURSORSIZE: {
			bool Visible = false;
			DWORD Size = 0;

			if (FarIsEdit(Type) && CurItem.GetEdit()) {
				CurItem.GetEdit()->GetCursorType(Visible, Size);
				CurItem.GetEdit()->SetCursorType(LOWORD(Param2) != 0, HIWORD(Param2));
			} else if (Type == DI_USERCONTROL && CurItem.UCData) {
				Visible = CurItem.UCData->CursorVisible;
				Size = CurItem.UCData->CursorSize;
				CurItem.UCData->CursorVisible = LOWORD(Param2) != 0;
				CurItem.UCData->CursorSize = HIWORD(Param2);
				int CCX = CurItem.UCData->CursorPos.X;
				int CCY = CurItem.UCData->CursorPos.Y;

				if (DialogMode.Check(DMODE_SHOW) && FocusPos == Param1 && CCX != -1 && CCY != -1)
					SetCursorType(CurItem.UCData->CursorVisible, CurItem.UCData->CursorSize);
			}

			return MAKELONG(Visible, Size);
		}
		/*****************************************************************/
		case DN_LISTCHANGE: {
			return CallDlgProc(Msg, Param1, Param2);
		}
		/*****************************************************************/
		case DN_EDITCHANGE: {
			FarDialogItem Item;

			if (!CurItem.ConvertToPlugin(&Item, false))
				return FALSE;	// no memory TODO: may be needed diagnostic

			INT_PTR I = 0;
			if (CurItem.Type == DI_EDIT || CurItem.Type == DI_COMBOBOX || CurItem.Type == DI_FIXEDIT
					|| CurItem.Type == DI_PSWEDIT) {
				CurItem.GetEdit()->SetCallbackState(false);
				const wchar_t *original_PtrData = Item.PtrData;
				I = CallDlgProc(DN_EDITCHANGE, Param1, (LONG_PTR)&Item);
				if (I) {
					if (Type == DI_COMBOBOX && CurItem.ListPtr)
						CurItem.ListPtr->ChangeFlags(VMENU_DISABLED, CurItem.Flags & DIF_DISABLE);
				}
				if (original_PtrData)
					free((void *)original_PtrData);
				CurItem.GetEdit()->SetCallbackState(true);
			}

			return I;
		}
		/*****************************************************************/
		case DN_BTNCLICK: {
			LONG_PTR Ret = CallDlgProc(Msg, Param1, Param2);

			if (Ret && (CurItem.Flags & DIF_AUTOMATION) && CurItem.AutoCount && CurItem.AutoPtr) {
				DialogItemAutomation *Auto = CurItem.AutoPtr;
				Param2 %= 3;

				for (UINT I = 0; I < CurItem.AutoCount; ++I, ++Auto) {
					DWORD NewFlags = Item[Auto->ID].Flags;
					Item[Auto->ID].Flags =
							(NewFlags & (~Auto->Flags[Param2][1])) | Auto->Flags[Param2][0];
					// здесь намеренно в обработчик не посылаются эвенты об изменении
					// состояния...
				}
			}

			return Ret;
		}
		/*****************************************************************/
		case DM_GETCHECK: {
			if (Type == DI_CHECKBOX || Type == DI_RADIOBUTTON)
				return CurItem.Selected;

			return 0;
		}
		/*****************************************************************/
		case DM_SET3STATE: {
			if (Type == DI_CHECKBOX) {
				int OldState = CurItem.Flags & DIF_3STATE ? TRUE : FALSE;

				if (Param2)
					CurItem.Flags|= DIF_3STATE;
				else
					CurItem.Flags&= ~DIF_3STATE;

				return OldState;
			}

			return 0;
		}
		/*****************************************************************/
		case DM_SETCHECK: {
			if (Type == DI_CHECKBOX) {
				int Selected = CurItem.Selected;

				if (Param2 == BSTATE_TOGGLE)
					Param2 = ++Selected;

				if (CurItem.Flags & DIF_3STATE)
					Param2 %= 3;
				else
					Param2 &= 1;

				CurItem.Selected = (int)Param2;

				if (Selected != (int)Param2 && DialogMode.Check(DMODE_SHOW)) {
					// автоматизация
					if ((CurItem.Flags & DIF_AUTOMATION) && CurItem.AutoCount && CurItem.AutoPtr) {
						DialogItemAutomation *Auto = CurItem.AutoPtr;
						Param2 %= 3;

						for (UINT I = 0; I < CurItem.AutoCount; ++I, ++Auto) {
							DWORD NewFlags = Item[Auto->ID].Flags;
							Item[Auto->ID].Flags =
									(NewFlags & (~Auto->Flags[Param2][1])) | Auto->Flags[Param2][0];
							// здесь намеренно в обработчик не посылаются эвенты об изменении
							// состояния...
						}

						Param1 = -1;
					}

					ShowDialog(Param1);
					ScrBuf.Flush();
				}

				return Selected;
			} else if (Type == DI_RADIOBUTTON) {
				Param1 = ProcessRadioButton(Param1);

				if (DialogMode.Check(DMODE_SHOW)) {
					ShowDialog();
					ScrBuf.Flush();
				}

				return Param1;
			}

			return 0;
		}
		/*****************************************************************/
		case DN_DRAWDLGITEM: {
			FarDialogItem Item;

			if (!CurItem.ConvertToPlugin(&Item, false))
				return FALSE;	// no memory TODO: may be needed diagnostic

			INT_PTR I = CallDlgProc(Msg, Param1, (LONG_PTR)&Item);

			if ((Type == DI_LISTBOX || Type == DI_COMBOBOX) && CurItem.ListPtr)
				CurItem.ListPtr->ChangeFlags(VMENU_DISABLED, CurItem.Flags & DIF_DISABLE);

			if (Item.PtrData)
				free((wchar_t *)Item.PtrData);

			return I;
		}
		/*****************************************************************/
		case DM_SETFOCUS: {
			if (!CurItem.IsFocusable())
				return FALSE;

			if (FocusPos == Param1)	// уже и так установлено все!
				return TRUE;

			ChangeFocus2(Param1);

			if (FocusPos == Param1) {
				ShowDialog();
				return TRUE;
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_GETFOCUS:		// Получить ID фокуса
		{
			return FocusPos;
		}
		/*****************************************************************/
		case DM_GETCONSTTEXTPTR: {
			return (LONG_PTR)Ptr;
		}
		/*****************************************************************/
		case DM_GETTEXTPTR:

			if (Param2) {
				FarDialogItemData IData = {0, (wchar_t *)Param2};
				return SendDlgMessage(DM_GETTEXT, Param1, (LONG_PTR)&IData);
			}
			[[fallthrough]];

			/*****************************************************************/
		case DM_GETTEXT:

			if (Param2)		// если здесь nullptr, то это еще один способ получить размер
			{
				FarDialogItemData *did = (FarDialogItemData *)Param2;
				Len = 0;

				switch (Type) {
					case DI_MEMOEDIT:
						if (CurItem.GetEdit()) {
							FARString strData;
							CurItem.GetEdit()->GetString(strData);
							Ptr = strData.CPtr();
							Len = strData.GetLength();
							if (!did->PtrLength)
								did->PtrLength = Len;
							else if (Len > did->PtrLength)
								Len = did->PtrLength;
							if (did->PtrData) {
								wmemmove(did->PtrData, Ptr, Len);
								did->PtrData[Len] = 0;
							}
						}
						return Len;

					case DI_COMBOBOX:
					case DI_EDIT:
					case DI_PSWEDIT:
					case DI_FIXEDIT:
						if (!CurItem.GetEdit())
							break;

						Ptr = const_cast<const wchar_t *>(CurItem.GetEdit()->GetStringAddr());
						[[fallthrough]];

					case DI_TEXT:
					case DI_VTEXT:
					case DI_SINGLEBOX:
					case DI_DOUBLEBOX:
					case DI_CHECKBOX:
					case DI_RADIOBUTTON:
					case DI_BUTTON:
						Len = StrLength(Ptr) + 1;

						if (Type == DI_BUTTON) {
							if (!(CurItem.Flags & DIF_NOBRACKETS)) {
								Ptr+= 2;
								Len-= 4;
							}
							if (CurItem.Flags & DIF_SETSHIELD) {
								Ptr+= 2;
							}
						}

						if (!did->PtrLength)
							did->PtrLength = Len;
						else if (Len > did->PtrLength)
							Len = did->PtrLength + 1;	// Прибавим 1, чтобы учесть нулевой байт.

						if (Len > 0 && did->PtrData) {
							wmemmove(did->PtrData, Ptr, Len);
							did->PtrData[Len - 1] = 0;
						}

						break;
					case DI_USERCONTROL:
						/*did->PtrLength=CurItem.Ptr.PtrLength; BUGBUG
						did->PtrData=(char*)CurItem.Ptr.PtrData;*/
						break;
					case DI_LISTBOX: {
						//						if(!CurItem.ListPtr)
						//							break;
						//						did->PtrLength=CurItem.ListPtr->GetUserData(did->PtrData,did->PtrLength,-1);
						break;
					}
					default:	// подразумеваем, что остались
						did->PtrLength = 0;
						break;
				}

				return Len - (!Len ? 0 : 1);
			}
			[[fallthrough]];

			// здесь умышленно не ставим return, т.к. хотим получить размер
			// следовательно сразу должен идти "case DM_GETTEXTLENGTH"!!!
			/*****************************************************************/
		case DM_GETTEXTLENGTH: {
			switch (Type) {
				case DI_BUTTON:
					Len = StrLength(Ptr) + 1;

					if (!(CurItem.Flags & DIF_NOBRACKETS))
						Len-= 4;

					break;
				case DI_USERCONTROL:
					// Len=CurItem.Ptr.PtrLength; BUGBUG
					break;
				case DI_TEXT:
				case DI_VTEXT:
				case DI_SINGLEBOX:
				case DI_DOUBLEBOX:
				case DI_CHECKBOX:
				case DI_RADIOBUTTON:
					Len = StrLength(Ptr) + 1;
					break;
				case DI_COMBOBOX:
				case DI_EDIT:
				case DI_PSWEDIT:
				case DI_FIXEDIT:
				case DI_MEMOEDIT:

					if (CurItem.GetEdit()) {
						Len = CurItem.GetEdit()->GetLength() + 1;
						break;
					}
					[[fallthrough]];

				case DI_LISTBOX: {
					Len = 0;
					MenuItemEx *ListMenuItem;

					if ((ListMenuItem = CurItem.ListPtr->GetItemPtr(-1))) {
						Len = (int)ListMenuItem->strName.GetLength() + 1;
					}

					break;
				}
				default:
					Len = 0;
					break;
			}

			return Len - (!Len ? 0 : 1);
		}
		/*****************************************************************/
		case DM_SETTEXTPTR: {
			if (!Param2)
				return 0;

			FarDialogItemData IData = {(size_t)StrLength((wchar_t *)Param2), (wchar_t *)Param2};
			return SendDlgMessage(DM_SETTEXT, Param1, (LONG_PTR)&IData);
		}

		case DM_SETTEXTPTRSILENT: {
			if (!Param2)
				return 0;

			if (CurItem.Type != DI_FIXEDIT && CurItem.Type != DI_EDIT)
				return 0;

			CurItem.GetEdit()->SetCallbackState(false);
			FarDialogItemData IData = {(size_t)StrLength((wchar_t *)Param2), (wchar_t *)Param2};
			intptr_t rv = SendDlgMessage(DM_SETTEXT, Param1, (LONG_PTR)&IData);
			CurItem.GetEdit()->SetCallbackState(true);

			return rv;
		}

		/*****************************************************************/
		case DM_SETTEXT: {
			if (Param2) {
				bool NeedInit = true;
				FarDialogItemData *did = (FarDialogItemData *)Param2;

				switch (Type) {
					case DI_MEMOEDIT:
					case DI_COMBOBOX:
					case DI_EDIT:
					case DI_TEXT:
					case DI_VTEXT:
					case DI_SINGLEBOX:
					case DI_DOUBLEBOX:
					case DI_BUTTON:
					case DI_CHECKBOX:
					case DI_RADIOBUTTON:
					case DI_PSWEDIT:
					case DI_FIXEDIT:
					case DI_LISTBOX:	// меняет только текущий итем
						CurItem.strData = did->PtrData;
						Len = CurItem.strData.GetLength();
						break;
					default:
						Len = 0;
						break;
				}

				switch (Type) {
					case DI_USERCONTROL:
						/*CurItem.Ptr.PtrLength=did->PtrLength;
						CurItem.Ptr.PtrData=did->PtrData;
						return CurItem.Ptr.PtrLength;*/
						return 0;	// BUGBUG
					case DI_TEXT:
					case DI_VTEXT:
					case DI_SINGLEBOX:
					case DI_DOUBLEBOX:

						if (DialogMode.Check(DMODE_SHOW)) {
							if (!DialogMode.Check(DMODE_KEEPCONSOLETITLE))
								ConsoleTitle::SetFarTitle(GetDialogTitle());
							ShowDialog(Param1);
							ScrBuf.Flush();
						}

						return Len;
					case DI_BUTTON:
					case DI_CHECKBOX:
					case DI_RADIOBUTTON:
						break;
					case DI_MEMOEDIT:
						NeedInit = false;
						if (DlgEdit *EditLine = CurItem.GetEdit()) {
							bool ReadOnly = EditLine->GetReadOnly();
							EditLine->SetReadOnly(false);
							EditLine->SetString(CurItem.strData);
							EditLine->SetReadOnly(ReadOnly);
							if (DialogMode.Check(DMODE_INITOBJECTS))
								EditLine->SetClearFlag(false);
							EditLine->Select(-1, 0);
						}
						break;
					case DI_COMBOBOX:
					case DI_EDIT:
					case DI_PSWEDIT:
					case DI_FIXEDIT:
						NeedInit = false;

						if (DlgEdit *EditLine = CurItem.GetEdit()) {
							bool ReadOnly = EditLine->GetReadOnly();
							EditLine->SetReadOnly(false);
							EditLine->DisableAC();
							EditLine->SetString(CurItem.strData);
							EditLine->RevertAC();
							EditLine->SetReadOnly(ReadOnly);

							if (DialogMode.Check(DMODE_INITOBJECTS))	// не меняем клеар-флаг, пока не проиницализировались
								EditLine->SetClearFlag(false);

							EditLine->Select(-1, 0);	// снимаем выделение
														// ...оно уже снимается в DlgEdit::SetString()
						}

						break;
					case DI_LISTBOX:	// меняет только текущий итем
					{
						VMenu *ListBox = CurItem.ListPtr;

						if (ListBox) {
							FarListUpdate LUpdate;
							LUpdate.Index = ListBox->GetSelectPos();
							MenuItemEx *ListMenuItem = ListBox->GetItemPtr(LUpdate.Index);

							if (ListMenuItem) {
								LUpdate.Item.Flags = ListMenuItem->Flags;
								LUpdate.Item.Text = Ptr;
								SendDlgMessage(DM_LISTUPDATE, Param1, (LONG_PTR)&LUpdate);
							}

							break;
						} else
							return 0;
					}
					default:	// подразумеваем, что остались
						return 0;
				}

				if (NeedInit)
					InitDialogObjects(Param1);			// переинициализируем элементы диалога

				if (DialogMode.Check(DMODE_SHOW))		// достаточно ли этого????!!!!
				{
					ShowDialog(Param1);
					ScrBuf.Flush();
				}

				// CurItem.strData = did->PtrData;
				return CurItem.strData.GetLength();	//???
			}

			return 0;
		}
		/*****************************************************************/
		case DM_SETMAXTEXTLENGTH: {
			if ((Type == DI_EDIT || Type == DI_PSWEDIT
						|| (Type == DI_COMBOBOX && !(CurItem.Flags & DIF_DROPDOWNLIST)))
					&& CurItem.GetEdit()) {
				int MaxLen = CurItem.GetEdit()->GetMaxLength();
				// BugZ#628 - Неправильная длина редактируемого текста.
				CurItem.GetEdit()->SetMaxLength((int)Param2);
				// if (DialogMode.Check(DMODE_INITOBJECTS)) //???
				InitDialogObjects(Param1);		// переинициализируем элементы диалога
				if (!DialogMode.Check(DMODE_KEEPCONSOLETITLE))
					ConsoleTitle::SetFarTitle(GetDialogTitle());
				return MaxLen;
			}

			return 0;
		}
		/*****************************************************************/
		case DM_GETDLGITEM: {
			FarDialogItem *Item = (FarDialogItem *)Param2;
			return (LONG_PTR)CurItem.ConvertItemEx2(Item);
		}
		/*****************************************************************/
		case DM_GETDLGITEMSHORT: {
			return Param2 && CurItem.ConvertToPlugin((FarDialogItem *)Param2, true);
		}
		/*****************************************************************/
		case DM_SETDLGITEM:
		case DM_SETDLGITEMSHORT: {
			if (!Param2)
				return FALSE;

			if (Type != ((FarDialogItem *)Param2)->Type)	// пока нефига менять тип
				return FALSE;

			// не менять
			if (!CurItem.ConvertFromPlugin((FarDialogItem *)Param2, Msg == DM_SETDLGITEMSHORT))
				return FALSE;	// invalid parameters

			CurItem.Type = Type;

			if ((Type == DI_LISTBOX || Type == DI_COMBOBOX) && CurItem.ListPtr)
				CurItem.ListPtr->ChangeFlags(VMENU_DISABLED, CurItem.Flags & DIF_DISABLE);

			// еще разок, т.к. данные могли быть изменены
			InitDialogObjects(Param1);
			if (!DialogMode.Check(DMODE_KEEPCONSOLETITLE))
				ConsoleTitle::SetFarTitle(GetDialogTitle());

			if (DialogMode.Check(DMODE_SHOW)) {
				ShowDialog(Param1);
				ScrBuf.Flush();
			}

			return TRUE;
		}
		/*****************************************************************/
		/*
			$ 03.01.2001 SVS
			+ показать/скрыть элемент
			Param2:
				-1 - получить состояние
				0  - погасить
				1  - показать
			Return:  предыдущее состояние
		*/
		case DM_SHOWITEM: {
			DWORD PrevFlags = CurItem.Flags;

			if (Param2 != -1) {
				if (Param2)
					CurItem.Flags&= ~DIF_HIDDEN;
				else
					CurItem.Flags|= DIF_HIDDEN;

				if (DialogMode.Check(DMODE_SHOW))		// && (PrevFlags&DIF_HIDDEN) != (CurItem.Flags&DIF_HIDDEN))//!(CurItem.Flags&DIF_HIDDEN))
				{
					if ((CurItem.Flags & DIF_HIDDEN) && FocusPos == Param1) {
						Param2 = ChangeFocus(Param1, 1, true);
						ChangeFocus2((int)Param2);
					}

					// Либо все, либо... только 1
					ShowDialog(GetDropDownOpened() || (CurItem.Flags & DIF_HIDDEN) ? -1 : Param1);
					ScrBuf.Flush();
				}
			}

			return !(PrevFlags & DIF_HIDDEN);
		}
		/*****************************************************************/
		case DM_SETDROPDOWNOPENED:		// Param1=ID; Param2={TRUE|FALSE}
		{
			if (!Param2)				// Закрываем любой открытый комбобокс или историю
			{
				if (GetDropDownOpened()) {
					SetDropDownOpened(false);
					WINPORT(Sleep)(10);
				}

				return TRUE;
			}
			/*
				$ 09.12.2001 DJ
				у DI_PSWEDIT не бывает хистори!
			*/
			if (Type == DI_COMBOBOX
					|| ((Type == DI_EDIT || Type == DI_FIXEDIT) && (CurItem.Flags & DIF_HISTORY)))	/* DJ $ */
			{
				// Открываем заданный в Param1 комбобокс или историю
				if (GetDropDownOpened()) {
					SetDropDownOpened(false);
					WINPORT(Sleep)(10);
				}

				if (SendDlgMessage(DM_SETFOCUS, Param1, 0)) {
					ProcessOpenComboBox(Type, CurItem, Param1);	//?? Param1 ??
					// ProcessKey(KEY_CTRLDOWN);
					return TRUE;
				} else
					return FALSE;
			}

			return FALSE;
		}
		/*****************************************************************/
		case DM_SETITEMPOSITION:	// Param1 = ID; Param2 = SMALL_RECT
		{
			return SetItemRect((int)Param1, (SMALL_RECT *)Param2);
		}
		/*****************************************************************/
		case DM_ENABLE: {
			DWORD PrevFlags = CurItem.Flags;

			if (Param2 != -1) {
				if (Param2)
					CurItem.Flags&= ~DIF_DISABLE;
				else
					CurItem.Flags|= DIF_DISABLE;

				if ((Type == DI_LISTBOX || Type == DI_COMBOBOX) && CurItem.ListPtr)
					CurItem.ListPtr->ChangeFlags(VMENU_DISABLED, CurItem.Flags & DIF_DISABLE);
			}

			if (DialogMode.Check(DMODE_SHOW))		//???
			{
				ShowDialog(Param1);
				ScrBuf.Flush();
			}

			return !(PrevFlags & DIF_DISABLE);
		}

//		case DM_GETCOLOR: {
//			*(DWORD *)Param2 = (DWORD)CtlColorDlgItem(Param1, CurItem);
//			*(DWORD *)Param2|= (CurItem.Flags & DIF_SETCOLOR);
//			return TRUE;
//		}

//		case DM_SETCOLOR: {
//			CurItem.Flags&= ~(DIF_SETCOLOR | DIF_COLORMASK);
//			CurItem.Flags|= Param2 & (DIF_SETCOLOR | DIF_COLORMASK);
//			if (DialogMode.Check(DMODE_SHOW)) {		//???
//				ShowDialog(Param1);
//				ScrBuf.Flush();
//			}
//			return TRUE;
//		}

		case DM_GETDEFAULTCOLOR: {
			if (Param2)
				CtlColorDlgItem(Param1, (uint64_t *)Param2);

			return TRUE;
		}

		///case DM_GETCOLOR:///
		case DM_GETTRUECOLOR: {
			if (Param2)
				memcpy((void*)Param2, CurItem.customItemColor, sizeof(CurItem.customItemColor));
//			if (!CurItem.TrueColors) {
//				memset((uint64_t *)Param2, 0, sizeof(DialogItemTrueColors));
//			} else {
//				*(DialogItemTrueColors *)Param2 = *CurItem.TrueColors;
//			}
//			Param2 = CurItem.customItemColor

			return TRUE;
		}

		///case DM_SETCOLOR:///
		case DM_SETTRUECOLOR: {
			if (Param2)
				memcpy(CurItem.customItemColor, (void*)Param2, sizeof(CurItem.customItemColor));

//			if (!CurItem.TrueColors) {
//				CurItem.TrueColors.reset(new DialogItemTrueColors);
//			}
//			*CurItem.TrueColors = *(const DialogItemTrueColors *)Param2;

			if (InCtlColorDlgItem == 0 && DialogMode.Check(DMODE_SHOW)) {		//???
				ShowDialog(Param1);
				ScrBuf.Flush();
			}
			return TRUE;
		}

		case DM_SETREADONLY: {
			if (Param2) {
				CurItem.Flags|= DIF_READONLY;
			} else {
				CurItem.Flags&= ~DIF_READONLY;
			}
			if (FarIsEdit(Type)) {
				DlgEdit *CurItemEdit = CurItem.GetEdit();
				if (CurItemEdit) {
					CurItemEdit->SetReadOnly(Param2 != 0);
				}
			} else {
				fprintf(stderr,
					"%s: DM_SETREADONLY invoked for non-edit item %d\n", __FUNCTION__, Param1);
			}
			if (DialogMode.Check(DMODE_SHOW)) {		//???
				ShowDialog(Param1);
				ScrBuf.Flush();
			}
			return TRUE;
		}

		/*****************************************************************/
		// получить позицию и размеры контрола
		case DM_GETITEMPOSITION:	// Param1=ID, Param2=*SMALL_RECT

			if (Param2) {
				SMALL_RECT Rect;
				if (GetItemRect(Param1, Rect)) {
					*reinterpret_cast<PSMALL_RECT>(Param2) = Rect;
					return TRUE;
				}
			}

			return FALSE;
			/*****************************************************************/
		case DM_SETITEMDATA: {
			LONG_PTR PrewDataDialog = CurItem.UserData;
			CurItem.UserData = Param2;
			return PrewDataDialog;
		}
		/*****************************************************************/
		case DM_GETITEMDATA: {
			return CurItem.UserData;
		}
		/*****************************************************************/
		case DM_EDITUNCHANGEDFLAG:		// -1 Get, 0 - Skip, 1 - Set; Выделение блока снимается.
		{
			if (FarIsEdit(Type)) {
				DlgEdit *EditLine = CurItem.GetEdit();
				bool ClearFlag = EditLine->GetClearFlag();

				if (Param2 >= 0) {
					EditLine->SetClearFlag(Param2 != 0);
					EditLine->Select(-1, 0);					// снимаем выделение

					if (DialogMode.Check(DMODE_SHOW))		//???
					{
						ShowDialog(Param1);
						ScrBuf.Flush();
					}
				}

				return ClearFlag;
			}

			break;
		}
		/*****************************************************************/
		case DM_GETSELECTION:		// Msg=DM_GETSELECTION, Param1=ID, Param2=*EditorSelect
		case DM_SETSELECTION:		// Msg=DM_SETSELECTION, Param1=ID, Param2=*EditorSelect
		{
			if (FarIsEdit(Type) && Param2) {
				if (Msg == DM_GETSELECTION) {
					EditorSelect *EdSel = (EditorSelect *)Param2;
					DlgEdit *EditLine = CurItem.GetEdit();
					EdSel->BlockStartLine = 0;
					EdSel->BlockHeight = 1;
					EditLine->GetSelection(EdSel->BlockStartPos, EdSel->BlockWidth);

					if (EdSel->BlockStartPos == -1 && !EdSel->BlockWidth)
						EdSel->BlockType = BTYPE_NONE;
					else {
						EdSel->BlockType = BTYPE_STREAM;
						EdSel->BlockWidth-= EdSel->BlockStartPos;
					}

					return TRUE;
				} else {
					EditorSelect *EdSel = (EditorSelect *)Param2;
					DlgEdit *EditLine = CurItem.GetEdit();

					if (EdSel->BlockType == BTYPE_NONE)
						EditLine->Select(-1, 0);
					else
						EditLine->Select(EdSel->BlockStartPos, EdSel->BlockStartPos + EdSel->BlockWidth);

					if (DialogMode.Check(DMODE_SHOW))		//???
					{
						ShowDialog(Param1);
						ScrBuf.Flush();
					}

					return TRUE;
				}
			}

			break;
		}

		case DM_GETMEMOEDITID:
			if (Type == DI_MEMOEDIT && Param2) {
				auto dialogEdit = CurItem.GetEdit();
				*reinterpret_cast<int*>(Param2) = dialogEdit->GetMemoEdit()->GetEditorID();
				return TRUE;
			}
			return FALSE;

	}

	// Все, что сами не отрабатываем - посылаем на обработку обработчику.
	return CallDlgProc(Msg, Param1, Param2);
}

LONG_PTR Dialog::SendDlgMessage(int Msg, int Param1, LONG_PTR Param2)
{
	return ::SendDlgMessage(this, Msg, Param1, Param2);
}

LONG_PTR SendDlgMessageSynched(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	return reinterpret_cast<Dialog*>(hDlg)->SendDlgMessageSynched(Msg, Param1, Param2);
}

LONG_PTR WINAPI SendDlgMessage(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (!hDlg)
		return 0;

	return InterThreadCall<LONG_PTR, 0>(std::bind(SendDlgMessageSynched, hDlg, Msg, Param1, Param2));
}

void Dialog::SetPosition(int x1, int y1, int x2, int y2)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);

	if (x1 >= 0)
		RealWidth = x2 - x1 + 1;
	else
		RealWidth = x2;

	if (y1 >= 0)
		RealHeight = y2 - y1 + 1;
	else
		RealHeight = y2;

	ScreenObject::SetPosition(x1, y1, x2, y2);
}
//////////////////////////////////////////////////////////////////////////
bool Dialog::IsInited()
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	return DialogMode.Check(DMODE_INITOBJECTS);
}

void Dialog::SetComboBoxPos(DialogItemEx *CurItem)
{
	if (GetDropDownOpened()) {
		if (!CurItem) {
			CurItem = &Item[FocusPos];
		}
		int EditX1, EditY1, EditX2, EditY2;
		CurItem->GetEdit()->GetPosition(EditX1, EditY1, EditX2, EditY2);

		EditX2 = Max(EditX2, EditX1 + 20);

		if (ScrY - EditY1 < Min(Opt.Dialogs.CBoxMaxHeight, CurItem->ListPtr->GetItemCount()) + 2
				&& EditY1 > ScrY / 2)
			CurItem->ListPtr->SetPosition(EditX1,
					Max(0, EditY1 - 1 - Min(Opt.Dialogs.CBoxMaxHeight, CurItem->ListPtr->GetItemCount()) - 1),
					EditX2, EditY1 - 1);
		else
			CurItem->ListPtr->SetPosition(EditX1, EditY1 + 1, EditX2, 0);
	}
}

bool Dialog::ProcessEvents() const
{
	return !DialogMode.Check(DMODE_ENDLOOP);
}

void Dialog::SetId(const GUID &Guid)
{
	Id = Guid;
	IdExist = true;
}

Editor* Dialog::GetMemoEdit(int Pos) const
{
	Pos = (Pos < 0) ? FocusPos : Pos;
	if (Pos >= 0 && Pos < ItemCount() && Item[Pos].Type == DI_MEMOEDIT) {
		return Item[Pos].GetEdit()->GetMemoEdit();
	}
	return nullptr;
}
