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
		DeleteFrame(ModalStack[i]);
		DeleteCommit();
		DeletedFrame=nullptr;
	}

	for (int i=(int)FrameList.size()-1; i>=0; i--)
	{
		DeleteFrame(FrameList[i]);
		DeleteCommit();
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
		InsertedFrame=NonModal;
		ExecutedFrame=nullptr;
		InsertCommit();
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

	if (!Executed && !ExecutedFrame)
	{
		return;
	}

	if (Executed)
	{
		if (ExecutedFrame)
		{
			return;
		}
		else
		{
			ExecutedFrame=Executed;
		}
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
	return;
}

/* $ 15.05.2002 SKV
  Так как нужно это в разных местах,
  а глобальные счётчики не концептуально,
  то лучше это делать тут.
*/
void Manager::ExecuteModalEV()
{
	ModalEVCount++;
	ExecuteModal(nullptr);
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
		int Key=GetInputRecord(&LastInputRecord);
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

static bool ConfirmExit()
{
	int r;
	if (WINPORT(ConsoleBackgroundMode)(FALSE)) {
		r = Message(0,3,&FarAskQuitId,Msg::Quit,Msg::AskQuit,Msg::Yes,Msg::No,Msg::Background);
		if (r == 2) {
			WINPORT(ConsoleBackgroundMode)(TRUE);
		}

	} else {
		r = Message(0,2,&FarAskQuitId,Msg::Quit,Msg::AskQuit,Msg::Yes,Msg::No);
	}

	return r == 0;
}

void Manager::ExitMainLoop(bool Ask)
{
	if (CloseFAR)
	{
		CloseFAR=FALSE;
		CloseFARMenu=TRUE;
	};

	if (!Ask || ((!Opt.Confirm.Exit || ConfirmExit()) && CtrlObject->Plugins.MayExitFar()))
	{
		/* $ 29.12.2000 IS
		   + Проверяем, сохранены ли все измененные файлы. Если нет, то не выходим
		     из фара.
		*/
		if (ExitAll())
		{
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


int Manager::ProcessKey(DWORD Key)
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
					SetVideoMode();
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
					int TypeFrame=FrameManager->GetCurrentFrame()->GetType();

					if (TypeFrame != MODALTYPE_HELP && TypeFrame != MODALTYPE_DIALOG)
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

void Manager::Commit()
{
	while(true)
	{
		_BASICLOG("Commit");

		if (DeletedFrame && (InsertedFrame||ExecutedFrame))
		{
			UpdateCommit();
			DeletedFrame = nullptr;
			InsertedFrame = nullptr;
			ExecutedFrame=nullptr;
		}
		else if (ExecutedFrame)
		{
			ExecuteCommit();
			ExecutedFrame=nullptr;
		}
		else if (DeletedFrame)
		{
			DeleteCommit();
			DeletedFrame = nullptr;
		}
		else if (InsertedFrame)
		{
			InsertCommit();
			InsertedFrame = nullptr;
		}
		else if (DeactivatedFrame)
		{
			DeactivateCommit();
			DeactivatedFrame=nullptr;
		}
		else if (ActivatedFrame)
		{
			ActivateCommit();
			ActivatedFrame=nullptr;
		}
		else if (RefreshedFrame)
		{
			RefreshCommit();
			RefreshedFrame=nullptr;
		}
		else
		{
			break;
		}
	}
}

void Manager::DeactivateCommit()
{
	_FRAMELOG("DeactivateCommit", DeactivatedFrame);

	/*$ 18.04.2002 skv
	  Если нечего активировать, то в общем-то не надо и деактивировать.
	*/
	if (DeactivatedFrame && ActivatedFrame)
	{
		DeactivatedFrame->OnChangeFocus(0);

		if (!ModalStack.empty() && DeactivatedFrame==ModalStack.back())
		{
			if (InStack(ActivatedFrame))
			{
				ModalStack.pop_back();
			}
			else
			{
				ModalStack.back()=ActivatedFrame;
			}
		}
	}
}


void Manager::ActivateCommit()
{
	_FRAMELOG("ActivateCommit", ActivatedFrame);

	if (CurrentFrame==ActivatedFrame)
	{
		RefreshedFrame=ActivatedFrame;
		return;
	}

	int FrameIndex=IndexOfList(ActivatedFrame);

	if (-1!=FrameIndex)
	{
		FramePos=FrameIndex;
	}

	/* 14.05.2002 SKV
	  Если мы пытаемся активировать полумодальный фрэйм,
	  то надо его вытащить на верх стэка модалов.
	*/
	FrameIndex=IndexOfStack(ActivatedFrame);
	if (FrameIndex!=-1)
	{
		ModalStack.erase(ModalStack.begin()+FrameIndex);
		ModalStack.push_back(ActivatedFrame);
	}

	RefreshedFrame=CurrentFrame=ActivatedFrame;
}

void Manager::UpdateCommit()
{
	_BASICLOG("UpdateCommit: DeletedFrame=%p, InsertedFrame=%p, ExecutedFrame=%p", DeletedFrame,InsertedFrame, ExecutedFrame);

	if (ExecutedFrame)
	{
		DeleteCommit();
		ExecuteCommit();
		return;
	}

	int FrameIndex=IndexOfList(DeletedFrame);

	if (-1!=FrameIndex)
	{
		FrameList[FrameIndex]=InsertedFrame;
		ActivateFrame(InsertedFrame);
		ActivatedFrame->FrameToBack=CurrentFrame;
		DeleteCommit();
	}
}

//! Удаляет DeletedFrame изо всех очередей!
//! Назначает следующий активный, (исходя из своих представлений)
//! Но только в том случае, если активный фрейм еще не назначен заранее.
void Manager::DeleteCommit()
{
	_FRAMELOG("DeleteCommit", DeletedFrame);

	if (!DeletedFrame)
	{
		return;
	}

	/* $ 14.05.2002 SKV
	  Надёжнее найти и удалить именно то, что
	  нужно, а не просто верхний.
	*/
	for (auto it=ModalStack.begin(); it!=ModalStack.end(); it++)
	{
		if (*it==DeletedFrame)
		{
			ModalStack.erase(it);
			if (!ModalStack.empty())
			{
				ActivateFrame(ModalStack.back());
			}
			break;
		}
	}

	for (auto iFrame: FrameList)
	{
		if (iFrame->FrameToBack==DeletedFrame)
		{
			iFrame->FrameToBack=CtrlObject->Cp();
		}
	}

	int FrameIndex=IndexOfList(DeletedFrame);

	if (-1!=FrameIndex)
	{
		_DUMP_FRAME_LIST();

		DeletedFrame->DestroyAllModal();

		FrameList.erase(FrameList.begin()+FrameIndex);

		if (FramePos >= (int)FrameList.size())
		{
			FramePos=0;
		}

		if (DeletedFrame->FrameToBack==CtrlObject->Cp())
		{
			_BASICLOG("== ActivateFrame(FrameList[FramePos])");
			ActivateFrame(FrameList[FramePos]);
		}
		else
		{
			_BASICLOG("== ActivateFrame(DeletedFrame->FrameToBack)");
			ActivateFrame(DeletedFrame->FrameToBack);
		}
	}

	DeletedFrame->OnDestroy();

	if (DeletedFrame->GetDynamicallyBorn())
	{
		if (CurrentFrame==DeletedFrame)
			CurrentFrame=nullptr;

		/* $ 14.05.2002 SKV
		  Так как в деструкторе фрэйма неявно может быть
		  вызван commit, то надо подстраховаться.
		*/
		Frame *tmp=DeletedFrame;
		DeletedFrame=nullptr;
		delete tmp;
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

void Manager::InsertCommit()
{
	_FRAMELOG("InsertCommit", InsertedFrame);

	if (InsertedFrame)
	{
		InsertedFrame->FrameToBack=CurrentFrame;
		FrameList.push_back(InsertedFrame);

		if (!ActivatedFrame)
		{
			ActivatedFrame=InsertedFrame;
		}
	}
}

void Manager::RefreshCommit()
{
	_FRAMELOG("RefreshCommit", RefreshedFrame);

	if (!RefreshedFrame)
		return;

	if (!InList(RefreshedFrame) && !InStack(RefreshedFrame))
		return;

	if (!RefreshedFrame->Locked())
	{
		if (!IsRedrawFramesInProcess)
			RefreshedFrame->ShowConsoleTitle();

		RefreshedFrame->Refresh();

		CtrlObject->Macro.SetArea(RefreshedFrame->GetMacroArea());
	}

	if ((Opt.ViewerEditorClock &&
	        (RefreshedFrame->GetType() == MODALTYPE_EDITOR ||
	         RefreshedFrame->GetType() == MODALTYPE_VIEWER))
	        || (WaitInMainLoop && Opt.Clock))
		ShowTime(1);
}

void Manager::ExecuteCommit()
{
	_FRAMELOG("ExecuteCommit", ExecutedFrame);

	if (ExecutedFrame)
	{
		ModalStack.push_back(ExecutedFrame);
		ActivatedFrame=ExecutedFrame;
	}
}

/*$ 26.06.2001 SKV
  Для вызова из плагинов посредством ACTL_COMMIT
*/
void Manager::PluginCommit()
{
	_BASICLOG("PluginCommit");
	Commit();
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
