#pragma once

/*
filepanels.hpp

файловые панели
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

#include "frame.hpp"
#include "keybar.hpp"
#include "menubar.hpp"


class Panel;
class CommandLine;

class FilePanels:public Frame
{
	private:
		void DisplayObject() override;

	public:
		Panel *LastLeftFilePanel;
		Panel *LastRightFilePanel;
		Panel *LeftPanel;
		Panel *RightPanel;
		Panel *ActivePanel;

		KeyBar      MainKeyBar;
		MenuBar     TopMenuBar;

		int LastLeftType;
		int LastRightType;
		bool LeftStateBeforeHide;
		bool RightStateBeforeHide;

	public:
		FilePanels();
		~FilePanels() override;

	public:
		void Init();

		Panel* CreatePanel(int Type);
		void   DeletePanel(Panel *Deleted);
		Panel* GetAnotherPanel(Panel *Current);
		Panel* ChangePanelToFilled(Panel *Current,int NewType);
		Panel* ChangePanel(Panel *Current,int NewType,bool CreateNew,bool Force);
		void   SetPanelPositions(bool LeftFullScreen, bool RightFullScreen);
//    void   SetPanelPositions();

		void   SetupKeyBar();

		int ProcessKey(FarKey Key) override;
		int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent) override;
		int64_t VMProcess(int OpCode, void *vParam=nullptr, int64_t iParam=0) override;

		bool SetAnotherPanelFocus();
		bool SwapPanels();
		bool ChangePanelViewMode(Panel *Current, int Mode, bool RefreshFrame);

		void SetScreenPosition() override;

		void Update();

		int GetTypeAndName(FARString &strType, FARString &strName) override;
		int GetType() override { return MODALTYPE_PANELS; }
		const wchar_t *GetTypeName() override {return L"[FilePanels]";}

		void OnChangeFocus(bool focus) override;

		void RedrawKeyBar() override; // virtual
		void ShowConsoleTitle() override;
		void ResizeConsole() override;
		int FastHide() override;
		void Refresh() override;
		void GoToFile(const wchar_t *FileName);

		FARMACROAREA GetMacroArea() override;
};
