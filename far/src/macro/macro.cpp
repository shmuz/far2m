/*
macro.cpp

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

#include "headers.hpp"

#include "Bookmarks.hpp"
#include "clipboard.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "fileedit.hpp"
#include "filelist.hpp"
#include "filepanels.hpp"
#include "filetype.hpp"
#include "fileview.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "lang.hpp"
#include "macro.hpp"
#include "macroopcode.hpp"
#include "macrovalues.hpp"
#include "MaskGroups.hpp"
#include "message.hpp"
#include "scrbuf.hpp"
#include "usermenu.hpp"

extern Panel* SelectPanel(int Type);

static long long msValues[constMsLAST];

int Log(const char* Format, ...)
{
	va_list valist;
	va_start(valist, Format);

	static int N = 0;
	if (const char* home = getenv("HOME")) {
		char* buf = (char*) malloc(strlen(home) + 64);
		if (buf) {
			strcpy(buf, home);
			strcat(buf, "/luafar_log.txt");
			FILE* fp = fopen(buf, "a");
			if (fp) {
				if (++N == 1) {
					time_t rtime = time(nullptr);
					fprintf(fp, "\n%s------------------------------\n", ctime(&rtime));
				}
				fprintf(fp, "%d: ", N);
				vfprintf(fp, Format, valist);
				fprintf(fp, "\n");
				fclose(fp);
			}
			free(buf);
		}
	}
	va_end(valist);
	return N;
}

bool StrToGuid(const wchar_t *Str, GUID& Guid)
{
	const char tmpl[] = "HHHHHHHH-HHHH-HHHH-HHHH-HHHHHHHHHHHH";
	uint8_t buf[20];
	wchar_t aux[] = {0,0,0};

	for (int i=0,j=0; ; ) {
		if (tmpl[i] == 'H') {
			if (iswxdigit(aux[0] = Str[i++]) && iswxdigit(aux[1] = Str[i++]))
				buf[j++] = wcstol(aux, nullptr, 16);
			else
				return false;
		}
		else {
			if (tmpl[i] != Str[i])
				return false;

			if (!tmpl[i++])
				break;
		}
	}

	Guid.Data1 = (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3];
	Guid.Data2 = (buf[4]<<8) + buf[5];
	Guid.Data3 = (buf[6]<<8) + buf[7];
	memcpy(Guid.Data4, buf+8, 8);
	return true;
}

std::string GuidToString(const GUID& Guid)
{
	char buf[64];
	sprintf(buf, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			Guid.Data1, (uint32_t)Guid.Data2, (uint32_t)Guid.Data3,
			(uint32_t)Guid.Data4[0], (uint32_t)Guid.Data4[1],
			(uint32_t)Guid.Data4[2], (uint32_t)Guid.Data4[3],
			(uint32_t)Guid.Data4[4], (uint32_t)Guid.Data4[5],
			(uint32_t)Guid.Data4[6], (uint32_t)Guid.Data4[7]);
	return buf;
}

// для диалога назначения клавиши
struct DlgParam
{
	bool Changed;
	FarKey MacroKey;
	DWORD Flags;
};

FARString KeyMacro::m_RecCode;
FARString KeyMacro::m_RecDescription;
FARMACROAREA KeyMacro::m_StartArea;

void ShowUserMenu(size_t Count, const FarMacroValue *Values)
{
	if (Count == 0)
		UserMenu(false);
	else if (Values[0].Type == FMVT_BOOLEAN)
		UserMenu(Values[0].Boolean != 0);
	else if (Values[0].Type == FMVT_STRING)
		UserMenu(FARString(Values[0].String));
}

static const wchar_t* GetMacroLanguage(DWORD Flags)
{
	switch(Flags & KMFLAGS_LANGMASK)
	{
		default:
		case KMFLAGS_LUA:        return L"lua";
		case KMFLAGS_MOONSCRIPT: return L"moonscript";
	}
}

static bool CallMacroPlugin(OpenMacroPluginInfo* Info)
{
#ifdef USELUA
	return CtrlObject->Plugins.CallPlugin(SYSID_LUAMACRO, OPEN_LUAMACRO, Info) != 0;
#else
	return false;
#endif
}

static bool MacroPluginOp(int OpCode, const FarMacroValue& Param, MacroPluginReturn* Ret = nullptr)
{
	FarMacroValue values[] = {static_cast<double> (OpCode), Param};
	FarMacroCall fmc = {sizeof(FarMacroCall), 2, values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_KEYMACRO, &fmc};
	if (CallMacroPlugin(&info))
	{
		if (Ret) *Ret = info.Ret;
		return true;
	}
	return false;
}

FARMACROSTATE KeyMacro::GetExecutingState()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISEXECUTING, false, &Ret) ?
		static_cast<FARMACROSTATE>(Ret.ReturnType) : MACROSTATE_NOMACRO;
}

bool KeyMacro::IsOutputDisabled()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISDISABLEOUTPUT, false, &Ret) && Ret.ReturnType;
}

DWORD SetHistoryDisableMask(DWORD Mask)
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_HISTORYDISABLEMASK, static_cast<double>(Mask), &Ret)? Ret.ReturnType : 0;
}

DWORD GetHistoryDisableMask()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_HISTORYDISABLEMASK, false, &Ret) ? Ret.ReturnType : 0;
}

bool KeyMacro::IsHistoryDisabled(int TypeHistory)
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISHISTORYDISABLE, static_cast<double>(TypeHistory), &Ret) && Ret.ReturnType;
}

bool IsTopMacroOutputDisabled()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISTOPMACROOUTPUTDISABLED, false, &Ret) && Ret.ReturnType;
}

static bool IsPostMacroEnabled()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISPOSTMACROENABLED, false, &Ret) && Ret.ReturnType;
}

static void SetMacroValue(bool Value)
{
	MacroPluginOp(OP_SETMACROVALUE, Value);
}

static bool TryToPostMacro(int Area, const FARString& TextKey, DWORD IntKey)
{
	FarMacroValue values[] = { static_cast<double>(OP_TRYTOPOSTMACRO), static_cast<double>(Area),
		TextKey.CPtr(), static_cast<double>(IntKey) };
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_KEYMACRO, &fmc};
	return CallMacroPlugin(&info);
}

KeyMacro::KeyMacro():
	m_Area(MACROAREA_SHELL),
	m_Recording(MACROSTATE_NOMACRO),
	m_InternalInput(0),
	m_WaitKey(0)
{
}

bool KeyMacro::LoadMacros(bool FromFar, bool InitedRAM, const FarMacroLoad *Data)
{
	if (FromFar)
	{
		if (Opt.Macro.DisableMacro&MDOL_ALL) return false;
	}
	else
	{
		if (!CtrlObject->Plugins.IsPluginsLoaded()) return false;
	}

	m_Recording = MACROSTATE_NOMACRO;

	FarMacroValue values[] = {InitedRAM, false, 0.0};
	if (Data)
	{
		if (Data->Path) values[1] = Data->Path;
		values[2] = static_cast<double>(Data->Flags);
	}
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_LOADMACROS, &fmc};
	return CallMacroPlugin(&info);
}

bool KeyMacro::SaveMacros()
{
	OpenMacroPluginInfo info = {MCT_WRITEMACROS, nullptr};
	return CallMacroPlugin(&info);
}

FARMACROSTATE KeyMacro::GetState() const
{
	return (m_Recording != MACROSTATE_NOMACRO) ? m_Recording : GetExecutingState();
}

bool KeyMacro::CanSendKeysToPlugin() const
{
	int state = GetState();
	return state != MACROSTATE_RECORDING && state != MACROSTATE_EXECUTING;
}

static bool GetInputFromMacro(MacroPluginReturn *mpr)
{
	return MacroPluginOp(OP_GETINPUTFROMMACRO, false, mpr);
}

void KeyMacro::RestoreMacroChar() const
{
	ScrBuf.RestoreMacroChar();

	if (m_Area == MACROAREA_EDITOR &&
					CtrlObject->Plugins.CurEditor &&
					CtrlObject->Plugins.CurEditor->IsVisible()
					/* && LockScr*/) // Mantis#0001595
	{
		CtrlObject->Plugins.CurEditor->Show();
	}
	else if (m_Area == MACROAREA_VIEWER &&
					CtrlObject->Plugins.CurViewer &&
					CtrlObject->Plugins.CurViewer->IsVisible())
	{
		CtrlObject->Plugins.CurViewer->Show(); // иначе может быть неправильный верхний левый символ экрана
	}
}

struct GetMacroData
{
	FARMACROAREA Area;
	const wchar_t *Code;
	const wchar_t *Description;
	MACROFLAGS_MFLAGS Flags;
	bool IsKeyboardMacro;
};

static bool LM_GetMacro(GetMacroData* Data, int Area, const FARString& TextKey, bool UseCommon)
{
	FarMacroValue InValues[] = { static_cast<double>(Area), TextKey.CPtr(), UseCommon };
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(InValues), InValues, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_GETMACRO, &fmc};

	if (CallMacroPlugin(&info) && info.Ret.Count >= 5)
	{
		const auto* Values = info.Ret.Values;
		Data->Area        = static_cast<FARMACROAREA>(static_cast<int>(Values[0].Double));
		Data->Code        = Values[1].Type == FMVT_STRING ? Values[1].String : L"";
		Data->Description = Values[2].Type == FMVT_STRING ? Values[2].String : L"";
		Data->Flags       = static_cast<MACROFLAGS_MFLAGS>(Values[3].Double);
		Data->IsKeyboardMacro = Values[4].Boolean != 0;
		return true;
	}
	return false;
}

static void LM_ProcessRecordedMacro(int Area, const FARString& TextKey, const FARString& Code,
	MACROFLAGS_MFLAGS Flags, const FARString& Description)
{
	FarMacroValue values[] = { static_cast<double>(Area), TextKey.CPtr(), Code.CPtr(), Flags, Description.CPtr() };
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_RECORDEDMACRO, &fmc};
	CallMacroPlugin(&info);
}

bool KeyMacro::ParseMacroString(const wchar_t* Sequence, DWORD Flags, bool skipFile)
{
	const wchar_t* lang = GetMacroLanguage(Flags);
	const auto onlyCheck = (Flags&KMFLAGS_SILENTCHECK) != 0;

	// Перекладываем вывод сообщения об ошибке на плагин, т.к. штатный Message()
	// не умеет сворачивать строки и обрезает сообщение.
	FarMacroValue values[] = {lang, Sequence, onlyCheck, skipFile};
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_MACROPARSE, &fmc};

	if (CallMacroPlugin(&info))
	{
		if (info.Ret.ReturnType == MPRT_NORMALFINISH)
		{
			return true;
		}
		else if (info.Ret.ReturnType == MPRT_ERRORPARSE)
		{
			if (!onlyCheck)
			{
				ScrBuf.RestoreMacroChar();
				//### TODO WindowManager->RefreshWindow(); // Нужно после вывода сообщения плагином. Иначе панели не перерисовываются.
			}
		}
	}
	return false;
}

bool KeyMacro::ExecuteString(MacroExecuteString *Data)
{
	const auto onlyCheck = (Data->Flags & KMFLAGS_SILENTCHECK) != 0;
	FarMacroValue values[] = {GetMacroLanguage(Data->Flags), Data->SequenceText, FarMacroValue(Data->InValues, Data->InCount), onlyCheck};
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_EXECSTRING, &fmc};

	if (CallMacroPlugin(&info) && info.Ret.ReturnType == MPRT_NORMALFINISH)
	{
		Data->OutValues = info.Ret.Values;
		Data->OutCount = info.Ret.Count;
		return true;
	}
	Data->OutCount = 0;
	return false;
}

bool KeyMacro::GetMacroParseError(COORD& ErrPos, FARString& ErrSrc)
{
	MacroPluginReturn Ret;
	if (MacroPluginOp(OP_GETLASTERROR, false, &Ret))
	{
		ErrSrc = Ret.Values[0].String;
		ErrPos.Y = static_cast<int>(Ret.Values[1].Double);
		ErrPos.X = static_cast<int>(Ret.Values[2].Double);
		return ErrSrc.IsEmpty();
	}
	else
	{
		ErrSrc = L"No response from macro plugin";
		ErrPos = {};
		return false;
	}
}

static DWORD LayoutKey(DWORD key)
{
	if ((key&0x00FFFFFF) > 0x7F && (key&0x00FFFFFF) < 0xFFFF)
		key = KeyToKeyLayout(key&0x0000FFFF) | (key&0xFFFF0000);
	return key;
}

bool KeyMacro::ProcessKey(FarKey dwKey, const INPUT_RECORD *Rec)
{
	if (m_InternalInput || dwKey == KEY_IDLE || dwKey == KEY_NONE || !FrameManager->GetCurrentFrame())
		return false;

	dwKey = CorrectKey(dwKey, Rec);

	FARString textKey;
	if (!KeyToText(dwKey, textKey) || textKey.IsEmpty())
		return false;

	const bool ctrldot = textKey == Opt.Macro.strKeyMacroCtrlDot;
	const bool ctrlshiftdot = textKey == Opt.Macro.strKeyMacroCtrlShiftDot;

	if (m_Recording == MACROSTATE_NOMACRO)
	{
		if ((ctrldot||ctrlshiftdot) && !IsExecuting())
		{
			bool OK = false;
			if (auto Plug = CtrlObject->Plugins.FindPlugin(SYSID_LUAMACRO)) { //### && !IsPendingRemove())
				PluginInfo Info{};
				Plug->GetPluginInfo(&Info);
				OK = Info.StructSize && Info.CommandPrefix;
			}
			if (!OK) {
				m_InternalInput = true; //prevent multiple message boxes
				Message(MSG_WARNING, 1, Msg::Error,
				   Msg::MacroPluginLuamacroNotLoaded,
				   Msg::MacroRecordingIsDisabled,
				   Msg::HOk);
				m_InternalInput = false;
				return false;
			}

			// Где мы?
			m_StartArea = m_Area;
			// В зависимости от того, КАК НАЧАЛИ писать макрос, различаем общий режим (Ctrl-.
			// с передачей плагину кеев) или специальный (Ctrl-Shift-. - без передачи клавиш плагину)
			m_Recording = ctrldot?MACROSTATE_RECORDING_COMMON:MACROSTATE_RECORDING;

			m_RecCode.Clear();
			m_RecDescription.Clear();
			ScrBuf.Flush(true);
			return true;
		}
		else
		{
			if (!m_WaitKey && IsPostMacroEnabled())
			{
				DWORD key = LayoutKey(dwKey);

				if (key < 0xFFFF)
					key = Upper(static_cast<wchar_t>(key));

				FARString str = textKey;
				if (key != dwKey)
					KeyToText(key, str);

				auto last_input = *FrameManager->GetLastInputRecord();
				if (TryToPostMacro(m_Area, str, dwKey))
					return true;
				// Mantis 0002307: При вызове msgbox из condition(), ключ закрытия msgbox передаётся дальше (не съедается)
				*FrameManager->GetLastInputRecord() = last_input;
			}
		}
	}
	else // m_Recording != MACROSTATE_NOMACRO
	{
		if (ctrldot||ctrlshiftdot) // признак конца записи?
		{
			m_InternalInput = 1;
			DlgParam Param {false, 0, 0};
			bool AssignRet = AssignMacroKey(&Param);

			if (AssignRet && !Param.Changed && !m_RecCode.IsEmpty())
			{
				m_RecCode = L"Keys(\"" + m_RecCode + L"\")";
				if (ctrlshiftdot && !GetMacroSettings(Param.MacroKey, Param.Flags, m_RecCode, m_RecDescription))
				{
					AssignRet = false;
				}
			}
			m_InternalInput = 0;

			if (AssignRet)
			{
				FARString strKey;
				KeyToText(Param.MacroKey, strKey);
				Param.Flags |= m_Recording == MACROSTATE_RECORDING_COMMON ? MFLAGS_NONE : MFLAGS_NOSENDKEYSTOPLUGINS;
				LM_ProcessRecordedMacro(m_StartArea, strKey, m_RecCode, Param.Flags, m_RecDescription);
				if (Opt.AutoSaveSetup)
					SaveMacros();
			}

			m_Recording = MACROSTATE_NOMACRO;
			ScrBuf.Flush(true);
			return true;
		}
		else
		{
			if (!IsProcessAssignMacroKey)
			{
				if (!m_RecCode.IsEmpty())
					m_RecCode += L' ';

				m_RecCode += textKey == L"\"" ? L"\\\"" : textKey;
			}
			return false;
		}
	}

	return false;
}

FarKey KeyMacro::GetKey()
{
	if (m_InternalInput || !FrameManager->GetCurrentFrame())
		return 0;

	MacroPluginReturn mpr;
	while (GetInputFromMacro(&mpr))
	{
		switch (mpr.ReturnType)
		{
			default:
				return 0;

			case MPRT_HASNOMACRO:
				if (m_Area == MACROAREA_EDITOR &&
					CtrlObject->Plugins.CurEditor &&
					CtrlObject->Plugins.CurEditor->IsVisible() &&
					ScrBuf.GetLockCount())
				{
					CtrlObject->Plugins.CurEditor->Show();
				}

				ScrBuf.Unlock();

				Clipboard::SetUseInternalClipboardState(false);

				return 0;

			case MPRT_KEYS:
			{
				switch (static_cast<int>(mpr.Values[0].Double))
				{
					case 1:
						return KEY_OP_SELWORD;
					case 2:
						return KEY_OP_XLAT;
					default:
						return static_cast<FarKey>(mpr.Values[1].Double);
				}
			}

			case MPRT_PRINT:
			{
				m_StringToPrint = mpr.Values[0].String;
				return KEY_OP_PLAINTEXT;
			}

			case MPRT_PLUGINMENU:   // N = Plugin.Menu(Uuid[, MenuUuid])
			case MPRT_PLUGINCONFIG: // N = Plugin.Config(Uuid[, MenuUuid])
			case MPRT_PLUGINCOMMAND: // N = Plugin.Command(Uuid[, Command])
			{
				SetMacroValue(false);

				if (!mpr.Count || mpr.Values[0].Type != FMVT_DOUBLE)
					break;

				DWORD SysID = static_cast<DWORD>(mpr.Values[0].Double);
				if (!CtrlObject->Plugins.FindPlugin(SysID))
					break;

				bool IsLuamacro = (SysID == SYSID_LUAMACRO);
				GUID Guid;

				PluginManager::CallPluginInfo cpInfo = { CPT_CHECKONLY };
				if (mpr.ReturnType == MPRT_PLUGINMENU || mpr.ReturnType == MPRT_PLUGINCONFIG)
				{
					if (!IsLuamacro) {
						if (mpr.Count > 1) {
							if (mpr.Values[1].Type == FMVT_DOUBLE)
								cpInfo.ItemNumber = static_cast<DWORD>(mpr.Values[1].Double);
							else
								break;
						}
					}
					else {
						if (mpr.Count > 1) {
							if (mpr.Values[1].Type == FMVT_STRING && StrToGuid(mpr.Values[1].String, Guid))
								cpInfo.ItemUuid = &Guid;
							else
								break;
						}
					}
				}

				if (mpr.ReturnType == MPRT_PLUGINMENU)
					cpInfo.CallFlags |= CPT_MENU;
				else if (mpr.ReturnType == MPRT_PLUGINCONFIG)
					cpInfo.CallFlags |= CPT_CONFIGURE;
				else if (mpr.ReturnType == MPRT_PLUGINCOMMAND)
				{
					cpInfo.CallFlags |= CPT_CMDLINE;
					cpInfo.Command = L"";
					if (mpr.Count > 1) {
						if (mpr.Values[1].Type == FMVT_STRING)
							cpInfo.Command = mpr.Values[1].String;
						else
							break;
					}
				}

				// Чтобы вернуть результат "выполнения" нужно проверить наличие плагина/пункта
				if (CtrlObject->Plugins.CallPluginItem(SysID, &cpInfo))
				{
					// Если нашли успешно - то теперь выполнение
					SetMacroValue(true);
					cpInfo.CallFlags&=~CPT_CHECKONLY;
					CtrlObject->Plugins.CallPluginItem(SysID, &cpInfo);
				}
				FrameManager->RefreshFrame();

				//с текущим переключением окон могут быть проблемы с заголовком консоли.
				FrameManager->Commit();

				break;
			}

			case MPRT_USERMENU:
				ShowUserMenu(mpr.Count, mpr.Values);
				break;

			case MPRT_FOLDERSHORTCUTS:
				if (IsPanelsArea(m_Area)) {
					ShowBookmarksMenu();
				}
				break;

			case MPRT_FILEASSOCIATIONS:
				if (IsPanelsArea(m_Area)) {
					EditFileTypes();
				}
				break;

			case MPRT_FILEHIGHLIGHT:
				if (IsPanelsArea(m_Area)) {
					CtrlObject->HiFiles->HiEdit(0);
				}
				break;

			case MPRT_FILEPANELMODES:
				if (IsPanelsArea(m_Area)) {
					FileList::SetFilePanelModes();
				}
				break;

			case MPRT_FILEMASKGROUPS:
				if (IsPanelsArea(m_Area)) {
					MaskGroupsSettings();
				}
				break;
		}
	}

	return 0;
}

// Проверить - есть ли еще клавиша?
FarKey KeyMacro::PeekKey() const
{
	return !m_InternalInput && IsExecuting();
}

bool KeyMacro::GetMacroKeyInfo(const FARString& StrArea, int Pos, FARString &strKeyName, FARString &strDescription)
{
	FarMacroValue values[] = {StrArea.CPtr(), !Pos};
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_ENUMMACROS, &fmc};

	if (CallMacroPlugin(&info) && info.Ret.Count >= 2)
	{
		strKeyName = info.Ret.Values[0].String;
		strDescription = info.Ret.Values[1].String;
		return true;
	}
	return false;
}

void KeyMacro::SetMacroConst(int ConstIndex, long long Value)
{
	msValues[ConstIndex] = Value;
}

long long KeyMacro::GetMacroConst(int ConstIndex)
{
	return msValues[ConstIndex];
}

// Функция, запускающая макросы при старте ФАРа
void KeyMacro::RunStartMacro()
{
	if (Opt.Macro.DisableMacro & (MDOL_ALL|MDOL_AUTOSTART))
		return;

	if (!CtrlObject || !CtrlObject->Cp() || !CtrlObject->Cp()->ActivePanel || !CtrlObject->Plugins.IsPluginsLoaded())
		return;

	static bool IsRunStartMacro = false, IsInside = false;

	if (!IsRunStartMacro && !IsInside)
	{
		IsInside = true;
		OpenMacroPluginInfo info = {MCT_RUNSTARTMACRO, nullptr};
		IsRunStartMacro = CallMacroPlugin(&info);
		IsInside = false;
	}
}

bool KeyMacro::AddMacro(DWORD PluginId, const MacroAddMacro* Data)
{
	if (!(Data->Area >= 0 && (Data->Area < MACROAREA_LAST || Data->Area == MACROAREA_COMMON)))
		return false;

	if (!Data->AKey || !*Data->AKey)
		return false;

	MACROFLAGS_MFLAGS Flags = 0;
	if (Data->Flags & KMFLAGS_ENABLEOUTPUT)        Flags |= MFLAGS_ENABLEOUTPUT;
	if (Data->Flags & KMFLAGS_NOSENDKEYSTOPLUGINS) Flags |= MFLAGS_NOSENDKEYSTOPLUGINS;

	FarMacroValue values[] = {
		static_cast<double>(Data->Area),
		Data->AKey,
		GetMacroLanguage(Data->Flags),
		Data->SequenceText,
		Flags,
		Data->Description,
		PluginId,
		reinterpret_cast<void*>(Data->Callback),
		Data->Id,
		Data->Priority
	};
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_ADDMACRO, &fmc};
	return CallMacroPlugin(&info);
}

bool KeyMacro::DelMacro(DWORD PluginId, void* Id)
{
	FarMacroValue values[] = {PluginId, Id};
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_DELMACRO, &fmc};
	return CallMacroPlugin(&info);
}

// обработчик диалогового окна назначения клавиши
LONG_PTR WINAPI KeyMacro::AssignMacroDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	FARString strKeyText;
	static int Recurse;
	static FarKey LastKey;
	static DlgParam *KMParam;
	bool KeyIsValid = false;

	if (Msg == DN_INITDIALOG)
	{
		KMParam = reinterpret_cast<DlgParam*>(Param2);
		LastKey = 0;
		Recurse = 0;
		// <Клавиши, которые не введешь в диалоге назначения>
		FarKey PreDefKeyMain[]=
		{
			KEY_CTRLDOWN, KEY_ENTER, KEY_NUMENTER, KEY_ESC, KEY_F1, KEY_CTRLF5,
		};

		for (auto Key: PreDefKeyMain)
		{
			KeyToText(Key, strKeyText);
			SendDlgMessage(hDlg, DM_LISTADDSTR, 2, reinterpret_cast<LONG_PTR>(strKeyText.CPtr()));
		}

		FarKey PreDefKey[]=
		{
			KEY_MSWHEEL_UP, KEY_MSWHEEL_DOWN, KEY_MSWHEEL_LEFT, KEY_MSWHEEL_RIGHT,
			KEY_MSLCLICK, KEY_MSRCLICK, KEY_MSM1CLICK, KEY_MSM2CLICK, KEY_MSM3CLICK,
#if 0
			KEY_MSLDBLCLICK, KEY_MSRDBLCLICK, KEY_MSM1DBLCLICK, KEY_MSM2DBLCLICK, KEY_MSM3DBLCLICK,
#endif
		};
		FarKey PreDefModKey[]=
		{
			0, KEY_CTRL, KEY_SHIFT, KEY_ALT, KEY_CTRLSHIFT, KEY_CTRLALT, KEY_ALTSHIFT,
		};

		for (auto Key: PreDefKey)
		{
			SendDlgMessage(hDlg, DM_LISTADDSTR, 2, reinterpret_cast<LONG_PTR>(L"\1"));

			for (auto Mod: PreDefModKey)
			{
				KeyToText(Key | Mod, strKeyText);
				SendDlgMessage(hDlg, DM_LISTADDSTR, 2, reinterpret_cast<LONG_PTR>(strKeyText.CPtr()));
			}
		}

		SendDlgMessage(hDlg, DM_SETTEXTPTR, 2, reinterpret_cast<LONG_PTR>(L""));
		// </Клавиши, которые не введешь в диалоге назначения>
	}
	else if (Param1 == 2 && Msg == DN_EDITCHANGE)
	{
		LastKey = 0;
		FarKey KeyCode = KeyNameToKey(((FarDialogItem*)Param2)->PtrData);

		if (KeyCode != KEY_INVALID && !Recurse)
		{
			Param2 = KeyCode;
			KeyIsValid = true;
		}
	}
	else if (Msg == DN_KEY)
	{
		DWORD dw = Param2 & KEY_END_SKEY;
		if ((dw < KEY_END_FKEY) || (dw > INTERNAL_KEY_BASE && dw < INTERNAL_KEY_BASE_2))
		{
			// Обработка особых клавиш
			if (Param2 == KEY_ESC || Param2 == KEY_CTRLDOWN || Param2 == KEY_F1)
				return FALSE;

			// Было что-то уже нажато и Enter`ом подтверждаем
			if (LastKey && (Param2 == KEY_ENTER || Param2 == KEY_NUMENTER))
				return FALSE;

			KeyIsValid = true;
		}
	}

	if (KeyIsValid)
	{
		Param2 = LayoutKey(Param2);

		//косметика
		if (Param2 < 0xFFFF)
			Param2 = Upper((wchar_t)Param2);

		KMParam->MacroKey = (FarKey)Param2;
		KeyToText((FarKey)Param2, strKeyText);

		// если УЖЕ есть такой макрос...
		GetMacroData Data;
		if (LM_GetMacro(&Data, m_StartArea, strKeyText, true) && Data.IsKeyboardMacro)
		{
			// общие макросы учитываем только при удалении.
			if (m_RecCode.IsEmpty() || Data.Area != MACROAREA_COMMON)
			{
				FARString strBufKey;
				bool SetDelete = m_RecCode.IsEmpty();
				if (Data.Code)
				{
					strBufKey = Data.Code;
					InsertQuote(strBufKey);
				}

				FARString strBuf;
				if (Data.Area == MACROAREA_COMMON)
					strBuf.Format(SetDelete ? Msg::MacroCommonDeleteKey : Msg::MacroCommonReDefinedKey, strKeyText.CPtr());
				else
					strBuf.Format(SetDelete ? Msg::MacroDeleteKey : Msg::MacroReDefinedKey, strKeyText.CPtr());

				int	Result = SetDelete ?
					Message(MSG_WARNING, 3, Msg::Warning, strBuf, Msg::MacroSequence, strBufKey,
							Msg::MacroDeleteKey2,
							Msg::Yes, Msg::MacroEditKey, Msg::No) :
					Message(MSG_WARNING, 2, Msg::Warning, strBuf, Msg::MacroSequence, strBufKey,
							Msg::MacroReDefinedKey2,
							Msg::Yes, Msg::No);

				if (Result == 0)
				{
					SendDlgMessage(hDlg, DM_CLOSE, 1, 0);
					return TRUE;
				}

				if (SetDelete && Result == 1)
				{
					auto key = (DWORD)Param2;
					FARString strDescription;

					if ( *Data.Code )
						strBufKey = Data.Code;

					if ( *Data.Description )
						strDescription = Data.Description;

					if (GetMacroSettings(key, Data.Flags, strBufKey, strDescription))
					{
						KMParam->Flags = Data.Flags;
						KMParam->Changed = true;
						SendDlgMessage(hDlg, DM_CLOSE, 1, 0);
						return TRUE;
					}
				}

				// здесь - здесь мы нажимали "Нет", ну а на нет и суда нет
				//  и значит очистим поле ввода.
				strKeyText.Clear();
			}
		}

		Recurse++;
		SendDlgMessage(hDlg, DM_SETTEXTPTR, 2, (LONG_PTR)strKeyText.CPtr());
		Recurse--;
		LastKey=(FarKey)Param2;
		return TRUE;
	}
	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool KeyMacro::AssignMacroKey(DlgParam *Param)
{
	/*
	  +------ Define macro ------+
	  | Press the desired key    |
	  | ________________________ |
	  +--------------------------+
	*/
	DialogDataEx MacroAssignDlgData[]=
	{
		{DI_DOUBLEBOX, 3, 1, 30, 4, {}, 0, Msg::DefineMacroTitle},
		{DI_TEXT, -1, 2, 0, 2, {}, 0, Msg::DefineMacro},
		{DI_COMBOBOX, 5, 3, 28, 3, {}, DIF_FOCUS|DIF_DEFAULT, L""}
	};
	MakeDialogItemsEx(MacroAssignDlgData, MacroAssignDlg);
	IsProcessAssignMacroKey++;
	Dialog Dlg(MacroAssignDlg, ARRAYSIZE(MacroAssignDlg), AssignMacroDlgProc, (LONG_PTR)Param);
	Dlg.SetPosition(-1, -1, 34, 6);
	Dlg.SetHelp(L"KeyMacro");
	Dlg.Process();
	IsProcessAssignMacroKey--;

	return (Dlg.GetExitCode() != -1);
}

static int Set3State(DWORD Flags, DWORD Chk1, DWORD Chk2)
{
	bool b1 = (Flags & Chk1) != 0;
	bool b2 = (Flags & Chk2) != 0;
	return b1 == b2 ? 2 : b1 ? 1 : 0;
}

static DWORD Get3State(int Selected, DWORD Chk1, DWORD Chk2)
{
	return Selected == 2 ? 0 : Selected == 0 ? Chk1 : Chk2;
}

enum MACROSETTINGSDLG
{
	MS_DOUBLEBOX,
	MS_TEXT_SEQUENCE,
	MS_EDIT_SEQUENCE,
	MS_TEXT_DESCR,
	MS_EDIT_DESCR,
	MS_SEPARATOR1,
	MS_CHECKBOX_OUTPUT,
	MS_CHECKBOX_START,
	MS_SEPARATOR2,
	MS_CHECKBOX_A_PANEL,
	MS_CHECKBOX_A_PLUGINPANEL,
	MS_CHECKBOX_A_FOLDERS,
	MS_CHECKBOX_A_SELECTION,
	MS_CHECKBOX_P_PANEL,
	MS_CHECKBOX_P_PLUGINPANEL,
	MS_CHECKBOX_P_FOLDERS,
	MS_CHECKBOX_P_SELECTION,
	MS_SEPARATOR3,
	MS_CHECKBOX_CMDLINE,
	MS_CHECKBOX_SELBLOCK,
	MS_SEPARATOR4,
	MS_BUTTON_OK,
	MS_BUTTON_CANCEL,
};

LONG_PTR WINAPI KeyMacro::ParamMacroDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg)
	{
		case DN_BTNCLICK:

			if (Param1 == MS_CHECKBOX_A_PANEL || Param1 == MS_CHECKBOX_P_PANEL)
				for (int i = 1; i <= 3; i++)
					SendDlgMessage(hDlg, DM_ENABLE, Param1+i, Param2);

			break;
		case DN_CLOSE:

			if (Param1 == MS_BUTTON_OK)
			{
				LPCWSTR Sequence=(LPCWSTR)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, MS_EDIT_SEQUENCE, 0);

				if (*Sequence)
				{
					if (ParseMacroString(Sequence, KMFLAGS_LUA, true))
					{
						m_RecCode = Sequence;
						m_RecDescription = (LPCWSTR)SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, MS_EDIT_DESCR, 0);
						return TRUE;
					}
				}
				return FALSE;
			}

			break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool KeyMacro::GetMacroSettings(FarKey Key, DWORD &Flags, const wchar_t* Src, const wchar_t* Descr)
{
	/*
	          1         2         3         4         5         6
	   3456789012345678901234567890123456789012345678901234567890123456789
	 1 г=========== Параметры макрокоманды для 'CtrlP' ==================¬
	 2 | Последовательность:                                             |
	 3 | _______________________________________________________________ |
	 4 | Описание:                                                       |
	 5 | _______________________________________________________________ |
	 6 |-----------------------------------------------------------------|
	 7 | [ ] Разрешить во время выполнения вывод на экран                |
	 8 | [ ] Выполнять после запуска FAR                                 |
	 9 |-----------------------------------------------------------------|
	10 | [ ] Активная панель             [ ] Пассивная панель            |
	11 |   [?] На панели плагина           [?] На панели плагина         |
	12 |   [?] Выполнять для папок         [?] Выполнять для папок       |
	13 |   [?] Отмечены файлы              [?] Отмечены файлы            |
	14 |-----------------------------------------------------------------|
	15 | [?] Пустая командная строка                                     |
	16 | [?] Отмечен блок                                                |
	17 |-----------------------------------------------------------------|
	18 |               [ Продолжить ]  [ Отменить ]                      |
	19 L=================================================================+

	*/
	DialogDataEx MacroSettingsDlgData[]=
	{
		{DI_DOUBLEBOX, 3,  1, 69, 19, {},  0, L""},
		{DI_TEXT,      5,  2,  0,  2, {},  0, Msg::MacroSequence},
		{DI_EDIT,      5,  3, 67,  3, {reinterpret_cast<DWORD_PTR>(L"MacroSequence")}, DIF_HISTORY|DIF_FOCUS, L""},
		{DI_TEXT,      5,  4,  0,  4, {},  0, Msg::MacroDescription},
		{DI_EDIT,      5,  5, 67,  5, {reinterpret_cast<DWORD_PTR>(L"MacroDescription")}, DIF_HISTORY, L""},
		{DI_TEXT,      3,  6,  0,  6, {},  DIF_SEPARATOR, L""},
		{DI_CHECKBOX,  5,  7,  0,  7, {},  0, Msg::MacroSettingsEnableOutput},
		{DI_CHECKBOX,  5,  8,  0,  8, {},  0, Msg::MacroSettingsRunAfterStart},
		{DI_TEXT,      3,  9,  0,  9, {},  DIF_SEPARATOR, L""},
		{DI_CHECKBOX,  5, 10,  0, 10, {},  0, Msg::MacroSettingsActivePanel},
		{DI_CHECKBOX,  7, 11,  0, 11, {2}, DIF_3STATE, Msg::MacroSettingsPluginPanel},
		{DI_CHECKBOX,  7, 12,  0, 12, {2}, DIF_3STATE, Msg::MacroSettingsFolders},
		{DI_CHECKBOX,  7, 13,  0, 13, {2}, DIF_3STATE, Msg::MacroSettingsSelectionPresent},
		{DI_CHECKBOX, 37, 10,  0, 10, {},  0, Msg::MacroSettingsPassivePanel},
		{DI_CHECKBOX, 39, 11,  0, 11, {2}, DIF_3STATE, Msg::MacroSettingsPluginPanel},
		{DI_CHECKBOX, 39, 12,  0, 12, {2}, DIF_3STATE, Msg::MacroSettingsFolders},
		{DI_CHECKBOX, 39, 13,  0, 13, {2}, DIF_3STATE, Msg::MacroSettingsSelectionPresent},
		{DI_TEXT,      3, 14,  0, 14, {},  DIF_SEPARATOR, L""},
		{DI_CHECKBOX,  5, 15,  0, 15, {2}, DIF_3STATE, Msg::MacroSettingsCommandLine},
		{DI_CHECKBOX,  5, 16,  0, 16, {2}, DIF_3STATE, Msg::MacroSettingsSelectionBlockPresent},
		{DI_TEXT,      3, 17,  0, 17, {},  DIF_SEPARATOR, L""},
		{DI_BUTTON,    0, 18,  0, 18, {},  DIF_DEFAULT|DIF_CENTERGROUP, Msg::Ok},
		{DI_BUTTON,    0, 18,  0, 18, {},  DIF_CENTERGROUP, Msg::Cancel}
	};
	MakeDialogItemsEx(MacroSettingsDlgData, MacroSettingsDlg);
	FARString strKeyText;
	KeyToText(Key, strKeyText);
	MacroSettingsDlg[MS_DOUBLEBOX].strData.Format(Msg::MacroSettingsTitle, strKeyText.CPtr());
	MacroSettingsDlg[MS_CHECKBOX_OUTPUT].Selected = Flags&MFLAGS_ENABLEOUTPUT?1:0;
	MacroSettingsDlg[MS_CHECKBOX_START].Selected = Flags&MFLAGS_RUNAFTERFARSTART?1:0;

	int a = Set3State(Flags, MFLAGS_NOFILEPANELS, MFLAGS_NOPLUGINPANELS);
	int b = Set3State(Flags, MFLAGS_NOFILES, MFLAGS_NOFOLDERS);
	int c = Set3State(Flags, MFLAGS_SELECTION, MFLAGS_NOSELECTION);
	MacroSettingsDlg[MS_CHECKBOX_A_PLUGINPANEL].Selected = a;
	MacroSettingsDlg[MS_CHECKBOX_A_FOLDERS].Selected = b;
	MacroSettingsDlg[MS_CHECKBOX_A_SELECTION].Selected = c;
	MacroSettingsDlg[MS_CHECKBOX_A_PANEL].Selected = (a == 2 && b == 2 && c == 2) ? 0 : 1;
	if (0 == MacroSettingsDlg[MS_CHECKBOX_A_PANEL].Selected)
	{
		MacroSettingsDlg[MS_CHECKBOX_A_PLUGINPANEL].Flags |= DIF_DISABLE;
		MacroSettingsDlg[MS_CHECKBOX_A_FOLDERS].Flags |= DIF_DISABLE;
		MacroSettingsDlg[MS_CHECKBOX_A_SELECTION].Flags |= DIF_DISABLE;
	}

	a = Set3State(Flags, MFLAGS_PNOFILEPANELS, MFLAGS_PNOPLUGINPANELS);
	b = Set3State(Flags, MFLAGS_PNOFILES, MFLAGS_PNOFOLDERS);
	c = Set3State(Flags, MFLAGS_PSELECTION, MFLAGS_PNOSELECTION);
	MacroSettingsDlg[MS_CHECKBOX_P_PLUGINPANEL].Selected = a;
	MacroSettingsDlg[MS_CHECKBOX_P_FOLDERS].Selected = b;
	MacroSettingsDlg[MS_CHECKBOX_P_SELECTION].Selected = c;
	MacroSettingsDlg[MS_CHECKBOX_P_PANEL].Selected = (a == 2 && b == 2 && c == 2) ? 0 : 1;
	if (0 == MacroSettingsDlg[MS_CHECKBOX_P_PANEL].Selected)
	{
		MacroSettingsDlg[MS_CHECKBOX_P_PLUGINPANEL].Flags |= DIF_DISABLE;
		MacroSettingsDlg[MS_CHECKBOX_P_FOLDERS].Flags |= DIF_DISABLE;
		MacroSettingsDlg[MS_CHECKBOX_P_SELECTION].Flags |= DIF_DISABLE;
	}

	MacroSettingsDlg[MS_CHECKBOX_CMDLINE].Selected = Set3State(Flags, MFLAGS_EMPTYCOMMANDLINE, MFLAGS_NOTEMPTYCOMMANDLINE);
	MacroSettingsDlg[MS_CHECKBOX_SELBLOCK].Selected = Set3State(Flags, MFLAGS_EDITSELECTION, MFLAGS_EDITNOSELECTION);
	MacroSettingsDlg[MS_EDIT_SEQUENCE].strData = Src;
	MacroSettingsDlg[MS_EDIT_DESCR].strData = Descr;
	Dialog Dlg(MacroSettingsDlg, ARRAYSIZE(MacroSettingsDlg), ParamMacroDlgProc, 0);
	Dlg.SetPosition(-1, -1, 73, 21);
	Dlg.SetHelp(L"KeyMacroSetting");
	Frame* BottomFrame = FrameManager->GetBottomFrame();
	if(BottomFrame)
	{
		BottomFrame->Lock(); // отменим прорисовку фрейма
	}
	Dlg.Process();
	if(BottomFrame)
	{
		BottomFrame->Unlock(); // теперь можно :-)
	}

	if (Dlg.GetExitCode()!=MS_BUTTON_OK)
		return false;

	Flags = MacroSettingsDlg[MS_CHECKBOX_OUTPUT].Selected?MFLAGS_ENABLEOUTPUT:0;
	Flags|=MacroSettingsDlg[MS_CHECKBOX_START].Selected?MFLAGS_RUNAFTERFARSTART:0;

	if (MacroSettingsDlg[MS_CHECKBOX_A_PANEL].Selected)
	{
		Flags |= Get3State(MacroSettingsDlg[MS_CHECKBOX_A_PLUGINPANEL].Selected,
		         MFLAGS_NOPLUGINPANELS, MFLAGS_NOFILEPANELS);
		Flags |= Get3State(MacroSettingsDlg[MS_CHECKBOX_A_FOLDERS].Selected,
		         MFLAGS_NOFOLDERS, MFLAGS_NOFILES);
		Flags |= Get3State(MacroSettingsDlg[MS_CHECKBOX_A_SELECTION].Selected,
		         MFLAGS_NOSELECTION, MFLAGS_SELECTION);
	}

	if (MacroSettingsDlg[MS_CHECKBOX_P_PANEL].Selected)
	{
		Flags |= Get3State(MacroSettingsDlg[MS_CHECKBOX_P_PLUGINPANEL].Selected,
		         MFLAGS_PNOPLUGINPANELS, MFLAGS_PNOFILEPANELS);
		Flags |= Get3State(MacroSettingsDlg[MS_CHECKBOX_P_FOLDERS].Selected,
		         MFLAGS_PNOFOLDERS, MFLAGS_PNOFILES);
		Flags |= Get3State(MacroSettingsDlg[MS_CHECKBOX_P_SELECTION].Selected,
		         MFLAGS_PNOSELECTION, MFLAGS_PSELECTION);
	}

	Flags |= Get3State(MacroSettingsDlg[MS_CHECKBOX_CMDLINE].Selected,
	         MFLAGS_NOTEMPTYCOMMANDLINE, MFLAGS_EMPTYCOMMANDLINE);
	Flags |= Get3State(MacroSettingsDlg[MS_CHECKBOX_SELBLOCK].Selected,
	         MFLAGS_EDITNOSELECTION, MFLAGS_EDITSELECTION);
	return true;
}

bool KeyMacro::PostNewMacro(const wchar_t* Sequence, DWORD InputFlags, FarKey AKey)
{
	const wchar_t* Lang = GetMacroLanguage(InputFlags);
	const auto onlyCheck = (InputFlags & KMFLAGS_SILENTCHECK) != 0;
	MACROFLAGS_MFLAGS Flags = MFLAGS_POSTFROMPLUGIN;
	if (InputFlags & KMFLAGS_ENABLEOUTPUT)        Flags |= MFLAGS_ENABLEOUTPUT;
	if (InputFlags & KMFLAGS_NOSENDKEYSTOPLUGINS) Flags |= MFLAGS_NOSENDKEYSTOPLUGINS;

	FarMacroValue values[] = { static_cast<double>(OP_POSTNEWMACRO), Lang, Sequence,
		static_cast<double>(Flags), static_cast<double>(AKey), onlyCheck };
	FarMacroCall fmc = {sizeof(FarMacroCall), ARRAYSIZE(values), values, nullptr, nullptr};
	OpenMacroPluginInfo info = {MCT_KEYMACRO, &fmc};
	return CallMacroPlugin(&info);
}

bool KeyMacro::CheckEditSelected(FARMACROAREA Area, DWORD CurFlags)
{
	if (Area == MACROAREA_EDITOR || Area == MACROAREA_DIALOG || Area == MACROAREA_VIEWER ||
		(Area == MACROAREA_SHELL && CtrlObject->CmdLine->IsVisible()))
	{
		int NeedType = Area == MACROAREA_EDITOR ? MODALTYPE_EDITOR :
			(Area == MACROAREA_VIEWER ? MODALTYPE_VIEWER :
			(Area == MACROAREA_DIALOG ? MODALTYPE_DIALOG : MODALTYPE_PANELS));
		Frame* CurFrame = FrameManager->GetCurrentFrame();

		if (CurFrame && CurFrame->GetType()==NeedType)
		{
			int CurSelected;

			if (Area == MACROAREA_SHELL && CtrlObject->CmdLine->IsVisible())
				CurSelected=(int)CtrlObject->CmdLine->VMProcess(MCODE_C_SELECTED);
			else
				CurSelected=(int)CurFrame->VMProcess(MCODE_C_SELECTED);

			if (((CurFlags&MFLAGS_EDITSELECTION) && !CurSelected) ||	((CurFlags&MFLAGS_EDITNOSELECTION) && CurSelected))
				return false;
		}
	}

	return true;
}

bool KeyMacro::CheckCmdLine(int CmdLength, DWORD CurFlags)
{
	if (((CurFlags&MFLAGS_EMPTYCOMMANDLINE) && CmdLength) || ((CurFlags&MFLAGS_NOTEMPTYCOMMANDLINE) && CmdLength == 0))
		return false;

	return true;
}

bool KeyMacro::CheckPanel(int PanelMode, DWORD CurFlags, bool IsPassivePanel)
{
	if (IsPassivePanel)
	{
		if ((PanelMode == PLUGIN_PANEL && (CurFlags&MFLAGS_PNOPLUGINPANELS)) || (PanelMode == NORMAL_PANEL && (CurFlags&MFLAGS_PNOFILEPANELS)))
			return false;
	}
	else
	{
		if ((PanelMode == PLUGIN_PANEL && (CurFlags&MFLAGS_NOPLUGINPANELS)) || (PanelMode == NORMAL_PANEL && (CurFlags&MFLAGS_NOFILEPANELS)))
			return false;
	}

	return true;
}

bool KeyMacro::CheckFileFolder(DWORD CurFlags, bool IsPassivePanel)
{
	FARString strFileName;
	DWORD FileAttr = INVALID_FILE_ATTRIBUTES;
	Panel *SelPanel = SelectPanel(IsPassivePanel ? 1 : 0);
	SelPanel->GetFileName(strFileName, SelPanel->GetCurrentPos(), FileAttr);

	if (FileAttr != INVALID_FILE_ATTRIBUTES)
	{
		bool IsDir = (FileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0;
		return IsPassivePanel ?
			(IsDir ? !(CurFlags&MFLAGS_PNOFOLDERS) : !(CurFlags&MFLAGS_PNOFILES)) :
			(IsDir ? !(CurFlags&MFLAGS_NOFOLDERS) : !(CurFlags&MFLAGS_NOFILES));
	}

	return true;
}

bool KeyMacro::CheckAll(FARMACROAREA Area, DWORD CurFlags)
{
	// проверка на пусто/не пусто в ком.строке (а в редакторе? :-)
	if (CurFlags&(MFLAGS_EMPTYCOMMANDLINE|MFLAGS_NOTEMPTYCOMMANDLINE))
		if (CtrlObject->CmdLine && !CheckCmdLine(CtrlObject->CmdLine->GetLength(), CurFlags))
			return false;

	FilePanels *Cp = CtrlObject->Cp();

	if (!Cp)
		return false;

	// проверки панели и типа файла
	Panel *ActivePanel = Cp->ActivePanel;
	Panel *PassivePanel = Cp->GetAnotherPanel(Cp->ActivePanel);

	if (ActivePanel && PassivePanel)// && (CurFlags&MFLAGS_MODEMASK)==MACROAREA_SHELL)
	{
		if (CurFlags&(MFLAGS_NOPLUGINPANELS|MFLAGS_NOFILEPANELS))
			if (!CheckPanel(ActivePanel->GetMode(), CurFlags, false))
				return false;

		if (CurFlags&(MFLAGS_PNOPLUGINPANELS|MFLAGS_PNOFILEPANELS))
			if (!CheckPanel(PassivePanel->GetMode(), CurFlags, true))
				return false;

		if (CurFlags&(MFLAGS_NOFOLDERS|MFLAGS_NOFILES))
			if (!CheckFileFolder(CurFlags, false))
				return false;

		if (CurFlags&(MFLAGS_PNOFOLDERS|MFLAGS_PNOFILES))
			if (!CheckFileFolder(CurFlags, true))
				return false;

		if (CurFlags&(MFLAGS_SELECTION|MFLAGS_NOSELECTION|MFLAGS_PSELECTION|MFLAGS_PNOSELECTION))
			if (Area != MACROAREA_EDITOR && Area != MACROAREA_DIALOG && Area != MACROAREA_VIEWER)
			{
				int SelCount = ActivePanel->GetRealSelCount();

				if (((CurFlags&MFLAGS_SELECTION) && SelCount < 1) || ((CurFlags&MFLAGS_NOSELECTION) && SelCount >= 1))
					return false;

				SelCount = PassivePanel->GetRealSelCount();

				if (((CurFlags&MFLAGS_PSELECTION) && SelCount < 1) || ((CurFlags&MFLAGS_PNOSELECTION) && SelCount >= 1))
					return false;
			}
	}

	if (!CheckEditSelected(Area, CurFlags))
		return false;

	return true;
}

bool KeyMacro::CheckWaitKeyFunc() const
{
	return m_WaitKey != 0;
}
