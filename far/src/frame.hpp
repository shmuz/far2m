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
	MODALTYPE_PANELS,
	MODALTYPE_VIEWER,
	MODALTYPE_EDITOR,
	MODALTYPE_DIALOG,
	MODALTYPE_VMENU,
	MODALTYPE_HELP,
	MODALTYPE_COMBOBOX,
	MODALTYPE_FINDFOLDER,
};

class Frame: public ScreenObject
{
	friend class Manager;
private:
	Frame *FrameToBack;
	Frame *NextModal;
	bool RegularIdle = false;
	FARMACROAREA MacroArea;

protected:
	bool DynamicallyBorn;
	bool CanLoseFocus;
	int  ExitCode;
	int  KeyBarVisible;
	int  TitleBarVisible;
	KeyBar *FrameKeyBar;

public:
	Frame();
	~Frame() override;

	virtual int FastHide();
	virtual bool GetCanLoseFocus(bool DynamicMode=false) { return CanLoseFocus; }
	virtual FARMACROAREA GetMacroArea() { return MacroArea; }
	virtual FARString &GetTitle(FARString &Title, int SubLen=-1, int TruncSize=0) { return Title; }
	virtual int GetType() const { return MODALTYPE_VIRTUAL; }
	virtual int GetTypeAndName(FARString &strType, FARString &strName) { return MODALTYPE_VIRTUAL; }
	virtual const wchar_t *GetTypeName() const { return L"[FarModal]"; }
	virtual void InitKeyBar() {}
	virtual bool IsFileModified() const { return false; }
	virtual void OnChangeFocus(bool focus); // вызывается при смене фокуса
	virtual void OnCreate() {};  // вызывается перед созданием окна
	virtual void OnDestroy();  // вызывается перед уничтожением окна
	virtual bool ProcessEvents() const { return true; }
	virtual void RedrawKeyBar() { Frame::UpdateKeyBar(); }
	virtual void Refresh() { OnChangeFocus(true); }  // Просто перерисоваться :)
	virtual void ResizeConsole();
	virtual void SetExitCode(int Code) { ExitCode = Code; }

	void DestroyAllModal();
	bool GetDynamicallyBorn() const { return DynamicallyBorn; }
	int  GetExitCode() const { return ExitCode; }
	bool HasSaveScreen();
	int  IsTitleBarVisible() const {return TitleBarVisible;}
	bool IsTopFrame();
	void PushFrame(Frame* Modalized);
	bool RemoveModal(Frame *aFrame);
	void SetCanLoseFocus(bool Mode) { CanLoseFocus = Mode; }
	void SetDynamicallyBorn(bool Born) {DynamicallyBorn = Born;}
	void SetKeyBar(KeyBar *FrameKeyBar);
	void SetMacroArea(FARMACROAREA Area) { MacroArea = Area; }
	void SetRegularIdle(bool enabled);
	void UpdateKeyBar();
};
