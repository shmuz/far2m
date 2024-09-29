#pragma once

/*
frame.hpp

Немодальное окно (базовый класс для FilePanels, FileEditor, FileViewer)
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

#include "scrobj.hpp"
#include "FARString.hpp"


class KeyBar;

enum
{
	MODALTYPE_VIRTUAL,
	MODALTYPE_PANELS=1,
	MODALTYPE_VIEWER,
	MODALTYPE_EDITOR,
	MODALTYPE_DIALOG,
	MODALTYPE_VMENU,
	MODALTYPE_HELP,
	MODALTYPE_COMBOBOX,
	MODALTYPE_FINDFOLDER,
	MODALTYPE_USER,
};

class Frame: public ScreenObject
{
		friend class Manager;
	private:
		Frame *FrameToBack;
		Frame *NextModal;
		bool RegularIdle = false;

	protected:
		bool DynamicallyBorn;
		bool CanLoseFocus;
		int  ExitCode;
		int  KeyBarVisible;
		int  TitleBarVisible;
		KeyBar *FrameKeyBar;
		FARMACROAREA MacroArea;

	public:
		Frame();
		virtual ~Frame();

		virtual bool GetCanLoseFocus(bool DynamicMode=false) { return(CanLoseFocus); }
		void SetCanLoseFocus(bool Mode) { CanLoseFocus=Mode; }
		void SetRegularIdle(bool enabled);
		int  GetExitCode() const { return ExitCode; }
		virtual void SetExitCode(int Code) { ExitCode=Code; }

		virtual bool IsFileModified() const { return false; }

		virtual const wchar_t *GetTypeName() {return L"[FarModal]";}
		virtual int GetTypeAndName(FARString &strType, FARString &strName) {return(MODALTYPE_VIRTUAL);}
		virtual int GetType() { return MODALTYPE_VIRTUAL; }

		virtual void OnDestroy();  // вызывается перед уничтожением окна
		virtual void OnCreate() {};   // вызывается перед созданием окна
		virtual void OnChangeFocus(int focus); // вызывается при смене фокуса
		virtual void Refresh() {OnChangeFocus(1);}  // Просто перерисоваться :)

		virtual void InitKeyBar() {}
		void SetKeyBar(KeyBar *FrameKeyBar);
		void UpdateKeyBar();
		virtual void RedrawKeyBar() { Frame::UpdateKeyBar(); }

		int IsTitleBarVisible() const {return TitleBarVisible;}

		int IsTopFrame();
		virtual FARMACROAREA GetMacroArea() { return MacroArea; }
		void PushFrame(Frame* Modalized);
		void DestroyAllModal();
		void SetDynamicallyBorn(bool Born) {DynamicallyBorn=Born;}
		bool GetDynamicallyBorn() const {return DynamicallyBorn;}
		virtual int FastHide();
		bool RemoveModal(Frame *aFrame);
		virtual void ResizeConsole();
		bool HasSaveScreen();
		virtual FARString &GetTitle(FARString &Title,int SubLen=-1,int TruncSize=0) { return Title; }
		virtual bool ProcessEvents() {return true;}
};
