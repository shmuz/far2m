/*
cmdline.cpp

Командная строка
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

#include "cmdline.hpp"
#include "execute.hpp"
#include "macroopcode.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "ctrlobj.hpp"
#include "manager.hpp"
#include "farcolors.hpp"
#include "history.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "foldtree.hpp"
#include "treelist.hpp"
#include "fileview.hpp"
#include "fileedit.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "scrbuf.hpp"
#include "interf.hpp"
#include "syslog.hpp"
#include "ConfigOpt.hpp"
#include "usermenu.hpp"
#include "datetime.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "keyboard.hpp"
#include "vmenu.hpp"
#include "CachedCreds.hpp"
#include "exitcode.hpp"
#include "vtlog.h"
#include "vtshell.h"
#include "vtcompletor.h"
#include <limits>

CommandLine::CommandLine()
	:
	CmdStr(CtrlObject->Cp(), 0, true, CtrlObject->CmdHistory, 0,
			(Opt.CmdLine.AutoComplete ? EditControl::EC_ENABLEAUTOCOMPLETE : 0)
					| EditControl::EC_ENABLEFNCOMPLETE
					| EditControl::EC_ENABLEFNCOMPLETE_ESCAPED),
	BackgroundScreen(nullptr),
	LastCmdPartLength(-1),
	LastKey(0),
	PushDirStackSize(0)
{
	CmdStr.SetEditBeyondEnd(FALSE);
	SetPersistentBlocks(Opt.CmdLine.EditBlock);
	SetDelRemovesBlocks(Opt.CmdLine.DelRemovesBlocks);
}

CommandLine::~CommandLine()
{
	if (BackgroundScreen)
		delete BackgroundScreen;
}

void CommandLine::SetPersistentBlocks(int Mode)
{
	CmdStr.SetPersistentBlocks(Mode);
}

void CommandLine::SetDelRemovesBlocks(int Mode)
{
	CmdStr.SetDelRemovesBlocks(Mode);
}

void CommandLine::SetAutoComplete(int Mode)
{
	if(Mode)
	{
		CmdStr.EnableAC(true);
	}
	else
	{
		CmdStr.DisableAC(true);
	}
}

void CommandLine::DisplayObject()
{
	_OT(SysLog(L"[%p] CommandLine::DisplayObject()",this));
	FARString strTruncDir;
	GetPrompt(strTruncDir);
	TruncPathStr(strTruncDir,(X2-X1)/2);
	GotoXY(X1,Y1);
	SetFarColor(COL_COMMANDLINEPREFIX);
	Text(strTruncDir);
	CmdStr.SetObjectColor(FarColorToReal(COL_COMMANDLINE), FarColorToReal(COL_COMMANDLINESELECTED));
	CmdStr.SetPosition(X1+(int)strTruncDir.CellsCount(),Y1,X2,Y2);

	CmdStr.Show();

	GotoXY(X2+1,Y1);
	SetFarColor(COL_COMMANDLINEPREFIX);
	Text(L"\x2191");
}

void CommandLine::DrawComboBoxMark(wchar_t MarkChar)
{
	wchar_t MarkWz[2] = {MarkChar, 0};
	GotoXY(X2 + 1, Y1);
	SetFarColor(COL_COMMANDLINEPREFIX);
	Text(MarkWz);
}

void CommandLine::SetCurPos(int Pos, int LeftPos)
{
	CmdStr.SetLeftPos(LeftPos);
	CmdStr.SetCurPos(Pos);
	CmdStr.Redraw();
}

int64_t CommandLine::VMProcess(int OpCode,void *vParam,int64_t iParam)
{
	if (OpCode >= MCODE_C_CMDLINE_BOF && OpCode <= MCODE_C_CMDLINE_SELECTED)
		return CmdStr.VMProcess(OpCode-MCODE_C_CMDLINE_BOF+MCODE_C_BOF,vParam,iParam);

	if (OpCode >= MCODE_C_BOF && OpCode <= MCODE_C_SELECTED)
		return CmdStr.VMProcess(OpCode,vParam,iParam);

	if (OpCode == MCODE_V_ITEMCOUNT || OpCode == MCODE_V_CURPOS)
		return CmdStr.VMProcess(OpCode,vParam,iParam);

	if (OpCode == MCODE_V_CMDLINE_ITEMCOUNT || OpCode == MCODE_V_CMDLINE_CURPOS)
		return CmdStr.VMProcess(OpCode-MCODE_V_CMDLINE_ITEMCOUNT+MCODE_V_ITEMCOUNT,vParam,iParam);

	if (OpCode == MCODE_F_EDITOR_SEL)
		return CmdStr.VMProcess(MCODE_F_EDITOR_SEL,vParam,iParam);

	return 0;
}

void CommandLine::ProcessCompletion(bool possibilities)
{
	FARString strStr;
	CmdStr.GetString(strStr);
	if (!strStr.IsEmpty()) {
		std::string cmd = strStr.GetMB();
		VTCompletor vtc;
		if (possibilities) {
			std::vector<std::string>  possibilities;
			if (vtc.GetPossibilities(cmd, possibilities) && !possibilities.empty()) {
				//fprintf(stderr, "Possibilities: ");
				CmdStr.ShowCustomCompletionList(possibilities);
			}
		} else {
			if (vtc.ExpandCommand(cmd)) {
				strStr = cmd;
				CmdStr.SetString(strStr);
				CmdStr.Show();
			}
		}
	}
}

std::string CommandLine::GetConsoleLog(HANDLE con_hnd, bool colored)
{
	bool vtshell_busy = VTShell_Busy();
	if (!vtshell_busy) {
		++ProcessShowClock;
		ShowBackground();
		Redraw();
		ScrBuf.Flush();
	}
	const std::string &histfile = VTLog::GetAsFile(con_hnd, colored);
	if (!vtshell_busy) {
		--ProcessShowClock;
		Redraw();
		ScrBuf.Flush();
	}
	return histfile;
}

int CommandLine::ProcessKey(FarKey Key)
{
	const wchar_t *PStr;
	FARString strStr;

	FarKey SavedLastKey = LastKey;

	if ( Key!=KEY_NONE)
		LastKey = Key;

	if ( Key==KEY_TAB || Key==KEY_SHIFTTAB) {
		ProcessCompletion(SavedLastKey==Key);
		return TRUE;
	}

	if (Key == (KEY_MSWHEEL_UP | KEY_CTRL | KEY_SHIFT))
	{
		ViewConsoleHistory(NULL, false, true);
		return TRUE;
	}

	if ( Key==KEY_CTRLSHIFTF3 || Key==KEY_F3) {
		ViewConsoleHistory(NULL, false, false);
		return TRUE;
	}

	if ( Key==KEY_CTRLSHIFTF4 || Key==KEY_F4) {
		EditConsoleHistory(NULL, false);
		return TRUE;
	}

	if ( Key==KEY_F8) {
		ClearScreen(FarColorToReal(COL_COMMANDLINEUSERSCREEN));
		SaveBackground();
		VTLog::Reset(NULL);
		ShowBackground();
		Redraw();
//		ShellUpdatePanels(CtrlObject->Cp()->ActivePanel, FALSE);
		CtrlObject->MainKeyBar->Refresh(Opt.ShowKeyBar);

//		CmdExecute(L"reset", true, false, true, false, false, false);
		return TRUE;
	}


	if ((Key==KEY_CTRLEND || Key==KEY_CTRLNUMPAD1) && CmdStr.GetCurPos()==CmdStr.GetLength())
	{
		if (LastCmdPartLength==-1)
			strLastCmdStr = CmdStr.GetStringAddr();

		strStr = strLastCmdStr;
		int CurCmdPartLength=(int)strStr.GetLength();
		CtrlObject->CmdHistory->GetSimilar(strStr,LastCmdPartLength);

		if (LastCmdPartLength==-1)
		{
			strLastCmdStr = CmdStr.GetStringAddr();
			LastCmdPartLength=CurCmdPartLength;
		}
		CmdStr.DisableAC();
		CmdStr.SetString(strStr);
		CmdStr.Select(LastCmdPartLength,static_cast<int>(strStr.GetLength()));
		CmdStr.RevertAC();
		Show();
		return TRUE;
	}

	if (Key == KEY_UP || Key == KEY_NUMPAD8)
	{
		if (CtrlObject->Cp()->LeftPanel->IsVisible() || CtrlObject->Cp()->RightPanel->IsVisible())
			return FALSE;

		Key=KEY_CTRLE;
	}
	else if (Key == KEY_DOWN || Key == KEY_NUMPAD2)
	{
		if (CtrlObject->Cp()->LeftPanel->IsVisible() || CtrlObject->Cp()->RightPanel->IsVisible())
			return FALSE;

		Key=KEY_CTRLX;
	}

	// $ 25.03.2002 VVM + При погашенных панелях колесом крутим историю
	if (!CtrlObject->Cp()->LeftPanel->IsVisible() && !CtrlObject->Cp()->RightPanel->IsVisible())
	{
		switch (Key)
		{
			case KEY_MSWHEEL_UP:    Key = KEY_CTRLE; break;
			case KEY_MSWHEEL_DOWN:  Key = KEY_CTRLX; break;
			case KEY_MSWHEEL_LEFT:  Key = KEY_CTRLS; break;
			case KEY_MSWHEEL_RIGHT: Key = KEY_CTRLD; break;
		}
	}

	switch (Key)
	{
		case KEY_CTRLE:
		case KEY_CTRLX:
			{
				if (Key == KEY_CTRLE)
				{
					CtrlObject->CmdHistory->GetPrev(strStr);
				}
				else
				{
					CtrlObject->CmdHistory->GetNext(strStr);
				}
				CmdStr.DisableAC();
				SetString(strStr);
				CmdStr.RevertAC();
			}
			return TRUE;

		case KEY_ESC:

			if (Key == KEY_ESC)
			{
				// $ 24.09.2000 SVS - Если задано поведение по "Несохранению при Esc", то позицию в хистори не меняем и ставим в первое положение.
				if (Opt.CmdHistoryRule)
					CtrlObject->CmdHistory->ResetPosition();

				PStr=L"";
			}
			else
				PStr=strStr;

			SetString(PStr);
			return TRUE;
		case KEY_F2:
		{
			UserMenu Menu(false);
			return TRUE;
		}
		case KEY_ALTF8:
		{
			int Type;
			int SelectType=CtrlObject->CmdHistory->Select(Msg::HistoryTitle,L"History",strStr,Type);
			switch (SelectType)
			{
				case HRT_ENTER:
				case HRT_SHIFTENTER:
				case HRT_CTRLALTENTER:
					CmdStr.DisableAC();
					SetString(strStr);
					ProcessKey(SelectType == HRT_CTRLALTENTER ? FarKey(KEY_CTRLALTENTER)
						: SelectType == HRT_ENTER ? FarKey(KEY_ENTER)
						: FarKey(KEY_SHIFTENTER));
					CmdStr.RevertAC();
					break;

				case HRT_CTRLENTER:
					SetString(strStr);
					break;
			}
			return TRUE;
		}
		case KEY_SHIFTF9:
			ConfigOptSave(true);
			return TRUE;
		case KEY_F10:
			FrameManager->ExitMainLoop(true);
			return TRUE;
		case KEY_ALTF10:
		{
			Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
			{
				// TODO: здесь можно добавить проверку, что мы в корне диска и отсутствие файла Tree.Far...
				FolderTree Tree(strStr,MODALTREE_ACTIVE,TRUE,FALSE);
			}
			CtrlObject->Cp()->RedrawKeyBar();

			if (!strStr.IsEmpty())
			{
				ActivePanel->SetCurDir(strStr,true);
				ActivePanel->Show();

				if (ActivePanel->GetType()==TREE_PANEL)
					ActivePanel->ProcessKey(KEY_ENTER);
			}
			else
			{
				// TODO: ... а здесь проверить факт изменения/появления файла Tree.Far и мы опять же в корне (чтобы лишний раз не апдейтить панель)
				ActivePanel->Update(UPDATE_KEEP_SELECTION);
				ActivePanel->Redraw();
				Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

				if (AnotherPanel->NeedUpdatePanel(ActivePanel))
				{
					AnotherPanel->Update(UPDATE_KEEP_SELECTION);//|UPDATE_SECONDARY);
					AnotherPanel->Redraw();
				}
			}
		}
		return TRUE;
		case KEY_F11:
			CtrlObject->Plugins.CommandsMenu(FALSE,FALSE,0);
			return TRUE;
		case KEY_ALTF11:
			ShowViewEditHistory();
			CtrlObject->Cp()->Redraw();
			return TRUE;
		case KEY_ALTF12:
		{
			int Type;
			int SelectType=CtrlObject->FolderHistory->Select(Msg::FolderHistoryTitle,L"HistoryFolders",strStr,Type);

			if (SelectType == HRT_ENTER || SelectType == HRT_SHIFTENTER || SelectType == HRT_CTRLSHIFTENTER)
			{
				if (SelectType == HRT_SHIFTENTER)
					CtrlObject->FolderHistory->SetAddMode(false,2,true);

				// пусть плагин сам прыгает... ;-)
				Panel *Panel=CtrlObject->Cp()->ActivePanel;

				if (SelectType == HRT_CTRLSHIFTENTER) // на пассивную панель со сменой позиции
					Panel=CtrlObject->Cp()->GetAnotherPanel(Panel);

				//Type==1 - плагиновый путь
				//Type==0 - обычный путь
				//если путь плагиновый то сначала попробуем запустить его (а вдруг там префикс)
				//ну а если путь не плагиновый то запускать его точно не надо
				if (!Type || !CtrlObject->Plugins.ProcessCommandLine(strStr,Panel))
				{
					if (Panel->GetMode() == PLUGIN_PANEL || CheckShortcutFolder(&strStr,FALSE))
					{
						Panel->SetCurDir(strStr,Type ? false:true);
						// restore current directory to active panel path
						if(SelectType == HRT_CTRLSHIFTENTER)
						{
							CtrlObject->Cp()->ActivePanel->SetCurPath();
						}
						Panel->Redraw();
						CtrlObject->FolderHistory->SetAddMode(true,2,true);
					}
				}
			}
			else if (SelectType == HRT_CTRLENTER)
				SetString(strStr);
		}
		return TRUE;
		case KEY_NUMENTER:
		case KEY_SHIFTNUMENTER:
		case KEY_ENTER:
		case KEY_SHIFTENTER:
		case KEY_CTRLALTENTER:
		case KEY_CTRLALTNUMENTER:
		{
			Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
			CmdStr.Select(-1,0);
			CmdStr.Show();
			CmdStr.GetString(strStr);
			RemoveTrailingSpaces(strStr);

			if (strStr.IsEmpty())
				break;

			ActivePanel->SetCurPath();

			if (!(Opt.ExcludeCmdHistory & EXCLUDECMDHISTORY_NOTCMDLINE)) {
				FARString curDir;
				ActivePanel->GetCurDirPluginAware(curDir);
				CtrlObject->CmdHistory->AddToHistoryExtra(strStr, curDir);
			}

			if (ActivePanel->ProcessPluginEvent(FE_COMMAND,(void *)strStr.CPtr())) {
				ActivePanel->GetCurDirPluginAware(strCurDir, false);
				Show();
				ActivePanel->SetTitle();

			} else {
				CmdExecute(strStr, Key==KEY_SHIFTENTER||Key==KEY_SHIFTNUMENTER, false, false, false, Key == KEY_CTRLALTENTER || Key == KEY_CTRLALTNUMENTER);
			}

		}
		return TRUE;
		case KEY_CTRLU:
			CmdStr.Select(-1,0);
			CmdStr.Show();
			return TRUE;
		case KEY_OP_XLAT:
		{
			// 13.12.2000 SVS - ! Для CmdLine - если нет выделения, преобразуем всю строку (XLat)
			CmdStr.Xlat(Opt.XLat.Flags&XLAT_CONVERTALLCMDLINE?TRUE:FALSE);

			// иначе неправильно работает ctrl-end
			strLastCmdStr = CmdStr.GetStringAddr();
			LastCmdPartLength=(int)strLastCmdStr.GetLength();

			return TRUE;
		}
		/* дополнительные клавиши для выделения в ком строке.
		   ВНИМАНИЕ!
		   Для сокращения кода этот кусок должен стоять перед "default"
		*/
		case KEY_ALTSHIFTLEFT:  case KEY_ALTSHIFTNUMPAD4:
		case KEY_ALTSHIFTRIGHT: case KEY_ALTSHIFTNUMPAD6:
		case KEY_ALTSHIFTEND:   case KEY_ALTSHIFTNUMPAD1:
		case KEY_ALTSHIFTHOME:  case KEY_ALTSHIFTNUMPAD7:
			Key&=~KEY_ALT;
		default:

			//   Сбрасываем выделение на некоторых клавишах
			if (!Opt.CmdLine.EditBlock)
			{
				static FarKey UnmarkKeys[]=
				{
					KEY_LEFT,       KEY_NUMPAD4,
					KEY_CTRLS,
					KEY_RIGHT,      KEY_NUMPAD6,
					KEY_CTRLD,
					KEY_CTRLLEFT,   KEY_CTRLNUMPAD4,
					KEY_CTRLRIGHT,  KEY_CTRLNUMPAD6,
					KEY_CTRLHOME,   KEY_CTRLNUMPAD7,
					KEY_CTRLEND,    KEY_CTRLNUMPAD1,
					KEY_HOME,       KEY_NUMPAD7,
					KEY_END,        KEY_NUMPAD1
				};

				for (size_t I=0; I< ARRAYSIZE(UnmarkKeys); I++)
					if (Key==UnmarkKeys[I])
					{
						CmdStr.Select(-1,0);
						break;
					}
			}

			if (Key == KEY_CTRLD)
				Key=KEY_RIGHT;

			if (!CmdStr.ProcessKey(Key))
				break;

			LastCmdPartLength=-1;

			if(Key == KEY_CTRLSHIFTEND || Key == KEY_CTRLSHIFTNUMPAD1)
			{
				CmdStr.EnableAC();
				CmdStr.AutoComplete(true,false);
				CmdStr.RevertAC();
			}

			return TRUE;
	}

	return FALSE;
}


BOOL CommandLine::SetCurDir(const wchar_t *CurDir)
{
	if (StrCmp(strCurDir,CurDir) || !TestCurrentDirectory(CurDir))
	{
		strCurDir = CurDir;
	}

	return TRUE;
}


int CommandLine::GetCurDir(FARString &strCurDir)
{
	strCurDir = CommandLine::strCurDir;
	return (int)strCurDir.GetLength();
}


void CommandLine::SetString(const wchar_t *Str,bool Redraw)
{
	LastCmdPartLength=-1;
	CmdStr.SetString(Str);
	CmdStr.SetLeftPos(0);

	if (Redraw)
		CmdStr.Show();
}


void CommandLine::ExecString(const wchar_t *Str, bool SeparateWindow,
                             bool DirectRun, bool WaitForIdle, bool Silent, bool RunAs)
{
	CmdStr.DisableAC();
	SetString(Str);
	CmdStr.RevertAC();
	CmdExecute(Str,SeparateWindow,DirectRun, WaitForIdle, Silent, RunAs);
}


void CommandLine::InsertString(const wchar_t *Str)
{
	LastCmdPartLength=-1;
	CmdStr.InsertString(Str);
	CmdStr.Show();
}


int CommandLine::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if(MouseEvent->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED && MouseEvent->dwMousePosition.X==X2+1)
	{
		return ProcessKey(KEY_ALTF8);
	}
	int r = CmdStr.ProcessMouse(MouseEvent);
	if (r==0) {
		if (MouseEvent->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED) {
			WINPORT(BeginConsoleAdhocQuickEdit)();
		}
	}
	return r;
}

void CommandLine::GetPrompt(FARString &strDestStr)
{
	FARString strExpandedFormatStr;
	if (Opt.CmdLine.UsePromptFormat) {
		FARString strFormatStr;
		strFormatStr = Opt.CmdLine.strPromptFormat;
		apiExpandEnvironmentStrings(strFormatStr, strExpandedFormatStr);

	} else {	// default prompt
		strExpandedFormatStr = "$p$# ";
	}

	constexpr wchar_t ChrFmt[][2] = {
			{L'A', L'&'},		// $A - & (Ampersand)
			{L'B', L'|'},		// $B - | (pipe)
			{L'C', L'('},		// $C - ( (Left parenthesis)
			{L'F', L')'},		// $F - ) (Right parenthesis)
			{L'G', L'>'},		// $G - > (greater-than sign)
			{L'L', L'<'},		// $L - < (less-than sign)
			{L'Q', L'='},		// $Q - = (equal sign)
			{L'S', L' '},		// $S - (space)
			{L'$', L'$'},		// $$ - $ (dollar sign)
	};

	const wchar_t *Format = strExpandedFormatStr;
	while (*Format) {
		if (*Format == L'$') {
			wchar_t Chr = Upper(*++Format);
			size_t I;

			for (I = 0; I < ARRAYSIZE(ChrFmt); ++I) {
				if (ChrFmt[I][0] == Chr) {
					strDestStr+= ChrFmt[I][1];
					break;
				}
			}

			if (I == ARRAYSIZE(ChrFmt)) {
				switch (Chr) {
						/*
						эти не раелизованы
						$E - Escape code (ASCII code 27)
						$V - Windows XP version number
						$_ - Carriage return and linefeed
						$M - Отображение полного имени удаленного диска, связанного с именем текущего диска, или пустой строки, если текущий диск не является сетевым.
						*/
					case L'+':		// $+ - Отображение нужного числа знаков плюс (+) в зависимости от текущей глубины стека каталогов PUSHD, по одному знаку на каждый сохраненный путь.
					{
						if (PushDirStackSize) {
							strDestStr.Append(L'+', PushDirStackSize);
						}

						break;
					}
					case L'H':		// $H - Backspace (erases previous character)
					{
						if (!strDestStr.IsEmpty())
							strDestStr.Truncate(strDestStr.GetLength() - 1);

						break;
					}
					case L'@':		// $@xx - Admin
					{
						wchar_t lb = *++Format;
						wchar_t rb = *++Format;
						if (Opt.IsUserAdmin) {
							strDestStr+= lb;
							strDestStr+= Msg::ConfigCmdlinePromptFormatAdmin;
							strDestStr+= rb;
						}
						break;
					}
					case L'D':		// $D - Current date
					case L'T':		// $T - Current time
					{
						FARString strDateTime;
						MkStrFTime(strDateTime, (Chr == L'D' ? L"%D" : L"%T"));
						strDestStr+= strDateTime;
						break;
					}
					case L'R':		// $R - Current drive and path, always full
					{
						strDestStr+= strCurDir;
						break;
					}
					case L'P':		// $P - Current drive and path, shortened home
					{
						const auto &strHome = CachedHomeDir();
						if (strCurDir.Begins(strHome)) {
							strDestStr+= L'~';
							strDestStr+= strCurDir.CPtr() + strHome.GetLength();
						} else {
							strDestStr+= strCurDir;
						}
						break;
					}
					case L'#':		// # or $ - depending of user root or not
					{
						strDestStr+= Opt.IsUserAdmin ? L"#" : L"$";
						break;
					}
					case L'U':		// User name
					{
						strDestStr+= CachedUserName();
						break;
					}
					case L'N':		// Host name
					{
						strDestStr+= CachedComputerName();
						break;
					}
				}
			}

			Format++;
		} else {
			strDestStr+= *(Format++);
		}
	}
}


void CommandLine::ShowViewEditHistory()
{
	FARString strStr;
	int Type;
	int SelectType=CtrlObject->ViewHistory->Select(Msg::ViewHistoryTitle,L"HistoryViews",strStr,Type);

	if (SelectType == HRT_ENTER || SelectType == HRT_SHIFTENTER)
	{
		if (SelectType == HRT_ENTER)
			CtrlObject->ViewHistory->AddToHistory(strStr,Type);

		CtrlObject->ViewHistory->SetAddMode(false, 1,true);

		switch (Type)
		{
			case HR_VIEWER:
			{
				new FileViewer(strStr,TRUE);
				break;
			}
			case HR_EDITOR: // обычное открытие в редакторе
			case HR_EDITOR_RO: // открытие с локом
			{
				// пусть файл создается
				FileEditor *FEdit=new FileEditor(strStr,CP_AUTODETECT,FFILEEDIT_CANNEWFILE|FFILEEDIT_ENABLEF6);

				if (Type == HR_EDITOR_RO)
					FEdit->SetLockEditor(TRUE);

				break;
			}
			// заполняется в ProcessExternal
			case HR_EXTERNAL:
			case HR_EXTERNAL_WAIT:
			{
				if (strStr.At(0) !=L'@')
				{
					ExecString(strStr);
					if (Type == HR_EXTERNAL_WAIT)
					{
						WaitForClose(strStr.CPtr());
					}
				}
				else
				{
					SaveScreen SaveScr;
					CtrlObject->Cp()->LeftPanel->CloseFile();
					CtrlObject->Cp()->RightPanel->CloseFile();
					Execute(strStr.CPtr()+1);
					if (Type == HR_EXTERNAL_WAIT)
					{
						WaitForClose(strStr.CPtr()+1);
					}
				}

				break;
			}
		}

		CtrlObject->ViewHistory->SetAddMode(true, 1, true);
	}
	else if (SelectType == HRT_CTRLENTER) // скинуть из истории в ком.строку?
		SetString(strStr);
}

void CommandLine::SaveBackground(int X1,int Y1,int X2,int Y2)
{
	if (BackgroundScreen)
	{
		delete BackgroundScreen;
	}

	BackgroundScreen=new SaveScreen(X1,Y1,X2,Y2);
}

void CommandLine::SaveBackground()
{
	if (BackgroundScreen)
	{
//		BackgroundScreen->Discard();
		BackgroundScreen->SaveArea();
		fprintf(stderr, "CommandLine::SaveBackground: done\n");
	}
	else
		fprintf(stderr, "CommandLine::SaveBackground: no BackgroundScreen\n");
}
void CommandLine::ShowBackground()
{
	if (BackgroundScreen)
	{
		BackgroundScreen->RestoreArea();
		fprintf(stderr, "CommandLine::ShowBackground: done\n");
	} else
		fprintf(stderr, "CommandLine::ShowBackground: no BackgroundScreen\n");
}

void CommandLine::CorrectRealScreenCoord()
{
	if (BackgroundScreen)
	{
		BackgroundScreen->CorrectRealScreenCoord();
	}
}

void CommandLine::ResizeConsole()
{
	BackgroundScreen->Resize(ScrX+1,ScrY+1,2,Opt.WindowMode!=FALSE);
//  this->DisplayObject();
}

void CommandLine::RedrawWithoutComboBoxMark()
{
	Redraw();
	// erase \x2191 character...
	DrawComboBoxMark(L' ');
}

const CHAR_INFO *CommandLine::GetBackgroundScreen(int &W, int &H)
{
	if (!BackgroundScreen)
		return NULL;

	W = (BackgroundScreen->X2 - BackgroundScreen->X1) + 1;
	H = (BackgroundScreen->Y2 - BackgroundScreen->Y1) + 1;
	return BackgroundScreen->GetBufferAddress();
}

