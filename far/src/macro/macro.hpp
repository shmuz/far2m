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

enum MACROMOUSEINDEX
{
	constMsX,
	constMsY,
	constMsButton,
	constMsCtrlState,
	constMsEventFlags,
	constMsLastCtrlState,
	constMsLAST
};

enum MACRODISABLEONLOAD
{
	MDOL_ALL       = 0x00000001, // дисаблим все макросы при загрузке
	MDOL_AUTOSTART = 0x00000002, // дисаблим автостартующие макросы
};

struct MacroPanelSelect {
	int     Action;
	int     Mode;
	int64_t Index;
	const wchar_t *Item;
};

class KeyMacro
{
public:
	KeyMacro();

	static bool    AddMacro(DWORD PluginId, const MacroAddMacro* Data);
	static bool    DelMacro(DWORD PluginId, void* Id);
	static bool    ExecuteString(MacroExecuteString *Data);
	static int64_t GetMacroConst(MACROMOUSEINDEX ConstIndex);
	static bool    GetMacroKeyInfo(const FARString& Area, int Pos, FARString &KeyName, FARString &Description);
	static bool    GetMacroParseError(COORD& ErrPos, FARString& ErrSrc);
	static bool    IsExecuting() { return GetExecutingState() != MACROSTATE_NOMACRO; }
	static bool    IsHistoryDisabled(int TypeHistory);
	static bool    IsOutputDisabled();
	static bool    ParseMacroString(const wchar_t* Sequence,DWORD Flags,bool skipFile);
	static bool    PostNewMacro(const wchar_t* Sequence, DWORD InputFlags, FarKey AKey = 0);
	static void    RunStartMacro();
	static bool    SaveMacros();
	static void    SetMacroConst(MACROMOUSEINDEX ConstIndex, int64_t Value);

	void           CallFar(int CheckCode, const FarMacroCall* Data);
	bool           CanSendKeysToPlugin() const;
	bool           CheckWaitKeyFunc() const;
	FARMACROAREA   GetArea() const { return m_Area; }
	FarKey         GetKey();
	FARMACROSTATE  GetState() const;
	const wchar_t* GetStringToPrint() const { return m_StringToPrint.CPtr(); }
	bool           IsRecording() const { return m_Recording != MACROSTATE_NOMACRO; }
	bool           LoadMacros(bool FromFar, const FarMacroLoad *Data=nullptr);
	FarKey         PeekKey() const;
	bool           ProcessKey(FarKey Key, const INPUT_RECORD *Rec=nullptr);
	void           SetArea(FARMACROAREA Area) { m_Area=Area; }
	void           SuspendMacros(bool Suspend) { Suspend ? ++m_InternalInput : --m_InternalInput; }

private:
	static FARString    m_RecCode;
	static FARString    m_RecDescription;
	static FARMACROAREA m_StartArea;

	FARMACROAREA  m_Area;
	int           m_InternalInput;
	FARMACROSTATE m_Recording;
	FARString     m_StringToPrint;
	int           m_WaitKey;

private:
	void RestoreMacroChar() const;

	static bool AssignMacroKey(void *Param);
	static bool CheckAll(FARMACROAREA Area, DWORD CurFlags);
	static bool CheckCmdLine(DWORD Flags);
	static bool CheckEditSelected(FARMACROAREA Area, DWORD CurFlags);
	static bool CheckFileFolder(DWORD CurFlags, bool IsPassivePanel);
	static bool CheckPanel(int PanelMode,DWORD CurFlags, bool IsPassivePanel);
	static FARMACROSTATE GetExecutingState();
	static bool GetMacroSettings(FarKey Key, DWORD &Flags, const wchar_t* Src, const wchar_t* Descr);

	static LONG_PTR WINAPI AssignMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
	static LONG_PTR WINAPI ParamMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
};

inline bool IsNum(const FarMacroValue &val) {
	return val.Type==FMVT_DOUBLE || val.Type==FMVT_INTEGER;
}

inline int64_t ToInt(const FarMacroValue &val) {
	return val.Type==FMVT_DOUBLE ? (int64_t)val.Double : val.Type==FMVT_INTEGER ? val.Integer : 0;
}

inline bool IsStr(const FarMacroValue &val) {
	return val.Type==FMVT_STRING;
}

DWORD       GetHistoryDisableMask();
std::string GuidToString(const GUID& Guid);
bool        IsMenuArea(int Area);
bool        IsPanelsArea(int Area);
bool        IsTopMacroOutputDisabled();
DWORD       SetHistoryDisableMask(DWORD Mask);
void        ShowUserMenu(size_t Count, const FarMacroValue *Values);
bool        StrToGuid(const wchar_t *Str, GUID& Guid);
