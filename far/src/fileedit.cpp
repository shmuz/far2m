/*
fileedit.cpp

Редактирование файла - надстройка над editor.cpp
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

#include "chgprior.hpp"
#include "cmdline.hpp"
#include "codepage.hpp"
#include "constitle.hpp"
#include "ctrlobj.hpp"
#include "datetime.hpp"
#include "dialog.hpp"
#include "dirmix.hpp"
#include "DlgGuid.hpp"
#include "exitcode.hpp"
#include "fileedit.hpp"
#include "filepanels.hpp"
#include "filestr.hpp"
#include "fileview.hpp"
#include "help.hpp"
#include "history.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "macroopcode.hpp"
#include "message.hpp"
#include "pathmix.hpp"
#include "savescr.hpp"
#include "scrbuf.hpp"
#include "syslog.hpp"
#include "TPreRedrawFunc.hpp"
#include "wakeful.hpp"

enum enumOpenEditor
{
	ID_OE_TITLE,
	ID_OE_OPENFILETITLE,
	ID_OE_FILENAME,
	ID_OE_SEPARATOR1,
	ID_OE_CODEPAGETITLE,
	ID_OE_CODEPAGE,
	ID_OE_SEPARATOR2,
	ID_OE_OK,
	ID_OE_CANCEL,
};

static const wchar_t *EOLName(const wchar_t *eol)
{
	if (wcscmp(eol, L"\n") == 0)
		return L"LF";

	if (wcscmp(eol, L"\r") == 0)
		return L"CR";

	if (wcscmp(eol, L"\r\n") == 0)
		return L"CRLF";

	if (wcscmp(eol, L"\r\r\n") == 0)
		return L"CRRLF";

	return eol;    // L"WTF";
}

LONG_PTR __stdcall hndOpenEditor(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	if (msg == DN_INITDIALOG) {
		int codepage = *(int *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
		FillCodePagesList(hDlg, ID_OE_CODEPAGE, codepage, true, false);
	}

	if (msg == DN_CLOSE) {
		if (param1 == ID_OE_OK) {
			int *param = (int *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			FarListPos pos;
			SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_OE_CODEPAGE, (LONG_PTR)&pos);
			*param = (int)SendDlgMessage(hDlg, DM_LISTGETDATA, ID_OE_CODEPAGE, pos.SelectPos);
			return TRUE;
		}
	}

	return DefDlgProc(hDlg, msg, param1, param2);
}

bool dlgOpenEditor(FARString &strFileName, UINT &codepage)
{
	const wchar_t *HistoryName = L"NewEdit";
	DialogDataEx EditDlgData[] = {
		{DI_DOUBLEBOX, 3,  1, 72, 8, {}, 0, Msg::EditTitle},
		{DI_TEXT,      5,  2, 0,  2, {}, 0, Msg::EditOpenCreateLabel},
		{DI_EDIT,      5,  3, 70, 3, {(DWORD_PTR)HistoryName}, DIF_FOCUS|DIF_HISTORY|DIF_EDITEXPAND|DIF_EDITPATH, L""},
		{DI_TEXT,      3,  4, 0,  4, {}, DIF_SEPARATOR, L""},
		{DI_TEXT,      5,  5, 0,  5, {}, 0, Msg::EditCodePage},
		{DI_COMBOBOX,  25, 5, 70, 5, {}, DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_LISTAUTOHIGHLIGHT, L""},
		{DI_TEXT,      3,  6, 0,  6, {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,    0,  7, 0,  7, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::Ok},
		{DI_BUTTON,    0,  7, 0,  7, {}, DIF_CENTERGROUP, Msg::Cancel}
	};
	MakeDialogItemsEx(EditDlgData, EditDlg);
	EditDlg[ID_OE_FILENAME].strData = strFileName;
	Dialog Dlg(EditDlg, ARRAYSIZE(EditDlg), (FARWINDOWPROC)hndOpenEditor, (LONG_PTR)&codepage);
	Dlg.SetPosition(-1, -1, 76, 10);
	Dlg.SetHelp(L"FileOpenCreate");
	Dlg.SetId(FileOpenCreateId);
	Dlg.Process();

	if (Dlg.GetExitCode() == ID_OE_OK) {
		strFileName = EditDlg[ID_OE_FILENAME].strData;
		ConvertHomePrefixInPath(strFileName);
		return true;
	}

	return false;
}

enum enumSaveFileAs
{
	ID_SF_TITLE,
	ID_SF_SAVEASFILETITLE,
	ID_SF_FILENAME,
	ID_SF_SEPARATOR1,
	ID_SF_CODEPAGETITLE,
	ID_SF_CODEPAGE,
	ID_SF_SIGNATURE,
	ID_SF_SEPARATOR2,
	ID_SF_SAVEASFORMATTITLE,
	ID_SF_DONOTCHANGE,
	ID_SF_DOS,
	ID_SF_UNIX,
	ID_SF_MAC,
	ID_SF_SEPARATOR3,
	ID_SF_OK,
	ID_SF_CANCEL,
};

LONG_PTR __stdcall hndSaveFileAs(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static UINT codepage = 0;

	switch (msg) {
		case DN_INITDIALOG: {
			codepage = *(UINT *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			FillCodePagesList(hDlg, ID_SF_CODEPAGE, codepage, false, false);

			if (IsUnicodeOrUtfCodePage(codepage)) {
				SendDlgMessage(hDlg, DM_ENABLE, ID_SF_SIGNATURE, TRUE);
			} else {
				SendDlgMessage(hDlg, DM_SETCHECK, ID_SF_SIGNATURE, BSTATE_UNCHECKED);
				SendDlgMessage(hDlg, DM_ENABLE, ID_SF_SIGNATURE, FALSE);
			}

			break;
		}
		case DN_CLOSE: {
			if (param1 == ID_SF_OK) {
				UINT *codepage = (UINT *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
				FarListPos pos;
				SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_SF_CODEPAGE, (LONG_PTR)&pos);
				*codepage = (UINT)SendDlgMessage(hDlg, DM_LISTGETDATA, ID_SF_CODEPAGE, pos.SelectPos);
				return TRUE;
			}

			break;
		}
		case DN_EDITCHANGE: {
			if (param1 == ID_SF_CODEPAGE) {
				FarListPos pos;
				SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_SF_CODEPAGE, (LONG_PTR)&pos);
				UINT Cp = static_cast<UINT>(
						SendDlgMessage(hDlg, DM_LISTGETDATA, ID_SF_CODEPAGE, pos.SelectPos));

				if (Cp != codepage) {
					codepage = Cp;

					if (IsUnicodeOrUtfCodePage(codepage)) {
						SendDlgMessage(hDlg, DM_SETCHECK, ID_SF_SIGNATURE, BSTATE_CHECKED);
						SendDlgMessage(hDlg, DM_ENABLE, ID_SF_SIGNATURE, TRUE);
					} else {
						SendDlgMessage(hDlg, DM_SETCHECK, ID_SF_SIGNATURE, BSTATE_UNCHECKED);
						SendDlgMessage(hDlg, DM_ENABLE, ID_SF_SIGNATURE, FALSE);
					}

					return TRUE;
				}
			}

			break;
		}
	}

	return DefDlgProc(hDlg, msg, param1, param2);
}

bool dlgSaveFileAs(FARString &strFileName, int &TextFormat, UINT &codepage, bool &AddSignature)
{
	const wchar_t *HistoryName = L"NewEdit";
	DialogDataEx EditDlgData[] = {
		{DI_DOUBLEBOX,   3,  1,  72, 15, {}, 0,         Msg::EditTitle},
		{DI_TEXT,        5,  2,  0,  2,  {}, 0,         Msg::EditSaveAs},
		{DI_EDIT,        5,  3,  70, 3,  {(DWORD_PTR)HistoryName}, DIF_FOCUS|DIF_HISTORY|DIF_EDITEXPAND|DIF_EDITPATH, L""},
		{DI_TEXT,        3,  4,  0,  4,  {}, DIF_SEPARATOR, L""},
		{DI_TEXT,        5,  5,  0,  5,  {}, 0,         Msg::EditCodePage},
		{DI_COMBOBOX,    25, 5,  70, 5,  {}, DIF_DROPDOWNLIST|DIF_LISTWRAPMODE|DIF_LISTAUTOHIGHLIGHT, L""},
		{DI_CHECKBOX,    5,  6,  0,  6,  {AddSignature}, DIF_DISABLE, Msg::EditAddSignature},
		{DI_TEXT,        3,  7,  0,  7,  {}, DIF_SEPARATOR, L""},
		{DI_TEXT,        5,  8,  0,  8,  {}, 0,         Msg::EditSaveAsFormatTitle},
		{DI_RADIOBUTTON, 5,  9,  0,  9,  {}, DIF_GROUP, Msg::EditSaveOriginal},
		{DI_RADIOBUTTON, 5,  10, 0,  10, {}, 0,         Msg::EditSaveDOS},
		{DI_RADIOBUTTON, 5,  11, 0,  11, {}, 0,         Msg::EditSaveUnix},
		{DI_RADIOBUTTON, 5,  12, 0,  12, {}, 0,         Msg::EditSaveMac},
		{DI_TEXT,        3,  13, 0,  13, {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,      0,  14, 0,  14, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::Ok},
		{DI_BUTTON,      0,  14, 0,  14, {}, DIF_CENTERGROUP, Msg::Cancel}
	};
	MakeDialogItemsEx(EditDlgData, EditDlg);
	EditDlg[ID_SF_FILENAME].strData =
			(/*Flags.Check(FFILEEDIT_SAVETOSAVEAS)?strFullFileName:strFileName*/ strFileName);
	{
		size_t pos = 0;
		if (EditDlg[ID_SF_FILENAME].strData.Pos(pos, Msg::NewFileName))
			EditDlg[ID_SF_FILENAME].strData.Truncate(pos);
	}
	EditDlg[ID_SF_DONOTCHANGE + TextFormat].Selected = TRUE;
	Dialog Dlg(EditDlg, ARRAYSIZE(EditDlg), (FARWINDOWPROC)hndSaveFileAs, (LONG_PTR)&codepage);
	Dlg.SetPosition(-1, -1, 76, 17);
	Dlg.SetHelp(L"FileSaveAs");
	Dlg.SetId(FileSaveAsId);
	Dlg.Process();

	if ((Dlg.GetExitCode() == ID_SF_OK) && !EditDlg[ID_SF_FILENAME].strData.IsEmpty()) {
		strFileName = EditDlg[ID_SF_FILENAME].strData;
		ConvertHomePrefixInPath(strFileName);
		AddSignature = EditDlg[ID_SF_SIGNATURE].Selected != 0;

		if (EditDlg[ID_SF_DONOTCHANGE].Selected)
			TextFormat = 0;
		else if (EditDlg[ID_SF_DOS].Selected)
			TextFormat = 1;
		else if (EditDlg[ID_SF_UNIX].Selected)
			TextFormat = 2;
		else if (EditDlg[ID_SF_MAC].Selected)
			TextFormat = 3;

		return true;
	}

	return false;
}

FileEditor::FileEditor(const wchar_t *Name, UINT codepage, DWORD InitFlags, int StartLine, int StartChar,
		const wchar_t *PluginData, int OpenModeExstFile)
	:
	BadConversion(false), SaveAsTextFormat(0)
{
	ScreenObject::SetPosition(0, 0, ScrX, ScrY);
	Flags.Set(InitFlags);
	Flags.Set(FFILEEDIT_FULLSCREEN);
	Init(Name, codepage, nullptr, InitFlags, StartLine, StartChar, PluginData, OpenModeExstFile);
}

FileEditor::FileEditor(const wchar_t *Name, UINT codepage, DWORD InitFlags, int StartLine, int StartChar,
		const wchar_t *Title, int X1, int Y1, int X2, int Y2, int OpenModeExstFile)
	:
	BadConversion(false), SaveAsTextFormat(0)
{
	Flags.Set(InitFlags);

	if (X1 < 0)
		X1 = 0;

	if (X2 < 0 || X2 > ScrX)
		X2 = ScrX;

	if (Y1 < 0)
		Y1 = 0;

	if (Y2 < 0 || Y2 > ScrY)
		Y2 = ScrY;

	if (X1 >= X2) {
		X1 = 0;
		X2 = ScrX;
	}

	if (Y1 >= Y2) {
		Y1 = 0;
		Y2 = ScrY;
	}

	ScreenObject::SetPosition(X1, Y1, X2, Y2);
	Flags.Change(FFILEEDIT_FULLSCREEN, (!X1 && !Y1 && X2 == ScrX && Y2 == ScrY));
	Init(Name, codepage, Title, InitFlags, StartLine, StartChar, L"", OpenModeExstFile);
}

/* $ 07.05.2001 DJ
   в деструкторе грохаем EditNamesList, если он был создан, а в SetNamesList()
   создаем EditNamesList и копируем туда значения
*/
/*
  Вызов деструкторов идет так:
	FileEditor::~FileEditor()
	Editor::~Editor()
	...
*/
FileEditor::~FileEditor()
{
	// AY: флаг оповещающий закрытие редактора.
	m_bClosing = true;

	if (m_editor->m_EdOpt.SavePos && CtrlObject)
		SaveToCache();

	if (bEE_READ_Sent && CtrlObject) {
		int FEditEditorID = m_editor->m_EditorID;
		FileEditor *save = CtrlObject->Plugins.CurEditor;
		CtrlObject->Plugins.CurEditor = this;
		CtrlObject->Plugins.ProcessEditorEvent(EE_CLOSE, &FEditEditorID);
		CtrlObject->Plugins.CurEditor = save;
	}

	if (m_editor)
		delete m_editor;

	m_editor = nullptr;

	if (EditNamesList)
		delete EditNamesList;
}

void FileEditor::Init(const wchar_t *Name, UINT codepage, const wchar_t *Title, DWORD InitFlags,
		int StartLine, int StartChar, const wchar_t *PluginData, int OpenModeExstFile)
{
	SudoClientRegion sdc_rgn;

	class SmartLock
	{
	private:
		Editor *editor;

	public:
		SmartLock(Editor *e) : editor(e) { editor->Lock(); }
		~SmartLock() { editor->Unlock(); }
	};

	SysErrorCode = 0;
	int BlankFileName = !StrCmp(Name, Msg::NewFileName);
	// AY: флаг оповещающий закрытие редактора.
	m_bClosing = false;
	bEE_READ_Sent = false;
	m_AddSignature = FB_NO;

	m_editor = new (std::nothrow) Editor;
	if (!m_editor) {
		ExitCode = XC_OPEN_ERROR;
		return;
	}
	SCOPED_ACTION(SmartLock)(m_editor);

	m_codepage = codepage;
	m_editor->SetOwner(this);
	m_editor->SetCodePage(m_codepage);
	*AttrStr = 0;
	SetTitle(Title);
	EditNamesList = nullptr;
	KeyBarVisible = Opt.EdOpt.ShowKeyBar;
	TitleBarVisible = Opt.EdOpt.ShowTitleBar;
	// $ 17.08.2001 KM - Добавлено для поиска по AltF7. При редактировании найденного файла из архива для клавиши F2 сделать вызов ShiftF2.
	Flags.Change(FFILEEDIT_SAVETOSAVEAS,
			(InitFlags & FFILEEDIT_SAVETOSAVEAS) == FFILEEDIT_SAVETOSAVEAS || BlankFileName != 0);

	if (!*Name) {
		ExitCode = XC_OPEN_ERROR;
		return;
	}

	SetPluginData(PluginData);
	m_editor->SetHostFileEditor(this);
	SetCanLoseFocus(Flags.Check(FFILEEDIT_ENABLEF6));
	apiGetCurrentDirectory(strStartDir);

	if (!SetFileName(Name)) {
		ExitCode = XC_OPEN_ERROR;
		return;
	}

	if (Flags.Check(FFILEEDIT_ENABLEF6)) {
		Frame *iFrame = FrameManager->FindFrameByFile(MODALTYPE_EDITOR, strFullFileName);

		if (iFrame) {
			int SwitchTo = FALSE;
			int MsgCode = 0;

			if (!iFrame->GetCanLoseFocus(true) || Opt.Confirm.AllowReedit) {
				if (OpenModeExstFile == FEOPMODE_QUERY) {
					SetMessageHelp(L"EditorReload");
					MsgCode = Message(0, 3, &EditorReloadId, Msg::EditTitle, strFullFileName, Msg::AskReload,
							Msg::Current, Msg::NewOpen, Msg::Reload);
				}
				else {
					switch (OpenModeExstFile) {
						case FEOPMODE_USEEXISTING: MsgCode = 0; break;
						case FEOPMODE_NEWIFOPEN:   MsgCode = 1; break;
						case FEOPMODE_RELOAD:      MsgCode = 2; break;
						default:                   MsgCode = -100; break;
					}
				}

				switch (MsgCode) {
					case 0:    // Current
						SwitchTo = TRUE;
						break;
					case 1:    // NewOpen
						SwitchTo = FALSE;
						break;
					case 2:    // Reload
						FrameManager->DeleteFrame(iFrame);
						SetExitCode(-2);
						break;
					case -100:
						SetExitCode(XC_EXISTS);
						return;
					default:
						SetExitCode(XC_QUIT);
						return;
				}
			}
			else {
				SwitchTo = TRUE;
			}

			if (SwitchTo) {
				FrameManager->ActivateFrame(iFrame);
				SetExitCode((OpenModeExstFile != FEOPMODE_QUERY) ? XC_EXISTS : TRUE);
				return;
			}
		}
	}

	/* $ 29.11.2000 SVS
	   Если файл имеет атрибут ReadOnly или System или Hidden,
	   И параметр на запрос выставлен, то сначала спросим.
	*/
	/* $ 03.12.2000 SVS
	   System или Hidden - задаются отдельно
	*/
	/* $ 15.12.2000 SVS
	  - Shift-F4, новый файл. Выдает сообщение :-(
	*/
	DWORD FAttr = apiGetFileAttributes(Name);

	/* $ 05.06.2001 IS
	   + посылаем подальше всех, кто пытается отредактировать каталог
	*/
	if (FAttr != INVALID_FILE_ATTRIBUTES && FAttr & FILE_ATTRIBUTE_DIRECTORY) {
		Message(MSG_WARNING, 1, &EditorCanNotEditDirectoryId, Msg::EditTitle, Msg::EditCanNotEditDirectory,
				Msg::Ok);
		ExitCode = XC_OPEN_ERROR;
		return;
	}

	if ((m_editor->m_EdOpt.ReadOnlyLock & 2) && IsLockAttributes(FAttr)) {
		if (Message(MSG_WARNING, 2, &EditorOpenRSHId, Msg::EditTitle, Name, Msg::EditRSH, Msg::EditROOpen,
					Msg::Yes, Msg::No)) {
			ExitCode = XC_OPEN_ERROR;
			return;
		}
	}

	m_editor->SetPosition(X1, Y1 + (Opt.EdOpt.ShowTitleBar ? 1 : 0), X2, Y2 - (Opt.EdOpt.ShowKeyBar ? 1 : 0));
	m_editor->SetStartPos(StartLine, StartChar);
	int UserBreak;

	/* $ 06.07.2001 IS
	   При создании файла с нуля так же посылаем плагинам событие EE_READ, дабы
	   не нарушать однообразие.
	*/
	if (FAttr == INVALID_FILE_ATTRIBUTES)
		Flags.Set(FFILEEDIT_NEW);

	if (BlankFileName && Flags.Check(FFILEEDIT_CANNEWFILE))
		Flags.Set(FFILEEDIT_NEW);

	if (Flags.Check(FFILEEDIT_NEW))
		m_AddSignature = FB_MAYBE;

	if (Flags.Check(FFILEEDIT_LOCKED))
		m_editor->Flags.Set(FEDITOR_LOCKMODE);

	if (!LoadFile(strFullFileName, UserBreak)) {
		if (BlankFileName) {
			Flags.Clear(FFILEEDIT_OPENFAILED);    // AY: ну так как редактор мы открываем то видимо надо и сбросить ошибку открытия
			UserBreak = 0;
		}

		if (!Flags.Check(FFILEEDIT_NEW) || UserBreak) {
			if (UserBreak != 1) {
				WINPORT(SetLastError)(SysErrorCode);
				Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::EditTitle, Msg::EditCannotOpen, strFileName,
						Msg::Ok);
				ExitCode = XC_OPEN_ERROR;
			} else {
				ExitCode = XC_LOADING_INTERRUPTED;
			}

			// Ахтунг. Ниже комментарии оставлены в назидании потомкам (до тех пор, пока не измениться манагер)
			// FrameManager->DeleteFrame(this); // BugZ#546 - Editor валит фар!
			// CtrlObject->Cp()->Redraw(); //AY: вроде как не надо, делает проблемы с проресовкой если в редакторе из истории попытаться выбрать несуществующий файл

			// если прервали загрузку, то фремы нужно проапдейтить, чтобы предыдущие месаги не оставались на экране
			if (!Opt.Confirm.Esc && UserBreak && ExitCode == XC_LOADING_INTERRUPTED && FrameManager)
				FrameManager->RefreshFrame();

			return;
		}

		if (m_codepage == CP_AUTODETECT)
			m_codepage = Opt.EdOpt.DefaultCodePage;

		m_editor->SetCodePage(m_codepage);
	}

	CtrlObject->Plugins.CurEditor = this;    //&FEdit;
	CtrlObject->Plugins.ProcessEditorEvent(EE_READ, nullptr);
	bEE_READ_Sent = true;
	ShowConsoleTitle();
	EditKeyBar.SetOwner(this);
	EditKeyBar.SetPosition(X1, Y2, X2, Y2);
	InitKeyBar();

	if (!Opt.EdOpt.ShowKeyBar)
		EditKeyBar.Hide0();

	MacroArea = MACROAREA_EDITOR;
	CtrlObject->Macro.SetArea(MACROAREA_EDITOR);

	F4KeyOnly = true;

	if (Flags.Check(FFILEEDIT_ENABLEF6))
		FrameManager->InsertFrame(this);
	else
		FrameManager->ExecuteFrame(this);
}

void FileEditor::InitKeyBar()
{
	EditKeyBar.SetAllGroup(KBL_MAIN, Opt.OnlyEditorViewerUsed ? Msg::SingleEditF1 : Msg::EditF1, 12);
	EditKeyBar.SetAllGroup(KBL_SHIFT, Opt.OnlyEditorViewerUsed ? Msg::SingleEditShiftF1 : Msg::EditShiftF1,
			12);
	EditKeyBar.SetAllGroup(KBL_ALT, Opt.OnlyEditorViewerUsed ? Msg::SingleEditAltF1 : Msg::EditAltF1, 12);
	EditKeyBar.SetAllGroup(KBL_CTRL, Opt.OnlyEditorViewerUsed ? Msg::SingleEditCtrlF1 : Msg::EditCtrlF1, 12);
	EditKeyBar.SetAllGroup(KBL_CTRLSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleEditCtrlShiftF1 : Msg::EditCtrlShiftF1, 12);
	EditKeyBar.SetAllGroup(KBL_CTRLALT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleEditCtrlAltF1 : Msg::EditCtrlAltF1, 12);
	EditKeyBar.SetAllGroup(KBL_ALTSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleEditAltShiftF1 : Msg::EditAltShiftF1, 12);
	EditKeyBar.SetAllGroup(KBL_CTRLALTSHIFT,
			Opt.OnlyEditorViewerUsed ? Msg::SingleEditCtrlAltShiftF1 : Msg::EditCtrlAltShiftF1, 12);

	if (!GetCanLoseFocus())
		EditKeyBar.Change(KBL_SHIFT, L"", 4 - 1);

	if (Flags.Check(FFILEEDIT_SAVETOSAVEAS))
		EditKeyBar.Change(KBL_MAIN, Msg::EditShiftF2, 2 - 1);

	if (!Flags.Check(FFILEEDIT_ENABLEF6))
		EditKeyBar.Change(KBL_MAIN, L"", 6 - 1);

	if (!GetCanLoseFocus())
		EditKeyBar.Change(KBL_MAIN, L"", 12 - 1);

	if (!GetCanLoseFocus())
		EditKeyBar.Change(KBL_ALT, L"", 11 - 1);

	if (!Opt.UsePrintManager || CtrlObject->Plugins.FindPlugin(SYSID_PRINTMANAGER))
		EditKeyBar.Change(KBL_ALT, L"", 5 - 1);

	ChangeEditKeyBar();

	EditKeyBar.ReadRegGroup(L"Editor", Opt.strLanguage);
	EditKeyBar.SetAllRegGroup();
	EditKeyBar.Refresh(true);
	m_editor->SetPosition(X1, Y1 + (Opt.EdOpt.ShowTitleBar ? 1 : 0), X2, Y2 - (Opt.EdOpt.ShowKeyBar ? 1 : 0));
	SetKeyBar(&EditKeyBar);
}

void FileEditor::SetNamesList(NamesList *Names)
{
	if (!EditNamesList)
		EditNamesList = new NamesList;

	Names->MoveData(*EditNamesList);
}

void FileEditor::Show()
{
	if (Flags.Check(FFILEEDIT_FULLSCREEN)) {
		if (Opt.EdOpt.ShowKeyBar) {
			EditKeyBar.SetPosition(0, ScrY, ScrX, ScrY);
			EditKeyBar.Redraw();
		}

		ScreenObject::SetPosition(0, 0, ScrX, ScrY - (Opt.EdOpt.ShowKeyBar ? 1 : 0));
		m_editor->SetPosition(0, (Opt.EdOpt.ShowTitleBar ? 1 : 0), ScrX,
				ScrY - (Opt.EdOpt.ShowKeyBar ? 1 : 0));
	}

	ScreenObject::Show();
}

void FileEditor::DisplayObject()
{
	if (!m_editor->Locked()) {
		WaitInMainLoop = FALSE;
		if (m_editor->Flags.Check(FEDITOR_ISRESIZEDCONSOLE)) {
			m_editor->Flags.Clear(FEDITOR_ISRESIZEDCONSOLE);
			CtrlObject->Plugins.CurEditor = this;
			CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW, EEREDRAW_CHANGE);    // EEREDRAW_ALL);
		}

		m_editor->Show();
	}
}

int64_t FileEditor::VMProcess(int OpCode, void *vParam, int64_t iParam)
{
	if (OpCode == MCODE_V_EDITORSTATE) {
		DWORD MacroEditState = 0;
		MacroEditState |= Flags.Flags & FFILEEDIT_NEW ? 0x00000001 : 0;
		MacroEditState |= Flags.Flags & FFILEEDIT_ENABLEF6 ? 0x00000002 : 0;
		MacroEditState |= m_editor->Flags.Flags & FEDITOR_MODIFIED ? 0x00000008 : 0;
		MacroEditState |= m_editor->m_BlockStart ? 0x00000010 : 0;
		MacroEditState |= m_editor->m_VBlockStart ? 0x00000020 : 0;
		MacroEditState |= m_editor->Flags.Flags & FEDITOR_WASCHANGED ? 0x00000040 : 0;
		MacroEditState |= m_editor->Flags.Flags & FEDITOR_OVERTYPE ? 0x00000080 : 0;
		MacroEditState |= m_editor->Flags.Flags & FEDITOR_CURPOSCHANGEDBYPLUGIN ? 0x00000100 : 0;
		MacroEditState |= m_editor->Flags.Flags & FEDITOR_LOCKMODE ? 0x00000200 : 0;
		MacroEditState |= m_editor->m_EdOpt.PersistentBlocks ? 0x00000400 : 0;
		MacroEditState |= Opt.OnlyEditorViewerUsed ? 0x08000000 | 0x00000800 : 0;
		MacroEditState |= !GetCanLoseFocus() ? 0x00000800 : 0;
		return (int64_t)MacroEditState;
	}

	if (OpCode == MCODE_V_EDITORCURPOS)
		return (int64_t)(m_editor->m_CurLine->GetCellCurPos() + 1);

	if (OpCode == MCODE_V_EDITORCURLINE)
		return (int64_t)(m_editor->m_NumLine + 1);

	if (OpCode == MCODE_V_ITEMCOUNT || OpCode == MCODE_V_EDITORLINES)
		return (int64_t)(m_editor->m_NumLastLine);

	if (OpCode == MCODE_F_KEYBAR_SHOW) {
		int PrevMode = Opt.EdOpt.ShowKeyBar ? 2 : 1;
		switch (iParam) {
			case 0:
				break;
			case 1:
				Opt.EdOpt.ShowKeyBar = 0;
				goto Label3;
			case 2:
				Opt.EdOpt.ShowKeyBar = 1;
				goto Label3;
			case 3:
			Label3:
				Opt.EdOpt.ShowKeyBar = !Opt.EdOpt.ShowKeyBar;

				if (!Opt.EdOpt.ShowKeyBar)
					EditKeyBar.Hide0();    // 0 mean - Don't purge saved screen

				EditKeyBar.Refresh(Opt.EdOpt.ShowKeyBar);
				Show();
				KeyBarVisible = Opt.EdOpt.ShowKeyBar;
				break;
			default:
				PrevMode = 0;
				break;
		}
		return PrevMode;
	}

	return m_editor->VMProcess(OpCode, vParam, iParam);
}

int FileEditor::ProcessKey(FarKey Key)
{
	return ReProcessKey(Key, FALSE);
}

int FileEditor::ReProcessKey(FarKey Key, int CalledFromControl)
{
	SudoClientRegion sdc_rgn;
	if (Key != KEY_F4 && Key != KEY_IDLE)
		F4KeyOnly = false;

	DWORD FNAttr;

	if (Flags.Check(FFILEEDIT_REDRAWTITLE)
			&& ((Key & 0x00ffffff) < KEY_END_FKEY || IS_INTERNAL_KEY_REAL(Key & 0x00ffffff)))
		ShowConsoleTitle();

	// BugZ#488 - Shift=enter
	if (ShiftPressed && (Key == KEY_ENTER || Key == KEY_NUMENTER) && !CtrlObject->Macro.IsExecuting()) {
		Key = Key == KEY_ENTER ? KEY_SHIFTENTER : KEY_SHIFTNUMENTER;
	}

	// Все сотальные необработанные клавиши пустим далее
	/* $ 28.04.2001 DJ
	   не передаем KEY_MACRO* плагину - поскольку ReadRec в этом случае
	   никак не соответствует обрабатываемой клавише, возникают разномастные
	   глюки
	*/
	if ((Key >= KEY_MACRO_BASE && Key <= KEY_MACRO_ENDBASE)
			|| (Key >= KEY_OP_BASE && Key <= KEY_OP_ENDBASE))    // исключаем MACRO
	{
		;                                                        //
	}

	switch (Key) {
			/* $ 27.09.2000 SVS
			   Печать файла/блока с использованием плагина PrintMan
			*/
		case KEY_ALTF5: {
			if (Opt.UsePrintManager && CtrlObject->Plugins.FindPlugin(SYSID_PRINTMANAGER)) {
				CtrlObject->Plugins.CallPlugin(SYSID_PRINTMANAGER, OPEN_EDITOR, nullptr);    // printman
				return TRUE;
			}

			break;    // отдадим Alt-F5 на растерзание плагинам, если не установлен PrintMan
		}
		case KEY_F6: {
			if (Flags.Check(FFILEEDIT_ENABLEF6)) {
				int FirstSave = 1, NeedQuestion = 1;
				UINT cp = m_codepage;

				// проверка на "а может это говно удалили уже?"
				// возможно здесь она и не нужна!
				// хотя, раз уж были изменени, то
				if (m_editor->IsFileChanged() &&                                             // в текущем сеансе были изменения?
						apiGetFileAttributes(strFullFileName) == INVALID_FILE_ATTRIBUTES)    // а файл еще существует?
				{
					switch (Message(MSG_WARNING, 2, &EditorSaveF6DeletedId, Msg::EditTitle,
							Msg::EditSavedChangedNonFile, Msg::EditSavedChangedNonFile2, Msg::HYes,
							Msg::HNo)) {
						case 0:

							if (ProcessKey(KEY_F2)) {
								FirstSave = 0;
								break;
							}

						default:
							return FALSE;
					}
				}

				if (!FirstSave || m_editor->IsFileChanged()
						|| apiGetFileAttributes(strFullFileName) != INVALID_FILE_ATTRIBUTES) {
					long FilePos = m_editor->GetCurPos();

					/* $ 01.02.2001 IS
					   ! Открываем вьюер с указанием длинного имени файла, а не короткого
					*/
					if (ProcessQuitKey(FirstSave, NeedQuestion)) {
						// объект будет в конце удалён в FrameManager
						auto *Viewer = new FileViewer(strFullFileName, GetCanLoseFocus(),
								Flags.Check(FFILEEDIT_DISABLEHISTORY), FALSE, FilePos, nullptr, EditNamesList,
								Flags.Check(FFILEEDIT_SAVETOSAVEAS), cp);
						Viewer->SetFileHolder(FHP);
						Viewer->SetPluginData(strPluginData);
					}

					ShowTime(2);
				}

				return TRUE;
			}

			break;    // отдадим F6 плагинам, если есть запрет на переключение
		}
		/* $ 10.05.2001 DJ
		   Alt-F11 - показать view/edit history
		*/
		case KEY_ALTF11: {
			if (GetCanLoseFocus()) {
				CtrlObject->CmdLine->ShowViewEditHistory();
				return TRUE;
			}

			break;    // отдадим Alt-F11 на растерзание плагинам, если редактор модальный
		}
	}

	BOOL ProcessedNext = TRUE;

	_SVS(if (Key == 'n' || Key == 'm'))
	_SVS(SysLog(L"%d Key='%c'", __LINE__, Key));

	if (!CalledFromControl && CtrlObject->Macro.CanSendKeysToPlugin()) {
		ProcessedNext = !ProcessEditorInput(FrameManager->GetLastInputRecord());
	}

	if (ProcessedNext) {

		switch (Key) {
			case KEY_F1: {
				Help Hlp(L"Editor");
				return TRUE;
			}
			/* $ 25.04.2001 IS
				 ctrl+f - вставить в строку полное имя редактируемого файла
			*/
			case KEY_CTRLF: {
				if (!m_editor->Flags.Check(FEDITOR_LOCKMODE)) {
					m_editor->m_Pasting++;
					m_editor->TextChanged(true);
					BOOL IsBlock = m_editor->m_VBlockStart || m_editor->m_BlockStart;

					if (!m_editor->m_EdOpt.PersistentBlocks && IsBlock) {
						m_editor->Flags.Clear(FEDITOR_MARKINGVBLOCK | FEDITOR_MARKINGBLOCK);
						m_editor->DeleteBlock();
					}

					// AddUndoData(CurLine->EditLine.GetStringAddr(),NumLine,
					//                 CurLine->EditLine.GetCurPos(),UNDO_EDIT);
					m_editor->Paste(strFullFileName);    //???
					// if (!EdOpt.PersistentBlocks)
					m_editor->UnmarkBlock();
					m_editor->m_Pasting--;
					m_editor->Show();    //???
				}

				return (TRUE);
			}
			/* $ 24.08.2000 SVS
			   + Добавляем реакцию показа бакграунда на клавишу CtrlAltShift
			*/
			case KEY_CTRLO: {
				if (!Opt.OnlyEditorViewerUsed) {
					m_editor->Hide();    // $ 27.09.2000 skv - To prevent redraw in macro with Ctrl-O

					if (FrameManager->ShowBackground()) {
						SetCursorType(false, 0);
						WaitKey();
					}

					Show();
				}

				return TRUE;
			}
			case KEY_F2:
			case KEY_SHIFTF2: {
				BOOL Done = FALSE;
				FARString strOldCurDir;
				apiGetCurrentDirectory(strOldCurDir);

				while (!Done)    // бьемся до упора
				{
					size_t pos;

					// проверим путь к файлу, может его уже снесли...
					if (FindLastSlash(pos, strFullFileName)) {
						wchar_t *lpwszPtr = strFullFileName.GetBuffer();
						wchar_t wChr = lpwszPtr[pos + 1];
						lpwszPtr[pos + 1] = 0;

						// В корне?
						if (!IsLocalRootPath(lpwszPtr)) {
							// а дальше? каталог существует?
							if ((FNAttr = apiGetFileAttributes(lpwszPtr)) == INVALID_FILE_ATTRIBUTES
									|| !(FNAttr & FILE_ATTRIBUTE_DIRECTORY)
									//|| LocalStricmp(OldCurDir,FullFileName)  // <- это видимо лишнее.
							)
								Flags.Set(FFILEEDIT_SAVETOSAVEAS);
						}

						lpwszPtr[pos + 1] = wChr;
						// strFullFileName.ReleaseBuffer (); так как ничего не поменялось то это лишнее.
					}

					if (Key == KEY_F2
							&& (FNAttr = apiGetFileAttributes(strFullFileName)) != INVALID_FILE_ATTRIBUTES
							&& !(FNAttr & FILE_ATTRIBUTE_DIRECTORY)) {
						Flags.Clear(FFILEEDIT_SAVETOSAVEAS);
					}

					UINT codepage = m_codepage;
					bool SaveAs = Key == KEY_SHIFTF2 || Flags.Check(FFILEEDIT_SAVETOSAVEAS);
					int NameChanged = FALSE;
					FARString strFullSaveAsName = strFullFileName;

					if (SaveAs) {
						FARString strSaveAsName =
								Flags.Check(FFILEEDIT_SAVETOSAVEAS) ? strFullFileName : strFileName;

						bool AddSignature = DecideAboutSignature();
						if (!dlgSaveFileAs(strSaveAsName, SaveAsTextFormat, codepage, AddSignature))
							return FALSE;

						m_AddSignature = AddSignature ? FB_YES : FB_NO;

						apiExpandEnvironmentStrings(strSaveAsName, strSaveAsName);
						NameChanged = StrCmpI(strSaveAsName,
								(Flags.Check(FFILEEDIT_SAVETOSAVEAS) ? strFullFileName : strFileName));

						if (!NameChanged)
							FarChDir(strStartDir);    // ПОЧЕМУ? А нужно ли???

						if (NameChanged) {
							if (!AskOverwrite(strSaveAsName)) {
								FarChDir(strOldCurDir);
								return TRUE;
							}
						}

						ConvertNameToFull(strSaveAsName, strFullSaveAsName);    // BUGBUG, не проверяем имя на правильность
						// это не про нас, про нас ниже, все куда страшнее
						/*FARString strFileNameTemp = strSaveAsName;

						if(!SetFileName(strFileNameTemp))
						{
						  WINPORT(SetLastError)(ERROR_INVALID_NAME);
										Message(MSG_WARNING|MSG_ERRORTYPE,1,Msg::EditTitle,strFileNameTemp,Msg::Ok);
						  if(!NameChanged)
							FarChDir(strOldCurDir);
						  continue;
						  //return FALSE;
						} */

						if (!NameChanged)
							FarChDir(strOldCurDir);
					}

					ShowConsoleTitle();
					FarChDir(strStartDir);    //???
					int SaveResult = SaveFile(strFullSaveAsName, 0, SaveAs, SaveAsTextFormat, codepage,
							DecideAboutSignature());

					if (SaveResult == SAVEFILE_ERROR) {
						WINPORT(SetLastError)(SysErrorCode);

						if (Message(MSG_WARNING | MSG_ERRORTYPE, 2, Msg::EditTitle, Msg::EditCannotSave,
									strFileName, Msg::Retry, Msg::Cancel)) {
							Done = TRUE;
							break;
						}
					} else if (SaveResult == SAVEFILE_SUCCESS) {
						// здесь идет полная жопа, проверка на ошибки вообще пока отсутствует
						{
							bool bInPlace = /*(!IsUnicodeOrUtfCodePage(m_codepage) && !IsUnicodeOrUtfCodePage(codepage)) || */
									(m_codepage == codepage);

							if (!bInPlace) {
								m_editor->ProcessKey(KEY_CTRLU);
								m_editor->FreeAllocatedData();
								m_editor->InsertString(nullptr, 0);
							}

							SetFileName(strFullSaveAsName);
							SetCodePage(codepage);    //

							if (!bInPlace) {
								Message(MSG_WARNING, 1, L"WARNING!",
										L"Editor will be reopened with new file!", Msg::Ok);
								int UserBreak;
								LoadFile(strFullSaveAsName, UserBreak);
								// TODO: возможно подобный ниже код здесь нужен (copy/paste из FileEditor::Init()). оформить его нужно по иному
								// if(!Opt.Confirm.Esc && UserBreak && ExitCode==XC_LOADING_INTERRUPTED && FrameManager)
								//  FrameManager->RefreshFrame();
							}

							// перерисовывать надо как минимум когда изменилась кодировка или имя файла
							ShowConsoleTitle();
							Show();    //!!! BUGBUG
						}
						Done = TRUE;
					} else {
						Done = TRUE;
						break;
					}
				}

				return TRUE;
			}
			// $ 30.05.2003 SVS - Shift-F4 в редакторе/вьювере позволяет открывать другой редактор/вьювер (пока только редактор)
			case KEY_SHIFTF4: {
				if (!Opt.OnlyEditorViewerUsed && GetCanLoseFocus())
					CtrlObject->Cp()->ActivePanel->ProcessKey(Key);

				return TRUE;
			}
			// $ 21.07.2000 SKV + выход с позиционированием на редактируемом файле по CTRLF10
			case KEY_CTRLF10: {
				if (isTemporary()) {
					return TRUE;
				}

				FARString strFullFileNameTemp = strFullFileName;

				if (apiGetFileAttributes(strFullFileName) == INVALID_FILE_ATTRIBUTES)    // а сам файл то еще на месте?
				{
					if (!CheckShortcutFolder(strFullFileNameTemp, false))
						return FALSE;

					strFullFileNameTemp+= L"/.";    // для вваливания внутрь :-)
				}

				Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;

				if (Flags.Check(FFILEEDIT_NEW)
						|| (ActivePanel && ActivePanel->FindFile(strFileName) == -1))    // Mantis#279
				{
					UpdateFileList();
					Flags.Clear(FFILEEDIT_NEW);
				}

				{
					SCOPED_ACTION(SaveScreen);
					CtrlObject->Cp()->GoToFile(strFullFileNameTemp);
					Flags.Set(FFILEEDIT_REDRAWTITLE);
				}

				return (TRUE);
			}
			case KEY_ALTF10: {
				FrameManager->ExitMainLoop(true);
				return (TRUE);
			}
			case KEY_CTRLB: {
				Opt.EdOpt.ShowKeyBar = !Opt.EdOpt.ShowKeyBar;

				if (!Opt.EdOpt.ShowKeyBar)
					EditKeyBar.Hide0();    // 0 mean - Don't purge saved screen

				EditKeyBar.Refresh(Opt.EdOpt.ShowKeyBar);
				Show();
				KeyBarVisible = Opt.EdOpt.ShowKeyBar;
				return (TRUE);
			}
			case KEY_CTRLSHIFTB: {
				Opt.EdOpt.ShowTitleBar = !Opt.EdOpt.ShowTitleBar;
				TitleBarVisible = Opt.EdOpt.ShowTitleBar;
				Show();
				return (TRUE);
			}
			case KEY_SHIFTF10:

				if (!ProcessKey(KEY_F2))    // учтем факт того, что могли отказаться от сохранения
					return FALSE;

			case KEY_F4:
				if (F4KeyOnly)
					return TRUE;
			case KEY_ESC:
			case KEY_F10: {
				int FirstSave = 1, NeedQuestion = 1;

				if (Key != KEY_SHIFTF10)    // KEY_SHIFTF10 не учитываем!
				{
					bool FilePlaced = apiGetFileAttributes(strFullFileName) == INVALID_FILE_ATTRIBUTES
							&& !Flags.Check(FFILEEDIT_NEW);

					if (m_editor->IsFileChanged() ||    // в текущем сеансе были изменения?
							FilePlaced)                 // а сам файл то еще на месте?
					{
						int Res;

						if (m_editor->IsFileChanged() && FilePlaced)
							Res = Message(MSG_WARNING, 3, &EditorSaveExitDeletedId, Msg::EditTitle,
									Msg::EditSavedChangedNonFile, Msg::EditSavedChangedNonFile2, Msg::HYes,
									Msg::HNo, Msg::HCancel);
						else if (!m_editor->IsFileChanged() && FilePlaced)
							Res = Message(MSG_WARNING, 3, &EditorSaveExitDeletedId, Msg::EditTitle,
									Msg::EditSavedChangedNonFile1, Msg::EditSavedChangedNonFile2, Msg::HYes,
									Msg::HNo, Msg::HCancel);
						else
							Res = 100;

						switch (Res) {
							case 0:

								if (!ProcessKey(KEY_F2))    // попытка сначала сохранить
									NeedQuestion = 0;

								FirstSave = 0;
								break;
							case 1:
								NeedQuestion = 0;
								FirstSave = 0;
								break;
							case 100:
								FirstSave = NeedQuestion = 1;
								break;
							case 2:
							default:
								return FALSE;
						}
					} else if (!m_editor->Flags.Check(FEDITOR_MODIFIED))    //????
						NeedQuestion = 0;
				}

				if (!ProcessQuitKey(FirstSave, NeedQuestion))
					return FALSE;

				return TRUE;
			}
			case KEY_F8:
			case KEY_SHIFTF8: {
				UINT codepage;
				if (Key == KEY_F8) {
					if (m_codepage == CP_UTF8)
						codepage = WINPORT(GetACP)();
					else if (m_codepage == WINPORT(GetACP)())
						codepage = WINPORT(GetOEMCP)();
					else
						codepage = CP_UTF8;
				} else {
					codepage = SelectCodePage(m_codepage, false, true, false, true);
					if (codepage == CP_AUTODETECT) {
						if (!GetFileFormat2(strFileName, codepage, nullptr, true, true)) {
							codepage = (UINT)-1;
						}
					}
				}
				if (codepage != (UINT)-1 && codepage != m_codepage) {
					const bool need_reload = 0
							//								|| IsFixedSingleCharCodePage(m_codepage) != IsFixedSingleCharCodePage(codepage)
							|| IsUTF8(m_codepage) != IsUTF8(codepage)
							|| IsUTF7(m_codepage) != IsUTF7(codepage)
							|| IsUTF16(m_codepage) != IsUTF16(codepage)
							|| IsUTF32(m_codepage) != IsUTF32(codepage);
					if (!IsFileModified() || !need_reload) {
						Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);
						if (need_reload) {
							m_editor->ProcessKey(KEY_CTRLU);
							m_codepage = codepage;
							int UserBreak = 0;
							SaveToCache();
							LoadFile(strLoadedFileName, UserBreak);
						} else {
							SetCodePage(codepage);
						}
						Show();    // need to force redraw after F8 UTF8<->ANSI/OEM
						ChangeEditKeyBar();
					} else
						Message(0, 1, Msg::EditTitle, L"Save file before changing this codepage", Msg::HOk,
								nullptr);
				}
				return TRUE;
			}
			case KEY_ALTSHIFTF9: {
				//     Работа с локальной копией EditorOptions
				EditorOptions EdOpt;
				GetEditorOptions(EdOpt);
				EditorConfig(EdOpt, true);    // $ 27.11.2001 DJ - Local в EditorConfig
				EditKeyBar.Refresh(true);     //???? Нужно ли????
				SetEditorOptions(EdOpt);

				EditKeyBar.Refresh(Opt.EdOpt.ShowKeyBar);

				m_editor->Show();
				return TRUE;
			}
			default: {
				if (Flags.Check(FFILEEDIT_FULLSCREEN) && !CtrlObject->Macro.IsExecuting())
					EditKeyBar.Refresh(Opt.EdOpt.ShowKeyBar);

				if (!EditKeyBar.ProcessKey(Key))
					return (m_editor->ProcessKey(Key));
			}
		}
	}
	return TRUE;
}

int FileEditor::ProcessQuitKey(int FirstSave, BOOL NeedQuestion)
{
	SudoClientRegion sdc_rgn;
	FARString strOldCurDir;
	apiGetCurrentDirectory(strOldCurDir);

	for (;;) {
		FarChDir(strStartDir);    // ПОЧЕМУ? А нужно ли???
		int SaveCode = SAVEFILE_SUCCESS;

		if (NeedQuestion) {
			SaveCode = SaveFile(strFullFileName, FirstSave, 0, FALSE);
		}

		if (SaveCode == SAVEFILE_CANCEL)
			break;

		if (SaveCode == SAVEFILE_SUCCESS) {
			/* $ 09.02.2002 VVM
			  + Обновить панели, если писали в текущий каталог */
			if (NeedQuestion) {
				if (apiGetFileAttributes(strFullFileName) != INVALID_FILE_ATTRIBUTES) {
					UpdateFileList();
				}
			}

			FrameManager->DeleteFrame();
			SetExitCode(XC_QUIT);
			break;
		}

		if (!StrCmp(strFileName, Msg::NewFileName)) {
			if (!ProcessKey(KEY_SHIFTF2)) {
				FarChDir(strOldCurDir);
				return FALSE;
			} else
				break;
		}

		WINPORT(SetLastError)(SysErrorCode);

		if (Message(MSG_WARNING | MSG_ERRORTYPE, 2, Msg::EditTitle, Msg::EditCannotSave, strFileName,
					Msg::Retry, Msg::Cancel))
			break;

		FirstSave = 0;
	}

	FarChDir(strOldCurDir);
	return (unsigned int)GetExitCode() == XC_QUIT;
}

int FileEditor::LoadFile(const wchar_t *Name, int &UserBreak)
{
	SudoClientRegion sdc_rgn;
	SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
	SCOPED_ACTION(TPreRedrawFuncGuard)(Editor::PR_EditorShowMsg);
	SCOPED_ACTION(wakeful);
	int LastLineCR = 0;
	EditorCacheParams cp;
	UserBreak = 0;
	File EditFile;
	DWORD FileAttr = apiGetFileAttributes(Name);
	if ((FileAttr != INVALID_FILE_ATTRIBUTES && (FileAttr & FILE_ATTRIBUTE_DEVICE) != 0) ||    // avoid stuck
			!EditFile.Open(Name, GENERIC_READ,
					FILE_SHARE_READ | (Opt.EdOpt.EditOpenedForWrite ? FILE_SHARE_WRITE : 0), nullptr,
					OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN)) {
		SysErrorCode = WINPORT(GetLastError)();
		if ((SysErrorCode != ERROR_FILE_NOT_FOUND) && (SysErrorCode != ERROR_PATH_NOT_FOUND)) {
			UserBreak = -1;
			Flags.Set(FFILEEDIT_OPENFAILED);
		} else if (m_codepage != CP_AUTODETECT && Flags.Check(FFILEEDIT_NEW)) {
			Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);
		}

		return FALSE;
	}

	/*if (GetFileType(hEdit) != FILE_TYPE_DISK)
	{
		fclose(EditFile);
		WINPORT(SetLastError)(ERROR_INVALID_NAME);
		UserBreak=-1;
		Flags.Set(FFILEEDIT_OPENFAILED);
		return FALSE;
	}*/

	if (Opt.EdOpt.FileSizeLimitLo || Opt.EdOpt.FileSizeLimitHi) {
		UINT64 FileSize = 0;
		if (EditFile.GetSize(FileSize)) {
			UINT64 MaxSize = Opt.EdOpt.FileSizeLimitHi * 0x100000000ull + Opt.EdOpt.FileSizeLimitLo;

			if (FileSize > MaxSize) {
				FARString strTempStr1, strTempStr2, strTempStr3, strTempStr4;
				// Ширина = 8 - это будет... в Kb и выше...
				FileSizeToStr(strTempStr1, FileSize, 8);
				FileSizeToStr(strTempStr2, MaxSize, 8);
				strTempStr3.Format(Msg::EditFileLong, RemoveExternalSpaces(strTempStr1).CPtr());
				strTempStr4.Format(Msg::EditFileLong2, RemoveExternalSpaces(strTempStr2).CPtr());

				if (Message(MSG_WARNING, 2, &EditorFileLongId, Msg::EditTitle, Name, strTempStr3, strTempStr4,
							Msg::EditROOpen, Msg::Yes, Msg::No)) {
					EditFile.Close();
					UserBreak = 1;
					Flags.Set(FFILEEDIT_OPENFAILED);
					errno = EFBIG;
					return FALSE;
				}
			}
		} else {
			ErrnoSaver ErSr;
			if (Message(MSG_WARNING | MSG_ERRORTYPE, 2, &EditorFileGetSizeErrorId, Msg::EditTitle, Name,
						Msg::EditFileGetSizeError, Msg::EditROOpen, Msg::Yes, Msg::No)) {
				EditFile.Close();
				UserBreak = 1;
				Flags.Set(FFILEEDIT_OPENFAILED);
				return FALSE;
			}
		}
	}

	m_editor->FreeAllocatedData(false);
	bool bCached = LoadFromCache(&cp);

	DWORD FileAttributes = apiGetFileAttributes(Name);
	if ((m_editor->m_EdOpt.ReadOnlyLock & 1) && IsLockAttributes(FileAttributes)) {
		m_editor->Flags.Swap(FEDITOR_LOCKMODE);
	}

	// Проверяем поддерживается или нет загруженная кодовая страница
	if (bCached && cp.CodePage && !IsCodePageSupported(cp.CodePage))
		cp.CodePage = 0;

	GetFileString GetStr(EditFile);
	wcscpy(m_editor->m_GlobalEOL, NATIVE_EOLW);
	wchar_t *Str;
	int StrLength, GetCode;
	UINT dwCP = 0;
	bool Detect = false;
	if (m_codepage == CP_AUTODETECT || IsUnicodeOrUtfCodePage(m_codepage)) {
		bool bSignatureDetected = false;
		Detect = GetFileFormat(EditFile, dwCP, &bSignatureDetected, Opt.EdOpt.AutoDetectCodePage != 0);

		// Проверяем поддерживается или нет задетектировання кодовая страница
		if (Detect) {
			Detect = IsCodePageSupported(dwCP);
			if (Detect) {
				m_AddSignature = (bSignatureDetected ? FB_YES : FB_NO);
			}
		}
	}

	if (m_codepage == CP_AUTODETECT) {
		if (Detect) {
			m_codepage = dwCP;
		}

		if (bCached) {
			if (cp.CodePage) {
				m_codepage = cp.CodePage;
				Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);
			}
		}

		if (m_codepage == CP_AUTODETECT)
			m_codepage = Opt.EdOpt.DefaultCodePage;
	} else {
		Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);
	}

	m_editor->SetCodePage(m_codepage);    // BUGBUG

	if (!IsUnicodeOrUtfCodePage(m_codepage)) {
		EditFile.SetPointer(0, nullptr, FILE_BEGIN);
	}

	UINT64 FileSize = 0;
	EditFile.GetSize(FileSize);
	DWORD StartTime = WINPORT(GetTickCount)();

	while ((GetCode = GetStr.GetString(&Str, m_codepage, StrLength))) {
		if (GetCode == -1) {
			EditFile.Close();
			return FALSE;
		}

		LastLineCR = 0;
		DWORD CurTime = WINPORT(GetTickCount)();

		if (CurTime - StartTime > RedrawTimeout) {
			StartTime = CurTime;

			if (CheckForEscSilent()) {
				if (ConfirmAbortOp()) {
					UserBreak = 1;
					EditFile.Close();
					return FALSE;
				}
			}

			SetCursorType(false, 0);
			INT64 CurPos = 0;
			EditFile.GetPointer(CurPos);
			int Percent = static_cast<int>(CurPos * 100 / FileSize);
			// В случае если во время загрузки файл увеличивается размере, то количество
			// процентов может быть больше 100. Обрабатываем эту ситуацию.
			if (Percent > 100) {
				EditFile.GetSize(FileSize);
				Percent = static_cast<int>(CurPos * 100 / FileSize);
				if (Percent > 100) {
					Percent = 100;
				}
			}
			Editor::EditorShowMsg(Msg::EditTitle, Msg::EditReading, Name, Percent);
		}

		const wchar_t *CurEOL;

		int Offset = StrLength > 3 ? StrLength - 3 : 0;

		if (!LastLineCR
				&& ((CurEOL = wmemchr(Str + Offset, L'\r', StrLength - Offset))
						|| (CurEOL = wmemchr(Str + Offset, L'\n', StrLength - Offset)))) {
			far_wcsncpy(m_editor->m_GlobalEOL, CurEOL, ARRAYSIZE(m_editor->m_GlobalEOL));
			m_editor->m_GlobalEOL[ARRAYSIZE(m_editor->m_GlobalEOL) - 1] = 0;
			LastLineCR = 1;
		}

		if (!m_editor->InsertString(Str, StrLength)) {
			EditFile.Close();
			return FALSE;
		}
	}

	BadConversion = !GetStr.IsConversionValid();
	if (BadConversion) {
		Message(MSG_WARNING, 1, Msg::Warning, Msg::EditorLoadCPWarn1, Msg::EditorLoadCPWarn2,
				Msg::EditorSaveNotRecommended, Msg::Ok);
	}

	if (LastLineCR || !m_editor->m_NumLastLine)
		m_editor->InsertString(L"", 0);

	EditFile.Close();
	// if ( bCached )
	m_editor->SetCacheParams(&cp);
	SysErrorCode = WINPORT(GetLastError)();
	apiGetFindDataForExactPathName(Name, FileInfo);
	EditorGetFileAttributes(Name);
	strLoadedFileName = Name;
	return TRUE;
}

// TextFormat и Codepage используются ТОЛЬКО, если bSaveAs = true!
void FileEditor::SaveContent(const wchar_t *Name, BaseContentWriter *Writer, bool bSaveAs, int TextFormat,
		UINT codepage, bool AddSignature, int Phase)
{
	DWORD dwSignature = 0;
	DWORD SignLength = 0;
	switch (codepage) {
		case CP_UTF32LE:
			dwSignature = SIGN_UTF32LE;
			SignLength = 4;
			if (!bSaveAs)
				AddSignature = (m_AddSignature != FB_NO);
			break;
		case CP_UTF32BE:
			dwSignature = SIGN_UTF32BE;
			SignLength = 4;
			if (!bSaveAs)
				AddSignature = (m_AddSignature != FB_NO);
			break;
		case CP_UTF16LE:
			dwSignature = SIGN_UTF16LE;
			SignLength = 2;
			if (!bSaveAs)
				AddSignature = (m_AddSignature != FB_NO);
			break;
		case CP_UTF16BE:
			dwSignature = SIGN_UTF16BE;
			SignLength = 2;
			if (!bSaveAs)
				AddSignature = (m_AddSignature != FB_NO);
			break;
		case CP_UTF8:
			dwSignature = SIGN_UTF8;
			SignLength = 3;
			if (!bSaveAs)
				AddSignature = (m_AddSignature == FB_YES);
			break;
	}
	if (AddSignature)
		Writer->Write(&dwSignature, SignLength);

	DWORD StartTime = WINPORT(GetTickCount)();
	size_t LineNumber = 0;

	for (Edit *CurPtr = m_editor->m_TopList; CurPtr; CurPtr = CurPtr->m_next, LineNumber++) {
		DWORD CurTime = WINPORT(GetTickCount)();

		if (CurTime - StartTime > RedrawTimeout) {
			StartTime = CurTime;
			if (Phase == 0)
				Editor::EditorShowMsg(Msg::EditTitle, Msg::EditSaving, Name,
						(int)(LineNumber * 50 / m_editor->m_NumLastLine));
			else
				Editor::EditorShowMsg(Msg::EditTitle, Msg::EditSaving, Name,
						(int)(50 + (LineNumber * 50 / m_editor->m_NumLastLine)));
		}

		const wchar_t *SaveStr, *EndSeq;

		int Length;

		CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);

		if (!*EndSeq && CurPtr->m_next)
			EndSeq = *m_editor->m_GlobalEOL ? m_editor->m_GlobalEOL : DOS_EOL_fmt;

		if (TextFormat && *EndSeq) {
			EndSeq = m_editor->m_GlobalEOL;
			CurPtr->SetEOL(EndSeq);
		}

		Writer->EncodeAndWrite(codepage, SaveStr, Length);
		Writer->EncodeAndWrite(codepage, EndSeq, StrLength(EndSeq));
	}
}

void FileEditor::BaseContentWriter::EncodeAndWrite(UINT codepage, const wchar_t *Str, size_t Length)
{
	if (!Length)
		return;

	if (codepage == CP_WIDE_LE) {
		Write(Str, Length * sizeof(wchar_t));
	} else if (codepage == CP_UTF8) {
		Wide2MB(Str, Length, _tmpstr);
		Write(_tmpstr.data(), _tmpstr.size());
	} else if (codepage == CP_WIDE_BE) {
		if (_tmpwstr.size() < Length)
			_tmpwstr.resize(Length + 0x20);

		WideReverse(Str, (wchar_t *)_tmpwstr.data(), Length);
		Write(_tmpwstr.data(), Length * sizeof(wchar_t));

	} else {
		int cnt = WINPORT(WideCharToMultiByte)(codepage, 0, Str, Length, nullptr, 0, nullptr, nullptr);
		if (cnt <= 0)
			return;

		if (_tmpstr.size() < (size_t)cnt)
			_tmpstr.resize(cnt + 0x20);

		cnt = WINPORT(WideCharToMultiByte)(codepage, 0, Str, Length, (char *)_tmpstr.data(), _tmpstr.size(),
				nullptr, nullptr);
		if (cnt > 0)
			Write(_tmpstr.data(), cnt);
	}
}

struct ContentMeasurer : FileEditor::BaseContentWriter
{
	INT64 MeasuredSize = 0;

	virtual void Write(const void *Data, size_t Length) { MeasuredSize+= Length; }
};

class ContentSaver : public FileEditor::BaseContentWriter
{
	CachedWrite CW;

public:
	ContentSaver(File &EditFile) : CW(EditFile) {}

	virtual void Write(const void *Data, size_t Length)
	{
		if (!CW.Write(Data, Length))
			throw WINPORT(GetLastError)();
	}

	void Flush()
	{
		if (!CW.Flush())
			throw WINPORT(GetLastError)();
	}
};

int FileEditor::SaveFile(const wchar_t *Name, int Ask, bool bSaveAs, int TextFormat, UINT codepage,
		bool AddSignature)
{
	SudoClientRegion sdc_rgn;
	if (!bSaveAs) {
		TextFormat = 0;
		codepage = m_editor->GetCodePage();
	}

	SCOPED_ACTION(wakeful);

	if (m_editor->Flags.Check(FEDITOR_LOCKMODE) && !m_editor->Flags.Check(FEDITOR_MODIFIED) && !bSaveAs)
		return SAVEFILE_SUCCESS;

	if (Ask) {
		if (!m_editor->Flags.Check(FEDITOR_MODIFIED))
			return SAVEFILE_SUCCESS;

		switch (Message(MSG_WARNING, 3, &EditAskSaveId, Msg::EditTitle, Msg::EditAskSave, Msg::HYes, Msg::HNo,
				Msg::HCancel)) {
			default:                             // Continue Edit
				return SAVEFILE_CANCEL;
			case 0:                              // Save
				break;
			case 1:                              // Don't Save
				m_editor->TextChanged(false);
				return SAVEFILE_SUCCESS;
		}
	}

	int NewFile = TRUE;

	FileUnmakeWritable = apiMakeWritable(Name);
	if (FileUnmakeWritable.get()) {
		// BUGBUG
		int AskOverwrite = Message(MSG_WARNING, 2, &EditorSavedROId, Msg::EditTitle, Name, Msg::EditRO,
				Msg::EditOvr, Msg::Yes, Msg::No);

		if (AskOverwrite) {
			FileUnmakeWritable->Unmake();
			FileUnmakeWritable.reset();
			return SAVEFILE_CANCEL;
		}
	}

	DWORD FileAttributes = EditorGetFileAttributes(Name);
	if (FileAttributes != INVALID_FILE_ATTRIBUTES) {
		// Проверка времени модификации...
		if (!Flags.Check(FFILEEDIT_SAVEWQUESTIONS)) {
			FAR_FIND_DATA_EX FInfo;

			if (apiGetFindDataForExactPathName(Name, FInfo) && !FileInfo.strFileName.IsEmpty()) {
				int64_t RetCompare = FileTimeDifference(&FileInfo.ftLastWriteTime, &FInfo.ftLastWriteTime);

				if (RetCompare || !(FInfo.nFileSize == FileInfo.nFileSize)) {
					SetMessageHelp(L"WarnEditorSavedEx");

					switch (Message(MSG_WARNING, 3, &EditAskSaveExtId, Msg::EditTitle, Msg::EditAskSaveExt,
							Msg::HYes, Msg::EditBtnSaveAs, Msg::HCancel)) {
						default:   // Continue Edit
							return SAVEFILE_CANCEL;

						case 1:    // Save as
							if (ProcessKey(KEY_SHIFTF2))
								return SAVEFILE_SUCCESS;
							else
								return SAVEFILE_CANCEL;

						case 0:    // Save
							break;
					}
				}
			}
		}

		Flags.Clear(FFILEEDIT_SAVEWQUESTIONS);
		NewFile = FALSE;
	} else {
		// проверим путь к файлу, может его уже снесли...
		FARString strCreatedPath = Name;
		const wchar_t *Ptr = LastSlash(strCreatedPath);

		if (Ptr) {
			CutToSlash(strCreatedPath);
			DWORD FAttr = 0;

			if (apiGetFileAttributes(strCreatedPath) == INVALID_FILE_ATTRIBUTES) {
				// и попробуем создать.
				// Раз уж
				CreatePath(strCreatedPath);
				FAttr = apiGetFileAttributes(strCreatedPath);
			}

			if (FAttr == INVALID_FILE_ATTRIBUTES)
				return SAVEFILE_ERROR;
		}
	}

	if (BadConversion) {
		if (Message(MSG_WARNING, 2, Msg::Warning, Msg::EditDataLostWarn, Msg::EditorSaveNotRecommended,
					Msg::Ok, Msg::Cancel)) {
			return SAVEFILE_CANCEL;
		} else {
			BadConversion = false;
		}
	}

	int RetCode = SAVEFILE_SUCCESS;

	if (TextFormat)
		m_editor->Flags.Set(FEDITOR_WASCHANGED);

	switch (TextFormat) {
		case 1:
			wcscpy(m_editor->m_GlobalEOL, DOS_EOL_fmt);
			break;
		case 2:
			wcscpy(m_editor->m_GlobalEOL, UNIX_EOL_fmt);
			break;
		case 3:
			wcscpy(m_editor->m_GlobalEOL, MAC_EOL_fmt);
			break;
		case 4:
			wcscpy(m_editor->m_GlobalEOL, WIN_EOL_fmt);
			break;
	}

	if (apiGetFileAttributes(Name) == INVALID_FILE_ATTRIBUTES)
		Flags.Set(FFILEEDIT_NEW);

	{
		// SaveScreen SaveScr;
		CtrlObject->Plugins.CurEditor = this;
		//_D(SysLog(L"%08d EE_SAVE",__LINE__));

		if (!IsUnicodeOrUtfCodePage(codepage)) {
			int LineNumber = 0;
			bool BadSaveConfirmed = false;
			for (Edit *CurPtr = m_editor->m_TopList; CurPtr; CurPtr = CurPtr->m_next, LineNumber++) {
				const wchar_t *SaveStr, *EndSeq;
				int Length;
				CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);
				BOOL UsedDefaultCharStr = FALSE, UsedDefaultCharEOL = FALSE;
				if (Length
						&& !WINPORT(WideCharToMultiByte)(codepage, WC_NO_BEST_FIT_CHARS, SaveStr, Length,
								nullptr, 0, nullptr, &UsedDefaultCharStr))
					return SAVEFILE_ERROR;

				if (!*EndSeq && CurPtr->m_next)
					EndSeq = *m_editor->m_GlobalEOL ? m_editor->m_GlobalEOL : DOS_EOL_fmt;

				if (TextFormat && *EndSeq)
					EndSeq = m_editor->m_GlobalEOL;

				int EndSeqLen = StrLength(EndSeq);
				if (EndSeqLen
						&& !WINPORT(WideCharToMultiByte)(codepage, WC_NO_BEST_FIT_CHARS, EndSeq, EndSeqLen,
								nullptr, 0, nullptr, &UsedDefaultCharEOL))
					return SAVEFILE_ERROR;

				if (!BadSaveConfirmed && (UsedDefaultCharStr || UsedDefaultCharEOL)) {
					// SetMessageHelp(L"EditorDataLostWarning")
					int Result = Message(MSG_WARNING, 3, Msg::Warning, Msg::EditorSaveCPWarn1,
							Msg::EditorSaveCPWarn2, Msg::EditorSaveNotRecommended, Msg::Ok,
							Msg::EditorSaveCPWarnShow, Msg::Cancel);
					if (!Result) {
						BadSaveConfirmed = true;
						break;
					} else {
						if (Result == 1) {
							m_editor->GoToLine(LineNumber);
							if (UsedDefaultCharStr) {
								for (int Pos = 0; Pos < Length; Pos++) {
									BOOL UseDefChar = 0;
									WINPORT(WideCharToMultiByte)
									(codepage, WC_NO_BEST_FIT_CHARS, SaveStr + Pos, 1, nullptr, 0, nullptr,
											&UseDefChar);
									if (UseDefChar) {
										CurPtr->SetCurPos(Pos);
										break;
									}
								}
							} else {
								CurPtr->SetCurPos(CurPtr->GetLength());
							}
							Show();
						}
						return SAVEFILE_CANCEL;
					}
				}
			}
		}

		CtrlObject->Plugins.ProcessEditorEvent(EE_SAVE, nullptr);

		m_editor->m_UndoSavePos = m_editor->m_UndoPos;
		m_editor->Flags.Clear(FEDITOR_UNDOSAVEPOSLOST);
		//    ConvertNameToFull(Name,FileName, sizeof(FileName));
		/*
			if (ConvertNameToFull(Name,m_editor->m_FileName, sizeof(m_editor->m_FileName)) >= sizeof(m_editor->m_FileName))
			{
			  m_editor->Flags.Set(FEDITOR_OPENFAILED);
			  RetCode=SAVEFILE_ERROR;
			  goto end;
			}
		*/
		SetCursorType(false, 0);
		SCOPED_ACTION(TPreRedrawFuncGuard)(Editor::PR_EditorShowMsg);

		try {
			ContentMeasurer cm;
			SaveContent(Name, &cm, bSaveAs, TextFormat, codepage, AddSignature, 0);

			try {
				File EditFile;
				bool EditFileOpened = EditFile.Open(Name, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
						OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN);
				if (!EditFileOpened
						&& (WINPORT(GetLastError)() == ERROR_NOT_SUPPORTED
								|| WINPORT(GetLastError)() == ERROR_CALL_NOT_IMPLEMENTED)) {
					EditFileOpened = EditFile.Open(Name, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
							CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN);
					if (EditFileOpened) {
						fprintf(stderr, "FileEditor::SaveFile: CREATE_ALWAYS for '%ls'\n", Name);
					}
				}
				if (!EditFileOpened) {
					throw WINPORT(GetLastError)();
				}

				if (!Flags.Check(FFILEEDIT_NEW)) {
					if (!EditFile.AllocationRequire(cm.MeasuredSize))
						throw WINPORT(GetLastError)();
				}

				ContentSaver cs(EditFile);
				SaveContent(Name, &cs, bSaveAs, TextFormat, codepage, AddSignature, 1);
				cs.Flush();

				EditFile.SetEnd();

			} catch (...) {
				if (Flags.Check(FFILEEDIT_NEW))
					apiDeleteFile(Name);

				throw;
			}
		} catch (DWORD ErrorCode) {
			SysErrorCode = ErrorCode;
			RetCode = SAVEFILE_ERROR;
		} catch (std::exception &e) {
			SysErrorCode = ENOMEM;
			RetCode = SAVEFILE_ERROR;
		}
	}

	if (FHP && RetCode != SAVEFILE_ERROR)
		FHP->OnFileEdited(Name);

	if (FileUnmakeWritable) {
		FileUnmakeWritable->Unmake();
		FileUnmakeWritable.reset();
		//		apiSetFileAttributes(Name,FileAttributes|FILE_ATTRIBUTE_ARCHIVE);
	}

	apiGetFindDataForExactPathName(Name, FileInfo);
	EditorGetFileAttributes(Name);

	if (m_editor->Flags.Check(FEDITOR_MODIFIED) || NewFile)
		m_editor->Flags.Set(FEDITOR_WASCHANGED);

	/* Этот кусок раскомметировать в том случае, если народ решит, что
	   для если файл был залочен и мы его переписали под други именем...
	   ...то "лочка" должна быть снята.
	*/

	//  if(SaveAs)
	//    Flags.Clear(FEDITOR_LOCKMODE);
	/* 28.12.2001 VVM
	  ! Проверить на успешную запись */
	if (RetCode == SAVEFILE_SUCCESS)
		m_editor->TextChanged(false);

	if (GetDynamicallyBorn())    // принудительно сбросим Title // Flags.Check(FFILEEDIT_SAVETOSAVEAS) ????????
		strTitle.Clear();

	Show();
	// ************************************
	Flags.Clear(FFILEEDIT_NEW);
	return RetCode;
}

int FileEditor::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	F4KeyOnly = false;
	if (!EditKeyBar.ProcessMouse(MouseEvent))
		if (!ProcessEditorInput(FrameManager->GetLastInputRecord()))
			if (!m_editor->ProcessMouse(MouseEvent))
				return FALSE;

	return TRUE;
}

int FileEditor::GetTypeAndName(FARString &strType, FARString &strName)
{
	strType = Msg::ScreensEdit;
	strName = strFullFileName;
	return (MODALTYPE_EDITOR);
}

void FileEditor::ShowConsoleTitle()
{
	FARString strTitle;
	strTitle.Format(Msg::InEditor, PointToName(strFileName));
	ConsoleTitle::SetFarTitle(strTitle);
	Flags.Clear(FFILEEDIT_REDRAWTITLE);
}

void FileEditor::SetScreenPosition()
{
	if (Flags.Check(FFILEEDIT_FULLSCREEN)) {
		SetPosition(0, 0, ScrX, ScrY);
	}
}

/* $ 10.05.2001 DJ
   добавление в view/edit history
*/

void FileEditor::OnDestroy()
{
	_OT(SysLog(L"[%p] FileEditor::OnDestroy()", this));

	if (!Flags.Check(FFILEEDIT_DISABLEHISTORY) && StrCmpI(strFileName, Msg::NewFileName))
		CtrlObject->ViewHistory->AddToHistory(strFullFileName,
				(m_editor->Flags.Check(FEDITOR_LOCKMODE) ? HR_EDITOR_RO : HR_EDITOR));

	if (CtrlObject->Plugins.CurEditor == this)    //&this->FEdit)
	{
		CtrlObject->Plugins.CurEditor = nullptr;
	}
}

bool FileEditor::GetCanLoseFocus(bool DynamicMode)
{
	return DynamicMode ? !m_editor->IsFileModified() : CanLoseFocus;
}

void FileEditor::SetLockEditor(BOOL LockMode)
{
	if (LockMode)
		m_editor->Flags.Set(FEDITOR_LOCKMODE);
	else
		m_editor->Flags.Clear(FEDITOR_LOCKMODE);
}

int FileEditor::FastHide()
{
	return Opt.AllCtrlAltShiftRule & CASR_EDITOR;
}

BOOL FileEditor::isTemporary()
{
	return (!GetDynamicallyBorn());
}

void FileEditor::ResizeConsole()
{
	m_editor->PrepareResizedConsole();
}

int FileEditor::ProcessEditorInput(INPUT_RECORD *Rec)
{
	int RetCode;
	CtrlObject->Plugins.CurEditor = this;
	RetCode = CtrlObject->Plugins.ProcessEditorInput(Rec);
	return RetCode;
}

void FileEditor::SetPluginTitle(const wchar_t *PluginTitle)
{
	if (!PluginTitle)
		strPluginTitle.Clear();
	else
		strPluginTitle = PluginTitle;
}

BOOL FileEditor::SetFileName(const wchar_t *NewFileName)
{
	strFileName = NewFileName;

	if (StrCmp(strFileName, Msg::NewFileName)) {
		ConvertNameToFull(strFileName, strFullFileName);
		FARString strFilePath = strFullFileName;

		if (CutToSlash(strFilePath, true)) {
			FARString strCurPath;

			if (apiGetCurrentDirectory(strCurPath)) {
				DeleteEndSlash(strCurPath);

				if (!StrCmpI(strFilePath, strCurPath))
					strFileName = PointToName(strFullFileName);
			}
		}

	} else {
		strFullFileName = strStartDir;
		AddEndSlash(strFullFileName);
		strFullFileName+= strFileName;
	}

	return TRUE;
}

void FileEditor::SetTitle(const wchar_t *Title)
{
	strTitle = Title;
}

void FileEditor::ChangeEditKeyBar()
{
	if (m_codepage == CP_UTF8)
		EditKeyBar.Change(KBL_MAIN, (Opt.OnlyEditorViewerUsed ? Msg::SingleEditF8 : Msg::EditF8), 7);
	else if (m_codepage == WINPORT(GetACP)())
		EditKeyBar.Change(KBL_MAIN, (Opt.OnlyEditorViewerUsed ? Msg::SingleEditF8DOS : Msg::EditF8DOS), 7);
	else
		EditKeyBar.Change(KBL_MAIN, (Opt.OnlyEditorViewerUsed ? Msg::SingleEditF8UTF8 : Msg::EditF8UTF8), 7);

	EditKeyBar.Redraw();
}

FARString &FileEditor::GetTitle(FARString &strLocalTitle, int SubLen, int TruncSize)
{
	if (!strPluginTitle.IsEmpty())
		strLocalTitle = strPluginTitle;
	else {
		if (!strTitle.IsEmpty())
			strLocalTitle = strTitle;
		else
			strLocalTitle = strFullFileName;
	}

	return strLocalTitle;
}

void FileEditor::ShowStatus()
{
	if (m_editor->Locked() || !Opt.EdOpt.ShowTitleBar)
		return;

	SetFarColor(COL_EDITORSTATUS);
	GotoXY(X1, Y1);    //??
	FARString strLineStr;
	FARString strLocalTitle;
	GetTitle(strLocalTitle);
	int NameLength = Opt.ViewerEditorClock && Flags.Check(FFILEEDIT_FULLSCREEN) ? 17 : 23;

	if (X2 > 80)
		NameLength+= (X2 - 80);

	if (!strPluginTitle.IsEmpty() || !strTitle.IsEmpty())
		TruncPathStr(strLocalTitle, (ObjWidth < NameLength ? ObjWidth : NameLength));
	else
		TruncPathStr(strLocalTitle, NameLength);

	// предварительный расчет
	strLineStr.Format(L"%d/%d", m_editor->m_NumLastLine, m_editor->m_NumLastLine);
	int SizeLineStr = (int)strLineStr.GetLength();

	if (SizeLineStr > 12)
		NameLength-= (SizeLineStr - 12);
	else
		SizeLineStr = 12;

	strLineStr.Format(L"%d/%d", m_editor->m_NumLine + 1, m_editor->m_NumLastLine);
	FARString strAttr(AttrStr);
	FormatString FString;
	FString << fmt::LeftAlign() << fmt::Expand(NameLength) << strLocalTitle << L' '
			<< (m_editor->Flags.Check(FEDITOR_MODIFIED) ? L'*' : L' ')
			<< (m_editor->Flags.Check(FEDITOR_LOCKMODE) ? L'-' : L' ')
			<< (m_editor->Flags.Check(FEDITOR_PROCESSCTRLQ) ? L'"' : L' ') << fmt::Expand(5)
			<< EOLName(m_editor->m_GlobalEOL) << L' ' << fmt::Expand(5) << m_codepage << L' '
			<< fmt::Expand(7) << Msg::EditStatusLine << L' ' << fmt::Expand(SizeLineStr)
			<< fmt::Truncate(SizeLineStr) << strLineStr << L' ' << fmt::Expand(5) << Msg::EditStatusCol
			<< L' ' << fmt::LeftAlign() << fmt::Expand(4) << m_editor->m_CurLine->GetCellCurPos() + 1 << L' '
			<< fmt::Expand(3) << strAttr;
	int StatusWidth = ObjWidth - (Opt.ViewerEditorClock && Flags.Check(FFILEEDIT_FULLSCREEN) ? 5 : 0);

	if (StatusWidth < 0)
		StatusWidth = 0;

	FS << fmt::LeftAlign() << fmt::Size(StatusWidth) << FString.strValue();
	{
		const wchar_t *Str;
		int Length;
		m_editor->m_CurLine->GetBinaryString(&Str, nullptr, Length);
		int CurPos = m_editor->m_CurLine->GetCurPos();

		if (CurPos < Length) {
			GotoXY(X2 - (Opt.ViewerEditorClock && Flags.Check(FFILEEDIT_FULLSCREEN) ? 16 : 10), Y1);
			SetFarColor(COL_EDITORSTATUS);
			/* $ 27.02.2001 SVS
			Показываем в зависимости от базы */
			static const wchar_t *FmtWCharCode[] = {L"%05o", L"%5d", L"%04Xh"};
			mprintf(FmtWCharCode[m_editor->m_EdOpt.CharCodeBase % ARRAYSIZE(FmtWCharCode)], Str[CurPos]);

			if (!IsUnicodeOrUtfCodePage(m_codepage)) {
				char C = 0;
				BOOL UsedDefaultChar = FALSE;
				WINPORT(WideCharToMultiByte)
				(m_codepage, WC_NO_BEST_FIT_CHARS, &Str[CurPos], 1, &C, 1, 0, &UsedDefaultChar);

				int UC = static_cast<int>(static_cast<unsigned char>(C));
				if (UC && !UsedDefaultChar && UC != Str[CurPos]) {
					static const wchar_t *FmtCharCode[] = {L"%o", L"%d", L"%Xh"};
					Text(L" (");
					mprintf(FmtCharCode[m_editor->m_EdOpt.CharCodeBase % ARRAYSIZE(FmtCharCode)], UC);
					Text(L")");
				}
			}
		}
	}

	if (Opt.ViewerEditorClock && Flags.Check(FFILEEDIT_FULLSCREEN))
		ShowTime(FALSE);
}

/* $ 13.02.2001
	 Узнаем атрибуты файла и заодно сформируем готовую строку атрибутов для
	 статуса.
*/
DWORD FileEditor::EditorGetFileAttributes(const wchar_t *Name)
{
	SudoClientRegion sdc_rgn;
	DWORD FileAttributes = apiGetFileAttributes(Name);
	int ind = 0;

	if (FileAttributes != INVALID_FILE_ATTRIBUTES) {
		if (FileAttributes & FILE_ATTRIBUTE_READONLY)
			AttrStr[ind++] = L'R';

		if (FileAttributes & FILE_ATTRIBUTE_SYSTEM)
			AttrStr[ind++] = L'S';

		if (FileAttributes & FILE_ATTRIBUTE_HIDDEN)
			AttrStr[ind++] = L'H';
	}
	AttrStr[ind] = 0;
	return FileAttributes;
}

/* Return TRUE - панель обовили
 */
BOOL FileEditor::UpdateFileList()
{
	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
	const wchar_t *FileName = PointToName(strFullFileName);
	FARString strFilePath, strPanelPath;
	strFilePath = strFullFileName;
	strFilePath.Truncate(FileName - strFullFileName.CPtr());
	ActivePanel->GetCurDir(strPanelPath);
	AddEndSlash(strPanelPath);
	AddEndSlash(strFilePath);

	if (!StrCmp(strPanelPath, strFilePath)) {
		ActivePanel->Update(UPDATE_KEEP_SELECTION | UPDATE_DRAW_MESSAGE);
		return TRUE;
	}

	return FALSE;
}

void FileEditor::GetEditorOptions(EditorOptions &EdOpt)
{
	EdOpt = m_editor->m_EdOpt;
}

void FileEditor::SetEditorOptions(EditorOptions &EdOpt)
{
	m_editor->SetTabSize(EdOpt.TabSize);
	m_editor->SetConvertTabs(EdOpt.ExpandTabs);
	m_editor->SetPersistentBlocks(EdOpt.PersistentBlocks);
	m_editor->SetDelRemovesBlocks(EdOpt.DelRemovesBlocks);
	m_editor->SetAutoIndent(EdOpt.AutoIndent);
	m_editor->SetAutoDetectCodePage(EdOpt.AutoDetectCodePage);
	m_editor->SetCursorBeyondEOL(EdOpt.CursorBeyondEOL);
	m_editor->SetCharCodeBase(EdOpt.CharCodeBase);
	m_editor->SetSavePosMode(EdOpt.SavePos, EdOpt.SaveShortPos);
	m_editor->SetReadOnlyLock(EdOpt.ReadOnlyLock);
	m_editor->SetShowScrollBar(EdOpt.ShowScrollBar);
	m_editor->SetShowWhiteSpace(EdOpt.ShowWhiteSpace);
	m_editor->SetSearchPickUpWord(EdOpt.SearchPickUpWord);
	// m_editor->SetBSLikeDel(EdOpt.BSLikeDel);
}

void FileEditor::OnChangeFocus(int focus)
{
	Frame::OnChangeFocus(focus);
	CtrlObject->Plugins.CurEditor = this;
	int FEditEditorID = m_editor->m_EditorID;
	CtrlObject->Plugins.ProcessEditorEvent(focus ? EE_GOTFOCUS : EE_KILLFOCUS, &FEditEditorID);
}

int FileEditor::EditorControl(int Command, void *Param)
{
#if defined(SYSLOG_KEYMACRO)
	_KEYMACRO(CleverSysLog SL(L"FileEditor::EditorControl()"));

	if (Command == ECTL_READINPUT || Command == ECTL_PROCESSINPUT) {
		_KEYMACRO(SysLog(L"(Command=%ls, Param=[%d/0x%08X]) Macro.IsExecuting()=%d", _ECTL_ToName(Command),
				(int)((DWORD_PTR)Param), (int)((DWORD_PTR)Param), CtrlObject->Macro.IsExecuting()));
	}

#else
	_ECTLLOG(CleverSysLog SL(L"FileEditor::EditorControl()"));
	_ECTLLOG(SysLog(L"(Command=%ls, Param=[%d/0x%08X])", _ECTL_ToName(Command), (int)Param, Param));
#endif

	if (m_bClosing && (Command != ECTL_GETINFO) && (Command != ECTL_GETBOOKMARKS)
			&& (Command != ECTL_GETFILENAME))
		return FALSE;

	switch (Command) {
		case ECTL_GETFILENAME: {
			if (Param) {
				wcscpy(reinterpret_cast<LPWSTR>(Param), strFullFileName);
			}

			return static_cast<int>(strFullFileName.GetLength() + 1);
		}
		case ECTL_GETBOOKMARKS: {
			if (!Flags.Check(FFILEEDIT_OPENFAILED) && Param) {
				EditorBookMarks *ebm = reinterpret_cast<EditorBookMarks *>(Param);
				for (size_t i = 0; i < POSCACHE_BOOKMARK_COUNT; i++) {
					if (ebm->Line) {
						ebm->Line[i] = static_cast<long>(m_editor->m_SavePos.Line[i]);
					}
					if (ebm->Cursor) {
						ebm->Cursor[i] = static_cast<long>(m_editor->m_SavePos.Cursor[i]);
					}
					if (ebm->ScreenLine) {
						ebm->ScreenLine[i] = static_cast<long>(m_editor->m_SavePos.ScreenLine[i]);
					}
					if (ebm->LeftPos) {
						ebm->LeftPos[i] = static_cast<long>(m_editor->m_SavePos.LeftPos[i]);
					}
				}
				return TRUE;
			}

			return FALSE;
		}
		case ECTL_ADDSTACKBOOKMARK: {
			return m_editor->AddStackBookmark();
		}
		case ECTL_PREVSTACKBOOKMARK: {
			return m_editor->PrevStackBookmark();
		}
		case ECTL_NEXTSTACKBOOKMARK: {
			return m_editor->NextStackBookmark();
		}
		case ECTL_CLEARSTACKBOOKMARKS: {
			return m_editor->ClearStackBookmarks();
		}
		case ECTL_DELETESTACKBOOKMARK: {
			return m_editor->DeleteStackBookmark(m_editor->PointerToStackBookmark((int)(INT_PTR)Param));
		}
		case ECTL_GETSTACKBOOKMARKS: {
			return m_editor->GetStackBookmarks((EditorBookMarks *)Param);
		}
		case ECTL_GETTITLE: {
			FARString title;
			GetTitle(title);
			if (Param) {
				wcscpy(reinterpret_cast<LPWSTR>(Param), title);
			}
			return static_cast<int>(title.GetLength() + 1);
		}
		case ECTL_SETTITLE: {
			strPluginTitle = (const wchar_t *)Param;
			ShowStatus();
			ScrBuf.Flush();    //???
			return TRUE;
		}
		case ECTL_REDRAW: {
			FileEditor::DisplayObject();
			ScrBuf.Flush();
			return TRUE;
		}
		/*
			Функция установки Keybar Labels
			Param = nullptr - восстановить, пред. значение
			Param = -1   - обновить полосу (перерисовать)
			Param = KeyBarTitles
		*/
		case ECTL_SETKEYBAR: {
			KeyBarTitles *Kbt = (KeyBarTitles *)Param;

			if (!Kbt)    // восстановить изначальное
				InitKeyBar();
			else {
				if ((LONG_PTR)Param != (LONG_PTR)-1)    // не только перерисовать?
				{
					for (int I = 0; I < 12; ++I) {
						if (Kbt->Titles[I])
							EditKeyBar.Change(KBL_MAIN, Kbt->Titles[I], I);

						if (Kbt->CtrlTitles[I])
							EditKeyBar.Change(KBL_CTRL, Kbt->CtrlTitles[I], I);

						if (Kbt->AltTitles[I])
							EditKeyBar.Change(KBL_ALT, Kbt->AltTitles[I], I);

						if (Kbt->ShiftTitles[I])
							EditKeyBar.Change(KBL_SHIFT, Kbt->ShiftTitles[I], I);

						if (Kbt->CtrlShiftTitles[I])
							EditKeyBar.Change(KBL_CTRLSHIFT, Kbt->CtrlShiftTitles[I], I);

						if (Kbt->AltShiftTitles[I])
							EditKeyBar.Change(KBL_ALTSHIFT, Kbt->AltShiftTitles[I], I);

						if (Kbt->CtrlAltTitles[I])
							EditKeyBar.Change(KBL_CTRLALT, Kbt->CtrlAltTitles[I], I);
					}
				}

				EditKeyBar.Refresh(Opt.EdOpt.ShowKeyBar);
			}

			return TRUE;
		}
		case ECTL_SAVEFILE: {
			FARString strName = strFullFileName;
			int EOL = 0;
			UINT codepage = m_codepage;

			if (Param) {
				EditorSaveFile *esf = (EditorSaveFile *)Param;

				if (esf->FileName && *esf->FileName)
					strName = esf->FileName;

				if (esf->FileEOL) {
					if (!StrCmp(esf->FileEOL, DOS_EOL_fmt))
						EOL = 1;
					else if (!StrCmp(esf->FileEOL, UNIX_EOL_fmt))
						EOL = 2;
					else if (!StrCmp(esf->FileEOL, MAC_EOL_fmt))
						EOL = 3;
					else if (!StrCmp(esf->FileEOL, WIN_EOL_fmt))
						EOL = 4;
				}

				codepage = esf->CodePage;
			}

			{
				FARString strOldFullFileName = strFullFileName;

				if (SetFileName(strName)) {
					if (StrCmpI(strFullFileName, strOldFullFileName)) {
						if (!AskOverwrite(strName)) {
							SetFileName(strOldFullFileName);
							return FALSE;
						}
					}

					Flags.Set(FFILEEDIT_SAVEWQUESTIONS);
					// всегда записываем в режиме save as - иначе не сменить кодировку и концы линий.
					return SaveFile(strName, FALSE, true, EOL, codepage, DecideAboutSignature());
				}
			}

			return FALSE;
		}
		case ECTL_QUIT: {
			SetExitCode(SAVEFILE_ERROR);    // что-то меня терзают смутные сомнения ...???
			FrameManager->DeleteFrame(this);
			FrameManager->Commit();
			return TRUE;
		}
		case ECTL_READINPUT: {
			//	if (!CtrlObject->Macro.CanSendKeysToPlugin())
			//	{
			//		return FALSE;
			//	}

			if (Param) {
				INPUT_RECORD *rec = (INPUT_RECORD *)Param;
				FarKey Key;

				for (;;) {
					Key = GetInputRecord(rec);

					if ((!rec->EventType || rec->EventType == KEY_EVENT)
							&& ((Key >= KEY_MACRO_BASE && Key <= KEY_MACRO_ENDBASE)
									|| (Key >= KEY_OP_BASE && Key <= KEY_OP_ENDBASE)))    // исключаем MACRO
						ReProcessKey(Key);
					else
						break;
				}

				// if(Key==KEY_CONSOLE_BUFFER_RESIZE) //????
				//   Show();                          //????
#if defined(SYSLOG_KEYMACRO)

				if (rec->EventType == KEY_EVENT) {
					SysLog(L"ECTL_READINPUT={KEY_EVENT,{%d,%d,Vk=0x%04X,0x%08X}}",
							rec->Event.KeyEvent.bKeyDown, rec->Event.KeyEvent.wRepeatCount,
							rec->Event.KeyEvent.wVirtualKeyCode, rec->Event.KeyEvent.dwControlKeyState);
				}

#endif
				return TRUE;
			}

			return FALSE;
		}
		case ECTL_PROCESSINPUT: {
			if (Param) {
				INPUT_RECORD *rec = (INPUT_RECORD *)Param;

				if (ProcessEditorInput(rec))
					return TRUE;

				if (rec->EventType == MOUSE_EVENT)
					ProcessMouse(&rec->Event.MouseEvent);
				else {
#if defined(SYSLOG_KEYMACRO)

					if (!rec->EventType || rec->EventType == KEY_EVENT) {
						SysLog(L"ECTL_PROCESSINPUT={KEY_EVENT,{%d,%d,Vk=0x%04X,0x%08X}}",
								rec->Event.KeyEvent.bKeyDown, rec->Event.KeyEvent.wRepeatCount,
								rec->Event.KeyEvent.wVirtualKeyCode, rec->Event.KeyEvent.dwControlKeyState);
					}

#endif
					FarKey Key = CalcKeyCode(rec, FALSE);
					ReProcessKey(Key);
				}

				return TRUE;
			}

			return FALSE;
		}
		case ECTL_PROCESSKEY: {
			ReProcessKey((int)(INT_PTR)Param);
			return TRUE;
		}
		case ECTL_SETPARAM: {
			if (Param) {
				EditorSetParameter *espar = (EditorSetParameter *)Param;
				if (ESPT_SETBOM == espar->Type) {
					if (IsUnicodeOrUtfCodePage(m_codepage)) {
						m_AddSignature = espar->Param.iParam ? FB_YES : FB_NO;
						return TRUE;
					}
					return FALSE;
				}
			}
			break;
		}
	}

	int result = m_editor->EditorControl(Command, Param);
	if (result && Param && ECTL_GETINFO == Command) {
		EditorInfo *Info = (EditorInfo *)Param;
		if (DecideAboutSignature())
			Info->Options|= EOPT_BOM;
	}
	return result;
}

bool FileEditor::DecideAboutSignature()
{
	return (m_AddSignature == FB_YES
			|| (m_AddSignature == FB_MAYBE && IsUnicodeOrUtfCodePage(m_codepage) && m_codepage != CP_UTF8));
}

bool FileEditor::LoadFromCache(EditorCacheParams *pp)
{
	memset(pp, 0, sizeof(EditorCacheParams));
	memset(&pp->SavePos, 0xff, sizeof(InternalEditorBookMark));

	FARString strCacheName;

	if (*GetPluginData()) {
		strCacheName = GetPluginData();
		strCacheName+= PointToName(strFullFileName);
	} else {
		strCacheName+= strFullFileName;
	}

	PosCache PosCache{};

	if (Opt.EdOpt.SaveShortPos) {
		PosCache.Position[0] = pp->SavePos.Line;
		PosCache.Position[1] = pp->SavePos.Cursor;
		PosCache.Position[2] = pp->SavePos.ScreenLine;
		PosCache.Position[3] = pp->SavePos.LeftPos;
	}

	if (CtrlObject->EditorPosCache->GetPosition(strCacheName, PosCache)) {
		pp->Line = static_cast<int>(PosCache.Param[0]);
		pp->ScreenLine = static_cast<int>(PosCache.Param[1]);
		pp->LinePos = static_cast<int>(PosCache.Param[2]);
		pp->LeftPos = static_cast<int>(PosCache.Param[3]);
		pp->CodePage = static_cast<UINT>(PosCache.Param[4]);

		if (pp->Line < 0)
			pp->Line = 0;

		if (pp->ScreenLine < 0)
			pp->ScreenLine = 0;

		if (pp->LinePos < 0)
			pp->LinePos = 0;

		if (pp->LeftPos < 0)
			pp->LeftPos = 0;

		if ((int)pp->CodePage < 0)
			pp->CodePage = 0;

		return true;
	}

	return false;
}

void FileEditor::SaveToCache()
{
	EditorCacheParams cp;
	m_editor->GetCacheParams(&cp);
	FARString strCacheName =
			strPluginData.IsEmpty() ? strFullFileName : strPluginData + PointToName(strFullFileName);

	if (!Flags.Check(FFILEEDIT_OPENFAILED))    //????
	{
		PosCache poscache{};
		poscache.Param[0] = cp.Line;
		poscache.Param[1] = cp.ScreenLine;
		poscache.Param[2] = cp.LinePos;
		poscache.Param[3] = cp.LeftPos;
		poscache.Param[4] = Flags.Check(FFILEEDIT_CODEPAGECHANGEDBYUSER) ? m_codepage : 0;

		if (Opt.EdOpt.SaveShortPos) {
			// if no position saved these are nulls
			poscache.Position[0] = cp.SavePos.Line;
			poscache.Position[1] = cp.SavePos.Cursor;
			poscache.Position[2] = cp.SavePos.ScreenLine;
			poscache.Position[3] = cp.SavePos.LeftPos;
		}

		CtrlObject->EditorPosCache->AddPosition(strCacheName, poscache);
	}
}

void FileEditor::SetCodePage(UINT codepage)
{
	if (codepage != m_codepage) {
		m_codepage = codepage;

		if (m_editor) {
			if (!m_editor->SetCodePage(m_codepage)) {
				Message(MSG_WARNING, 1, Msg::Warning, Msg::EditorSwitchCPWarn1, Msg::EditorSwitchCPWarn2,
						Msg::EditorSaveNotRecommended, Msg::Ok);
				BadConversion = true;
			}

			ChangeEditKeyBar();    //???
		}
	}
}

bool FileEditor::AskOverwrite(const FARString &FileName)
{
	bool result = true;
	DWORD FNAttr = apiGetFileAttributes(FileName);

	if (FNAttr != INVALID_FILE_ATTRIBUTES) {
		if (Message(MSG_WARNING, 2, &EditorAskOverwriteId, Msg::EditTitle, FileName, Msg::EditExists,
					Msg::EditOvr, Msg::Yes, Msg::No)) {
			result = false;
		} else {
			Flags.Set(FFILEEDIT_SAVEWQUESTIONS);
		}
	}

	return result;
}

int FileEditor::GetEditorID() const
{
	return m_editor->m_EditorID;
}

bool FileEditor::IsLockAttributes(DWORD FileAttributes)
{
	/* Hidden=0x2 System=0x4 - располагаются во 2-м полубайте опции ReadOnlyLock,
	   поэтому сдвигаем на свое место (>> 4) и применяем маску 0000.0110
	   и получаем те самые нужные атрибуты
	*/
	if (FileAttributes != INVALID_FILE_ATTRIBUTES) {
		auto ExtraAttr = (m_editor->m_EdOpt.ReadOnlyLock >> 4) & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
		return (FileAttributes & (FILE_ATTRIBUTE_READONLY | ExtraAttr)) != 0;
	}
	return false;
}

void EditConsoleHistory(HANDLE con_hnd, bool Modal)
{
	const std::string &histfile = CtrlObject->CmdLine->GetConsoleLog(con_hnd, false);
	if (histfile.empty())
		return;

	FileEditor *ShellEditor = new (std::nothrow) FileEditor(StrMB2Wide(histfile).c_str(), CP_UTF8,
			FFILEEDIT_DISABLEHISTORY | FFILEEDIT_NEW | FFILEEDIT_SAVETOSAVEAS
					| (Modal ? 0 : FFILEEDIT_ENABLEF6),
			std::numeric_limits<int>::max());
	unlink(histfile.c_str());
	if (ShellEditor) {
		DWORD editorExitCode = ShellEditor->GetExitCode();
		if (editorExitCode != XC_LOADING_INTERRUPTED && editorExitCode != XC_OPEN_ERROR) {
			if (Modal)
				FrameManager->ExecuteModalEV(false);
			else
				FrameManager->Commit();
		} else
			delete ShellEditor;
	}
}

//////////////////////////////////
