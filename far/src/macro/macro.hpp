#pragma once

/*
macro.hpp

Макросы
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

#include "farplug-wide.h"

// Macro Const
enum
{
	constMsX          = 0,
	constMsY          = 1,
	constMsButton     = 2,
	constMsCtrlState  = 3,
	constMsEventFlags = 4,
	constMsLastCtrlState = 5,
	constMsLAST       = 6,
};

enum MACRODISABLEONLOAD
{
	MDOL_ALL            = 0x80000000, // дисаблим все макросы при загрузке
	MDOL_AUTOSTART      = 0x00000001, // дисаблим автостартующие макросы
};

struct MacroPanelSelect {
	int     Action;
	int     Mode;
	int64_t Index;
	const wchar_t *Item;
};

struct DlgParam;

class KeyMacro
{
public:
	KeyMacro();

	static bool AddMacro(DWORD PluginId, const MacroAddMacro* Data);
	static bool DelMacro(DWORD PluginId, void* Id);
	static bool ExecuteString(MacroExecuteString *Data);
	static bool GetMacroKeyInfo(const FARString& StrArea, int Pos, FARString &strKeyName, FARString &strDescription);
	static bool IsOutputDisabled();
	static bool IsExecuting() { return GetExecutingState() != MACROSTATE_NOMACRO; }
	static bool IsHistoryDisabled(int TypeHistory);
	static void RunStartMacro();
	static bool SaveMacros();
	static void SetMacroConst(int ConstIndex, long long Value);
	static long long GetMacroConst(int ConstIndex);
	static bool PostNewMacro(const wchar_t* Sequence, DWORD InputFlags, FarKey AKey = 0);
	static bool GetMacroParseError(COORD& ErrPos, FARString& ErrSrc);
	static bool ParseMacroString(const wchar_t* Sequence,DWORD Flags,bool skipFile);

	int64_t CallFar(int CheckCode, const FarMacroCall* Data);
	bool CheckWaitKeyFunc() const;
	FARMACROSTATE GetState() const;
	FarKey PeekKey() const;
	FARMACROAREA GetArea() const { return m_Area; }
	FarKey GetKey();
	bool ProcessKey(FarKey Key, const INPUT_RECORD *Rec=nullptr);
	const wchar_t* GetStringToPrint() const { return m_StringToPrint.CPtr(); }
	bool IsRecording() const { return m_Recording != MACROSTATE_NOMACRO; }
	bool LoadMacros(bool FromFar, bool InitedRAM=true, const FarMacroLoad *Data=nullptr);
	void SetArea(FARMACROAREA Area) { m_Area=Area; }
	void SuspendMacros(bool Suspend) { Suspend ? ++m_InternalInput : --m_InternalInput; }
	bool CanSendKeysToPlugin() const;

private:
	static FARMACROSTATE GetExecutingState();
	static bool GetMacroSettings(FarKey Key, DWORD &Flags, const wchar_t* Src, const wchar_t* Descr);

	bool AssignMacroKey(DlgParam *Param);
	void RestoreMacroChar() const;

	static FARString m_RecCode;
	static FARString m_RecDescription;
	static FARMACROAREA m_StartArea;
	FARMACROAREA m_Area;
	FARMACROSTATE m_Recording;
	int m_InternalInput;
	int m_WaitKey;
	FARString m_StringToPrint;

private:
	static bool CheckEditSelected(FARMACROAREA Area, DWORD CurFlags);
	static bool CheckPanel(int PanelMode,DWORD CurFlags, bool IsPassivePanel);
	static bool CheckCmdLine(int CmdLength,DWORD Flags);
	static bool CheckFileFolder(DWORD CurFlags, bool IsPassivePanel);
	static bool CheckAll(FARMACROAREA Area, DWORD CurFlags);

	static LONG_PTR WINAPI AssignMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
	static LONG_PTR WINAPI ParamMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
};

inline bool IsMenuArea(int Area) { return
	Area==MACROAREA_MAINMENU || Area==MACROAREA_MENU || Area==MACROAREA_DISKS ||
	Area==MACROAREA_USERMENU || Area==MACROAREA_SHELLAUTOCOMPLETION ||
	Area==MACROAREA_DIALOGAUTOCOMPLETION; }

inline bool IsPanelsArea(int Area) { return
	Area==MACROAREA_SHELL || Area==MACROAREA_INFOPANEL || Area==MACROAREA_QVIEWPANEL ||
	Area==MACROAREA_TREEPANEL; }

DWORD GetHistoryDisableMask();
DWORD SetHistoryDisableMask(DWORD Mask);
bool IsTopMacroOutputDisabled();

void ShowUserMenu(size_t Count, const FarMacroValue *Values);

bool StrToGuid(const wchar_t *Str, GUID& Guid);
std::string GuidToString(const GUID& Guid);
