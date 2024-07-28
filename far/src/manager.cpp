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

#include "manager.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "frame.hpp"
#include "vmenu.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "savescr.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "grabber.hpp"
#include "message.hpp"
#include "config.hpp"
#include "plist.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "exitcode.hpp"
#include "scrbuf.hpp"
#include "console.hpp"
#include "InterThreadCall.hpp"
#include "DlgGuid.hpp"
#include "vtshell.h"

// #define DEBUG_MANAGER
#ifdef DEBUG_MANAGER

#define _BASICLOG Log
void _FRAMELOG(const char* prefix, Frame* frame)
{
	if (frame)
	{
		FARString tp, nm;
		frame->GetTypeAndName(tp,nm);
		Log("%s: %p : %ls : %ls", prefix, frame, tp.CPtr(), nm.CPtr());
	}
	else
		Log("%s: nullptr", prefix);
}

#define _DUMP_FRAME_LIST() do {          \
		for (auto iFrame: FrameList)      \
			_FRAMELOG("--> ", iFrame);       \
	} while(false)

#define _DUMP_FRAME_STACK() do {         \
		for (auto iFrame: ModalStack)     \
			_FRAMELOG("==> ", iFrame);       \
	} while(false)

#else

#define _BASICLOG(a,...)
#define _FRAMELOG(a,b)
#define _DUMP_FRAME_LIST()
#define _DUMP_FRAME_STACK()

#endif // #ifdef DEBUG_MANAGER

Manager *FrameManager;

Manager::Manager():
	FramePos(-1),
	InsertedFrame(nullptr),
	DeletedFrame(nullptr),
	ActivatedFrame(nullptr),
	RefreshedFrame(nullptr),
	DeactivatedFrame(nullptr),
	ExecutedFrame(nullptr),
	CurrentFrame(nullptr),
	ModalEVCount(0),
	EndLoop(false),
	StartManager(false)
{
}

Manager::~Manager()
{
}


/* $ 29.12.2000 IS
  Аналог CloseAll, но разрешает продолжение полноценной работы в фаре,
  если пользователь продолжил редактировать файл.
  Возвращает TRUE, если все закрыли и можно выходить из фара.
*/
bool Manager::ExitAll()
{
	for (int i=(int)ModalStack.size()-1; i>=0; i--)
	{
		Frame *iFrame=ModalStack[i];

		if (!iFrame->GetCanLoseFocus(true))
		{
			auto PrevFrameCount=ModalStack.size();
			iFrame->ProcessKey(KEY_ESC);
			Commit();

			if (PrevFrameCount==ModalStack.size())
			{
				return false;
			}
		}
	}

	for (int i=(int)FrameList.size()-1; i>=0; i--)
	{
		Frame *iFrame=FrameList[i];

		if (!iFrame->GetCanLoseFocus(true))
		{
			ActivateFrame(iFrame);
			Commit();
			auto PrevFrameCount=FrameList.size();
			iFrame->ProcessKey(KEY_ESC);
			Commit();

			if (PrevFrameCount==FrameList.size())
			{
				return false;
			}
		}
	}

	return true;
}

void Manager::CloseAll()
{
	for (int i=(int)ModalStack.size()-1; i>=0; i--)
	{
		DeleteCommit(ModalStack[i]);
		DeletedFrame=nullptr;
	}

	for (int i=(int)FrameList.size()-1; i>=0; i--)
	{
		DeleteCommit(FrameList[i]);
		DeletedFrame=nullptr;
	}

	FrameList.clear();
	FramePos=0;
}

void Manager::InsertFrame(Frame *Inserted)
{
	_FRAMELOG("InsertFrame", Inserted);

	InsertedFrame=Inserted;
}

void Manager::DeleteFrame(Frame *Deleted)
{
	_FRAMELOG("DeleteFrame", Deleted);

	if (!Deleted)
	{
		DeletedFrame=CurrentFrame;
	}
	else
	{
		for (auto iFrame: FrameList)
		{
			if (iFrame->RemoveModal(Deleted))
			{
				//### it seems we never get here
				//Log("if (iFrame->RemoveModal(Deleted))");
				return;
			}
		}
		DeletedFrame=Deleted;
	}
}

void Manager::ModalizeFrame(Frame *Modalized)
{
	_FRAMELOG("ModalizeFrame", Modalized);

	if (ActivatedFrame) // Issue #26 (the 1-st problem)
	{
		ActivateCommit(ActivatedFrame);
		ActivatedFrame=nullptr;
	}

	CurrentFrame->PushFrame(Modalized);
}

void Manager::UnmodalizeFrame(Frame *Unmodalized)
{
	_FRAMELOG("UnmodalizeFrame", Unmodalized);

	for (auto iFrame: FrameList)
	{
		if (iFrame->RemoveModal(Unmodalized))
		{
			break;
		}
	}

	for (auto iFrame: ModalStack)
	{
		if (iFrame->RemoveModal(Unmodalized))
		{
			break;
		}
	}
}

void Manager::ExecuteNonModal()
{
	Frame *NonModal=InsertedFrame?InsertedFrame:(ExecutedFrame?ExecutedFrame:ActivatedFrame);
	_FRAMELOG("ExecuteNonModal", NonModal);

	if (!NonModal)
	{
		return;
	}

	if (InList(NonModal))
	{
		ActivateFrame(NonModal);
	}
	else
	{
		ExecutedFrame=nullptr;
		InsertCommit(NonModal);
		InsertedFrame=nullptr;
	}

	for (;;)
	{
		Commit();

		if (CurrentFrame!=NonModal || EndLoop)
		{
			break;
		}

		ProcessMainLoop();
	}
}

void Manager::ExecuteModal(Frame *Executed)
{
	_FRAMELOG("ExecuteModal", Executed);

	if (!Executed == !ExecutedFrame)
	{
		return;
	}

	if (Executed)
	{
		ExecutedFrame=Executed;
	}

	auto ModalStartLevel=ModalStack.size();
	bool OriginalStartManager=StartManager;
	StartManager=true;

	for (;;)
	{
		Commit();

		if (ModalStack.size()<=ModalStartLevel)
		{
			break;
		}

		ProcessMainLoop();
	}

	StartManager=OriginalStartManager;
}

/* $ 15.05.2002 SKV
  Так как нужно это в разных местах,
  а глобальные счётчики не концептуально,
  то лучше это делать тут.
*/
void Manager::ExecuteModalEV()
{
	ModalEVCount++;
	ExecuteModal();
	ModalEVCount--;
}

/* $ 11.10.2001 IS
   Подсчитать количество фреймов с указанным именем.
*/
int Manager::CountFramesWithName(const wchar_t *Name, bool IgnoreCase) const
{
	int Counter=0;
	typedef int (__cdecl *cmpfunc_t)(const wchar_t *s1, const wchar_t *s2);
	cmpfunc_t cmpfunc=IgnoreCase ? StrCmpI : StrCmp;
	FARString strType, strCurName;

	for (auto iFrame: FrameList)
	{
		iFrame->GetTypeAndName(strType, strCurName);

		if (!cmpfunc(Name, strCurName)) ++Counter;
	}

	return Counter;
}

/*!
  \return Возвращает nullptr если нажат "отказ" или если нажат текущий фрейм.
  Другими словами, если немодальный фрейм не поменялся.
  Если же фрейм поменялся, то тогда функция должна возвратить
  указатель на предыдущий фрейм.
*/
Frame *Manager::FrameMenu()
{
	/* $ 28.04.2002 KM
	    Флаг для определения того, что меню переключения
	    экранов уже активировано.
	*/
	static bool AlreadyShown=false;

	if (AlreadyShown)
		return nullptr;

	int ExitCode;
	bool CheckCanLoseFocus=CurrentFrame->GetCanLoseFocus();
	{
		MenuItemEx ModalMenuItem;
		VMenu ModalMenu(Msg::ScreensTitle,nullptr,0,ScrY-4);
		ModalMenu.SetHelp(L"ScrSwitch");
		ModalMenu.SetFlags(VMENU_WRAPMODE);
		ModalMenu.SetPosition(-1,-1,0,0);
		ModalMenu.SetId(ScreensSwitchId);

		if (!CheckCanLoseFocus)
			ModalMenuItem.SetDisable(TRUE);

		for (int I=0; I<(int)FrameList.size(); I++)
		{
			FARString strType, strName, strNumText;
			FrameList[I]->GetTypeAndName(strType, strName);
			ModalMenuItem.Clear();

			if (I<10)
				strNumText.Format(L"&%d. ",I);
			else if (I<36)
				strNumText.Format(L"&%lc. ",I+55);  // 55='A'-10
			else
				strNumText = L"&   ";

			//TruncPathStr(strName,ScrX-24);
			ReplaceStrings(strName,L"&",L"&&",-1);
			/*  добавляется "*" если файл изменен */
			ModalMenuItem.strName.Format(L"%ls%-10.10ls %lc %ls", strNumText.CPtr(), strType.CPtr(),
				(FrameList[I]->IsFileModified()?L'*':L' '), strName.CPtr());
			ModalMenuItem.SetSelect(I==FramePos);
			ModalMenu.AddItem(&ModalMenuItem);
		}

		AlreadyShown=true;
		ModalMenu.Process();
		AlreadyShown=false;
		ExitCode=ModalMenu.Modal::GetExitCode();
	}

	if (CheckCanLoseFocus)
	{
		if (ExitCode>=0)
		{
			ActivateFrame(ExitCode);
			return (ActivatedFrame==CurrentFrame || !CurrentFrame->GetCanLoseFocus()?nullptr:CurrentFrame);
		}

		return (ActivatedFrame==CurrentFrame?nullptr:CurrentFrame);
	}

	return nullptr;
}

int Manager::GetFrameCountByType(int Type) const
{
	int ret=0;

	for (auto iFrame: FrameList)
	{
		/* $ 10.05.2001 DJ
		   не учитываем фрейм, который собираемся удалять
		*/
		if (iFrame == DeletedFrame || (unsigned int)iFrame->GetExitCode() == XC_QUIT)
			continue;

		if (iFrame->GetType()==Type)
			ret++;
	}

	return ret;
}

/*$ 11.05.2001 OT Теперь можно искать файл не только по полному имени, но и отдельно - путь, отдельно имя */
Frame* Manager::FindFrameByFile(int ModalType,const wchar_t *FileName, const wchar_t *Dir) const
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
		if (iFrame->GetType()==ModalType)
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

void Manager::ActivateFrame(Frame *Activated)
{
	_FRAMELOG("ActivateFrame", Activated);

	if (!ActivatedFrame && (InList(Activated) || InStack(Activated)))
	{
		ActivatedFrame = Activated;
	}
}

void Manager::ActivateFrame(int Index)
{
	ActivateFrame((*this)[Index]);
}

void Manager::DeactivateFrame(Frame *Deactivated,int Direction)
{
	_FRAMELOG("DeactivateFrame", Deactivated);

	if (Direction)
	{
		FramePos+=Direction;

		if (FramePos>=(int)FrameList.size())
		{
			FramePos=0;
		}
		else if (FramePos<0)
		{
			FramePos=(int)FrameList.size()-1;
		}

		ActivateFrame(FramePos);
	}

	DeactivatedFrame=Deactivated;
}

void Manager::RefreshFrame(Frame *Refreshed)
{
	_FRAMELOG("RefreshFrame", Refreshed);

	if (ActivatedFrame)
		return;

	if (Refreshed)
	{
		RefreshedFrame=Refreshed;
	}
	else
	{
		RefreshedFrame=CurrentFrame;
	}

	if (InList(Refreshed) || InStack(Refreshed))
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
		if (RefreshedFrame && RefreshedFrame->NextModal)
			Commit();
	}
}

void Manager::RefreshFrame(int Index)
{
	RefreshFrame((*this)[Index]);
}

void Manager::ExecuteFrame(Frame *Executed)
{
	_FRAMELOG("ExecuteFrame", Executed);

	ExecutedFrame=Executed;
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
	WaitInFastFind=0;
	StartManager=true;

	for (;;)
	{
		Commit();

		if (EndLoop || !HaveAnyFrame())
		{
			break;
		}

		ProcessMainLoop();
	}
}

void Manager::SetLastInputRecord(const INPUT_RECORD *Rec)
{
	if (&LastInputRecord != Rec)
		LastInputRecord=*Rec;
}


void Manager::ProcessMainLoop()
{
	if ( CurrentFrame )
		CtrlObject->Macro.SetArea(CurrentFrame->GetMacroArea());

	DispatchInterThreadCalls();

	if ( CurrentFrame && !CurrentFrame->ProcessEvents() )
	{
		ProcessKey(KEY_IDLE);
	}
	else
	{
		// Mantis#0000073: Не работает автоскролинг в QView
		WaitInMainLoop=IsPanelsActive() && ((FilePanels*)CurrentFrame)->ActivePanel->GetType()!=QVIEW_PANEL;
		//WaitInFastFind++;
		FarKey Key = GetInputRecord(&LastInputRecord);
		//WaitInFastFind--;
		WaitInMainLoop=FALSE;

		if (EndLoop)
			return;

		if (LastInputRecord.EventType==MOUSE_EVENT)
		{
				// используем копию структуры, т.к. LastInputRecord может внезапно измениться во время выполнения ProcessMouse
				MOUSE_EVENT_RECORD mer=LastInputRecord.Event.MouseEvent;
				ProcessMouse(&mer);
		}
		else
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
		CloseFAR=FALSE;
		CloseFARMenu=TRUE;
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

			if (!cp || (!cp->LeftPanel->ProcessPluginEvent(FE_CLOSE,nullptr) &&
			            !cp->RightPanel->ProcessPluginEvent(FE_CLOSE,nullptr)))
			{
				EndLoop=true;
			}
		}
		else
		{
			CloseFARMenu=FALSE;
		}
	}
}

#if defined(FAR_ALPHA_VERSION)
#include <float.h>
#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4717)
//#ifdef __cplusplus
//#if defined(_MSC_VER < 1500) // TODO: See REMINDER file, section intrin.h
#ifndef _M_IA64
extern "C" void __ud2();
#else
extern "C" void __setReg(int, uint64_t);
#endif
//#endif                       // TODO: See REMINDER file, section intrin.h
//#endif
#endif
static void Test_EXCEPTION_STACK_OVERFLOW(char* target)
{
	char Buffer[1024]; /* чтобы быстрее рвануло */
	strcpy(Buffer, "zzzz");
	Test_EXCEPTION_STACK_OVERFLOW(Buffer);
}
#if defined(_MSC_VER)
#pragma warning( pop )
#endif
#endif


int Manager::ProcessKey(FarKey Key)
{
	int ret=FALSE;

	if (CurrentFrame)
	{
		DWORD KeyM=(Key&(~KEY_CTRLMASK));

		if (!((KeyM >= KEY_MACRO_BASE && KeyM <= KEY_MACRO_ENDBASE) || (KeyM >= KEY_OP_BASE && KeyM <= KEY_OP_ENDBASE))) // пропустим макро-коды
		{
			switch (CurrentFrame->GetType())
			{
				case MODALTYPE_PANELS:
				{
					_ALGO(CleverSysLog clv(L"Manager::ProcessKey()"));
					_ALGO(SysLog(L"Key=%ls",_FARKEY_ToName(Key)));

					if (CtrlObject->Cp()->ActivePanel->SendKeyToPlugin(Key,TRUE))
						return TRUE;

					break;
				}
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
				case MODALTYPE_VMENU:
				case MODALTYPE_HELP:
				case MODALTYPE_COMBOBOX:
				case MODALTYPE_USER:
				case MODALTYPE_FINDFOLDER:
				default:
					break;
			}
		}

		/*** БЛОК ПРИВИЛЕГИРОВАННЫХ КЛАВИШ ! ***/

		/***   КОТОРЫЕ НЕЛЬЗЯ НАМАКРОСИТЬ    ***/
		switch (Key)
		{
			case KEY_ALT|KEY_NUMPAD0:
			case KEY_ALTINS:
			{
				RunGraber();
				return TRUE;
			}
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
					FrameManager->RefreshFrame();
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
						int PScrX=ScrX;
						int PScrY=ScrY;
						WINPORT(Sleep)(10);
						GetVideoMode(CurSize);

						if (PScrX+1 == CurSize.X && PScrY+1 == CurSize.Y)
						{
							return TRUE;
						}
						else
						{
							PrevScrX=PScrX;
							PrevScrY=PScrY;
							WINPORT(Sleep)(10);
							return ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
						}
					}

					return TRUE;
				}
				case KEY_F12:
				{
					auto CurFrame=FrameManager->GetCurrentFrame();
					int TypeFrame=CurFrame->GetType();

					if ((TypeFrame != MODALTYPE_HELP && TypeFrame != MODALTYPE_DIALOG) || CurFrame->GetCanLoseFocus())
					{
						DeactivateFrame(FrameMenu(),0);
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
							int isPanelFocus=CurrentFrame->GetType() == MODALTYPE_PANELS;

							if (isPanelFocus)
							{
								int LeftVisible=CtrlObject->Cp()->LeftPanel->IsVisible();
								int RightVisible=CtrlObject->Cp()->RightPanel->IsVisible();
								int CmdLineVisible=CtrlObject->CmdLine->IsVisible();
								int KeyBarVisible=CtrlObject->Cp()->MainKeyBar.IsVisible();
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
								WaitKey(Key==KEY_CTRLALTSHIFTPRESS?KEY_CTRLALTSHIFTRELEASE:KEY_RCTRLALTSHIFTRELEASE);
							}

							FrameManager->RefreshFrame();
						}

						return TRUE;
					}

					break;
				}
				case KEY_CTRLTAB:
				case KEY_CTRLSHIFTTAB:

					if (CurrentFrame->GetCanLoseFocus())
					{
						DeactivateFrame(CurrentFrame,Key==KEY_CTRLTAB?1:-1);
					}

					return TRUE;
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
	int ret=FALSE;

//    _D(SysLog(1,"Manager::ProcessMouse()"));
	if (CurrentFrame)
		ret=CurrentFrame->ProcessMouse(MouseEvent);

//    _D(SysLog(L"Manager::ProcessMouse() ret=%i",ret));
	return ret;
}

void Manager::PluginsMenu()
{
	int curType = CurrentFrame->GetType();

	if (curType == MODALTYPE_PANELS || curType == MODALTYPE_EDITOR || curType == MODALTYPE_VIEWER || curType == MODALTYPE_DIALOG)
	{
		/* 02.01.2002 IS
		   ! Вывод правильной помощи по Shift-F1 в меню плагинов в редакторе/вьюере/диалоге
		   ! Если на панели QVIEW или INFO открыт файл, то считаем, что это
		     полноценный вьюер и запускаем с соответствующим параметром плагины
		*/
		if (curType==MODALTYPE_PANELS)
		{
			int pType=CtrlObject->Cp()->ActivePanel->GetType();

			if (pType==QVIEW_PANEL || pType==INFO_PANEL)
			{
				FARString strType, strCurFileName;
				CtrlObject->Cp()->GetTypeAndName(strType, strCurFileName);

				if (!strCurFileName.IsEmpty())
				{
					DWORD Attr=apiGetFileAttributes(strCurFileName);

					// интересуют только обычные файлы
					if (Attr!=INVALID_FILE_ATTRIBUTES && !(Attr&FILE_ATTRIBUTE_DIRECTORY))
						curType=MODALTYPE_VIEWER;
				}
			}
		}

		// в редакторе, вьюере или диалоге покажем свою помощь по Shift-F1
		const wchar_t *Topic=curType==MODALTYPE_EDITOR?L"Editor":
		                     curType==MODALTYPE_VIEWER?L"Viewer":
		                     curType==MODALTYPE_DIALOG?L"Dialog":nullptr;
		CtrlObject->Plugins.CommandsMenu(curType,0,Topic);
	}
}

bool Manager::IsPanelsActive() const
{
	return FramePos>=0 && CurrentFrame && CurrentFrame->GetType() == MODALTYPE_PANELS;
}

Frame *Manager::operator[](int Index) const
{
	return Index>=0 && Index<(int)FrameList.size() ? FrameList[Index] : nullptr;
}

Frame *Manager::GetModalByIndex(int Index) const
{
	return Index>=0 && Index<(int)ModalStack.size() ? ModalStack[Index] : nullptr;
}

int Manager::IndexOfStack(Frame *Frame) const
{
	for (int i=0; i<(int)ModalStack.size(); i++)
	{
		if (Frame==ModalStack[i])
			return i;
	}
	return -1;
}

int Manager::IndexOfList(Frame *Frame) const
{
	for (int i=0; i<(int)FrameList.size(); i++)
	{
		if (Frame==FrameList[i])
			return i;
	}
	return -1;
}

void Manager::Commit(int Count)
{
	Frame *tmp, *tmp2, *tmp3;
	while(true)
	{
		_BASICLOG("Commit");

		if (DeletedFrame && (InsertedFrame||ExecutedFrame))
		{
			tmp = DeletedFrame;
			tmp2 = InsertedFrame;
			tmp3 = ExecutedFrame;
			DeletedFrame = InsertedFrame = ExecutedFrame = nullptr;
			UpdateCommit(tmp,tmp2,tmp3);
		}
		else if (ExecutedFrame)
		{
			tmp = ExecutedFrame;
			ExecutedFrame = nullptr;
			ExecuteCommit(tmp);
		}
		else if (DeletedFrame)
		{
			tmp = DeletedFrame;
			DeletedFrame = nullptr;
			DeleteCommit(tmp);
		}
		else if (InsertedFrame)
		{
			tmp = InsertedFrame;
			InsertedFrame = nullptr;
			InsertCommit(tmp);
		}
		else if (DeactivatedFrame)
		{
			// $ 18.04.2002 skv
			// Если нечего активировать, то в общем-то не надо и деактивировать.
			if (ActivatedFrame)
			{
				tmp = DeactivatedFrame;
				tmp2 = ActivatedFrame;
				DeactivatedFrame = ActivatedFrame = nullptr;
				DeactivateCommit(tmp, tmp2);
				ActivateCommit(tmp2);
			}
			else
				DeactivatedFrame = nullptr;
		}
		else if (ActivatedFrame)
		{
			tmp = ActivatedFrame;
			ActivatedFrame = nullptr;
			ActivateCommit(tmp);
		}
		else if (RefreshedFrame)
		{
			tmp = RefreshedFrame;
			RefreshedFrame = nullptr;
			RefreshCommit(tmp);
		}
		else
		{
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

	aDeactivated->OnChangeFocus(0);

	if (!ModalStack.empty() && aDeactivated==ModalStack.back())
	{
		if (InStack(aActivated))
		{
			ModalStack.pop_back();
		}
		else
		{
			ModalStack.back()=aActivated;
		}
	}
}

void Manager::ActivateCommit(Frame *aFrame)
{
	_FRAMELOG("ActivateCommit", aFrame);

	if (CurrentFrame==aFrame)
	{
		RefreshedFrame=aFrame;
		return;
	}

	int Index=IndexOfList(aFrame);
	if (Index!=-1)
	{
		FramePos=Index;
	}

	/* 14.05.2002 SKV
	  Если мы пытаемся активировать полумодальный фрэйм,
	  то надо его вытащить на верх стэка модалов.
	*/
	Index=IndexOfStack(aFrame);
	if (Index!=-1)
	{
		ModalStack.erase(ModalStack.begin()+Index);
		ModalStack.push_back(aFrame);
	}

	RefreshedFrame=CurrentFrame=aFrame;
}

void Manager::UpdateCommit(Frame *aDeleted, Frame *aInserted, Frame *aExecuted)
{
	_BASICLOG("UpdateCommit: DeletedFrame=%p, InsertedFrame=%p, ExecutedFrame=%p", aDeleted,aInserted,aExecuted);

	if (aExecuted)
	{
		DeleteCommit(aDeleted);
		ExecuteCommit(aExecuted);
	}
	else if (aInserted)
	{
		int Index=IndexOfList(aDeleted);
		if (-1!=Index)
		{
			FrameList[Index]=aInserted;
			ActivateFrame(aInserted);
			ActivatedFrame->FrameToBack=CurrentFrame;
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

	/* $ 14.05.2002 SKV
	  Надёжнее найти и удалить именно то, что
	  нужно, а не просто верхний.
	*/
	int Index=IndexOfStack(aFrame);
	if (Index!=-1)
	{
		ModalStack.erase(ModalStack.begin()+Index);
		if (!ModalStack.empty())
		{
			ActivateFrame(ModalStack.back());
		}
	}

	for (auto iFrame: FrameList)
	{
		if (iFrame->FrameToBack==aFrame)
		{
			iFrame->FrameToBack=CtrlObject->Cp();
		}
	}

	Index=IndexOfList(aFrame);
	if (Index!=-1)
	{
		_DUMP_FRAME_LIST();

		aFrame->DestroyAllModal();

		FrameList.erase(FrameList.begin()+Index);

		if (FramePos >= (int)FrameList.size())
		{
			FramePos=0;
		}

		if (aFrame->FrameToBack==CtrlObject->Cp())
		{
			_BASICLOG("== ActivateFrame(FrameList[FramePos])");
			ActivateFrame(FrameList[FramePos]);
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
		if (CurrentFrame==aFrame)
			CurrentFrame=nullptr;

		/* $ 14.05.2002 SKV
		  Так как в деструкторе фрэйма неявно может быть
		  вызван commit, то надо подстраховаться.
		*/
		DeletedFrame=nullptr;
		delete aFrame;
	}

	// Полагаемся на то, что в ActivateFrame не будет переписан уже
	// присвоенный  ActivatedFrame
	if (!ModalStack.empty())
	{
		ActivateFrame(ModalStack.back());
	}
	else
	{
		ActivateFrame(FramePos);
	}
}

void Manager::InsertCommit(Frame *aFrame)
{
	_FRAMELOG("InsertCommit", aFrame);

	aFrame->FrameToBack=CurrentFrame;
	FrameList.push_back(aFrame);

	if (!ActivatedFrame)
	{
		ActivatedFrame=aFrame;
	}
}

void Manager::RefreshCommit(Frame *aFrame)
{
	_FRAMELOG("RefreshCommit", aFrame);

	if (!InList(aFrame) && !InStack(aFrame))
		return;

	if (!aFrame->Locked())
	{
		if (!IsRedrawFramesInProcess)
			aFrame->ShowConsoleTitle();

		aFrame->Refresh();

		CtrlObject->Macro.SetArea(aFrame->GetMacroArea());
	}

	bool bShowTime = (aFrame->GetType() == MODALTYPE_EDITOR || aFrame->GetType() == MODALTYPE_VIEWER) ?
		Opt.ViewerEditorClock : (WaitInMainLoop && Opt.Clock);

	if (bShowTime)
		ShowTime(1);
}

void Manager::ExecuteCommit(Frame *aFrame)
{
	_FRAMELOG("ExecuteCommit", aFrame);

	ModalStack.push_back(aFrame);
	ActivatedFrame=aFrame;
}

/* $ Введена для нужд CtrlAltShift OT */
void Manager::ImmediateHide()
{
	_BASICLOG("ImmediateHide");
	if (FramePos<0)
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
		if (type==MODALTYPE_EDITOR || type==MODALTYPE_VIEWER)
		{
			if (CtrlObject->CmdLine)
				CtrlObject->CmdLine->ShowBackground();
		}
		else
		{
			int UnlockCount=0;
			IsRedrawFramesInProcess++;

			while ((*this)[FramePos]->Locked())
			{
				(*this)[FramePos]->Unlock();
				UnlockCount++;
			}

			RefreshFrame((*this)[FramePos]);
			Commit();

			for (int i=0; i<UnlockCount; i++)
			{
				(*this)[FramePos]->Lock();
			}

			if (ModalStack.size()>1)
			{
				for (int i=0; i<(int)ModalStack.size()-1; i++)
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
	Frame *iModal=ModalFrame->NextModal;

	while (iModal)
	{
		iModal->ResizeConsole();
		iModal=iModal->NextModal;
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
	Frame *f=CurrentFrame;
	if (f)
	{
		while (f->NextModal)
			f=f->NextModal;
	}
	return f;
}

LockBottomFrame::LockBottomFrame()
	: _frame(FrameManager ? FrameManager->GetBottomFrame() : nullptr)
{
	if (_frame)
	{
		if (_frame->Locked())
			_frame = nullptr;
		else
			_frame->Lock();
	}
}

LockBottomFrame::~LockBottomFrame()
{
	if (_frame)
		_frame->Unlock();
}
