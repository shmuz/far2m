#pragma once

/*
fileview.hpp

Просмотр файла - надстройка над viewer.cpp
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
#include "viewer.hpp"
#include "keybar.hpp"

struct FileViewerParams {
	const wchar_t *Name;
	FileHolderPtr FHP {};
	bool EnableSwitch;
	bool DisableHistory;
	bool DisableEdit;
	long ViewStartPos = -1;
	const wchar_t *PluginData;
	NamesList *ViewNamesList;
	bool ToSaveAs;
	UINT CodePage = CP_AUTODETECT;
};

class FileViewer : public Frame
{
private:
	void Show() override;
	void DisplayObject() override;
	SudoClientRegion _sdc_rgn;
	Viewer View;
	bool RedrawTitle;
	KeyBar ViewKeyBar;
	bool AutoClose;
	bool F3KeyOnly;
	bool FullScreen;
	bool DisableEdit;
	bool DisableHistory;

	FARString strName;

	/* $ 17.08.2001 KM
	  Добавлено для поиска по AltF7. При редактировании найденного файла из
	  архива для клавиши F2 сделать вызов ShiftF2.
	*/
	bool SaveToSaveAs;
	FARString strPluginData;
	FileHolderPtr UngreppedFH;
	int64_t UngreppedPos{0};

	void GrepFilter();
	void GrepFilterDismiss();

public:
	FileViewer(FileViewerParams &Params);
	FileViewer(const wchar_t *Name, bool EnableSwitch, bool DisableHistory, const wchar_t *Title, int X1,
			int Y1, int X2, int Y2, UINT aCodePage = CP_AUTODETECT);
	~FileViewer() override;

public:
	void Init(const wchar_t *Name, bool EnableSwitch, bool DisableHistory, long ViewStartPos,
			const wchar_t *PluginData, NamesList *ViewNamesList, bool ToSaveAs);
	void InitKeyBar() override;
	int ProcessKey(FarKey Key) override;
	int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent) override;
	int64_t VMProcess(int OpCode, void *vParam = nullptr, int64_t iParam = 0) override;
	void ShowConsoleTitle() override;
	void OnDestroy() override;
	void OnChangeFocus(bool focus) override;

	int GetTypeAndName(FARString &strType, FARString &strName) override;
	const wchar_t *GetTypeName() const override { return L"[FileView]"; }
	int GetType() const override { return MODALTYPE_VIEWER; }

	void SetEnableF6(bool Enable)
	{
		DisableEdit = !Enable;
		InitKeyBar();
	}
	void SetFileHolder(FileHolderPtr Observer) { View.SetFileHolder(Observer); }
	void SetPluginData(const wchar_t *PluginData) { strPluginData = PluginData; }

	/* $ Введена для нужд CtrlAltShift OT */
	int FastHide() override;

	/* $ 17.08.2001 KM
	  Добавлено для поиска по AltF7. При редактировании найденного файла из
	  архива для клавиши F2 сделать вызов ShiftF2.
	*/
	void SetSaveToSaveAs(int ToSaveAs)
	{
		SaveToSaveAs = ToSaveAs;
		InitKeyBar();
	}
	int ViewerControl(int Command, void *Param);
	bool IsFullScreen() { return FullScreen; }
	FARString &GetTitle(FARString &Title, int SubLen = -1, int TruncSize = 0) override;
	int64_t GetViewFileSize() const;
	int64_t GetViewFilePos() const;
	void ShowStatus();
	void SetAutoClose(bool AC) { AutoClose = AC; }
	int GetViewerID() const;
};

void ModalViewFile(const std::string &pathname);
void ViewConsoleHistory(HANDLE con_hnd, bool modal, bool autoclose);
