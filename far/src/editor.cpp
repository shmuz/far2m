/*
editor.cpp

Редактор
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
#include "codepage.hpp"
#include "ctrlobj.hpp"
#include "DialogBuilder.hpp"
#include "dialog.hpp"
#include "DlgGuid.hpp"
#include "edit.hpp"
#include "editor.hpp"
#include "fileedit.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "macroopcode.hpp"
#include "message.hpp"
#include "mix.hpp"
#include "scrbuf.hpp"
#include "syslog.hpp"
#include "TPreRedrawFunc.hpp"
#include "xlat.hpp"

size_t EditorUndoData::UndoDataSize = 0;

static bool GlobalReplaceMode;

static int EditorID = 0;
std::map<int, Editor*> Editor::IdMap;

// EditorUndoData
enum
{
	UNDO_EDIT = 1,
	UNDO_INSSTR,
	UNDO_DELSTR,
	UNDO_BEGIN,
	UNDO_END
};

Editor::Editor(ScreenObject *Owner, bool DialogUsed)
	:
	m_UndoPos(m_UndoData.end()),
	m_UndoSavePos(m_UndoData.end()),
	m_UndoSkipLevel(0),
	m_LastChangeStrPos(0),
	m_NumLastLine(0),
	m_NumLine(0),
	m_Pasting(0),
	m_BlockStart(nullptr),
	m_BlockStartLine(0),
	m_VBlockStart(nullptr),
	m_MaxRightPos(0),
	m_codepage(CP_OEMCP),
	m_StartLine(-1),
	m_StartChar(-1),
	m_StackPos(0),
	m_NewStackPos(false),
	m_EditorID(EditorID++),
	m_HostFileEditor(nullptr),
	m_TopList(nullptr),
	m_EndList(nullptr),
	m_TopScreen(nullptr),
	m_CurLine(nullptr),
	m_LastGetLine(nullptr),
	m_LastGetLineNumber(0),
	m_showCursor(true)
{
	_KEYMACRO(SysLog(L"Editor::Editor()"));
	_KEYMACRO(SysLog(1));
	m_LastSearch.CaseSens = GlobalSearchCase;
	m_LastSearch.WholeWords = GlobalSearchWholeWords;
	m_LastSearch.Reverse = GlobalSearchReverse;
	m_LastSearch.SelectFound = Opt.EdOpt.SearchSelFound;
	m_LastSearch.Regexp = Opt.EdOpt.SearchRegexp;

	m_EdOpt = Opt.EdOpt;
	SetOwner(Owner);
	IdMap[m_EditorID] = this;

	if (DialogUsed)
		Flags.Set(FEDITOR_DIALOGMEMOEDIT);

	/* $ 26.10.2003 KM
	   Если установлен глобальный режим поиска 16-ричных кодов, тогда
	   сконвертируем GlobalSearchString в строку, ибо она содержит строку в
	   16-ричном представлении.
	*/
	if (GlobalSearchHex)
		Transform(m_LastSearch.SearchStr, strGlobalSearchString, L'S');
	else
		m_LastSearch.SearchStr = strGlobalSearchString;

	UnmarkMacroBlock();

	wcscpy(m_GlobalEOL, NATIVE_EOLW);
	memset(&m_SavePos, 0xff, sizeof(m_SavePos));
	InsertString(nullptr, 0);
}

Editor::~Editor()
{
	//_SVS(SysLog(L"[%p] Editor::~Editor()",this));
	FreeAllocatedData();
	KeepInitParameters();
	IdMap.erase(m_EditorID);
	_KEYMACRO(SysLog(-1));
	_KEYMACRO(SysLog(L"Editor::~Editor()"));
}

void Editor::FreeAllocatedData(bool FreeUndo)
{
	m_AutoDeletedColors.clear();
	while (m_EndList) {
		Edit *Prev = m_EndList->m_prev;
		delete m_EndList;
		m_EndList = Prev;
	}

	m_UndoData.clear();
	m_UndoSavePos = m_UndoData.end();
	m_UndoPos = m_UndoData.end();
	m_UndoSkipLevel = 0;
	ClearStackBookmarks();
	m_TopList = m_EndList = m_CurLine = nullptr;
	m_NumLastLine = 0;
	m_NumLine = 0;
}

void Editor::KeepInitParameters()
{
	// Установлен глобальный режим поиска 16-ричных данных?
	if (GlobalSearchHex)
		Transform(strGlobalSearchString, m_LastSearch.SearchStr, L'X');
	else
		strGlobalSearchString = m_LastSearch.SearchStr;

	GlobalSearchCase = m_LastSearch.CaseSens;
	GlobalSearchWholeWords = m_LastSearch.WholeWords;
	GlobalSearchReverse = m_LastSearch.Reverse;
	Opt.EdOpt.SearchSelFound = m_LastSearch.SelectFound;
	Opt.EdOpt.SearchRegexp = m_LastSearch.Regexp;
}

/*
	преобразование из буфера в список
*/
int Editor::SetRawData(const wchar_t *SrcBuf, int SizeSrcBuf, int TextFormat)
{
	FreeAllocatedData(true);

	if (!SrcBuf) {
		fprintf(stderr, "Editor::SetRawData null\n");
		InsertString(nullptr, 0);
		m_CurLine = m_TopList;
		m_TopScreen = m_TopList;
		m_NumLine = 0;
		TextChanged(1);
		return TRUE;
	}

	if (SizeSrcBuf < 0)
		SizeSrcBuf = (int)StrLength(SrcBuf);

	if (SizeSrcBuf == 0) {
		fprintf(stderr, "Editor::SetRawData empty\n");
		InsertString(nullptr, 0);
		m_CurLine = m_TopList;
		m_TopScreen = m_TopList;
		m_NumLine = 0;
		TextChanged(1);
		return TRUE;
	}

	const wchar_t *ptr = SrcBuf;
	const wchar_t *end = SrcBuf + SizeSrcBuf;
	const wchar_t *text_eol = *m_GlobalEOL ? m_GlobalEOL : NATIVE_EOLW;

	while (ptr < end) {
		const wchar_t *line_start = ptr;
		const wchar_t *eol_ptr = ptr;
		while (eol_ptr < end && *eol_ptr != L'\r' && *eol_ptr != L'\n')
			eol_ptr++;

		int line_len = (int)(eol_ptr - line_start);

		const wchar_t *eol = L"";
		int eol_len = 0;
		if (eol_ptr < end) {
			if (*eol_ptr == L'\r') {
				if (eol_ptr + 2 < end && eol_ptr[1] == L'\r' && eol_ptr[2] == L'\n') {
					eol = L"\r\r\n";
					eol_len = 3;
				} else if (eol_ptr + 1 < end && eol_ptr[1] == L'\n') {
					eol = L"\r\n";
					eol_len = 2;
				} else {
					eol = L"\r";
					eol_len = 1;
				}
			} else {
				eol = L"\n";
				eol_len = 1;
			}
		}

		Edit *line = InsertString(line_start, line_len, nullptr, -1);
		if (!line)
			return FALSE;

		if (eol_len > 0) {
			line->SetEOL(TextFormat ? text_eol : eol);
		}

		if (eol_len == 0)
			break;

		ptr = eol_ptr + eol_len;
	}

	if (!m_TopList)
		InsertString(nullptr, 0);

	m_CurLine = m_TopList;
	m_TopScreen = m_TopList;
	m_NumLine = 0;
	TextChanged(true);
	return TRUE;
}

/*
	Editor::Edit2Str - преобразование из списка в буфер с учетом EOL

		DestBuf     - куда сохраняем (выделяется динамически!)
		SizeDestBuf - размер сохранения
		TextFormat  - тип концовки строк
*/
int Editor::GetRawData(wchar_t **DestBuf, int &SizeDestBuf, int TextFormat)
{
	wchar_t *PDest = nullptr;
	SizeDestBuf = 0;	// общий размер = 0

	const wchar_t *SaveStr, *EndSeq;

	int Length;

	// посчитаем количество строк и общий размер памяти (чтобы не дергать realloc)
	Edit *CurPtr = m_TopList;

	DWORD AllLength = 0;

	while (CurPtr) {
		CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);
		AllLength+= Length + StrLength(!TextFormat ? EndSeq : m_GlobalEOL) + 1;
		CurPtr = CurPtr->m_next;
	}

	wchar_t *MemEditStr = reinterpret_cast<wchar_t *>(malloc((AllLength + 8) * sizeof(wchar_t)));

	if (MemEditStr) {
		*MemEditStr = 0;
		PDest = MemEditStr;

		// прйдемся по списку строк
		CurPtr = m_TopList;

		AllLength = 0;

		while (CurPtr) {
			CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);
			wmemcpy(PDest, SaveStr, Length);
			PDest+= Length;

			size_t LenEndSeq;
			if (!TextFormat) {
				LenEndSeq = StrLength(EndSeq);
				wmemcpy(PDest, EndSeq, LenEndSeq);
			} else {
				LenEndSeq = StrLength(m_GlobalEOL);
				wmemcpy(PDest, m_GlobalEOL, LenEndSeq);
			}

			PDest+= LenEndSeq;

			AllLength+= LenEndSeq + Length;

			CurPtr = CurPtr->m_next;
		}

		*PDest = 0;

		SizeDestBuf = (int)(PDest - MemEditStr);
		if (DestBuf)
			*DestBuf = MemEditStr;
		return TRUE;
	} else
		return FALSE;
}

void Editor::DisplayObject()
{
	ShowEditor(false);
}

void Editor::ShowEditor(bool CurLineOnly)
{
	if (Locked() || !m_TopList)
		return;

	Edit *CurPtr;
	int LeftPos, CurPos, Y;

	//_SVS(SysLog(L"Enter to ShowEditor, CurLineOnly=%i",CurLineOnly));
	/*$ 10.08.2000 skv
	  To make sure that CurEditor is set to required value.
	*/
	if (!Flags.Check(FEDITOR_DIALOGMEMOEDIT))
		CtrlObject->Plugins.CurEditor = m_HostFileEditor;    // this;

	if (m_NumLastLine > (Y2 - Y1) + 1)
		m_XX2 = X2 - (m_EdOpt.ShowScrollBar ? 1 : 0);
	else
		m_XX2 = X2;

	/* 17.04.2002 skv
	  Что б курсор не бегал при Alt-F9 в конце длинного файла.
	  Если на экране есть свободное место, и есть текст сверху,
	  перепозиционируем.
	*/

	if (!m_EdOpt.AllowEmptySpaceAfterEof) {
		while (CalcDistance(m_TopScreen, nullptr, Y2 - Y1) < Y2 - Y1) {
			if (m_TopScreen->m_prev)
				m_TopScreen = m_TopScreen->m_prev;
			else
				break;
		}
	}

	/*
	  если курсор удруг оказался "за экраном",
	  подвинем экран под курсор, а не
	  курсор загоним в экран.
	*/

	while (CalcDistance(m_TopScreen, m_CurLine, -1) >= Y2 - Y1 + 1) {
		m_TopScreen = m_TopScreen->m_next;
		// DisableOut=TRUE;
		// ProcessKey(KEY_UP);
		// DisableOut=FALSE;
	}

	CurPos = m_CurLine->GetCellCurPos();

	if (!m_EdOpt.CursorBeyondEOL) {
		m_MaxRightPos = CurPos;
		int RealCurPos = m_CurLine->GetCurPos();
		int Length = m_CurLine->GetLength();

		if (RealCurPos > Length) {
			m_CurLine->SetCurPos(Length);
			m_CurLine->SetLeftPos(0);
			CurPos = m_CurLine->GetCellCurPos();
		}
	}

	if (!m_Pasting) {
		/*$ 10.08.2000 skv
		  Don't send EE_REDRAW while macro is being executed.
		  Send EE_REDRAW with param=2 if text was just modified.
		*/
		if (!ScrBuf.GetLockCount()) {
			auto NeedRedraw = !Flags.Check(FEDITOR_DIALOGMEMOEDIT)
						|| (CtrlObject && CtrlObject->Plugins.CurDialogEditor == this);

			if (Flags.Check(FEDITOR_JUSTMODIFIED)) {
				Flags.Clear(FEDITOR_JUSTMODIFIED);

				if (NeedRedraw)
					CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW, EEREDRAW_CHANGE, this);
			}
			else {
				if (NeedRedraw)
					CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW, CurLineOnly ? EEREDRAW_LINE : EEREDRAW_ALL, this);
			}
		}
	}

	DrawScrollbar();

	if (!CurLineOnly) {
		LeftPos = m_CurLine->GetLeftPos();

		for (CurPtr = m_TopScreen, Y = Y1; Y <= Y2; Y++)
			if (CurPtr) {
				CurPtr->SetEditBeyondEnd(TRUE);
				CurPtr->SetPosition(X1, Y, m_XX2, Y);
				// CurPtr->SetTables(UseDecodeTable ? &TableSet:nullptr);
				//_D(SysLog(L"Setleftpos 3 to %i",LeftPos));
				CurPtr->SetLeftPos(LeftPos);
				if (CurPtr != m_CurLine) {
					CurPtr->SetCellCurPos(CurPos);
					CurPtr->FastShow();
				}
				CurPtr->SetEditBeyondEnd(m_EdOpt.CursorBeyondEOL);
				CurPtr = CurPtr->m_next;
			} else {
				SetScreen(X1, Y, m_XX2, Y, L' ', FarColorToReal(COL_EDITORTEXT));    // Пустые строки после конца текста
			}
	}

	m_CurLine->SetOvertypeMode(Flags.Check(FEDITOR_OVERTYPE));
	m_CurLine->SetCursorVisibleFlag(m_showCursor);
	m_CurLine->Show();

	if (m_VBlockStart && m_VBlockSizeX > 0 && m_VBlockSizeY > 0) {
		int CurScreenLine = m_NumLine - CalcDistance(m_TopScreen, m_CurLine, -1);
		LeftPos = m_CurLine->GetLeftPos();

		for (CurPtr = m_TopScreen, Y = Y1; Y <= Y2; Y++) {
			if (CurPtr) {
				if (CurScreenLine >= m_VBlockY && CurScreenLine < m_VBlockY + m_VBlockSizeY) {
					int BlockX1 = m_VBlockX - LeftPos + X1;
					int BlockX2 = m_VBlockX + m_VBlockSizeX - 1 - LeftPos + X1;

					if (BlockX1 < X1)
						BlockX1 = X1;

					if (BlockX2 > m_XX2)
						BlockX2 = m_XX2;

					if (BlockX1 <= m_XX2 && BlockX2 >= X1)
						ChangeBlockColor(BlockX1, Y, BlockX2, Y, FarColorToReal(COL_EDITORSELECTEDTEXT));
				}

				CurPtr = CurPtr->m_next;
				CurScreenLine++;
			}
		}
	}

	if (m_HostFileEditor)
		m_HostFileEditor->ShowStatus();

	//_SVS(SysLog(L"Exit from ShowEditor"));
}

/*$ 10.08.2000 skv
  Wrapper for Modified.
  Set JustModified every call to 1
  to track any text state change.
  Even if state==0, this can be
  last UNDO.
*/
void Editor::TextChanged(bool State)
{
	Flags.Change(FEDITOR_MODIFIED, State);
	Flags.Set(FEDITOR_JUSTMODIFIED);
}

bool Editor::CheckLine(Edit *line)
{
	if (line) {
		for (Edit *eLine = m_TopList; eLine; eLine = eLine->m_next) {
			if (eLine == line)
				return true;
		}
	}

	return false;
}

int Editor::BlockStart2NumLine(int *Pos)
{
	if (m_BlockStart || m_VBlockStart) {
		Edit *eBlock = m_VBlockStart ? m_VBlockStart : m_BlockStart;

		if (Pos) {
			if (m_VBlockStart)
				*Pos = eBlock->RealPosToCell(eBlock->CellPosToReal(m_VBlockX));
			else
				*Pos = eBlock->RealPosToCell(eBlock->m_SelStart);
		}

		return CalcDistance(m_TopList, eBlock, -1);
	}

	return -1;
}

int Editor::BlockEnd2NumLine(int *Pos)
{
	int iLine = -1, iPos = -1;
	Edit *eBlock = m_VBlockStart ? m_VBlockStart : m_BlockStart;

	if (eBlock) {
		int StartSel, EndSel;
		Edit *eLine = eBlock;
		iLine = BlockStart2NumLine(nullptr);    // получили строку начала блока

		if (m_VBlockStart) {
			for (int Line = m_VBlockSizeY; eLine && Line > 0; Line--, eLine = eLine->m_next) {
				iPos = eLine->RealPosToCell(eLine->CellPosToReal(m_VBlockX + m_VBlockSizeX));
				iLine++;
			}

			iLine--;
		} else {
			while (eLine)    // поиск строки, содержащую конец блока
			{
				eLine->GetSelection(StartSel, EndSel);

				if (EndSel == -1)    // это значит, что конец блока "за строкой"
					eLine->GetRealSelection(StartSel, EndSel);

				if (StartSel == -1) {
					// Если в текущей строки нет выделения, это еще не значит что мы в конце. Это может быть только начало :)
					if (eLine->m_next) {
						eLine->m_next->GetSelection(StartSel, EndSel);

						if (EndSel == -1)    // это значит, что конец блока "за строкой"
							eLine->m_next->GetRealSelection(StartSel, EndSel);

						if (StartSel == -1) {
							break;
						}
					} else
						break;
				} else {
					iPos = eLine->RealPosToCell(EndSel);
					iLine++;
				}

				eLine = eLine->m_next;
			}

			iLine--;
		}
	}

	if (Pos)
		*Pos = iPos;

	return iLine;
}

int64_t Editor::VMProcess(int OpCode, void *vParam, int64_t iParam)
{
	const int CurPos = m_CurLine->GetCurPos();

	switch (OpCode) {
		case MCODE_C_EMPTY:
			return !m_CurLine->m_next && !m_CurLine->m_prev;    //??
		case MCODE_C_EOF:
			return !m_CurLine->m_next && CurPos >= m_CurLine->GetLength();
		case MCODE_C_BOF:
			return !m_CurLine->m_prev && !CurPos;
		case MCODE_C_SELECTED:
			return m_BlockStart || m_VBlockStart ? TRUE : FALSE;
		case MCODE_V_EDITORCURPOS:
			return m_CurLine->GetCellCurPos() + 1;
		case MCODE_V_EDITORREALPOS:
			return m_CurLine->GetCurPos() + 1;
		case MCODE_V_EDITORCURLINE:
			return m_NumLine + 1;
		case MCODE_V_ITEMCOUNT:
		case MCODE_V_EDITORLINES:
			return m_NumLastLine;
			// работа со стековыми закладками
		case MCODE_F_BM_ADD:
			return AddStackBookmark(true);
		case MCODE_F_BM_CLEAR:
			return ClearStackBookmarks();
		case MCODE_F_BM_NEXT:
			return NextStackBookmark();
		case MCODE_F_BM_PREV:
			return PrevStackBookmark();
		case MCODE_F_BM_BACK:
			return BackStackBookmark();
		case MCODE_F_BM_STAT: {
			switch (iParam) {
				case 0:    // BM.Stat(0) возвращает количество
					return GetStackBookmarks(nullptr);
				case 1:    // индекс текущей закладки (0 если закладок нет)
					return CurrentStackBookmarkIdx() + 1;
			}
			return 0;
		}
		case MCODE_F_BM_PUSH:    // N=BM.push() - сохранить текущую позицию в виде закладки в конце стека
			return PushStackBookMark();
		case MCODE_F_BM_POP:     // N=BM.pop() - восстановить текущую позицию из закладки в конце стека и удалить закладку
			return PopStackBookMark();
		case MCODE_F_BM_GOTO:    // N=BM.goto([n]) - переход на закладку с указанным индексом (0 --> текущую)
			return GotoStackBookmark((int)iParam - 1);
		case MCODE_F_BM_GET:     // N=BM.Get(Idx,M) - возвращает координаты строки (M==0) или колонки (M==1) закладки с индексом (Idx=1...)
		{
			int64_t Ret = -1;
			long Val[1];
			EditorBookMarks ebm = {0};
			auto iMode = (LONG_PTR)vParam;

			switch (iMode) {
				case 0:
					ebm.Line = Val;
					break;
				case 1:
					ebm.Cursor = Val;
					break;
				case 2:
					ebm.LeftPos = Val;
					break;
				case 3:
					ebm.ScreenLine = Val;
					break;
				default:
					return Ret;
			}

			if (GetStackBookmark((int)iParam - 1, &ebm))
				Ret = Val[0] + 1;

			return Ret;
		}
		case MCODE_F_BM_DEL:    // N=BM.Del(Idx) - удаляет закладку с указанным индексом (x=1...), 0 - удаляет текущую закладку
			return DeleteStackBookmark(PointerToStackBookmark((int)iParam - 1));

		case MCODE_F_EDITOR_SEL: {
			int iLine;
			int iPos;
			INT_PTR Action = (INT_PTR)vParam;

			switch (Action) {
				case 0:    // Get Param
				{
					switch (iParam) {
						case 0:    // return FirstLine
						{
							return BlockStart2NumLine(nullptr) + 1;
						}
						case 1:    // return FirstPos
						{
							if (BlockStart2NumLine(&iPos) != -1)
								return iPos + 1;

							return 0;
						}
						case 2:    // return LastLine
						{
							return BlockEnd2NumLine(nullptr) + 1;
						}
						case 3:    // return LastPos
						{
							if (BlockEnd2NumLine(&iPos) != -1)
								return iPos + 1;

							return 0;
						}
						case 4:    // return block type (0=nothing 1=stream, 2=column)
						{
							return m_VBlockStart ? 2 : (m_BlockStart ? 1 : 0);
						}
					}

					break;
				}
				case 1:    // Set Pos
				{
					switch (iParam) {
						case 0:    // begin block (FirstLine & FirstPos)
						case 1:    // end block (LastLine & LastPos)
						{
							if (!iParam)
								iLine = BlockStart2NumLine(&iPos);
							else
								iLine = BlockEnd2NumLine(&iPos);

							if (iLine > -1 && iPos > -1) {
								GoToLine(iLine);
								m_CurLine->SetCurPos(m_CurLine->CellPosToReal(iPos));
								return 1;
							}

							return 0;
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
							m_MBlockStart = m_CurLine;
							m_MBlockStartX = m_CurLine->GetCurPos();
							return 1;
						}
						case 1:    // selection finish
						{
							int Ret = 0;

							if (CheckLine(m_MBlockStart)) {
								EditorSelect eSel;
								eSel.BlockType = (Action == 2) ? BTYPE_STREAM : BTYPE_COLUMN;
								eSel.BlockStartPos = m_MBlockStartX;
								eSel.BlockWidth = m_CurLine->GetCurPos() - m_MBlockStartX;

								if (eSel.BlockWidth || (Action == 2 && m_MBlockStart != m_CurLine)) {
									int bl = CalcDistance(m_TopList, m_MBlockStart, -1);
									int el = CalcDistance(m_TopList, m_CurLine, -1);

									if (bl > el) {
										eSel.BlockStartLine = el;
										eSel.BlockHeight = CalcDistance(m_CurLine, m_MBlockStart, -1) + 1;
									} else {
										eSel.BlockStartLine = bl;
										eSel.BlockHeight = CalcDistance(m_MBlockStart, m_CurLine, -1) + 1;
									}

									if (bl > el || (bl == el && eSel.BlockWidth < 0)) {
										eSel.BlockWidth*= -1;
										eSel.BlockStartPos = m_CurLine->GetCurPos();
									}

									Ret = EditorControl(ECTL_SELECT, &eSel);
								} else if (m_MBlockStart == m_CurLine) {
									UnmarkBlock();
								}
							}

							UnmarkMacroBlock();
							Show();
							return Ret;
						}
					}

					break;
				}
				case 4:    // UnMark sel block
				{
					bool NeedRedraw = m_BlockStart || m_VBlockStart;
					UnmarkBlock();
					UnmarkMacroBlock();

					if (NeedRedraw)
						Show();

					return 1;
				}
			}

			break;
		}
		case MCODE_V_EDITORSELVALUE:    // Editor.SelValue
		{
			FARString strText;
			wchar_t *Text;

			if (m_VBlockStart)
				Text = VBlock2Text(nullptr);
			else
				Text = Block2Text(nullptr);

			if (Text) {
				strText = Text;
				free(Text);
			}

			*(FARString *)vParam = strText;
			return 1;
		}
	}

	return 0;
}

void Editor::ProcessPasteEvent()
{
	if (Flags.Check(FEDITOR_LOCKMODE))
		return;

	m_Pasting++;
	if (!m_EdOpt.PersistentBlocks && !m_VBlockStart)
		DeleteBlock();

	Paste();
	// MarkingBlock=!m_VBlockStart;
	Flags.Change(FEDITOR_MARKINGBLOCK, !m_VBlockStart);
	Flags.Clear(FEDITOR_MARKINGVBLOCK);

	if (!m_EdOpt.PersistentBlocks)
		UnmarkBlock();

	m_Pasting--;
	Show();
}

int Editor::ProcessKey(FarKey Key)
{
	if (Key == KEY_IDLE) {
		if (Opt.ViewerEditorClock && m_HostFileEditor && m_HostFileEditor->IsFullScreen()
				&& Opt.EdOpt.ShowTitleBar)
			ShowTime(FALSE);

		return TRUE;
	}

	if (Key == KEY_NONE)
		return TRUE;

	_KEYMACRO(CleverSysLog SL(L"Editor::ProcessKey()"));
	_KEYMACRO(SysLog(L"Key=%ls", _FARKEY_ToName(Key)));
	int CurPos, CurVisPos, I;
	CurPos = m_CurLine->GetCurPos();
	CurVisPos = GetLineCurPos();
	bool isk = IsShiftKey(Key);
	_SVS(SysLog(L"[%d] isk=%d", __LINE__, isk));

	// if ((!isk || CtrlObject->Macro.IsExecuting()) && !isk && !m_Pasting)
	if (!isk && !m_Pasting
			&& !((Key >= KEY_MACRO_BASE && Key <= KEY_MACRO_ENDBASE)
					|| (Key >= KEY_OP_BASE && Key <= KEY_OP_ENDBASE))) {
		_SVS(SysLog(L"[%d] m_BlockStart=(%d,%d)", __LINE__, m_BlockStart, m_VBlockStart));

		if (m_BlockStart || m_VBlockStart) {
			TurnOffMarkingBlock();
		}

		if ((m_BlockStart || m_VBlockStart) && !m_EdOpt.PersistentBlocks)
		//    if (m_BlockStart || m_VBlockStart && !m_EdOpt.PersistentBlocks)
		{
			TurnOffMarkingBlock();

			if (!m_EdOpt.PersistentBlocks) {
				static FarKey UnmarkKeys[] = {
						KEY_LEFT,
						KEY_NUMPAD4,
						KEY_RIGHT,
						KEY_NUMPAD6,
						KEY_HOME,
						KEY_NUMPAD7,
						KEY_END,
						KEY_NUMPAD1,
						KEY_UP,
						KEY_NUMPAD8,
						KEY_DOWN,
						KEY_NUMPAD2,
						KEY_PGUP,
						KEY_NUMPAD9,
						KEY_PGDN,
						KEY_NUMPAD3,
						KEY_CTRLHOME,
						KEY_CTRLNUMPAD7,
						KEY_CTRLPGUP,
						KEY_CTRLNUMPAD9,
						KEY_CTRLEND,
						KEY_CTRLNUMPAD1,
						KEY_CTRLPGDN,
						KEY_CTRLNUMPAD3,
						KEY_CTRLLEFT,
						KEY_CTRLNUMPAD4,
						KEY_CTRLRIGHT,
						KEY_CTRLNUMPAD7,
						KEY_CTRLUP,
						KEY_CTRLNUMPAD8,
						KEY_CTRLDOWN,
						KEY_CTRLNUMPAD2,
						KEY_CTRLN,
						KEY_CTRLE,
						KEY_CTRLS,
				};

				for (size_t I = 0; I < ARRAYSIZE(UnmarkKeys); I++)
					if (Key == UnmarkKeys[I]) {
						UnmarkBlock();
						break;
					}
			} else {
				int StartSel, EndSel;
				//        Edit *BStart=!m_BlockStart?m_VBlockStart:m_BlockStart;
				//        BStart->GetRealSelection(StartSel,EndSel);
				m_BlockStart->GetRealSelection(StartSel, EndSel);
				_SVS(SysLog(L"[%d] PersistentBlocks! StartSel=%d, EndSel=%d", __LINE__, StartSel, EndSel));

				if (StartSel == -1 || StartSel == EndSel)
					UnmarkBlock();
			}
		}
	}

	if (Key == KEY_ALTD)
		Key = KEY_CTRLK;

	// работа с закладками
	if (Key >= KEY_CTRL0 && Key <= KEY_CTRL9)
		return GotoBookmark(Key - KEY_CTRL0);

	if (Key >= KEY_CTRLSHIFT0 && Key <= KEY_CTRLSHIFT9)
		Key = Key - KEY_CTRLSHIFT0 + KEY_RCTRL0;

	if (Key >= KEY_RCTRL0 && Key <= KEY_RCTRL9)
		return SetBookmark(Key - KEY_RCTRL0);

	int SelStart = 0, SelEnd = 0;
	bool SelFirst = false;
	bool SelAtBeginning = false;
	EditorBlockGuard _bg(*this, &Editor::UnmarkEmptyBlock);

	switch (Key) {
		case KEY_SHIFTLEFT:
		case KEY_SHIFTRIGHT:
		case KEY_SHIFTUP:
		case KEY_SHIFTDOWN:
		case KEY_SHIFTHOME:
		case KEY_SHIFTEND:
		case KEY_SHIFTNUMPAD4:
		case KEY_SHIFTNUMPAD6:
		case KEY_SHIFTNUMPAD8:
		case KEY_SHIFTNUMPAD2:
		case KEY_SHIFTNUMPAD7:
		case KEY_SHIFTNUMPAD1:
		case KEY_CTRLSHIFTLEFT:
		case KEY_CTRLSHIFTNUMPAD4: /* 12.11.2002 DJ */
		{
			_KEYMACRO(CleverSysLog SL(L"Editor::ProcessKey(KEY_SHIFT*)"));
			_SVS(SysLog(L"[%d] SelStart=%d, SelEnd=%d", __LINE__, SelStart, SelEnd));
			UnmarkEmptyBlock();    // уберем выделение, если его размер равен 0
			_bg.SetNeedCheckUnmark(true);
			m_CurLine->GetRealSelection(SelStart, SelEnd);

			if (Flags.Check(FEDITOR_CURPOSCHANGEDBYPLUGIN)) {
				if (SelStart != -1
						&& (CurPos < SelStart ||                  // если курсор до выделения
								(SelEnd != -1
										&& (CurPos > SelEnd ||    // ... после выделения
												(CurPos > SelStart && CurPos < SelEnd))))
						&& CurPos < m_CurLine->GetLength())         // ... внутри выдления
					TurnOffMarkingBlock();

				Flags.Clear(FEDITOR_CURPOSCHANGEDBYPLUGIN);
			}

			_SVS(SysLog(L"[%d] SelStart=%d, SelEnd=%d", __LINE__, SelStart, SelEnd));

			if (!Flags.Check(FEDITOR_MARKINGBLOCK)) {
				UnmarkBlock();
				Flags.Set(FEDITOR_MARKINGBLOCK);
				m_BlockStart = m_CurLine;
				m_BlockStartLine = m_NumLine;
				SelFirst = true;
				SelStart = SelEnd = CurPos;
			} else {
				SelAtBeginning = m_CurLine == m_BlockStart && CurPos == SelStart;

				if (SelStart == -1) {
					SelStart = SelEnd = CurPos;
				}
			}

			_SVS(SysLog(L"[%d] SelStart=%d, SelEnd=%d", __LINE__, SelStart, SelEnd));
		}
	}

	switch (Key) {
		case KEY_CTRLSHIFTPGUP:
		case KEY_CTRLSHIFTNUMPAD9:
		case KEY_CTRLSHIFTHOME:
		case KEY_CTRLSHIFTNUMPAD7: {
			Lock();
			m_Pasting++;

			while (m_CurLine != m_TopList) {
				ProcessKey(KEY_SHIFTPGUP);
			}

			if (Key == KEY_CTRLSHIFTHOME || Key == KEY_CTRLSHIFTNUMPAD7)
				ProcessKey(KEY_SHIFTHOME);

			m_Pasting--;
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_CTRLSHIFTPGDN:
		case KEY_CTRLSHIFTNUMPAD3:
		case KEY_CTRLSHIFTEND:
		case KEY_CTRLSHIFTNUMPAD1: {
			Lock();
			m_Pasting++;

			while (m_CurLine != m_EndList) {
				ProcessKey(KEY_SHIFTPGDN);
			}

			/* $ 06.02.2002 IS
			   Принудительно сбросим флаг того, что позиция изменена плагином.
			   Для чего:
				 при выполнении "ProcessKey(KEY_SHIFTPGDN)" (см. чуть выше)
				 позиция плагины (в моем случае - колорер) могут дергать
				 ECTL_SETPOSITION, в результате чего выставляется флаг
				 FEDITOR_CURPOSCHANGEDBYPLUGIN. А при обработке KEY_SHIFTEND
				 выделение в подобном случае начинается с нуля, что сводит на нет
				 предыдущее выполнение KEY_SHIFTPGDN.
			*/
			Flags.Clear(FEDITOR_CURPOSCHANGEDBYPLUGIN);

			if (Key == KEY_CTRLSHIFTEND || Key == KEY_CTRLSHIFTNUMPAD1)
				ProcessKey(KEY_SHIFTEND);

			m_Pasting--;
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_SHIFTPGUP:
		case KEY_SHIFTNUMPAD9: {
			m_Pasting++;
			Lock();

			for (I = Y1; I < Y2; I++) {
				ProcessKey(KEY_SHIFTUP);

				if (!m_EdOpt.CursorBeyondEOL) {
					if (m_CurLine->GetCurPos() > m_CurLine->GetLength()) {
						m_CurLine->SetCurPos(m_CurLine->GetLength());
					}
				}
			}

			m_Pasting--;
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_SHIFTPGDN:
		case KEY_SHIFTNUMPAD3: {
			m_Pasting++;
			Lock();

			for (I = Y1; I < Y2; I++) {
				ProcessKey(KEY_SHIFTDOWN);

				if (!m_EdOpt.CursorBeyondEOL) {
					if (m_CurLine->GetCurPos() > m_CurLine->GetLength()) {
						m_CurLine->SetCurPos(m_CurLine->GetLength());
					}
				}
			}

			m_Pasting--;
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_SHIFTHOME:
		case KEY_SHIFTNUMPAD7: {
			m_Pasting++;
			Lock();

			if (SelAtBeginning) {
				m_CurLine->Select(0, SelEnd);
			} else {
				if (!SelStart) {
					m_CurLine->Select(-1, 0);
				} else {
					m_CurLine->Select(0, SelStart);
				}
			}

			ProcessKey(KEY_HOME);
			m_Pasting--;
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_SHIFTEND:
		case KEY_SHIFTNUMPAD1: {
			{
				int LeftPos = m_CurLine->GetLeftPos();
				m_Pasting++;
				Lock();
				int CurLength = m_CurLine->GetLength();

				if (!SelAtBeginning || SelFirst) {
					m_CurLine->Select(SelStart, CurLength);
				} else {
					if (SelEnd != -1)
						m_CurLine->Select(SelEnd, CurLength);
					else
						m_CurLine->Select(CurLength, -1);
				}

				m_CurLine->ObjWidth = m_XX2 - X1;
				ProcessKey(KEY_END);
				m_Pasting--;
				Unlock();

				if (m_EdOpt.PersistentBlocks)
					Show();
				else {
					m_CurLine->FastShow();
					ShowEditor(LeftPos == m_CurLine->GetLeftPos());
				}
			}
			return TRUE;
		}
		case KEY_SHIFTLEFT:
		case KEY_SHIFTNUMPAD4: {
			_SVS(CleverSysLog SL(L"case KEY_SHIFTLEFT"));

			if (!CurPos && !m_CurLine->m_prev)
				return TRUE;

			if (!CurPos)               // курсор в начале строки
			{
				if (SelAtBeginning)    // курсор в начале блока
				{
					m_BlockStart = m_CurLine->m_prev;
					m_CurLine->m_prev->Select(m_CurLine->m_prev->GetLength(), -1);
				} else    // курсор в конце блока
				{
					m_CurLine->Select(-1, 0);
					m_CurLine->m_prev->GetRealSelection(SelStart, SelEnd);
					m_CurLine->m_prev->Select(SelStart, m_CurLine->m_prev->GetLength());
				}
			} else {
				if (SelAtBeginning || SelFirst) {
					m_CurLine->Select(m_CurLine->CalcPosBwdTo(SelStart), SelEnd);
				} else {
					m_CurLine->Select(SelStart, m_CurLine->CalcPosBwdTo(SelEnd));
				}
			}

			int LeftPos = m_CurLine->GetLeftPos();
			Edit *OldCur = m_CurLine;
			int _OldNumLine = m_NumLine;
			m_Pasting++;
			ProcessKey(KEY_LEFT);
			m_Pasting--;

			if (_OldNumLine != m_NumLine) {
				m_BlockStartLine = m_NumLine;
			}

			ShowEditor(OldCur == m_CurLine && LeftPos == m_CurLine->GetLeftPos());
			return TRUE;
		}
		case KEY_SHIFTRIGHT:
		case KEY_SHIFTNUMPAD6: {
			_SVS(CleverSysLog SL(L"case KEY_SHIFTRIGHT"));

			if (!m_CurLine->m_next && CurPos == m_CurLine->GetLength() && !m_EdOpt.CursorBeyondEOL) {
				return TRUE;
			}

			if (SelAtBeginning) {
				m_CurLine->Select(m_CurLine->CalcPosFwdTo(SelStart), SelEnd);
			} else {
				m_CurLine->Select(SelStart, m_CurLine->CalcPosFwdTo(SelEnd));
			}

			Edit *OldCur = m_CurLine;
			int OldLeft = m_CurLine->GetLeftPos();
			m_Pasting++;
			ProcessKey(KEY_RIGHT);
			m_Pasting--;

			if (OldCur != m_CurLine) {
				if (SelAtBeginning) {
					OldCur->Select(-1, 0);
					m_BlockStart = m_CurLine;
					m_BlockStartLine = m_NumLine;
				} else {
					OldCur->Select(SelStart, -1);
				}
			}

			ShowEditor(OldCur == m_CurLine && OldLeft == m_CurLine->GetLeftPos());
			return TRUE;
		}
		case KEY_CTRLSHIFTLEFT:
		case KEY_CTRLSHIFTNUMPAD4: {
			_SVS(CleverSysLog SL(L"case KEY_CTRLSHIFTLEFT"));
			_SVS(SysLog(L"[%d] m_Pasting=%d, SelEnd=%d", __LINE__, m_Pasting, SelEnd));
			{
				bool SkipSpace = true;
				m_Pasting++;
				Lock();
				int CurPos;

				for (;;) {
					const wchar_t *Str;
					int Length;
					m_CurLine->GetBinaryString(&Str, nullptr, Length);
					/* $ 12.11.2002 DJ
					   обеспечим корректную работу Ctrl-Shift-Left за концом строки
					*/
					CurPos = m_CurLine->GetCurPos();

					if (CurPos > Length) {
						int SelStartPos = CurPos;
						m_CurLine->ProcessKey(KEY_END);
						CurPos = m_CurLine->GetCurPos();

						if (m_CurLine->m_SelStart >= 0) {
							if (!SelAtBeginning)
								m_CurLine->Select(m_CurLine->m_SelStart, CurPos);
							else
								m_CurLine->Select(CurPos, m_CurLine->m_SelEnd);
						} else
							m_CurLine->Select(CurPos, SelStartPos);
					}

					if (!CurPos)
						break;

					if (IsSpace(Str[CurPos - 1]) || IsWordDiv(m_EdOpt.strWordDiv, Str[CurPos - 1])) {
						if (SkipSpace) {
							ProcessKey(KEY_SHIFTLEFT);
							continue;
						} else
							break;
					}

					SkipSpace = false;
					ProcessKey(KEY_SHIFTLEFT);
				}

				m_Pasting--;
				Unlock();
				Show();
			}
			return TRUE;
		}
		case KEY_CTRLSHIFTRIGHT:
		case KEY_CTRLSHIFTNUMPAD6: {
			_SVS(CleverSysLog SL(L"case KEY_CTRLSHIFTRIGHT"));
			_SVS(SysLog(L"[%d] m_Pasting=%d, SelEnd=%d", __LINE__, m_Pasting, SelEnd));
			{
				bool SkipSpace = true;
				m_Pasting++;
				Lock();
				int CurPos;

				for (;;) {
					const wchar_t *Str;
					int Length;
					m_CurLine->GetBinaryString(&Str, nullptr, Length);
					CurPos = m_CurLine->GetCurPos();

					if (CurPos >= Length)
						break;

					if (IsSpace(Str[CurPos]) || IsWordDiv(m_EdOpt.strWordDiv, Str[CurPos])) {
						if (SkipSpace) {
							ProcessKey(KEY_SHIFTRIGHT);
							continue;
						} else
							break;
					}

					SkipSpace = false;
					ProcessKey(KEY_SHIFTRIGHT);
				}

				m_Pasting--;
				Unlock();
				Show();
			}
			return TRUE;
		}
		case KEY_SHIFTDOWN:
		case KEY_SHIFTNUMPAD2: {
			if (!m_CurLine->m_next)
				return TRUE;

			CurPos = m_CurLine->RealPosToCell(CurPos);

			if (SelAtBeginning)    // Снимаем выделение
			{
				if (SelEnd == -1) {
					m_CurLine->Select(-1, 0);
					m_BlockStart = m_CurLine->m_next;
					m_BlockStartLine = m_NumLine + 1;
				} else {
					m_CurLine->Select(SelEnd, -1);
				}

				m_CurLine->m_next->GetRealSelection(SelStart, SelEnd);

				if (SelStart != -1)
					SelStart = m_CurLine->m_next->RealPosToCell(SelStart);

				if (SelEnd != -1)
					SelEnd = m_CurLine->m_next->RealPosToCell(SelEnd);

				if (SelStart == -1) {
					SelStart = 0;
					SelEnd = CurPos;
				} else {
					if (SelEnd != -1 && SelEnd < CurPos) {
						SelStart = SelEnd;
						SelEnd = CurPos;
					} else {
						SelStart = CurPos;
					}
				}

				if (SelStart != -1)
					SelStart = m_CurLine->m_next->CellPosToReal(SelStart);

				if (SelEnd != -1)
					SelEnd = m_CurLine->m_next->CellPosToReal(SelEnd);

				/*if(!m_EdOpt.CursorBeyondEOL && SelEnd>m_CurLine->m_next->GetLength())
				{
				  SelEnd=m_CurLine->m_next->GetLength();
				}
				if(!m_EdOpt.CursorBeyondEOL && SelStart>m_CurLine->m_next->GetLength())
				{
				  SelStart=m_CurLine->m_next->GetLength();
				}*/
			} else    // расширяем выделение
			{
				m_CurLine->Select(SelStart, -1);
				SelStart = m_CurLine->m_next->CellPosToReal(0);
				SelEnd = m_CurLine->m_next->CellPosToReal(CurPos);
			}

			if (!m_EdOpt.CursorBeyondEOL && SelEnd > m_CurLine->m_next->GetLength()) {
				SelEnd = m_CurLine->m_next->GetLength();
			}

			if (!m_EdOpt.CursorBeyondEOL && SelStart > m_CurLine->m_next->GetLength()) {
				SelStart = m_CurLine->m_next->GetLength();
			}

			//      if(!SelStart && !SelEnd)
			//        m_CurLine->m_next->Select(-1,0);
			//      else
			m_CurLine->m_next->Select(SelStart, SelEnd);
			Down();
			Show();
			return TRUE;
		}
		case KEY_SHIFTUP:
		case KEY_SHIFTNUMPAD8: {
			if (!m_CurLine->m_prev)
				return 0;

			if (SelAtBeginning || SelFirst)    // расширяем выделение
			{
				m_CurLine->Select(0, SelEnd);
				SelStart = m_CurLine->RealPosToCell(CurPos);

				if (!m_EdOpt.CursorBeyondEOL
						&& m_CurLine->m_prev->CellPosToReal(SelStart) > m_CurLine->m_prev->GetLength()) {
					SelStart = m_CurLine->m_prev->RealPosToCell(m_CurLine->m_prev->GetLength());
				}

				SelStart = m_CurLine->m_prev->CellPosToReal(SelStart);
				m_CurLine->m_prev->Select(SelStart, -1);
				m_BlockStart = m_CurLine->m_prev;
				m_BlockStartLine = m_NumLine - 1;
			} else    // снимаем выделение
			{
				CurPos = m_CurLine->RealPosToCell(CurPos);

				if (!SelStart) {
					m_CurLine->Select(-1, 0);
				} else {
					m_CurLine->Select(0, SelStart);
				}

				m_CurLine->m_prev->GetRealSelection(SelStart, SelEnd);

				if (SelStart != -1)
					SelStart = m_CurLine->m_prev->RealPosToCell(SelStart);

				if (SelStart != -1)
					SelEnd = m_CurLine->m_prev->RealPosToCell(SelEnd);

				if (SelStart == -1) {
					m_BlockStart = m_CurLine->m_prev;
					m_BlockStartLine = m_NumLine - 1;
					SelStart = m_CurLine->m_prev->CellPosToReal(CurPos);
					SelEnd = -1;
				} else {
					if (CurPos < SelStart) {
						SelEnd = SelStart;
						SelStart = CurPos;
					} else {
						SelEnd = CurPos;
					}

					SelStart = m_CurLine->m_prev->CellPosToReal(SelStart);
					SelEnd = m_CurLine->m_prev->CellPosToReal(SelEnd);

					if (!m_EdOpt.CursorBeyondEOL && SelEnd > m_CurLine->m_prev->GetLength()) {
						SelEnd = m_CurLine->m_prev->GetLength();
					}

					if (!m_EdOpt.CursorBeyondEOL && SelStart > m_CurLine->m_prev->GetLength()) {
						SelStart = m_CurLine->m_prev->GetLength();
					}
				}

				m_CurLine->m_prev->Select(SelStart, SelEnd);
			}

			Up();
			Show();
			return TRUE;
		}
		case KEY_CTRLADD: {
			Copy(true);
			return TRUE;
		}
		case KEY_CTRLA: {
			UnmarkBlock();
			SelectAll();
			return TRUE;
		}
		case KEY_CTRLU: {
			UnmarkMacroBlock();
			UnmarkBlock();
			return TRUE;
		}
		case KEY_CTRLC:
		case KEY_CTRLINS:
		case KEY_CTRLNUMPAD0: {
			if (/*!m_EdOpt.PersistentBlocks && */ !m_BlockStart && !m_VBlockStart) {
				m_BlockStart = m_CurLine;
				m_BlockStartLine = m_NumLine;
				m_CurLine->AddSelect(0, -1);
				Show();
			}

			Copy(false);
			return TRUE;
		}
		case KEY_CTRLP:
		case KEY_CTRLM: {
			if (Flags.Check(FEDITOR_LOCKMODE))
				return TRUE;

			if (m_BlockStart || m_VBlockStart) {
				int SelStart, SelEnd;
				m_CurLine->GetSelection(SelStart, SelEnd);
				m_Pasting++;
				bool OldUseInternalClipboard = Clipboard::SetUseInternalClipboardState(true);
				ProcessKey(Key == KEY_CTRLP ? KEY_CTRLINS : KEY_SHIFTDEL);

				/* $ 10.04.2001 SVS
				  ^P/^M - некорректно работали: уловие для CurPos должно быть ">=",
				   а не "меньше".
				*/
				if (Key == KEY_CTRLM && SelStart != -1 && SelEnd != -1) {
					if (CurPos >= SelEnd)
						m_CurLine->SetCurPos(CurPos - (SelEnd - SelStart));
					else
						m_CurLine->SetCurPos(CurPos);
				}

				ProcessKey(KEY_SHIFTINS);
				m_Pasting--;
				EmptyInternalClipboard();
				Clipboard::SetUseInternalClipboardState(OldUseInternalClipboard);
				/*$ 08.02.2001 SKV
				  всё делалось с pasting'ом, поэтому redraw плагинам не ушел.
				  сделаем его.
				*/
				Show();
			}

			return TRUE;
		}
		case KEY_CTRLX:
		case KEY_SHIFTDEL:
		case KEY_SHIFTNUMDEL:
		case KEY_SHIFTDECIMAL: {
			Copy(false);
			[[fallthrough]];
		}
		case KEY_CTRLD: {
			if (Flags.Check(FEDITOR_LOCKMODE))
				return TRUE;

			TurnOffMarkingBlock();
			DeleteBlock();
			Show();
			return TRUE;
		}
		case KEY_CTRLV:
		case KEY_SHIFTINS:
		case KEY_SHIFTNUMPAD0: {
			ProcessPasteEvent();
			return TRUE;
		}
		case KEY_LEFT:
		case KEY_NUMPAD4: {
			Flags.Set(FEDITOR_NEWUNDO);

			if (!CurPos && m_CurLine->m_prev) {
				Up();
				Show();
				m_CurLine->ProcessKey(KEY_END);
				Show();
			} else {
				int LeftPos = m_CurLine->GetLeftPos();
				m_CurLine->ProcessKey(KEY_LEFT);
				ShowEditor(LeftPos == m_CurLine->GetLeftPos());
			}

			return TRUE;
		}
		case KEY_INS:
		case KEY_NUMPAD0: {
			Flags.Swap(FEDITOR_OVERTYPE);
			Show();
			return TRUE;
		}
		case KEY_NUMDEL:
		case KEY_DEL: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				// Del в самой последней позиции ничего не удаляет, поэтому не модифицируем...
				if (!m_CurLine->m_next && CurPos >= m_CurLine->GetLength() && !m_BlockStart && !m_VBlockStart)
					return TRUE;

				/* $ 07.03.2002 IS
				   Снимем выделение, если блок все равно пустой
				*/
				if (!m_Pasting)
					UnmarkEmptyBlock();

				if (!m_Pasting && m_EdOpt.DelRemovesBlocks && (m_BlockStart || m_VBlockStart))
					DeleteBlock();
				else {
					if (CurPos >= m_CurLine->GetLength()) {
						AddUndoData(UNDO_BEGIN);
						AddUndoData(m_CurLine, m_NumLine);

						if (!m_CurLine->m_next)
							m_CurLine->SetEOL(L"");
						else {
							int SelStart, SelEnd, NextSelStart, NextSelEnd;
							int Length = m_CurLine->GetLength();
							m_CurLine->GetSelection(SelStart, SelEnd);
							m_CurLine->m_next->GetSelection(NextSelStart, NextSelEnd);
							const wchar_t *Str;
							int NextLength;
							m_CurLine->m_next->GetBinaryString(&Str, nullptr, NextLength);
							m_CurLine->InsertBinaryString(Str, NextLength);
							m_CurLine->SetEOL(m_CurLine->m_next->GetEOL());
							m_CurLine->SetCurPos(CurPos);
							DeleteString(m_CurLine->m_next, m_NumLine + 1, true, m_NumLine + 1);

							if (!NextLength)
								m_CurLine->SetEOL(L"");

							if (NextSelStart != -1) {
								if (SelStart == -1) {
									m_CurLine->Select(Length + NextSelStart,
											NextSelEnd == -1 ? -1 : Length + NextSelEnd);
									m_BlockStart = m_CurLine;
									m_BlockStartLine = m_NumLine;
								} else
									m_CurLine->Select(SelStart, NextSelEnd == -1 ? -1 : Length + NextSelEnd);
							}
						}

						AddUndoData(UNDO_END);
					} else {
						AddUndoData(m_CurLine, m_NumLine);
						m_CurLine->ProcessKey(KEY_DEL);
					}

					TextChanged(true);
				}

				Show();
			}

			return TRUE;
		}
		case KEY_BS: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				// Bs в самом начале нихрена ничего не удаляет, посему не будем выставлять
				if (!m_CurLine->m_prev && !CurPos && !m_BlockStart && !m_VBlockStart)
					return TRUE;

				TextChanged(true);
				bool IsDelBlock = false;

				if (m_EdOpt.BSLikeDel) {
					if (!m_Pasting && m_EdOpt.DelRemovesBlocks
							&& (m_BlockStart || (m_VBlockStart && /* #279 */ m_VBlockSizeX > 0 && m_VBlockSizeY > 0)))
						IsDelBlock = true;
				} else {
					if (!m_Pasting && !m_EdOpt.PersistentBlocks && m_BlockStart)
						IsDelBlock = true;
				}

				if (IsDelBlock)
					DeleteBlock();
				else if (!CurPos && m_CurLine->m_prev) {
					m_Pasting++;
					Up();
					m_CurLine->ProcessKey(KEY_CTRLEND);
					ProcessKey(KEY_DEL);
					m_Pasting--;
				} else {
					AddUndoData(m_CurLine, m_NumLine);
					m_CurLine->ProcessKey(KEY_BS);
				}

				Show();
			}

			return TRUE;
		}
		case KEY_CTRLBS: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				TextChanged(true);

				if (!m_Pasting && !m_EdOpt.PersistentBlocks && m_BlockStart)
					DeleteBlock();
				else if (!CurPos && m_CurLine->m_prev)
					ProcessKey(KEY_BS);
				else {
					AddUndoData(m_CurLine, m_NumLine);
					m_CurLine->ProcessKey(KEY_CTRLBS);
				}

				Show();
			}

			return TRUE;
		}
		case KEY_UP:
		case KEY_NUMPAD8: {
			{
				Flags.Set(FEDITOR_NEWUNDO);
				int PrevMaxPos = m_MaxRightPos;
				Edit *LastTopScreen = m_TopScreen;
				Up();

				if (m_TopScreen == LastTopScreen)
					ShowEditor(true);
				else
					Show();

				if (PrevMaxPos > m_CurLine->GetCellCurPos()) {
					m_CurLine->SetCellCurPos(PrevMaxPos);
					m_CurLine->FastShow();
					m_CurLine->SetCellCurPos(PrevMaxPos);
					Show();
				}
			}
			return TRUE;
		}
		case KEY_DOWN:
		case KEY_NUMPAD2: {
			{
				Flags.Set(FEDITOR_NEWUNDO);
				int PrevMaxPos = m_MaxRightPos;
				Edit *LastTopScreen = m_TopScreen;
				Down();

				if (m_TopScreen == LastTopScreen)
					ShowEditor(true);
				else
					Show();

				if (PrevMaxPos > m_CurLine->GetCellCurPos()) {
					m_CurLine->SetCellCurPos(PrevMaxPos);
					m_CurLine->FastShow();
					m_CurLine->SetCellCurPos(PrevMaxPos);
					Show();
				}
			}
			return TRUE;
		}
		case KEY_MSWHEEL_UP:
		case (KEY_MSWHEEL_UP | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsWheelDeltaEdit;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_CTRLUP);

			return TRUE;
		}
		case KEY_MSWHEEL_DOWN:
		case (KEY_MSWHEEL_DOWN | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsWheelDeltaEdit;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_CTRLDOWN);

			return TRUE;
		}
		case KEY_MSWHEEL_LEFT:
		case (KEY_MSWHEEL_LEFT | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsHWheelDeltaEdit;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_LEFT);

			return TRUE;
		}
		case KEY_MSWHEEL_RIGHT:
		case (KEY_MSWHEEL_RIGHT | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsHWheelDeltaEdit;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_RIGHT);

			return TRUE;
		}
		case KEY_CTRLUP:
		case KEY_CTRLNUMPAD8: {
			Flags.Set(FEDITOR_NEWUNDO);
			ScrollUp();
			Show();
			return TRUE;
		}
		case KEY_CTRLDOWN:
		case KEY_CTRLNUMPAD2: {
			Flags.Set(FEDITOR_NEWUNDO);
			ScrollDown();
			Show();
			return TRUE;
		}
		case KEY_PGUP:
		case KEY_NUMPAD9: {
			Flags.Set(FEDITOR_NEWUNDO);

			for (I = Y1; I < Y2; I++)
				ScrollUp();

			Show();
			return TRUE;
		}
		case KEY_PGDN:
		case KEY_NUMPAD3: {
			Flags.Set(FEDITOR_NEWUNDO);

			for (I = Y1; I < Y2; I++)
				ScrollDown();

			Show();
			return TRUE;
		}
		case KEY_CTRLHOME:
		case KEY_CTRLNUMPAD7:
		case KEY_CTRLPGUP:
		case KEY_CTRLNUMPAD9: {
			{
				Flags.Set(FEDITOR_NEWUNDO);
				int StartPos = m_CurLine->GetCellCurPos();
				m_NumLine = 0;
				m_TopScreen = m_CurLine = m_TopList;

				if (Key == KEY_CTRLHOME || Key == KEY_CTRLNUMPAD7)
					m_CurLine->SetCurPos(0);
				else
					m_CurLine->SetCellCurPos(StartPos);

				Show();
			}
			return TRUE;
		}
		case KEY_CTRLEND:
		case KEY_CTRLNUMPAD1:
		case KEY_CTRLPGDN:
		case KEY_CTRLNUMPAD3: {
			{
				Flags.Set(FEDITOR_NEWUNDO);
				int StartPos = m_CurLine->GetCellCurPos();
				m_NumLine = m_NumLastLine - 1;
				m_CurLine = m_EndList;

				for (m_TopScreen = m_CurLine, I = Y1; I < Y2 && m_TopScreen->m_prev; I++) {
					m_TopScreen->SetPosition(X1, I, m_XX2, I);
					m_TopScreen = m_TopScreen->m_prev;
				}

				m_CurLine->SetLeftPos(0);

				if (Key == KEY_CTRLEND || Key == KEY_CTRLNUMPAD1) {
					m_CurLine->SetCurPos(m_CurLine->GetLength());
					m_CurLine->FastShow();
				} else
					m_CurLine->SetCellCurPos(StartPos);

				Show();
			}
			return TRUE;
		}
		case KEY_NUMENTER:
		case KEY_ENTER: {
			if (m_Pasting || !ShiftPressed || CtrlObject->Macro.IsExecuting()) {
				if (!m_Pasting && !m_EdOpt.PersistentBlocks && m_BlockStart)
					DeleteBlock();

				Flags.Set(FEDITOR_NEWUNDO);
				InsertString();
				m_CurLine->FastShow();
				Show();
			}

			return TRUE;
		}
		case KEY_CTRLN: {
			Flags.Set(FEDITOR_NEWUNDO);

			while (m_CurLine != m_TopScreen) {
				m_CurLine = m_CurLine->m_prev;
				m_NumLine--;
			}

			m_CurLine->SetCurPos(CurPos);
			Show();
			return TRUE;
		}
		case KEY_CTRLE: {
			{
				Flags.Set(FEDITOR_NEWUNDO);
				Edit *CurPtr = m_TopScreen;
				bool CurLineFound = false;

				for (I = Y1; I < Y2; I++) {
					if (!CurPtr->m_next)
						break;

					if (CurPtr == m_CurLine)
						CurLineFound = true;

					if (CurLineFound)
						m_NumLine++;

					CurPtr = CurPtr->m_next;
				}

				m_CurLine = CurPtr;
				m_CurLine->SetCurPos(CurPos);
				Show();
			}
			return TRUE;
		}
		case KEY_CTRLL: {
			Flags.Swap(FEDITOR_LOCKMODE);

			if (m_HostFileEditor)
				m_HostFileEditor->ShowStatus();

			return TRUE;
		}
		case KEY_CTRLY: {
			DeleteString(m_CurLine, m_NumLine, false, m_NumLine);
			Show();
			return TRUE;
		}
		case KEY_F7: {
			if (Search(false, NEXT_NONE))
				GlobalReplaceMode = false;

			return TRUE;
		}
		case KEY_CTRLF7: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				if (Search(true, NEXT_NONE))
					GlobalReplaceMode = true;
			}

			return TRUE;
		}
		case KEY_SHIFTF7: {
			TurnOffMarkingBlock();
			Search(GlobalReplaceMode, m_LastSearch.Reverse ? NEXT_REVERSE : NEXT_FORWARD);
			return TRUE;
		}
		case KEY_ALTF7: {
			TurnOffMarkingBlock();
			Search(GlobalReplaceMode, m_LastSearch.Reverse ? NEXT_FORWARD : NEXT_REVERSE);
			return TRUE;
		}
		case KEY_F11: {
			return TRUE;
		}
		case KEY_CTRLSHIFTZ:
		case KEY_ALTBS:
		case KEY_CTRLZ: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				Lock();
				Undo(Key == KEY_CTRLSHIFTZ);
				Flags.Set(FEDITOR_NEWUNDO);
				Unlock();
				Show();
			}

			return TRUE;
		}
		case KEY_ALTF8: {
			{
				GoToPosition();

				// <GOTO_UNMARK:1>
				if (!m_EdOpt.PersistentBlocks)
					UnmarkBlock();

				// </GOTO_UNMARK>
				Show();
			}
			return TRUE;
		}
		case KEY_ALTU: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				BlockLeft();
				Show();
			}

			return TRUE;
		}
		case KEY_ALTI: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				BlockRight();
				Show();
			}

			return TRUE;
		}
		case KEY_ALTSHIFTLEFT:
		case KEY_ALTSHIFTNUMPAD4:
		case KEY_ALTLEFT: {
			if (!CurPos)
				return TRUE;

			if (!Flags.Check(FEDITOR_MARKINGVBLOCK))
				BeginVBlockMarking();

			m_Pasting++;
			{
				int Delta = m_CurLine->GetCellCurPos() - m_CurLine->RealPosToCell(CurPos - 1);

				if (m_CurLine->GetCellCurPos() > m_VBlockX)
					m_VBlockSizeX-= Delta;
				else {
					m_VBlockX-= Delta;
					m_VBlockSizeX+= Delta;
				}

				/* $ 25.07.2000 tran
				   остатки бага 22 - подправка при перебега за границу блока */
				if (m_VBlockSizeX < 0) {
					m_VBlockSizeX = -m_VBlockSizeX;
					m_VBlockX-= m_VBlockSizeX;
				}

				ProcessKey(KEY_LEFT);
			}
			m_Pasting--;
			Show();
			//_D(SysLog(L"m_VBlockX=%i, m_VBlockSizeX=%i, GetLineCurPos=%i",m_VBlockX,m_VBlockSizeX,GetLineCurPos()));
			//_D(SysLog(L"~~~~~~~~~~~~~~~~ KEY_ALTLEFT END, m_VBlockY=%i:%i, m_VBlockX=%i:%i",m_VBlockY,m_VBlockSizeY,m_VBlockX,m_VBlockSizeX));
			return TRUE;
		}
		case KEY_ALTSHIFTRIGHT:
		case KEY_ALTSHIFTNUMPAD6:
		case KEY_ALTRIGHT: {
			/* $ 23.10.2000 tran
			   вместо GetCellCurPos надо вызывать GetCurPos -
			   сравнивать реальную позицию с реальной длиной
			   а было сравнение видимой позицией с реальной длиной*/
			if (!m_EdOpt.CursorBeyondEOL && m_CurLine->GetCurPos() >= m_CurLine->GetLength())
				return TRUE;

			if (!Flags.Check(FEDITOR_MARKINGVBLOCK))
				BeginVBlockMarking();

			//_D(SysLog(L"---------------- KEY_ALTRIGHT, getLineCurPos=%i",GetLineCurPos()));
			m_Pasting++;
			{
				int Delta;
				/* $ 18.07.2000 tran
					 встань в начало текста, нажми alt-right, alt-pagedown,
					 выделится блок шириной в 1 колонку, нажми еще alt-right
					 выделение сбросится
				*/
				int VisPos = m_CurLine->RealPosToCell(CurPos), NextVisPos = m_CurLine->RealPosToCell(CurPos + 1);
				//_D(SysLog(L"CurPos=%i, VisPos=%i, NextVisPos=%i",
				//    CurPos,VisPos, NextVisPos); //,m_CurLine->GetCellCurPos()));
				Delta = NextVisPos - VisPos;
				//_D(SysLog(L"Delta=%i",Delta));

				if (m_CurLine->GetCellCurPos() >= m_VBlockX + m_VBlockSizeX)
					m_VBlockSizeX+= Delta;
				else {
					m_VBlockX+= Delta;
					m_VBlockSizeX-= Delta;
				}

				/* $ 25.07.2000 tran
				   остатки бага 22 - подправка при перебега за границу блока */
				if (m_VBlockSizeX < 0) {
					m_VBlockSizeX = -m_VBlockSizeX;
					m_VBlockX-= m_VBlockSizeX;
				}

				ProcessKey(KEY_RIGHT);
				//_D(SysLog(L"m_VBlockX=%i, m_VBlockSizeX=%i, GetLineCurPos=%i",m_VBlockX,m_VBlockSizeX,GetLineCurPos()));
			}
			m_Pasting--;
			Show();
			//_D(SysLog(L"~~~~~~~~~~~~~~~~ KEY_ALTRIGHT END, m_VBlockY=%i:%i, m_VBlockX=%i:%i",m_VBlockY,m_VBlockSizeY,m_VBlockX,m_VBlockSizeX));
			return TRUE;
		}
		/* $ 29.06.2000 IG
		  + CtrlAltLeft, CtrlAltRight для вертикальный блоков
		*/
		case KEY_CTRLALTLEFT:
		case KEY_CTRLALTNUMPAD4: {
			{
				bool SkipSpace = true;
				m_Pasting++;
				Lock();

				for (;;) {
					const wchar_t *Str;
					int Length;
					m_CurLine->GetBinaryString(&Str, nullptr, Length);
					int CurPos = m_CurLine->GetCurPos();

					if (CurPos > Length) {
						m_CurLine->ProcessKey(KEY_END);
						CurPos = m_CurLine->GetCurPos();
					}

					if (!CurPos)
						break;

					if (IsSpace(Str[CurPos - 1]) || IsWordDiv(m_EdOpt.strWordDiv, Str[CurPos - 1])) {
						if (SkipSpace) {
							ProcessKey(KEY_ALTSHIFTLEFT);
							continue;
						} else
							break;
					}

					SkipSpace = false;
					ProcessKey(KEY_ALTSHIFTLEFT);
				}

				m_Pasting--;
				Unlock();
				Show();
			}
			return TRUE;
		}
		case KEY_CTRLALTRIGHT:
		case KEY_CTRLALTNUMPAD6: {
			{
				bool SkipSpace = true;
				m_Pasting++;
				Lock();

				for (;;) {
					const wchar_t *Str;
					int Length;
					m_CurLine->GetBinaryString(&Str, nullptr, Length);
					int CurPos = m_CurLine->GetCurPos();

					if (CurPos >= Length)
						break;

					if (IsSpace(Str[CurPos]) || IsWordDiv(m_EdOpt.strWordDiv, Str[CurPos])) {
						if (SkipSpace) {
							ProcessKey(KEY_ALTSHIFTRIGHT);
							continue;
						} else
							break;
					}

					SkipSpace = false;
					ProcessKey(KEY_ALTSHIFTRIGHT);
				}

				m_Pasting--;
				Unlock();
				Show();
			}
			return TRUE;
		}
		case KEY_ALTSHIFTUP:
		case KEY_ALTSHIFTNUMPAD8:
		case KEY_ALTUP: {
			if (!m_CurLine->m_prev)
				return TRUE;

			if (!Flags.Check(FEDITOR_MARKINGVBLOCK))
				BeginVBlockMarking();

			if (!m_EdOpt.CursorBeyondEOL
					&& m_VBlockX >= m_CurLine->m_prev->RealPosToCell(m_CurLine->m_prev->GetLength()))
				return TRUE;

			m_Pasting++;

			if (m_NumLine > m_VBlockY)
				m_VBlockSizeY--;
			else {
				m_VBlockY--;
				m_VBlockSizeY++;
				m_VBlockStart = m_VBlockStart->m_prev;
				m_BlockStartLine--;
			}

			ProcessKey(KEY_UP);
			AdjustVBlock(CurVisPos);
			m_Pasting--;
			Show();
			//_D(SysLog(L"~~~~~~~~ ALT_PGUP, m_VBlockY=%i:%i, m_VBlockX=%i:%i",m_VBlockY,m_VBlockSizeY,m_VBlockX,m_VBlockSizeX));
			return TRUE;
		}
		case KEY_ALTSHIFTDOWN:
		case KEY_ALTSHIFTNUMPAD2:
		case KEY_ALTDOWN: {
			if (!m_CurLine->m_next)
				return TRUE;

			if (!Flags.Check(FEDITOR_MARKINGVBLOCK))
				BeginVBlockMarking();

			if (!m_EdOpt.CursorBeyondEOL
					&& m_VBlockX >= m_CurLine->m_next->RealPosToCell(m_CurLine->m_next->GetLength()))
				return TRUE;

			m_Pasting++;

			if (m_NumLine >= m_VBlockY + m_VBlockSizeY - 1)
				m_VBlockSizeY++;
			else {
				m_VBlockY++;
				m_VBlockSizeY--;
				m_VBlockStart = m_VBlockStart->m_next;
				m_BlockStartLine++;
			}

			ProcessKey(KEY_DOWN);
			AdjustVBlock(CurVisPos);
			m_Pasting--;
			Show();
			//_D(SysLog(L"~~~~ Key_AltDOWN: m_VBlockY=%i:%i, m_VBlockX=%i:%i",m_VBlockY,m_VBlockSizeY,m_VBlockX,m_VBlockSizeX));
			return TRUE;
		}
		case KEY_ALTSHIFTHOME:
		case KEY_ALTSHIFTNUMPAD7:
		case KEY_ALTHOME: {
			m_Pasting++;
			Lock();

			while (m_CurLine->GetCurPos() > 0)
				ProcessKey(KEY_ALTSHIFTLEFT);

			Unlock();
			m_Pasting--;
			Show();
			return TRUE;
		}
		case KEY_ALTSHIFTEND:
		case KEY_ALTSHIFTNUMPAD1:
		case KEY_ALTEND: {
			m_Pasting++;
			Lock();

			if (m_CurLine->GetCurPos() < m_CurLine->GetLength())
				while (m_CurLine->GetCurPos() < m_CurLine->GetLength())
					ProcessKey(KEY_ALTSHIFTRIGHT);

			if (m_CurLine->GetCurPos() > m_CurLine->GetLength())
				while (m_CurLine->GetCurPos() > m_CurLine->GetLength())
					ProcessKey(KEY_ALTSHIFTLEFT);

			Unlock();
			m_Pasting--;
			Show();
			return TRUE;
		}
		case KEY_ALTSHIFTPGUP:
		case KEY_ALTSHIFTNUMPAD9:
		case KEY_ALTPGUP: {
			m_Pasting++;
			Lock();

			for (I = Y1; I < Y2; I++)
				ProcessKey(KEY_ALTSHIFTUP);

			Unlock();
			m_Pasting--;
			Show();
			return TRUE;
		}
		case KEY_ALTSHIFTPGDN:
		case KEY_ALTSHIFTNUMPAD3:
		case KEY_ALTPGDN: {
			m_Pasting++;
			Lock();

			for (I = Y1; I < Y2; I++)
				ProcessKey(KEY_ALTSHIFTDOWN);

			Unlock();
			m_Pasting--;
			Show();
			return TRUE;
		}
		case KEY_CTRLALTPGUP:
		case KEY_CTRLALTNUMPAD9:
		case KEY_CTRLALTHOME:
		case KEY_CTRLALTNUMPAD7: {
			Lock();
			m_Pasting++;

			Edit *PrevLine = nullptr;
			while (m_CurLine != m_TopList && PrevLine != m_CurLine) {
				PrevLine = m_CurLine;
				ProcessKey(KEY_ALTUP);
			}

			m_Pasting--;
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_CTRLALTPGDN:
		case KEY_CTRLALTNUMPAD3:
		case KEY_CTRLALTEND:
		case KEY_CTRLALTNUMPAD1: {
			Lock();
			m_Pasting++;

			Edit *PrevLine = nullptr;
			while (m_CurLine != m_EndList && PrevLine != m_CurLine) {
				PrevLine = m_CurLine;
				ProcessKey(KEY_ALTDOWN);
			}

			m_Pasting--;
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_CTRLALTBRACKET:          // Вставить реальный (разрешенный) путь из левой панели
		case KEY_CTRLALTBACKBRACKET:      // Вставить реальный (разрешенный) путь из правой панели
		case KEY_ALTSHIFTBRACKET:         // Вставить реальный (разрешенный) путь из активной панели
		case KEY_ALTSHIFTBACKBRACKET:     // Вставить реальный (разрешенный) путь из пассивной панели
		case KEY_CTRLBRACKET:             // Вставить путь из левой панели
		case KEY_CTRLBACKBRACKET:         // Вставить путь из правой панели
		case KEY_CTRLSHIFTBRACKET:        // Вставить путь из активной панели
		case KEY_CTRLSHIFTBACKBRACKET:    // Вставить путь из пассивной панели
		case KEY_CTRLSHIFTNUMENTER:
		case KEY_SHIFTNUMENTER:
		case KEY_CTRLSHIFTENTER:
		case KEY_SHIFTENTER: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				m_Pasting++;
				AddUndoData(UNDO_BEGIN);
				TextChanged(true);

				if (!m_EdOpt.PersistentBlocks && m_BlockStart) {
					TurnOffMarkingBlock();
					DeleteBlock();
				}

				AddUndoData(m_CurLine, m_NumLine);
				m_CurLine->ProcessKey(Key);
				m_Pasting--;
				AddUndoData(UNDO_END);
				Show();
			}

			return TRUE;
		}
		case KEY_CTRLQ: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				Flags.Set(FEDITOR_PROCESSCTRLQ);

				if (m_HostFileEditor)
					m_HostFileEditor->ShowStatus();

				m_Pasting++;
				TextChanged(true);

				if (!m_EdOpt.PersistentBlocks && m_BlockStart) {
					TurnOffMarkingBlock();
					DeleteBlock();
				}

				AddUndoData(m_CurLine, m_NumLine);
				m_CurLine->ProcessCtrlQ();
				Flags.Clear(FEDITOR_PROCESSCTRLQ);
				m_Pasting--;
				Show();
			}

			return TRUE;
		}
		case KEY_OP_SELWORD: {
			int OldCurPos = CurPos;
			int SStart, SEnd;
			m_Pasting++;
			Lock();
			UnmarkBlock();

			// m_CurLine->TableSet ??? => UseDecodeTable?m_CurLine->TableSet:nullptr !!!
			if (CalcWordFromString(m_CurLine->GetStringAddr(), CurPos, &SStart, &SEnd, m_EdOpt.strWordDiv)) {
				m_CurLine->Select(SStart, SEnd + (SEnd < m_CurLine->StrSize() ? 1 : 0));

				if (m_CurLine->IsSelection()) {
					Flags.Set(FEDITOR_MARKINGBLOCK);
					m_BlockStart = m_CurLine;
					m_BlockStartLine = m_NumLine;
					// SelFirst=true;
					SelStart = SStart;
					SelEnd = SEnd;
					// m_CurLine->ProcessKey(MCODE_OP_SELWORD);
				}
			}

			CurPos = OldCurPos;    // возвращаем обратно
			m_Pasting--;
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_OP_PLAINTEXT: {
			if (!Flags.Check(FEDITOR_LOCKMODE)) {
				FARString strTStr = CtrlObject->Macro.GetStringToPrint();
				if (strTStr.IsEmpty())
					return TRUE;

				for (wchar_t *Ptr = strTStr.GetBuffer(); *Ptr; ++Ptr) {
					if (*Ptr == L'\n') // заменим L'\n' на L'\r' по правилам Paset ;-)
						*Ptr = L'\r';
				}
				strTStr.ReleaseBuffer();

				m_Pasting++;
				//_SVS(SysLogDump(Fmt,0,TStr,strlen(TStr),nullptr));
				TextChanged(true);

				if (!m_EdOpt.PersistentBlocks && (m_VBlockStart || m_BlockStart)) {
					TurnOffMarkingBlock();
					DeleteBlock();
				}

				// AddUndoData(m_CurLine, m_NumLine);
				Paste(strTStr);
				// if (!m_EdOpt.PersistentBlocks && IsBlock)
				UnmarkBlock();
				m_Pasting--;
				Show();
			}

			return TRUE;
		}
		default: {
			{
				if ((Key == KEY_CTRLDEL || Key == KEY_CTRLNUMDEL || Key == KEY_CTRLDECIMAL
							|| Key == KEY_CTRLT)
						&& CurPos >= m_CurLine->GetLength()) {
					/*$ 08.12.2000 skv
					  - CTRL-DEL в начале строки при выделенном блоке и
						включенном EditorDelRemovesBlocks
					*/
					int save = m_EdOpt.DelRemovesBlocks;
					m_EdOpt.DelRemovesBlocks = 0;
					int ret = ProcessKey(KEY_DEL);
					m_EdOpt.DelRemovesBlocks = save;
					return ret;
				}

				if (!m_Pasting && !m_EdOpt.PersistentBlocks && m_BlockStart && IsCharKey(Key)) {
					DeleteBlock();
					/* $ 19.09.2002 SKV
					  Однако надо.
					  Иначе есди при надичии выделения набирать
					  текст с шифтом флаги не сбросятся и следующий
					  выделенный блок будет глючный.
					*/
					TurnOffMarkingBlock();
					Show();
				}

				int SkipCheckUndo = (Key == KEY_RIGHT || Key == KEY_NUMPAD6 || Key == KEY_CTRLLEFT
						|| Key == KEY_CTRLNUMPAD4 || Key == KEY_CTRLRIGHT || Key == KEY_CTRLNUMPAD6
						|| Key == KEY_HOME || Key == KEY_NUMPAD7 || Key == KEY_END || Key == KEY_NUMPAD1
						|| Key == KEY_CTRLS);

				if (Flags.Check(FEDITOR_LOCKMODE) && !SkipCheckUndo)
					return TRUE;

				if ((Key == KEY_CTRLLEFT || Key == KEY_CTRLNUMPAD4) && !m_CurLine->GetCurPos()) {
					m_Pasting++;
					ProcessKey(KEY_LEFT);
					m_Pasting--;
					/* $ 24.9.2001 SKV
					  fix бага с ctrl-left в начале строки
					  в блоке с переопределённым плагином фоном.
					*/
					ShowEditor(false);
					// if(!Flags.Check(FEDITOR_DIALOGMEMOEDIT)){
					// CtrlObject->Plugins.CurEditor=m_HostFileEditor; // this;
					//_D(SysLog(L"%08d EE_REDRAW",__LINE__));
					// CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW,EEREDRAW_ALL);
					// }
					return TRUE;
				}

				if (((!m_EdOpt.CursorBeyondEOL && (Key == KEY_RIGHT || Key == KEY_NUMPAD6))
							|| Key == KEY_CTRLRIGHT || Key == KEY_CTRLNUMPAD6)
						&& m_CurLine->GetCurPos() >= m_CurLine->GetLength() && m_CurLine->m_next) {
					m_Pasting++;
					ProcessKey(KEY_HOME);
					ProcessKey(KEY_DOWN);
					m_Pasting--;

					if (!Flags.Check(FEDITOR_DIALOGMEMOEDIT)) {
						CtrlObject->Plugins.CurEditor = m_HostFileEditor;    // this;
						CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW, EEREDRAW_ALL, this);
					}

					/*$ 03.02.2001 SKV
					  А то EEREDRAW_ALL то уходит, а на самом деле
					  только текущая линия перерисовывается.
					*/
					ShowEditor(false);
					return TRUE;
				}

				const wchar_t *Str;

				wchar_t *CmpStr = nullptr;

				int Length, CurPos;

				m_CurLine->GetBinaryString(&Str, nullptr, Length);

				CurPos = m_CurLine->GetCurPos();

				if (IsCharKey(Key) && CurPos > 0 && !Length) {
					Edit *PrevLine = m_CurLine->m_prev;

					while (PrevLine && !PrevLine->GetLength())
						PrevLine = PrevLine->m_prev;

					if (PrevLine) {
						int TabPos = m_CurLine->GetCellCurPos();
						m_CurLine->SetCurPos(0);
						const wchar_t *PrevStr = nullptr;
						int PrevLength = 0;
						PrevLine->GetBinaryString(&PrevStr, nullptr, PrevLength);

						for (int I = 0; I < PrevLength && IsSpace(PrevStr[I]); I++) {
							int NewTabPos = m_CurLine->GetCellCurPos();

							if (NewTabPos == TabPos)
								break;

							if (NewTabPos > TabPos) {
								m_CurLine->ProcessKey(KEY_BS);

								while (m_CurLine->GetCellCurPos() < TabPos)
									m_CurLine->ProcessKey(' ');

								break;
							}

							if (NewTabPos < TabPos)
								m_CurLine->ProcessKey(PrevStr[I]);
						}

						m_CurLine->SetCellCurPos(TabPos);
					}
				}

				if (!SkipCheckUndo) {
					m_CurLine->GetBinaryString(&Str, nullptr, Length);
					CurPos = m_CurLine->GetCurPos();
					CmpStr = new wchar_t[Length + 1];
					wmemcpy(CmpStr, Str, Length);
					CmpStr[Length] = 0;
				}

				int LeftPos = m_CurLine->GetLeftPos();

				if (Key == KEY_OP_XLAT) {
					Xlat();
					Show();
					delete[] CmpStr;
					return TRUE;
				}

				// <comment> - это требуется для корректной работы логики блоков для Ctrl-K
				int PreSelStart, PreSelEnd;
				m_CurLine->GetSelection(PreSelStart, PreSelEnd);
				// </comment>
				// AY: Это что бы при FastShow LeftPos не становился в конец строки.
				m_CurLine->ObjWidth = m_XX2 - X1 + 1;

				if (m_CurLine->ProcessKey(Key)) {
					int SelStart, SelEnd;

					/* $ 17.09.2002 SKV
					  Если находимся в середине блока,
					  в начале строки, и нажимаем tab, который заменяется
					  на пробелы, выделение съедет. Это фикс.
					*/
					if (Key == KEY_TAB && m_CurLine->GetConvertTabs() && m_BlockStart && m_BlockStart != m_CurLine) {
						m_CurLine->GetSelection(SelStart, SelEnd);
						m_CurLine->Select(SelStart == -1 ? -1 : 0, SelEnd);
					}

					if (!SkipCheckUndo) {
						const wchar_t *NewCmpStr;
						int NewLength;
						m_CurLine->GetBinaryString(&NewCmpStr, nullptr, NewLength);

						if (NewLength != Length || memcmp(CmpStr, NewCmpStr, Length * sizeof(wchar_t))) {
							AddUndoData(UNDO_EDIT, CmpStr, m_CurLine->GetEOL(), m_NumLine, CurPos, Length);    // EOL? - m_CurLine->GetEOL()  m_GlobalEOL   ""
							TextChanged(true);
						}

						delete[] CmpStr;
					}

					// <Bug 794>
					// обработаем только первую и последнюю строку с блоком
					if (Key == KEY_CTRLK && m_EdOpt.PersistentBlocks) {
						if (m_CurLine == m_BlockStart) {
							if (CurPos) {
								m_CurLine->GetSelection(SelStart, SelEnd);

								// 1. блок за концом строки (CurPos был ближе к началу, чем SelStart)
								if ((SelEnd == -1 && PreSelStart > CurPos) || SelEnd > CurPos)
									SelStart = SelEnd = -1;    // в этом случае снимаем выделение

								// 2. CurPos внутри блока
								else if (SelEnd == -1 && PreSelEnd > CurPos && SelStart < CurPos)
									SelEnd = PreSelEnd;    // в этом случае усекаем блок

								// 3. блок остался слева от CurPos или выделение нужно снять (см. выше)
								if (SelEnd >= CurPos || SelStart == -1)
									m_CurLine->Select(SelStart, CurPos);
							} else {
								m_CurLine->Select(-1, -1);
								m_BlockStart = m_BlockStart->m_next;
							}
						} else    // ЗДЕСЬ ЗАСАДА !!! ЕСЛИ ВЫДЕЛЕННЫЙ БЛОК ДОСТАТОЧНО БОЛЬШОЙ (ПО СТРОКАМ), ТО ЦИКЛ ПЕРЕБОРА... МОЖЕТ ЗАТЯНУТЬ...
						{
							// найдем эту последнюю строку (и последняя ли она)
							Edit *CurPtrBlock = m_BlockStart, *CurPtrBlock2 = m_BlockStart;

							while (CurPtrBlock) {
								CurPtrBlock->GetRealSelection(SelStart, SelEnd);

								if (SelStart == -1)
									break;

								CurPtrBlock2 = CurPtrBlock;
								CurPtrBlock = CurPtrBlock->m_next;
							}

							if (m_CurLine == CurPtrBlock2) {
								if (CurPos) {
									m_CurLine->GetSelection(SelStart, SelEnd);
									m_CurLine->Select(SelStart, CurPos);
								} else {
									m_CurLine->Select(-1, -1);
									CurPtrBlock2 = CurPtrBlock2->m_next;
								}
							}
						}
					}

					// </Bug 794>
					ShowEditor(LeftPos == m_CurLine->GetLeftPos());
					return TRUE;
				} else if (!SkipCheckUndo)
					delete[] CmpStr;

				if (m_VBlockStart)
					Show();
			}
			return FALSE;
		}
	}
}

int Editor::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	// Shift + Mouse click -> adhoc quick edit
	if ((MouseEvent->dwControlKeyState & SHIFT_PRESSED) != 0 && (MouseEvent->dwEventFlags & MOUSE_MOVED) == 0
			&& (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0) {
		WINPORT(BeginConsoleAdhocQuickEdit)();
		return TRUE;
	}

	if ((MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) == 0) {
		MouseSelStartingLine = -1;
	}

	// $ 28.12.2000 VVM - Щелчок мышкой снимает непостоянный блок всегда
	if ((MouseEvent->dwButtonState & 3) && !(MouseEvent->dwEventFlags & MOUSE_MOVED)) {
		if (!m_EdOpt.PersistentBlocks) {
			UnmarkBlock();
		}
	}

	if (m_EdOpt.ShowScrollBar && MouseEvent->dwMousePosition.X == X2
			&& !(MouseEvent->dwEventFlags & MOUSE_MOVED)) {
		if (MouseEvent->dwMousePosition.Y == Y1) {
			if (!Flags.Check(FEDITOR_DIALOGMEMOEDIT)) {
				while (IsMouseButtonPressed()) ProcessKey(KEY_CTRLUP);
			} else {
				ProcessKey(KEY_CTRLUP);
			}
		} else if (MouseEvent->dwMousePosition.Y == Y2) {
			if (!Flags.Check(FEDITOR_DIALOGMEMOEDIT)) {
				while (IsMouseButtonPressed()) ProcessKey(KEY_CTRLDOWN);
			} else {
				ProcessKey(KEY_CTRLDOWN);
			}
		} else {
			if (!Flags.Check(FEDITOR_DIALOGMEMOEDIT)) {
				while (IsMouseButtonPressed())
					GoToLine((m_NumLastLine - 1) * (MouseY - Y1) / (Y2 - Y1));
			} else {
				GoToLine((m_NumLastLine - 1) * (MouseY - Y1) / (Y2 - Y1));
			}
		}
		return TRUE;
	}

	// scroll up/down by dragging outside editor window
	if (MouseEvent->dwMousePosition.Y < Y1 && (MouseEvent->dwButtonState & 3)) {
		if (!Flags.Check(FEDITOR_DIALOGMEMOEDIT)) {
			while (IsMouseButtonPressed() && MouseY < Y1) ProcessKey(KEY_UP);
		} else {
			ProcessKey(KEY_UP);
		}
		return TRUE;
	}
	if (MouseEvent->dwMousePosition.Y > Y2 && (MouseEvent->dwButtonState & 3)) {
		if (!Flags.Check(FEDITOR_DIALOGMEMOEDIT)) {
			while (IsMouseButtonPressed() && MouseY > Y2) ProcessKey(KEY_DOWN);
		} else {
			ProcessKey(KEY_DOWN);
		}
		return TRUE;
	}

	// For any click inside the editor window, first position the cursor
	if (MouseEvent->dwMousePosition.X >= X1 && MouseEvent->dwMousePosition.X <= m_XX2
		&& MouseEvent->dwMousePosition.Y >= Y1 && MouseEvent->dwMousePosition.Y <= Y2)
	{
		if((MouseEvent->dwButtonState & 3))
		{
			// Calculate line number width if needed
			int LineNumWidth = 0;

			Edit* TargetLine = nullptr;
			int TargetPos = -1;

			// Non-word-wrap mode
			{
				int line_offset = MouseEvent->dwMousePosition.Y - Y1;
				TargetLine = m_TopScreen;
				while (line_offset-- && TargetLine && TargetLine->m_next) {
					TargetLine = TargetLine->m_next;
				}

				if (TargetLine) {
					int mouseCellPos = MouseEvent->dwMousePosition.X - X1 - LineNumWidth + TargetLine->GetLeftPos();
					TargetPos = TargetLine->CellPosToReal(mouseCellPos);
				}
			}

			if (TargetLine)
			{
				const int screenHeight = Y2 - Y1;
				auto visibleOffset = [&](Edit* line) -> int
				{
					int offset = 0;
					for (Edit* ptr = m_TopScreen; ptr && offset <= screenHeight; ptr = ptr->m_next, ++offset)
					{
						if (ptr == line)
							return offset;
					}
					return -1;
				};

				int targetOffset = visibleOffset(TargetLine);
				int currentOffset = visibleOffset(m_CurLine);

				if (targetOffset != -1 && currentOffset != -1)
				{
					int delta = targetOffset - currentOffset;
					while (delta > 0)
					{
						Down();
						--delta;
					}
					while (delta < 0)
					{
						Up();
						++delta;
					}
				}
					else
					{
						const int topLineNumber = GetTopScreenLineNumber();
						Edit* topLinePtr = m_TopScreen;

						if (!topLinePtr)
							topLinePtr = m_TopList ? m_TopList : TargetLine;

						m_CurLine = TargetLine;
						int offsetFromTop = (topLinePtr && m_CurLine) ? CalcDistance(topLinePtr, m_CurLine, -1) : 0;
						m_NumLine = topLineNumber + offsetFromTop;
					}

				// Снимаем любое предыдущее выделение при новом клике.
				// UnmarkBlock() должен вызываться только при первом клике, а не при каждом движении
				if ((MouseEvent->dwEventFlags & MOUSE_MOVED) == 0) {
					UnmarkBlock();
				}
				m_CurLine->SetCurPos(TargetPos);
				if (MouseSelStartingLine == -1) {
					MouseSelStartingLine = m_NumLine;
					MouseSelStartingPos = TargetPos;
				} else {
					const bool SelVBlock = (MouseEvent->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
					if (MouseSelStartingLine < m_NumLine || (MouseSelStartingLine == m_NumLine && TargetPos >= MouseSelStartingPos)) {
						MarkBlock(SelVBlock, MouseSelStartingLine, MouseSelStartingPos,
							TargetPos - MouseSelStartingPos, m_NumLine + 1 - MouseSelStartingLine);
					} else {
						MarkBlock(SelVBlock, m_NumLine, TargetPos,
							MouseSelStartingPos - TargetPos, MouseSelStartingLine + 1 - m_NumLine);
					}
				}
				Show();
			}
		}

		// --- Common logic for click/double-click/triple-click ---
		if (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
		{
			static int EditorPrevClickCount = 0;
			static DWORD EditorPrevClickTime = 0;
			static COORD EditorPrevPosition = {0,0};

			if ( (WINPORT(GetTickCount)() - EditorPrevClickTime <= WINPORT(GetDoubleClickTime)())
				&& (MouseEvent->dwEventFlags != MOUSE_MOVED)
				&& (EditorPrevPosition.X == MouseEvent->dwMousePosition.X)
				&& (EditorPrevPosition.Y == MouseEvent->dwMousePosition.Y) )
			{
				EditorPrevClickCount++;
			}
			else
			{
				EditorPrevClickCount = 1;
			}

			EditorPrevClickTime = WINPORT(GetTickCount)();
			EditorPrevPosition = MouseEvent->dwMousePosition;

			if (EditorPrevClickCount == 2) // Double-click
			{
				ProcessKey(KEY_OP_SELWORD);
			}
			else if (EditorPrevClickCount >= 3) // Triple-click (and more)
			{
				m_CurLine->Select(0, m_CurLine->GetLength());
				if (m_CurLine->IsSelection()) {
					Flags.Set(FEDITOR_MARKINGBLOCK);
					m_BlockStart = m_CurLine;
					m_BlockStartLine = m_NumLine;
				}
				EditorPrevClickCount = 0; // Reset to avoid re-triggering
			}
			Show();
		}
	}

	if (MouseEvent->dwButtonState == FROM_LEFT_2ND_BUTTON_PRESSED
			&& (MouseEvent->dwEventFlags & (DOUBLE_CLICK | MOUSE_MOVED | MOUSE_HWHEELED | MOUSE_WHEELED)) == 0) {
		ProcessPasteEvent();
	}

	return TRUE;
}

int Editor::CalcDistance(Edit *From, Edit *To, int MaxDist)
{
	int Distance = 0;

	while (From != To && From->m_next && (MaxDist == -1 || MaxDist-- > 0)) {
		Distance++;
		From = From->m_next;
	}

	return (Distance);
}

int Editor::GetTopScreenLineNumber()
{
	if (!m_CurLine)
		return 1;

	Edit* topLinePtr = m_TopScreen;
	if (!topLinePtr)
		topLinePtr = m_TopList ? m_TopList : m_CurLine;

	int relative = (m_CurLine == topLinePtr) ? 0 : CalcDistance(topLinePtr, m_CurLine, -1);
	return std::max(0, m_NumLine - relative) + 1;
}

void Editor::DeleteString(Edit *DelPtr, int LineNumber, bool DeleteLast, int UndoLine)
{
	if (Flags.Check(FEDITOR_LOCKMODE))
		return;

	/* $ 16.12.2000 OT
	   CtrlY на последней строке с выделенным вертикальным блоком не снимал выделение */
	if (m_VBlockStart && m_NumLine < m_VBlockY + m_VBlockSizeY) {
		if (m_NumLine < m_VBlockY) {
			if (m_VBlockY > 0) {
				m_VBlockY--;
				m_BlockStartLine--;
			}
		} else if (--m_VBlockSizeY <= 0)
			m_VBlockStart = nullptr;
	}

	TextChanged(true);

	if (!DelPtr->m_next && (!DeleteLast || !DelPtr->m_prev)) {
		AddUndoData(DelPtr, UndoLine);
		DelPtr->SetString(L"");
		return;
	}

	for (size_t I = 0; I < ARRAYSIZE(m_SavePos.Line); I++)
		if (m_SavePos.Line[I] != POS_NONE && UndoLine < static_cast<int>(m_SavePos.Line[I]))
			m_SavePos.Line[I]--;

	if (m_StackPos) {
		InternalEditorStackBookMark *sb_temp = m_StackPos, *sb_new;

		while (sb_temp->prev)
			sb_temp = sb_temp->prev;

		while (sb_temp) {
			sb_new = sb_temp->next;

			if (UndoLine < static_cast<int>(sb_temp->Line))
				sb_temp->Line--;
			else {
				if (UndoLine == static_cast<int>(sb_temp->Line))
					DeleteStackBookmark(sb_temp);
			}

			sb_temp = sb_new;
		}
	}

	m_NumLastLine--;

	if (m_LastGetLine) {
		if (LineNumber <= m_LastGetLineNumber) {
			if (LineNumber == m_LastGetLineNumber) {
				m_LastGetLine = m_LastGetLine->m_prev;
			}
			m_LastGetLineNumber--;
		}
	}

	if (m_CurLine == DelPtr) {
		int LeftPos, CurPos;
		CurPos = DelPtr->GetCellCurPos();
		LeftPos = DelPtr->GetLeftPos();

		if (DelPtr->m_next)
			m_CurLine = DelPtr->m_next;
		else {
			m_CurLine = DelPtr->m_prev;
			/* $ 04.11.2002 SKV
			  Вроде как если это произошло, номер текущей строки надо изменить.
			*/
			m_NumLine--;
		}

		m_CurLine->SetLeftPos(LeftPos);
		m_CurLine->SetCellCurPos(CurPos);
	}

	if (DelPtr->m_prev) {
		DelPtr->m_prev->m_next = DelPtr->m_next;

		if (DelPtr == m_EndList)
			m_EndList = m_EndList->m_prev;
	}

	if (DelPtr->m_next)
		DelPtr->m_next->m_prev = DelPtr->m_prev;

	if (DelPtr == m_TopScreen) {
		if (m_TopScreen->m_next)
			m_TopScreen = m_TopScreen->m_next;
		else
			m_TopScreen = m_TopScreen->m_prev;
	}

	if (DelPtr == m_TopList)
		m_TopList = m_TopList->m_next;

	if (DelPtr == m_BlockStart) {
		m_BlockStart = m_BlockStart->m_next;

		// Mantis#0000316: Не работает копирование строки
		if (m_BlockStart && !m_BlockStart->IsSelection())
			m_BlockStart = nullptr;
	}

	if (DelPtr == m_VBlockStart)
		m_VBlockStart = m_VBlockStart->m_next;

	if (UndoLine != -1)
		AddUndoData(UNDO_DELSTR, DelPtr->GetStringAddr(), DelPtr->GetEOL(), UndoLine, 0, DelPtr->GetLength());

	if (m_LastGetLine == DelPtr) {
		m_LastGetLine = nullptr;
		m_LastGetLineNumber = 0;
	}

	m_AutoDeletedColors.erase(DelPtr);

	delete DelPtr;
}

void Editor::InsertString()
{
	if (Flags.Check(FEDITOR_LOCKMODE))
		return;

	/*$ 10.08.2000 skv
	  There is only one return - if new will fail.
	  In this case things are realy bad.
	  Move TextChanged to the end of functions
	  AFTER all modifications are made.
	*/
	//  TextChanged(true);
	Edit *NewString;
	Edit *SrcIndent = nullptr;
	int SelStart, SelEnd;
	int CurPos;
	bool NewLineEmpty = true;
	NewString = InsertString(nullptr, 0, m_CurLine, m_NumLine);

	if (!NewString)
		return;

	// NewString->SetTables(UseDecodeTable ? &TableSet:nullptr); // ??
	int Length;
	const wchar_t *CurLineStr;
	const wchar_t *EndSeq;
	m_CurLine->GetBinaryString(&CurLineStr, &EndSeq, Length);

	/* $ 13.01.2002 IS
	   Если не был определен тип конца строки, то считаем что конец строки
	   у нас равен DOS_EOL_fmt и установим его явно.
	*/
	if (!*EndSeq)
		m_CurLine->SetEOL(*m_GlobalEOL ? m_GlobalEOL : NATIVE_EOLW);

	CurPos = m_CurLine->GetCurPos();
	m_CurLine->GetSelection(SelStart, SelEnd);

	for (size_t I = 0; I < ARRAYSIZE(m_SavePos.Line); I++)
		if (m_SavePos.Line[I] != POS_NONE
				&& (m_NumLine < (int)m_SavePos.Line[I] || (m_NumLine == (int)m_SavePos.Line[I] && !CurPos)))
			m_SavePos.Line[I]++;

	if (m_StackPos) {
		InternalEditorStackBookMark *sb_temp = m_StackPos;

		while (sb_temp->prev)
			sb_temp = sb_temp->prev;

		while (sb_temp) {
			if (m_NumLine < static_cast<int>(sb_temp->Line)
					|| (m_NumLine == static_cast<int>(sb_temp->Line) && !CurPos))
				sb_temp->Line++;

			sb_temp = sb_temp->next;
		}
	}

	int IndentPos = 0;

	if (m_EdOpt.AutoIndent && !m_Pasting) {
		Edit *Line = m_CurLine;

		for (bool Found=false; Line && !Found; Line = Line->m_prev) {
			const wchar_t *Str;
			int Length;
			Line->GetBinaryString(&Str, nullptr, Length);

			for (int I = 0; I < Length; I++) {
				if (!IsSpace(Str[I])) {
					Line->SetCurPos(I);
					IndentPos = Line->GetCellCurPos();
					SrcIndent = Line;
					Found = true;
					break;
				}
			}
		}
	}

	bool SpaceOnly = true;

	if (CurPos < Length) {
		if (IndentPos > 0) {
			for (int I = 0; I < CurPos; I++) {
				if (!IsSpace(CurLineStr[I])) {
					SpaceOnly = false;
					break;
				}
			}
		}

		NewString->SetBinaryString(&CurLineStr[CurPos], Length - CurPos);

		for (int i = 0; i < Length - CurPos; i++) {
			if (!IsSpace(CurLineStr[i + CurPos])) {
				NewLineEmpty = false;
				break;
			}
		}

		AddUndoData(UNDO_BEGIN);
		AddUndoData(m_CurLine, m_NumLine);
		AddUndoData(UNDO_INSSTR, nullptr, m_EndList == m_CurLine ? L"" : m_GlobalEOL, m_NumLine + 1, 0);    // EOL? - m_CurLine->GetEOL()  m_GlobalEOL   ""
		AddUndoData(UNDO_END);
		wchar_t *NewCurLineStr = (wchar_t *)malloc((CurPos + 1) * sizeof(wchar_t));

		if (!NewCurLineStr)
			return;

		wmemcpy(NewCurLineStr, CurLineStr, CurPos);
		NewCurLineStr[CurPos] = 0;
		int StrSize = CurPos;

		if (m_EdOpt.AutoIndent && NewLineEmpty) {
			RemoveTrailingSpaces(NewCurLineStr);
			StrSize = StrLength(NewCurLineStr);
		}

		m_CurLine->SetBinaryString(NewCurLineStr, StrSize);
		m_CurLine->SetEOL(EndSeq);
		free(NewCurLineStr);
	} else {
		NewString->SetString(L"");
		AddUndoData(UNDO_INSSTR, nullptr, L"", m_NumLine + 1, 0);    // EOL? - m_CurLine->GetEOL()  m_GlobalEOL   ""
	}

	if (m_VBlockStart && m_NumLine < m_VBlockY + m_VBlockSizeY) {
		if (m_NumLine < m_VBlockY) {
			m_VBlockY++;
			m_BlockStartLine++;
		} else
			m_VBlockSizeY++;
	}

	if (SelStart != -1 && (SelEnd == -1 || CurPos < SelEnd)) {
		if (CurPos >= SelStart) {
			m_CurLine->Select(SelStart, -1);
			NewString->Select(0, SelEnd == -1 ? -1 : SelEnd - CurPos);
		} else {
			m_CurLine->Select(-1, 0);
			NewString->Select(SelStart - CurPos, SelEnd == -1 ? -1 : SelEnd - CurPos);
			m_BlockStart = NewString;
			m_BlockStartLine++;
		}
	} else if (m_BlockStart && m_NumLine < m_BlockStartLine)
		m_BlockStartLine++;

	NewString->SetEOL(EndSeq);
	m_CurLine->SetCurPos(0);

	if (m_CurLine == m_EndList)
		m_EndList = NewString;

	Down();

	if (IndentPos > 0) {
		int OrgIndentPos = IndentPos;
		ShowEditor(false);
		m_CurLine->GetBinaryString(&CurLineStr, nullptr, Length);

		if (SpaceOnly) {
			int Decrement = 0;

			for (int I = 0; I < IndentPos && I < Length; I++) {
				if (!IsSpace(CurLineStr[I]))
					break;

				if (CurLineStr[I] == L' ')
					Decrement++;
				else {
					int TabPos = m_CurLine->RealPosToCell(I);
					Decrement+= m_EdOpt.TabSize - (TabPos % m_EdOpt.TabSize);
				}
			}

			IndentPos-= Decrement;
		}

		if (IndentPos > 0) {
			if (m_CurLine->GetLength() || !m_EdOpt.CursorBeyondEOL) {
				m_CurLine->ProcessKey(KEY_HOME);
				bool SaveOvertypeMode = m_CurLine->GetOvertypeMode();
				m_CurLine->SetOvertypeMode(false);
				const wchar_t *PrevStr = nullptr;
				int PrevLength = 0;

				if (SrcIndent) {
					SrcIndent->GetBinaryString(&PrevStr, nullptr, PrevLength);
				}

				for (int I = 0; m_CurLine->GetCellCurPos() < IndentPos; I++) {
					if (SrcIndent && I < PrevLength && IsSpace(PrevStr[I])) {
						m_CurLine->ProcessKey(PrevStr[I]);
					} else {
						m_CurLine->ProcessKey(KEY_SPACE);
					}
				}

				while (m_CurLine->GetCellCurPos() > IndentPos)
					m_CurLine->ProcessKey(KEY_BS);

				m_CurLine->SetOvertypeMode(SaveOvertypeMode);
			}

			m_CurLine->SetCellCurPos(IndentPos);
		}

		m_CurLine->GetBinaryString(&CurLineStr, nullptr, Length);
		CurPos = m_CurLine->GetCurPos();

		if (SpaceOnly) {
			int NewPos = 0;

			for (int I = 0; I < Length; I++) {
				NewPos = I;

				if (!IsSpace(CurLineStr[I]))
					break;
			}

			if (NewPos > OrgIndentPos)
				NewPos = OrgIndentPos;

			if (NewPos > CurPos)
				m_CurLine->SetCurPos(NewPos);
		}
	}

	TextChanged(true);
}

void Editor::Down()
{
	// TODO: "Свертка" - если учесть "!Flags.Check(FSCROBJ_VISIBLE)", то крутить надо до следующей видимой строки
	Edit *CurPtr;
	int LeftPos, CurPos, Y;

	if (!m_CurLine->m_next)
		return;

	for (Y = 0, CurPtr = m_TopScreen; CurPtr && CurPtr != m_CurLine; CurPtr = CurPtr->m_next)
		Y++;

	if (Y >= Y2 - Y1)
		m_TopScreen = m_TopScreen->m_next;

	CurPos = m_CurLine->GetCellCurPos();
	LeftPos = m_CurLine->GetLeftPos();
	m_CurLine = m_CurLine->m_next;
	m_NumLine++;
	m_CurLine->SetLeftPos(LeftPos);
	m_CurLine->SetCellCurPos(CurPos);
}

void Editor::ScrollDown()
{
	// TODO: "Свертка" - если учесть "!Flags.Check(FSCROBJ_VISIBLE)", то крутить надо до следующей видимой строки
	int LeftPos, CurPos;

	if (!m_CurLine->m_next || !m_TopScreen->m_next)
		return;

	if (!m_EdOpt.AllowEmptySpaceAfterEof && CalcDistance(m_TopScreen, m_EndList, Y2 - Y1) < Y2 - Y1) {
		Down();
		return;
	}

	m_TopScreen = m_TopScreen->m_next;
	CurPos = m_CurLine->GetCellCurPos();
	LeftPos = m_CurLine->GetLeftPos();
	m_CurLine = m_CurLine->m_next;
	m_NumLine++;
	m_CurLine->SetLeftPos(LeftPos);
	m_CurLine->SetCellCurPos(CurPos);
}

void Editor::Up()
{
	// TODO: "Свертка" - если учесть "!Flags.Check(FSCROBJ_VISIBLE)", то крутить надо до следующей видимой строки
	int LeftPos, CurPos;

	if (!m_CurLine->m_prev)
		return;

	if (m_CurLine == m_TopScreen)
		m_TopScreen = m_TopScreen->m_prev;

	CurPos = m_CurLine->GetCellCurPos();
	LeftPos = m_CurLine->GetLeftPos();
	m_CurLine = m_CurLine->m_prev;
	m_NumLine--;
	m_CurLine->SetLeftPos(LeftPos);
	m_CurLine->SetCellCurPos(CurPos);
}

void Editor::ScrollUp()
{
	// TODO: "Свертка" - если учесть "!Flags.Check(FSCROBJ_VISIBLE)", то крутить надо до следующей видимой строки
	int LeftPos, CurPos;

	if (!m_CurLine->m_prev)
		return;

	if (!m_TopScreen->m_prev) {
		Up();
		return;
	}

	m_TopScreen = m_TopScreen->m_prev;
	CurPos = m_CurLine->GetCellCurPos();
	LeftPos = m_CurLine->GetLeftPos();
	m_CurLine = m_CurLine->m_prev;
	m_NumLine--;
	m_CurLine->SetLeftPos(LeftPos);
	m_CurLine->SetCellCurPos(CurPos);
}

/* $ 21.01.2001 SVS
   Диалоги поиска/замены выведен из Editor::Search
   в отдельную функцию GetSearchReplaceParams
   (файл stddlg.cpp)
*/
bool Editor::Search(bool ReplaceMode, NextType NextTp)
{
	FARString strMsgStr;
	const wchar_t *TextHistoryName = L"SearchText";
	const wchar_t *ReplaceHistoryName = L"ReplaceText";
	bool ReplaceAll = false;
	bool Match = false;
	bool UserBreak = false;
	bool Next = (NextTp != NEXT_NONE);

	if (Next && m_LastSearch.SearchStr.IsEmpty())
		return true;

	auto LS = m_LastSearch;
	LS.Reverse = (NextTp == NEXT_NONE) ? m_LastSearch.Reverse : (NextTp == NEXT_REVERSE);

	if (!Next) {
		if (m_EdOpt.SearchPickUpWord) {
			int StartPickPos = -1, EndPickPos = -1;
			const wchar_t *Ptr = CalcWordFromString(m_CurLine->GetStringAddr(), m_CurLine->GetCurPos(),
					&StartPickPos, &EndPickPos, GetWordDiv());

			if (Ptr)
				LS.SearchStr = FARString(Ptr, (size_t)EndPickPos - StartPickPos + 1);
		}

		if (GetSearchReplaceParams(ReplaceMode, LS, TextHistoryName,	ReplaceHistoryName, L"EditorSearch"))
			m_LastSearch = LS;
		else
			return false;

		if (LS.SearchStr.IsEmpty())
			return true;
	}

	if (!m_EdOpt.PersistentBlocks || (LS.SelectFound && !ReplaceMode))
		UnmarkBlock();

	{
		SCOPED_ACTION(TPreRedrawFuncGuard)(Editor::PR_EditorShowMsg);
		strMsgStr = LS.SearchStr;
		InsertQuote(strMsgStr);
		SetCursorType(false, -1);

		const int StartLine = m_NumLine;
		int NewNumLine = m_NumLine;
		DWORD StartTime = WINPORT(GetTickCount)();

		RegExp re;
		if (LS.Regexp && !CompileRegexp(LS.SearchStr, LS.CaseSens, &re))
			return true;

		Edit *CurPtr = m_CurLine;
		int FromPos = m_CurLine->GetCurPos();

		auto ChangeLine = [&] {
			if (!LS.Reverse) {
				CurPtr = CurPtr->m_next;
				FromPos = 0;
				NewNumLine++;
			}
			else {
				CurPtr = CurPtr->m_prev;
				if (CurPtr) {
					FromPos = CurPtr->GetLength();
					NewNumLine--;
				}
			}
		};

		if (Next) {
			if (!LS.Reverse)
				++FromPos;
			else if (--FromPos < 0)
				ChangeLine();
		}

		while (CurPtr) {
			DWORD CurTime = WINPORT(GetTickCount)();

			if (CurTime - StartTime > RedrawTimeout) {
				StartTime = CurTime;

				if (CheckForEscSilent()) {
					if (ConfirmAbortOp()) {
						UserBreak = true;
						break;
					}
				}

				SetCursorType(false, -1);
				int Total = LS.Reverse ? StartLine : m_NumLastLine - StartLine;
				int Current = abs(NewNumLine - StartLine);
				EditorShowMsg(Msg::EditSearchTitle, Msg::EditSearchingFor, strMsgStr, ToPercent64(Current, Total));
			}

			int SearchLength = 0;
			FARString ReplaceStrCurrent(ReplaceMode ? LS.ReplaceStr : L"");

			if (CurPtr->Search(LS.SearchStr, ReplaceStrCurrent, FromPos, LS.CaseSens, LS.WholeWords,
					LS.Reverse, LS.Regexp ? &re:nullptr, SearchLength))
			{
				bool ReverseNewLine = false;
				const bool EmptyMatch = (SearchLength == 0);
				bool Skip = false;

				if (LS.SelectFound && !ReplaceMode) {
					m_Pasting++;
					Lock();
					UnmarkBlock();
					Flags.Set(FEDITOR_MARKINGBLOCK);
					int iFoundPos = CurPtr->GetCurPos();
					CurPtr->Select(iFoundPos, iFoundPos + SearchLength);
					m_BlockStart = CurPtr;
					m_BlockStartLine = NewNumLine;
					Unlock();
					m_Pasting--;
				}

				/* $ 24.01.2003 KM
				   ! По окончании поиска отступим от верха экрана на треть отображаемой высоты.
				*/
				/* $ 15.04.2003 VVM
				   Отступим на четверть и проверим на перекрытие диалогом замены */
				int FromTop = (ScrY - 2) / 4;

				if (FromTop < 0 || FromTop >= ((ScrY - 5) / 2 - 2))
					FromTop = 0;

				m_CurLine = CurPtr;
				m_TopScreen = CurPtr;
				for (int i = 0; (i < FromTop) && m_TopScreen->m_prev; i++) {
					m_TopScreen = m_TopScreen->m_prev;
				}

				m_NumLine = NewNumLine;
				int LeftPos = CurPtr->GetLeftPos();
				int CellCurPos = CurPtr->GetCellCurPos();

				if (ObjWidth > 8 && CellCurPos - LeftPos + SearchLength > ObjWidth - 8)
					CurPtr->SetLeftPos(CellCurPos + SearchLength - ObjWidth + 8);

				if (ReplaceMode) {
					int MsgCode = 0;

					if (!ReplaceAll) {
						Show();
						SHORT CurX, CurY;
						GetCursorPos(CurX, CurY);
						ScrBuf.ApplyColor(CurX, CurY,
								CurPtr->RealPosToCell(CurPtr->CellPosToReal(CurX) + SearchLength) - 1, CurY,
								FarColorToReal(COL_EDITORSELECTEDTEXT));
						FARString QSearchStr(CurPtr->GetStringAddr() + CurPtr->GetCurPos(), SearchLength);
						FARString QReplaceStr = ReplaceStrCurrent;
						InsertQuote(ReplaceNulls(QSearchStr));
						InsertQuote(ReplaceNulls(QReplaceStr));
						PreRedrawItem pitem = PreRedraw.Pop();
						MsgCode = Message(0, 4, &EditorConfirmReplaceId, Msg::EditReplaceTitle,
								Msg::EditAskReplace, QSearchStr, Msg::EditAskReplaceWith, QReplaceStr,
								Msg::EditReplace, Msg::EditReplaceAll, Msg::EditSkip, Msg::EditCancel);
						PreRedraw.Push(pitem);

						switch (MsgCode) {
							case 0:  break;
							case 1:  ReplaceAll = true; break;
							case 2:  Skip = true; break;
							default: UserBreak = true;  break;
						}

						if (UserBreak)
							break;
					}

					if (!MsgCode || MsgCode == 1) {
						m_Pasting++;

						/*$ 15.08.2000 skv
						  If Replace FARString doesn't contain control symbols (tab and return),
						  processed with fast method, otherwise use improved old one.
						*/
						if (ReplaceStrCurrent.Contains(L'\r')) {
							bool SaveOvertypeMode = Flags.Check(FEDITOR_OVERTYPE);
							Flags.Set(FEDITOR_OVERTYPE);
							m_CurLine->SetOvertypeMode(true);

							int I = 0;
							for (; SearchLength && ReplaceStrCurrent[I]; I++, SearchLength--) {
								int Ch = ReplaceStrCurrent[I];

								if (Ch == L'\t') {
									Flags.Clear(FEDITOR_OVERTYPE);
									m_CurLine->SetOvertypeMode(false);
									ProcessKey(KEY_DEL);
									ProcessKey(KEY_TAB);
									Flags.Set(FEDITOR_OVERTYPE);
									m_CurLine->SetOvertypeMode(true);
									continue;
								}

								/* $ 24.05.2002 SKV
								  Если реплэйсим на Enter, то overtype не спасёт.
								  Нужно сначала удалить то, что заменяем.
								*/
								if (Ch == L'\r') {
									ProcessKey(KEY_DEL);
								}

								if (Ch != KEY_BS && !(Ch == KEY_DEL || Ch == KEY_NUMDEL))
									ProcessKey(Ch);
							}

							if (!SearchLength) {
								Flags.Clear(FEDITOR_OVERTYPE);
								m_CurLine->SetOvertypeMode(false);

								for (; ReplaceStrCurrent[I]; I++) {
									int Ch = ReplaceStrCurrent[I];

									if (Ch != KEY_BS && !(Ch == KEY_DEL || Ch == KEY_NUMDEL))
										ProcessKey(Ch);
								}
							} else {
								for (; SearchLength; SearchLength--) {
									ProcessKey(KEY_DEL);
								}
							}

							int Cnt = 0;
							const wchar_t *Tmp = ReplaceStrCurrent;

							while ((Tmp = wcschr(Tmp, L'\r'))) {
								Cnt++;
								Tmp++;
							}

							if (Cnt > 0) {
								CurPtr = m_CurLine;
								NewNumLine+= Cnt;
							}

							Flags.Change(FEDITOR_OVERTYPE, SaveOvertypeMode);
						}
						else {
							/* Fast method */
							const int SStrLen = SearchLength;
							const int RStrLen = ReplaceStrCurrent.GetLength();
							int StrLen;
							const wchar_t *Str, *Eol;
							m_CurLine->GetBinaryString(&Str, &Eol, StrLen);
							int EolLen = StrLength(Eol);
							int NewStrLen = StrLen - SStrLen + RStrLen + EolLen;
							wchar_t *NewStr = new wchar_t[NewStrLen + 1];
							int CurPos = m_CurLine->GetCurPos();
							wmemcpy(NewStr, Str, CurPos);
							wmemcpy(NewStr + CurPos, ReplaceStrCurrent, RStrLen);
							wmemcpy(NewStr + CurPos + RStrLen, Str + CurPos + SStrLen, StrLen - CurPos - SStrLen);
							wmemcpy(NewStr + NewStrLen - EolLen, Eol, EolLen);
							AddUndoData(m_CurLine, m_NumLine);

							if (!LS.Reverse) {
								if (m_CurLine->GetConvertTabs() == EXPAND_ALLTABS) {
									m_CurLine->SetConvertTabs(EXPAND_NOTABS);      // change temporarily
									m_CurLine->SetBinaryString(NewStr, NewStrLen);
									m_CurLine->SetCurPos(CurPos + RStrLen);
									m_CurLine->ExpandTabs();                       // ExpandTabs() adjusts m_CurPos
									m_CurLine->SetConvertTabs(EXPAND_ALLTABS);     // restore
								}
								else {
									m_CurLine->SetBinaryString(NewStr, NewStrLen);
									m_CurLine->SetCurPos(CurPos + RStrLen);
								}
							}
							else {
								m_CurLine->SetBinaryString(NewStr, NewStrLen);
								if (CurPos > 0)
									m_CurLine->SetCurPos(CurPos - 1);
								else {
									m_CurLine->SetCurPos(CurPos);
									ReverseNewLine = true;
								}
							}

							delete[] NewStr;
							TextChanged(true);
						}

						m_Pasting--;
					}
				}

				Match = true;

				if (!ReplaceMode)
					break;

				if (ReverseNewLine) {
					ChangeLine();
				}
				else {
					FromPos = m_CurLine->GetCurPos();

					if (Skip || EmptyMatch)
						if (!LS.Reverse)
							FromPos++;
				}
			}
			else {
				ChangeLine();
			}
		}
	}
	Show();

	if (!Match && !UserBreak)
		Message(MSG_WARNING, 1, &EditSearchNotFound, Msg::EditSearchTitle, Msg::EditNotFound, strMsgStr, Msg::Ok);

	return true;
}

void Editor::Paste(const wchar_t *Src)
{
	if (Flags.Check(FEDITOR_LOCKMODE))
		return;

	wchar_t *ClipText = (wchar_t *)Src;
	bool IsDeleteClipText = false;

	if (!ClipText) {
		Clipboard clip;

		if (!clip.Open())
			return;

		bool IsVertical = false;
		ClipText = clip.Paste(IsVertical);
		if (ClipText && IsVertical) {
			VPaste(ClipText);
			clip.Close();
			return;
		}

		clip.Close();

		IsDeleteClipText = true;
	}

	if (ClipText && *ClipText) {
		AddUndoData(UNDO_BEGIN);
		Flags.Set(FEDITOR_NEWUNDO);
		TextChanged(true);
		int SaveOvertype = Flags.Check(FEDITOR_OVERTYPE);
		UnmarkBlock();
		m_Pasting++;
		Lock();

		if (Flags.Check(FEDITOR_OVERTYPE)) {
			Flags.Clear(FEDITOR_OVERTYPE);
			m_CurLine->SetOvertypeMode(false);
		}

		m_BlockStart = m_CurLine;
		m_BlockStartLine = m_NumLine;
		/* $ 19.05.2001 IS
		   Решение проблемы непрошеной конвертации табуляции (которая должна быть
		   добавлена в начало строки при автоотступе) в пробелы.
		*/
		int StartPos = m_CurLine->GetCurPos();
		int oldAutoIndent = m_EdOpt.AutoIndent;

		for (int I = 0; ClipText[I];) {
			if (ClipText[I] == L'\n' || ClipText[I] == L'\r') {
				m_CurLine->Select(StartPos, -1);
				StartPos = 0;
				m_EdOpt.AutoIndent = FALSE;
				ProcessKey(KEY_ENTER);

				if (ClipText[I] == L'\r' && ClipText[I + 1] == L'\n')
					I++;

				I++;
			} else {
				if (m_EdOpt.AutoIndent)    // первый символ вставим так, чтобы
				{                        // сработал автоотступ
					// ProcessKey(UseDecodeTable?TableSet.DecodeTable[(unsigned)ClipText[I]]:ClipText[I]); //BUGBUG
					ProcessKey(ClipText[I]);    // BUGBUG
					I++;
					StartPos = m_CurLine->GetCurPos();

					if (StartPos)
						StartPos--;
				}

				int Pos = I;

				while (ClipText[Pos] && ClipText[Pos] != L'\n' && ClipText[Pos] != L'\r')
					Pos++;

				if (Pos > I) {
					const wchar_t *Str;
					int Length, CurPos;
					m_CurLine->GetBinaryString(&Str, nullptr, Length);
					CurPos = m_CurLine->GetCurPos();
					AddUndoData(UNDO_EDIT, Str, m_CurLine->GetEOL(), m_NumLine, CurPos, Length);    // EOL? - m_CurLine->GetEOL()  m_GlobalEOL   ""
					m_CurLine->InsertBinaryString(&ClipText[I], Pos - I);
				}

				I = Pos;
			}
		}

		m_EdOpt.AutoIndent = oldAutoIndent;
		m_CurLine->Select(StartPos, m_CurLine->GetCurPos());
		/* IS $ */

		if (SaveOvertype) {
			Flags.Set(FEDITOR_OVERTYPE);
			m_CurLine->SetOvertypeMode(true);
		}

		m_Pasting--;
		Unlock();
		AddUndoData(UNDO_END);
	}

	if (IsDeleteClipText)
		free(ClipText);
}

void Editor::Copy(bool Append)
{
	if (m_VBlockStart) {
		VCopy(Append);
		return;
	}

	wchar_t *CopyData = nullptr;

	Clipboard clip;

	if (!clip.Open())
		return;

	if (Append)
		CopyData = clip.Paste();

	if ((CopyData = Block2Text(CopyData))) {
		clip.Copy(CopyData);
		free(CopyData);
	}

	clip.Close();
}

wchar_t *Editor::Block2Text(wchar_t *ptrInitData)
{
	size_t DataSize = 0;

	if (ptrInitData)
		DataSize = wcslen(ptrInitData);

	size_t TotalChars = DataSize;
	int StartSel, EndSel;
	for (Edit *Ptr = m_BlockStart; Ptr; Ptr = Ptr->m_next) {
		Ptr->GetSelection(StartSel, EndSel);
		if (StartSel == -1)
			break;
		if (EndSel == -1) {
			TotalChars+= Ptr->GetLength() - StartSel;
			TotalChars+= wcslen(NATIVE_EOLW);    // CRLF
		} else
			TotalChars+= EndSel - StartSel;
	}
	TotalChars++;    // '\0'

	wchar_t *CopyData = (wchar_t *)malloc(TotalChars * sizeof(wchar_t));

	if (!CopyData) {
		if (ptrInitData)
			free(ptrInitData);

		return nullptr;
	}

	if (ptrInitData) {
		wcscpy(CopyData, ptrInitData);
		free(ptrInitData);
	} else {
		*CopyData = 0;
	}

	for (Edit *Ptr = m_BlockStart; Ptr; Ptr = Ptr->m_next) {
		Ptr->GetSelection(StartSel, EndSel);
		if (StartSel == -1)
			break;

		int Length;
		if (EndSel == -1)
			Length = Ptr->GetLength() - StartSel;
		else
			Length = EndSel - StartSel;

		Ptr->GetSelString(CopyData + DataSize, Length + 1);
		DataSize+= Length;

		if (EndSel == -1) {
			wcscpy(CopyData + DataSize, NATIVE_EOLW);
			DataSize+= wcslen(NATIVE_EOLW);
		}
	}

	return CopyData;
}

void Editor::DeleteBlock()
{
	if (Flags.Check(FEDITOR_LOCKMODE))
		return;

	if (m_VBlockStart) {
		DeleteVBlock();
		return;
	}

	Edit *CurPtr = m_BlockStart;
	AddUndoData(UNDO_BEGIN);

	for (int i = m_BlockStartLine; CurPtr; i++) {
		TextChanged(true);
		int StartSel, EndSel;
		/* $ 17.09.2002 SKV
		  меняем на Real что б ловить выделение за концом строки.
		*/
		CurPtr->GetRealSelection(StartSel, EndSel);

		if (EndSel != -1 && EndSel > CurPtr->GetLength())
			EndSel = -1;

		if (StartSel == -1)
			break;

		if (!StartSel && EndSel == -1) {
			Edit *NextLine = CurPtr->m_next;
			DeleteString(CurPtr, i, false, m_BlockStartLine);

			if (m_BlockStartLine < m_NumLine)
				m_NumLine--;

			if (NextLine) {
				CurPtr = NextLine;
				continue;
			} else
				break;
		}

		int Length = CurPtr->GetLength();

		if (StartSel || EndSel)
			AddUndoData(CurPtr, m_BlockStartLine);

		/* $ 17.09.2002 SKV
		  опять про выделение за концом строки.
		  InsertBinaryString добавит trailing space'ов
		*/
		if (StartSel > Length) {
			Length = StartSel;
			CurPtr->SetCurPos(Length);
			CurPtr->InsertBinaryString(L"", 0);
		}

		const wchar_t *CurStr, *EndSeq;

		CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);

		// дальше будет realloc, поэтому тут malloc.
		wchar_t *TmpStr = (wchar_t *)malloc((Length + 3) * sizeof(wchar_t));

		wmemcpy(TmpStr, CurStr, Length);

		TmpStr[Length] = 0;

		bool DeleteNext = false;

		if (EndSel == -1) {
			EndSel = Length;

			if (CurPtr->m_next)
				DeleteNext = true;
		}

		wmemmove(TmpStr + StartSel, TmpStr + EndSel, Length - EndSel + 1);
		int CurPos = StartSel;
		Length -= EndSel - StartSel;

		if (DeleteNext) {
			const wchar_t *NextStr, *EndSeq;
			int NextLength, NextStartSel, NextEndSel;
			CurPtr->m_next->GetSelection(NextStartSel, NextEndSel);

			if (NextStartSel == -1)
				NextEndSel = 0;

			if (NextEndSel == -1)
				EndSel = -1;
			else {
				CurPtr->m_next->GetBinaryString(&NextStr, &EndSeq, NextLength);
				NextLength-= NextEndSel;

				if (NextLength > 0) {
					TmpStr = (wchar_t *)realloc(TmpStr, (Length + NextLength + 3) * sizeof(wchar_t));
					wmemcpy(TmpStr + Length, NextStr + NextEndSel, NextLength);
					Length+= NextLength;
				}
			}

			if (m_CurLine == CurPtr->m_next) {
				m_CurLine = CurPtr;
				m_NumLine--;
			}

			if (m_CurLine == CurPtr && CurPtr->m_next && CurPtr->m_next == m_TopScreen) {
				m_TopScreen = CurPtr;
			}

			DeleteString(CurPtr->m_next, i, false, m_BlockStartLine + 1);

			if (m_BlockStartLine + 1 < m_NumLine)
				m_NumLine--;
		}

		int EndLength = StrLength(EndSeq);
		wmemcpy(TmpStr + Length, EndSeq, EndLength);
		Length+= EndLength;
		CurPtr->SetBinaryString(TmpStr, Length);
		free(TmpStr);
		CurPtr->SetCurPos(CurPos);

		if (DeleteNext && EndSel == -1) {
			CurPtr->Select(CurPtr->GetLength(), -1);
		} else {
			CurPtr->Select(-1, 0);
			CurPtr = CurPtr->m_next;
			m_BlockStartLine++;
		}
	}

	AddUndoData(UNDO_END);
	m_BlockStart = nullptr;
}

bool Editor::MarkBlock(bool SelVBlock, int SelStartLine, int SelStartPos, int SelWidth, int SelHeight)
{
	fprintf(stderr, "Editor::MarkBlock: VBlock=%d StartLine=%d StartPos=%d Width=%d Height=%d\n",
		SelVBlock, SelStartLine, SelStartPos, SelWidth, SelHeight);

	Edit *CurPtr = GetStringByNumber(SelStartLine);

	if (!CurPtr) {
		fprintf(stderr, "Editor::MarkBlock: fail cuz StartLine=%d not found\n", SelStartLine);
		return false;
	}
	if (SelHeight <= 0 || SelStartPos < 0) {
		fprintf(stderr, "Editor::MarkBlock: fail cuz Height=%d <= 0 || StartPos=%d < 0\n", SelHeight, SelStartPos);
		return false;
	}

	UnmarkBlock();

	if (SelVBlock) {
		Flags.Set(FEDITOR_MARKINGVBLOCK);
		m_VBlockStart = CurPtr;

		if ((m_BlockStartLine = SelStartLine) == -1)
			m_BlockStartLine = m_NumLine;

		m_VBlockX = CurPtr->RealPosToCell(SelStartPos);

		if ((m_VBlockY = SelStartLine) == -1)
			m_VBlockY = m_NumLine;

		auto LastPtr = CurPtr;
		for (int i = SelHeight; --i > 0 && LastPtr->m_next; ) {
			LastPtr = LastPtr->m_next;
		}
		m_VBlockSizeX = LastPtr->RealPosToCell(SelStartPos + SelWidth) - m_VBlockX;
		m_VBlockSizeY = SelHeight;

		if (m_VBlockSizeX < 0) {
			m_VBlockSizeX = -m_VBlockSizeX;
			m_VBlockX-= m_VBlockSizeX;

			if (m_VBlockX < 0)
				m_VBlockX = 0;
		}

	} else {
		Flags.Set(FEDITOR_MARKINGBLOCK);
		m_BlockStart = CurPtr;

		if ((m_BlockStartLine = SelStartLine) == -1)
			m_BlockStartLine = m_NumLine;

		for (int i = 0; i < SelHeight && CurPtr; i++) {
			int SelStart = i ? 0 : SelStartPos;
			int SelEnd = (i < SelHeight - 1) ? -1 : SelStartPos + SelWidth;
			CurPtr->Select(SelStart, SelEnd);
			CurPtr = CurPtr->m_next;
			// ранее было if (!CurPtr) return FALSE
		}
	}

	return true;
}

void Editor::UnmarkBlock()
{
	if (!m_BlockStart && !m_VBlockStart)
		return;

	m_VBlockStart = nullptr;
	_SVS(SysLog(L"[%d] Editor::UnmarkBlock()", __LINE__));
	TurnOffMarkingBlock();

	while (m_BlockStart) {
		int StartSel, EndSel;
		m_BlockStart->GetSelection(StartSel, EndSel);

		if (StartSel == -1) {
			/* $ 24.06.2002 SKV
			  Если в текущей строки нет выделения,
			  это еще не значит что мы в конце.
			  Это может быть только начало :)
			*/
			if (m_BlockStart->m_next) {
				m_BlockStart->m_next->GetSelection(StartSel, EndSel);

				if (StartSel == -1) {
					break;
				}
			} else
				break;
		}

		m_BlockStart->Select(-1, 0);
		m_BlockStart = m_BlockStart->m_next;
	}

	m_BlockStart = nullptr;
	Show();
}

/* $ 07.03.2002 IS
   Удалить выделение, если оно пустое (выделено ноль символов в ширину)
*/
void Editor::UnmarkEmptyBlock()
{
	_SVS(SysLog(L"[%d] Editor::UnmarkEmptyBlock()", __LINE__));

	if (m_BlockStart || m_VBlockStart)    // присутствует выделение
	{
		int Lines = 0, StartSel, EndSel;
		Edit *Block = m_BlockStart;

		if (m_VBlockStart) {
			if (m_VBlockSizeX)
				Lines = m_VBlockSizeY;
		} else
			while (Block)    // пробегаем по всем выделенным строкам
			{
				Block->GetRealSelection(StartSel, EndSel);

				if (StartSel == -1)
					break;

				if (StartSel != EndSel)    // выделено сколько-то символов
				{
					++Lines;               // увеличим счетчик непустых строк
					break;
				}

				Block = Block->m_next;
			}

		if (!Lines)           // если выделено ноль символов в ширину, то
			UnmarkBlock();    // перестанем морочить голову и снимем выделение
	}
}

void Editor::UnmarkMacroBlock()
{
	m_MBlockStart = nullptr;
	m_MBlockStartX = -1;
}

void Editor::GoToLine(int Line)
{
	if (Line != m_NumLine) {
		bool bReverse = false;
		int LastNumLine = m_NumLine;
		int CurScrLine = CalcDistance(m_TopScreen, m_CurLine, -1);
		int CurPos = m_CurLine->GetCellCurPos();
		int LeftPos = m_CurLine->GetLeftPos();

		if (Line < m_NumLine) {
			if (Line > m_NumLine / 2) {
				bReverse = true;
			} else {
				m_CurLine = m_TopList;
				m_NumLine = 0;
			}
		} else {
			if (Line > (m_NumLine + (m_NumLastLine - m_NumLine) / 2)) {
				bReverse = true;
				m_CurLine = m_EndList;
				m_NumLine = m_NumLastLine - 1;
			}
		}

		if (bReverse) {
			for (; m_NumLine > Line && m_CurLine->m_prev; m_NumLine--)
				m_CurLine = m_CurLine->m_prev;
		} else {
			for (; m_NumLine < Line && m_CurLine->m_next; m_NumLine++)
				m_CurLine = m_CurLine->m_next;
		}

		CurScrLine+= m_NumLine - LastNumLine;

		if (CurScrLine < 0 || CurScrLine > Y2 - Y1)
			m_TopScreen = m_CurLine;

		m_CurLine->SetLeftPos(LeftPos);
		m_CurLine->SetCellCurPos(CurPos);
	}

	// <GOTO_UNMARK:2>
	//  if (!m_EdOpt.PersistentBlocks)
	//     UnmarkBlock();
	// </GOTO_UNMARK>
	Show();
	return;
}

void Editor::GoToPosition()
{
	DialogBuilder Builder(Msg::EditGoToLine, L"EditorGotoPos");
	FARString strData;
	Builder.AddEditField(&strData, 28, L"LineNumber",
			DIF_FOCUS | DIF_HISTORY | DIF_USELASTHISTORY | DIF_NOAUTOCOMPLETE);
	Builder.AddOKCancel();
	Builder.ShowDialog();
	if (!strData.IsEmpty()) {
		int LeftPos = m_CurLine->GetCellCurPos() + 1;
		int CurPos = m_CurLine->GetCurPos();

		int NewLine = 0, NewCol = 0;
		GetRowCol(strData, &NewLine, &NewCol);
		GoToLine(NewLine);

		if (NewCol == -1) {
			m_CurLine->SetCellCurPos(CurPos);
			m_CurLine->SetLeftPos(LeftPos);
		} else {
			m_CurLine->SetCellCurPos(NewCol);
		}
		Show();
	}
}

void Editor::GetRowCol(const wchar_t *_argv, int *row, int *col)
{
	int x = 0xffff, y;
	int l;
	wchar_t *argvx = 0;
	int LeftPos = m_CurLine->GetCellCurPos() + 1;
	FARString strArg = _argv;
	// что бы не оставить "врагу" выбора - только то, что мы хотим ;-)
	// "прибьем" все внешние пробелы.
	RemoveExternalSpaces(strArg);
	wchar_t *argv = strArg.GetBuffer();
	// получаем индекс вхождения любого разделителя
	// в искомой строке
	l = (int)wcscspn(argv, L",:;. ");
	// если разделителя нету, то l=strlen(argv)

	if (l < StrLength(argv))    // Варианты: "row,col" или ",col"?
	{
		argv[l] = L'\0';        // Вместо разделителя впиндюлим "конец строки" :-)
		argvx = argv + l + 1;
		x = _wtoi(argvx);
	}

	y = _wtoi(argv);

	// + переход на проценты
	if (wcschr(argv, L'%'))
		y = m_NumLastLine * y / 100;

	//   вычисляем относительность
	if (argv[0] == L'-' || argv[0] == L'+')
		y = m_NumLine + y + 1;

	if (argvx) {
		if (argvx[0] == L'-' || argvx[0] == L'+') {
			x = LeftPos + x;
		}
	}

	strArg.ReleaseBuffer();
	// теперь загоним результат назад
	*row = y;

	if (x != 0xffff)
		*col = x;
	else
		*col = LeftPos;

	(*row)--;

	if (*row < 0)          // если ввели ",Col"
		*row = m_NumLine;    //   то переходим на текущую строку и колонку

	(*col)--;

	if (*col < -1)
		*col = -1;

	return;
}

void Editor::AddUndoData(int Type, const wchar_t *Str, const wchar_t *Eol, int StrNum, int StrPos, int Length)
{
	if (Flags.Check(FEDITOR_DISABLEUNDO))
		return;

	if (StrNum == -1)
		StrNum = m_NumLine;

	auto u = m_UndoPos;
	for (u++; u != m_UndoData.end();) {
		if (u == m_UndoSavePos) {
			m_UndoSavePos = m_UndoData.end();
			Flags.Set(FEDITOR_UNDOSAVEPOSLOST);
		}

		u = m_UndoData.erase(u);
	}

	auto PrevUndo = --m_UndoData.end();

	if (Type == UNDO_END) {
		if (PrevUndo != m_UndoData.end() && PrevUndo->Type != UNDO_BEGIN)
			PrevUndo--;

		if (PrevUndo != m_UndoData.end() && PrevUndo->Type == UNDO_BEGIN) {
			m_UndoData.erase(PrevUndo);
			m_UndoPos = --m_UndoData.end();

			if (PrevUndo == m_UndoSavePos)
				m_UndoSavePos = m_UndoPos;

			return;
		}
	}

	if (Type == UNDO_EDIT && !Flags.Check(FEDITOR_NEWUNDO)) {
		if (PrevUndo != m_UndoData.end() && PrevUndo->Type == UNDO_EDIT && StrNum == PrevUndo->StrNum
				&& (abs(StrPos - PrevUndo->StrPos) <= 1 || abs(StrPos - m_LastChangeStrPos) <= 1)) {
			m_LastChangeStrPos = StrPos;
			return;
		}
	}

	Flags.Clear(FEDITOR_NEWUNDO);
	m_UndoData.push_back(EditorUndoData());
	m_UndoPos = --m_UndoData.end();
	m_UndoPos->SetData(Type, Str, Eol, StrNum, StrPos, Length);

	while (!m_UndoData.empty()
			&& (EditorUndoData::GetUndoDataSize() > m_EdOpt.UndoSize || m_UndoSkipLevel > 0)) {
		auto u = m_UndoData.begin();

		if (u->Type == UNDO_BEGIN)
			++m_UndoSkipLevel;

		if (u->Type == UNDO_END && m_UndoSkipLevel > 0)
			--m_UndoSkipLevel;

		if (m_UndoSavePos == m_UndoData.end())
			Flags.Set(FEDITOR_UNDOSAVEPOSLOST);

		if (u == m_UndoSavePos)
			m_UndoSavePos = m_UndoData.end();

		m_UndoData.erase(u);
	}

	m_UndoPos = --m_UndoData.end();
}

void Editor::AddUndoData(Edit *pEdit, int StrNum)
{
	AddUndoData(UNDO_EDIT, pEdit->GetStringAddr(), pEdit->GetEOL(), StrNum,
			pEdit->GetCurPos(), pEdit->GetLength());
}

void Editor::Undo(bool redo)
{
	auto ustart = m_UndoPos;
	if (redo)
		ustart++;

	if (ustart == m_UndoData.end())
		return;

	TextChanged(true);
	Flags.Set(FEDITOR_DISABLEUNDO);
	int level = 0;
	auto uend = ustart;

	for (; uend != m_UndoData.end(); redo ? uend++ : uend--) {
		if (uend->Type == UNDO_BEGIN || uend->Type == UNDO_END) {
			int l = uend->Type == UNDO_BEGIN ? -1 : 1;
			level+= redo ? -l : l;
		}

		if (level <= 0)
			break;
	}

	if (level)
		uend = ustart;

	UnmarkBlock();
	auto ud = ustart;

	for (;;) {
		if (ud->Type != UNDO_BEGIN && ud->Type != UNDO_END)
			GoToLine(ud->StrNum);

		switch (ud->Type) {
			case UNDO_INSSTR:
				ud->SetData(UNDO_DELSTR, m_CurLine->GetStringAddr(), m_CurLine->GetEOL(), ud->StrNum, ud->StrPos,
						m_CurLine->GetLength());
				DeleteString(m_CurLine, m_NumLine, true, m_NumLine > 0 ? m_NumLine - 1 : m_NumLine);
				break;
			case UNDO_DELSTR:
				ud->Type = UNDO_INSSTR;
				m_Pasting++;

				if (m_NumLine < ud->StrNum) {
					ProcessKey(KEY_END);
					ProcessKey(KEY_ENTER);
				} else {
					ProcessKey(KEY_HOME);
					ProcessKey(KEY_ENTER);
					ProcessKey(KEY_UP);
				}

				m_Pasting--;

				if (ud->Str) {
					m_CurLine->SetString(ud->Str, ud->Length);
					m_CurLine->SetEOL(ud->EOL);    // необходимо дополнительно выставлять, т.к. SetString вызывает Edit::SetBinaryString и... дальше по тексту
				}

				break;
			case UNDO_EDIT: {
				EditorUndoData tmp;
				tmp.SetData(UNDO_EDIT, m_CurLine->GetStringAddr(), m_CurLine->GetEOL(), ud->StrNum, ud->StrPos,
						m_CurLine->GetLength());

				if (ud->Str) {
					m_CurLine->SetString(ud->Str, ud->Length);
					m_CurLine->SetEOL(ud->EOL);    // необходимо дополнительно выставлять, т.к. SetString вызывает Edit::SetBinaryString и... дальше по тексту
				}

				m_CurLine->SetCurPos(ud->StrPos);
				ud->SetData(tmp.Type, tmp.Str, tmp.EOL, tmp.StrNum, tmp.StrPos, tmp.Length);
				break;
			}
		}

		if (ud == uend)
			break;

		redo ? ud++ : ud--;
	}

	m_UndoPos = redo ? ud : --ud;

	if (!Flags.Check(FEDITOR_UNDOSAVEPOSLOST) && m_UndoPos == m_UndoSavePos)
		TextChanged(false);

	Flags.Clear(FEDITOR_DISABLEUNDO);
}

void Editor::SelectAll()
{
	Edit *CurPtr;
	m_BlockStart = m_TopList;
	m_BlockStartLine = 0;

	for (CurPtr = m_TopList; CurPtr; CurPtr = CurPtr->m_next)
		if (CurPtr->m_next)
			CurPtr->Select(0, -1);
		else
			CurPtr->Select(0, CurPtr->GetLength());

	Show();
}

void Editor::SetStartPos(int LineNum, int CharNum)
{
	m_StartLine = LineNum ? LineNum : 1;
	m_StartChar = CharNum ? CharNum : 1;
}

bool Editor::IsFileChanged() const
{
	return Flags.Check(FEDITOR_MODIFIED | FEDITOR_WASCHANGED);
}

bool Editor::IsFileModified() const
{
	return Flags.Check(FEDITOR_MODIFIED);
}

// используется в FileEditor
long Editor::GetCurPos()
{
	Edit *CurPtr = m_TopList;
	long TotalSize = 0;

	while (CurPtr != m_TopScreen) {
		const wchar_t *SaveStr, *EndSeq;
		int Length;
		CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);
		TotalSize+= Length + StrLength(EndSeq);
		CurPtr = CurPtr->m_next;
	}

	return (TotalSize);
}

/*
void Editor::SetStringsTable()
{
  Edit *CurPtr=m_TopList;
  while (CurPtr)
  {
	CurPtr->SetTables(UseDecodeTable ? &TableSet:nullptr);
	CurPtr=CurPtr->m_next;
  }
}
*/

void Editor::BlockLeft()
{
	if (m_VBlockStart) {
		VBlockShift(true);
		return;
	}

	Edit *CurPtr = m_BlockStart;
	int LineNum = m_BlockStartLine;
	/* $ 14.02.2001 VVM
	  + При отсутствии блока AltU/AltI сдвигают текущую строчку */
	int MoveLine = 0;

	if (!CurPtr) {
		MoveLine = 1;
		CurPtr = m_CurLine;
		LineNum = m_NumLine;
	}

	AddUndoData(UNDO_BEGIN);

	while (CurPtr) {
		int StartSel, EndSel;
		CurPtr->GetSelection(StartSel, EndSel);

		/* $ 14.02.2001 VVM
		  + Блока нет - сделаем его искусственно */
		if (MoveLine) {
			StartSel = 0;
			EndSel = -1;
		}

		if (StartSel == -1)
			break;

		int Length = CurPtr->GetLength();
		wchar_t *TmpStr = new wchar_t[Length + m_EdOpt.TabSize + 5];
		const wchar_t *CurStr, *EndSeq;
		CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);
		Length--;

		if (*CurStr == L' ')
			wmemcpy(TmpStr, CurStr + 1, Length);
		else if (*CurStr == L'\t') {
			wmemset(TmpStr, L' ', m_EdOpt.TabSize - 1);
			wmemcpy(TmpStr + m_EdOpt.TabSize - 1, CurStr + 1, Length);
			Length+= m_EdOpt.TabSize - 1;
		}

		if ((EndSel == -1 || EndSel > StartSel) && IsSpace(*CurStr)) {
			int EndLength = StrLength(EndSeq);
			wmemcpy(TmpStr + Length, EndSeq, EndLength);
			Length+= EndLength;
			TmpStr[Length] = 0;
			AddUndoData(UNDO_EDIT, CurStr, CurPtr->GetEOL(), LineNum, 0, CurPtr->GetLength());    // EOL? - m_CurLine->GetEOL()  m_GlobalEOL   ""
			int CurPos = CurPtr->GetCurPos();
			CurPtr->SetBinaryString(TmpStr, Length);
			CurPtr->SetCurPos(CurPos > 0 ? CurPos - 1 : CurPos);

			if (!MoveLine)
				CurPtr->Select(StartSel > 0 ? StartSel - 1 : StartSel, EndSel > 0 ? EndSel - 1 : EndSel);

			TextChanged(true);
		}

		delete[] TmpStr;
		CurPtr = CurPtr->m_next;
		LineNum++;
		MoveLine = 0;
	}

	AddUndoData(UNDO_END);
}

void Editor::BlockRight()
{
	if (m_VBlockStart) {
		VBlockShift(false);
		return;
	}

	Edit *CurPtr = m_BlockStart;
	int LineNum = m_BlockStartLine;
	/* $ 14.02.2001 VVM
	  + При отсутствии блока AltU/AltI сдвигают текущую строчку */
	int MoveLine = 0;

	if (!CurPtr) {
		MoveLine = 1;
		CurPtr = m_CurLine;
		LineNum = m_NumLine;
	}

	AddUndoData(UNDO_BEGIN);

	while (CurPtr) {
		int StartSel, EndSel;
		CurPtr->GetSelection(StartSel, EndSel);

		/* $ 14.02.2001 VVM
		  + Блока нет - сделаем его искусственно */
		if (MoveLine) {
			StartSel = 0;
			EndSel = -1;
		}

		if (StartSel == -1)
			break;

		int Length = CurPtr->GetLength();
		wchar_t *TmpStr = new wchar_t[Length + 5];
		const wchar_t *CurStr, *EndSeq;
		CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);
		*TmpStr = L' ';
		wmemcpy(TmpStr + 1, CurStr, Length);
		Length++;

		if (EndSel == -1 || EndSel > StartSel) {
			int EndLength = StrLength(EndSeq);
			wmemcpy(TmpStr + Length, EndSeq, EndLength);
			TmpStr[Length + EndLength] = 0;
			AddUndoData(UNDO_EDIT, CurStr, CurPtr->GetEOL(), LineNum, 0, CurPtr->GetLength());    // EOL? - m_CurLine->GetEOL()  m_GlobalEOL   ""
			int CurPos = CurPtr->GetCurPos();

			if (Length > 1)
				CurPtr->SetBinaryString(TmpStr, Length + EndLength);

			CurPtr->SetCurPos(CurPos + 1);

			if (!MoveLine)
				CurPtr->Select(StartSel > 0 ? StartSel + 1 : StartSel, EndSel > 0 ? EndSel + 1 : EndSel);

			TextChanged(true);
		}

		delete[] TmpStr;
		CurPtr = CurPtr->m_next;
		LineNum++;
		MoveLine = 0;
	}

	AddUndoData(UNDO_END);
}

void Editor::DeleteVBlock()
{
	if (Flags.Check(FEDITOR_LOCKMODE) || m_VBlockSizeX <= 0 || m_VBlockSizeY <= 0)
		return;

	AddUndoData(UNDO_BEGIN);

	if (!m_EdOpt.PersistentBlocks) {
		Edit *CurPtr = m_CurLine;
		Edit *NewTopScreen = m_TopScreen;

		while (CurPtr) {
			if (CurPtr == m_VBlockStart) {
				m_TopScreen = NewTopScreen;
				m_CurLine = CurPtr;
				CurPtr->SetCellCurPos(m_VBlockX);
				break;
			}

			m_NumLine--;

			if (NewTopScreen == CurPtr && CurPtr->m_prev)
				NewTopScreen = CurPtr->m_prev;

			CurPtr = CurPtr->m_prev;
		}
	}

	Edit *CurPtr = m_VBlockStart;

	for (int Line = 0; CurPtr && Line < m_VBlockSizeY; Line++, CurPtr = CurPtr->m_next) {
		TextChanged(true);
		int TBlockX = CurPtr->CellPosToReal(m_VBlockX);
		int TBlockSizeX = CurPtr->CellPosToReal(m_VBlockX + m_VBlockSizeX) - CurPtr->CellPosToReal(m_VBlockX);
		const wchar_t *CurStr, *EndSeq;
		int Length;
		CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);

		if (TBlockX >= Length)
			continue;

		AddUndoData(CurPtr, m_BlockStartLine + Line);
		wchar_t *TmpStr = new wchar_t[Length + 3];
		int CurLength = TBlockX;
		wmemcpy(TmpStr, CurStr, TBlockX);

		if (Length > TBlockX + TBlockSizeX) {
			int CopySize = Length - (TBlockX + TBlockSizeX);
			wmemcpy(TmpStr + CurLength, CurStr + TBlockX + TBlockSizeX, CopySize);
			CurLength+= CopySize;
		}

		int EndLength = StrLength(EndSeq);
		wmemcpy(TmpStr + CurLength, EndSeq, EndLength);
		CurLength+= EndLength;
		int CurPos = CurPtr->GetCurPos();
		CurPtr->SetBinaryString(TmpStr, CurLength);

		if (CurPos > TBlockX) {
			CurPos-= TBlockSizeX;

			if (CurPos < TBlockX)
				CurPos = TBlockX;
		}

		CurPtr->SetCurPos(CurPos);
		delete[] TmpStr;
	}

	AddUndoData(UNDO_END);
	m_VBlockStart = nullptr;
}

void Editor::VCopy(bool Append)
{
	wchar_t *CopyData = nullptr;

	Clipboard clip;

	if (!clip.Open())
		return;

	if (Append)
		CopyData = clip.Paste();

	if ((CopyData = VBlock2Text(CopyData))) {
		clip.Copy(CopyData, true);
		free(CopyData);
	}

	clip.Close();
}

wchar_t *Editor::VBlock2Text(wchar_t *ptrInitData)
{
	size_t DataSize = 0;

	if (ptrInitData)
		DataSize = wcslen(ptrInitData);

	// RealPos всегда <= TabPos, поэтому берём максимальный размер буффера
	size_t TotalChars = DataSize + (m_VBlockSizeX + wcslen(NATIVE_EOLW)) * m_VBlockSizeY + 1;

	wchar_t *CopyData = (wchar_t *)malloc(TotalChars * sizeof(wchar_t));

	if (!CopyData) {
		if (ptrInitData)
			free(ptrInitData);

		return nullptr;
	}

	if (ptrInitData) {
		wcscpy(CopyData, ptrInitData);
		free(ptrInitData);
	} else {
		*CopyData = 0;
	}

	Edit *CurPtr = m_VBlockStart;

	for (int Line = 0; CurPtr && Line < m_VBlockSizeY; Line++, CurPtr = CurPtr->m_next) {
		int TBlockX = CurPtr->CellPosToReal(m_VBlockX);
		int TBlockSizeX = CurPtr->CellPosToReal(m_VBlockX + m_VBlockSizeX) - TBlockX;
		const wchar_t *CurStr, *EndSeq;
		int Length;
		CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);

		if (Length > TBlockX) {
			int CopySize = Length - TBlockX;

			if (CopySize > TBlockSizeX)
				CopySize = TBlockSizeX;

			wmemcpy(CopyData + DataSize, CurStr + TBlockX, CopySize);

			if (CopySize < TBlockSizeX)
				wmemset(CopyData + DataSize + CopySize, L' ', TBlockSizeX - CopySize);
		} else {
			wmemset(CopyData + DataSize, L' ', TBlockSizeX);
		}

		DataSize+= TBlockSizeX;
		wcscpy(CopyData + DataSize, NATIVE_EOLW);
		DataSize+= wcslen(NATIVE_EOLW);
	}

	return CopyData;
}

void Editor::VPaste(wchar_t *ClipText)
{
	if (Flags.Check(FEDITOR_LOCKMODE))
		return;

	if (*ClipText) {
		AddUndoData(UNDO_BEGIN);
		Flags.Set(FEDITOR_NEWUNDO);
		TextChanged(true);
		int SaveOvertype = Flags.Check(FEDITOR_OVERTYPE);
		UnmarkBlock();
		m_Pasting++;
		Lock();

		if (Flags.Check(FEDITOR_OVERTYPE)) {
			Flags.Clear(FEDITOR_OVERTYPE);
			m_CurLine->SetOvertypeMode(false);
		}

		m_VBlockStart = m_CurLine;
		m_BlockStartLine = m_NumLine;
		int StartPos = m_CurLine->GetCellCurPos();
		m_VBlockX = StartPos;
		m_VBlockSizeX = 0;
		m_VBlockY = m_NumLine;
		m_VBlockSizeY = 0;
		Edit *SavedTopScreen = m_TopScreen;

		for (int I = 0; ClipText[I]; I++)
			if (ClipText[I] != '\r' && ClipText[I] != '\n') {
				ProcessKey(ClipText[I]);
			} else {
				const size_t EolLength = (ClipText[I] == '\r' || ClipText[I + 1] == '\n') ? 2 : 1;

				int CurWidth = m_CurLine->GetCellCurPos() - StartPos;

				if (CurWidth > m_VBlockSizeX)
					m_VBlockSizeX = CurWidth;

				m_VBlockSizeY++;

				if (!m_CurLine->m_next) {
					if (ClipText[I + EolLength]) {
						ProcessKey(KEY_END);
						ProcessKey(KEY_ENTER);

						// Mantis 0002966: Неправильная вставка вертикального блока в конце файла
						for (int I = 0; I < StartPos; I++)
							ProcessKey(L' ');
					}
				} else {
					ProcessKey(KEY_DOWN);
					m_CurLine->SetCellCurPos(StartPos);
					m_CurLine->SetOvertypeMode(false);
				}

				I+= EolLength - 1;
				continue;
			}

		int CurWidth = m_CurLine->GetCellCurPos() - StartPos;

		if (CurWidth > m_VBlockSizeX)
			m_VBlockSizeX = CurWidth;

		if (!m_VBlockSizeY)
			m_VBlockSizeY++;

		if (SaveOvertype) {
			Flags.Set(FEDITOR_OVERTYPE);
			m_CurLine->SetOvertypeMode(true);
		}

		m_TopScreen = SavedTopScreen;
		m_CurLine = m_VBlockStart;
		m_NumLine = m_BlockStartLine;
		m_CurLine->SetCellCurPos(StartPos);
		m_Pasting--;
		Unlock();
		AddUndoData(UNDO_END);
	}

	free(ClipText);
}

void Editor::VBlockShift(bool Left)
{
	if (Flags.Check(FEDITOR_LOCKMODE) || (Left && !m_VBlockX) || m_VBlockSizeX <= 0 || m_VBlockSizeY <= 0)
		return;

	Edit *CurPtr = m_VBlockStart;
	AddUndoData(UNDO_BEGIN);

	for (int Line = 0; CurPtr && Line < m_VBlockSizeY; Line++, CurPtr = CurPtr->m_next) {
		TextChanged(true);
		int TBlockX = CurPtr->CellPosToReal(m_VBlockX);
		int TBlockSizeX = CurPtr->CellPosToReal(m_VBlockX + m_VBlockSizeX) - CurPtr->CellPosToReal(m_VBlockX);
		const wchar_t *CurStr, *EndSeq;
		int Length;
		CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);

		if (TBlockX > Length)
			continue;

		if ((Left && CurStr[TBlockX - 1] == L'\t')
				|| (!Left && TBlockX + TBlockSizeX < Length && CurStr[TBlockX + TBlockSizeX] == L'\t')) {
			CurPtr->ExpandTabs();
			CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);
			TBlockX = CurPtr->CellPosToReal(m_VBlockX);
			TBlockSizeX = CurPtr->CellPosToReal(m_VBlockX + m_VBlockSizeX) - CurPtr->CellPosToReal(m_VBlockX);
		}

		AddUndoData(CurPtr, m_BlockStartLine + Line);
		int StrLen = Max(Length, TBlockX + TBlockSizeX + (Left ? 0 : 1));
		wchar_t *TmpStr = new wchar_t[StrLen + 3];
		wmemset(TmpStr, L' ', StrLen);
		wmemcpy(TmpStr, CurStr, Length);

		if (Left) {
			WCHAR Ch = TmpStr[TBlockX - 1];

			for (int I = TBlockX; I < TBlockX + TBlockSizeX; I++)
				TmpStr[I - 1] = TmpStr[I];

			TmpStr[TBlockX + TBlockSizeX - 1] = Ch;
		} else {
			int Ch = TmpStr[TBlockX + TBlockSizeX];

			for (int I = TBlockX + TBlockSizeX - 1; I >= TBlockX; I--)
				TmpStr[I + 1] = TmpStr[I];

			TmpStr[TBlockX] = Ch;
		}

		while (StrLen > 0 && TmpStr[StrLen - 1] == L' ')
			StrLen--;

		int EndLength = StrLength(EndSeq);
		wmemcpy(TmpStr + StrLen, EndSeq, EndLength);
		StrLen+= EndLength;
		CurPtr->SetBinaryString(TmpStr, StrLen);
		delete[] TmpStr;
	}

	m_VBlockX+= Left ? -1 : 1;
	m_CurLine->SetCellCurPos(Left ? m_VBlockX : m_VBlockX + m_VBlockSizeX);
	AddUndoData(UNDO_END);
}

int Editor::EditorControl(int Command, void *Param)
{
	_ECTLLOG(CleverSysLog SL(L"Editor::EditorControl()"));
	_ECTLLOG(SysLog(L"Command=%ls Param=[%d/0x%08X]", _ECTL_ToName(Command), Param, Param));

	switch (Command) {
		case ECTL_GETSTRING: {
			EditorGetString *GetString = (EditorGetString *)Param;

			if (GetString) {
				Edit *CurPtr = GetStringByNumber(GetString->StringNumber);

				if (!CurPtr) {
					_ECTLLOG(SysLog(L"EditorGetString => GetStringByNumber(%d) return nullptr",
							GetString->StringNumber));
					return FALSE;
				}

				CurPtr->GetBinaryString(const_cast<const wchar_t **>(&GetString->StringText),
						const_cast<const wchar_t **>(&GetString->StringEOL), GetString->StringLength);
				GetString->SelStart = -1;
				GetString->SelEnd = 0;
				int DestLine = GetString->StringNumber;

				if (DestLine == -1)
					DestLine = m_NumLine;

				if (m_BlockStart) {
					CurPtr->GetRealSelection(GetString->SelStart, GetString->SelEnd);
				} else if (m_VBlockStart && DestLine >= m_VBlockY && DestLine < m_VBlockY + m_VBlockSizeY) {
					GetString->SelStart = CurPtr->CellPosToReal(m_VBlockX);
					GetString->SelEnd = GetString->SelStart + CurPtr->CellPosToReal(m_VBlockX + m_VBlockSizeX)
							- CurPtr->CellPosToReal(m_VBlockX);
				}

				_ECTLLOG(SysLog(L"EditorGetString{"));
				_ECTLLOG(SysLog(L"  StringNumber    =%d", GetString->StringNumber));
				_ECTLLOG(SysLog(L"  StringText      ='%ls'", GetString->StringText));
				_ECTLLOG(SysLog(L"  StringEOL       ='%ls'",
						GetString->StringEOL ? _SysLog_LinearDump((LPBYTE)GetString->StringEOL,
								StrLength(GetString->StringEOL))
											 : L"(null)"));
				_ECTLLOG(SysLog(L"  StringLength    =%d", GetString->StringLength));
				_ECTLLOG(SysLog(L"  SelStart        =%d", GetString->SelStart));
				_ECTLLOG(SysLog(L"  SelEnd          =%d", GetString->SelEnd));
				_ECTLLOG(SysLog(L"}"));
				return TRUE;
			}

			break;
		}
		case ECTL_INSERTSTRING: {
			if (Flags.Check(FEDITOR_LOCKMODE)) {
				_ECTLLOG(SysLog(L"FEDITOR_LOCKMODE!"));
				return FALSE;
			}
			bool Indent = Param && *(int*)Param;

			if (!Indent)
				m_Pasting++;

			Flags.Set(FEDITOR_NEWUNDO);
			InsertString();

			if (!Indent)
				m_Pasting--;

			return TRUE;
		}
		case ECTL_INSERTTEXT:
		case ECTL_INSERTTEXT_V2: {
			if (!Param)
				return FALSE;

			if (Flags.Check(FEDITOR_LOCKMODE))
				return FALSE;

			const wchar_t *Str = (const wchar_t *)Param;
			m_Pasting++;
			Lock();

			for (; *Str; Str++) {
				if (Command == ECTL_INSERTTEXT_V2 && L'\n' == *Str) {
					--m_Pasting;
					InsertString();
					++m_Pasting;
				} else
					ProcessKey(*Str);
			}

			Unlock();
			m_Pasting--;

			return TRUE;
		}
		case ECTL_SETSTRING: {
			EditorSetString *SetString = (EditorSetString *)Param;

			if (!SetString)
				break;

			_ECTLLOG(SysLog(L"EditorSetString{"));
			_ECTLLOG(SysLog(L"  StringNumber    =%d", SetString->StringNumber));
			_ECTLLOG(SysLog(L"  StringText      ='%ls'", SetString->StringText));
			_ECTLLOG(SysLog(L"  StringEOL       ='%ls'",
					SetString->StringEOL ? _SysLog_LinearDump((LPBYTE)SetString->StringEOL,
							StrLength(SetString->StringEOL))
										 : L"(null)"));
			_ECTLLOG(SysLog(L"  StringLength    =%d", SetString->StringLength));
			_ECTLLOG(SysLog(L"}"));

			if (Flags.Check(FEDITOR_LOCKMODE)) {
				_ECTLLOG(SysLog(L"FEDITOR_LOCKMODE!"));
				break;
			} else {
				/* $ 06.08.2002 IS
				   Проверяем корректность StringLength и вернем FALSE, если оно меньше
				   нуля.
				*/
				int Length = SetString->StringLength;

				if (Length < 0) {
					_ECTLLOG(SysLog(L"SetString->StringLength < 0"));
					return FALSE;
				}

				Edit *CurPtr = GetStringByNumber(SetString->StringNumber);

				if (!CurPtr) {
					_ECTLLOG(SysLog(L"GetStringByNumber(%d) return nullptr", SetString->StringNumber));
					return FALSE;
				}

				const wchar_t *EOL = SetString->StringEOL ? SetString->StringEOL : m_GlobalEOL;

				int LengthEOL = StrLength(EOL);

				wchar_t *NewStr = (wchar_t *)malloc((Length + LengthEOL + 1) * sizeof(wchar_t));

				if (!NewStr) {
					_ECTLLOG(SysLog(L"malloc(%d) return nullptr", Length + LengthEOL + 1));
					return FALSE;
				}

				int DestLine = SetString->StringNumber;

				if (DestLine == -1)
					DestLine = m_NumLine;

				wmemcpy(NewStr, SetString->StringText, Length);
				wmemcpy(NewStr + Length, EOL, LengthEOL);
				AddUndoData(CurPtr, DestLine);
				int CurPos = CurPtr->GetCurPos();
				CurPtr->SetBinaryString(NewStr, Length + LengthEOL);
				CurPtr->SetCurPos(CurPos);
				TextChanged(true);    // 10.08.2000 skv - Modified->TextChanged
				free(NewStr);
			}

			return TRUE;
		}
		case ECTL_DELETESTRING: {
			if (Flags.Check(FEDITOR_LOCKMODE)) {
				_ECTLLOG(SysLog(L"FEDITOR_LOCKMODE!"));
				return FALSE;
			}

			TurnOffMarkingBlock();
			DeleteString(m_CurLine, m_NumLine, false, m_NumLine);
			return TRUE;
		}
		case ECTL_DELETECHAR: {
			if (Flags.Check(FEDITOR_LOCKMODE)) {
				_ECTLLOG(SysLog(L"FEDITOR_LOCKMODE!"));
				return FALSE;
			}

			TurnOffMarkingBlock();
			m_Pasting++;
			ProcessKey(KEY_DEL);
			m_Pasting--;
			return TRUE;
		}
		case ECTL_GETINFO: {
			EditorInfo *Info = (EditorInfo *)Param;

			if (Info) {
				Info->EditorID = m_EditorID;
				Info->WindowSizeX = ObjWidth;
				Info->WindowSizeY = Y2 - Y1 + 1;
				Info->TotalLines = m_NumLastLine;
				Info->CurLine = m_NumLine;
				Info->CurPos = m_CurLine->GetCurPos();
				Info->CurTabPos = m_CurLine->GetCellCurPos();
				Info->TopScreenLine = m_NumLine - CalcDistance(m_TopScreen, m_CurLine, -1);
				Info->LeftPos = m_CurLine->GetLeftPos();
				Info->Overtype = Flags.Check(FEDITOR_OVERTYPE);
				Info->BlockType = m_VBlockStart ? BTYPE_COLUMN : m_BlockStart ? BTYPE_STREAM : BTYPE_NONE;
				Info->BlockStartLine = Info->BlockType == BTYPE_NONE ? 0 : m_BlockStartLine;
				Info->Options = 0;

				if (m_EdOpt.ExpandTabs == EXPAND_ALLTABS)
					Info->Options|= EOPT_EXPANDALLTABS;

				if (m_EdOpt.ExpandTabs == EXPAND_NEWTABS)
					Info->Options|= EOPT_EXPANDONLYNEWTABS;

				if (m_EdOpt.PersistentBlocks)
					Info->Options|= EOPT_PERSISTENTBLOCKS;

				if (m_EdOpt.DelRemovesBlocks)
					Info->Options|= EOPT_DELREMOVESBLOCKS;

				if (m_EdOpt.AutoIndent)
					Info->Options|= EOPT_AUTOINDENT;

				if (m_EdOpt.SavePos)
					Info->Options|= EOPT_SAVEFILEPOSITION;

				if (m_EdOpt.AutoDetectCodePage)
					Info->Options|= EOPT_AUTODETECTCODEPAGE;

				if (m_EdOpt.CursorBeyondEOL)
					Info->Options|= EOPT_CURSORBEYONDEOL;

				if (m_EdOpt.ShowWhiteSpace)
					Info->Options|= EOPT_SHOWWHITESPACE;

				if (IsScrollbarShown())
					Info->Options|= EOPT_SHOWSCROLLBAR;

				Info->TabSize = m_EdOpt.TabSize;
				Info->BookMarkCount = POSCACHE_BOOKMARK_COUNT;
				Info->SessionBookmarkCount = GetStackBookmarks(nullptr);
				Info->CurState = Flags.Check(FEDITOR_LOCKMODE) ? ECSTATE_LOCKED : 0;
				Info->CurState|= !Flags.Check(FEDITOR_MODIFIED) ? ECSTATE_SAVED : 0;
				Info->CurState|= Flags.Check(FEDITOR_MODIFIED | FEDITOR_WASCHANGED) ? ECSTATE_MODIFIED : 0;
				Info->CodePage = m_codepage;

				Info->ClientArea = RECT { X1,Y1,X2,Y2 };
				Info->ClientArea.right += IsScrollbarShown() ? -1 : 0;

				return TRUE;
			}

			_ECTLLOG(SysLog(L"Error: !Param"));
			return FALSE;
		}
		case ECTL_GETFILENAME: {
			if (m_virtualFileName.IsEmpty())
				return 0;
			if (Param) {
				wcscpy(reinterpret_cast<LPWSTR>(Param), m_virtualFileName);
			}
			return static_cast<int>(m_virtualFileName.GetLength() + 1);
		}
		case ECTL_SETPOSITION: {
			// "Вначале было слово..."
			if (Param) {
				// ...а вот теперь поработаем с тем, что передалаи
				EditorSetPosition *Pos = (EditorSetPosition *)Param;
				_ECTLLOG(SysLog(L"EditorSetPosition{"));
				_ECTLLOG(SysLog(L"  m_CurLine       = %d", Pos->m_CurLine));
				_ECTLLOG(SysLog(L"  CurPos        = %d", Pos->CurPos));
				_ECTLLOG(SysLog(L"  CurTabPos     = %d", Pos->CurTabPos));
				_ECTLLOG(SysLog(L"  TopScreenLine = %d", Pos->TopScreenLine));
				_ECTLLOG(SysLog(L"  LeftPos       = %d", Pos->LeftPos));
				_ECTLLOG(SysLog(L"  Overtype      = %d", Pos->Overtype));
				_ECTLLOG(SysLog(L"}"));
				Lock();
				int CurPos = m_CurLine->GetCurPos();

				// выставим флаг об изменении поз (если надо)
				if ((Pos->CurLine >= 0 || Pos->CurPos >= 0)
						&& (Pos->CurLine != m_NumLine || Pos->CurPos != CurPos))
					Flags.Set(FEDITOR_CURPOSCHANGEDBYPLUGIN);

				if (Pos->CurLine >= 0)    // поменяем строку
				{
					if (Pos->CurLine == m_NumLine - 1)
						Up();
					else if (Pos->CurLine == m_NumLine + 1)
						Down();
					else
						GoToLine(Pos->CurLine);
				}

				if (Pos->TopScreenLine >= 0 && Pos->TopScreenLine <= m_NumLine) {
					m_TopScreen = m_CurLine;

					for (int I = m_NumLine; I > 0 && m_NumLine - I < Y2 - Y1 && I != Pos->TopScreenLine; I--)
						m_TopScreen = m_TopScreen->m_prev;
				}

				if (Pos->CurPos >= 0)
					m_CurLine->SetCurPos(Pos->CurPos);

				if (Pos->CurTabPos >= 0)
					m_CurLine->SetCellCurPos(Pos->CurTabPos);

				if (Pos->LeftPos >= 0)
					m_CurLine->SetLeftPos(Pos->LeftPos);

				/* $ 30.08.2001 IS
				   Изменение режима нужно выставлять сразу, в противном случае приходят
				   глюки, т.к. плагинописатель думает, что режим изменен, и ведет себя
				   соответствующе, в результате чего получает неопределенное поведение.
				*/
				if (Pos->Overtype >= 0) {
					Flags.Change(FEDITOR_OVERTYPE, Pos->Overtype);
					m_CurLine->SetOvertypeMode(Flags.Check(FEDITOR_OVERTYPE));
				}

				Unlock();
				return TRUE;
			}

			_ECTLLOG(SysLog(L"Error: !Param"));
			break;
		}
		case ECTL_SELECT: {
			if (Param) {
				EditorSelect *Sel = (EditorSelect *)Param;
				if (Sel->BlockType == BTYPE_NONE || Sel->BlockStartPos < 0) {
					fprintf(stderr, "ECTL_SELECT: unmark cuz Type=%d StartPos=%d\n", Sel->BlockType, Sel->BlockStartPos);
					UnmarkBlock();
					return TRUE;
				}
				return MarkBlockFromPlugin(Sel->BlockType == BTYPE_COLUMN, Sel->BlockStartLine,
						Sel->BlockStartPos, Sel->BlockWidth, Sel->BlockHeight);
			}
			fprintf(stderr, "ECTL_SELECT: !Param\n");
			break;
		}
		case ECTL_REDRAW: {
			Show();
			ScrBuf.Flush();
			return TRUE;
		}
		case ECTL_TABTOREAL: {
			if (Param) {
				EditorConvertPos *ecp = (EditorConvertPos *)Param;
				Edit *CurPtr = GetStringByNumber(ecp->StringNumber);

				if (!CurPtr) {
					_ECTLLOG(SysLog(L"GetStringByNumber(%d) return nullptr", ecp->StringNumber));
					return FALSE;
				}

				ecp->DestPos = CurPtr->CellPosToReal(ecp->SrcPos);
				_ECTLLOG(SysLog(L"EditorConvertPos{"));
				_ECTLLOG(SysLog(L"  StringNumber =%d", ecp->StringNumber));
				_ECTLLOG(SysLog(L"  SrcPos       =%d", ecp->SrcPos));
				_ECTLLOG(SysLog(L"  DestPos      =%d", ecp->DestPos));
				_ECTLLOG(SysLog(L"}"));
				return TRUE;
			}

			break;
		}
		case ECTL_REALTOTAB: {
			if (Param) {
				EditorConvertPos *ecp = (EditorConvertPos *)Param;
				Edit *CurPtr = GetStringByNumber(ecp->StringNumber);

				if (!CurPtr) {
					_ECTLLOG(SysLog(L"GetStringByNumber(%d) return nullptr", ecp->StringNumber));
					return FALSE;
				}

				ecp->DestPos = CurPtr->RealPosToCell(ecp->SrcPos);
				_ECTLLOG(SysLog(L"EditorConvertPos{"));
				_ECTLLOG(SysLog(L"  StringNumber =%d", ecp->StringNumber));
				_ECTLLOG(SysLog(L"  SrcPos       =%d", ecp->SrcPos));
				_ECTLLOG(SysLog(L"  DestPos      =%d", ecp->DestPos));
				_ECTLLOG(SysLog(L"}"));
				return TRUE;
			}

			break;
		}
		case ECTL_EXPANDTABS: {
			if (Flags.Check(FEDITOR_LOCKMODE)) {
				_ECTLLOG(SysLog(L"FEDITOR_LOCKMODE!"));
				return FALSE;
			} else {
				int StringNumber = *(int *)Param;
				Edit *CurPtr = GetStringByNumber(StringNumber);

				if (!CurPtr) {
					_ECTLLOG(SysLog(L"GetStringByNumber(%d) return nullptr", StringNumber));
					return FALSE;
				}

				AddUndoData(CurPtr, StringNumber);
				CurPtr->ExpandTabs();
			}

			return TRUE;
		}
		// TODO: Если DI_MEMOEDIT не будет юзать раскаску, то должно выполняется в FileEditor::EditorControl(), в диалоге - нафиг ненать
		case ECTL_ADDTRUECOLOR:
		case ECTL_ADDCOLOR: {
			if (Param) {
				const EditorColor *col = (EditorColor *)Param;
				_ECTLLOG(SysLog(L"EditorColor{"));
				_ECTLLOG(SysLog(L"  StringNumber=%d", col->StringNumber));
				_ECTLLOG(SysLog(L"  ColorItem   =%d (0x%08X)", col->ColorItem, col->ColorItem));
				_ECTLLOG(SysLog(L"  StartPos    =%d", col->StartPos));
				_ECTLLOG(SysLog(L"  EndPos      =%d", col->EndPos));
				_ECTLLOG(SysLog(L"  Color       =%d (0x%08X)", col->Color, col->Color));
				_ECTLLOG(SysLog(L"}"));
				ColorItem newcol{0};

				int xoff = X1; // was: = Flags.Check(FEDITOR_DIALOGMEMOEDIT) ? 0 : X1;
				newcol.StartPos = col->StartPos + (col->StartPos != -1 ? xoff : 0);
				newcol.EndPos = col->EndPos + xoff;
				newcol.Color = col->Color;
				newcol.Flags = col->Color & 0xFFFF0000;
				Edit *CurPtr = GetStringByNumber(col->StringNumber);

				if (!CurPtr) {
					_ECTLLOG(SysLog(L"GetStringByNumber(%d) return nullptr", col->StringNumber));
					return FALSE;
				}

				if (!col->Color)
					return (CurPtr->DeleteColor(newcol.StartPos));

				if (Command == ECTL_ADDTRUECOLOR) {
					const EditorTrueColor *tcol = (EditorTrueColor *)Param;
					FarTrueColorToAttributes(newcol.Color, tcol->TrueColor);
				}
				CurPtr->AddColor(&newcol);
				if (col->Color & ECF_AUTODELETE)
					m_AutoDeletedColors.insert(CurPtr);
				return TRUE;
			}

			break;
		}
		// TODO: Если DI_MEMOEDIT не будет юзать раскаску, то должно выполняется в FileEditor::EditorControl(), в диалоге - нафиг ненать
		case ECTL_GETTRUECOLOR:
		case ECTL_GETCOLOR: {
			if (Param) {
				EditorColor *col = (EditorColor *)Param;
				Edit *CurPtr = GetStringByNumber(col->StringNumber);

				if (!CurPtr) {
					_ECTLLOG(SysLog(L"GetStringByNumber(%d) return nullptr", col->StringNumber));
					return FALSE;
				}

				ColorItem curcol;

				if (!CurPtr->GetColor(&curcol, col->ColorItem)) {
					_ECTLLOG(SysLog(L"GetColor() return nullptr"));
					return FALSE;
				}

				int xoff = Flags.Check(FEDITOR_DIALOGMEMOEDIT) ? 0 : X1;
				col->StartPos = curcol.StartPos - xoff;
				col->EndPos = curcol.EndPos - xoff;
				col->Color = curcol.Color & 0xffff;
				if (Command == ECTL_GETTRUECOLOR) {
					EditorTrueColor *tcol = (EditorTrueColor *)Param;
					FarTrueColorFromAttributes(tcol->TrueColor, curcol.Color);
				}
				_ECTLLOG(SysLog(L"EditorColor{"));
				_ECTLLOG(SysLog(L"  StringNumber=%d", col->StringNumber));
				_ECTLLOG(SysLog(L"  ColorItem   =%d (0x%08X)", col->ColorItem, col->ColorItem));
				_ECTLLOG(SysLog(L"  StartPos    =%d", col->StartPos));
				_ECTLLOG(SysLog(L"  EndPos      =%d", col->EndPos));
				_ECTLLOG(SysLog(L"  Color       =%d (0x%08X)", col->Color, col->Color));
				_ECTLLOG(SysLog(L"}"));
				return TRUE;
			}

			break;
		}
		// должно выполняется в FileEditor::EditorControl()
		case ECTL_PROCESSKEY: {
			_ECTLLOG(SysLog(L"Key = %ls", _FARKEY_ToName((DWORD)Param)));
			ProcessKey((int)(INT_PTR)Param);
			return TRUE;
		}
		/* $ 16.02.2001 IS
			 Изменение некоторых внутренних настроек редактора. Param указывает на
			 структуру EditorSetParameter
		*/
		case ECTL_SETPARAM: {
			if (Param) {
				EditorSetParameter *espar = (EditorSetParameter *)Param;
				int rc = TRUE;
				_ECTLLOG(SysLog(L"EditorSetParameter{"));
				_ECTLLOG(SysLog(L"  Type        =%ls", _ESPT_ToName(espar->Type)));

				switch (espar->Type) {
					case ESPT_GETWORDDIV:
						_ECTLLOG(SysLog(L"  wszParam    =(%p)", espar->Param.wszParam));

						if (espar->Param.wszParam && espar->Size)
							far_wcsncpy(espar->Param.wszParam, m_EdOpt.strWordDiv, espar->Size);

						rc = (int)m_EdOpt.strWordDiv.GetLength() + 1;
						break;
					case ESPT_SETWORDDIV:
						_ECTLLOG(SysLog(L"  wszParam    =[%ls]", espar->Param.wszParam));
						SetWordDiv((!espar->Param.wszParam || !*espar->Param.wszParam)
										? Opt.strWordDiv.CPtr()
										: espar->Param.wszParam);
						break;
					case ESPT_TABSIZE:
						_ECTLLOG(SysLog(L"  iParam      =%d", espar->Param.iParam));
						SetTabSize(espar->Param.iParam);
						break;
					case ESPT_EXPANDTABS:
						_ECTLLOG(SysLog(L"  iParam      =%ls", espar->Param.iParam ? L"On" : L"Off"));
						SetConvertTabs(espar->Param.iParam);
						break;
					case ESPT_AUTOINDENT:
						_ECTLLOG(SysLog(L"  iParam      =%ls", espar->Param.iParam ? L"On" : L"Off"));
						SetAutoIndent(espar->Param.iParam);
						break;
					case ESPT_CURSORBEYONDEOL:
						_ECTLLOG(SysLog(L"  iParam      =%ls", espar->Param.iParam ? L"On" : L"Off"));
						SetCursorBeyondEOL(espar->Param.iParam);
						break;
					case ESPT_CHARCODEBASE:
						_ECTLLOG(SysLog(L"  iParam      =%ls",
								(!espar->Param.iParam
												? L"0 (Oct)"
												: (espar->Param.iParam == 1
																? L"1 (Dec)"
																: (espar->Param.iParam == 2 ? L"2 (Hex)"
																							: L"?????")))));
						SetCharCodeBase(espar->Param.iParam);
						break;
						/* $ 07.08.2001 IS сменить кодировку из плагина */
					case ESPT_CODEPAGE: {
						if ((UINT)espar->Param.iParam == CP_AUTODETECT)    // BUGBUG
						{
							rc = FALSE;
						} else if (!IsCodePageSupported(espar->Param.iParam)) {
							rc = FALSE;
						} else {
							if (m_HostFileEditor) {
								m_HostFileEditor->SetCodePage(espar->Param.iParam);
								m_HostFileEditor->CodepageChangedByUser();
							} else {
								SetCodePage(espar->Param.iParam);
							}

							Show();
						}
					} break;
					/* $ 29.10.2001 IS изменение настройки "Сохранять позицию файла" */
					case ESPT_SAVEFILEPOSITION:
						_ECTLLOG(SysLog(L"  iParam      =%ls", espar->Param.iParam ? L"On" : L"Off"));
						SetSavePosMode(espar->Param.iParam, -1);
						break;
						/* $ 23.03.2002 IS запретить/отменить изменение файла */
					case ESPT_LOCKMODE:
						_ECTLLOG(SysLog(L"  iParam      =%ls", espar->Param.iParam ? L"On" : L"Off"));
						Flags.Change(FEDITOR_LOCKMODE, espar->Param.iParam);
						break;
					case ESPT_SHOWWHITESPACE:
						SetShowWhiteSpace(espar->Param.iParam);
						break;
					default:
						_ECTLLOG(SysLog(L"}"));
						return FALSE;
				}

				_ECTLLOG(SysLog(L"}"));
				return rc;
			}

			return FALSE;
		}
		// Убрать флаг редактора "осуществляется выделение блока"
		case ECTL_TURNOFFMARKINGBLOCK: {
			TurnOffMarkingBlock();
			return TRUE;
		}
		case ECTL_DELETEBLOCK: {
			if (Flags.Check(FEDITOR_LOCKMODE) || !(m_VBlockStart || m_BlockStart)) {

				_ECTLLOG(if (Flags.Check(FEDITOR_LOCKMODE)) SysLog(L"FEDITOR_LOCKMODE!"));

				_ECTLLOG(if (!(m_VBlockStart || m_BlockStart)) SysLog(L"Not selected block!"));

				return FALSE;
			}

			TurnOffMarkingBlock();
			DeleteBlock();
			Show();
			return TRUE;
		}
		case ECTL_UNDOREDO: {
			if (Param) {
				EditorUndoRedo *eur = (EditorUndoRedo *)Param;

				switch (eur->Command) {
					case EUR_BEGIN:
						AddUndoData(UNDO_BEGIN);
						return TRUE;
					case EUR_END:
						AddUndoData(UNDO_END);
						return TRUE;
					case EUR_UNDO:
					case EUR_REDO:
						Lock();
						Undo(eur->Command == EUR_REDO);
						Unlock();
						return TRUE;
				}
			}

			return FALSE;
		}
	}

	return FALSE;
}

int Editor::SetBookmark(DWORD Pos)
{
	if (Pos < POSCACHE_BOOKMARK_COUNT) {
		m_SavePos.Line[Pos] = m_NumLine;
		m_SavePos.Cursor[Pos] = m_CurLine->GetCurPos();
		m_SavePos.LeftPos[Pos] = m_CurLine->GetLeftPos();
		m_SavePos.ScreenLine[Pos] = CalcDistance(m_TopScreen, m_CurLine, -1);
		return TRUE;
	}

	return FALSE;
}

int Editor::GotoBookmark(DWORD Pos)
{
	if (Pos < POSCACHE_BOOKMARK_COUNT) {
		if (m_SavePos.Line[Pos] != POS_NONE) {
			GoToLine(static_cast<int>(m_SavePos.Line[Pos]));
			m_CurLine->SetCurPos(static_cast<int>(m_SavePos.Cursor[Pos]));
			m_CurLine->SetLeftPos(static_cast<int>(m_SavePos.LeftPos[Pos]));
			m_TopScreen = m_CurLine;

			for (DWORD I = 0; I < m_SavePos.ScreenLine[Pos] && m_TopScreen->m_prev; I++)
				m_TopScreen = m_TopScreen->m_prev;

			if (!m_EdOpt.PersistentBlocks)
				UnmarkBlock();

			Show();
		}

		return TRUE;
	}

	return FALSE;
}

int Editor::ClearStackBookmarks()
{
	m_NewStackPos = false;

	if (m_StackPos) {
		InternalEditorStackBookMark *sb_prev = m_StackPos->prev, *sb_next;

		while (m_StackPos) {
			sb_next = m_StackPos->next;
			free(m_StackPos);
			m_StackPos = sb_next;
		}

		m_StackPos = sb_prev;

		while (m_StackPos) {
			sb_prev = m_StackPos->prev;
			free(m_StackPos);
			m_StackPos = sb_prev;
		}
	}

	return TRUE;
}

int Editor::DeleteStackBookmark(InternalEditorStackBookMark *sb_delete)
{
	m_NewStackPos = false;

	if (sb_delete) {
		if (sb_delete->next)
			sb_delete->next->prev = sb_delete->prev;

		if (sb_delete->prev)
			sb_delete->prev->next = sb_delete->next;

		if (m_StackPos == sb_delete)
			m_StackPos = (sb_delete->next) ? sb_delete->next : sb_delete->prev;

		free(sb_delete);
		return TRUE;
	}

	return FALSE;
}

int Editor::RestoreStackBookmark()
{
	m_NewStackPos = false;

	if (m_StackPos && ((int)m_StackPos->Line != m_NumLine || (int)m_StackPos->Cursor != m_CurLine->GetCurPos())) {
		GoToLine(m_StackPos->Line);
		m_CurLine->SetCurPos(m_StackPos->Cursor);
		m_CurLine->SetLeftPos(m_StackPos->LeftPos);
		m_TopScreen = m_CurLine;

		for (DWORD I = 0; I < m_StackPos->ScreenLine && m_TopScreen->m_prev; I++)
			m_TopScreen = m_TopScreen->m_prev;

		if (!m_EdOpt.PersistentBlocks)
			UnmarkBlock();

		Show();
		return TRUE;
	}

	return FALSE;
}

int Editor::AddStackBookmark(bool blNewPos)
{
	InternalEditorStackBookMark *sb_old = m_StackPos;

	if (m_StackPos && m_StackPos->next) {
		m_StackPos = m_StackPos->next;
		m_StackPos->prev = 0;
		ClearStackBookmarks();
		m_StackPos = sb_old;
		m_StackPos->next = 0;
	}

	InternalEditorStackBookMark *sb_new =
			(InternalEditorStackBookMark *)malloc(sizeof(InternalEditorStackBookMark));

	if (sb_new) {
		if (m_StackPos)
			m_StackPos->next = sb_new;

		m_StackPos = sb_new;
		m_StackPos->prev = sb_old;
		m_StackPos->next = 0;
		m_StackPos->Line = m_NumLine;
		m_StackPos->Cursor = m_CurLine->GetCurPos();
		m_StackPos->LeftPos = m_CurLine->GetLeftPos();
		m_StackPos->ScreenLine = CalcDistance(m_TopScreen, m_CurLine, -1);
		m_NewStackPos = blNewPos;    // We had to save current position, if we will go to previous bookmark (by default)
		return TRUE;
	}

	return FALSE;
}

InternalEditorStackBookMark *Editor::PointerToFirstStackBookmark(int *piCount)
{
	InternalEditorStackBookMark *sb_temp = m_StackPos;
	int iCount = 0;

	if (sb_temp) {
		for (; sb_temp->prev; iCount++)
			sb_temp = sb_temp->prev;
	}

	if (piCount)
		*piCount = iCount;

	return sb_temp;
}

InternalEditorStackBookMark *Editor::PointerToLastStackBookmark(int *piCount)
{
	InternalEditorStackBookMark *sb_temp = m_StackPos;
	int iCount = 0;

	if (sb_temp) {
		for (; sb_temp->next; iCount++)
			sb_temp = sb_temp->next;
		iCount++;
	}

	if (piCount)
		*piCount+= iCount;

	return sb_temp;
}

InternalEditorStackBookMark *Editor::PointerToStackBookmark(int iIdx)    // Returns null_ptr if failed!
{
	InternalEditorStackBookMark *sb_temp = m_StackPos;

	if (iIdx != -1 && sb_temp)    // -1 == current
	{
		while (sb_temp->prev)
			sb_temp = sb_temp->prev;

		for (int i = 0; i != iIdx && sb_temp; i++)
			sb_temp = sb_temp->next;
	}

	return sb_temp;
}

int Editor::BackStackBookmark()
{
	if (m_StackPos) {
		if (m_NewStackPos)    // If we had to save current position ...
		{
			m_NewStackPos = false;
			// ... if current bookmark is last and current_position != bookmark_position
			// save current position as new bookmark
			if (!m_StackPos->next
					&& ((int)m_StackPos->Line != m_NumLine || (int)m_StackPos->Cursor != m_CurLine->GetCurPos()))
				AddStackBookmark(false);
		}

		return PrevStackBookmark();
	}

	return FALSE;
}

int Editor::PrevStackBookmark()
{
	if (m_StackPos) {
		if (m_StackPos->prev)    // If not first bookmark - go
		{
			m_StackPos = m_StackPos->prev;
		}

		return RestoreStackBookmark();
	}

	return FALSE;
}

int Editor::NextStackBookmark()
{
	if (m_StackPos) {
		if (m_StackPos->next)    // If not last bookmark - go
		{
			m_StackPos = m_StackPos->next;
		}

		return RestoreStackBookmark();
	}

	return FALSE;
}

int Editor::LastStackBookmark()
{
	if (m_StackPos) {
		m_StackPos = PointerToLastStackBookmark();
		return RestoreStackBookmark();
	}

	return FALSE;
}

int Editor::GotoStackBookmark(int iIdx)
{
	if (m_StackPos) {
		InternalEditorStackBookMark *sb_temp = PointerToStackBookmark(iIdx);
		if (sb_temp) {
			m_StackPos = sb_temp;
			return RestoreStackBookmark();
		}
	}

	return FALSE;
}

int Editor::PushStackBookMark()
{
	m_StackPos = PointerToLastStackBookmark();
	return AddStackBookmark(false);
}

int Editor::PopStackBookMark()
{
	return (LastStackBookmark() && DeleteStackBookmark(m_StackPos));
}

int Editor::CurrentStackBookmarkIdx()
{
	int iIdx;
	if (PointerToFirstStackBookmark(&iIdx))
		return iIdx;
	return -1;
}

int Editor::GetStackBookmark(int iIdx, EditorBookMarks *Param)
{
	InternalEditorStackBookMark *sb_temp = PointerToStackBookmark(iIdx);

	if (sb_temp && Param) {
		if (Param->Line)
			Param->Line[0] = sb_temp->Line;
		if (Param->Cursor)
			Param->Cursor[0] = sb_temp->Cursor;
		if (Param->LeftPos)
			Param->LeftPos[0] = sb_temp->LeftPos;
		if (Param->ScreenLine)
			Param->ScreenLine[0] = sb_temp->ScreenLine;

		return TRUE;
	}

	return FALSE;
}

int Editor::GetStackBookmarks(EditorBookMarks *Param)
{
	int iCount = 0;

	if (m_StackPos) {
		InternalEditorStackBookMark *sb_temp = PointerToFirstStackBookmark(&iCount);
		PointerToLastStackBookmark(&iCount);

		if (Param) {
			if (Param->Line || Param->Cursor || Param->LeftPos || Param->ScreenLine) {
				for (int i = 0; i < iCount; i++) {
					if (Param->Line)
						Param->Line[i] = sb_temp->Line;

					if (Param->Cursor)
						Param->Cursor[i] = sb_temp->Cursor;

					if (Param->LeftPos)
						Param->LeftPos[i] = sb_temp->LeftPos;

					if (Param->ScreenLine)
						Param->ScreenLine[i] = sb_temp->ScreenLine;

					sb_temp = sb_temp->next;
				}
			} else
				iCount = 0;
		}
	}

	return iCount;
}

Edit *Editor::GetStringByNumber(int DestLine)
{
	if (DestLine == m_NumLine || DestLine < 0) {
		m_LastGetLine = m_CurLine;
		m_LastGetLineNumber = m_NumLine;
		return m_CurLine;
	}

	if (DestLine > m_NumLastLine)
		return nullptr;

	Edit *CurPtr = m_CurLine;
	int StartLine = m_NumLine;

	if (m_LastGetLine) {
		CurPtr = m_LastGetLine;
		StartLine = m_LastGetLineNumber;
	}

	bool Forward = (DestLine > StartLine && DestLine < StartLine + (m_NumLastLine - StartLine) / 2)
			|| (DestLine < StartLine / 2);

	if (DestLine > StartLine) {
		if (!Forward) {
			StartLine = m_NumLastLine - 1;
			CurPtr = m_EndList;
		}
	} else {
		if (Forward) {
			StartLine = 0;
			CurPtr = m_TopList;
		}
	}

	for (int Line = StartLine; Line != DestLine; Forward ? Line++ : Line--) {
		CurPtr = (Forward ? CurPtr->m_next : CurPtr->m_prev);
		if (!CurPtr) {
			m_LastGetLine = Forward ? m_TopList : m_EndList;
			m_LastGetLineNumber = Forward ? 0 : m_NumLastLine - 1;
			return nullptr;
		}
	}
	m_LastGetLine = CurPtr;
	m_LastGetLineNumber = DestLine;
	return CurPtr;
}

void Editor::SetReplaceMode(bool Mode)
{
	GlobalReplaceMode = Mode;
}

int Editor::GetLineCurPos()
{
	return m_CurLine->GetCellCurPos();
}

void Editor::BeginVBlockMarking()
{
	UnmarkBlock();
	m_VBlockStart = m_CurLine;
	m_VBlockX = m_CurLine->GetCellCurPos();
	m_VBlockSizeX = 0;
	m_VBlockY = m_NumLine;
	m_VBlockSizeY = 1;
	Flags.Set(FEDITOR_MARKINGVBLOCK);
	m_BlockStartLine = m_NumLine;
	//_D(SysLog(L"BeginVBlockMarking, set vblock to  m_VBlockY=%i:%i, m_VBlockX=%i:%i",m_VBlockY,m_VBlockSizeY,m_VBlockX,m_VBlockSizeX));
}

void Editor::AdjustVBlock(int PrevX)
{
	int x = GetLineCurPos();
	int c2;

	//_D(SysLog(L"AdjustVBlock, x=%i,   vblock is m_VBlockY=%i:%i, m_VBlockX=%i:%i, PrevX=%i",x,m_VBlockY,m_VBlockSizeY,m_VBlockX,m_VBlockSizeX,PrevX));
	if (x == m_VBlockX + m_VBlockSizeX)    // ничего не случилось, никаких табуляций нет
		return;

	if (x > m_VBlockX)    // курсор убежал внутрь блока
	{
		m_VBlockSizeX = x - m_VBlockX;
		//_D(SysLog(L"x>m_VBlockX");
	} else if (x < m_VBlockX)    // курсор убежал за начало блока
	{
		c2 = m_VBlockX;

		if (PrevX > m_VBlockX)    // сдвигались вправо, а пришли влево
		{
			m_VBlockX = x;
			m_VBlockSizeX = c2 - x;    // меняем блок
		} else                       // сдвигались влево и пришли еще больше влево
		{
			m_VBlockX = x;
			m_VBlockSizeX+= c2 - x;    // расширяем блок
		}

		//_D(SysLog(L"x<m_VBlockX"));
	} else if (x == m_VBlockX && x != PrevX) {
		m_VBlockSizeX = 0;    // ширина в 0, потому прыгнули прям на табуляцию
							//_D(SysLog(L"x==m_VBlockX && x!=PrevX"));
	}

	// примечание
	//   случай x>VBLockX+m_VBlockSizeX не может быть
	//   потому что курсор прыгает назад на табуляцию, но не вперед
	//_D(SysLog(L"AdjustVBlock, changed vblock  m_VBlockY=%i:%i, m_VBlockX=%i:%i",m_VBlockY,m_VBlockSizeY,m_VBlockX,m_VBlockSizeX));
}

void Editor::Xlat()
{
	Edit *CurPtr;
	int Line;
	bool DoXlat = false;
	AddUndoData(UNDO_BEGIN);

	if (m_VBlockStart) {
		CurPtr = m_VBlockStart;

		for (Line = 0; CurPtr && Line < m_VBlockSizeY; Line++, CurPtr = CurPtr->m_next) {
			int TBlockX = CurPtr->CellPosToReal(m_VBlockX);
			int TBlockSizeX = CurPtr->CellPosToReal(m_VBlockX + m_VBlockSizeX) - CurPtr->CellPosToReal(m_VBlockX);
			const wchar_t *CurStr, *EndSeq;
			int Length;
			CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);
			int CopySize = Length - TBlockX;

			if (CopySize > TBlockSizeX)
				CopySize = TBlockSizeX;

			AddUndoData(UNDO_EDIT, CurPtr->GetStringAddr(), CurPtr->GetEOL(), m_BlockStartLine + Line,
					m_CurLine->GetCurPos(), CurPtr->GetLength());
			auto Buf = CurPtr->m_Str.GetBuffer();
			::Xlat(Buf, TBlockX, TBlockX + CopySize, Opt.XLat.Flags);
			CurPtr->m_Str.ReleaseBuffer();
		}

		DoXlat = true;
	} else {
		Line = 0;
		CurPtr = m_BlockStart;

		// $ 25.11.2000 IS
		//     Если нет выделения, то обработаем текущее слово. Слово определяется на
		//     основе специальной группы разделителей.
		if (CurPtr) {
			while (CurPtr) {
				int StartSel, EndSel;
				CurPtr->GetSelection(StartSel, EndSel);

				if (StartSel == -1)
					break;

				if (EndSel == -1)
					EndSel = CurPtr->GetLength();    // StrLength(CurPtr->Str);

				AddUndoData(UNDO_EDIT, CurPtr->GetStringAddr(), CurPtr->GetEOL(), m_BlockStartLine + Line,
						m_CurLine->GetCurPos(), CurPtr->GetLength());
				auto Buf = CurPtr->m_Str.GetBuffer();
				::Xlat(Buf, StartSel, EndSel, Opt.XLat.Flags);
				CurPtr->m_Str.ReleaseBuffer();
				Line++;
				CurPtr = CurPtr->m_next;
			}

			DoXlat = true;
		} else {
			FARString &Str = m_CurLine->m_Str;
			int start = m_CurLine->GetCurPos(), end, StrSize = m_CurLine->GetLength();    // StrLength(Str);
			// $ 10.12.2000 IS
			//   Обрабатываем только то слово, на котором стоит курсор, или то слово,
			//   что находится левее позиции курсора на 1 символ
			DoXlat = true;

			if (IsWordDiv(Opt.XLat.strWordDivForXlat, Str[start])) {
				if (start)
					start--;

				DoXlat = !IsWordDiv(Opt.XLat.strWordDivForXlat, Str[start]);
			}

			if (DoXlat) {
				while (start >= 0 && !IsWordDiv(Opt.XLat.strWordDivForXlat, Str[start]))
					start--;

				start++;
				end = start + 1;

				while (end < StrSize && !IsWordDiv(Opt.XLat.strWordDivForXlat, Str[end]))
					end++;

				AddUndoData(UNDO_EDIT, m_CurLine->GetStringAddr(), m_CurLine->GetEOL(), m_NumLine, start,
						m_CurLine->GetLength());
				auto Buf = Str.GetBuffer();
				::Xlat(Buf, start, end, Opt.XLat.Flags);
				Str.ReleaseBuffer();
			}
		}
	}

	AddUndoData(UNDO_END);

	if (DoXlat)
		TextChanged(true);
}
/* SVS $ */

/* $ 15.02.2001 IS
	 Манипуляции с табуляцией на уровне всего загруженного файла.
	 Может быть длительной во времени операцией, но тут уж, imho,
	 ничего не поделать.
*/
// Обновим размер табуляции
void Editor::SetTabSize(int NewSize)
{
	if (NewSize < 1 || NewSize > 512 || NewSize == m_EdOpt.TabSize)
		return; /* Меняем размер табуляции только в том случае, если он
				   на самом деле изменился */

	m_EdOpt.TabSize = NewSize;
	Edit *CurPtr = m_TopList;

	while (CurPtr) {
		CurPtr->SetTabSize(NewSize);
		CurPtr = CurPtr->m_next;
	}
}

// обновим режим пробелы вместо табуляции
// операция необратима, кстати, т.е. пробелы на табуляцию обратно не изменятся
void Editor::SetConvertTabs(int NewMode)
{
	if (NewMode != m_EdOpt.ExpandTabs) /* Меняем режим только в том случае, если он
								на самом деле изменился */
	{
		m_EdOpt.ExpandTabs = NewMode;
		Edit *CurPtr = m_TopList;

		while (CurPtr) {
			CurPtr->SetConvertTabs(NewMode);

			if (NewMode == EXPAND_ALLTABS)
				CurPtr->ExpandTabs();

			CurPtr = CurPtr->m_next;
		}
	}
}

void Editor::SetDelRemovesBlocks(int NewMode)
{
	if (NewMode != m_EdOpt.DelRemovesBlocks) {
		m_EdOpt.DelRemovesBlocks = NewMode;
		Edit *CurPtr = m_TopList;

		while (CurPtr) {
			CurPtr->SetDelRemovesBlocks(NewMode);
			CurPtr = CurPtr->m_next;
		}
	}
}

void Editor::SetShowWhiteSpace(int NewMode)
{
	if (NewMode != m_EdOpt.ShowWhiteSpace) {
		m_EdOpt.ShowWhiteSpace = NewMode;

		for (Edit *CurPtr = m_TopList; CurPtr; CurPtr = CurPtr->m_next) {
			CurPtr->SetShowWhiteSpace(NewMode);
		}
	}
}

void Editor::SetPersistentBlocks(int NewMode)
{
	if (NewMode != m_EdOpt.PersistentBlocks) {
		m_EdOpt.PersistentBlocks = NewMode;
		Edit *CurPtr = m_TopList;

		while (CurPtr) {
			CurPtr->SetPersistentBlocks(NewMode);
			CurPtr = CurPtr->m_next;
		}
	}
}

//     "Курсор за пределами строки"
void Editor::SetCursorBeyondEOL(int NewMode)
{
	if (NewMode != m_EdOpt.CursorBeyondEOL) {
		m_EdOpt.CursorBeyondEOL = NewMode;
		Edit *CurPtr = m_TopList;

		while (CurPtr) {
			CurPtr->SetEditBeyondEnd(NewMode);
			CurPtr = CurPtr->m_next;
		}
	}

	/* $ 16.10.2001 SKV
	  Если переключились туда сюда этот режим,
	  то из-за этой штуки возникают нехилые глюки
	  при выделении вертикальных блоков.
	*/
	if (m_EdOpt.CursorBeyondEOL) {
		m_MaxRightPos = 0;
	}
}

void Editor::GetSavePosMode(int &SavePos, int &SaveShortPos)
{
	SavePos = m_EdOpt.SavePos;
	SaveShortPos = m_EdOpt.SaveShortPos;
}

// передавайте в качестве значения параметра "-1" для параметра,
// который не нужно менять
void Editor::SetSavePosMode(int SavePos, int SaveShortPos)
{
	if (SavePos != -1)
		m_EdOpt.SavePos = SavePos;

	if (SaveShortPos != -1)
		m_EdOpt.SaveShortPos = SaveShortPos;
}

void Editor::EditorShowMsg(const wchar_t *Title, const wchar_t *Msg, const wchar_t *Name, int Percent)
{
	if (Title == nullptr)
		return;

	FARString strProgress;

	if (Percent > -1) {
		FormatString strPercent;
		strPercent << Percent;

		size_t PercentLength = Max(strPercent.strValue().GetLength(), (size_t)3);
		size_t Length =
				Max(Min(static_cast<int>(MAX_WIDTH_MESSAGE - 2), StrLength(Name)), 40) - PercentLength - 2;
		wchar_t *Progress = strProgress.GetBuffer(Length);

		if (Progress) {
			size_t CurPos = Min(Percent, 100) * Length / 100;
			wmemset(Progress, BoxSymbols[BS_X_DB], CurPos);
			wmemset(Progress + (CurPos), BoxSymbols[BS_X_B0], Length - CurPos);
			strProgress.ReleaseBuffer(Length);
			FormatString strTmp;
			strTmp << L" " << fmt::Expand(PercentLength) << strPercent << L"%";
			strProgress+= strTmp;
		}
	}

	Message(0, 0, Title, Msg, Name, strProgress.IsEmpty() ? nullptr : strProgress.CPtr());
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Param1 = (void *)Title;
	preRedrawItem.Param.Param2 = (void *)Msg;
	preRedrawItem.Param.Param3 = (void *)Name;
	preRedrawItem.Param.Param4 = (void *)(INT_PTR)(Percent);
	PreRedraw.SetParam(preRedrawItem.Param);
}

void Editor::PR_EditorShowMsg()
{
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	Editor::EditorShowMsg((wchar_t *)preRedrawItem.Param.Param1, (wchar_t *)preRedrawItem.Param.Param2,
			(wchar_t *)preRedrawItem.Param.Param3, (int)(INT_PTR)preRedrawItem.Param.Param4);
}

Edit *Editor::CreateString(const wchar_t *lpwszStr, int nLength)
{
	Edit *pEdit = new (std::nothrow) Edit(this, nullptr);

	if (pEdit) {
		pEdit->SetTabSize(m_EdOpt.TabSize);
		pEdit->SetPersistentBlocks(m_EdOpt.PersistentBlocks);
		pEdit->SetConvertTabs(m_EdOpt.ExpandTabs);
		pEdit->SetCodePage(m_codepage);

		if (lpwszStr)
			pEdit->SetBinaryString(lpwszStr, nLength);

		pEdit->SetCurPos(0);
		pEdit->SetObjectColor(FarColorToReal(COL_EDITORTEXT), FarColorToReal(COL_EDITORSELECTEDTEXT));
		pEdit->SetEditorMode(TRUE);
		pEdit->SetWordDiv(m_EdOpt.strWordDiv);
		pEdit->SetShowWhiteSpace(m_EdOpt.ShowWhiteSpace);
	}

	return pEdit;
}

Edit *Editor::InsertString(const wchar_t *lpwszStr, int nLength, Edit *pAfter, int AfterLineNumber)
{
	Edit *pNewEdit = CreateString(lpwszStr, nLength);

	if (pNewEdit) {
		if (!m_TopList || !m_NumLastLine)    //???
			m_TopList = m_EndList = m_TopScreen = m_CurLine = pNewEdit;
		else {
			Edit *pWork = pAfter ? pAfter : m_EndList;
			Edit *pNext = pWork->m_next;
			pNewEdit->m_next = pNext;
			pNewEdit->m_prev = pWork;
			pWork->m_next = pNewEdit;

			if (pNext)
				pNext->m_prev = pNewEdit;

			if (!pAfter) {
				m_EndList = pNewEdit;
				AfterLineNumber = m_NumLastLine - 1;
			}
		}

		m_NumLastLine++;

		if (AfterLineNumber < m_LastGetLineNumber) {
			m_LastGetLineNumber++;
		}
	}

	return pNewEdit;
}

void Editor::SetCacheParams(EditorCacheParams *pp)
{
	bool translateTabs = false;
	m_SavePos = pp->SavePos;
	// m_codepage = pp->Table; //BUGBUG!!!, LoadFile do it itself

	if (m_StartLine == -2)    // from Viewer!
	{
		Edit *CurPtr = m_TopList;
		long TotalSize = 0;

		while (CurPtr && CurPtr->m_next) {
			const wchar_t *SaveStr, *EndSeq;
			int Length;
			CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);
			TotalSize+= Length + StrLength(EndSeq);

			if (TotalSize > m_StartChar)
				break;

			CurPtr = CurPtr->m_next;
			m_NumLine++;
		}

		m_TopScreen = m_CurLine = CurPtr;

		if (m_NumLine == pp->Line - pp->ScreenLine) {
			Lock();

			for (DWORD I = 0; I < (DWORD)pp->ScreenLine; I++)
				ProcessKey(KEY_DOWN);

			m_CurLine->SetCellCurPos(pp->LinePos);
			Unlock();
		}

		m_CurLine->SetLeftPos(pp->LeftPos);
	} else if (m_StartLine != -1 || m_EdOpt.SavePos) {
		if (m_StartLine != -1) {
			pp->Line = m_StartLine - 1;
			pp->ScreenLine = ObjHeight / 2;    // ScrY

			if (pp->ScreenLine > pp->Line)
				pp->ScreenLine = pp->Line;

			pp->LinePos = 0;
			if (m_StartChar > 0) {
				pp->LinePos = m_StartChar - 1;
				translateTabs = true;
			}
		}

		if (pp->ScreenLine > ObjHeight)    // ScrY //BUGBUG
			pp->ScreenLine = ObjHeight;    // ScrY;

		if (pp->Line >= pp->ScreenLine) {
			Lock();
			GoToLine(pp->Line - pp->ScreenLine);
			m_TopScreen = m_CurLine;

			for (int I = 0; I < pp->ScreenLine; I++)
				ProcessKey(KEY_DOWN);

			if (translateTabs)
				m_CurLine->SetCurPos(pp->LinePos);
			else
				m_CurLine->SetCellCurPos(pp->LinePos);
			m_CurLine->SetLeftPos(pp->LeftPos);
			Unlock();
		}
	}
}

void Editor::GetCacheParams(EditorCacheParams *pp)
{
	memset(pp, 0, sizeof(EditorCacheParams));
	memset(&pp->SavePos, 0xff, sizeof(InternalEditorBookMark));
	pp->Line = m_NumLine;
	pp->ScreenLine = CalcDistance(m_TopScreen, m_CurLine, -1);
	pp->LinePos = m_CurLine->GetCellCurPos();
	pp->LeftPos = m_CurLine->GetLeftPos();
	pp->CodePage = m_codepage;

	if (Opt.EdOpt.SaveShortPos) {
		pp->SavePos = m_SavePos;
	}
}

bool Editor::SetCodePage(UINT codepage)
{
	if (m_codepage != codepage) {
		m_codepage = codepage;
		Edit *current = m_TopList;
		DWORD Result = 0;

		while (current) {
			Result|= current->SetCodePage(m_codepage);
			current = current->m_next;
		}

		Show();
		return !Result;    // BUGBUG, more details
	}

	return true;
}

UINT Editor::GetCodePage()
{
	return m_codepage;
}

void Editor::SetDialogParent(DWORD Sets) {}

void Editor::SetPosition(int X1, int Y1, int X2, int Y2)
{
	ScreenObject::SetPosition(X1,Y1,X2,Y2);

	for(Edit *CurPtr=m_TopList; CurPtr; CurPtr=CurPtr->m_next)
	{
		CurPtr->SetPosition(X1,Y1,X2,Y2);
	}
}

void Editor::SetOvertypeMode(int Mode) {}

int Editor::GetOvertypeMode()
{
	return 0;
}

void Editor::SetEditBeyondEnd(int Mode) {}

void Editor::SetClearFlag(int Flag) {}

int Editor::GetClearFlag()
{
	return 0;
}

int Editor::GetCurCol()
{
	return m_CurLine->GetCurPos();
}

void Editor::SetCurPos(int NewCol, int NewRow)
{
	Lock();
	GoToLine(NewRow);
	m_CurLine->SetCellCurPos(NewCol);
	// m_CurLine->SetLeftPos(LeftPos); ???
	Unlock();
}

void Editor::SetCursorType(bool Visible, DWORD Size)
{
	m_CurLine->SetCursorType(Visible, Size);    //???
}

void Editor::GetCursorType(bool &Visible, DWORD &Size)
{
	m_CurLine->GetCursorType(Visible, Size);    //???
}

void Editor::SetObjectColor(uint64_t Color, uint64_t SelColor, uint64_t ColorUnChanged)
{
	for (Edit *CurPtr = m_TopList; CurPtr; CurPtr = CurPtr->m_next)    //???
		CurPtr->SetObjectColor(Color, SelColor, ColorUnChanged);
}

void Editor::DrawScrollbar()
{
	if (m_EdOpt.ShowScrollBar) {
		SetFarColor(COL_EDITORSCROLLBAR);
		m_XX2 = X2;
		int TopItem = m_NumLine - CalcDistance(m_TopScreen, m_CurLine, -1);
		if (ScrollBarEx(X2, Y1, Y2 - Y1 + 1, TopItem, m_NumLastLine))
			--m_XX2;
	}
}

void Editor::AutoDeleteColors()
{
	for (auto line : m_AutoDeletedColors) {
		line->AutoDeleteColors();
	}
	m_AutoDeletedColors.clear();
}

void Editor::TurnOffMarkingBlock()
{
	Flags.Clear(FEDITOR_MARKINGVBLOCK | FEDITOR_MARKINGBLOCK);
}

bool Editor::IsScrollbarShown() const
{
	return m_EdOpt.ShowScrollBar && ScrollBarRequired(ObjHeight, m_NumLastLine);
}

Editor* Editor::GetEditorById(int Id)
{
	auto It = IdMap.find(Id);
	return It == IdMap.end() ? nullptr : It->second;
}

// This function keeps Far3 compatibility of ECTL_SELECT
bool Editor::MarkBlockFromPlugin(bool SelVBlock, int SelStartLine, int SelStartPos, int SelWidth, int SelHeight)
{
	if (SelHeight < 1)
		return false;

	Edit *CurPtr = GetStringByNumber(SelStartLine);

	if (!CurPtr)
		return false;

	UnmarkBlock();

	if (SelVBlock) {
		Flags.Set(FEDITOR_MARKINGVBLOCK);
		m_VBlockStart = CurPtr;

		if ((m_BlockStartLine = SelStartLine) == -1)
			m_BlockStartLine = m_NumLine;

		m_VBlockX = SelStartPos;

		if ((m_VBlockY = SelStartLine) == -1)
			m_VBlockY = m_NumLine;

		m_VBlockSizeX = SelWidth;
		m_VBlockSizeY = SelHeight;

		if (m_VBlockSizeX < 0) {
			m_VBlockSizeX = -m_VBlockSizeX;
			m_VBlockX-= m_VBlockSizeX;

			if (m_VBlockX < 0)
				m_VBlockX = 0;
		}

	} else {
		Flags.Set(FEDITOR_MARKINGBLOCK);
		m_BlockStart = CurPtr;

		if ((m_BlockStartLine = SelStartLine) == -1)
			m_BlockStartLine = m_NumLine;

		for (int i = 0; i < SelHeight && CurPtr; i++) {
			int SelStart = i ? 0 : SelStartPos;
			int SelEnd = (i < SelHeight - 1) ? -1 : SelStartPos + SelWidth;
			CurPtr->Select(SelStart, SelEnd);
			CurPtr = CurPtr->m_next;
			// ранее было if (!CurPtr) return FALSE
		}
	}

	return true;
}
