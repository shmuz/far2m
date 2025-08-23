/*
edit.cpp

Реализация одиночной строки редактирования
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

#include "edit.hpp"
#include "keyboard.hpp"
#include "macroopcode.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "editor.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "panel.hpp"
#include "scrbuf.hpp"
#include "interf.hpp"
#include "farcolors.hpp"
#include "clipboard.hpp"
#include "xlat.hpp"
#include "datetime.hpp"
#include "Bookmarks.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "RegExp.hpp"
#include "history.hpp"
#include "vmenu.hpp"
#include "chgmmode.hpp"
#include <cwctype>

static int Recurse = 0;

enum
{
	EOL_NONE,
	EOL_CR,
	EOL_LF,
	EOL_CRLF,
	EOL_CRCRLF
};
static const wchar_t *EOL_TYPE_CHARS[] = {L"", L"\r", L"\n", L"\r\n", L"\r\r\n"};

#define EDMASK_ANY    L'X'    // позволяет вводить в строку ввода любой символ;
#define EDMASK_DSS    L'#'    // позволяет вводить в строку ввода цифры, пробел и знак минуса;
#define EDMASK_DIGIT  L'9'    // позволяет вводить в строку ввода только цифры;
#define EDMASK_DIGITS L'N'    // позволяет вводить в строку ввода только цифры и пробелы;
#define EDMASK_ALPHA  L'A'    // позволяет вводить в строку ввода только буквы.
#define EDMASK_HEX    L'H'    // позволяет вводить в строку ввода шестнадцатиричные символы.

class DisableCallback
{
	bool OldState;
	bool *CurState;

public:
	DisableCallback(bool &State)
	{
		OldState = State;
		CurState = &State;
		State = false;
	}
	void Restore() { *CurState = OldState; }
	~DisableCallback() { Restore(); }
};

Edit::Edit(ScreenObject *pOwner, Callback *aCallback)
	:
	m_next(nullptr),
	m_prev(nullptr),
	MaxLength(-1),
	Mask(nullptr),
	LeftPos(0),
	CurPos(0),
	PrevCurPos(0),
	MSelStart(-1),
	m_SelStart(-1),
	m_SelEnd(0),
	CursorSize(-1),
	CursorPos(0)
{
	m_Callback.Active = true;
	m_Callback.m_Callback = nullptr;
	m_Callback.m_Param = nullptr;

	if (aCallback)
		m_Callback = *aCallback;

	SetOwner(pOwner);
	SetWordDiv(Opt.strWordDiv);

	Flags.Set(FEDITLINE_EDITBEYONDEND);
	Color = F_LIGHTGRAY | B_BLACK;
	SelColor = F_WHITE | B_BLACK;
	ColorUnChanged = FarColorToReal(COL_DIALOGEDITUNCHANGED);
	EndType = EOL_NONE;
	TabSize = Opt.EdOpt.TabSize;
	TabExpandMode = EXPAND_NOTABS;
	Flags.Change(FEDITLINE_DELREMOVESBLOCKS, Opt.EdOpt.DelRemovesBlocks);
	Flags.Change(FEDITLINE_PERSISTENTBLOCKS, Opt.EdOpt.PersistentBlocks);
	Flags.Change(FEDITLINE_SHOWWHITESPACE, Opt.EdOpt.ShowWhiteSpace);
	m_codepage = 0;    // BUGBUG
}

Edit::~Edit()
{
	if (Mask)
		free(Mask);
}

DWORD Edit::SetCodePage(UINT codepage)
{
	DWORD Ret = SETCP_NOERROR;
	DWORD wc2mbFlags = WC_NO_BEST_FIT_CHARS;
	BOOL UsedDefaultChar = FALSE;
	LPBOOL lpUsedDefaultChar = &UsedDefaultChar;

	if (m_codepage == CP_UTF7 || m_codepage == CP_UTF8 || m_codepage == CP_UTF16LE
			|| m_codepage == CP_UTF16BE)    // BUGBUG: CP_SYMBOL, 50xxx, 57xxx too
	{
		wc2mbFlags = 0;
		lpUsedDefaultChar = nullptr;
	}

	DWORD mb2wcFlags = MB_ERR_INVALID_CHARS;

	if (codepage == CP_UTF7)    // BUGBUG: CP_SYMBOL, 50xxx, 57xxx too
	{
		mb2wcFlags = 0;
	}

	if (codepage != m_codepage) {
		if (!m_Str.IsEmpty()) {
			// m_codepage = codepage;
			int length = WINPORT(WideCharToMultiByte)(m_codepage, wc2mbFlags, m_Str, StrSize(), nullptr, 0,
					nullptr, lpUsedDefaultChar);

			if (UsedDefaultChar)
				Ret|= SETCP_WC2MBERROR;

			char *decoded = (char *)malloc(length);

			if (!decoded) {
				Ret|= SETCP_OTHERERROR;
				return Ret;
			}

			WINPORT(WideCharToMultiByte)(m_codepage, 0, m_Str, StrSize(), decoded, length, nullptr, nullptr);
			int length2 = WINPORT(MultiByteToWideChar)(codepage, mb2wcFlags, decoded, length, nullptr, 0);

			if (!length2 && WINPORT(GetLastError)() == ERROR_NO_UNICODE_TRANSLATION) {
				Ret|= SETCP_MB2WCERROR;
				length2 = WINPORT(MultiByteToWideChar)(codepage, 0, decoded, length, nullptr, 0);
			}

			wchar_t *encoded = (wchar_t *)malloc((length2 + 1) * sizeof(wchar_t));

			if (!encoded) {
				free(decoded);
				Ret|= SETCP_OTHERERROR;
				return Ret;
			}

			length2 = WINPORT(MultiByteToWideChar)(codepage, 0, decoded, length, encoded, length2);
			encoded[length2] = L'\0';
			free(decoded);
			m_Str = encoded;
			m_Str.Truncate(length2);
			HasSpecialWidthChars = false;
			CheckForSpecialWidthChars();
		}

		m_codepage = codepage;
		Changed();
	}

	return Ret;
}

UINT Edit::GetCodePage()
{
	return m_codepage;
}

void Edit::DisplayObject()
{
	if (Flags.Check(FEDITLINE_DROPDOWNBOX)) {
		Flags.Clear(FEDITLINE_CLEARFLAG);    // при дроп-даун нам не нужно никакого unchanged text
		m_SelStart = 0;
		m_SelEnd = StrSize();                // а также считаем что все выделено -
											 //    надо же отличаться от обычных Edit
	}

	//   Вычисление нового положения курсора в строке с учётом Mask.
	int Value = (PrevCurPos > CurPos) ? -1 : 1;
	CurPos = GetNextCursorPos(CurPos, Value);
	FastShow();

	/* $ 26.07.2000 tran
	   при DropDownBox курсор выключаем
	   не знаю даже - попробовал но не очень красиво вышло */
	if (Flags.Check(FEDITLINE_DROPDOWNBOX))
		::SetCursorType(false, 10);
	else {
		if (Flags.Check(FEDITLINE_OVERTYPE)) {
			int NewCursorSize = (Opt.CursorSize[2] ? Opt.CursorSize[2] : 99);
			::SetCursorType(true, CursorSize == -1 ? NewCursorSize : CursorSize);
		} else {
			int NewCursorSize = (Opt.CursorSize[0] ? Opt.CursorSize[0] : 10);
			::SetCursorType(true, CursorSize == -1 ? NewCursorSize : CursorSize);
		}
	}

	MoveCursor(X1 + CursorPos - LeftPos, Y1);
}

void Edit::SetCursorType(bool Visible, DWORD Size)
{
	Flags.Change(FEDITLINE_CURSORVISIBLE, Visible);
	CursorSize = Size;
	::SetCursorType(Visible, Size);
}

void Edit::GetCursorType(bool &Visible, DWORD &Size)
{
	Visible = Flags.Check(FEDITLINE_CURSORVISIBLE);
	Size = CursorSize;
}

//   Вычисление нового положения курсора в строке с учётом Mask.
int Edit::GetNextCursorPos(int Position, int Where)
{
	int Result = Position;

	if (Mask && *Mask && (Where == -1 || Where == 1)) {
		bool PosChanged = false;
		int MaskLen = StrLength(Mask);

		for (int i = Position; i < MaskLen && i >= 0; i+= Where) {
			if (CheckCharMask(Mask[i])) {
				Result = i;
				PosChanged = true;
				break;
			}
		}

		if (!PosChanged) {
			for (int i = Position; i >= 0; i--) {
				if (CheckCharMask(Mask[i])) {
					Result = i;
					PosChanged = true;
					break;
				}
			}
		}

		if (!PosChanged) {
			for (int i = Position; i < MaskLen; i++) {
				if (CheckCharMask(Mask[i])) {
					Result = i;
					break;
				}
			}
		}
	}

	return Result;
}

void Edit::FastShow()
{
	int EditLength = ObjWidth;

	if (!Flags.Check(FEDITLINE_EDITBEYONDEND) && CurPos > StrSize())
		CurPos = StrSize();

	if (MaxLength != -1) {
		if (StrSize() > MaxLength) {
			m_Str.Truncate(MaxLength);
		}

		if (CurPos > MaxLength - 1)
			CurPos = MaxLength > 0 ? (MaxLength - 1) : 0;
	}

	int CellCurPos = GetCellCurPos();

	/* $ 31.07.2001 KM
	  ! Для комбобокса сделаем отображение строки
		с первой позиции.
	*/
	int RealLeftPos = -1;
	if (!Flags.Check(FEDITLINE_DROPDOWNBOX)) {
		if (CellCurPos - LeftPos > EditLength - 1) {
			// tricky left pos shifting to
			// - avoid LeftPos pointing into middle of full-width char cells pair
			// - ensure RealLeftPos really shifted in case string starts by some long character
			for (int ShiftBy = 1; ShiftBy <= std::max(TabSize, 2); ++ShiftBy) {
				RealLeftPos = CellPosToReal(CellCurPos - EditLength + ShiftBy);
				int NewLeftPos = RealPosToCell(RealLeftPos);
				if (LeftPos != NewLeftPos) {
					LeftPos = NewLeftPos;
					break;
				}
			}
		}

		if (CellCurPos < LeftPos)
			LeftPos = CellCurPos;
	}

	if (RealLeftPos == -1)
		RealLeftPos = CellPosToReal(LeftPos);

	GotoXY(X1, Y1);
	int CellSelStart = (m_SelStart == -1) ? -1 : RealPosToCell(m_SelStart);
	int CellSelEnd = (m_SelEnd < 0) ? -1 : RealPosToCell(m_SelEnd);

	/* $ 17.08.2000 KM
	   Если есть маска, сделаем подготовку строки, то есть
	   все "постоянные" символы в маске, не являющиеся шаблонными
	   должны постоянно присутствовать в m_Str
	*/
	if (Mask && *Mask)
		RefreshStrByMask();

	CursorPos = CellCurPos;

	OutStr.clear();
	size_t OutStrCells = 0;
	for (int i = RealLeftPos; i < StrSize() && int(OutStrCells) < EditLength; ++i) {
		auto wc = m_Str[i];
		if (Flags.Check(FEDITLINE_SHOWWHITESPACE) && Flags.Check(FEDITLINE_EDITORMODE)) {
			switch(wc) {
				case 0x0020: //space
					wc = L'\xB7'; // ·
					break;
				case 0x00A0: //no-break space
					wc = L'\xB0'; // °
					break;
				case 0x00AD: //soft hyphen
					wc = L'\xAC'; // ¬
					break;
				case 0x2028: //line separator
					wc = L'\x2424'; // ␤
					break;
				case 0x2029: //paragraph separator
					wc = L'\xB6'; // ¶
					break;
				case 0x2000 ... 0x200A: //other spaces
				case 0x202F: case 0x205F:
				case 0x180E: case 0x3000:
					wc = L'\x2420'; // ␠
					break;
				case 0x200B ... 0x200D: //zero-width
				case 0x2060: case 0xFEFF:
					wc = L'\x2422'; // ␢
					break;
				case 0x200E ... 0x200F: //text direction marks and shaping controls
				case 0x202A ... 0x202E:
				case 0x2066 ... 0x206F:
					wc = L'\x2194'; // ↔
					break;
			}
		}

		if (wc == L'\t') {
			for (int j = 0, S = TabSize - ((LeftPos + OutStrCells) % TabSize);
					j < S && int(OutStrCells) < EditLength; ++j, ++OutStrCells) {
				OutStr.emplace_back(
						(Flags.Check(FEDITLINE_SHOWWHITESPACE) && Flags.Check(FEDITLINE_EDITORMODE) && !j)
								? L'\x2192'
								: L' ');
			}
		} else {
			if (CharClasses::IsFullWidth(wc)) {
				if (int(OutStrCells + 2) > EditLength) {
					OutStr.emplace_back(L' ');
					OutStrCells++;
					break;
				}
				OutStrCells+= 2;
			} else if (!CharClasses::IsXxxfix(wc))
				OutStrCells++;

			OutStr.emplace_back(wc ? wc : L' ');
		}
	}

	if (Flags.Check(FEDITLINE_PASSWORDMODE)) {
		OutStr.resize(OutStrCells);
		std::fill(OutStr.begin(), OutStr.end(), L'*');
	}

	OutStr.emplace_back(0);
	SetColor(Color);

	if (CellSelStart == -1) {
		if (Flags.Check(FEDITLINE_CLEARFLAG)) {
			SetColor(ColorUnChanged);

			if (Mask && *Mask) {
				RemoveTrailingSpaces(OutStr.data());
				OutStr.resize(wcslen(OutStr.data()));
				OutStrCells = StrCellsCount(OutStr.data(), OutStr.size());
				OutStr.emplace_back(0);
			}

			FS << fmt::Cells() << fmt::LeftAlign() << OutStr.data();
			SetColor(Color);
			int BlankLength = EditLength - (int)OutStrCells;

			if (BlankLength > 0) {
				FS << fmt::Cells() << fmt::Expand(BlankLength) << L"";
			}
		} else {
			FS << fmt::LeftAlign() << fmt::Cells() << fmt::Size(EditLength) << OutStr.data();
		}
	} else {
		if ((CellSelStart-= LeftPos) < 0)
			CellSelStart = 0;

		int AllString = (CellSelEnd == -1);

		if (AllString)
			CellSelEnd = EditLength;
		else if ((CellSelEnd-= LeftPos) < 0)
			CellSelEnd = 0;

		for (; int(OutStrCells) < EditLength; ++OutStrCells) {
			OutStr.emplace(OutStr.begin() + OutStr.size() - 1, L' ');
		}

		/* $ 24.08.2000 SVS
		   ! У DropDowList`а выделение по полной программе - на всю видимую длину
			 ДАЖЕ ЕСЛИ ПУСТАЯ СТРОКА
		*/
		if (CellSelStart >= EditLength /*|| !AllString && CellSelStart>=StrSize()*/
				|| CellSelEnd < CellSelStart) {
			if (Flags.Check(FEDITLINE_DROPDOWNBOX)) {
				SetColor(SelColor);
				FS << fmt::Cells() << fmt::Expand(X2 - X1 + 1) << OutStr.data();
			} else
				Text(OutStr.data());
		} else {
			FS << fmt::Cells() << fmt::Truncate(CellSelStart) << OutStr.data();
			SetColor(SelColor);

			if (!Flags.Check(FEDITLINE_DROPDOWNBOX)) {
				FS << fmt::Cells() << fmt::Skip(CellSelStart) << fmt::Truncate(CellSelEnd - CellSelStart)
				   << OutStr.data();

				if (CellSelEnd < EditLength) {
					// SetColor(Flags.Check(FEDITLINE_CLEARFLAG) ? SelColor:Color);
					SetColor(Color);
					FS << fmt::Cells() << fmt::Skip(CellSelEnd) << OutStr.data();
				}
			} else {
				FS << fmt::Cells() << fmt::Expand(X2 - X1 + 1) << OutStr.data();
			}
		}
	}

	/* $ 26.07.2000 tran
	   при дроп-даун цвета нам не нужны */
	if (!Flags.Check(FEDITLINE_DROPDOWNBOX))
		ApplyColor();
}

int Edit::RecurseProcessKey(FarKey Key)
{
	Recurse++;
	int RetCode = ProcessKey(Key);
	Recurse--;
	return RetCode;
}

// Функция вставки всякой хреновени - от шорткатов до имен файлов
bool Edit::ProcessInsPath(FarKey Key, int PrevSelStart, int PrevSelEnd)
{
	bool RetCode = false;
	FARString strPathName;

	if (Key >= KEY_RCTRL0 && Key <= KEY_RCTRL9)    // шорткаты?
	{
		FARString strPluginModule, strPluginFile, strPluginData;

		if (Bookmarks().Get(Key - KEY_RCTRL0, &strPathName, &strPluginModule, &strPluginFile, &strPluginData))
			RetCode = true;
	} else                                                 // Пути/имена?
	{
		RetCode = _MakePath1(Key, strPathName, L"", 0) != 0; // 0 - always not escaping path names
	}

	// Если что-нить получилось, именно его и вставим (PathName)
	if (RetCode) {
		if (Flags.Check(FEDITLINE_CLEARFLAG)) {
			LeftPos = 0;
			SetString(L"");
		}

		if (PrevSelStart != -1) {
			m_SelStart = PrevSelStart;
			m_SelEnd = PrevSelEnd;
		}

		if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
			DeleteBlock();

		InsertString(strPathName);
		Flags.Clear(FEDITLINE_CLEARFLAG);
	}

	return RetCode;
}

int64_t Edit::VMProcess(int OpCode, void *vParam, int64_t iParam)
{
	switch (OpCode) {
		case MCODE_C_EMPTY:
			return !GetLength();
		case MCODE_C_SELECTED:
			return m_SelStart != -1 && m_SelStart < m_SelEnd;
		case MCODE_C_EOF:
			return CurPos >= StrSize();
		case MCODE_C_BOF:
			return !CurPos;
		case MCODE_V_ITEMCOUNT:
			return StrSize();
		case MCODE_V_CURPOS:
			return CurPos + 1;
		case MCODE_F_EDITOR_SEL: {

			switch ((INT_PTR)vParam) {
				case 0:    // Get Param
				{
					switch (iParam) {
						case 0:    // return FirstLine
						case 2:    // return LastLine
							return IsSelection() ? 1 : 0;
						case 1:    // return FirstPos
							return IsSelection() ? m_SelStart + 1 : 0;
						case 3:    // return LastPos
							return IsSelection() ? m_SelEnd : 0;
						case 4:    // return block type (0=nothing 1=stream, 2=column)
							return IsSelection() ? 1 : 0;
					}

					break;
				}
				case 1:    // Set Pos
				{
					if (IsSelection()) {
						switch (iParam) {
							case 0:    // begin block (FirstLine & FirstPos)
							case 1:    // end block (LastLine & LastPos)
							{
								SetCellCurPos(iParam ? m_SelEnd : m_SelStart);
								Show();
								return 1;
							}
						}
					}

					break;
				}
				case 2:    // Set Stream Selection Edge
				case 3:    // Set Column Selection Edge
				{
					switch (iParam) {
						case 0:    // selection start
						{
							MSelStart = GetCurPos();
							return 1;
						}
						case 1:    // selection finish
						{
							if (MSelStart != -1) {
								if (MSelStart != GetCurPos())
									Select(MSelStart, GetCurPos());
								else
									Select(-1, 0);

								Show();
								MSelStart = -1;
								return 1;
							}

							return 0;
						}
					}

					break;
				}
				case 4:    // UnMark sel block
				{
					Select(-1, 0);
					MSelStart = -1;
					Show();
					return 1;
				}
			}

			break;
		}
	}

	return 0;
}

int Edit::CalcRTrimmedStrSize() const
{
	int TrimSize = StrSize();
	while (TrimSize > 0 && (IsSpace(m_Str[TrimSize - 1]) || IsEol(m_Str[TrimSize - 1]))) {
		--TrimSize;
	}
	return TrimSize;
}

int Edit::CalcPosFwdTo(int Pos, int LimitPos) const
{
	if (LimitPos != -1) {
		if (Pos < LimitPos)
			do {
				Pos++;
			} while (Pos < LimitPos && Pos < StrSize() && CharClasses::IsXxxfix(m_Str[Pos]));
	} else
		do {
			Pos++;
		} while (Pos < StrSize() && CharClasses::IsXxxfix(m_Str[Pos]));

	return Pos;
}

int Edit::CalcPosBwdTo(int Pos) const
{
	if (Pos <= 0)
		return 0;

	do {
		--Pos;
	} while (Pos > 0 && Pos < StrSize() && CharClasses::IsXxxfix(m_Str[Pos]));

	return Pos;
}

int Edit::ProcessKey(FarKey Key)
{
	switch (Key) {
		case KEY_ADD:
			Key = L'+';
			break;
		case KEY_SUBTRACT:
			Key = L'-';
			break;
		case KEY_MULTIPLY:
			Key = L'*';
			break;
		case KEY_DIVIDE:
			Key = L'/';
			break;
		case KEY_DECIMAL:
			Key = L'.';
			break;
		case KEY_CTRLC:
			Key = KEY_CTRLINS;
			break;
		case KEY_CTRLV:
			Key = KEY_SHIFTINS;
			break;
		case KEY_CTRLX:
			Key = KEY_SHIFTDEL;
			break;
	}

	int PrevSelStart = -1, PrevSelEnd = 0;

	if (!Flags.Check(FEDITLINE_DROPDOWNBOX) && Key == KEY_CTRLL) {
		Flags.Swap(FEDITLINE_READONLY);
	}

	/* $ 26.07.2000 SVS
	   Bugs #??
		 В строках ввода при выделенном блоке нажимаем BS и вместо
		 ожидаемого удаления блока (как в редакторе) получаем:
		   - символ перед курсором удален
		   - выделение блока снято
	*/
	if ((((Key == KEY_BS || Key == KEY_DEL || Key == KEY_NUMDEL) && Flags.Check(FEDITLINE_DELREMOVESBLOCKS))
				|| Key == KEY_CTRLD)
			&& !Flags.Check(FEDITLINE_EDITORMODE) && m_SelStart != -1 && m_SelStart < m_SelEnd) {
		DeleteBlock();
		Show();
		return TRUE;
	}

	int _Macro_IsExecuting = CtrlObject->Macro.IsExecuting();

	// $ 04.07.2000 IG - добавлена проврерка на запуск макроса (00025.edit.cpp.txt)
	if (!ShiftPressed && (!_Macro_IsExecuting || (IsNavKey(Key) && _Macro_IsExecuting)) && !IsShiftKey(Key)
			&& !Recurse && Key != KEY_SHIFT && Key != KEY_CTRL && Key != KEY_ALT && Key != KEY_RCTRL
			&& Key != KEY_RALT && Key != KEY_NONE && Key != KEY_INS && Key != KEY_KILLFOCUS
			&& Key != KEY_GOTFOCUS
			&& ((Key & (~KEY_CTRLMASK)) != KEY_LWIN && (Key & (~KEY_CTRLMASK)) != KEY_RWIN
					&& (Key & (~KEY_CTRLMASK)) != KEY_APPS)) {
		Flags.Clear(FEDITLINE_MARKINGBLOCK);    // хмм... а это здесь должно быть?

		if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) && !(Key == KEY_CTRLINS || Key == KEY_CTRLNUMPAD0)
				&& !(Key == KEY_SHIFTDEL || Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL) && !Flags.Check(FEDITLINE_EDITORMODE)
				&& Key != KEY_CTRLQ && !(Key == KEY_SHIFTINS || Key == KEY_SHIFTNUMPAD0))    // Key != KEY_SHIFTINS) //??
		{
			/* $ 12.11.2002 DJ
			   зачем рисоваться, если ничего не изменилось?
			*/
			if (m_SelStart != -1 || m_SelEnd) {
				PrevSelStart = m_SelStart;
				PrevSelEnd = m_SelEnd;
				Select(-1, 0);
				Show();
			}
		}
	}

	/* $ 11.09.2000 SVS
	   если Opt.DlgEULBsClear = 1, то BS в диалогах для UnChanged строки
	   удаляет такую строку также, как и Del
	*/
	if (((Opt.Dialogs.EULBsClear && Key == KEY_BS) || Key == KEY_DEL || Key == KEY_NUMDEL)
			&& Flags.Check(FEDITLINE_CLEARFLAG) && CurPos >= StrSize())
		Key = KEY_CTRLY;

	/* $ 15.09.2000 SVS
	   Bug - Выделяем кусочек строки -> Shift-Del удяляет всю строку
			 Так должно быть только для UnChanged состояния
	*/
	if ((Key == KEY_SHIFTDEL || Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL)
			&& Flags.Check(FEDITLINE_CLEARFLAG) && CurPos >= StrSize() && m_SelStart == -1) {
		m_SelStart = 0;
		m_SelEnd = StrSize();
	}

	if (Flags.Check(FEDITLINE_CLEARFLAG)
			&& ((Key <= 0xFFFF && Key != KEY_BS) || Key == KEY_CTRLBRACKET || Key == KEY_CTRLBACKBRACKET
					|| Key == KEY_CTRLSHIFTBRACKET || Key == KEY_CTRLSHIFTBACKBRACKET || Key == KEY_SHIFTENTER
					|| Key == KEY_SHIFTNUMENTER)) {
		LeftPos = 0;
		DisableCallback DC(m_Callback.Active);
		SetString(L"");    // mantis#0001722
		DC.Restore();
		Show();
	}

	// Здесь - вызов функции вставки путей/файлов
	if (ProcessInsPath(Key, PrevSelStart, PrevSelEnd)) {
		Show();
		return TRUE;
	}

	if (Key != KEY_NONE && Key != KEY_IDLE && Key != KEY_SHIFTINS && Key != KEY_SHIFTNUMPAD0 && Key != KEY_CTRLINS
			&& !(Key & (KEY_ALT | KEY_RALT)) && (Key < KEY_F1 || Key > KEY_F12) && Key != KEY_ALT
			&& Key != KEY_SHIFT && Key != KEY_CTRL && Key != KEY_RALT && Key != KEY_RCTRL
			&& (Key < KEY_ALT_BASE || Key > KEY_ALT_BASE + 0xFFFF) &&    // ???? 256 ???
			!((Key >= KEY_MACRO_BASE && Key <= KEY_MACRO_ENDBASE)
					|| (Key >= KEY_OP_BASE && Key <= KEY_OP_ENDBASE))
			&& Key != KEY_CTRLQ) {
		Flags.Clear(FEDITLINE_CLEARFLAG);
		Show();
	}

	switch (Key) {
		case KEY_SHIFTLEFT:
		case KEY_SHIFTNUMPAD4: {
			if (CurPos > 0) {
				RecurseProcessKey(KEY_LEFT);

				if (!Flags.Check(FEDITLINE_MARKINGBLOCK)) {
					Select(-1, 0);
					Flags.Set(FEDITLINE_MARKINGBLOCK);
				}

				if (m_SelStart != -1 && m_SelStart <= CurPos)
					Select(m_SelStart, CurPos);
				else {
					int EndPos = CalcPosFwd((Mask && *Mask) ? CalcRTrimmedStrSize() : -1);
					int NewStartPos = CurPos;

					if (EndPos > StrSize())
						EndPos = StrSize();

					if (NewStartPos > StrSize())
						NewStartPos = StrSize();

					AddSelect(NewStartPos, EndPos);
				}

				Show();
			}

			return TRUE;
		}
		case KEY_SHIFTRIGHT:
		case KEY_SHIFTNUMPAD6: {
			if (!Flags.Check(FEDITLINE_MARKINGBLOCK)) {
				Select(-1, 0);
				Flags.Set(FEDITLINE_MARKINGBLOCK);
			}

			if ((m_SelStart != -1 && m_SelEnd == -1) || m_SelEnd > CurPos) {
				if (CalcPosFwd() == m_SelEnd)
					Select(-1, 0);
				else
					Select(CalcPosFwd(), m_SelEnd);
			} else
				AddSelect(CurPos, CalcPosFwd());

			RecurseProcessKey(KEY_RIGHT);
			return TRUE;
		}
		case KEY_CTRLSHIFTLEFT:
		case KEY_CTRLSHIFTNUMPAD4: {
			if (CurPos > StrSize()) {
				PrevCurPos = CurPos;
				CurPos = StrSize();
			}

			if (CurPos > 0)
				RecurseProcessKey(KEY_SHIFTLEFT);

			while (CurPos > 0
					&& !(!IsWordDiv(WordDiv(), m_Str[CurPos]) && IsWordDiv(WordDiv(), m_Str[CurPos - 1])
							&& !IsSpace(m_Str[CurPos]))) {
				if (!IsSpace(m_Str[CurPos])
						&& (IsSpace(m_Str[CurPos - 1]) || IsWordDiv(WordDiv(), m_Str[CurPos - 1])))
					break;

				RecurseProcessKey(KEY_SHIFTLEFT);
			}

			Show();
			return TRUE;
		}
		case KEY_CTRLSHIFTRIGHT:
		case KEY_CTRLSHIFTNUMPAD6: {
			if (CurPos >= StrSize())
				return FALSE;

			RecurseProcessKey(KEY_SHIFTRIGHT);

			while (CurPos < StrSize()
					&& !(IsWordDiv(WordDiv(), m_Str[CurPos]) && !IsWordDiv(WordDiv(), m_Str[CurPos - 1]))) {
				if (!IsSpace(m_Str[CurPos])
						&& (IsSpace(m_Str[CurPos - 1]) || IsWordDiv(WordDiv(), m_Str[CurPos - 1])))
					break;

				RecurseProcessKey(KEY_SHIFTRIGHT);

				if (MaxLength != -1 && CurPos == MaxLength - 1)
					break;
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTHOME:
		case KEY_SHIFTNUMPAD7: {
			Lock();

			while (CurPos > 0)
				RecurseProcessKey(KEY_SHIFTLEFT);

			Unlock();
			Show();
			return TRUE;
		}
		case KEY_SHIFTEND:
		case KEY_SHIFTNUMPAD1: {
			Lock();
			int Len = (Mask && *Mask) ? CalcRTrimmedStrSize() : StrSize();

			int LastCurPos = CurPos;

			while (CurPos < Len /*StrSize()*/) {
				RecurseProcessKey(KEY_SHIFTRIGHT);

				if (LastCurPos == CurPos)
					break;

				LastCurPos = CurPos;
			}

			Unlock();
			Show();
			return TRUE;
		}
		case KEY_BS: {
			if (CurPos <= 0)
				return FALSE;

			PrevCurPos = CurPos;
			CurPos = CalcPosBwd();

			while (LeftPos > 0 && RealPosToCell(CurPos) <= LeftPos) {
				LeftPos-= 15;
				if (LeftPos > 0)
					LeftPos = RealPosToCell(CellPosToReal(LeftPos));
				else
					LeftPos = 0;
			}

			if (!RecurseProcessKey(KEY_DEL))
				Show();

			return TRUE;
		}
		case KEY_CTRLSHIFTBS: {
			DisableCallback DC(m_Callback.Active);

			// BUGBUG
			for (int i = CurPos; i >= 0; i--) {
				RecurseProcessKey(KEY_BS);
			}
			DC.Restore();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLBS: {
			if (CurPos > StrSize()) {
				PrevCurPos = CurPos;
				CurPos = StrSize();
			}

			Lock();

			DisableCallback DC(m_Callback.Active);

			// BUGBUG
			for (;;) {
				int StopDelete = FALSE;

				if (CurPos > 1 && IsSpace(m_Str[CurPos - 1]) != IsSpace(m_Str[CurPos - 2]))
					StopDelete = TRUE;

				RecurseProcessKey(KEY_BS);

				if (!CurPos || StopDelete)
					break;

				if (IsWordDiv(WordDiv(), m_Str[CurPos - 1]))
					break;
			}

			Unlock();
			DC.Restore();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLQ: {
			Lock();

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)
					&& (m_SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG)))
				RecurseProcessKey(KEY_DEL);

			ProcessCtrlQ();
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_OP_SELWORD: {
			int OldCurPos = CurPos;
			PrevSelStart = m_SelStart;
			PrevSelEnd = m_SelEnd;
#if defined(MOUSEKEY)

			if (CurPos >= m_SelStart && CurPos <= m_SelEnd) {    // выделяем ВСЮ строку при повторном двойном клике
				Select(0, StrSize());
			} else
#endif
			{
				int SStart, SEnd;

				if (CalcWordFromString(m_Str, CurPos, &SStart, &SEnd, WordDiv()))
					Select(SStart, SEnd + (SEnd < StrSize() ? 1 : 0));
			}

			CurPos = OldCurPos;    // возвращаем обратно
			Show();
			return TRUE;
		}
		case KEY_OP_PLAINTEXT: {
			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				if (m_SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG))    // BugZ#1053 - Неточности в $Text
					RecurseProcessKey(KEY_DEL);
			}

			const wchar_t *S = CtrlObject->Macro.GetStringToPrint();

			ProcessInsPlainText(S);

			Show();
			return TRUE;
		}
		case KEY_CTRLT:
		case KEY_CTRLDEL:
		case KEY_CTRLNUMDEL:
		case KEY_CTRLDECIMAL: {
			if (CurPos >= StrSize())
				return FALSE;

			Lock();
			DisableCallback DC(m_Callback.Active);
			if (Mask && *Mask) {
				int MaskLen = StrLength(Mask);
				int ptr = CurPos;

				while (ptr < MaskLen) {
					ptr++;

					if (!CheckCharMask(Mask[ptr]) || (IsSpace(m_Str[ptr]) && !IsSpace(m_Str[ptr + 1]))
							|| (IsWordDiv(WordDiv(), m_Str[ptr])))
						break;
				}

				// BUGBUG
				for (int i = 0; i < ptr - CurPos; i++)
					RecurseProcessKey(KEY_DEL);
			} else {
				for (;;) {
					int StopDelete = FALSE;

					if (CurPos < StrSize() - 1 && IsSpace(m_Str[CurPos]) && !IsSpace(m_Str[CurPos + 1]))
						StopDelete = TRUE;

					RecurseProcessKey(KEY_DEL);

					if (CurPos >= StrSize() || StopDelete)
						break;

					if (IsWordDiv(WordDiv(), m_Str[CurPos]))
						break;
				}
			}

			Unlock();
			DC.Restore();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLY: {
			if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			PrevCurPos = CurPos;
			LeftPos = CurPos = 0;
			m_Str.Clear();
			Select(-1, 0);
			Changed();
			Show();
			return TRUE;
		}
		case KEY_CTRLK: {
			if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			if (CurPos >= StrSize())
				return FALSE;

			if (!Flags.Check(FEDITLINE_EDITBEYONDEND)) {
				if (CurPos < m_SelEnd)
					m_SelEnd = CurPos;

				if (m_SelEnd < m_SelStart && m_SelEnd != -1) {
					m_SelEnd = 0;
					m_SelStart = -1;
				}
			}

			m_Str.Truncate(CurPos);
			Changed();
			Show();
			return TRUE;
		}
		case KEY_HOME:
		case KEY_NUMPAD7:
		case KEY_CTRLHOME:
		case KEY_CTRLNUMPAD7: {
			PrevCurPos = CurPos;
			CurPos = 0;
			Show();
			return TRUE;
		}
		case KEY_END:
		case KEY_NUMPAD1:
		case KEY_CTRLEND:
		case KEY_CTRLNUMPAD1:
		case KEY_CTRLSHIFTEND:
		case KEY_CTRLSHIFTNUMPAD1: {
			PrevCurPos = CurPos;
			CurPos = (Mask && *Mask) ? CalcRTrimmedStrSize() : StrSize();
			Show();
			return TRUE;
		}
		case KEY_LEFT:
		case KEY_NUMPAD4:
		case KEY_MSWHEEL_LEFT:
		case KEY_CTRLS: {
			if (CurPos > 0) {
				PrevCurPos = CurPos;
				CurPos = CalcPosBwd();
				Show();
			}

			return TRUE;
		}
		case KEY_RIGHT:
		case KEY_NUMPAD6:
		case KEY_MSWHEEL_RIGHT:
		case KEY_CTRLD: {
			PrevCurPos = CurPos;
			CurPos = CalcPosFwd((Mask && *Mask) ? CalcRTrimmedStrSize() : -1);
			Show();
			return TRUE;
		}
		case KEY_INS:
		case KEY_NUMPAD0: {
			Flags.Swap(FEDITLINE_OVERTYPE);
			Show();
			return TRUE;
		}
		case KEY_NUMDEL:
		case KEY_DEL: {
			if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			if (CurPos >= StrSize())
				return FALSE;

			if (m_SelStart != -1) {
				if (m_SelEnd != -1 && CurPos < m_SelEnd)
					m_SelEnd--;

				if (CurPos < m_SelStart)
					m_SelStart--;

				if (m_SelEnd != -1 && m_SelEnd <= m_SelStart) {
					m_SelStart = -1;
					m_SelEnd = 0;
				}
			}

			if (Mask && *Mask) {
				const size_t MaskLen = wcslen(Mask);
				size_t j = CurPos;
				for (size_t i = CurPos; i < MaskLen; ++i) {
					if (i + 1 < MaskLen && CheckCharMask(Mask[i + 1])) {
						while (j < MaskLen && !CheckCharMask(Mask[j]))
							j++;

						if (!CharInMask(m_Str[i + 1], Mask[j]))
							break;

						m_Str.ReplaceChar(j, m_Str[i + 1]);
						j++;
					}
				}

				m_Str.ReplaceChar(j, L' ');
			} else {
				auto NextPos = CalcPosFwd();
				if (NextPos > CurPos) {
					m_Str.Replace(CurPos, NextPos - CurPos, L"", 0);
				}
			}

			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLLEFT:
		case KEY_CTRLNUMPAD4: {
			PrevCurPos = CurPos;

			if (CurPos > StrSize())
				CurPos = StrSize();

			CurPos = CalcPosBwd();

			while (CurPos > 0
					&& !(!IsWordDiv(WordDiv(), m_Str[CurPos]) && IsWordDiv(WordDiv(), m_Str[CurPos - 1])
							&& !IsSpace(m_Str[CurPos]))) {
				if (!IsSpace(m_Str[CurPos]) && IsSpace(m_Str[CurPos - 1]))
					break;

				CurPos--;
			}

			Show();
			return TRUE;
		}
		case KEY_CTRLRIGHT:
		case KEY_CTRLNUMPAD6: {
			if (CurPos >= StrSize())
				return FALSE;

			PrevCurPos = CurPos;
			int Len;

			if (Mask && *Mask) {
				Len = CalcRTrimmedStrSize();
				CurPos = CalcPosFwd(Len);
			} else {
				Len = StrSize();
				CurPos = CalcPosFwd();
			}

			while (CurPos < Len /*StrSize()*/
					&& !(IsWordDiv(WordDiv(), m_Str[CurPos]) && !IsWordDiv(WordDiv(), m_Str[CurPos - 1]))) {
				if (!IsSpace(m_Str[CurPos]) && IsSpace(m_Str[CurPos - 1]))
					break;

				CurPos++;
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTNUMDEL:
		case KEY_SHIFTDECIMAL:
		case KEY_SHIFTDEL: {
			if (m_SelStart == -1 || m_SelStart >= m_SelEnd)
				return FALSE;

			RecurseProcessKey(KEY_CTRLINS);
			DeleteBlock();
			Show();
			return TRUE;
		}
		case KEY_CTRLINS:
		case KEY_CTRLNUMPAD0: {
			if (!Flags.Check(FEDITLINE_PASSWORDMODE)) {
				if (m_SelStart == -1 || m_SelStart >= m_SelEnd) {
					if (Mask && *Mask) {
						std::wstring TrimmedStr(m_Str, CalcRTrimmedStrSize());
						CopyToClipboard(TrimmedStr.c_str());
					} else {
						CopyToClipboard(m_Str);
					}
				} else if (m_SelEnd <= StrSize()) // TODO: если в начало условия добавить "StrSize() &&", то пропадет баг "Ctrl-Ins в пустой строке очищает клипборд"
				{
					CopyToClipboard(FARString(m_Str+m_SelStart, m_SelEnd-m_SelStart));
				}
			}

			return TRUE;
		}
		case KEY_SHIFTINS:
		case KEY_SHIFTNUMPAD0: {
			wchar_t *ClipText = PasteFromClipboardEx(MaxLength);

			if (!ClipText)
				return TRUE;

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				DisableCallback DC(m_Callback.Active);
				DeleteBlock();
			}

			for (int i = StrLength(m_Str) - 1; i >= 0 && IsEol(m_Str[i]); i--)
				m_Str.ReplaceChar(i, 0);

			for (int i = 0; ClipText[i]; i++) {
				if (IsEol(ClipText[i])) {
					if (IsEol(ClipText[i + 1]))
						wmemmove(&ClipText[i], &ClipText[i + 1], StrLength(&ClipText[i + 1]) + 1);

					if (!ClipText[i + 1])
						ClipText[i] = 0;
					else
						ClipText[i] = L' ';
				}
			}

			if (Flags.Check(FEDITLINE_CLEARFLAG)) {
				LeftPos = 0;
				Flags.Clear(FEDITLINE_CLEARFLAG);
				SetString(ClipText);
			} else {
				InsertString(ClipText);
			}

			if (ClipText)
				free(ClipText);

			Show();
			return TRUE;
		}
		case KEY_SHIFTTAB: {
			PrevCurPos = CurPos;
			CursorPos-= (CursorPos - 1) % TabSize + 1;

			if (CursorPos < 0)
				CursorPos = 0;    // CursorPos=0,TabSize=1 case

			SetCellCurPos(CursorPos);
			Show();
			return TRUE;
		}
		case KEY_SHIFTSPACE:
			Key = KEY_SPACE;
		default: {
			// D(SysLog(L"Key=0x%08X",Key));
			if (Key == KEY_ENTER || !IS_KEY_NORMAL(Key))    // KEY_NUMENTER,KEY_IDLE,KEY_NONE covered by !IS_KEY_NORMAL
				break;

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)) {
				if (PrevSelStart != -1) {
					m_SelStart = PrevSelStart;
					m_SelEnd = PrevSelEnd;
				}
				DisableCallback DC(m_Callback.Active);
				DeleteBlock();
			}

			if (InsertKey(Key))
				Show();

			return TRUE;
		}
	}

	return FALSE;
}

// обработка Ctrl-Q
int Edit::ProcessCtrlQ()
{
	INPUT_RECORD rec;
	FarKey Key;

	for (;;) {
		Key = GetInputRecord(&rec);

		if (Key != KEY_NONE && Key != KEY_IDLE && rec.Event.KeyEvent.uChar.AsciiChar)
			break;

		if (Key == KEY_CONSOLE_BUFFER_RESIZE) {
			//      int Dis=EditOutDisabled;
			//      EditOutDisabled=0;
			Show();
			//      EditOutDisabled=Dis;
		}
	}

	/*
	  EditOutDisabled++;
	  if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
	  {
		DeleteBlock();
	  }
	  else
		Flags.Clear(FEDITLINE_CLEARFLAG);
	  EditOutDisabled--;
	*/
	CHAR ch = rec.Event.KeyEvent.uChar.UnicodeChar;
	if( rec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED |RIGHT_CTRL_PRESSED ) && ch >= 'A' && ch <= 'Z'  )
		ch -= ('A' - 1); // convert to binary
	return InsertKey(ch);
}

bool Edit::ProcessInsPlainText(const wchar_t *str)
{
	if (*str) {
		InsertString(str);
		return true;
	}

	return false;
}

bool Edit::InsertKey(FarKey Key)
{
	bool changed = false;

	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return true;

	if (Key == KEY_TAB && Flags.Check(FEDITLINE_OVERTYPE)) {
		PrevCurPos = CurPos;
		CursorPos+= TabSize - (CursorPos % TabSize);
		SetCellCurPos(CursorPos);
		return true;
	}

	if (Mask && *Mask) {
		int MaskLen = StrLength(Mask);

		if (CurPos < MaskLen) {
			if (KeyMatchedMask(Key)) {
				if (!Flags.Check(FEDITLINE_OVERTYPE)) {
					int i = MaskLen - 1;

					while (!CheckCharMask(Mask[i]) && i > CurPos)
						i--;

					for (int j = i; i > CurPos; i--) {
						if (CheckCharMask(Mask[i])) {
							while (!CheckCharMask(Mask[j - 1])) {
								if (j <= CurPos)
									break;

								j--;
							}

							m_Str.ReplaceChar(i, m_Str[j - 1]);
							j--;
						}
					}
				}

				PrevCurPos = CurPos;
				m_Str.ReplaceChar(CurPos++, Key);
				changed = true;
			} else {
				// Здесь вариант для "ввели символ из маски", например для SetAttr - ввесли '.'
				;    // char *Ptr=strchr(Mask+CurPos,Key);
			}
		} else if (CurPos < StrSize()) {
			PrevCurPos = CurPos;
			m_Str.ReplaceChar(CurPos++, Key);
			changed = true;
		}
	} else {
		if (MaxLength == -1 || StrSize() < MaxLength) {
			if (CurPos >= StrSize()) {
				m_Str.Replace(StrSize(), 0, L' ', CurPos - StrSize());
			}

			if (Key == KEY_TAB && (TabExpandMode == EXPAND_NEWTABS || TabExpandMode == EXPAND_ALLTABS)) {
				InsertTab();
				return true;
			}

			if (!Flags.Check(FEDITLINE_OVERTYPE)) {
				if (m_SelStart != -1) {
					if (m_SelEnd != -1 && CurPos < m_SelEnd)
						m_SelEnd++;

					if (CurPos < m_SelStart)
						m_SelStart++;
				}
			}

			PrevCurPos = CurPos;
			m_Str.Replace(CurPos++, 0, Key, 1);

			wchar_t ch = static_cast<wchar_t>(Key);
			CheckForSpecialWidthChars(&ch, 1);

			changed = true;
		} else if (Flags.Check(FEDITLINE_OVERTYPE)) {
			if (CurPos < StrSize()) {
				PrevCurPos = CurPos;
				m_Str.ReplaceChar(CurPos++, Key);
				changed = true;
			}
		}
		/*else
			MessageBeep(MB_ICONHAND);*/
	}

	if (changed)
		Changed();

	return true;
}

void Edit::SetObjectColor(uint64_t Color, uint64_t SelColor, uint64_t ColorUnChanged)
{
	this->Color = Color;
	this->SelColor = SelColor;
	this->ColorUnChanged = ColorUnChanged;
}

void Edit::GetString(wchar_t *Str, int MaxSize) const
{
	const auto Len = Min(StrSize(), MaxSize - 1);
	wmemmove(Str, m_Str, Len);
	Str[Len] = 0;
	Str[MaxSize - 1] = 0;
}

void Edit::GetString(FARString &strStr) const
{
	strStr = m_Str;
}

const wchar_t *Edit::GetStringAddr()
{
	return m_Str;
}

void Edit::SetHiString(const wchar_t *Str)
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	FARString NewStr;
	HiText2Str(NewStr, Str);
	Select(-1, 0);
	SetBinaryString(NewStr, StrLength(NewStr));
}

void Edit::SetString(const wchar_t *Str, int Length)
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	Select(-1, 0);
	SetBinaryString(Str, Length == -1 ? (int)StrLength(Str) : Length);
}

void Edit::SetEOL(const wchar_t *EOL)
{
	EndType = EOL_NONE;

	if (EOL && *EOL) {
		if (EOL[0] == L'\r')
			if (EOL[1] == L'\n')
				EndType = EOL_CRLF;
			else if (EOL[1] == L'\r' && EOL[2] == L'\n')
				EndType = EOL_CRCRLF;
			else
				EndType = EOL_CR;
		else if (EOL[0] == L'\n')
			EndType = EOL_LF;
	}
}

const wchar_t *Edit::GetEOL()
{
	return EOL_TYPE_CHARS[EndType];
}

void Edit::CheckForSpecialWidthChars(const wchar_t *CheckStr, int Length)
{
	if (HasSpecialWidthChars) return;

	if (!CheckStr) {
		CheckStr = m_Str;
		Length = StrSize();
	}

	for (int i = 0; i < Length; ++i) {
		auto wc = CheckStr[i];
		if (wc == L'\t' || CharClasses::IsFullWidth(wc)
						|| CharClasses::IsXxxfix(wc) ) {
			HasSpecialWidthChars = true;
			return;
		}
	}
}

/* $ 25.07.2000 tran
   примечание:
   в этом методе DropDownBox не обрабатывается
   ибо он вызывается только из SetString и из класса Editor
   в Dialog он нигде не вызывается */
void Edit::SetBinaryString(const wchar_t *Str, int Length)
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	// коррекция вставляемого размера, если определен MaxLength
	if (MaxLength != -1 && Length > MaxLength) {
		Length = MaxLength;    // ??
	}

	if (Length > 0 && !Flags.Check(FEDITLINE_PARENT_SINGLELINE)) {
		if (Str[Length - 1] == L'\r') {
			EndType = EOL_CR;
			Length--;
		} else {
			if (Str[Length - 1] == L'\n') {
				Length--;

				if (Length > 0 && Str[Length - 1] == L'\r') {
					Length--;

					if (Length > 0 && Str[Length - 1] == L'\r') {
						Length--;
						EndType = EOL_CRCRLF;
					} else
						EndType = EOL_CRLF;
				} else
					EndType = EOL_LF;
			} else
				EndType = EOL_NONE;
		}
	}

	CurPos = 0;

	if (Mask && *Mask) {
		RefreshStrByMask(TRUE);
		int maskLen = StrLength(Mask);

		for (int i = 0, j = 0; j < maskLen && j < Length;) {
			if (CheckCharMask(Mask[i])) {
				bool goLoop = false;

				if (KeyMatchedMask(Str[j]))
					InsertKey(Str[j]);
				else
					goLoop = true;

				j++;

				if (goLoop)
					continue;
			} else {
				PrevCurPos = CurPos;
				CurPos++;
			}

			i++;
		}

		/* Здесь необходимо условие (!*Str), т.к. для очистки строки
		   обычно вводится нечто вроде SetBinaryString("",0)
		   Т.е. таким образом мы добиваемся "инициализации" строки с маской
		*/
		RefreshStrByMask(!*Str);
	} else {
		m_Str = FARString(Str, Length);

		if (TabExpandMode == EXPAND_ALLTABS)
			ExpandTabs();

		PrevCurPos = CurPos;
		CurPos = StrSize();

		HasSpecialWidthChars=false;
		CheckForSpecialWidthChars();
	}

	Changed();
}

void Edit::GetBinaryString(const wchar_t **Str, const wchar_t **EOL, int &Length)
{
	*Str = m_Str;

	if (EOL)
		*EOL = EOL_TYPE_CHARS[EndType];

	Length = StrSize();    //???
}

bool Edit::GetSelString(wchar_t *Str, int MaxSize)
{
	if (m_SelStart == -1 || (m_SelEnd != -1 && m_SelEnd <= m_SelStart) || m_SelStart >= StrSize()) {
		*Str = 0;
		return false;
	}

	int CopyLength;

	if (m_SelEnd == -1)
		CopyLength = MaxSize;
	else
		CopyLength = Min(MaxSize, m_SelEnd - m_SelStart + 1);

	far_wcsncpy(Str, m_Str + m_SelStart, CopyLength);
	return true;
}

bool Edit::GetSelString(FARString &Str)
{
	if (m_SelStart == -1 || (m_SelEnd != -1 && m_SelEnd <= m_SelStart) || m_SelStart >= StrSize()) {
		Str.Clear();
		return false;
	}

	int CopyLength = m_SelEnd - m_SelStart + 1;
	wchar_t *Buf = Str.GetBuffer(CopyLength + 1);
	far_wcsncpy(Buf, m_Str + m_SelStart, CopyLength);
	Str.ReleaseBuffer();
	return true;
}

void Edit::InsertString(const wchar_t *Str)
{
	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return;

	if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
		DeleteBlock();

	InsertBinaryString(Str, StrLength(Str));
}

void Edit::InsertBinaryString(const wchar_t *Str, int Length)
{
	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return;

	Flags.Clear(FEDITLINE_CLEARFLAG);

	if (Mask && *Mask) {
		int Pos = CurPos;
		int MaskLen = StrLength(Mask);

		if (Pos < MaskLen) {
			//_SVS(SysLog(L"InsertBinaryString ==> m_Str='%ls' (Length=%d) Mask='%ls'",m_Str,Length,Mask+Pos));
			int StrLen = (MaskLen - Pos > Length) ? Length : MaskLen - Pos;

			/* $ 15.11.2000 KM
			   Внесены исправления для правильной работы PasteFromClipboard
			   в строке с маской
			*/
			for (int i = Pos, j = 0; j < StrLen + Pos;) {
				if (CheckCharMask(Mask[i])) {
					bool goLoop = false;

					if (j < Length && KeyMatchedMask(Str[j])) {
						InsertKey(Str[j]);
						//_SVS(SysLog(L"InsertBinaryString ==> InsertKey(m_Str[%d]='%c');",j,m_Str[j]));
					} else
						goLoop = true;

					j++;

					if (goLoop)
						continue;
				} else {
					if (Mask[j] == Str[j]) {
						j++;
					}
					PrevCurPos = CurPos;
					CurPos++;
				}

				i++;
			}
		}

		RefreshStrByMask();
		//_SVS(SysLog(L"InsertBinaryString ==> this->m_Str='%ls'",this->m_Str));
	} else {
		if (MaxLength != -1 && StrSize() + Length > MaxLength) {
			// коррекция вставляемого размера, если определен MaxLength
			if (StrSize() < MaxLength) {
				Length = MaxLength - StrSize();
			}
		}

		if (MaxLength == -1 || StrSize() + Length <= MaxLength) {
			if (CurPos > StrSize()) {
				m_Str.Replace(StrSize(), 0, L' ', CurPos - StrSize());
			}

			m_Str.Replace(CurPos, 0, Str, Length);
			PrevCurPos = CurPos;
			CurPos+= Length;

			if (TabExpandMode == EXPAND_ALLTABS)
				ExpandTabs();

			CheckForSpecialWidthChars(Str, Length);
			Changed();
		}
		/*else
			MessageBeep(MB_ICONHAND);*/
	}
}

int Edit::GetLength()
{
	return StrSize();
}

// Функция установки маски ввода в объект Edit
void Edit::SetInputMask(const wchar_t *InputMask)
{
	if (Mask)
		free(Mask);

	if (InputMask && *InputMask) {
		if (!(Mask = wcsdup(InputMask)))
			return;

		RefreshStrByMask(TRUE);
	} else
		Mask = nullptr;
}

// Функция обновления состояния строки ввода по содержимому Mask
void Edit::RefreshStrByMask(bool InitMode)
{
	if (Mask && *Mask) {
		int MaskLen = StrLength(Mask);

		if (StrSize() > MaskLen)
			m_Str.Truncate(MaskLen);
		else
			m_Str.Replace(StrSize(), 0, L' ', MaskLen - StrSize());

		for (int i = 0; i < MaskLen; i++) {
			if (!CheckCharMask(Mask[i]))
				m_Str.ReplaceChar(i, Mask[i]);
			else if (InitMode)
				m_Str.ReplaceChar(i, L' ');
		}
	}
}

int Edit::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (!(MouseEvent->dwButtonState & 3))
		return FALSE;

	if (MouseEvent->dwMousePosition.X < X1 || MouseEvent->dwMousePosition.X > X2
			|| MouseEvent->dwMousePosition.Y != Y1)
		return FALSE;

	// SetClearFlag(0); // пусть едитор сам заботится о снятии клеар-текста?
	SetCellCurPos(MouseEvent->dwMousePosition.X - X1 + LeftPos);

	if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
		Select(-1, 0);

	if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
		static int PrevDoubleClick = 0;
		static COORD PrevPosition = {0, 0};

		if (WINPORT(GetTickCount)() - PrevDoubleClick <= WINPORT(GetDoubleClickTime)()
				&& MouseEvent->dwEventFlags != MOUSE_MOVED && PrevPosition.X == MouseEvent->dwMousePosition.X
				&& PrevPosition.Y == MouseEvent->dwMousePosition.Y) {
			Select(0, StrSize());
			PrevDoubleClick = 0;
			PrevPosition.X = 0;
			PrevPosition.Y = 0;
		}

		if (MouseEvent->dwEventFlags == DOUBLE_CLICK) {
			ProcessKey(KEY_OP_SELWORD);
			PrevDoubleClick = WINPORT(GetTickCount)();
			PrevPosition = MouseEvent->dwMousePosition;
		} else {
			PrevDoubleClick = 0;
			PrevPosition.X = 0;
			PrevPosition.Y = 0;
		}
	}

	Show();
	return TRUE;
}

/* $ 03.08.2000 KM
   Немного изменён алгоритм из-за необходимости
   добавления поиска целых слов.
*/
int Edit::Search(const FARString &Str, FARString &ReplaceStr, int Position, int Case, int WholeWords,
		int Reverse, int Regexp, int *SearchLength)
{
	return SearchString(m_Str, StrSize(), Str, ReplaceStr, CurPos, Position, Case, WholeWords,
			Reverse, Regexp, SearchLength, WordDiv());
}

void Edit::InsertTab()
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	const int Pos = CurPos;
	const int S = TabSize - (Pos % TabSize);

	if (m_SelStart != -1) {
		if (Pos <= m_SelStart) {
			m_SelStart+= S - (Pos == m_SelStart ? 0 : 1);
		}

		if (m_SelEnd != -1 && Pos < m_SelEnd) {
			m_SelEnd+= S;
		}
	}

	CurPos+= S;
	m_Str.Replace(Pos, 0, L' ', S);
	Changed();
}

void Edit::ExpandTabs()
{
	wchar_t *TabPtr;
	int Pos = 0, S;

	if (Flags.Check(FEDITLINE_READONLY))
		return;

	bool changed = false;

	while ((TabPtr = (wchar_t *)wmemchr(m_Str + Pos, L'\t', StrSize() - Pos))) {
		changed = true;
		Pos = (int)(TabPtr - m_Str);
		S = TabSize - ((int)(TabPtr - m_Str) % TabSize);

		if (m_SelStart != -1) {
			if (Pos <= m_SelStart) {
				m_SelStart+= S - (Pos == m_SelStart ? 0 : 1);
			}

			if (m_SelEnd != -1 && Pos < m_SelEnd) {
				m_SelEnd+= S - 1;
			}
		}

		if (CurPos > Pos)
			CurPos+= S - 1;

		m_Str.Replace(Pos, 1, L' ', S);
	}

	if (changed)
		Changed();
}

int Edit::GetCellCurPos()
{
	return (RealPosToCell(CurPos));
}

void Edit::SetCellCurPos(int NewPos)
{
	if (Mask && *Mask) {
		int NewPosLimit = CalcRTrimmedStrSize();
		if (NewPos > NewPosLimit)
			NewPos = NewPosLimit;
	}

	CurPos = CellPosToReal(NewPos);
}

int Edit::RealPosToCell(int Pos)
{
	return RealPosToCell(0, 0, Pos, nullptr);
}

int Edit::RealPosToCell(int PrevLength, int PrevPos, int Pos, int *CorrectPos)
{
	// Корректировка табов
	bool bCorrectPos = CorrectPos && *CorrectPos;
	if (CorrectPos)
		*CorrectPos = 0;

	// Инциализируем результирующую длину предыдущим значением
	int TabPos = PrevLength;

	// Если предыдущая позиция за концом строки, то табов там точно нет и
	// вычислять особо ничего не надо, иначе производим вычисление
	if (PrevPos >= StrSize() || !HasSpecialWidthChars)
		TabPos+= Pos - PrevPos;
	else {
		// Начинаем вычисление с предыдущей позиции
		int Index = PrevPos;

		// Проходим по всем символам до позиции поиска, если она ещё в пределах строки,
		// либо до конца строки, если позиция поиска за пределами строки
		for (; Index < Min(Pos, StrSize()); Index++)

			// Обрабатываем табы
			if (m_Str[Index] == L'\t' && TabExpandMode != EXPAND_ALLTABS) {
				// Если есть необходимость делать корректировку табов и эта коректировка
				// ещё не проводилась, то увеличиваем длину обрабатываемой строки на еденицу
				if (bCorrectPos) {
					++Pos;
					*CorrectPos = 1;
					bCorrectPos = false;
				}

				// Расчитываем длину таба с учётом настроек и текущей позиции в строке
				TabPos+= TabSize - (TabPos % TabSize);
			}
			// Обрабатываем все остальные символы
			else {
				if (CharClasses::IsFullWidth(m_Str[Index])) {
					TabPos+= 2;
				} else if (!CharClasses::IsXxxfix(m_Str[Index])) {
					TabPos++;
				}
			}

		// Если позиция находится за пределами строки, то там точно нет табов и всё просто
		if (Pos >= StrSize())
			TabPos+= Pos - Index;
	}
	return TabPos;
}

int Edit::CellPosToReal(int Pos)
{
	if (!HasSpecialWidthChars) return Pos;
	int Index = 0;
	for (int CellPos = 0; CellPos < Pos; Index++) {
		if (Index >= StrSize()) {
			Index+= Pos - CellPos;
			break;
		}

		if (m_Str[Index] == L'\t' && TabExpandMode != EXPAND_ALLTABS) {
			int NewCellPos = CellPos + TabSize - (CellPos % TabSize);

			if (NewCellPos > Pos)
				break;

			CellPos = NewCellPos;
		} else {
			CellPos+= CharClasses::IsFullWidth(m_Str[Index]) ? 2 : CharClasses::IsXxxfix(m_Str[Index]) ? 0 : 1;
			while (Index + 1 < StrSize() && CharClasses::IsXxxfix(m_Str[Index + 1])) {
				Index++;
			}
		}
	}
	return Index;
}

void Edit::SanitizeSelectionRange()
{
	if (HasSpecialWidthChars && m_SelEnd >= m_SelStart && m_SelStart >= 0) {
		while (m_SelStart > 0 && CharClasses::IsXxxfix(m_Str[m_SelStart]))
			--m_SelStart;

		while (m_SelEnd < StrSize() && CharClasses::IsXxxfix(m_Str[m_SelEnd]))
			++m_SelEnd;
	}

	/* $ 24.06.2002 SKV
	   Если начало выделения за концом строки, надо выделение снять.
	   17.09.2002 возвращаю обратно. Глюкодром.
	*/
	if (m_SelEnd < m_SelStart && m_SelEnd != -1) {
		m_SelStart = -1;
		m_SelEnd = 0;
	}

	if (m_SelStart == -1 && m_SelEnd == -1) {
		m_SelStart = -1;
		m_SelEnd = 0;
	}
}

void Edit::Select(int Start, int End)
{
	m_SelStart = Start;
	m_SelEnd = End;

	SanitizeSelectionRange();
}

void Edit::AddSelect(int Start, int End)
{
	if (Start < m_SelStart || m_SelStart == -1)
		m_SelStart = Start;

	if (End == -1 || (End > m_SelEnd && m_SelEnd != -1))
		m_SelEnd = End;

	if (m_SelEnd > StrSize())
		m_SelEnd = StrSize();

	SanitizeSelectionRange();
}

void Edit::GetSelection(int &Start, int &End)
{
	Start = m_SelStart;
	End = m_SelEnd;

	if (End > StrSize())
		End = -1;

	if (Start > StrSize())
		Start = StrSize();
}

void Edit::GetRealSelection(int &Start, int &End)
{
	Start = m_SelStart;
	End = m_SelEnd;
}

void Edit::DeleteBlock()
{
	if (Flags.Check(FEDITLINE_READONLY | FEDITLINE_DROPDOWNBOX))
		return;

	if (m_SelStart == -1 || m_SelStart >= m_SelEnd)
		return;

	PrevCurPos = CurPos;

	if (Mask && *Mask) {
		for (int i = m_SelStart; i < m_SelEnd; i++) {
			if (CheckCharMask(Mask[i]))
				m_Str.ReplaceChar(i, L' ');
		}

		CurPos = m_SelStart;
	} else {
		int From = m_SelStart, To = m_SelEnd;

		if (From > StrSize())
			From = StrSize();

		if (To > StrSize())
			To = StrSize();

		m_Str.Replace(From, To - From, L"", 0);

		if (CurPos > From) {
			if (CurPos < To)
				CurPos = From;
			else
				CurPos-= To - From;
		}
	}

	m_SelStart = -1;
	m_SelEnd = 0;
	Flags.Clear(FEDITLINE_MARKINGBLOCK);

	// OT: Проверка на корректность поведени строки при удалении и вставки
	if (Flags.Check((FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE))) {
		if (LeftPos > CurPos)
			LeftPos = CurPos;
	}

	Changed(true);
}

void Edit::AddColor(const ColorItem *col)
{
	ColorList.emplace_back(*col);
}

size_t Edit::DeleteColor(int ColorPos)
{
	if (ColorList.empty())
		return 0;

	size_t Dest, Src;

	for (Src = Dest = 0; Src < ColorList.size(); ++Src)
		if (ColorPos != -1 && ColorList[Src].StartPos != ColorPos) {
			if (Dest != Src)
				ColorList[Dest] = ColorList[Src];

			++Dest;
		}

	const size_t DelCount = ColorList.size() - Dest;
	ColorList.resize(Dest);

	return DelCount;
}

bool Edit::GetColor(ColorItem *col, int Item)
{
	if (Item >= (int)ColorList.size())
		return false;

	*col = ColorList[Item];
	return true;
}

void Edit::ApplyColor()
{
	// Для оптимизации сохраняем вычисленные позиции между итерациями цикла
	int Pos = INT_MIN, TabPos = INT_MIN, TabEditorPos = INT_MIN;

	// Обрабатываем элементы ракраски
	for (auto &CurItem : ColorList) {
		// Пропускаем элементы у которых начало больше конца
		if (CurItem.StartPos > CurItem.EndPos)
			continue;

		// Отсекаем элементы заведомо не попадающие на экран
		if (CurItem.StartPos - LeftPos > X2 && CurItem.EndPos - LeftPos < X1)
			continue;

		DWORD64 Attr = CurItem.Color;
		int Length = CurItem.EndPos - CurItem.StartPos + 1;

		if (CurItem.StartPos + Length >= StrSize())
			Length = StrSize() - CurItem.StartPos;

		// Получаем начальную позицию
		int RealStart, Start;

		/*
			Если предыдущая позиция равна текущей, то ничего не вычисляем
			и сразу берём ранее вычисленное значение
		*/
		if (Pos == CurItem.StartPos) {
			RealStart = TabPos;
			Start = TabEditorPos;
		}
		/*
			Если вычисление идёт первый раз или предыдущая позиция больше текущей,
			то производим вычисление с начала строки
		*/
		else if (Pos == INT_MIN || CurItem.StartPos < Pos) {
			RealStart = RealPosToCell(CurItem.StartPos);
			Start = RealStart - LeftPos;
		}
		// Для оптимизации делаем вычисление относительно предыдущей позиции
		else {
			RealStart = RealPosToCell(TabPos, Pos, CurItem.StartPos, nullptr);
			Start = RealStart - LeftPos;
		}

		// Запоминаем вычисленные значения для их дальнейшего повторного использования
		Pos = CurItem.StartPos;
		TabPos = RealStart;
		TabEditorPos = Start;

		// Пропускаем элементы раскраски у которых начальная позиция за экраном
		if (Start > X2)
			continue;

		// Корректировка относительно табов (отключается, если присутвует флаг ECF_TAB1)
		int CorrectPos = Attr & ECF_TAB1 ? 0 : 1;

		if (!CorrectPos)
			Attr&= ~ECF_TAB1;

		// Получаем конечную позицию
		int EndPos = CurItem.EndPos;
		int RealEnd, End;

		/*
			Обрабатываем случай, когда предыдущая позиция равна текущей, то есть
			длина раскрашиваемой строкии равна 1
		*/
		if (Pos == EndPos) {
			/*
				Если необходимо делать корректироку относительно табов и единственный
				символ строки -- это таб, то делаем расчёт с учтом корректировки,
				иначе ничего не вычисялем и берём старые значения
			*/
			if (CorrectPos && EndPos < StrSize() && m_Str[EndPos] == L'\t') {
				RealEnd = RealPosToCell(TabPos, Pos, ++EndPos, nullptr);
				End = RealEnd - LeftPos;
			} else {
				RealEnd = TabPos;
				CorrectPos = 0;
				End = TabEditorPos;
			}
		}
		/*
			Если предыдущая позиция больше текущей, то производим вычисление
			с начала строки (с учётом корректировки относительно табов)
		*/
		else if (EndPos < Pos) {
			RealEnd = RealPosToCell(0, 0, EndPos, &CorrectPos);
			EndPos+= CorrectPos;
			End = RealEnd - LeftPos;
		}
		/*
			Для оптимизации делаем вычисление относительно предыдущей позиции (с учётом
			корректировки относительно табов)
		*/
		else {
			RealEnd = RealPosToCell(TabPos, Pos, EndPos, &CorrectPos);
			EndPos+= CorrectPos;
			End = RealEnd - LeftPos;
		}

		// Запоминаем вычисленные значения для их дальнейшего повторного использования
		Pos = EndPos;
		TabPos = RealEnd;
		TabEditorPos = End;

		// Пропускаем элементы раскраски у которых конечная позиция меньше левой границы экрана
		if (End < X1)
			continue;

		// Обрезаем раскраску элемента по экрану
		if (Start < X1)
			Start = X1;

		if (End > X2)
			End = X2;

		// Устанавливаем длину раскрашиваемого элемента
		Length = End - Start + 1;

		if (Length < X2)
			Length-= CorrectPos;

		// Раскрашиваем элемент, если есть что раскрашивать
		if (Length > 0) {
			ScrBuf.ApplyColor(Start, Y1, Start + Length - 1, Y1, Attr, SelColor);
			// Не раскрашиваем выделение
			//					SelColor >= COL_FIRSTPALETTECOLOR ? Palette[SelColor - COL_FIRSTPALETTECOLOR] : SelColor);
		}
	}
}

/* $ 24.09.2000 SVS $
  Функция Xlat - перекодировка по принципу QWERTY <-> ЙЦУКЕН
*/
void Edit::Xlat(bool All)
{
	//   Для CmdLine - если нет выделения, преобразуем всю строку
	if (All && m_SelStart == -1 && !m_SelEnd) {
		auto Buf = m_Str.GetBuffer();
		::Xlat(Buf, 0, StrLength(Buf), Opt.XLat.Flags);
		m_Str.ReleaseBuffer();
		Changed();
		Show();
		return;
	}

	if (m_SelStart != -1 && m_SelStart != m_SelEnd) {
		if (m_SelEnd == -1)
			m_SelEnd = StrLength(m_Str);

		auto Buf = m_Str.GetBuffer();
		::Xlat(Buf, m_SelStart, m_SelEnd, Opt.XLat.Flags);
		m_Str.ReleaseBuffer();
		Changed();
		Show();
	}
	/* $ 25.11.2000 IS
	 Если нет выделения, то обработаем текущее слово. Слово определяется на
	 основе специальной группы разделителей.
	*/
	else {
		/* $ 10.12.2000 IS
		   Обрабатываем только то слово, на котором стоит курсор, или то слово, что
		   находится левее позиции курсора на 1 символ
		*/
		int start = CurPos, end, StrSize = StrLength(m_Str);
		bool DoXlat = true;

		if (IsWordDiv(Opt.XLat.strWordDivForXlat, m_Str[start])) {
			if (start)
				start--;

			DoXlat = (!IsWordDiv(Opt.XLat.strWordDivForXlat, m_Str[start]));
		}

		if (DoXlat) {
			while (start >= 0 && !IsWordDiv(Opt.XLat.strWordDivForXlat, m_Str[start]))
				start--;

			start++;
			end = start + 1;

			while (end < StrSize && !IsWordDiv(Opt.XLat.strWordDivForXlat, m_Str[end]))
				end++;

			auto Buf = m_Str.GetBuffer();
			::Xlat(Buf, start, end, Opt.XLat.Flags);
			m_Str.ReleaseBuffer();
			Changed();
			Show();
		}
	}
}

/* $ 15.11.2000 KM
   Проверяет: попадает ли символ в разрешённый
   диапазон символов, пропускаемых маской
*/
bool Edit::KeyMatchedMask(int Key) const
{
	return CharInMask(Key, Mask[CurPos]);
}

bool Edit::CharInMask(wchar_t Char, wchar_t Mask)
{
	return (Mask == EDMASK_ANY)
			|| (Mask == EDMASK_DSS && (std::iswdigit(Char) || Char == L' ' || Char == L'-'))
			|| (Mask == EDMASK_DIGITS && (std::iswdigit(Char) || Char == L' '))
			|| (Mask == EDMASK_DIGIT && (std::iswdigit(Char))) || (Mask == EDMASK_ALPHA && IsAlpha(Char))
			|| (Mask == EDMASK_HEX && std::iswxdigit(Char));
}

bool Edit::CheckCharMask(wchar_t Chr)
{
	return (Chr == EDMASK_ANY || Chr == EDMASK_DIGIT || Chr == EDMASK_DIGITS || Chr == EDMASK_DSS
				   || Chr == EDMASK_ALPHA || Chr == EDMASK_HEX);
}

void Edit::SetDialogParent(DWORD Sets)
{
	if ((Sets & (FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE))
					== (FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE)
			|| !(Sets & (FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE)))
		Flags.Clear(FEDITLINE_PARENT_SINGLELINE | FEDITLINE_PARENT_MULTILINE);
	else if (Sets & FEDITLINE_PARENT_SINGLELINE) {
		Flags.Clear(FEDITLINE_PARENT_MULTILINE);
		Flags.Set(FEDITLINE_PARENT_SINGLELINE);
	} else if (Sets & FEDITLINE_PARENT_MULTILINE) {
		Flags.Clear(FEDITLINE_PARENT_SINGLELINE);
		Flags.Set(FEDITLINE_PARENT_MULTILINE);
	}
}

void Edit::Changed(bool DelBlock)
{
	if (m_Callback.Active && m_Callback.m_Callback) {
		m_Callback.m_Callback(m_Callback.m_Param);
	}
}

EditControl::EditControl(ScreenObject *pOwner, Callback *aCallback, History *iHistory,
		FarList *iList, DWORD iFlags)
	:
	Edit(pOwner, aCallback),
	pCustomCompletionList(nullptr),
	pHistory(iHistory),
	pList(iList),
	Selection(false),
	SelectionStart(-1),
	ECFlags(iFlags)

{
	ACState = ECFlags.Check(EC_ENABLEAUTOCOMPLETE) != FALSE;
}

void EditControl::Show()
{
	if (X2 - X1 + 1 > StrSize()) {
		SetLeftPos(0);
	}
	Edit::Show();
}

void EditControl::Changed(bool DelBlock)
{
	if (m_Callback.Active) {
		Edit::Changed();
		AutoComplete(false, DelBlock);
	}
}

void EditControl::SetMenuPos(VMenu &menu)
{
	if (ScrY - Y1 < Min(Opt.Dialogs.CBoxMaxHeight, menu.GetItemCount()) + 2 && Y1 > ScrY / 2) {
		menu.SetPosition(X1, Max(0, Y1 - 1 - Min(Opt.Dialogs.CBoxMaxHeight, menu.GetItemCount()) - 1),
				Min(ScrX - 2, X2), Y1 - 1);
	} else {
		menu.SetPosition(X1, Y1 + 1, X2, 0);
	}
}

static void FilteredAddToMenu(VMenu &menu, const FARString &filter, const FARString &text)
{
	if (!StrCmpNI(text, filter, static_cast<int>(filter.GetLength())) && StrCmp(text, filter)) {
		menu.AddItem(text);
	}
}

void EditControl::PopulateCompletionMenu(VMenu &ComplMenu, const FARString &strFilter)
{
	SudoSilentQueryRegion ssqr;
	if (pCustomCompletionList) {
		for (const auto &possibility : *pCustomCompletionList)
			FilteredAddToMenu(ComplMenu, strFilter, FARString(possibility));

		if (ComplMenu.GetItemCount() < 10)
			ComplMenu.AssignHighlights(0);
	} else {
		if (pHistory) {
			pHistory->GetAllSimilar(ComplMenu, strFilter);
		} else if (pList) {
			for (int i = 0; i < pList->ItemsNumber; i++)
				FilteredAddToMenu(ComplMenu, strFilter, pList->Items[i].Text);
		}
		if (ECFlags.Check(EC_ENABLEFNCOMPLETE)) {
			if (!m_pSuggestor)
				m_pSuggestor.reset(new MenuFilesSuggestor);

			m_pSuggestor->Suggest(strFilter, ComplMenu, ECFlags.Check(EC_ENABLEFNCOMPLETE_ESCAPED));
		}
	}
}

void EditControl::RemoveSelectedCompletionMenuItem(VMenu &ComplMenu)
{
	int CurPos = ComplMenu.GetSelectPos();
	if (CurPos >= 0 && !pCustomCompletionList && pHistory) {
		FARString strName = ComplMenu.GetItemPtr(CurPos)->strName;
		if (pHistory->DeleteMatching(strName)) {
			ComplMenu.DeleteItem(CurPos, 1);
			ComplMenu.FastShow();
		}
	}
}

void EditControl::AutoCompleteProcMenu(int &Result, bool Manual, bool DelBlock, FarKey &BackKey)
{
	VMenu ComplMenu(nullptr, nullptr, 0, 0);
	FARString strTemp = m_Str;
	PopulateCompletionMenu(ComplMenu, strTemp);
	ComplMenu.SetBottomTitle(((!pCustomCompletionList && pHistory)
					? Msg::EditControlHistoryFooter
					: Msg::EditControlHistoryFooterNoDel));

	if (ComplMenu.GetItemCount() > 1
			|| (ComplMenu.GetItemCount() == 1 && StrCmpI(strTemp, ComplMenu.GetItemPtr(0)->strName))) {
		ComplMenu.SetFlags(VMENU_WRAPMODE | VMENU_NOTCENTER | VMENU_SHOWAMPERSAND);

		if (!DelBlock && Opt.AutoComplete.AppendCompletion
				&& (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) || Opt.AutoComplete.ShowList)) {
			int m_SelStart = GetLength();

			// magic
			if (IsSlash(m_Str[m_SelStart - 1]) && m_Str[m_SelStart - 2] == L'"'
					&& IsSlash(ComplMenu.GetItemPtr(0)->strName.At(m_SelStart - 2))) {
				m_Str.ReplaceChar(m_SelStart - 2, m_Str[m_SelStart - 1]);
				m_Str.Truncate(StrSize() - 1);
				m_SelStart--;
				CurPos--;
			}

			InsertString(ComplMenu.GetItemPtr(0)->strName.SubStr(m_SelStart));
			Select(m_SelStart, GetLength());
			Show();
		}
		if (Opt.AutoComplete.ShowList) {
			auto Area = GetOwner() == CtrlObject->Cp()
					? MACROAREA_SHELLAUTOCOMPLETION
					: MACROAREA_DIALOGAUTOCOMPLETION;
			SCOPED_ACTION(ChangeMacroArea)(Area);
			MenuItemEx EmptyItem;
			ComplMenu.AddItem(&EmptyItem, 0);
			SetMenuPos(ComplMenu);
			ComplMenu.SetSelectPos(0, 0);
			ComplMenu.SetBoxType(SHORT_SINGLE_BOX);
			ComplMenu.ClearDone();
			ComplMenu.Show();
			Show();
			int PrevPos = 0;

			while (!ComplMenu.Done()) {
				INPUT_RECORD ir;
				ComplMenu.ReadInput(&ir);
				if (!Opt.AutoComplete.ModalList) {
					int CurPos = ComplMenu.GetSelectPos();
					if (CurPos >= 0 && PrevPos != CurPos) {
						PrevPos = CurPos;
						SetString(CurPos ? ComplMenu.GetItemPtr(CurPos)->strName : strTemp);
						Show();
					}
				}
				if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT) {
					SetMenuPos(ComplMenu);
					ComplMenu.Show();
				} else if (ir.EventType == KEY_EVENT) {
					FarKey MenuKey = InputRecordToKey(&ir);

					// ввод
					if ((MenuKey >= int(L' ') && MenuKey <= MAX_VKEY_CODE) || MenuKey == KEY_BS
							|| MenuKey == KEY_DEL || MenuKey == KEY_NUMDEL) {
						FARString strPrev;
						GetString(strPrev);
						DeleteBlock();
						ProcessKey(MenuKey);
						GetString(strTemp);
						if (StrCmp(strPrev, strTemp)) {
							ComplMenu.DeleteItems();
							PrevPos = 0;
							if (!strTemp.IsEmpty()) {
								PopulateCompletionMenu(ComplMenu, strTemp);
							}
							if (ComplMenu.GetItemCount() > 1
									|| (ComplMenu.GetItemCount() == 1
											&& StrCmpI(strTemp, ComplMenu.GetItemPtr(0)->strName))) {
								if (MenuKey != KEY_BS && MenuKey != KEY_DEL && MenuKey != KEY_NUMDEL
										&& Opt.AutoComplete.AppendCompletion) {
									int m_SelStart = GetLength();

									// magic
									if (IsSlash(m_Str[m_SelStart - 1]) && m_Str[m_SelStart - 2] == L'"'
											&& IsSlash(ComplMenu.GetItemPtr(0)->strName.At(m_SelStart - 2))) {
										m_Str.ReplaceChar(m_SelStart - 2, m_Str[m_SelStart - 1]);
										m_Str.Truncate(StrSize() - 1);
										m_SelStart--;
										CurPos--;
									}

									DisableCallback DC(m_Callback.Active);
									InsertString(ComplMenu.GetItemPtr(0)->strName.SubStr(m_SelStart));
									if (X2 - X1 > GetLength())
										SetLeftPos(0);
									Select(m_SelStart, GetLength());
								}
								ComplMenu.AddItem(&EmptyItem, 0);
								SetMenuPos(ComplMenu);
								ComplMenu.SetSelectPos(0, 0);
								ComplMenu.Redraw();
							} else {
								ComplMenu.SetExitCode(-1);
							}
							Show();
						}
					} else {
						switch (MenuKey) {
							// "классический" перебор
							case KEY_CTRLEND: {
								ComplMenu.ProcessKey(KEY_DOWN);
								break;
							}

							// навигация по строке ввода
							case KEY_LEFT:
							case KEY_NUMPAD4:
							case KEY_CTRLS:
							case KEY_RIGHT:
							case KEY_NUMPAD6:
							case KEY_CTRLD:
							case KEY_CTRLLEFT:
							case KEY_CTRLRIGHT:
							case KEY_CTRLHOME: {
								if (MenuKey == KEY_LEFT || MenuKey == KEY_NUMPAD4) {
									MenuKey = KEY_CTRLS;
								} else if (MenuKey == KEY_RIGHT || MenuKey == KEY_NUMPAD6) {
									MenuKey = KEY_CTRLD;
								}
								pOwner->ProcessKey(MenuKey);
								break;
							}

							// навигация по списку
							case KEY_HOME:
							case KEY_NUMPAD7:
							case KEY_END:
							case KEY_NUMPAD1:
							case KEY_IDLE:
							case KEY_NONE:
							case KEY_ESC:
							case KEY_F10:
							case KEY_ALTF9:
							case KEY_UP:
							case KEY_NUMPAD8:
							case KEY_DOWN:
							case KEY_NUMPAD2:
							case KEY_PGUP:
							case KEY_NUMPAD9:
							case KEY_PGDN:
							case KEY_NUMPAD3:
							case KEY_ALTLEFT:
							case KEY_ALTRIGHT:
							case KEY_ALTHOME:
							case KEY_ALTEND:
							case KEY_MSWHEEL_UP:
							case KEY_MSWHEEL_DOWN:
							case KEY_MSWHEEL_LEFT:
							case KEY_MSWHEEL_RIGHT: {
								ComplMenu.ProcessInput();
								break;
							}

							case KEY_SHIFTNUMDEL:
							case KEY_SHIFTDEL: {
								RemoveSelectedCompletionMenuItem(ComplMenu);
								break;
							}

							case KEY_ENTER:
							case KEY_NUMENTER: {
								if (Opt.AutoComplete.ModalList) {
									ComplMenu.ProcessInput();
									break;
								}
							}

							// всё остальное закрывает список и идёт владельцу
							default: {
								ComplMenu.Hide();
								ComplMenu.SetExitCode(-1);
								BackKey = MenuKey;
								Result = 1;
							}
						}
					}
				} else {
					ComplMenu.ProcessInput();
				}
			}
			if (Opt.AutoComplete.ModalList) {
				int ExitCode = ComplMenu.GetExitCode();
				if (ExitCode > 0) {
					SetString(ComplMenu.GetItemPtr(ExitCode)->strName);
				}
			}
		}
	}
}

int EditControl::AutoCompleteProc(bool Manual, bool DelBlock, FarKey &BackKey)
{
	int Result = 0;
	static int Reenter = 0;

	if (ECFlags.Check(EC_ENABLEAUTOCOMPLETE) && *m_Str && !Reenter
			&& (Manual || CtrlObject->Macro.GetState() == MACROSTATE_NOMACRO)) {
		Reenter++;
		AutoCompleteProcMenu(Result, Manual, DelBlock, BackKey);
		Reenter--;
	}
	return Result;
}

void EditControl::AutoComplete(bool Manual, bool DelBlock)
{
	FarKey Key = 0;
	if (AutoCompleteProc(Manual, DelBlock, Key)) {
		// BUGBUG, hack
		int Wait = WaitInMainLoop;
		WaitInMainLoop = 1;
		if (!CtrlObject->Macro.ProcessKey(Key))
			pOwner->ProcessKey(Key);
		WaitInMainLoop = Wait;
		Show();
	}
}

int EditControl::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (Edit::ProcessMouse(MouseEvent)) {
		while (IsMouseButtonPressed() == FROM_LEFT_1ST_BUTTON_PRESSED) {
			Flags.Clear(FEDITLINE_CLEARFLAG);
			SetCellCurPos(MouseX - X1 + LeftPos);
			if (MouseEventFlags & MOUSE_MOVED) {
				if (!Selection) {
					Selection = true;
					SelectionStart = -1;
					Select(SelectionStart, 0);
				} else {
					if (SelectionStart == -1) {
						SelectionStart = CurPos;
					}
					Select(Min(SelectionStart, CurPos), Min(StrSize(), Max(SelectionStart, CurPos)));
					Show();
				}
			}
		}
		Selection = false;
		return TRUE;
	}
	return FALSE;
}

void EditControl::EnableAC(bool Permanent)
{
	ACState = Permanent || ECFlags.Check(EC_ENABLEAUTOCOMPLETE);
	ECFlags.Set(EC_ENABLEAUTOCOMPLETE);
}

void EditControl::DisableAC(bool Permanent)
{
	ACState = !Permanent && ECFlags.Check(EC_ENABLEAUTOCOMPLETE);
	ECFlags.Clear(EC_ENABLEAUTOCOMPLETE);
}

void EditControl::ShowCustomCompletionList(const std::vector<std::string> &list)
{
	pCustomCompletionList = &list;
	AutoComplete(true, false);
	pCustomCompletionList = nullptr;
}

void Edit::AutoDeleteColors()
{
	size_t Dest = 0;
	for (size_t Src = 0; Src < ColorList.size(); Src++) {
		if (!(ColorList[Src].Flags & ECF_AUTODELETE)) {
			if (Dest != Src)
				ColorList[Dest] = ColorList[Src];

			Dest++;
		}
	}
	ColorList.resize(Dest);
}
