/*
stddlg.cpp

Куча разных стандартных диалогов
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


#include "stddlg.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "dialog.hpp"
#include "ctrlobj.hpp"
#include "strmix.hpp"
#include "DlgGuid.hpp"
#include "message.hpp"
#include "RegExp.hpp"

static int PosSearchText = 2;
static int PosCheckBoxRegexp;

static LONG_PTR WINAPI SearchReplaceDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (Msg == DN_CLOSE && Param1 >= 0
			&& Param1 + 1 != reinterpret_cast<Dialog*>(hDlg)->ItemCount()) // button Cancel is the last element
	{
		const wchar_t *Txt = (const wchar_t*)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, PosSearchText);
		if (*Txt == 0)
		{
			SendDlgMessage(hDlg, DM_SETFOCUS, PosSearchText);
			Message(MSG_WARNING, 1, Msg::EditSearchTitle, Msg::EditEmptySearchField, Msg::Ok);
			return FALSE;
		}

		if (PosCheckBoxRegexp >= 0
				&& SendDlgMessage(hDlg, DM_GETCHECK, PosCheckBoxRegexp) == BSTATE_CHECKED)
		{
			RegExp Re;
			if (!CompileRegexp(Txt, 1, &Re)) {
				SendDlgMessage(hDlg, DM_SETFOCUS, PosSearchText);
				FARString strMsg(Txt);
				InsertQuote(strMsg);
				Message(MSG_WARNING, 1, Msg::EditSearchTitle, Msg::EditInvalidRegexp, strMsg, Msg::Ok);
				return FALSE;
			}
		}
	}
	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool GetSearchReplaceParams(
		bool IsReplaceMode,
		SearchReplaceDlgParams &Par,
		const wchar_t *TextHistoryName,
		const wchar_t *ReplaceHistoryName,
		const wchar_t *HelpTopic)
{
	PosCheckBoxRegexp = -1;

	static const auto TextHistoryName0 = L"SearchText";
	static const auto ReplaceHistoryName0 = L"ReplaceText";

	if (!TextHistoryName)
		TextHistoryName=TextHistoryName0;

	if (!ReplaceHistoryName)
		ReplaceHistoryName=ReplaceHistoryName0;

	if (IsReplaceMode)
	{
		enum {
			DBOX,
			LBSEARCH,
			EDSEARCH,
			LBREPLACE,
			EDREPLACE,
			SEP1,
			CBCASE,
			CBWHWORDS,
			CBREVERSE,
			CBREGEXP,
			SEP2,
			BTNREPLACE,
			BTNCANCEL,
		};
		/*
		  0         1         2         3         4         5         6         7
		  0123456789012345678901234567890123456789012345678901234567890123456789012345
		00
		01   +----------------------------- Replace ------------------------------+
		02   | Search for                                                         |
		03   |                                                                    |
		04   | Replace with                                                       |
		05   |                                                                    |
		06   +--------------------------------------------------------------------+
		07   | [ ] Case sensitive                 [ ] Regular expressions         |
		08   | [ ] Whole words                                                    |
		09   | [ ] Reverse search                                                 |
		10   +--------------------------------------------------------------------+
		11   |                      [ Replace ]  [ Cancel ]                       |
		12   +--------------------------------------------------------------------+
		13
		*/
		DialogDataEx ReplaceDlgData[]=
		{
			{DI_DOUBLEBOX,3,1,72,12,{},0,Msg::EditReplaceTitle},
			{DI_TEXT,5,2,0,2,{},0,Msg::EditSearchFor},
			{DI_EDIT,5,3,70,3,{},DIF_FOCUS|DIF_HISTORY|DIF_USELASTHISTORY,L""},
			{DI_TEXT,5,4,0,4,{},0,Msg::EditReplaceWith},
			{DI_EDIT,5,5,70,5,{},DIF_HISTORY|DIF_USELASTHISTORY,L""},
			{DI_TEXT,3,6,0,6,{},DIF_SEPARATOR,L""},
			{DI_CHECKBOX,5,7,0,7,{},0,Msg::EditSearchCase},
			{DI_CHECKBOX,5,8,0,8,{},0,Msg::EditSearchWholeWords},
			{DI_CHECKBOX,5,9,0,9,{},0,Msg::EditSearchReverse},
			{DI_CHECKBOX,40,7,0,7,{},0,Msg::EditSearchRegexp},
			{DI_TEXT,3,10,0,10,{},DIF_SEPARATOR,L""},
			{DI_BUTTON,0,11,0,11,{},DIF_DEFAULT|DIF_CENTERGROUP,Msg::EditReplaceReplace},
			{DI_BUTTON,0,11,0,11,{},DIF_CENTERGROUP,Msg::EditSearchCancel}
		};
		//индекс самого нижнего чекбокса каждой колонки в диалоге.
		//предполагаем, что чекбокс на позиции Y+1 имеет индекс, на единицу больший
		//чекбокса той же колонки на позиции Y.
		const int COL1_HIGH = CBREVERSE;
		const int COL2_HIGH = CBREGEXP;
		int HeightDialog = 14;
		int HeightCol1 = 3;
		int HeightCol2 = 1;
		const int HeightColMax = Max(HeightCol1, HeightCol2);
		MakeDialogItemsEx(ReplaceDlgData,ReplaceDlg);

		auto DeleteCheckBox = [&] (int Index, int Column)
		{
			const auto MaxIndex = (Column == 1) ? COL1_HIGH : COL2_HIGH;

			if (Column == 1) --HeightCol1; else --HeightCol2;

			ReplaceDlg[Index].Flags |= (DIF_HIDDEN | DIF_DISABLE);

			for (int I = Index+1; I <= MaxIndex; ++I)
			{
				ReplaceDlg[I].Y1--;
				ReplaceDlg[I].Y2--;
			}
		};

		if (!*TextHistoryName)
		{
			ReplaceDlg[EDSEARCH].strHistory.Clear();
			ReplaceDlg[EDSEARCH].Flags &= ~DIF_HISTORY;
		}
		else
			ReplaceDlg[EDSEARCH].strHistory=TextHistoryName;

		if (!*ReplaceHistoryName)
		{
			ReplaceDlg[EDREPLACE].strHistory.Clear();
			ReplaceDlg[EDREPLACE].Flags &= ~DIF_HISTORY;
		}
		else
			ReplaceDlg[EDREPLACE].strHistory=ReplaceHistoryName;

		ReplaceDlg[EDSEARCH].strData = Par.SearchStr;
		ReplaceDlg[EDREPLACE].strData = Par.ReplaceStr;

		if (Par.CaseSens >= 0)
			ReplaceDlg[CBCASE].Selected = Par.CaseSens;
		else
			DeleteCheckBox(CBCASE, 1);

		if (Par.WholeWords >= 0)
			ReplaceDlg[CBWHWORDS].Selected = Par.WholeWords;
		else
			DeleteCheckBox(CBWHWORDS, 1);

		if (Par.Reverse >= 0)
			ReplaceDlg[CBREVERSE].Selected = Par.Reverse;
		else
			DeleteCheckBox(CBREVERSE, 1);

		if (Par.Regexp >= 0)
		{
			PosCheckBoxRegexp = CBREGEXP;
			ReplaceDlg[CBREGEXP].Selected = Par.Regexp;
		}
		else
			DeleteCheckBox(CBREGEXP, 2);

		//сдвигаем кнопки
		int DeltaCol = HeightColMax - Max(HeightCol1, HeightCol2);

		if (DeltaCol > 0)
		{
			// нам не нужны 2 разделительных линии
			if (DeltaCol == HeightColMax)
				DeltaCol++;

			ReplaceDlg[DBOX].Y2 -= DeltaCol;
			HeightDialog -= DeltaCol;

			for (size_t I = SEP2; I < ARRAYSIZE(ReplaceDlgData); ++I)
			{
				ReplaceDlg[I].Y1 -= DeltaCol;
				ReplaceDlg[I].Y2 -= DeltaCol;
			}
		}

		{
			Dialog Dlg(ReplaceDlg, ARRAYSIZE(ReplaceDlgData), SearchReplaceDlgProc);
			Dlg.SetPosition(-1,-1,76,HeightDialog);
			Dlg.SetId(EditorReplaceId);

			if (HelpTopic && *HelpTopic)
				Dlg.SetHelp(HelpTopic);

			Dlg.Process();

			if (Dlg.GetExitCode() != BTNREPLACE)
				return false;
		}

		Par.SearchStr = ReplaceDlg[EDSEARCH].strData;

		Par.ReplaceStr = ReplaceDlg[EDREPLACE].strData;

		if (Par.CaseSens >= 0)
			Par.CaseSens = ReplaceDlg[CBCASE].Selected;

		if (Par.WholeWords >= 0)
			Par.WholeWords = ReplaceDlg[CBWHWORDS].Selected;

		if (Par.Reverse >= 0)
			Par.Reverse = ReplaceDlg[CBREVERSE].Selected;

		if (Par.Regexp >= 0)
			Par.Regexp = ReplaceDlg[CBREGEXP].Selected;
	}
	else
	{
		enum {
			DBOX,
			LBSEARCH,
			EDSEARCH,
			SEP1,
			CBCASE,
			CBWHWORDS,
			CBREVERSE,
			CBREGEXP,
			CBSELFOUND,
			SEP2,
			BTNSEARCH,
			BTNCANCEL,
		};
		/*
		  0         1         2         3         4         5         6         7
		  0123456789012345678901234567890123456789012345678901234567890123456789012345
		00
		01   +------------------------------ Search ------------------------------+
		02   | Search for                                                         |
		03   |                                                                    |
		04   +--------------------------------------------------------------------+
		05   | [ ] Case sensitive                 [ ] Regular expressions         |
		06   | [ ] Whole words                    [ ] Select found                |
		07   | [ ] Reverse search                                                 |
		08   +--------------------------------------------------------------------+
		09   |                       [ Search ]  [ Cancel ]                       |
		10   +--------------------------------------------------------------------+
		*/
		DialogDataEx SearchDlgData[]=
		{
			{DI_DOUBLEBOX,3,1,72,10,{},0,Msg::EditSearchTitle},
			{DI_TEXT,5,2,0,2,{},0,Msg::EditSearchFor},
			{DI_EDIT,5,3,70,3,{},DIF_FOCUS|DIF_HISTORY|DIF_USELASTHISTORY,L""},
			{DI_TEXT,3,4,0,4,{},DIF_SEPARATOR,L""},
			{DI_CHECKBOX,5,5,0,5,{},0,Msg::EditSearchCase},
			{DI_CHECKBOX,5,6,0,6,{},0,Msg::EditSearchWholeWords},
			{DI_CHECKBOX,5,7,0,7,{},0,Msg::EditSearchReverse},
			{DI_CHECKBOX,40,5,0,5,{},0,Msg::EditSearchRegexp},
			{DI_CHECKBOX,40,6,0,6,{},0,Msg::EditSearchSelFound},
			{DI_TEXT,3,8,0,8,{},DIF_SEPARATOR,L""},
			{DI_BUTTON,0,9,0,9,{},DIF_DEFAULT|DIF_CENTERGROUP,Msg::EditSearchSearch},
			{DI_BUTTON,0,9,0,9,{},DIF_CENTERGROUP,Msg::EditSearchCancel}
		};
		//индекс самого нижнего чекбокса каждой колонки в диалоге.
		//предполагаем, что чекбокс на позиции Y+1 имеет индекс, на единицу больший
		//чекбокса той же колонки на позиции Y.
		const int COL1_HIGH = CBREVERSE;
		const int COL2_HIGH = CBSELFOUND;
		int HeightDialog = 12;
		int HeightCol1 = 3;
		int HeightCol2 = 2;
		const int HeightColMax = Max(HeightCol1, HeightCol2);
		MakeDialogItemsEx(SearchDlgData,SearchDlg);

		auto DeleteCheckBox = [&] (int Index, int Column)
		{
			const auto MaxIndex = (Column == 1) ? COL1_HIGH : COL2_HIGH;

			if (Column == 1) --HeightCol1; else --HeightCol2;

			SearchDlg[Index].Flags |= (DIF_HIDDEN | DIF_DISABLE);

			for (int I = Index + 1; I <= MaxIndex; ++I)
			{
				SearchDlg[I].Y1--;
				SearchDlg[I].Y2--;
			}
		};

		if (!*TextHistoryName)
		{
			SearchDlg[EDSEARCH].strHistory.Clear();
			SearchDlg[EDSEARCH].Flags &= ~DIF_HISTORY;
		}
		else
			SearchDlg[EDSEARCH].strHistory=TextHistoryName;

		SearchDlg[EDSEARCH].strData = Par.SearchStr;

		if (Par.CaseSens >= 0)
			SearchDlg[CBCASE].Selected = Par.CaseSens;
		else
			DeleteCheckBox(CBCASE, 1);

		if (Par.WholeWords >= 0)
			SearchDlg[CBWHWORDS].Selected = Par.WholeWords;
		else
			DeleteCheckBox(CBWHWORDS, 1);

		if (Par.Reverse >= 0)
			SearchDlg[CBREVERSE].Selected = Par.Reverse;
		else
			DeleteCheckBox(CBREVERSE, 1);

		if (Par.Regexp >= 0)
		{
			PosCheckBoxRegexp = CBREGEXP;
			SearchDlg[CBREGEXP].Selected = Par.Regexp;
		}
		else
			DeleteCheckBox(CBREGEXP, 2);

		if (Par.SelectFound >= 0)
			SearchDlg[CBSELFOUND].Selected = Par.SelectFound;
		else
			DeleteCheckBox(CBSELFOUND, 2);

		//сдвигаем кнопки
		int DeltaCol = HeightColMax - Max(HeightCol1, HeightCol2);

		if (DeltaCol > 0)
		{
			// нам не нужны 2 разделительных линии
			if (DeltaCol == HeightColMax)
				DeltaCol++;

			SearchDlg[DBOX].Y2 -= DeltaCol;
			HeightDialog -= DeltaCol;

			for (size_t I = SEP2; I < ARRAYSIZE(SearchDlgData); ++I)
			{
				SearchDlg[I].Y1 -= DeltaCol;
				SearchDlg[I].Y2 -= DeltaCol;
			}
		}

		{
			Dialog Dlg(SearchDlg, ARRAYSIZE(SearchDlg), SearchReplaceDlgProc);
			Dlg.SetPosition(-1,-1,76,HeightDialog);
			Dlg.SetId(Par.Reverse >= 0 ? EditorSearchId : HelpSearchId);

			if (HelpTopic && *HelpTopic)
				Dlg.SetHelp(HelpTopic);

			Dlg.Process();

			if (Dlg.GetExitCode() != BTNSEARCH)
				return false;
		}

		Par.SearchStr = SearchDlg[EDSEARCH].strData;

		if (Par.CaseSens >= 0)
			Par.CaseSens = SearchDlg[CBCASE].Selected;

		if (Par.WholeWords >= 0)
			Par.WholeWords = SearchDlg[CBWHWORDS].Selected;

		if (Par.Reverse >= 0)
			Par.Reverse = SearchDlg[CBREVERSE].Selected;

		if (Par.Regexp >= 0)
			Par.Regexp = SearchDlg[CBREGEXP].Selected;

		if (Par.SelectFound)
			Par.SelectFound = SearchDlg[CBSELFOUND].Selected;
	}

	return true;
}


int WINAPI GetString(
    const wchar_t *Title,
    const wchar_t *Prompt,
    const wchar_t *HistoryName,
    const wchar_t *SrcText,
    FARString &strDestText,
    const wchar_t *HelpTopic,
    DWORD Flags,
    int *CheckBoxValue,
    const wchar_t *CheckBoxText,
    const GUID *Guid
)
{
	int Substract=5; // дополнительная величина :-)
	int ExitCode;
	bool addCheckBox=Flags&FIB_CHECKBOX && CheckBoxValue && CheckBoxText;
	int offset=addCheckBox?2:0;
	DialogDataEx StrDlgData[]=
	{
		{DI_DOUBLEBOX, 3, 1, 72, 4, {}, 0,                                L""},
		{DI_TEXT,      5, 2,  0, 2, {}, DIF_SHOWAMPERSAND,                L""},
		{DI_EDIT,      5, 3, 70, 3, {}, DIF_FOCUS|DIF_DEFAULT|(Flags&FIB_EDITPATH?DIF_EDITPATH:0),L""},
		{DI_TEXT,      0, 4,  0, 4, {}, DIF_SEPARATOR,                    L""},
		{DI_CHECKBOX,  5, 5,  0, 5, {}, 0,                                L""},
		{DI_TEXT,      0, 6,  0, 6, {}, DIF_SEPARATOR,                    L""},
		{DI_BUTTON,    0, 7,  0, 7, {}, DIF_CENTERGROUP,                  L""},
		{DI_BUTTON,    0, 7,  0, 7, {}, DIF_CENTERGROUP,                  L""}
	};
	MakeDialogItemsEx(StrDlgData,StrDlg);

	if (addCheckBox)
	{
		Substract-=2;
		StrDlg[0].Y2+=2;
		StrDlg[4].Selected=(*CheckBoxValue)?TRUE:FALSE;
		StrDlg[4].strData = CheckBoxText;
	}

	if (Flags&FIB_BUTTONS)
	{
		Substract-=3;
		StrDlg[0].Y2+=2;
		StrDlg[2].DefaultButton=FALSE;
		StrDlg[5+offset].Y1=StrDlg[4+offset].Y1=5+offset;
		StrDlg[4+offset].Type=StrDlg[5+offset].Type=DI_BUTTON;
		StrDlg[4+offset].Flags=StrDlg[5+offset].Flags=DIF_CENTERGROUP;
		StrDlg[4+offset].DefaultButton=TRUE;
		StrDlg[4+offset].strData = Msg::Ok;
		StrDlg[5+offset].strData = Msg::Cancel;
	}

	if (Flags&FIB_EXPANDENV)
	{
		StrDlg[2].Flags|=DIF_EDITEXPAND;
	}

	if (Flags&FIB_EDITPATH)
	{
		StrDlg[2].Flags|=DIF_EDITPATH;
	}

	if (HistoryName)
	{
		StrDlg[2].strHistory=HistoryName;
		StrDlg[2].Flags|=DIF_HISTORY|(Flags&FIB_NOUSELASTHISTORY?0:DIF_USELASTHISTORY);
	}

	if (Flags&FIB_PASSWORD)
		StrDlg[2].Type=DI_PSWEDIT;

	if (Title)
		StrDlg[0].strData = Title;

	if (Prompt)
	{
		StrDlg[1].strData = Prompt;
		TruncStrFromEnd(StrDlg[1].strData, 66);

		if (Flags&FIB_NOAMPERSAND)
			StrDlg[1].Flags&=~DIF_SHOWAMPERSAND;
	}

	if (SrcText)
		StrDlg[2].strData = SrcText;

	{
		Dialog Dlg(StrDlg,ARRAYSIZE(StrDlg)-Substract);
		Dlg.SetPosition(-1,-1,76,offset+((Flags&FIB_BUTTONS)?8:6));

		if (HelpTopic)
			Dlg.SetHelp(HelpTopic);

		if (Guid)
			Dlg.SetId(*Guid);

		Dlg.Process();

		ExitCode=Dlg.GetExitCode();
	}

	if (ExitCode == 2 || ExitCode == 4 || (addCheckBox && ExitCode == 6))
	{
		if (!(Flags&FIB_ENABLEEMPTY) && StrDlg[2].strData.IsEmpty())
			return FALSE;

		strDestText = StrDlg[2].strData;

		if (addCheckBox)
			*CheckBoxValue=StrDlg[4].Selected;

		return TRUE;
	}

	return FALSE;
}
