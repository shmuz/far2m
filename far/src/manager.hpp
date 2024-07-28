#pragma once

/*
manager.hpp

Переключение между несколькими file panels, viewers, editors
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

class Frame;

class Manager
{
#if defined(SYSLOG)
		friend void ManagerClass_Dump(const wchar_t *Title,const Manager *m,FILE *fp);
#endif
	private:
		std::vector<Frame*> ModalStack; // Стек модальных фреймов
		std::vector<Frame*> FrameList;  // Очередь немодальных фреймов
		int  FramePos;           // Индекс текущий немодального фрейма. Он не всегда совпадает с CurrentFrame
		// текущий немодальный фрейм можно получить с помощью FrameManager->GetBottomFrame();

		/*$ Претенденты на ... */
		Frame *InsertedFrame;   // Фрейм, который будет добавлен в конец немодальной очереди
		Frame *DeletedFrame;    // Фрейм, предназначенный для удаления из модальной очереди, из модального стека, либо одиночный (которого нет ни там, ни там)
		Frame *ActivatedFrame;  // Фрейм, который необходимо активировать после каких нибудь изменений
		Frame *RefreshedFrame;  // Фрейм, который нужно просто освежить, т.е. перерисовать
		Frame *DeactivatedFrame;// Фрейм, который указывает на предыдущий активный фрейм
		Frame *ExecutedFrame;   // Фрейм, которого вскорости нужно будет поставить на вершину модального стека

		Frame *CurrentFrame;     // текущий фрейм. Он может находиться как в немодальной очереди, так и в модальном стеке
		// его можно получить с помощью FrameManager->GetCurrentFrame();

		/* $ 15.05.2002 SKV
		  Так как есть полумодалы, что б не было путаницы,
		  заведём счётчик модальных editor/viewer'ов.
		  Дёргать его  надо ручками перед вызовом ExecuteModal.
		  А автоматом нельзя, так как ExecuteModal вызывается
		  1) не только для настоящих модалов (как это не парадоксально),
		  2) не только для editor/viewer'ов.
		*/
		int ModalEVCount;
		int RegularIdleWanters = 0;

		bool EndLoop;            // Признак выхода из цикла
		bool StartManager;
		INPUT_RECORD LastInputRecord;

	private:
		void StartupMainloop();
		Frame *FrameMenu(); // show window menu (F12)

		// Она в цикле вызывает себя, пока хотябы один из указателей отличен от nullptr
		// Функции, "подмастерья начальника" - Commit'a
		// Иногда вызываются не только из него и из других мест
		void RefreshCommit(Frame *aFrame);
		void DeactivateCommit(Frame *aDeactivated,Frame *aActivated);
		void ActivateCommit(Frame *aFrame);
		void UpdateCommit(Frame *aDeleted,Frame *aInserted,Frame *aExecuted); // выполняется тогда, когда нужно заменить один фрейм на другой
		void InsertCommit(Frame *aFrame);
		void DeleteCommit(Frame *aFrame);
		void ExecuteCommit(Frame *aFrame);

	public:
		Manager();
		~Manager();

	public:
		// Эти функции можно безопасно вызывать практически из любого места кода
		// они как бы накапливают информацию о том, что нужно будет сделать с фреймами при следующем вызове Commit()
		void InsertFrame(Frame *NewFrame);
		void DeleteFrame(Frame *Deleted=nullptr);
		void DeactivateFrame(Frame *Deactivated,int Direction);
		void ActivateFrame(Frame *Activated);
		void ActivateFrame(int Index);
		void RefreshFrame(Frame *Refreshed=nullptr);
		void RefreshFrame(int Index);

		// Функции для запуска модальных фреймов.
		void ExecuteFrame(Frame *Executed);

		// Входит в новый цикл обработки событий
		void ExecuteModal(Frame *Executed=nullptr);
		// Запускает немодальный фрейм в модальном режиме
		void ExecuteNonModal();

		void ExecuteModalEV();

		//  Функции, которые работают с очередью немодального фрейма.
		//  Сейчас используются только для хранения информаци о наличии запущенных объектов типа VFMenu
		void ModalizeFrame(Frame *Modalized);     // Поставить в "очередь" к текущему немодальному фрейму
		void UnmodalizeFrame(Frame *Unmodalized); // Убрать из "очереди" немодального фрейма

		void CloseAll();
		/* $ 29.12.2000 IS
		     Аналог CloseAll, но разрешает продолжение полноценной работы в фаре,
		     если пользователь продолжил редактировать файл.
		     Возвращает true, если все закрыли и можно выходить из фара.
		*/
		bool ExitAll();

		int  GetModalCount() const {return ModalStack.size();}
		int  GetFrameCount() const {return FrameList.size();}
		int  GetFrameCountByType(int Type) const;

		void Commit(int Count=0);  // завершает транзакцию по изменениям в очереди и стеке фреймов

		int CountFramesWithName(const wchar_t *Name, bool IgnoreCase=true) const;

		bool IsPanelsActive() const; // используется как признак WaitInMainLoop
		Frame* FindFrameByFile(int ModalType,const wchar_t *FileName,const wchar_t *Dir=nullptr) const;
		bool ShowBackground();

		void EnterMainLoop();
		void ProcessMainLoop();
		void ExitMainLoop(bool Ask, int ExitCode = 0);
		int ProcessKey(FarKey key);
		int ProcessMouse(MOUSE_EVENT_RECORD *me);

		void PluginsMenu(); // вызываем меню по F11
		void SwitchToPanels();

		INPUT_RECORD *GetLastInputRecord() { return &LastInputRecord; }
		void SetLastInputRecord(const INPUT_RECORD *Rec);

		Frame *GetCurrentFrame() const { return CurrentFrame; }

		Frame *operator[](int Index) const;
		Frame *GetModalByIndex(int Index) const;

		int IndexOfList(Frame *Frame) const;

		int IndexOfStack(Frame *Frame) const;
		bool HaveAnyFrame() const;

		bool InList(Frame *frame) const { return IndexOfList(frame)!=-1; }
		bool InStack(Frame *frame) const { return IndexOfStack(frame)!=-1; }

		void ImmediateHide();
		/* $ 13.04.2002 KM
		  Для вызова ResizeConsole для всех NextModal у
		  модального фрейма.
		*/
		void ResizeAllModal(Frame *ModalFrame);

		Frame *GetBottomFrame() const { return (*this)[FramePos]; }

		bool ManagerIsDown() const {return EndLoop;}
		bool ManagerStarted() const {return StartManager;}

		void InitKeyBar();

		bool InModalEV() const {return ModalEVCount!=0;}

		void ResizeAllFrame();

		// возвращает top-модал или сам фрейм, если у фрейма нету модалов
		Frame* GetTopModal() const;

		void RegularIdleWantersAdd() { RegularIdleWanters++; }
		void RegularIdleWantersRemove() { if (RegularIdleWanters) RegularIdleWanters--; }
		int RegularIdleWantersCount() const { return RegularIdleWanters; }
};

extern Manager *FrameManager;

class LockBottomFrame
{
	Frame *_frame;

public:
	LockBottomFrame();
	~LockBottomFrame();
};
