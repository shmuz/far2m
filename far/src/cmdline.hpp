#pragma once

/*
cmdline.hpp

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

#include "scrobj.hpp"
#include "edit.hpp"
#include <WinCompat.h>
#include "FARString.hpp"

enum
{
	FCMDOBJ_LOCKUPDATEPANEL = 0x00010000,
};

class CommandLine : public ScreenObject
{
private:
	EditControl CmdStr;
	SaveScreen *BackgroundScreen;
	FARString strCurDir;
	FARString strLastCmdStr;
	int LastCmdPartLength;
	int LastKey;
	int PushDirStackSize;

private:
	virtual void DisplayObject();
	int CmdExecute(const wchar_t *CmdLine, bool SeparateWindow, bool DirectRun, bool WaitForIdle = false,
			bool Silent = false, bool RunAs = false);
	void GetPrompt(FARString &strDestStr);
	bool IntChDir(const wchar_t *CmdLine, bool ClosePlugin, bool Silent = false);
	bool ProcessOSCommands(const wchar_t *CmdLine, bool SeparateWindow, bool &PrintCommand);
	void ProcessCompletion(bool possibilities);
	void DrawComboBoxMark(wchar_t MarkChar);
	void CheckForKeyPressAfterCmd(int r);

public:
	CommandLine();
	virtual ~CommandLine();

public:
	virtual int ProcessKey(FarKey Key);
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual int64_t VMProcess(int OpCode, void *vParam = nullptr, int64_t iParam = 0);

	virtual void ResizeConsole();

	std::string GetConsoleLog(HANDLE con_hnd, bool colored);
	const FARString &GetCurDir();
	void SetCurDir(const wchar_t *CurDir);

	void GetString(FARString &strStr) { CmdStr.GetString(strStr); }
	int GetLength() { return CmdStr.GetLength(); }
	void SetString(const wchar_t *Str, bool Redraw = true);
	void InsertString(const wchar_t *Str);

	void ExecString(const wchar_t *Str, bool SeparateWindow = false, bool DirectRun = false,
			bool WaitForIdle = false, bool Silent = false, bool RunAs = false);

	void SwitchToBackgroundTerminal(size_t vt_index);

	void ShowViewEditHistory();

	void SetCurPos(int Pos, int LeftPos = 0);
	int GetCurPos() { return CmdStr.GetCurPos(); }
	int GetLeftPos() { return CmdStr.GetLeftPos(); }

	void SetPersistentBlocks(int Mode);
	void SetDelRemovesBlocks(int Mode);
	void SetAutoComplete(int Mode);
	void SetWaitKeypress(int Mode);

	void GetSelString(FARString &strStr) { CmdStr.GetSelString(strStr); }
	void GetSelection(int &Start, int &End) { CmdStr.GetSelection(Start, End); }
	void Select(int Start, int End) { CmdStr.Select(Start, End); }

	void SaveBackground(int X1, int Y1, int X2, int Y2);
	void SaveBackground();
	void ShowBackground();
	void CorrectRealScreenCoord();
	void LockUpdatePanel(int Mode) { Flags.Change(FCMDOBJ_LOCKUPDATEPANEL, Mode); }

	void EnableAC() { return CmdStr.EnableAC(); }
	void DisableAC() { return CmdStr.DisableAC(); }
	void RevertAC() { return CmdStr.RevertAC(); }

	void RedrawWithoutComboBoxMark();

	const CHAR_INFO *GetBackgroundScreen(int &W, int &H);
};
