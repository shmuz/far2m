/*
manager.cpp

Переключение между несколькими file panels, viewers, editors, dialogs
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
#include "console.hpp"
#include "ctrlobj.hpp"
#include "DlgGuid.hpp"
#include "exitcode.hpp"
#include "filepanels.hpp"
#include "fileview.hpp"
#include "frame.hpp"
#include "grabber.hpp"
#include "interf.hpp"
#include "InterThreadCall.hpp"
#include "keyboard.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "manager.hpp"
#include "message.hpp"
#include "pathmix.hpp"
#include "plist.hpp"
#include "scrbuf.hpp"
#include "syslog.hpp"
#include "vmenu.hpp"
#include "vtshell.h"

// #define DEBUG_MANAGER
#ifdef DEBUG_MANAGER

#define _BASICLOG Log
void _FRAMELOG(const char* prefix, Frame* frame)
{
	if (frame)
	{
		FARString tp, nm;
		frame->GetTypeAndName(tp, nm);
		Log("%s: %p : %ls : %ls", prefix, frame, tp.CPtr(), nm.CPtr());
	}
	else
		Log("%s: nullptr", prefix);
}

#define _DUMP_FRAME_LIST() do { \
		for (auto iFrame: FrameList) \
			_FRAMELOG("--> ", iFrame); \
	} while(false)

#define _DUMP_FRAME_STACK() do { \
		for (auto iFrame: ModalStack) \
			_FRAMELOG("==> ", iFrame); \
	} while(false)

#else

#define _BASICLOG(a,...)
#define _FRAMELOG(a,b)
#define _DUMP_FRAME_LIST()
#define _DUMP_FRAME_STACK()

#endif // #ifdef DEBUG_MANAGER

Manager *FrameManager;

/* $ 29.12.2000 IS
  Аналог CloseAll, но разрешает продолжение полноценной работы в фаре,
  если пользователь продолжил редактировать файл.
  Возвращает TRUE, если все закрыли и можно выходить из фара.
*/
bool Manager::ExitAll()
{
	for (auto i=ModalStack.size(); i > 0; )
	{
		Frame *iFrame = ModalStack[--i];

		if (!iFrame->GetCanLoseFocus(false))
		{
			auto PrevFrameCount = ModalStack.size();
			iFrame->ProcessKey(KEY_ESC);
			Commit();

			if (PrevFrameCount == ModalStack.size())
				return false;
		}
	}

	for (auto i=FrameList.size(); i > 0; )
	{
		Frame *iFrame = FrameList[--i];

		if (!iFrame->GetCanLoseFocus(true))
		{
			ActivateFrame(iFrame);
			Commit();
			auto PrevFrameCount = FrameList.size();
			iFrame->ProcessKey(KEY_ESC);
			Commit();

			if (PrevFrameCount == FrameList.size())
				return false;
		}
	}

	return true;
}

void Manager::CloseAll()
{
	while (!ModalStack.empty())
	{
		DeleteCommit(ModalStack.back());
	}

	while (!FrameList.empty())
	{
		DeleteCommit(FrameList.back());
	}

	FrameList.clear();
	FramePos = 0;
}

void Manager::InsertFrame(Frame *aFrame)
{
	_FRAMELOG("InsertFrame", aFrame);

	InsertedFrame = aFrame;
}

void Manager::DeleteFrame(Frame *aFrame)
{
	_FRAMELOG("DeleteFrame", aFrame);

	if (!aFrame)
	{
		DeletedFrame = CurrentFrame;
	}
	else
	{
		for (auto I: FrameList)
		{
			if (I->RemoveModal(aFrame))
			{
				//### it seems we never get here
				//Log("if (iFrame->RemoveModal(aFrame))");
				return;
			}
		}
		DeletedFrame = aFrame;
	}
}

void Manager::ModalizeFrame(Frame *aFrame)
{
	_FRAMELOG("ModalizeFrame", aFrame);

	if (ActivatedFrame) // Issue #26 (the 1-st problem)
	{
		ActivateCommit(ActivatedFrame);
	}

	CurrentFrame->PushFrame(aFrame);
}

void Manager::UnmodalizeFrame(Frame *aFrame)
{
	_FRAMELOG("UnmodalizeFrame", aFrame);

	for (auto I: FrameList)
	{
		if (I->RemoveModal(aFrame))
			break;
	}

	for (auto I: ModalStack)
	{
		if (I->RemoveModal(aFrame))
			break;
	}
}

void Manager::ExecuteNonModal()
{
	Frame *iFrame = InsertedFrame ? InsertedFrame : ExecutedFrame ? ExecutedFrame : ActivatedFrame;
	_FRAMELOG("ExecuteNonModal", iFrame);

	if (!iFrame)
		return;

	if (InList(iFrame))
	{
		ActivateFrame(iFrame);
	}
	else
	{
		ExecutedFrame = nullptr;
		InsertCommit(iFrame);
	}

	while (Commit(), CurrentFrame == iFrame && !EndLoop)
		ProcessMainLoop();
}

void Manager::ExecuteModal(Frame *aFrame)
{
	_FRAMELOG("ExecuteModal", aFrame);

	if (aFrame)
		ExecutedFrame = aFrame;

	if (!ExecutedFrame)
		return;

	auto ModalStartLevel = ModalStack.size();
	bool OriginalStartManager = StartManager;
	StartManager = true;

	while (Commit(), ModalStack.size() > ModalStartLevel)
		ProcessMainLoop();

	StartManager = OriginalStartManager;
}

/* $ 15.05.2002 SKV
  Так как нужно это в разных местах,
  а глобальные счётчики не концептуально,
  то лучше это делать тут.
*/
void Manager::ExecuteModalEV(bool RefreshScreen)
{
	ModalEVCount++;
	ExecuteModal();
	ModalEVCount--;
	if (RefreshScreen) {
		ProcessKey(KEY_CONSOLE_BUFFER_RESIZE); //redraw all
		Commit();
	}
}

static FARString FrameMenuNumTextPrefix(int i)
{
	FARString out;
	if (i < 10)
		out.Format(L"&%d ", i);
	else if (i < 36)
		out.Format(L"&%c ", char(i - 10 + 'A'));
	else
		out.Format(L"&  ");
	return out;
}

class FramesMenu : public VMenu
{
	VTInfos _vts;
	int _vts_base_index{-1};

public:
	FramesMenu() : VMenu (Msg::ScreensTitle, nullptr, 0, ScrY - 4)
	{
		VTShell_Enum(_vts);
	}

	virtual int ProcessKey(FarKey Key)
	{
		if (Key == KEY_F3 && _vts_base_index >= 0
				&& _vts_base_index <= GetSelectPos()
				&& _vts_base_index + int(_vts.size()) > GetSelectPos() ) {
			ViewConsoleHistory(_vts[GetSelectPos() - _vts_base_index].con_hnd, true, false);
			return TRUE;
		}
		return VMenu::ProcessKey(Key);
	}

	void AddVTSItems(int FramePos)
	{
		if (_vts.empty()) {
			_vts_base_index = -1;
			return;
		}
		_vts_base_index = GetItemCount() + 1;
		MenuItemEx mi{};
		mi.strName = Msg::BackgroundCommands;
		mi.Flags = LIF_SEPARATOR;
		AddItem(&mi);
		for (const auto &vt : _vts) {
			mi.Clear();
			mi.strName = vt.title;
			ReplaceStrings(mi.strName, L"&", L"&&", -1);
			mi.strName.Insert(0, FrameMenuNumTextPrefix(GetItemCount() - 1) );
			mi.SetSelect(GetItemCount() == FramePos);
			if (vt.done)
				mi.SetCheck(vt.exit_code ? L'!' : L'#');

			AddItem(&mi);
		}
	}

	int Do()
	{
		VMenu::Process();
		int r = Modal::GetExitCode();
		if (_vts_base_index >= 0 && r >= _vts_base_index && r < GetItemCount()) {
			CtrlObject->CmdLine->SwitchToBackgroundTerminal(r - _vts_base_index);
			return -1;
		}
		return r;
	}
};

/*!
  \return Возвращает nullptr если нажат "отказ" или если нажат текущий фрейм.
  Другими словами, если немодальный фрейм не поменялся.
  Если же фрейм поменялся, то тогда функция должна возвратить
  указатель на предыдущий фрейм.
*/
Frame *Manager::FrameMenu()
{
	// Флаг для определения того, что меню переключения экранов уже активировано.
	static bool AlreadyShown = false;

	if (AlreadyShown)
		return nullptr;

	int ExitCode;
	bool CheckCanLoseFocus = CurrentFrame->GetCanLoseFocus();
	{
		MenuItemEx ModalMenuItem;
		FramesMenu ModalMenu;

		ModalMenu.SetHelp(L"ScrSwitch");
		ModalMenu.SetFlags(VMENU_WRAPMODE);
		ModalMenu.SetPosition(-1, -1, 0, 0);
		ModalMenu.SetId(ScreensSwitchId);

		if (!CheckCanLoseFocus)
			ModalMenuItem.SetDisable(true);

		for (int I=0; I < (int)FrameList.size(); I++)
		{
			/*  добавляется "*" если файл изменен */
			FARString strNumText = FrameMenuNumTextPrefix(I);
			FARString strType, strName;
			FrameList[I]->GetTypeAndName(strType, strName);
			ModalMenuItem.Clear();

			//TruncPathStr(strName, ScrX - 24);
			ReplaceStrings(strName, L"&", L"&&", -1);
			ModalMenuItem.strName.Format(L"%ls%-10.10ls %lc %ls", strNumText.CPtr(), strType.CPtr(),
				(FrameList[I]->IsFileModified() ? L'*' : L' '), strName.CPtr());
			ModalMenuItem.SetSelect(I == FramePos);
			if (FrameList[I]->IsFileModified())
				ModalMenuItem.SetCheck(L'*');

			ModalMenu.AddItem(&ModalMenuItem);
		}

		ModalMenu.AddVTSItems(FramePos);

		AlreadyShown = true;
		ExitCode = ModalMenu.Do();
		AlreadyShown = false;
	}

	if (CheckCanLoseFocus) {
		if (ExitCode >= 0 && ExitCode < GetFrameCount()) {
			ActivateFrame(ExitCode);
			return ActivatedFrame == CurrentFrame || !CurrentFrame->GetCanLoseFocus() ? nullptr : CurrentFrame;
		}

		return ActivatedFrame == CurrentFrame ? nullptr : CurrentFrame;
	}

	return nullptr;
}

int Manager::GetFrameCountByType(int Type) const
{
	int ret = 0;

	for (auto iFrame: FrameList)
	{
		/* $ 10.05.2001 DJ
		   не учитываем фрейм, который собираемся удалять
		*/
		if (iFrame == DeletedFrame || iFrame->GetExitCode() == XC_QUIT)
			continue;

		if (iFrame->GetType() == Type)
			ret++;
	}

	return ret;
}

/*$ 11.05.2001 OT Теперь можно искать файл не только по полному имени, но и отдельно - путь, отдельно имя */
Frame* Manager::FindFrameByFile(int ModalType, const wchar_t *FileName, const wchar_t *Dir) const
{
	FARString strFullFileName = FileName;

	if (Dir)
	{
		strFullFileName = Dir;
		AddEndSlash(strFullFileName);
		strFullFileName += FileName;
	}

	for (auto iFrame: FrameList)
	{
		// Mantis#0000469 - получать Name будем только при совпадении ModalType
		if (iFrame->GetType() == ModalType)
		{
			FARString strType, strName;
			iFrame->GetTypeAndName(strType, strName);

			if (!StrCmp(strName, strFullFileName))
				return iFrame;
		}
	}

	return nullptr;
}

bool Manager::ShowBackground()
{
	if (CtrlObject->CmdLine)
	{
		CtrlObject->CmdLine->ShowBackground();
		return true;
	}
	return false;
}

void Manager::ActivateFrame(Frame *aFrame)
{
	_FRAMELOG("ActivateFrame", aFrame);

	if (!ActivatedFrame && aFrame && (InList(aFrame) || InStack(aFrame)))
		ActivatedFrame = aFrame;
}

void Manager::ActivateFrame(int Index)
{
	if (!ActivatedFrame)
		ActivatedFrame = GetFrame(Index);
}

void Manager::DeactivateFrame(Frame *Deactivated, int Direction)
{
	_FRAMELOG("DeactivateFrame", Deactivated);

	if (Direction && FrameList.size())
	{
		FramePos = (FramePos + Direction + FrameList.size()) % FrameList.size();
		ActivateFrame(FramePos);
	}

	DeactivatedFrame = Deactivated;
}

void Manager::RefreshFrame(Frame *Refreshed)
{
	_FRAMELOG("RefreshFrame", Refreshed);

	if (ActivatedFrame)
		return;

	RefreshedFrame = Refreshed ? Refreshed : CurrentFrame;

	if (RefreshedFrame && (InList(RefreshedFrame) || InStack(RefreshedFrame)))
	{
		/* $ 13.04.2002 KM
			Вызываем принудительный Commit() для фрейма имеющего члена
			NextModal, это означает что активным сейчас является
			VMenu, а значит Commit() сам не будет вызван после возврата
			из функции.
			Устраняет ещё один момент неперерисовки, когда один над
			другим находится несколько объектов VMenu. Пример:
			настройка цветов. Теперь AltF9 в диалоге настройки
			цветов корректно перерисовывает меню.
		*/
		if (RefreshedFrame->NextModal)
			Commit();
	}
}

void Manager::RefreshFrame(int Index)
{
	RefreshFrame(GetFrame(Index));
}

void Manager::ExecuteFrame(Frame *Executed)
{
	_FRAMELOG("ExecuteFrame", Executed);

	ExecutedFrame = Executed;
}

/* $ 10.05.2001 DJ
   переключается на панели (фрейм с номером 0)
*/
void Manager::SwitchToPanels()
{
	ActivateFrame(0);
}

bool Manager::HaveAnyFrame() const
{
	return !FrameList.empty() || InsertedFrame || DeletedFrame || ActivatedFrame || RefreshedFrame ||
	        DeactivatedFrame || ExecutedFrame || CurrentFrame;
}

void Manager::EnterMainLoop()
{
	WaitInFastFind = 0;
	StartManager = true;

	while (Commit(0), !EndLoop && HaveAnyFrame())
		ProcessMainLoop();
}

void Manager::SetLastInputRecord(const INPUT_RECORD *Rec)
{
	if (&LastInputRecord != Rec)
		LastInputRecord = *Rec;
}


void Manager::ProcessMainLoop()
{
	DispatchInterThreadCalls();

	if (!CurrentFrame)
		return;

	CtrlObject->Macro.SetArea(CurrentFrame->GetMacroArea());

	if (!CurrentFrame->ProcessEvents())
	{
		ProcessKey(KEY_IDLE);
	}
	else
	{
		// Mantis#0000073: Не работает автоскролинг в QView
		WaitInMainLoop = IsPanelsActive() && ((FilePanels*)CurrentFrame)->ActivePanel->GetType() != QVIEW_PANEL;
		//WaitInFastFind++;
		FarKey Key = GetInputRecord(&LastInputRecord);
		//WaitInFastFind--;
		WaitInMainLoop = false;

		if (EndLoop)
			return;

		if (LastInputRecord.EventType == MOUSE_EVENT)
		{
				// используем копию структуры, т.к. LastInputRecord может внезапно измениться во время выполнения ProcessMouse
				MOUSE_EVENT_RECORD mer = LastInputRecord.Event.MouseEvent;
				ProcessMouse(&mer);
		}
		else if (Key != KEY_NONE)
			ProcessKey(Key);
	}
}

static bool ConfirmExit(size_t vts_cnt)
{
	int r;
	ExMessager m(Msg::Quit);
	m.Add(Msg::AskQuit);
	if (vts_cnt) {
		m.AddFormat(Msg::AskQuitVTS, (unsigned int)vts_cnt);
	}
	m.Add(Msg::Yes);
	m.Add(Msg::No);
	if (WINPORT(ConsoleBackgroundMode)(FALSE)) {
		m.Add(Msg::Background);
		r = m.Show(vts_cnt ? MSG_WARNING : 0, 3, &FarAskQuitId);
		if (r == 2) {
			WINPORT(ConsoleBackgroundMode)(TRUE);
		}
	} else {
		r = m.Show(vts_cnt ? MSG_WARNING : 0, 2, &FarAskQuitId);
	}

	return r == 0;
}

void Manager::ExitMainLoop(bool Ask, int ExitCode)
{
	if (CloseFAR)
	{
		CloseFAR = false;
		CloseFARMenu = true;
	}

	bool Exiting = true;
	if (Ask)
	{
		size_t vts_cnt = VTShell_Count();
		if (Opt.Confirm.ExitEffective() || vts_cnt)
			Exiting = ConfirmExit(vts_cnt);
		if (Exiting)
			Exiting = CtrlObject->Plugins.MayExitFar();
	}

	if (Exiting)
	{
		/* $ 29.12.2000 IS
		   + Проверяем, сохранены ли все измененные файлы. Если нет, то не выходим
		     из фара.
		*/
		if (ExitAll())
		{
			FarExitCode = ExitCode;

			//TODO: при закрытии по x нужно делать форсированный выход. Иначе могут быть
			//      глюки, например, при перезагрузке
			FilePanels *cp = CtrlObject->Cp();

			if (!cp || (!cp->LeftPanel->ProcessPluginEvent(FE_CLOSE) &&
			            !cp->RightPanel->ProcessPluginEvent(FE_CLOSE)))
			{
				EndLoop = true;
			}
		}
		else
		{
			CloseFARMenu = false;
		}
	}
}

int Manager::ProcessKey(FarKey Key)
{
	int ret = FALSE;

	if (CurrentFrame)
	{
		DWORD KeyM = Key & ~KEY_CTRLMASK;

		if (!((KeyM >= KEY_MACRO_BASE && KeyM <= KEY_MACRO_ENDBASE) || (KeyM >= KEY_OP_BASE && KeyM <= KEY_OP_ENDBASE))) // пропустим макро-коды
		{
			switch (CurrentFrame->GetType())
			{
				case MODALTYPE_PANELS:
					_ALGO(CleverSysLog clv(L"Manager::ProcessKey()"));
					_ALGO(SysLog(L"Key=%ls", _FARKEY_ToName(Key)));

					if (CtrlObject->Cp()->ActivePanel->SendKeyToPlugin(Key, true))
						return TRUE;

					break;

				case MODALTYPE_VIEWER:
					//if(((FileViewer*)CurrentFrame)->ProcessViewerInput(FrameManager->GetLastInputRecord()))
					//  return TRUE;
					break;

				case MODALTYPE_EDITOR:
					//if(((FileEditor*)CurrentFrame)->ProcessEditorInput(FrameManager->GetLastInputRecord()))
					//  return TRUE;
					break;

				case MODALTYPE_DIALOG:
					//((Dialog*)CurrentFrame)->CallDlgProc(DN_KEY,((Dialog*)CurrentFrame)->GetDlgFocusPos(),Key);
					break;

				default:
					break;
			}
		}

		/*** БЛОК ПРИВИЛЕГИРОВАННЫХ КЛАВИШ ! ***/

		/***   КОТОРЫЕ НЕЛЬЗЯ НАМАКРОСИТЬ    ***/
		switch (Key)
		{
			case KEY_ALT | KEY_NUMPAD0:
			case KEY_ALTINS:
				Grabber::Run();
				return TRUE;

			case KEY_CONSOLE_BUFFER_RESIZE:
				WINPORT(Sleep)(10);
				ResizeAllFrame();
				return TRUE;
		}

		/*** А вот здесь - все остальное! ***/
		if (!IsProcessAssignMacroKey)
			// в любом случае если кому-то ненужны все клавиши или
		{
			switch (Key)
			{
				// <Удалить после появления макрофункции Scroll>
				case KEY_CTRLALTUP:
					if(Opt.WindowMode)
					{
						Console.ScrollWindow(-1);
						return TRUE;
					}
					break;

				case KEY_CTRLALTDOWN:
					if(Opt.WindowMode)
					{
						Console.ScrollWindow(1);
						return TRUE;
					}
					break;

				case KEY_CTRLALTPGUP:
					if(Opt.WindowMode)
					{
						Console.ScrollWindow(-ScrY);
						return TRUE;
					}
					break;

				case KEY_CTRLALTHOME:
					if(Opt.WindowMode)
					{
						while(Console.ScrollWindow(-ScrY));
						return TRUE;
					}
					break;

				case KEY_CTRLALTPGDN:
					if(Opt.WindowMode)
					{
						Console.ScrollWindow(ScrY);
						return TRUE;
					}
					break;

				case KEY_CTRLALTEND:
					if(Opt.WindowMode)
					{
						while(Console.ScrollWindow(ScrY));
						return TRUE;
					}
					break;
				// </Удалить после появления макрофункции Scroll>

				case KEY_CTRLW:
					ShowProcessList();
					return TRUE;

				case KEY_F11:
					PluginsMenu();
					RefreshFrame();
					return TRUE;

				case KEY_ALTF9:
				{
					WINPORT(Sleep)(10);
					ToggleVideoMode();
					WINPORT(Sleep)(10);

					/* В процессе исполнения Alt-F9 (в нормальном режиме) в очередь
					   консоли попадает WINDOW_BUFFER_SIZE_EVENT, формируется в
					   ChangeVideoMode().
					   В режиме исполнения макросов ЭТО не происходит по вполне понятным
					   причинам.
					*/
					if (CtrlObject->Macro.IsExecuting())
					{
						int PScrX = ScrX;
						int PScrY = ScrY;
						WINPORT(Sleep)(10);
						GetVideoMode(CurSize);

						if (PScrX+1 == CurSize.X && PScrY+1 == CurSize.Y)
						{
							return TRUE;
						}
						else
						{
							PrevScrX = PScrX;
							PrevScrY = PScrY;
							WINPORT(Sleep)(10);
							return ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
						}
					}

					return TRUE;
				}

				case KEY_F12:
				{
					auto CurFrame = GetCurrentFrame();
					int TypeFrame = CurFrame->GetType();

					if ((TypeFrame != MODALTYPE_HELP && TypeFrame != MODALTYPE_DIALOG) || CurFrame->GetCanLoseFocus())
					{
						DeactivateFrame(FrameMenu(), 0);
						return TRUE;
					}

					break; // отдадим F12 дальше по цепочке
				}

				case KEY_CTRLALTSHIFTPRESS:
				case KEY_RCTRLALTSHIFTPRESS:
				{
					if (!(Opt.CASRule&1) && Key == KEY_CTRLALTSHIFTPRESS)
						break;

					if (!(Opt.CASRule&2) && Key == KEY_RCTRLALTSHIFTPRESS)
						break;

					if (!Opt.OnlyEditorViewerUsed)
					{
						if (CurrentFrame->FastHide())
						{
							int isPanelFocus = CurrentFrame->GetType() == MODALTYPE_PANELS;

							if (isPanelFocus)
							{
								int LeftVisible = CtrlObject->Cp()->LeftPanel->IsVisible();
								int RightVisible = CtrlObject->Cp()->RightPanel->IsVisible();
								int CmdLineVisible = CtrlObject->CmdLine->IsVisible();
								int KeyBarVisible = CtrlObject->Cp()->MainKeyBar.IsVisible();
								CtrlObject->CmdLine->ShowBackground();
								CtrlObject->Cp()->LeftPanel->Hide0();
								CtrlObject->Cp()->RightPanel->Hide0();

								switch (Opt.PanelCtrlAltShiftRule)
								{
									case 0:
										CtrlObject->CmdLine->Show();
										CtrlObject->Cp()->MainKeyBar.Refresh(true);
										break;
									case 1:
										CtrlObject->Cp()->MainKeyBar.Refresh(true);
										break;
								}
								WaitKey(Key==KEY_CTRLALTSHIFTPRESS?KEY_CTRLALTSHIFTRELEASE:KEY_RCTRLALTSHIFTRELEASE);

								if (LeftVisible)      CtrlObject->Cp()->LeftPanel->Show();

								if (RightVisible)     CtrlObject->Cp()->RightPanel->Show();

								if (CmdLineVisible)   CtrlObject->CmdLine->Show();

								CtrlObject->Cp()->MainKeyBar.Refresh(KeyBarVisible);
							}
							else
							{
								ImmediateHide();
								WaitKey(Key == KEY_CTRLALTSHIFTPRESS ? KEY_CTRLALTSHIFTRELEASE : KEY_RCTRLALTSHIFTRELEASE);
							}

							RefreshFrame();
						}

						return TRUE;
					}

					break;
				}

				case KEY_CTRLTAB:
				case KEY_CTRLSHIFTTAB:
					if (CurrentFrame->GetCanLoseFocus())
					{
						DeactivateFrame(CurrentFrame, Key == KEY_CTRLTAB ? 1 : -1);
						return TRUE;
					}
					break;
			}
		}

		CurrentFrame->UpdateKeyBar();
		CurrentFrame->ProcessKey(Key);
	}

	return ret;
}

int Manager::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	// При каптюренной мыши отдаем управление заданному объекту
//    if (ScreenObject::CaptureMouseObject)
//      return ScreenObject::CaptureMouseObject->ProcessMouse(MouseEvent);
	return CurrentFrame ? CurrentFrame->ProcessMouse(MouseEvent) : FALSE;
}

void Manager::PluginsMenu()
{
	int curType = CurrentFrame->GetType();

	if (curType == MODALTYPE_PANELS || curType == MODALTYPE_EDITOR || curType == MODALTYPE_VIEWER
			|| curType == MODALTYPE_DIALOG)
	{
		/* 02.01.2002 IS
		   ! Вывод правильной помощи по Shift-F1 в меню плагинов в редакторе/вьюере/диалоге
		   ! Если на панели QVIEW или INFO открыт файл, то считаем, что это
		     полноценный вьюер и запускаем с соответствующим параметром плагины
		*/
		if (curType == MODALTYPE_PANELS)
		{
			int pType = CtrlObject->Cp()->ActivePanel->GetType();

			if (pType == QVIEW_PANEL || pType == INFO_PANEL)
			{
				FARString strType, strCurFileName;
				CtrlObject->Cp()->GetTypeAndName(strType, strCurFileName);

				if (!strCurFileName.IsEmpty())
				{
					DWORD Attr = apiGetFileAttributes(strCurFileName);

					// интересуют только обычные файлы
					if (Attr != INVALID_FILE_ATTRIBUTES && !(Attr & FILE_ATTRIBUTE_DIRECTORY))
						curType = MODALTYPE_VIEWER;
				}
			}
		}

		CtrlObject->Plugins.CommandsMenu(curType);
	}
}

bool Manager::IsPanelsActive() const
{
	return FramePos >= 0 && CurrentFrame && CurrentFrame->GetType() == MODALTYPE_PANELS;
}

Frame *Manager::GetFrame(size_t Index) const
{
	return Index < FrameList.size() ? FrameList[Index] : nullptr;
}

Frame *Manager::GetModal(size_t Index) const
{
	return Index < ModalStack.size() ? ModalStack[Index] : nullptr;
}

Frame *Manager::GetFrameEx(int Pos) const
{
	//  Если Pos == -1 то берем текущий фрейм
	if (Pos == -1) {
		Frame *topModal = GetTopModal();
		bool bHiddenMenu = topModal && dynamic_cast<VMenu*>(topModal) && !topModal->IsVisible();
		return bHiddenMenu ? GetCurrentFrame() : topModal;
	}
	return GetFrame(Pos);
}

int Manager::IndexOfStack(Frame *aFrame) const
{
	auto it = std::find(ModalStack.begin(), ModalStack.end(), aFrame);
	return it != ModalStack.end() ? std::distance(ModalStack.begin(), it) : -1;
}

int Manager::IndexOfList(Frame *aFrame) const
{
	auto it = std::find(FrameList.begin(), FrameList.end(), aFrame);
	return it != FrameList.end() ? std::distance(FrameList.begin(), it) : -1;
}

void Manager::Commit(int Count)
{
	while(true)
	{
		_BASICLOG("Commit");

		if (DeletedFrame && (InsertedFrame || ExecutedFrame))
		{
			UpdateCommit(DeletedFrame, InsertedFrame, ExecutedFrame);
		}
		else if (ExecutedFrame)
		{
			ExecuteCommit(ExecutedFrame);
		}
		else if (DeletedFrame)
		{
			DeleteCommit(DeletedFrame);
		}
		else if (InsertedFrame)
		{
			InsertCommit(InsertedFrame);
		}
		else if (DeactivatedFrame)
		{
			// $ 18.04.2002 skv
			// Если нечего активировать, то в общем-то не надо и деактивировать.
			if (ActivatedFrame)
			{
				DeactivateCommit(DeactivatedFrame, ActivatedFrame);
				ActivateCommit(ActivatedFrame);
			}
			else
				DeactivatedFrame = nullptr;
		}
		else if (ActivatedFrame)
		{
			ActivateCommit(ActivatedFrame);
		}
		else if (RefreshedFrame)
		{
			RefreshCommit(RefreshedFrame);
		}
		else
		{
			if (FolderChangedCount)
			{
				FolderChangedCount = 0;
				CtrlObject->Plugins.ProcessSynchroEvent(SE_FOLDERCHANGED, nullptr);
			}
			break;
		}

		if (Count > 0 && --Count == 0)
		{
			break;
		}
	}
}

void Manager::DeactivateCommit(Frame *aDeactivated, Frame *aActivated)
{
	_FRAMELOG("DeactivateCommit (deactivated)", aDeactivated);
	_FRAMELOG("DeactivateCommit (activated)", aActivated);
	DeactivatedFrame = nullptr;

	aDeactivated->OnChangeFocus(false);

	if (!ModalStack.empty() && aDeactivated == ModalStack.back())
	{
		if (InStack(aActivated))
		{
			ModalStack.pop_back();
		}
		else
		{
			ModalStack.back() = aActivated;
		}
	}
}

void Manager::ActivateCommit(Frame *aFrame)
{
	_FRAMELOG("ActivateCommit", aFrame);
	ActivatedFrame = nullptr;

	if (CurrentFrame == aFrame)
	{
		RefreshedFrame = aFrame;
		return;
	}

	int Index = IndexOfList(aFrame);
	if (Index != -1)
	{
		FramePos = Index;
		if (CurrentFrame && InList(CurrentFrame)) {
			aFrame->FrameToBack = CurrentFrame;
		}
	}

	/* 14.05.2002 SKV
	  Если мы пытаемся активировать полумодальный фрэйм,
	  то надо его вытащить на верх стэка модалов.
	*/
	Index = IndexOfStack(aFrame);
	if (Index != -1)
	{
		ModalStack.erase(ModalStack.begin()+Index);
		ModalStack.push_back(aFrame);
	}

	RefreshedFrame = CurrentFrame = aFrame;
}

void Manager::UpdateCommit(Frame *aDeleted, Frame *aInserted, Frame *aExecuted)
{
	_BASICLOG("UpdateCommit: DeletedFrame=%p, InsertedFrame=%p, ExecutedFrame=%p", aDeleted,aInserted,aExecuted);
	DeletedFrame = InsertedFrame = ExecutedFrame = nullptr;

	if (aExecuted)
	{
		DeleteCommit(aDeleted);
		ExecuteCommit(aExecuted);
	}
	else if (aInserted)
	{
		int Index = IndexOfList(aDeleted);
		if (-1 != Index)
		{
			FrameList[Index] = aInserted;
			ActivateFrame(aInserted);
			ActivatedFrame->FrameToBack = CurrentFrame;
			DeleteCommit(aDeleted);
		}
	}
}

//! Удаляет DeletedFrame изо всех очередей!
//! Назначает следующий активный, (исходя из своих представлений)
//! Но только в том случае, если активный фрейм еще не назначен заранее.
void Manager::DeleteCommit(Frame *aFrame)
{
	_FRAMELOG("DeleteCommit", aFrame);
	DeletedFrame = nullptr;

	for (auto iFrame: FrameList)
	{
		if (iFrame->FrameToBack == aFrame)
		{
			iFrame->FrameToBack = CtrlObject->Cp();
		}
	}

	int Index = IndexOfList(aFrame);
	if (Index != -1)
	{
		_DUMP_FRAME_LIST();

		aFrame->DestroyAllModal();

		FrameList.erase(FrameList.begin() + Index);

		if (FramePos >= (int)FrameList.size())
		{
			FramePos = 0;
		}

		if (aFrame->FrameToBack == CtrlObject->Cp())
		{
			if (!FrameList.empty())
			{
				_BASICLOG("== ActivateFrame(FrameList[FramePos])");
				ActivateFrame(FrameList[FramePos]);
			}
		}
		else
		{
			_BASICLOG("== ActivateFrame(aFrame->FrameToBack)");
			ActivateFrame(aFrame->FrameToBack);
		}
	}

	aFrame->OnDestroy();

	if (aFrame->GetDynamicallyBorn())
	{
		if (CurrentFrame == aFrame)
			CurrentFrame = nullptr;

		/* $ 14.05.2002 SKV
		  Так как в деструкторе фрэйма неявно может быть
		  вызван commit, то надо подстраховаться.
		*/
		DeletedFrame = nullptr;
		delete aFrame;
	}

	if (Index == -1)
	{
		/* $ 14.05.2002 SKV
			Надёжнее найти и удалить именно то, что
			нужно, а не просто верхний.
		*/
		Index = IndexOfStack(aFrame);
		if (Index != -1)
		{
			ActivateFrame(FramePos);
			Commit();
			ModalStack.erase(ModalStack.begin() + Index);
			if (!ModalStack.empty())
				ActivateFrame(ModalStack.back());
		}
	}
}

void Manager::InsertCommit(Frame *aFrame)
{
	_FRAMELOG("InsertCommit", aFrame);
	InsertedFrame = nullptr;

	aFrame->FrameToBack = CurrentFrame;
	FrameList.push_back(aFrame);

	if (!ActivatedFrame)
		ActivatedFrame = aFrame;
}

void Manager::RefreshCommit(Frame *aFrame)
{
	_FRAMELOG("RefreshCommit", aFrame);
	RefreshedFrame = nullptr;

	if (!InList(aFrame) && !InStack(aFrame))
		return;

	if (!aFrame->Locked())
	{
		if (!IsRedrawFramesInProcess)
			aFrame->ShowConsoleTitle();

		aFrame->Refresh();

		if (auto tm = GetTopModal(); tm && tm->IsVisible() && tm != aFrame)
			tm->Refresh();

		while (aFrame->NextModal)
			aFrame = aFrame->NextModal;

		CtrlObject->Macro.SetArea(aFrame->GetMacroArea());
	}

	bool bShowTime = (aFrame->GetType() == MODALTYPE_EDITOR || aFrame->GetType() == MODALTYPE_VIEWER) ?
		Opt.ViewerEditorClock : (WaitInMainLoop && Opt.Clock);

	if (bShowTime)
		ShowTime(SHTM_FORCE);
}

void Manager::ExecuteCommit(Frame *aFrame)
{
	_FRAMELOG("ExecuteCommit", aFrame);
	ExecutedFrame = nullptr;

	ModalStack.push_back(aFrame);
	ActivatedFrame = aFrame;
}

/* $ Введена для нужд CtrlAltShift OT */
void Manager::ImmediateHide()
{
	_BASICLOG("ImmediateHide");
	if (FramePos < 0)
		return;

	// Сначала проверяем, есть ли у прятываемого фрейма SaveScreen
	if (CurrentFrame->HasSaveScreen())
	{
		CurrentFrame->Hide();
		return;
	}

	// Фреймы перерисовываются, значит для нижних
	// не выставляем заголовок консоли, чтобы не мелькал.
	if (!ModalStack.empty())
	{
		/* $ 28.04.2002 KM
		    Проверим, а не модальный ли редактор или вьювер на вершине
		    модального стека? И если да, покажем User screen.
		*/
		auto type = ModalStack.back()->GetType();
		if (type == MODALTYPE_EDITOR || type == MODALTYPE_VIEWER)
		{
			if (CtrlObject->CmdLine)
				CtrlObject->CmdLine->ShowBackground();
		}
		else if (auto BottomFrame = GetBottomFrame())
		{
			int UnlockCount = 0;
			IsRedrawFramesInProcess++;

			while (BottomFrame->Locked())
			{
				BottomFrame->Unlock();
				UnlockCount++;
			}

			RefreshFrame(BottomFrame);
			Commit();

			for (int i=0; i < UnlockCount; i++)
			{
				BottomFrame->Lock();
			}

			if (ModalStack.size() > 1)
			{
				for (int i=0; i < (int)ModalStack.size()-1; i++)
				{
					if (!(ModalStack[i]->FastHide() & CASR_HELP))
					{
						RefreshFrame(ModalStack[i]);
						Commit();
					}
					else
					{
						break;
					}
				}
			}

			/* $ 04.04.2002 KM
			   Перерисуем заголовок только у активного фрейма.
			   Этим мы предотвращаем мелькание заголовка консоли
			   при перерисовке всех фреймов.
			*/
			IsRedrawFramesInProcess--;
			CurrentFrame->ShowConsoleTitle();
		}
	}
	else
	{
		if (CtrlObject->CmdLine)
			CtrlObject->CmdLine->ShowBackground();
	}
}

/*  Вызов ResizeConsole для всех NextModal у
    модального фрейма. KM
*/
void Manager::ResizeAllModal(Frame *ModalFrame)
{
	Frame *iModal = ModalFrame->NextModal;

	while (iModal)
	{
		iModal->ResizeConsole();
		iModal = iModal->NextModal;
	}
}

void Manager::ResizeAllFrame()
{
	ScrBuf.Lock();
	for (auto iFrame: FrameList)
	{
		iFrame->ResizeConsole();
	}

	for (auto iFrame: ModalStack)
	{
		iFrame->ResizeConsole();
		/* $ 13.04.2002 KM
		  - А теперь проресайзим все NextModal...
		*/
		ResizeAllModal(iFrame);
	}

	ImmediateHide();
	RefreshFrame();
	ScrBuf.Unlock();
}

void Manager::InitKeyBar()
{
	for (auto iFrame: FrameList)
		iFrame->InitKeyBar();
}

// возвращает top-модал или сам фрейм, если у фрейма нету модалов
Frame* Manager::GetTopModal() const
{
	Frame *f = CurrentFrame;
	if (f)
	{
		while (f->NextModal)
			f = f->NextModal;
	}
	return f;
}

LockFrame::LockFrame(Frame *frame)
	:
	_frame(frame), _refresh(false)
{
	if (_frame)
		_frame->Lock();
}

LockFrame::~LockFrame()
{
	if (_frame)
		_frame->Unlock();

	if (_refresh && FrameManager)
		FrameManager->RefreshFrame(_frame);
}

LockBottomFrame::LockBottomFrame()
	:
	LockFrame(FrameManager ? FrameManager->GetBottomFrame() : nullptr)
{}

LockCurrentFrame::LockCurrentFrame()
	:
	LockFrame(FrameManager ? FrameManager->GetCurrentFrame() : nullptr)
{}
