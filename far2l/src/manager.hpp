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
		Frame *DeletedFrame;    // Фрейм, предназначений для удаления из модальной очереди, из модального стека, либо одиночный (которого нет ни там, ни там)
		Frame *ActivatedFrame;  // Фрейм, который необходимо активировать после каких нибудь изменений
		Frame *RefreshedFrame;  // Фрейм, который нужно просто освежить, т.е. перерисовать
		Frame *ModalizedFrame;  // Фрейм, который становится в "очередь" к текущему немодальному фрейму
		Frame *UnmodalizedFrame;// Фрейм, убираюющийся из "очереди" немодального фрейма
		Frame *DeactivatedFrame;// Фрейм, который указывает на предудущий активный фрейм
		Frame *ExecutedFrame;   // Фрейм, которого вскорости нужно будет поставить на вершину модального стека

		Frame *CurrentFrame;     // текущий фрейм. Он может находиться как в немодальной очереди, так и в модальном стеке
		// его можно получить с помощью FrameManager->GetCurrentFrame();

		/* $ 15.05.2002 SKV
		  Так как есть полумодалы, что б не было путаницы,
		  заведём счётчик модальных editor/viewer'ов.
		  Дёргать его  надо ручками перед вызовом ExecuteModal.
		  А автоматом нельзя, так как ExecuteModal вызывается
		  1) не только для настоящих модалов (как это не пародоксально),
		  2) не только для editor/viewer'ов.
		*/
		int ModalEVCount;
		unsigned int RegularIdleWanters = 0;

		bool EndLoop;            // Признак выхода из цикла
		bool StartManager;
		INPUT_RECORD LastInputRecord;

	private:
		void StartupMainloop();
		Frame *FrameMenu(); //    вместо void SelectFrame(); // show window menu (F12)

		void Commit();         // завершает транзакцию по изменениям в очереди и стеке фреймов
		// Она в цикле вызывает себя, пока хотябы один из указателей отличен от nullptr
		// Функции, "подмастерья начальника" - Commit'a
		// Иногда вызываются не только из него и из других мест
		void RefreshCommit();  //
		void DeactivateCommit(); //
		void ActivateCommit(); //
		void UpdateCommit();   // выполняется тогда, когда нужно заменить один фрейм на другой
		void InsertCommit();
		void DeleteCommit();
		void ExecuteCommit();
		void ModalizeCommit();
		void UnmodalizeCommit();

	public:
		Manager();
		~Manager();

	public:
		// Эти функции можно безопасно вызывать практически из любого места кода
		// они как бы накапливают информацию о том, что нужно будет сделать с фреймами при следующем вызове Commit()
		void InsertFrame(Frame *NewFrame, int Index=-1);
		void DeleteFrame(Frame *Deleted=nullptr);
		void DeleteFrame(int Index);
		void DeactivateFrame(Frame *Deactivated,int Direction);
		void ActivateFrame(Frame *Activated);
		void ActivateFrame(int Index);
		void RefreshFrame(Frame *Refreshed=nullptr);
		void RefreshFrame(int Index);

		//! Функции для запуска модальных фреймов.
		void ExecuteFrame(Frame *Executed);


		//! Входит в новый цикл обработки событий
		void ExecuteModal(Frame *Executed=nullptr);
		//! Запускает немодальный фрейм в модальном режиме
		void ExecuteNonModal();

		void ExecuteModalEV();

		//!  Функции, которые работают с очередью немодально фрейма.
		//  Сейчас используются только для хранения информаци о наличии запущенных объектов типа VFMenu
		void ModalizeFrame(Frame *Modalized=nullptr, int Mode=TRUE);
		void UnmodalizeFrame(Frame *Unmodalized);

		void CloseAll();
		/* $ 29.12.2000 IS
		     Аналог CloseAll, но разрешает продолжение полноценной работы в фаре,
		     если пользователь продолжил редактировать файл.
		     Возвращает true, если все закрыли и можно выходить из фара.
		*/
		bool ExitAll();

		int  GetFrameCount() const {return FrameList.size();};
		int  GetFrameCountByType(int Type) const;

		/*$ 26.06.2001 SKV
		Для вызова через ACTL_COMMIT
		*/
		void PluginCommit();

		int CountFramesWithName(const wchar_t *Name, bool IgnoreCase=true) const;

		bool IsPanelsActive() const; // используется как признак WaitInMainLoop
		int  FindFrameByFile(int ModalType,const wchar_t *FileName,const wchar_t *Dir=nullptr) const;
		bool ShowBackground();

		void EnterMainLoop();
		void ProcessMainLoop();
		void ExitMainLoop(bool Ask);
		int ProcessKey(DWORD key);
		int ProcessMouse(MOUSE_EVENT_RECORD *me);

		void PluginsMenu(); // вызываем меню по F11
		void SwitchToPanels();

		INPUT_RECORD *GetLastInputRecord() { return &LastInputRecord; }
		void SetLastInputRecord(const INPUT_RECORD *Rec);

		Frame *GetCurrentFrame() const { return CurrentFrame; }

		Frame *operator[](int Index) const;

		int IndexOfList(Frame *Frame) const;

		int IndexOfStack(Frame *Frame) const;
		bool HaveAnyFrame() const;

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
		unsigned int RegularIdleWantersCount() const { return RegularIdleWanters; };
};

extern Manager *FrameManager;

class LockBottomFrame
{
	Frame *_frame;

public:
	LockBottomFrame();
	~LockBottomFrame();
};
