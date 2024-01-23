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

#include "macro.hpp"
#include "tvar.hpp"
#include "macroopcode.hpp"
#include "keys.hpp"
#include "keyboard.hpp"
#include "lang.hpp"
#include "lockscrn.hpp"
#include "viewer.hpp"
#include "fileedit.hpp"
#include "fileview.hpp"
#include "dialog.hpp"
#include "dlgedit.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "cmdline.hpp"
#include "manager.hpp"
#include "scrbuf.hpp"
#include "udlist.hpp"
#include "filelist.hpp"
#include "treelist.hpp"
#include "TStack.hpp"
#include "syslog.hpp"
#include "ConfigRW.hpp"
#include "plugapi.hpp"
#include "plugins.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "grabber.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "xlat.hpp"
#include "datetime.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "drivemix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "constitle.hpp"
#include "dirmix.hpp"
#include "console.hpp"

int Log(const char* Format, ...)
{
	va_list valist;
	va_start(valist, Format);

	static int N = 0;
	const char* home = getenv("HOME");
	if (home) {
		char* buf = (char*) malloc(strlen(home) + 64);
		if (buf) {
			strcpy(buf, home);
			strcat(buf, "/luafar_log.txt");
			FILE* fp = fopen(buf, "a");
			if (fp) {
				if (++N == 1) {
					time_t rtime;
					time (&rtime);
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


static Frame* GetCurrentWindow()
{
	return FrameManager->GetTopModal();
}

typedef unsigned int MACROFLAGS_MFLAGS;
static const MACROFLAGS_MFLAGS
	MFLAGS_NONE                    = 0,
	// public flags, read from/saved to config
	MFLAGS_ENABLEOUTPUT            = (1 <<  0), // не подавлять обновление экрана во время выполнения макроса
	MFLAGS_NOSENDKEYSTOPLUGINS     = (1 <<  1), // НЕ передавать плагинам клавиши во время записи/воспроизведения макроса
	MFLAGS_RUNAFTERFARSTART        = (1 <<  3), // этот макрос запускается при старте ФАРа
	MFLAGS_EMPTYCOMMANDLINE        = (1 <<  4), // запускать, если командная линия пуста
	MFLAGS_NOTEMPTYCOMMANDLINE     = (1 <<  5), // запускать, если командная линия не пуста
	MFLAGS_EDITSELECTION           = (1 <<  6), // запускать, если есть выделение в редакторе
	MFLAGS_EDITNOSELECTION         = (1 <<  7), // запускать, если есть нет выделения в редакторе
	MFLAGS_SELECTION               = (1 <<  8), // активная:  запускать, если есть выделение
	MFLAGS_PSELECTION              = (1 <<  9), // пассивная: запускать, если есть выделение
	MFLAGS_NOSELECTION             = (1 << 10), // активная:  запускать, если есть нет выделения
	MFLAGS_PNOSELECTION            = (1 << 11), // пассивная: запускать, если есть нет выделения
	MFLAGS_NOFILEPANELS            = (1 << 12), // активная:  запускать, если это плагиновая панель
	MFLAGS_PNOFILEPANELS           = (1 << 13), // пассивная: запускать, если это плагиновая панель
	MFLAGS_NOPLUGINPANELS          = (1 << 14), // активная:  запускать, если это файловая панель
	MFLAGS_PNOPLUGINPANELS         = (1 << 15), // пассивная: запускать, если это файловая панель
	MFLAGS_NOFOLDERS               = (1 << 16), // активная:  запускать, если текущий объект "файл"
	MFLAGS_PNOFOLDERS              = (1 << 17), // пассивная: запускать, если текущий объект "файл"
	MFLAGS_NOFILES                 = (1 << 18), // активная:  запускать, если текущий объект "папка"
	MFLAGS_PNOFILES                = (1 << 19), // пассивная: запускать, если текущий объект "папка"
	// private flags, for runtime purposes only
	MFLAGS_POSTFROMPLUGIN          = (1 << 28); // последовательность пришла от АПИ

// для диалога назначения клавиши
struct DlgParam
{
	DWORD Flags;
	FarKey Key;
	int Area;
	int Recurse;
	bool Deleted;
};

enum ASSIGN_MACRO_KEY {
	AMK_CANCEL, AMK_MODIFY, AMK_DELETE
};

enum {
	OP_ISEXECUTING              = 1,
	OP_ISDISABLEOUTPUT          = 2,
	OP_HISTORYDISABLEMASK       = 3,
	OP_ISHISTORYDISABLE         = 4,
	OP_ISTOPMACROOUTPUTDISABLED = 5,
	OP_ISPOSTMACROENABLED       = 6,
	OP_SETMACROVALUE            = 8,
	OP_GETINPUTFROMMACRO        = 9,
	OP_GETLASTERROR             = 11,
};

FARString KeyMacro::m_RecCode;
FARString KeyMacro::m_RecDescription;

static bool ToDouble(long long v, double *d)
{
	if ((v >= 0 && v <= 0x1FFFFFFFFFFFFFLL) || (v < 0 && v >= -0x1FFFFFFFFFFFFFLL))
	{
		*d = (double)v;
		return true;
	}
	return false;
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
	FarMacroValue values[]={static_cast<double> (OpCode),Param};
	FarMacroCall fmc={sizeof(FarMacroCall),2,values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_KEYMACRO,&fmc};
	if (CallMacroPlugin(&info))
	{
		if (Ret) *Ret=info.Ret;
		return true;
	}
	return false;
}

int KeyMacro::GetExecutingState()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISEXECUTING,false,&Ret) ? Ret.ReturnType : static_cast<int>(MACROSTATE_NOMACRO);
}

bool KeyMacro::IsOutputDisabled()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISDISABLEOUTPUT,false,&Ret)? Ret.ReturnType != 0 : false;
}

static DWORD SetHistoryDisableMask(DWORD Mask)
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_HISTORYDISABLEMASK, static_cast<double>(Mask), &Ret)? Ret.ReturnType : 0;
}

static DWORD GetHistoryDisableMask()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_HISTORYDISABLEMASK,false,&Ret) ? Ret.ReturnType : 0;
}

bool KeyMacro::IsHistoryDisabled(int TypeHistory)
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISHISTORYDISABLE, static_cast<double>(TypeHistory), &Ret)? !!Ret.ReturnType : false;
}

static bool IsTopMacroOutputDisabled()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISTOPMACROOUTPUTDISABLED,false,&Ret) ? !!Ret.ReturnType : false;
}

static bool IsPostMacroEnabled()
{
	MacroPluginReturn Ret;
	return MacroPluginOp(OP_ISPOSTMACROENABLED,false,&Ret) && Ret.ReturnType==1;
}

static void SetMacroValue(bool Value)
{
	MacroPluginOp(OP_SETMACROVALUE, Value);
}

static bool TryToPostMacro(int Area,const FARString& TextKey,DWORD IntKey)
{
	FarMacroValue values[] = { 10.0, static_cast<double>(Area), TextKey.CPtr(), static_cast<double>(IntKey) };
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_KEYMACRO,&fmc};
	return CallMacroPlugin(&info);
}

static inline Panel* SelectPanel(int Type)
{
	Panel* ActivePanel = CtrlObject->Cp()->ActivePanel;
	Panel* PassivePanel = CtrlObject->Cp()->GetAnotherPanel(ActivePanel);
	return Type == 0 ? ActivePanel : (Type == 1 ? PassivePanel : nullptr);
}

KeyMacro::KeyMacro():
	m_Area(MACROAREA_SHELL),
	m_StartMode(MACROAREA_OTHER),
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

	FarMacroValue values[]={InitedRAM,false,0.0};
	if (Data)
	{
		if (Data->Path) values[1] = Data->Path;
		values[2] = static_cast<double>(Data->Flags);
	}
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_LOADMACROS,&fmc};
	return CallMacroPlugin(&info);
}

bool KeyMacro::SaveMacros(bool /*always*/)
{
	OpenMacroPluginInfo info={MCT_WRITEMACROS,nullptr};
	return CallMacroPlugin(&info);
}

int KeyMacro::GetState() const
{
	return (m_Recording != MACROSTATE_NOMACRO) ? m_Recording : GetExecutingState();
}

static bool GetInputFromMacro(MacroPluginReturn *mpr)
{
	return MacroPluginOp(OP_GETINPUTFROMMACRO,false,mpr);
}

void KeyMacro::RestoreMacroChar() const
{
	ScrBuf.RestoreMacroChar();

	if (m_Area==MACROAREA_EDITOR &&
					CtrlObject->Plugins.CurEditor &&
					CtrlObject->Plugins.CurEditor->IsVisible()
					/* && LockScr*/) // Mantis#0001595
	{
		CtrlObject->Plugins.CurEditor->Show();
	}
	else if (m_Area==MACROAREA_VIEWER &&
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
	FarMacroValue InValues[] = { static_cast<double>(Area),TextKey.CPtr(),UseCommon };
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(InValues),InValues,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_GETMACRO,&fmc};

	if (CallMacroPlugin(&info) && info.Ret.Count>=5)
	{
		const auto* Values = info.Ret.Values;
		Data->Area        = static_cast<FARMACROAREA>(static_cast<int>(Values[0].Double));
		Data->Code        = Values[1].Type==FMVT_STRING ? Values[1].String : L"";
		Data->Description = Values[2].Type==FMVT_STRING ? Values[2].String : L"";
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
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_RECORDEDMACRO,&fmc};
	CallMacroPlugin(&info);
}

bool KeyMacro::ParseMacroString(const wchar_t* Sequence, DWORD Flags, bool skipFile)
{
	const wchar_t* lang = GetMacroLanguage(Flags);
	const auto onlyCheck = (Flags&KMFLAGS_SILENTCHECK) != 0;

	// Перекладываем вывод сообщения об ошибке на плагин, т.к. штатный Message()
	// не умеет сворачивать строки и обрезает сообщение.
	FarMacroValue values[]={lang,Sequence,onlyCheck,skipFile};
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_MACROPARSE,&fmc};

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
	FarMacroValue values[]={GetMacroLanguage(Data->Flags), Data->SequenceText, FarMacroValue(Data->InValues,Data->InCount), onlyCheck};
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_EXECSTRING,&fmc};

	if (CallMacroPlugin(&info) && info.Ret.ReturnType == MPRT_NORMALFINISH)
	{
		Data->OutValues = info.Ret.Values;
		Data->OutCount = info.Ret.Count;
		return true;
	}
	Data->OutCount = 0;
	return false;
}

DWORD KeyMacro::GetMacroParseError(COORD& ErrPos, FARString& ErrSrc)
{
	MacroPluginReturn Ret;
	if (MacroPluginOp(OP_GETLASTERROR, false, &Ret))
	{
		ErrSrc = Ret.Values[0].String;
		ErrPos.Y = static_cast<int>(Ret.Values[1].Double);
		ErrPos.X = static_cast<int>(Ret.Values[2].Double);
		return ErrSrc.IsEmpty() ? MPEC_SUCCESS : MPEC_ERROR;
	}
	else
	{
		ErrSrc = L"No response from macro plugin";
		ErrPos = {};
		return MPEC_ERROR;
	}
}

class FarMacroApi
{
public:
	explicit FarMacroApi(FarMacroCall* Data) : mData(Data) {}

	std::vector<TVar> parseParams(size_t Count);
	int PassBoolean(int b);
	int PassError(const wchar_t* str);
	int PassInteger(int64_t Int);
	int PassNumber(double dbl);
	int PassString(const wchar_t* str);
	int PassString(const FARString& str);
	int PassValue(const TVar& Var);
	int PassBinary(const void* data, size_t size);
	int PassPointer(void* ptr);

	int absFunc();
	int ascFunc();
	int atoiFunc();
	int beepFunc();
	int chrFunc();                 //implemented in Lua
	int clipFunc();
	int dateFunc();
	int dlggetvalueFunc();
	int dlgsetfocusFunc();
	int editordellineFunc();
	int editorinsstrFunc();
	int editorposFunc();
	int editorselFunc();
	int editorsetFunc();
	int editorsetstrFunc();
	int editorsettitleFunc();
	int editorundoFunc();
	int environFunc();
	int farcfggetFunc();           //not implemented
	int farcfgsetFunc();           //not implemented
	int fargetconfigFunc();
	int fattrFunc();
	int fexistFunc();
	int floatFunc();
	int flockFunc();
	int fmatchFunc();
	int fsplitFunc();
	int indexFunc();
	int intFunc();
	int itowFunc();
	int kbdLayoutFunc();
	int keyFunc();
	int keybarshowFunc();
	int lcaseFunc();
	int lenFunc();
	int maxFunc();
	int menushowFunc();            //implemented in Lua (partially)
	int minFunc();
	int modFunc();
	int msgBoxFunc();
	int panelfattrFunc();
	int panelfexistFunc();
	int panelitemFunc();
	int panelselectFunc();
	int panelsetpathFunc();
	int panelsetposFunc();
	int panelsetposidxFunc();
	int pluginexistFunc();
	int pluginloadFunc();          //implemented in Lua
	int pluginunloadFunc();        //implemented in Lua
	int promptFunc();
	int replaceFunc();
	int rindexFunc();
	int size2strFunc();            //implemented in Lua
	int sleepFunc();
	int stringFunc();
	int strpadFunc();              //implemented in Lua
	int strwrapFunc();
	int substrFunc();
	int testfolderFunc();
	int trimFunc();
	int ucaseFunc();
	int waitkeyFunc();
	int windowscrollFunc();
	int xlatFunc();
	int UDList_Split();

private:
	int SendValue(FarMacroValue &Value);
	int fattrFuncImpl(int Type);

	FarMacroCall* mData;
};

int FarMacroApi::SendValue(FarMacroValue &Value)
{
	mData->Callback(mData->CallbackData, &Value, 1);
	return 0;
}

int FarMacroApi::PassString(const wchar_t *str)
{
	FarMacroValue val = NullToEmpty(str);
	return SendValue(val);
}

int FarMacroApi::PassString(const FARString& str)
{
	return PassString(str.CPtr());
}

int FarMacroApi::PassError(const wchar_t *str)
{
	FarMacroValue val = NullToEmpty(str);
	val.Type = FMVT_ERROR;
	return SendValue(val);
}

int FarMacroApi::PassNumber(double dbl)
{
	FarMacroValue val = dbl;
	return SendValue(val);
}

int FarMacroApi::PassInteger(int64_t Int)
{
	FarMacroValue val = Int;
	return SendValue(val);
}

int FarMacroApi::PassBoolean(int b)
{
	FarMacroValue val = (b != 0);
	return SendValue(val);
}

int FarMacroApi::PassValue(const TVar& Var)
{
	FarMacroValue val;
	double dd;

	if (Var.isDouble())
		val = Var.asDouble();
	else if (Var.isString())
		val = Var.s();
	else if (ToDouble(Var.asInteger(), &dd))
		val = dd;
	else
		val = Var.asInteger();

	return SendValue(val);
}

int FarMacroApi::PassBinary(const void* data, size_t size)
{
	FarMacroValue val;
	val.Type = FMVT_BINARY;
	val.Binary.Data = data;
	val.Binary.Size = size;
	return SendValue(val);
}

int FarMacroApi::PassPointer(void* ptr)
{
	FarMacroValue val(ptr);
	return SendValue(val);
}

std::vector<TVar> FarMacroApi::parseParams(size_t Count)
{
	auto argNum = std::min(mData->Count, Count);
	std::vector<TVar> Params;
	Params.reserve(Count);
	for (size_t i=0; i<argNum; i++)
	{
		const FarMacroValue& val = mData->Values[i];
		switch(val.Type)
		{
			case FMVT_INTEGER: Params.emplace_back(val.Integer); break;
			case FMVT_BOOLEAN: Params.emplace_back(val.Boolean); break;
			case FMVT_DOUBLE:  Params.emplace_back(val.Double);  break;
			case FMVT_STRING:  Params.emplace_back(val.String);  break;
			case FMVT_POINTER: Params.emplace_back((intptr_t)val.Pointer); break;
			default:           Params.emplace_back();            break;
		}
	}
	while (argNum++ < Count)
		Params.emplace_back();

	return Params;
}

static long long msValues[constMsLAST];

class LockOutput
{
public:
	LockOutput(bool Lock): m_Lock(Lock) { if (m_Lock) ScrBuf.Lock(); }
	~LockOutput() { if (m_Lock) ScrBuf.Unlock(); }

private:
	const bool m_Lock;
};

int KeyMacro::CallFar(int CheckCode, FarMacroCall* Data)
{
	int ret=0;
	DWORD FileAttr = INVALID_FILE_ATTRIBUTES;
	FarMacroApi api(Data);
	FARString tmpStr;

	// проверка на область
	if (CheckCode == 0)
	{
		return GetArea();
	}

	const auto ActivePanel = CtrlObject->Cp() ? CtrlObject->Cp()->ActivePanel : nullptr;
	const auto PassivePanel = CtrlObject->Cp() ? CtrlObject->Cp()->GetAnotherPanel(ActivePanel) : nullptr;

	auto CurrentWindow = FrameManager->GetCurrentFrame();

	switch (CheckCode)
	{
		case MCODE_C_MSX:             return api.PassNumber(msValues[constMsX]);
		case MCODE_C_MSY:             return api.PassNumber(msValues[constMsY]);
		case MCODE_C_MSBUTTON:        return api.PassNumber(msValues[constMsButton]);
		case MCODE_C_MSCTRLSTATE:     return api.PassNumber(msValues[constMsCtrlState]);
		case MCODE_C_MSEVENTFLAGS:    return api.PassNumber(msValues[constMsEventFlags]);
		case MCODE_C_MSLASTCTRLSTATE: return api.PassNumber(msValues[constMsLastCtrlState]);

		case MCODE_V_FAR_WIDTH:
			return ScrX + 1;

		case MCODE_V_FAR_HEIGHT:
			return ScrY + 1;

		case MCODE_V_FAR_TITLE:
			Console.GetTitle(tmpStr);
			return api.PassString(tmpStr);

		case MCODE_V_FAR_PID:
			return api.PassNumber(WINPORT(GetCurrentProcessId)());

		case MCODE_V_FAR_UPTIME:
			return api.PassNumber(GetProcessUptimeMSec());

		case MCODE_V_MACRO_AREA:
			return api.PassNumber(GetArea());

		case MCODE_C_FULLSCREENMODE: // Fullscreen?
			return api.PassBoolean(0);

		case MCODE_C_ISUSERADMIN:
			return api.PassBoolean(Opt.IsUserAdmin);

		case MCODE_V_DRVSHOWPOS:
			return Macro_DskShowPosType;

		case MCODE_V_DRVSHOWMODE: // Drv.ShowMode
			return Opt.ChangeDriveMode;

		case MCODE_C_CMDLINE_BOF:              // CmdLine.Bof - курсор в начале cmd-строки редактирования?
		case MCODE_C_CMDLINE_EOF:              // CmdLine.Eof - курсор в конце cmd-строки редактирования?
		case MCODE_C_CMDLINE_EMPTY:            // CmdLine.Empty
		case MCODE_C_CMDLINE_SELECTED:         // CmdLine.Selected
		{
			return api.PassBoolean(CtrlObject->CmdLine && CtrlObject->CmdLine->VMProcess(CheckCode));
		}

		case MCODE_V_CMDLINE_ITEMCOUNT:        // CmdLine.ItemCount
		case MCODE_V_CMDLINE_CURPOS:           // CmdLine.CurPos
		{
			return CtrlObject->CmdLine ? CtrlObject->CmdLine->VMProcess(CheckCode) : -1;
		}

		case MCODE_V_CMDLINE_VALUE:            // CmdLine.Value
		{
			if (CtrlObject->CmdLine)
				CtrlObject->CmdLine->GetString(tmpStr);
			return api.PassString(tmpStr);
		}

		case MCODE_C_APANEL_ROOT:  // APanel.Root
		case MCODE_C_PPANEL_ROOT:  // PPanel.Root
		{
			const auto SelPanel = (CheckCode == MCODE_C_APANEL_ROOT) ? ActivePanel:PassivePanel;
			return api.PassBoolean((SelPanel ? SelPanel->VMProcess(MCODE_C_ROOTFOLDER) ? 1:0:0));
		}

		case MCODE_C_APANEL_BOF:
		case MCODE_C_PPANEL_BOF:
		case MCODE_C_APANEL_EOF:
		case MCODE_C_PPANEL_EOF:
		{
			const auto SelPanel = (CheckCode == MCODE_C_APANEL_BOF || CheckCode == MCODE_C_APANEL_EOF)?ActivePanel:PassivePanel;
			if (SelPanel)
				ret=SelPanel->VMProcess(CheckCode==MCODE_C_APANEL_BOF || CheckCode==MCODE_C_PPANEL_BOF?MCODE_C_BOF:MCODE_C_EOF)?1:0;
			return api.PassBoolean(ret);
		}

		case MCODE_C_SELECTED:    // Selected?
		{
			int NeedType = m_Area == MACROAREA_EDITOR ? MODALTYPE_EDITOR :
				(m_Area == MACROAREA_VIEWER ? MODALTYPE_VIEWER :
				(m_Area == MACROAREA_DIALOG ? MODALTYPE_DIALOG : MODALTYPE_PANELS));

			if (!(m_Area == MACROAREA_USERMENU || m_Area == MACROAREA_MAINMENU || m_Area == MACROAREA_MENU) && CurrentWindow && CurrentWindow->GetType()==NeedType)
			{
				int CurSelected;

				if (m_Area==MACROAREA_SHELL && CtrlObject->CmdLine->IsVisible())
					CurSelected=(int)CtrlObject->CmdLine->VMProcess(CheckCode);
				else
					CurSelected=(int)CurrentWindow->VMProcess(CheckCode);

				return api.PassBoolean(CurSelected);
			}
			else
			{
				auto f = GetCurrentWindow();
				if (f)
					ret = f->VMProcess(CheckCode);
			}
			return api.PassBoolean(ret);
		}

		case MCODE_C_EMPTY:   // Empty
		case MCODE_C_BOF:
		case MCODE_C_EOF:
		{
			if (!(m_Area == MACROAREA_USERMENU || m_Area == MACROAREA_MAINMENU || m_Area == MACROAREA_MENU)
				&& CurrentWindow && CurrentWindow->GetType() == MODALTYPE_PANELS
				&& !(m_Area == MACROAREA_INFOPANEL || m_Area == MACROAREA_QVIEWPANEL || m_Area == MACROAREA_TREEPANEL))
			{
				if (CheckCode == MCODE_C_EMPTY)
					ret=CtrlObject->CmdLine->GetLength()?0:1;
				else
					ret=CtrlObject->CmdLine->VMProcess(CheckCode);
			}
			else
			{
				auto f = GetCurrentWindow();
				if (f)
					ret = f->VMProcess(CheckCode);
			}
			return api.PassBoolean(ret);
		}

		case MCODE_F_GETOPTIONS:
		{
			DWORD Options = 0;
			if (Opt.OnlyEditorViewerUsed)              Options |= 0x1;
			if (Opt.Macro.DisableMacro&MDOL_ALL)       Options |= 0x4;
			if (Opt.Macro.DisableMacro&MDOL_AUTOSTART) Options |= 0x8;
			//### if (Opt.ReadOnlyConfig)                    Options |= 0x10;
			api.PassNumber(Options);
			break;
		}

		case MCODE_F_KEYMACRO:
			if (Data->Count && Data->Values[0].Type==FMVT_DOUBLE)
			{
				switch (static_cast<int>(Data->Values[0].Double))
				{
					case 1: RestoreMacroChar(); break;
					case 2: ScrBuf.Lock(); break;
					case 3: ScrBuf.Unlock(); break;
					case 4: ScrBuf.ResetLockCount(); break;
					case 5: api.PassNumber(ScrBuf.GetLockCount()); break;
					case 6: if (Data->Count > 1) ScrBuf.SetLockCount(Data->Values[1].Double); break;
					case 7: api.PassBoolean(Clipboard::GetUseInternalClipboardState()); break;
					case 8: if (Data->Count > 1) Clipboard::SetUseInternalClipboardState(Data->Values[1].Boolean != 0); break;
					case 9: if (Data->Count > 1) api.PassNumber(KeyNameToKey(Data->Values[1].String)); break;
					case 10:
						if (Data->Count > 1)
						{
							FARString str;
							KeyToText(Data->Values[1].Double, str);
							api.PassString(str.CPtr());
						}
						break;
				}
			}
			break;

		case MCODE_V_DLGITEMCOUNT: // Dlg.ItemCount
		case MCODE_V_DLGCURPOS:    // Dlg.CurPos
		case MCODE_V_DLGITEMTYPE:  // Dlg.ItemType
		case MCODE_V_DLGPREVPOS:   // Dlg.PrevPos
		case MCODE_V_DLGINFOOWNER: // Dlg.Owner
		{
			if (CurrentWindow && CurrentWindow->GetType()==MODALTYPE_DIALOG) // ?? Mode == MACROAREA_DIALOG ??
				return CurrentWindow->VMProcess(CheckCode);
			break;
		}

		case MCODE_V_DLGINFOID:      // Dlg->Info.Id
		if (CurrentWindow && CurrentWindow->GetType()==MODALTYPE_DIALOG) // ?? Mode == MACROAREA_DIALOG ??
		{
			api.PassString( reinterpret_cast<LPCWSTR>(CurrentWindow->VMProcess(CheckCode)) );
		}
		break;

		case MCODE_C_APANEL_VISIBLE:  // APanel.Visible
		case MCODE_C_PPANEL_VISIBLE:  // PPanel.Visible
		{
			const auto SelPanel = CheckCode == MCODE_C_APANEL_VISIBLE?ActivePanel:PassivePanel;
			return api.PassBoolean(SelPanel && SelPanel->IsVisible());
		}

		case MCODE_C_APANEL_ISEMPTY: // APanel.Empty
		case MCODE_C_PPANEL_ISEMPTY: // PPanel.Empty
		{
			Panel *SelPanel=CheckCode==MCODE_C_APANEL_ISEMPTY?ActivePanel:PassivePanel;
			if (SelPanel)
			{
				SelPanel->GetFileName(tmpStr,SelPanel->GetCurrentPos(),FileAttr);
				size_t GetFileCount=SelPanel->GetFileCount();
				ret=(!GetFileCount || (GetFileCount == 1 && TestParentFolderName(tmpStr))) ? 1:0;
			}
			return api.PassBoolean(ret);
		}

		case MCODE_C_APANEL_FILTER:
		case MCODE_C_PPANEL_FILTER:
		{
			const auto SelPanel = (CheckCode == MCODE_C_APANEL_FILTER)?ActivePanel:PassivePanel;
			return api.PassBoolean(SelPanel && SelPanel->VMProcess(MCODE_C_APANEL_FILTER));
		}

		case MCODE_C_APANEL_LEFT: // APanel.Left
		case MCODE_C_PPANEL_LEFT: // PPanel.Left
		{
			const auto SelPanel = CheckCode == MCODE_C_APANEL_LEFT ? ActivePanel : PassivePanel;
			return api.PassBoolean(SelPanel && SelPanel==CtrlObject->Cp()->LeftPanel);
		}

		case MCODE_C_APANEL_FILEPANEL: // APanel.FilePanel
		case MCODE_C_PPANEL_FILEPANEL: // PPanel.FilePanel
		{
			const auto SelPanel = CheckCode == MCODE_C_APANEL_FILEPANEL ? ActivePanel : PassivePanel;
			return api.PassBoolean(SelPanel && SelPanel->GetType() == FILE_PANEL);
		}

		case MCODE_C_APANEL_PLUGIN: // APanel.Plugin
		case MCODE_C_PPANEL_PLUGIN: // PPanel.Plugin
		{
			const auto SelPanel = CheckCode == MCODE_C_APANEL_PLUGIN?ActivePanel:PassivePanel;
			return api.PassBoolean(SelPanel && SelPanel->GetMode() == PLUGIN_PANEL);
		}

		case MCODE_C_APANEL_FOLDER: // APanel.Folder
		case MCODE_C_PPANEL_FOLDER: // PPanel.Folder
		{
			const auto SelPanel = CheckCode == MCODE_C_APANEL_FOLDER?ActivePanel:PassivePanel;
			if (SelPanel)
			{
				SelPanel->GetFileName(tmpStr, SelPanel->GetCurrentPos(), FileAttr);

				if (FileAttr != INVALID_FILE_ATTRIBUTES)
					ret=(FileAttr&FILE_ATTRIBUTE_DIRECTORY)?1:0;
			}
			return api.PassBoolean(ret);
		}

		case MCODE_C_APANEL_SELECTED: // APanel.Selected
		case MCODE_C_PPANEL_SELECTED: // PPanel.Selected
		{
			const auto SelPanel = CheckCode == MCODE_C_APANEL_SELECTED?ActivePanel:PassivePanel;
			return api.PassBoolean(SelPanel && SelPanel->GetRealSelCount() > 0);
		}

		case MCODE_V_APANEL_CURRENT: // APanel.Current
		case MCODE_V_PPANEL_CURRENT: // PPanel.Current
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_CURRENT ? ActivePanel : PassivePanel;
			if (SelPanel)
			{
				SelPanel->GetFileName(tmpStr, SelPanel->GetCurrentPos(), FileAttr);
			}
			return api.PassString(tmpStr);
		}

		case MCODE_V_APANEL_SELCOUNT: // APanel.SelCount
		case MCODE_V_PPANEL_SELCOUNT: // PPanel.SelCount
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_SELCOUNT ? ActivePanel : PassivePanel;
			return SelPanel ? SelPanel->GetRealSelCount() : 0;
		}

		case MCODE_V_APANEL_COLUMNCOUNT:       // APanel.ColumnCount - активная панель:  количество колонок
		case MCODE_V_PPANEL_COLUMNCOUNT:       // PPanel.ColumnCount - пассивная панель: количество колонок
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_COLUMNCOUNT ? ActivePanel : PassivePanel;
			return SelPanel ? SelPanel->GetColumnsCount() : 0;
		}

		case MCODE_V_APANEL_WIDTH: // APanel.Width
		case MCODE_V_PPANEL_WIDTH: // PPanel.Width
		case MCODE_V_APANEL_HEIGHT: // APanel.Height
		case MCODE_V_PPANEL_HEIGHT: // PPanel.Height
		{
			Panel *SelPanel = CheckCode == MCODE_V_APANEL_WIDTH || CheckCode == MCODE_V_APANEL_HEIGHT? ActivePanel : PassivePanel;
			if (SelPanel )
			{
				int X1, Y1, X2, Y2;
				SelPanel->GetPosition(X1,Y1,X2,Y2);

				if (CheckCode == MCODE_V_APANEL_HEIGHT || CheckCode == MCODE_V_PPANEL_HEIGHT)
					ret = Y2-Y1+1;
				else
					ret = X2-X1+1;
			}
			return ret;
		}

		case MCODE_V_APANEL_OPIFLAGS:  // APanel.OPIFlags
		case MCODE_V_PPANEL_OPIFLAGS:  // PPanel.OPIFlags
		case MCODE_V_APANEL_HOSTFILE: // APanel.HostFile
		case MCODE_V_PPANEL_HOSTFILE: // PPanel.HostFile
		case MCODE_V_APANEL_FORMAT:           // APanel.Format
		case MCODE_V_PPANEL_FORMAT:           // PPanel.Format
		{
			const auto SelPanel =
				CheckCode == MCODE_V_APANEL_OPIFLAGS ||
				CheckCode == MCODE_V_APANEL_HOSTFILE ||
				CheckCode == MCODE_V_APANEL_FORMAT? ActivePanel : PassivePanel;

			const wchar_t* ptr = nullptr;
			if (CheckCode == MCODE_V_APANEL_HOSTFILE || CheckCode == MCODE_V_PPANEL_HOSTFILE ||
				CheckCode == MCODE_V_APANEL_FORMAT || CheckCode == MCODE_V_PPANEL_FORMAT)
				ptr = L"";

			if (SelPanel )
			{
				if (SelPanel->GetMode() == PLUGIN_PANEL)
				{
					OpenPluginInfo Info={};
					Info.StructSize=sizeof(OpenPluginInfo);
					SelPanel->GetOpenPluginInfo(&Info);
					switch (CheckCode)
					{
						case MCODE_V_APANEL_OPIFLAGS:
						case MCODE_V_PPANEL_OPIFLAGS:
							return Info.Flags;
						case MCODE_V_APANEL_HOSTFILE:
						case MCODE_V_PPANEL_HOSTFILE:
							return api.PassString(Info.HostFile);
						case MCODE_V_APANEL_FORMAT:
						case MCODE_V_PPANEL_FORMAT:
							return api.PassString(Info.Format);
					}
				}
			}

			return ptr ? api.PassString(ptr) : 0;
		}

		case MCODE_V_APANEL_PREFIX:           // APanel.Prefix
		case MCODE_V_PPANEL_PREFIX:           // PPanel.Prefix
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_PREFIX ? ActivePanel : PassivePanel;
			const wchar_t *ptr = L"";
			if (SelPanel)
			{
				PluginInfo PInfo = {sizeof(PInfo)};
				if (SelPanel->VMProcess(MCODE_V_APANEL_PREFIX,&PInfo))
					ptr = PInfo.CommandPrefix;
			}
			return api.PassString(ptr);
		}

		case MCODE_V_APANEL_PATH0:           // APanel.Path0
		case MCODE_V_PPANEL_PATH0:           // PPanel.Path0
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_PATH0? ActivePanel : PassivePanel;
			if (SelPanel)
			{
				SelPanel->GetCurDir(tmpStr);
			}
			return api.PassString(tmpStr);
		}

		case MCODE_V_APANEL_PATH: // APanel.Path
		case MCODE_V_PPANEL_PATH: // PPanel.Path
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_PATH? ActivePanel : PassivePanel;
			if (SelPanel)
			{
				if (SelPanel->GetMode() == PLUGIN_PANEL)
				{
					OpenPluginInfo Info={};
					Info.StructSize=sizeof(OpenPluginInfo);
					SelPanel->GetOpenPluginInfo(&Info);
					tmpStr = NullToEmpty(Info.CurDir);
				}
				else
					SelPanel->GetCurDir(tmpStr);

				DeleteEndSlash(tmpStr); // - чтобы у корня диска было C:, тогда можно писать так: APanel.Path + "\\file"
			}
			return api.PassString(tmpStr);
		}

		case MCODE_V_APANEL_UNCPATH: // APanel.UNCPath
		case MCODE_V_PPANEL_UNCPATH: // PPanel.UNCPath
		{
			const wchar_t *ptr = L"";
			if (_MakePath1(CheckCode == MCODE_V_APANEL_UNCPATH?KEY_ALTSHIFTBRACKET:KEY_ALTSHIFTBACKBRACKET,tmpStr,L""))
			{
				UnquoteExternal(tmpStr);
				DeleteEndSlash(tmpStr);
				ptr = tmpStr.CPtr();
			}
			return api.PassString(ptr);
		}

		//FILE_PANEL,TREE_PANEL,QVIEW_PANEL,INFO_PANEL
		case MCODE_V_APANEL_TYPE: // APanel.Type
		case MCODE_V_PPANEL_TYPE: // PPanel.Type
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_TYPE ? ActivePanel : PassivePanel;
			return SelPanel? SelPanel->GetType() : FILE_PANEL;
		}

		case MCODE_V_APANEL_DRIVETYPE: // APanel.DriveType - активная панель: тип привода
		case MCODE_V_PPANEL_DRIVETYPE: // PPanel.DriveType - пассивная панель: тип привода
		{
			Panel *SelPanel = CheckCode == MCODE_V_APANEL_DRIVETYPE ? ActivePanel : PassivePanel;
			ret=-1;

			if (SelPanel  && SelPanel->GetMode() != PLUGIN_PANEL)
			{
				SelPanel->GetCurDir(tmpStr);
				UINT DriveType=FAR_GetDriveType(tmpStr, 0);
				ret = DriveType;
			}
			return ret;
		}

		case MCODE_V_APANEL_ITEMCOUNT: // APanel.ItemCount
		case MCODE_V_PPANEL_ITEMCOUNT: // PPanel.ItemCount
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_ITEMCOUNT ? ActivePanel : PassivePanel;
			return SelPanel ? SelPanel->GetFileCount() : 0;
		}

		case MCODE_V_APANEL_CURPOS: // APanel.CurPos
		case MCODE_V_PPANEL_CURPOS: // PPanel.CurPos
		{
			const auto SelPanel = CheckCode == MCODE_V_APANEL_CURPOS ? ActivePanel : PassivePanel;
			return SelPanel ? SelPanel->GetCurrentPos()+(SelPanel->GetFileCount()>0?1:0) : 0;
		}

		case MCODE_V_TITLE: // Title
		{
			Frame *f=FrameManager->GetTopModal();
			if (f)
			{
				if (CtrlObject->Cp() == f)
				{
					ActivePanel->GetTitle(tmpStr);
				}
				else
				{
					FARString strType;
					switch (f->GetTypeAndName(strType,tmpStr))
					{
						case MODALTYPE_EDITOR:
						case MODALTYPE_VIEWER:
							f->GetTitle(tmpStr);
							break;
					}
				}
				RemoveExternalSpaces(tmpStr);
			}
			return api.PassString(tmpStr);
		}

		case MCODE_V_HEIGHT:  // Height - высота текущего объекта
		case MCODE_V_WIDTH:   // Width - ширина текущего объекта
		{
			Frame *f = FrameManager->GetTopModal();

			if (f)
			{
				int X1, Y1, X2, Y2;
				f->GetPosition(X1,Y1,X2,Y2);

				if (CheckCode == MCODE_V_HEIGHT)
					ret = Y2-Y1+1;
				else
					ret = X2-X1+1;
			}

			return ret;
		}

		case MCODE_V_MENU_VALUE: // Menu.Value
		{
			const wchar_t *ptr = L"";
			int CurArea = GetArea();
			auto f = GetCurrentWindow();

			if (f && (IsMenuArea(CurArea) || CurArea == MACROAREA_DIALOG))
			{
				FARString NewStr;
				if (f->VMProcess(CheckCode,&NewStr))
				{
					HiText2Str(tmpStr, NewStr);
					RemoveExternalSpaces(tmpStr);
					ptr = tmpStr.CPtr();
				}
			}
			return api.PassString(ptr);
		}

		case MCODE_V_MENUINFOID: // Menu.Id
		{
			auto f = GetCurrentWindow();

			if (f && f->GetType()==MODALTYPE_VMENU)
			{
				return api.PassString( reinterpret_cast<LPCWSTR>(f->VMProcess(CheckCode)) );
			}
			return api.PassString(L"");
		}

		case MCODE_V_ITEMCOUNT: // ItemCount - число элементов в текущем объекте
		case MCODE_V_CURPOS: // CurPos - текущий индекс в текущем объекте
		{
			auto f = GetCurrentWindow();
			if (f)
			{
				ret=f->VMProcess(CheckCode);
			}
			return ret;
		}

		case MCODE_V_EDITORCURLINE: // Editor.CurLine - текущая линия в редакторе (в дополнении к Count)
		case MCODE_V_EDITORSTATE:   // Editor.State
		case MCODE_V_EDITORLINES:   // Editor.Lines
		case MCODE_V_EDITORCURPOS:  // Editor.CurPos
		case MCODE_V_EDITORREALPOS: // Editor.RealPos
		case MCODE_V_EDITORFILENAME: // Editor.FileName
		case MCODE_V_EDITORSELVALUE: // Editor.SelValue
		{
			const wchar_t* ptr = nullptr;
			if (CheckCode == MCODE_V_EDITORSELVALUE)
				ptr=L"";

			if (GetArea()==MACROAREA_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
			{
				if (CheckCode == MCODE_V_EDITORFILENAME)
				{
					FARString strType;
					CtrlObject->Plugins.CurEditor->GetTypeAndName(strType, tmpStr);
					ptr=tmpStr.CPtr();
				}
				else if (CheckCode == MCODE_V_EDITORSELVALUE)
				{
					CtrlObject->Plugins.CurEditor->VMProcess(CheckCode,&tmpStr);
					ptr=tmpStr.CPtr();
				}
				else
					return CtrlObject->Plugins.CurEditor->VMProcess(CheckCode);
			}
			return ptr ? api.PassString(ptr) : 0;
		}

		case MCODE_V_HELPFILENAME:  // Help.FileName
		case MCODE_V_HELPTOPIC:     // Help.Topic
		case MCODE_V_HELPSELTOPIC:  // Help.SelTopic
		{
			const wchar_t *ptr=L"";
			if (CtrlObject->Macro.GetArea() == MACROAREA_HELP)
			{
				FrameManager->GetCurrentFrame()->VMProcess(CheckCode,&tmpStr,0);
				ptr=tmpStr.CPtr();
			}
			return api.PassString(ptr);
		}

		case MCODE_V_VIEWERFILENAME: // Viewer.FileName
		case MCODE_V_VIEWERSTATE: // Viewer.State
		{
			if ((GetArea()==MACROAREA_VIEWER || GetArea()==MACROAREA_QVIEWPANEL) &&
				CtrlObject->Plugins.CurViewer && CtrlObject->Plugins.CurViewer->IsVisible())
			{
				if (CheckCode == MCODE_V_VIEWERFILENAME)
				{
					CtrlObject->Plugins.CurViewer->GetFileName(tmpStr);
					return api.PassString(tmpStr);
				}
				else
					return api.PassNumber(CtrlObject->Plugins.CurViewer->VMProcess(MCODE_V_VIEWERSTATE));
			}
			return (CheckCode == MCODE_V_VIEWERFILENAME) ? api.PassString(L"") : 0;
		}

		//case MCODE_F_BEEP:             not_implemented;
		//case MCODE_F_EDITOR_DELLINE:   implemented_in_lua;
		//case MCODE_F_EDITOR_INSSTR:    implemented_in_lua;
		//case MCODE_F_EDITOR_SETSTR:    implemented_in_lua;
		//case MCODE_F_MENU_SHOW:        implemented_in_lua_partially;
		//case MCODE_F_USERMENU:         not_implemented;

		case MCODE_F_FAR_CFG_GET:        return api.farcfggetFunc();
		case MCODE_F_FAR_CFG_SET:        return api.farcfgsetFunc();

		case MCODE_F_SETCUSTOMSORTMODE:
			if (Data->Count>=3 && Data->Values[0].Type==FMVT_DOUBLE  &&
				Data->Values[1].Type==FMVT_DOUBLE && Data->Values[2].Type==FMVT_BOOLEAN)
			{
				auto panel = CtrlObject->Cp()->ActivePanel;
				if (panel && Data->Values[0].Double == 1)
					panel = CtrlObject->Cp()->GetAnotherPanel(panel);

				if (panel)
				{
					int SortMode = (int)Data->Values[1].Double;
					bool InvertByDefault = Data->Values[2].Boolean != 0;
					sort_order Order = sort_order::first;
					if (Data->Count>=4 && Data->Values[3].Type==FMVT_DOUBLE)
					{
						switch (static_cast<int>(Data->Values[3].Double))
						{
							default:
							case 0: Order = sort_order::first;   break;
							case 1: Order = sort_order::keep;    break;
							case 2: Order = sort_order::ascend;  break;
							case 3: Order = sort_order::descend; break;
						}
					}
					panel->SetCustomSortMode(SortMode, Order, InvertByDefault);
				}
			}
			break;

		case MCODE_F_CHECKALL:
		{
			auto Result = false;
			if (Data->Count >= 2)
			{
				const auto Area = static_cast<int>(Data->Values[0].Double);
				const auto Flags = static_cast<DWORD>(Data->Values[1].Double);
				const auto Callback = (Data->Count >= 3 && Data->Values[2].Type == FMVT_POINTER)? reinterpret_cast<FARMACROCALLBACK>(Data->Values[2].Pointer) : nullptr;
				const auto CallbackId = (Data->Count >= 4 && Data->Values[3].Type == FMVT_POINTER)? Data->Values[3].Pointer : nullptr;
				Result = CheckAll(Area, Flags) && (!Callback || Callback(CallbackId, AKMFLAGS_NONE));
			}
			return api.PassBoolean(Result);
		}

		case MCODE_F_MACROSETTINGS:
			if (Data->Count>=4 && Data->Values[0].Type==FMVT_STRING && Data->Values[1].Type==FMVT_DOUBLE
				&& Data->Values[2].Type==FMVT_STRING && Data->Values[3].Type==FMVT_STRING)
			{
				const auto Key = KeyNameToKey(Data->Values[0].String);
				auto Flags = static_cast<DWORD>(Data->Values[1].Double);
				const auto Src = Data->Values[2].String;
				const auto Descr = Data->Values[3].String;
				if (Key && GetMacroSettings(Key, Flags, Src, Descr))
				{
					api.PassNumber(static_cast<double>(Flags));
					api.PassString(m_RecCode);
					api.PassString(m_RecDescription);
					return 0;
				}
			}
			api.PassBoolean(false);
			break;

		case MCODE_F_PLUGIN_CALL:
			if(Data->Count>=2 && Data->Values[0].Type==FMVT_BOOLEAN && Data->Values[1].Type==FMVT_DOUBLE)
			{
				bool SyncCall = (Data->Values[0].Boolean == 0);
				DWORD SysID = (DWORD)Data->Values[1].Double;
				if (CtrlObject->Plugins.FindPlugin(SysID))
				{
					FarMacroValue *Values = Data->Count>2 ? Data->Values+2:nullptr;
					OpenMacroInfo info={sizeof(OpenMacroInfo),Data->Count-2,Values};
					void *ResultCallPlugin = nullptr;

					if (SyncCall) m_InternalInput++;

					if (!CtrlObject->Plugins.CallPlugin(SysID, OPEN_FROMMACRO, &info, &ResultCallPlugin))
						ResultCallPlugin = nullptr;

					if (SyncCall) m_InternalInput--;

					//в windows гарантируется, что не бывает указателей меньше 0x10000
					if (reinterpret_cast<uintptr_t>(ResultCallPlugin) >= 0x10000 && ResultCallPlugin != INVALID_HANDLE_VALUE)
						api.PassPointer(ResultCallPlugin);
					else
						api.PassBoolean(ResultCallPlugin != nullptr && ResultCallPlugin != INVALID_HANDLE_VALUE);

					return 0;
				}
			}
			return api.PassBoolean(false);

		case MCODE_F_PLUGIN_EXIST:       return api.pluginexistFunc();
		case MCODE_F_ABS:                return api.absFunc();
		case MCODE_F_ASC:                return api.ascFunc();
		case MCODE_F_ATOI:               return api.atoiFunc();
		case MCODE_F_CLIP:               return api.clipFunc();
		case MCODE_F_DATE:               return api.dateFunc();
		case MCODE_F_DLG_GETVALUE:       return api.dlggetvalueFunc();
		case MCODE_F_DLG_SETFOCUS:       return api.dlgsetfocusFunc();
		case MCODE_F_EDITOR_POS:
		{
			LockOutput Lock(IsTopMacroOutputDisabled());
			return api.editorposFunc();
		}
		case MCODE_F_EDITOR_SEL:         return api.editorselFunc();
		case MCODE_F_EDITOR_SET:         return api.editorsetFunc();
		case MCODE_F_EDITOR_SETTITLE:    return api.editorsettitleFunc();
		case MCODE_F_EDITOR_UNDO:        return api.editorundoFunc();
		case MCODE_F_ENVIRON:            return api.environFunc();
		case MCODE_F_FATTR:              return api.fattrFunc();
		case MCODE_F_FEXIST:             return api.fexistFunc();
		case MCODE_F_FLOAT:              return api.floatFunc();
		case MCODE_F_FLOCK:              return api.flockFunc();
		case MCODE_F_FMATCH:             return api.fmatchFunc();
		case MCODE_F_FSPLIT:             return api.fsplitFunc();
		case MCODE_F_INDEX:              return api.indexFunc();
		case MCODE_F_INT:                return api.intFunc();
		case MCODE_F_ITOA:               return api.itowFunc();
		case MCODE_F_KBDLAYOUT:          return api.kbdLayoutFunc();
		case MCODE_F_KEY:                return api.keyFunc();
		case MCODE_F_KEYBAR_SHOW:        return api.keybarshowFunc();
		case MCODE_F_LCASE:              return api.lcaseFunc();
		case MCODE_F_LEN:                return api.lenFunc();
		case MCODE_F_MAX:                return api.maxFunc();
		case MCODE_F_MIN:                return api.minFunc();
		case MCODE_F_MOD:                return api.modFunc();
		case MCODE_F_MSGBOX:             return api.msgBoxFunc();
		case MCODE_F_PANEL_FATTR:        return api.panelfattrFunc();
		case MCODE_F_PANEL_FEXIST:       return api.panelfexistFunc();
		case MCODE_F_PANELITEM:          return api.panelitemFunc();
		case MCODE_F_PANEL_SELECT:       return api.panelselectFunc();
		case MCODE_F_PANEL_SETPATH:      return api.panelsetpathFunc();
		case MCODE_F_PANEL_SETPOS:       return api.panelsetposFunc();
		case MCODE_F_PANEL_SETPOSIDX:    return api.panelsetposidxFunc();
		case MCODE_F_PROMPT:             return api.promptFunc();
		case MCODE_F_REPLACE:            return api.replaceFunc();
		case MCODE_F_RINDEX:             return api.rindexFunc();
		case MCODE_F_SLEEP:              return api.sleepFunc();
		case MCODE_F_STRING:             return api.stringFunc();
		case MCODE_F_STRWRAP:            return api.strwrapFunc();
		case MCODE_F_SUBSTR:             return api.substrFunc();
		case MCODE_F_TESTFOLDER:         return api.testfolderFunc();
		case MCODE_F_TRIM:               return api.trimFunc();
		case MCODE_F_UCASE:              return api.ucaseFunc();
		case MCODE_F_WAITKEY:
		{
			LockOutput Lock(IsTopMacroOutputDisabled());

			++m_WaitKey;
			int result = api.waitkeyFunc();
			--m_WaitKey;

			return result;
		}
		case MCODE_F_WINDOW_SCROLL:      return api.windowscrollFunc();
		case MCODE_F_XLAT:               return api.xlatFunc();
		case MCODE_F_FAR_GETCONFIG:      return api.fargetconfigFunc();

		case MCODE_F_BM_ADD:              // N=BM.Add()
		case MCODE_F_BM_CLEAR:            // N=BM.Clear()
		case MCODE_F_BM_NEXT:             // N=BM.Next()
		case MCODE_F_BM_PREV:             // N=BM.Prev()
		case MCODE_F_BM_BACK:             // N=BM.Back()
		case MCODE_F_BM_STAT:             // N=BM.Stat([N])
		case MCODE_F_BM_DEL:              // N=BM.Del([Idx]) - удаляет закладку с указанным индексом (x=1...), 0 - удаляет текущую закладку
		case MCODE_F_BM_GET:              // N=BM.Get(Idx,M) - возвращает координаты строки (M==0) или колонки (M==1) закладки с индексом (Idx=1...)
		case MCODE_F_BM_GOTO:             // N=BM.Goto([n]) - переход на закладку с указанным индексом (0 --> текущую)
		case MCODE_F_BM_PUSH:             // N=BM.Push() - сохранить текущую позицию в виде закладки в конце стека
		case MCODE_F_BM_POP:              // N=BM.Pop() - восстановить текущую позицию из закладки в конце стека и удалить закладку
		{
			auto Params = api.parseParams(2);
			auto& p1 = Params[0];
			auto& p2 = Params[1];
			int Result = 0;

			auto f = GetCurrentWindow();
			if (f)
				Result = f->VMProcess(CheckCode,(void*)(LONG_PTR)p2.i(),p1.i());

			return Result;
		}

		case MCODE_F_MENU_ITEMSTATUS:     // N=Menu.ItemStatus([N])
		case MCODE_F_MENU_GETVALUE:       // S=Menu.GetValue([N])
		case MCODE_F_MENU_GETHOTKEY:      // S=gethotkey([N])
		{
			auto Params = api.parseParams(1);
			auto MenuItemPos = Params[0].toInteger() - 1;

			TVar Out = L"";
			int CurMMode = GetArea();

			if (IsMenuArea(CurMMode) || CurMMode == MACROAREA_DIALOG)
			{
				auto f = GetCurrentWindow();
				if (f)
				{
					if (CheckCode == MCODE_F_MENU_GETHOTKEY)
					{
						int64_t Result;
						if ((Result=f->VMProcess(CheckCode,nullptr,MenuItemPos)) )
						{
							const wchar_t _value[]={static_cast<wchar_t>(Result),0};
							Out=_value;
						}
					}
					else if (CheckCode == MCODE_F_MENU_GETVALUE)
					{
						FARString NewStr;
						if (f->VMProcess(CheckCode,&NewStr,MenuItemPos))
						{
							FARString tmpStr = NewStr;
							HiText2Str(tmpStr, NewStr);
							RemoveExternalSpaces(tmpStr);
							Out=tmpStr.CPtr();
						}
					}
					else if (CheckCode == MCODE_F_MENU_ITEMSTATUS)
					{
						Out=f->VMProcess(CheckCode,nullptr,MenuItemPos);
					}
				}
			}

			return api.PassValue(Out);
		}

		case MCODE_F_MENU_SELECT:      // N=Menu.Select(S[,N[,Dir]])
		case MCODE_F_MENU_CHECKHOTKEY: // N=checkhotkey(S[,N])
		{
			auto Params = api.parseParams(3);
			int Result=-1;
			int64_t tmpMode=0;
			int64_t tmpDir=0;

			if (CheckCode == MCODE_F_MENU_SELECT)
				tmpDir=Params[2].getInteger();

			tmpMode=Params[1].getInteger();

			if (CheckCode == MCODE_F_MENU_SELECT)
				tmpMode |= (tmpDir << 8);
			else
			{
				if (tmpMode > 0)
					tmpMode--;
			}

			auto& tmpVar = Params[0];
			int CurMMode=CtrlObject->Macro.GetArea();

			if (IsMenuArea(CurMMode) || CurMMode == MACROAREA_DIALOG)
			{
				auto f = GetCurrentWindow();
				if (f)
					Result=f->VMProcess(CheckCode,(void*)tmpVar.toString(),tmpMode);
			}

			return Result;
		}

		case MCODE_F_MENU_FILTER:      // N=Menu.Filter([Action[,Mode]])
		case MCODE_F_MENU_FILTERSTR:   // S=Menu.FilterStr([Action[,S]])
		{
			auto Params = api.parseParams(2);
			bool success=false;
			TVar& tmpAction(Params[0]);

			TVar tmpVar=Params[1];
			if (tmpAction.isUnknown())
				tmpAction=CheckCode == MCODE_F_MENU_FILTER ? 4 : 0;

			int CurMMode=CtrlObject->Macro.GetArea();

			if (IsMenuArea(CurMMode) || CurMMode == MACROAREA_DIALOG)
			{
				Frame *f=GetCurrentWindow();

				if (f)
				{
					if (CheckCode == MCODE_F_MENU_FILTER)
					{
						if (tmpVar.isUnknown())
							tmpVar = -1;
						tmpVar=f->VMProcess(CheckCode,(void*)static_cast<intptr_t>(tmpVar.toInteger()),tmpAction.toInteger());
						success=true;
					}
					else
					{
						FARString NewStr;
						if (tmpVar.isString())
							NewStr = tmpVar.toString();
						if (f->VMProcess(CheckCode,(void*)&NewStr,tmpAction.toInteger()))
						{
							tmpVar=NewStr.CPtr();
							success=true;
						}
					}
				}
			}

			if (!success)
			{
				if (CheckCode == MCODE_F_MENU_FILTER)
					tmpVar = -1;
				else
					tmpVar = L"";
			}

			return api.PassValue(tmpVar);
		}

		case MCODE_UDLIST_SPLIT:
			return api.UDList_Split();
	}
	return 0;
}

/* ------------------------------------------------------------------- */
// S=trim(S[,N])
int FarMacroApi::trimFunc()
{
	auto Params = parseParams(2);
	const auto mode = static_cast<int>(Params[1].asInteger());
	auto p = wcsdup(Params[0].toString());

	switch (mode)
	{
		case 0: p=RemoveExternalSpaces(p); break;  // alltrim
		case 1: p=RemoveLeadingSpaces(p); break;   // ltrim
		case 2: p=RemoveTrailingSpaces(p); break;  // rtrim
		default: break;
	}
	PassString(p);
	free(p);
	return 0;
}

// S=substr(S,start[,length])
int FarMacroApi::substrFunc()
{
	/*
		TODO: http://bugs.farmanager.com/view.php?id=1480
			если start  >= 0, то вернётся подстрока, начиная со start-символа от начала строки.
			если start  <  0, то вернётся подстрока, начиная со start-символа от конца строки.
			если length >  0, то возвращаемая подстрока будет состоять максимум из length символов исходной строки начиная с start
			если length <  0, то в возвращаемой подстроке будет отсутствовать length символов от конца исходной строки, при том, что она будет начинаться с символа start.
								Или: length - длина того, что берем (если >=0) или отбрасываем (если <0).

			пустая строка возвращается:
				если length = 0
				если ...
	*/
	auto Params = parseParams(3);

	const auto& p = Params[0].toString();
	int start = static_cast<int>(Params[1].asInteger());
	const auto length_str = static_cast<int>(wcslen(p));
	int length = Params[2] == 0 ? length_str : static_cast<int>(Params[2].asInteger());

	if (length)
	{
		if (start < 0)
		{
			start=length_str+start;
			if (start < 0)
				start=0;
		}

		if (start >= length_str)
		{
			length=0;
		}
		else
		{
			if (length > 0)
			{
				if (start+length >= length_str)
					length=length_str-start;
			}
			else
			{
				length=length_str-start+length;

				if (length < 0)
				{
					length=0;
				}
			}
		}
	}

	return PassString(length ? FARString(p+start,length).CPtr() : L"");
}

static bool SplitFileName(const wchar_t *lpFullName,FARString &strDest,int nFlags)
{
#define FLAG_DISK   1
#define FLAG_PATH   2
#define FLAG_NAME   4
#define FLAG_EXT    8
	const wchar_t *s = lpFullName; //start of sub-string
	const wchar_t *p = s; //current FARString pointer
	const wchar_t *es = s+StrLength(s); //end of string
	const wchar_t *e; //end of sub-string

	if (!*p)
		return false;

	if ((*p == L'/') && (*(p+1) == L'/'))   //share
	{
		p += 2;
		p = wcschr(p, L'/');

		if (!p)
			return false; //invalid share (\\server\)

		p = wcschr(p+1, L'/');

		if (!p)
			p = es;

		if ((nFlags & FLAG_DISK) == FLAG_DISK)
		{
			strDest=s;
			strDest.Truncate(p-s);
		}
	}
	else
	{
		if (*(p+1) == L':')
		{
			p += 2;

			if ((nFlags & FLAG_DISK) == FLAG_DISK)
			{
				size_t Length=strDest.GetLength()+p-s;
				strDest+=s;
				strDest.Truncate(Length);
			}
		}
	}

	e = nullptr;
	s = p;

	while (p)
	{
		p = wcschr(p, L'/');

		if (p)
		{
			e = p;
			p++;
		}
	}

	if (e)
	{
		if ((nFlags & FLAG_PATH))
		{
			size_t Length=strDest.GetLength()+e-s;
			strDest+=s;
			strDest.Truncate(Length);
		}

		s = e+1;
		p = s;
	}

	if (!p)
		p = s;

	e = nullptr;

	while (p)
	{
		p = wcschr(p+1, L'.');

		if (p)
			e = p;
	}

	if (!e)
		e = es;

	if (!strDest.IsEmpty())
		AddEndSlash(strDest);

	if (nFlags & FLAG_NAME)
	{
		const wchar_t *ptr = wcschr(s, L':');

		if (ptr)
			s=ptr+1;

		size_t Length=strDest.GetLength()+e-s;
		strDest+=s;
		strDest.Truncate(Length);
	}

	if (nFlags & FLAG_EXT)
		strDest+=e;

	return true;
}

// S=fsplit(S,N)
int FarMacroApi::fsplitFunc()
{
	auto Params = parseParams(2);
	FARString strPath;
	if (!SplitFileName(Params[0].toString(), strPath, Params[1].asInteger()))
		strPath.Clear();

	return PassString(strPath);
}

// N=atoi(S[,radix])
int FarMacroApi::atoiFunc()
{
	auto Params = parseParams(2);
	wchar_t *endptr;
	const auto Ret = _wcstoi64(Params[0].toString(), &endptr, Params[1].toInteger());
	return PassInteger(Ret);
}

// N=Window.Scroll(Lines[,Axis])
int FarMacroApi::windowscrollFunc()
{
	auto Params = parseParams(2);
	int Ret=0;

	if (Opt.WindowMode)
	{
		int Lines = static_cast<int>(Params[0].asInteger()), Columns = 0;
		if (Params[1].asInteger())
		{
			Columns=Lines;
			Lines=0;
		}

		if (Console.ScrollWindow(Lines, Columns))
		{
			Ret=1;
		}
	}

	return PassBoolean(Ret);
}

// S=itoa(N[,radix])
int FarMacroApi::itowFunc()
{
	auto Params = parseParams(2);

	if (Params[0].isInteger() || Params[0].isDouble())
	{
		wchar_t value[80];
		int Radix = static_cast<int>(Params[1].toInteger());

		if (!Radix)
			Radix=10;

		Params[0]=TVar(_i64tow(Params[0].toInteger(),value,Radix));
	}

	return PassValue(Params[0]);
}

// os::chrono::sleep_for(Nms)
int FarMacroApi::sleepFunc()
{
	const auto Params = parseParams(1);
	const auto Period = Params[0].asInteger();

	if (Period > 0)
	{
		WINPORT(Sleep)(Period);
		PassNumber(1);
	}
	else
		PassNumber(0);
	return 0;
}

// N=KeyBar.Show([N])
int FarMacroApi::keybarshowFunc()
{
	/*
	Mode:
		0 - visible?
			ret: 0 - hide, 1 - show, -1 - KeyBar not found
		1 - show
		2 - hide
		3 - swap
		ret: prev mode or -1 - KeyBar not found
    */
	auto Params = parseParams(1);
	const auto f = GetCurrentWindow();

	return f ? f->VMProcess(MCODE_F_KEYBAR_SHOW,nullptr,Params[0].asInteger())-1 : -1;
}

// S=key(V)
int FarMacroApi::keyFunc()
{
	auto Params = parseParams(1);
	FARString strKeyText;

	if (Params[0].isInteger() || Params[0].isDouble())
	{
		if (Params[0].asInteger())
			KeyToText(static_cast<int>(Params[0].asInteger()), strKeyText);
	}
	else
	{
		// Проверим...
		if (mData->Values[0].Type == FMVT_STRING)
		{
			if (KeyNameToKey(mData->Values[0].String) != KEY_INVALID)
				strKeyText = mData->Values[0].String;
		}
	}

	return PassString(strKeyText);
}

// V=waitkey([N,[T]])
int FarMacroApi::waitkeyFunc()
{
	auto Params = parseParams(2);
	const auto Type = static_cast<long>(Params[1].asInteger());
	const auto Period = static_cast<long>(Params[0].asInteger());
	auto Key = WaitKey(static_cast<DWORD>(-1), Period, true, false);

	if (!Type)
	{
		FARString strKeyText;

		if (Key != KEY_NONE)
			KeyToText(Key, strKeyText);

		return PassString(strKeyText);
	}

	if (Key == KEY_NONE)
		Key=-1;

	return PassNumber(Key);
}

// n=min(n1,n2)
int FarMacroApi::minFunc()
{
	auto Params = parseParams(2);
	return PassValue(std::min(Params[0], Params[1]));
}

// n=max(n1,n2)
int FarMacroApi::maxFunc()
{
	auto Params = parseParams(2);
	return PassValue(std::max(Params[0], Params[1]));
}

// n=mod(n1,n2)
int FarMacroApi::modFunc()
{
	auto Params = parseParams(2);

	if (!Params[1].asInteger())
	{
		_KEYMACRO(___FILEFUNCLINE___;SysLog(L"Error: Divide (mod) by zero"));
		PassNumber(0);
	}
	else
		PassValue(Params[0].i() % Params[1].i());
	return 0;
}

// N=index(S1,S2[,Mode])
int FarMacroApi::indexFunc()
{
	auto Params = parseParams(3);
	const wchar_t *s = Params[0].toString();
	const wchar_t *p = Params[1].toString();
	const wchar_t *i = !Params[2].getInteger() ? StrStrI(s,p) : StrStr(s,p);
	return PassNumber(i ? i-s : -1);
}

// S=rindex(S1,S2[,Mode])
int FarMacroApi::rindexFunc()
{
	auto Params = parseParams(3);
	const wchar_t *s = Params[0].toString();
	const wchar_t *p = Params[1].toString();
	const wchar_t *i = !Params[2].getInteger() ? RevStrStrI(s,p) : RevStrStr(s,p);
	return PassNumber(i ? i-s : -1);
}

// S=date([S])
int FarMacroApi::dateFunc()
{
	auto Params = parseParams(1);

	if (Params[0].isInteger() && !Params[0].asInteger())
		Params[0] = L"";

	FARString strTStr;
	MkStrFTime(strTStr, Params[0].toString());
	return PassString(strTStr);
}

// S=xlat(S[,Flags])
/*
  Flags:
  	XLAT_SWITCHKEYBLAYOUT  = 1
		XLAT_SWITCHKEYBBEEP    = 2
		XLAT_USEKEYBLAYOUTNAME = 4
*/
int FarMacroApi::xlatFunc()
{
	auto Params = parseParams(2);
	auto StrParam = wcsdup(Params[0].toString());
	::Xlat(StrParam,0,StrLength(StrParam),Opt.XLat.Flags);
	PassString(StrParam);
	free(StrParam);
	return 0;
}

// S=prompt(["Title"[,"Prompt"[,flags[, "Src"[, "History"]]]]])
int FarMacroApi::promptFunc()
{
	auto Params = parseParams(5);
	auto& ValHistory(Params[4]);
	auto& ValSrc(Params[3]);
	const auto Flags = static_cast<DWORD>(Params[2].asInteger());
	auto& ValPrompt(Params[1]);
	auto& ValTitle(Params[0]);

	const wchar_t* title   = ValTitle.isString()   ? ValTitle.toString()   : L"";
	const wchar_t* history = ValHistory.isString() ? ValHistory.toString() : L"";
	const wchar_t* src     = ValSrc.isString()     ? ValSrc.toString()     : L"";
	const wchar_t* prompt  = ValPrompt.isString()  ? ValPrompt.toString()  : L"";

	FARString strDest;

	const auto oldHistoryDisable = GetHistoryDisableMask();
	if (!history[0]) // Mantis#0001743: Возможность отключения истории
		SetHistoryDisableMask(8); // если не указан history, то принудительно отключаем историю для ЭТОГО prompt()

	if (GetString(title, prompt, history, src, strDest, {}, (Flags&~FIB_CHECKBOX) | FIB_ENABLEEMPTY))
	{
		PassString(strDest);
	}
	else
		PassBoolean(0);

	SetHistoryDisableMask(oldHistoryDisable);

	return 0;
}

// N=msgbox(["Title"[,"Text"[,flags]]])
int FarMacroApi::msgBoxFunc()
{
	auto Params = parseParams(3);

	DWORD Flags = (DWORD)Params[2].getInteger();
	auto ValT = Params[0], ValB = Params[1];
	const wchar_t *title = L"";

	if (!(ValT.isInteger() && !ValT.i()))
		title=NullToEmpty(ValT.toString());

	const wchar_t *text  = L"";

	if (!(ValB.isInteger() && !ValB.i()))
		text =NullToEmpty(ValB.toString());

	Flags&=~(FMSG_KEEPBACKGROUND|FMSG_ERRORTYPE);
	Flags|=FMSG_ALLINONE;

	if (!HIWORD(Flags) || HIWORD(Flags) > HIWORD(FMSG_MB_RETRYCANCEL))
		Flags|=FMSG_MB_OK;

	//_KEYMACRO(SysLog(L"title='%ls'",title));
	//_KEYMACRO(SysLog(L"text='%ls'",text));
	FARString TempBuf = title;
	TempBuf += L"\n";
	TempBuf += text;
	auto Result=FarMessageFn(-1,Flags,nullptr,(const wchar_t * const *)TempBuf.CPtr(),0,0)+1;
	return PassNumber(Result);
}

// S=env(S)
int FarMacroApi::environFunc()
{
	auto Params = parseParams(1);
	FARString strEnv;

	if (!apiGetEnvironmentVariable(Params[0].toString(), strEnv))
		strEnv.Clear();

	return PassString(strEnv);
}

// V=Panel.Select(panelType,Action[,Mode[,Items]])
int FarMacroApi::panelselectFunc()
{
	auto Params = parseParams(4);

	auto& ValItems = Params[3];
	int Mode       = Params[2].getInt32();
	DWORD Action   = (int)Params[1].getInteger();
	int typePanel  = Params[0].getInt32();
	int64_t Result=-1;

	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
	Panel *PassivePanel=nullptr;

	if (ActivePanel)
		PassivePanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

	Panel *SelPanel = !typePanel ? ActivePanel : (typePanel == 1?PassivePanel:nullptr);

	if (SelPanel)
	{
		int64_t Index=-1;
		if (Mode == 1)
		{
			Index=ValItems.getInteger();
			if (!Index)
				Index=SelPanel->GetCurrentPos();
			else
				Index--;
		}

		FARString strStr;
		if (Mode == 2 || Mode == 3)
		{
			strStr=ValItems.s();
			ReplaceStrings(strStr,L"\r\n",L";");
			ReplaceStrings(strStr,L"\n",L";");
		}

		MacroPanelSelect mps;
		mps.Action      = Action & 0xF;
		mps.ActionFlags = (Action & (~0xF)) >> 4;
		mps.Mode        = Mode;
		mps.Index       = Index;
		mps.Item        = strStr.CPtr();
		Result=SelPanel->VMProcess(MCODE_F_PANEL_SELECT,&mps,0);
	}

	if (Result < 0)
		Result = 0;
	return PassNumber(Result);
}

// N=panel.SetPath(panelType,pathName[,fileName])
int FarMacroApi::panelsetpathFunc()
{
	auto Params = parseParams(3);
	int typePanel     = Params[0].getInt32();
	auto& Val         = Params[1];
	auto& ValFileName = Params[2];
	int Ret=0;

	if (Val.isString())
	{
		const wchar_t *pathName=Val.s();
		const wchar_t *fileName=ValFileName.isString() ? ValFileName.s():L"";

		Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
		Panel *PassivePanel=nullptr;

		if (ActivePanel)
			PassivePanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

		Panel *SelPanel = typePanel==0 ? ActivePanel : typePanel==1 ? PassivePanel : nullptr;

		if (SelPanel)
		{
			FARString strPath;
			if (apiExpandEnvironmentStrings(pathName, strPath))
				pathName = strPath.CPtr();

			if (SelPanel->SetCurDir(pathName,SelPanel->GetMode()==PLUGIN_PANEL && IsAbsolutePath(pathName),false))
			{
				ActivePanel=CtrlObject->Cp()->ActivePanel;
				PassivePanel=ActivePanel?CtrlObject->Cp()->GetAnotherPanel(ActivePanel):nullptr;
				SelPanel = typePanel? (typePanel == 1?PassivePanel:nullptr):ActivePanel;

				//восстановим текущую папку из активной панели.
				ActivePanel->SetCurPath();
				// Need PointToName()?
				SelPanel->GoToFile(fileName); // здесь без проверки, т.к. параметр fileName аля опциональный
				//SelPanel->Show();
				// <Mantis#0000289> - грозно, но со вкусом :-)
				//ShellUpdatePanels(SelPanel);
				SelPanel->UpdateIfChanged(UIC_UPDATE_NORMAL);
				FrameManager->RefreshFrame(FrameManager->GetTopModal());
				// </Mantis#0000289>
				Ret=1;
			}
		}
	}

	return PassBoolean(Ret);
}

int FarMacroApi::fattrFuncImpl(int Type)
{
	DWORD FileAttr=INVALID_FILE_ATTRIBUTES;
	long Pos=-1;

	if (!Type || Type == 2) // не панели: fattr(0) & fexist(2)
	{
		auto Params = parseParams(1);
		FAR_FIND_DATA_EX FindData;
		apiGetFindDataEx(Params[0].toString(), FindData);
		FileAttr=FindData.dwFileAttributes;
	}
	else // panel.fattr(1) & panel.fexist(3)
	{
		auto Params = parseParams(2);
		int typePanel = Params[0].getInt32();
		const wchar_t *Str = Params[1].toString();
		Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
		Panel *PassivePanel=nullptr;

		if (ActivePanel)
			PassivePanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

		//Frame* CurFrame=FrameManager->GetCurrentFrame();
		Panel *SelPanel = !typePanel ? ActivePanel : (typePanel == 1?PassivePanel:nullptr);

		if (SelPanel)
		{
			if (FindAnyOfChars(Str, "*?") )
				Pos=SelPanel->FindFirst(Str);
			else
				Pos=SelPanel->FindFile(Str, FindAnyOfChars(Str, "/") ? FALSE : TRUE);

			if (Pos >= 0)
			{
				FARString strFileName;
				SelPanel->GetFileName(strFileName,Pos,FileAttr);
			}
		}
	}

	if (Type == 2) // fexist(2)
	{
		return PassBoolean(FileAttr!=INVALID_FILE_ATTRIBUTES);
	}

	if (Type == 3) // panel.fexist(3)
		FileAttr=(DWORD)Pos+1;

	return PassNumber(static_cast<int32_t>(FileAttr));
}

// N=fattr(S)
int FarMacroApi::fattrFunc()
{
	return fattrFuncImpl(0);
}

// N=fexist(S)
int FarMacroApi::fexistFunc()
{
	return fattrFuncImpl(2);
}

// N=panel.fattr(S)
int FarMacroApi::panelfattrFunc()
{
	return fattrFuncImpl(1);
}

// N=panel.fexist(S)
int FarMacroApi::panelfexistFunc()
{
	return fattrFuncImpl(3);
}

// N=FLock(Nkey,NState)
/*
  Nkey:
     0 - NumLock
     1 - CapsLock
     2 - ScrollLock

  State:
    -1 get state
     0 off
     1 on
     2 flip
*/
int FarMacroApi::flockFunc()
{
	auto Params = parseParams(2);
	int Ret = -1;
	const auto stateFLock = static_cast<int>(Params[1].asInteger());
	auto vkKey = static_cast<unsigned>(Params[0].asInteger());

	switch (vkKey)
	{
		case 0:
			vkKey=VK_NUMLOCK;
			break;
		case 1:
			vkKey=VK_CAPITAL;
			break;
		case 2:
			vkKey=VK_SCROLL;
			break;
		default:
			vkKey=0;
			break;
	}

	if (vkKey)
		Ret=SetFLockState(vkKey,stateFLock);

	return PassNumber(Ret);
}

// N=Dlg->SetFocus([ID])
int FarMacroApi::dlgsetfocusFunc()
{
	auto Params = parseParams(1);
	TVar Ret(-1);
	const auto Index = static_cast<unsigned>(Params[0].asInteger()) - 1;

	Frame* CurFrame=FrameManager->GetCurrentFrame();
	auto Dlg = dynamic_cast<Dialog*>(CurFrame);
	if (Dlg && CtrlObject->Macro.GetArea() == MACROAREA_DIALOG)
	{
		Ret = Dlg->VMProcess(MCODE_V_DLGCURPOS);
		if (static_cast<int>(Index) >= 0)
		{
			//if (!Dlg->SendMessage(DM_SETFOCUS, Index, nullptr))
			if (!SendDlgMessage(Dlg, DM_SETFOCUS, Index, 0))
				Ret = 0;
		}
	}
	return PassValue(Ret);
}

// V=Far.GetConfig(Key.Name)
int FarMacroApi::fargetconfigFunc()
{
	const wchar_t *Keyname = (mData->Count >= 1 && mData->Values[0].Type==FMVT_STRING) ?
		mData->Values[0].String : L"";

	auto Dot = wcsrchr(Keyname, L'.');
	if (Dot)
	{
		DWORD dwValue;
		FARString strValue;
		const void *binValue;

		FARString Key(Keyname, Dot - Keyname);
		switch( GetConfigValue(Key.CPtr(), Dot+1, dwValue, strValue, &binValue) )
		{
			default:
				PassError(L"setting doesn't exist");
				break;
			case REG_DWORD:
				PassNumber(dwValue);
				PassString(L"integer");
				break;
			case REG_SZ:
				PassString(strValue);
				PassString(L"string");
				break;
			case REG_BINARY:
				PassBinary(binValue, dwValue);
				PassString(L"binary");
				break;
		}
	}
	else
		PassError(L"invalid argument #1");

	return 0;
}

// V=Far.Cfg_Get(Index)
int FarMacroApi::farcfggetFunc()
{
	auto Params = parseParams(1);
	auto Index = static_cast<size_t>(Params[0].asInteger()) - 1;
	GetConfig Data;

	bool Ret = GetConfigValue(Index, Data);
	if (!Ret)
	{
		PassBoolean(0);
		return 0;
	}

	PassString(Data.Key);
	PassString(Data.Name);

	switch(Data.Type)
	{
		case REG_DWORD:
			PassString(L"integer");
			PassNumber(Data.dwDefault);
			PassNumber(Data.dwValue);
			break;
		case REG_BOOLEAN:
			PassString(L"boolean");
			PassNumber(Data.dwDefault);
			PassNumber(Data.dwValue);
			break;
		case REG_3STATE:
			PassString(L"3-state");
			PassNumber(Data.dwDefault);
			PassNumber(Data.dwValue);
			break;
		case REG_SZ:
			PassString(L"string");
			PassString(Data.strDefault);
			PassString(Data.strValue);
			break;
		case REG_BINARY:
			PassString(L"binary");
			PassBinary(Data.binData, Data.dwValue);
			break;
	}

	return 0;
}

// Ok=Far.Cfg_Set(Index,Value)
int FarMacroApi::farcfgsetFunc()
{
	bool Res = false;

	if (mData->Count >= 2 && mData->Values[0].Type==FMVT_DOUBLE)
	{
		auto Index = static_cast<size_t>(mData->Values[0].Double - 1);
		switch (mData->Values[1].Type)
		{
			case FMVT_DOUBLE:
				Res = SetConfigValue(Index, static_cast<DWORD>(mData->Values[1].Double));
				break;
			case FMVT_STRING:
				Res = SetConfigValue(Index, mData->Values[1].String);
				break;
			default:
				break;
		}
	}

	PassBoolean(Res ? 1 : 0);
	return 0;
}

// V=Dlg.GetValue(ID,N)
int FarMacroApi::dlggetvalueFunc()
{
	auto Params = parseParams(2);
	TVar Ret(-1);
	int TypeInf = Params[1].getInt32();
	int Index=(int)Params[0].getInteger()-1;
	Frame* CurFrame=FrameManager->GetCurrentFrame();
	auto Dlg = dynamic_cast<Dialog*>(CurFrame);

	if (Dlg && CtrlObject->Macro.GetArea()==MACROAREA_DIALOG && CurFrame && CurFrame->GetType()==MODALTYPE_DIALOG)
	{
		if (Index < -1)
			Index=Dlg->GetDlgFocusPos();

		int DlgItemCount=((Dialog*)CurFrame)->GetAllItemCount();
		const DialogItemEx **DlgItem=((Dialog*)CurFrame)->GetAllItem();

		if (Index == -1)
		{
			SMALL_RECT Rect;

			if (SendDlgMessage(CurFrame,DM_GETDLGRECT,0,(LONG_PTR)&Rect))
			{
				switch (TypeInf)
				{
					case 0: Ret=(int64_t)DlgItemCount; break;
					case 2: Ret=Rect.Left; break;
					case 3: Ret=Rect.Top; break;
					case 4: Ret=Rect.Right; break;
					case 5: Ret=Rect.Bottom; break;
					case 6: Ret=(int64_t)(((Dialog*)CurFrame)->GetDlgFocusPos()+1); break;
				}
			}
		}
		else if (Index < DlgItemCount && DlgItem)
		{
			const DialogItemEx *Item=DlgItem[Index];
			int ItemType=Item->Type;
			DWORD ItemFlags=Item->Flags;

			if (!TypeInf)
			{
				if (ItemType == DI_CHECKBOX || ItemType == DI_RADIOBUTTON)
				{
					TypeInf=7;
				}
				else if (ItemType == DI_COMBOBOX || ItemType == DI_LISTBOX)
				{
					FarListGetItem ListItem;
					ListItem.ItemIndex=Item->ListPtr->GetSelectPos();

					if (SendDlgMessage(CurFrame,DM_LISTGETITEM,Index,(LONG_PTR)&ListItem))
					{
						Ret=ListItem.Item.Text;
					}
					else
					{
						Ret=L"";
					}

					TypeInf=-1;
				}
				else
				{
					TypeInf=10;
				}
			}

			switch (TypeInf)
			{
				case 1: Ret=ItemType;    break;
				case 2: Ret=Item->X1;    break;
				case 3: Ret=Item->Y1;    break;
				case 4: Ret=Item->X2;    break;
				case 5: Ret=Item->Y2;    break;
				case 6: Ret=Item->Focus; break;
				case 7:
				{
					if (ItemType == DI_CHECKBOX || ItemType == DI_RADIOBUTTON)
					{
						Ret=Item->Selected;
					}
					else if (ItemType == DI_COMBOBOX || ItemType == DI_LISTBOX)
					{
						Ret=Item->ListPtr->GetSelectPos()+1;
					}
					else
					{
						Ret = 0;
					}
					break;
				}
				case 8: Ret=(int64_t)ItemFlags; break;
				case 9: Ret=Item->DefaultButton; break;
				case 10:
				{
					Ret=Item->strData.CPtr();
					if (FarIsEdit(ItemType))
					{
						DlgEdit *EditPtr;
						if ((EditPtr = (DlgEdit *)(Item->ObjPtr)) )
							Ret=EditPtr->GetStringAddr();
					}
					break;
				}
			}
		}
	}

	return PassValue(Ret);
}

// N=Editor.Pos(Op,What[,Where])
// Op: 0 - get, 1 - set
int FarMacroApi::editorposFunc()
{
	auto Params = parseParams(3);
	TVar Ret(-1);
	int Where = Params[2].getInt32();
	int What  = Params[1].getInt32();
	int Op    = Params[0].getInt32();

	if (CtrlObject->Macro.GetArea()==MACROAREA_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
	{
		EditorInfo ei;
		CtrlObject->Plugins.CurEditor->EditorControl(ECTL_GETINFO,&ei);

		switch (Op)
		{
			case 0: // get
			{
				switch (What)
				{
					case 1: // CurLine
						Ret=ei.CurLine+1;
						break;
					case 2: // CurPos
						Ret=ei.CurPos+1;
						break;
					case 3: // CurTabPos
						Ret=ei.CurTabPos+1;
						break;
					case 4: // TopScreenLine
						Ret=ei.TopScreenLine+1;
						break;
					case 5: // LeftPos
						Ret=ei.LeftPos+1;
						break;
					case 6: // Overtype
						Ret=ei.Overtype;
						break;
				}

				break;
			}
			case 1: // set
			{
				EditorSetPosition esp;
				esp.CurLine=-1;
				esp.CurPos=-1;
				esp.CurTabPos=-1;
				esp.TopScreenLine=-1;
				esp.LeftPos=-1;
				esp.Overtype=-1;

				switch (What)
				{
					case 1: // CurLine
						esp.CurLine=Where-1;

						if (esp.CurLine < 0)
							esp.CurLine=-1;

						break;
					case 2: // CurPos
						esp.CurPos=Where-1;

						if (esp.CurPos < 0)
							esp.CurPos=-1;

						break;
					case 3: // CurTabPos
						esp.CurTabPos=Where-1;

						if (esp.CurTabPos < 0)
							esp.CurTabPos=-1;

						break;
					case 4: // TopScreenLine
						esp.TopScreenLine=Where-1;

						if (esp.TopScreenLine < 0)
							esp.TopScreenLine=-1;

						break;
					case 5: // LeftPos
					{
						int Delta=Where-1-ei.LeftPos;
						esp.LeftPos=Where-1;

						if (esp.LeftPos < 0)
							esp.LeftPos=-1;

						esp.CurPos=ei.CurPos+Delta;
						break;
					}
					case 6: // Overtype
						esp.Overtype=Where;
						break;
				}

				int Result=CtrlObject->Plugins.CurEditor->EditorControl(ECTL_SETPOSITION,&esp);

				if (Result)
					CtrlObject->Plugins.CurEditor->EditorControl(ECTL_REDRAW,nullptr);

				Ret=Result;
				break;
			}
		}
	}

	return PassValue(Ret);
}

// OldVar=Editor.Set(Idx,Var)
int FarMacroApi::editorsetFunc()
{
	TVar Ret(-1);
	auto Params = parseParams(2);
	auto& _longState = Params[1];
	int Index = Params[0].getInt32();

	if (CtrlObject->Macro.GetArea()==MACROAREA_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
	{
		long longState=-1L;

		if (Index != 12)
			longState=(long)_longState.toInteger();

		EditorOptions EdOpt;
		CtrlObject->Plugins.CurEditor->GetEditorOptions(EdOpt);

		switch (Index)
		{
			case 0:  // TabSize;
				Ret=EdOpt.TabSize; break;
			case 1:  // ExpandTabs;
				Ret=EdOpt.ExpandTabs; break;
			case 2:  // PersistentBlocks;
				Ret=EdOpt.PersistentBlocks; break;
			case 3:  // DelRemovesBlocks;
				Ret=EdOpt.DelRemovesBlocks; break;
			case 4:  // AutoIndent;
				Ret=EdOpt.AutoIndent; break;
			case 5:  // AutoDetectCodePage;
				Ret=EdOpt.AutoDetectCodePage; break;
			case 6:  // DefaultCodePage;
				Ret=(int)EdOpt.DefaultCodePage; break;
			case 7:  // CursorBeyondEOL;
				Ret=EdOpt.CursorBeyondEOL; break;
			case 8:  // BSLikeDel;
				Ret=EdOpt.BSLikeDel; break;
			case 9:  // CharCodeBase;
				Ret=EdOpt.CharCodeBase; break;
			case 10: // SavePos;
				Ret=EdOpt.SavePos; break;
			case 11: // SaveShortPos;
				Ret=EdOpt.SaveShortPos; break;
			case 12: // char WordDiv[256];
				Ret=TVar(EdOpt.strWordDiv); break;
			case 13: // F7Rules;
				Ret=EdOpt.F7Rules; break;
			case 14: // AllowEmptySpaceAfterEof;
				Ret=EdOpt.AllowEmptySpaceAfterEof; break;
			case 15: // ShowScrollBar;
				Ret=EdOpt.ShowScrollBar; break;
			case 16: // EditOpenedForWrite;
				Ret=EdOpt.EditOpenedForWrite; break;
			case 17: // SearchSelFound;
				Ret=EdOpt.SearchSelFound; break;
			case 18: // SearchRegexp;
				Ret=EdOpt.SearchRegexp; break;
			case 19: // SearchPickUpWord;
				Ret=EdOpt.SearchPickUpWord; break;
			case 20: // ShowWhiteSpace;
				Ret=EdOpt.ShowWhiteSpace; break;
			default:
				Ret = -1;
		}

		if ((Index != 12 && longState != -1) || (Index == 12 && _longState.i() == -1))
		{
			switch (Index)
			{
				case 0:  // TabSize;
					EdOpt.TabSize=longState; break;
				case 1:  // ExpandTabs;
					EdOpt.ExpandTabs=longState; break;
				case 2:  // PersistentBlocks;
					EdOpt.PersistentBlocks=longState; break;
				case 3:  // DelRemovesBlocks;
					EdOpt.DelRemovesBlocks=longState; break;
				case 4:  // AutoIndent;
					EdOpt.AutoIndent=longState; break;
				case 5:  // AutoDetectCodePage;
					EdOpt.AutoDetectCodePage=longState; break;
				case 6:  // DefaultCodePage;
					EdOpt.DefaultCodePage=longState; break;
				case 7:  // CursorBeyondEOL;
					EdOpt.CursorBeyondEOL=longState; break;
				case 8:  // BSLikeDel;
					EdOpt.BSLikeDel=longState; break;
				case 9:  // CharCodeBase;
					EdOpt.CharCodeBase=longState; break;
				case 10: // SavePos;
					EdOpt.SavePos=longState; break;
				case 11: // SaveShortPos;
					EdOpt.SaveShortPos=longState; break;
				case 12: // char WordDiv[256];
					EdOpt.strWordDiv = _longState.toString(); break;
				case 13: // F7Rules;
					EdOpt.F7Rules=longState; break;
				case 14: // AllowEmptySpaceAfterEof;
					EdOpt.AllowEmptySpaceAfterEof=longState; break;
				case 15: // ShowScrollBar;
					EdOpt.ShowScrollBar=longState; break;
				case 16: // EditOpenedForWrite;
					EdOpt.EditOpenedForWrite=longState; break;
				case 17: // SearchSelFound;
					EdOpt.SearchSelFound=longState; break;
				case 18: // SearchRegexp;
					EdOpt.SearchRegexp=longState; break;
				case 19: // SearchPickUpWord;
					EdOpt.SearchPickUpWord=longState; break;
				case 20: // ShowWhiteSpace;
					EdOpt.ShowWhiteSpace=longState; break;
				default:
					Ret=-1;
					break;
			}

			CtrlObject->Plugins.CurEditor->SetEditorOptions(EdOpt);
			CtrlObject->Plugins.CurEditor->ShowStatus();
		}
	}

	return PassValue(Ret);
}

// V=Clip(N[,V])
int FarMacroApi::clipFunc()
{
	auto Params = parseParams(2);
	auto& Val = Params[1];
	int cmdType = Params[0].getInt32();

	// принудительно второй параметр ставим AS string
	if (cmdType != 5 && Val.isInteger() && !Val.i())
	{
		Val=L"";
		Val.toString();
	}

	int Ret=0;

	switch (cmdType)
	{
		case 0: // Get from Clipboard, "S" - ignore
		{
			wchar_t *ClipText=PasteFromClipboard();
			if (ClipText)
			{
				TVar varClip(ClipText);
				free(ClipText);
				return PassValue(varClip);
			}
			break;
		}
		case 1: // Put "S" into Clipboard
		{
			Ret=CopyToClipboard(Val.s());
			return PassNumber(Ret);
		}
		case 2: // Add "S" into Clipboard
		{
			TVar varClip(Val.s());
			Clipboard clip;

			Ret=0;

			if (clip.Open())
			{
				wchar_t *CopyData=clip.Paste();

				if (CopyData)
				{
					size_t DataSize=StrLength(CopyData);
					wchar_t *NewPtr=(wchar_t *)realloc(CopyData,(DataSize+StrLength(Val.s())+2)*sizeof(wchar_t));

					if (NewPtr)
					{
						CopyData=NewPtr;
						wcscpy(CopyData+DataSize,Val.s());
						varClip=CopyData;
						free(CopyData);
					}
					else
					{
						free(CopyData);
					}
				}

				Ret=clip.Copy(varClip.s());

				clip.Close();
			}
			return PassNumber(Ret);
		}
		case 3: // Copy Win to internal, "S" - ignore
		case 4: // Copy internal to Win, "S" - ignore
		{

			bool OldUseInternalClipboard=Clipboard::SetUseInternalClipboardState((cmdType-3)?true:false);
			TVar varClip(L"");
			wchar_t *ClipText=PasteFromClipboard();

			if (ClipText)
			{
				varClip=ClipText;
				free(ClipText);
			}

			Clipboard::SetUseInternalClipboardState(!Clipboard::GetUseInternalClipboardState());
			Ret=CopyToClipboard(varClip.s());

			Clipboard::SetUseInternalClipboardState(OldUseInternalClipboard);
			return PassNumber(Ret); // 0!  ???
		}
		case 5: // ClipMode
		{
			// 0 - flip, 1 - виндовый буфер, 2 - внутренний, -1 - что сейчас?
			int Action = Val.getInt32();
			bool prev_mode = Clipboard::GetUseInternalClipboardState();
			if (Action >= 0 && Action <= 2)
			{
				bool mode = false;
				switch (Action)
				{
					case 0: mode=!prev_mode; break;
					case 1: mode=false; break;
					case 2: mode=true;  break;
				}
				Clipboard::SetUseInternalClipboardState(mode);
			}
			return prev_mode ? 2 : 1;
		}
	}

	return Ret ? 1:0;
}

// N=Panel.SetPosIdx(panelType,Idx[,InSelection])
/*
*/
int FarMacroApi::panelsetposidxFunc()
{
	auto Params = parseParams(3);
	int InSelection = Params[2].getInt32();
	long idxItem=(long)Params[1].getInteger();
	int typePanel = Params[0].getInt32();

	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
	Panel *PassivePanel=nullptr;

	if (ActivePanel)
		PassivePanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

	//Frame* CurFrame=FrameManager->GetCurrentFrame();
	Panel *SelPanel = typePanel? (typePanel == 1?PassivePanel:nullptr):ActivePanel;
	int64_t Ret=0;

	if (SelPanel)
	{
		int TypePanel=SelPanel->GetType(); //FILE_PANEL,TREE_PANEL,QVIEW_PANEL,INFO_PANEL

		if (TypePanel == FILE_PANEL || TypePanel ==TREE_PANEL)
		{
			long EndPos=SelPanel->GetFileCount();
			long StartPos;
			long I;
			long idxFoundItem=0;

			if (idxItem) // < 0 || > 0
			{
				EndPos--;
				if ( EndPos > 0 )
				{
					long Direct=idxItem < 0?-1:1;

					if( Direct < 0 )
						idxItem=-idxItem;
					idxItem--;

					if( Direct < 0 )
					{
						StartPos=EndPos;
						EndPos=0;//InSelection?0:idxItem;
					}
					else
						StartPos=0;//!InSelection?0:idxItem;

					bool found=false;

					for ( I=StartPos ; ; I+=Direct )
					{
						if (Direct > 0)
						{
							if(I > EndPos)
								break;
						}
						else
						{
							if(I < EndPos)
								break;
						}

						if ( (!InSelection || SelPanel->IsSelected(I)) && SelPanel->FileInFilter(I) )
						{
							if (idxFoundItem == idxItem)
							{
								idxItem=I;
								found=true;
								break;
							}
							idxFoundItem++;
						}
					}

					if (!found)
						idxItem=-1;

					if (idxItem != -1 && SelPanel->GoToFile(idxItem))
					{
						//SelPanel->Show();
						// <Mantis#0000289> - грозно, но со вкусом :-)
						//ShellUpdatePanels(SelPanel);
						SelPanel->UpdateIfChanged(UIC_UPDATE_NORMAL);
						FrameManager->RefreshFrame(FrameManager->GetTopModal());
						// </Mantis#0000289>

						if ( !InSelection )
							Ret = SelPanel->GetCurrentPos()+1;
						else
							Ret = idxFoundItem+1;
					}
				}
			}
			else // = 0 - вернем текущую позицию
			{
				if ( !InSelection )
					Ret = SelPanel->GetCurrentPos()+1;
				else
				{
					long CurPos=SelPanel->GetCurrentPos();
					for ( I=0 ; I < EndPos ; I++ )
					{
						if ( SelPanel->IsSelected(I) && SelPanel->FileInFilter(I) )
						{
							if (I == CurPos)
							{
								Ret=(int64_t)(idxFoundItem+1);
								break;
							}
							idxFoundItem++;
						}
					}
				}
			}
		}
	}

	return PassNumber(Ret);
}

// N=Panel.SetPos(panelType,fileName)
int FarMacroApi::panelsetposFunc()
{
	auto Params = parseParams(2);
	auto& Val = Params[1];
	int typePanel = Params[0].getInt32();
	const wchar_t *fileName=Val.s();

	if (!fileName || !*fileName)
		fileName=L"";

	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
	Panel *PassivePanel=nullptr;

	if (ActivePanel)
		PassivePanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

	//Frame* CurFrame=FrameManager->GetCurrentFrame();
	Panel *SelPanel = typePanel? (typePanel == 1?PassivePanel:nullptr):ActivePanel;
	int64_t Ret=0;

	if (SelPanel)
	{
		int TypePanel=SelPanel->GetType(); //FILE_PANEL,TREE_PANEL,QVIEW_PANEL,INFO_PANEL

		if (TypePanel == FILE_PANEL || TypePanel ==TREE_PANEL)
		{
			// Need PointToName()?
			if (SelPanel->GoToFile(fileName))
			{
				//SelPanel->Show();
				// <Mantis#0000289> - грозно, но со вкусом :-)
				//ShellUpdatePanels(SelPanel);
				SelPanel->UpdateIfChanged(UIC_UPDATE_NORMAL);
				FrameManager->RefreshFrame(FrameManager->GetTopModal());
				// </Mantis#0000289>
				Ret=(int64_t)(SelPanel->GetCurrentPos()+1);
			}
		}
	}

	return PassNumber(Ret);
}

// Result=replace(Str,Find,Replace[,Cnt[,Mode]])
/*
Find=="" - return Str
Cnt==0 - return Str
Replace=="" - return Str (с удалением всех подстрок Find)
Str=="" return ""

Mode:
      0 - case insensitive
      1 - case sensitive
*/
int FarMacroApi::replaceFunc()
{
	auto Params = parseParams(5);
	int Mode    = Params[4].getInt32();
	auto& Count = Params[3];
	auto& Repl  = Params[2];
	auto& Find  = Params[1];
	auto& Src   = Params[0];
	// TODO: Здесь нужно проверить в соответствии с УНИХОДОМ!
	int cnt=0;

	if( StrLength(Find.s()) )
	{
		auto func = Mode ? StrStr : StrStrI;
		if (func(Src.s(), Find.s()) )
			cnt++;
	}

	if (cnt)
	{
		FARString strStr=Src.s();
		cnt=(int)Count.i();

		if (cnt <= 0)
			cnt=-1;

		ReplaceStrings(strStr,Find.s(),Repl.s(),cnt,!Mode);
		PassString(strStr);
	}
	else
		PassString(Src.s());

	return 0;
}

// V=Panel.Item(typePanel,Index,TypeInfo)
int FarMacroApi::panelitemFunc()
{
	auto Params = parseParams(3);
	auto& P2 = Params[2];
	auto& P1 = Params[1];
	int typePanel = Params[0].getInt32();
	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
	Panel *PassivePanel=nullptr;

	if (ActivePanel)
		PassivePanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

	Panel *SelPanel = typePanel? (typePanel == 1?PassivePanel:nullptr):ActivePanel;

	if (!SelPanel)
	{
		return 0;
	}

	int TypePanel=SelPanel->GetType(); //FILE_PANEL,TREE_PANEL,QVIEW_PANEL,INFO_PANEL

	if (!(TypePanel == FILE_PANEL || TypePanel ==TREE_PANEL))
	{
		return 0;
	}

	int Index=(int)(P1.toInteger())-1;
	int TypeInfo=(int)P2.toInteger();
	FileList *fileList;
	TreeList *treeList;

	if ((treeList = dynamic_cast<TreeList*>(SelPanel)))
	{
		const TreeItem *treeItem = treeList->GetItem(Index);

		if (treeItem && !TypeInfo)
		{
			return PassString(treeItem->strName);
		}
	}
	else if ((fileList = dynamic_cast<FileList*>(SelPanel)))
	{
		const FileListItem *filelistItem;
		FARString strDate, strTime;

		if (nullptr == (filelistItem = fileList->GetItem(Index)))
			return 0;

		switch (TypeInfo)
		{
			case 0:  // Name
				return PassString(filelistItem->strName);

			case 1:  // ShortName obsolete, use Name
				return PassString(filelistItem->strName);

			case 2:  // FileAttr
				return PassNumber(filelistItem->FileAttr);

			case 3:  // CreationTime
				ConvertDate(filelistItem->CreationTime,strDate,strTime,8,FALSE,FALSE,TRUE,TRUE);
				strDate += L" ";
				strDate += strTime;
				return PassString(strDate);

			case 4:  // AccessTime
				ConvertDate(filelistItem->AccessTime,strDate,strTime,8,FALSE,FALSE,TRUE,TRUE);
				strDate += L" ";
				strDate += strTime;
				return PassString(strDate);

			case 5:  // WriteTime
				ConvertDate(filelistItem->WriteTime,strDate,strTime,8,FALSE,FALSE,TRUE,TRUE);
				strDate += L" ";
				strDate += strTime;
				return PassString(strDate);

			case 6:  // FileSize
				return PassNumber(filelistItem->FileSize);

			case 7:  // PhysicalSize
				return PassNumber(filelistItem->PhysicalSize);

			case 8:  // Selected
				return PassBoolean(filelistItem->Selected);

			case 9:  // NumberOfLinks
				return PassNumber(filelistItem->NumberOfLinks);

			case 10:  // SortGroup
				return PassNumber(filelistItem->SortGroup);

			case 11:  // DizText
				fileList->ReadDiz();
				return PassString(filelistItem->DizText);

			case 12:  // Owner
				return PassString(filelistItem->strOwner);

			case 13:  // CRC32
				return PassNumber(filelistItem->CRC32);

			case 14:  // Position
				return PassNumber(filelistItem->Position);

			case 15:  // CreationTime (FILETIME)
				return PassInteger((int64_t)FileTimeToUI64(&filelistItem->CreationTime));

			case 16:  // AccessTime (FILETIME)
				return PassInteger((int64_t)FileTimeToUI64(&filelistItem->AccessTime));

			case 17:  // WriteTime (FILETIME)
				return PassInteger((int64_t)FileTimeToUI64(&filelistItem->WriteTime));

			case 18: // NumberOfStreams (deprecated)
				return (filelistItem->FileAttr & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;

			case 19: // StreamsSize (deprecated)
				return 0;

			case 20:  // ChangeTime
				ConvertDate(filelistItem->ChangeTime,strDate,strTime,8,FALSE,FALSE,TRUE,TRUE);
				strDate += L" ";
				strDate += strTime;
				return PassString(strDate);

			case 21:  // ChangeTime (FILETIME)
				return PassInteger((int64_t)FileTimeToUI64(&filelistItem->ChangeTime));
		}
	}

	return 0;
}

// N=len(V)
int FarMacroApi::lenFunc()
{
	auto Params = parseParams(1);
	return PassNumber(StrLength(Params[0].toString()));
}

int FarMacroApi::ucaseFunc()
{
	auto Params = parseParams(1);
	wchar_t* Val = wcsdup(Params[0].toString());
	StrUpper(Val);
	PassString(Val);
	free(Val);
	return 0;
}

int FarMacroApi::lcaseFunc()
{
	auto Params = parseParams(1);
	wchar_t* Val = wcsdup(Params[0].toString());
	StrLower(Val);
	PassString(Val);
	free(Val);
	return 0;
}

int FarMacroApi::stringFunc()
{
	auto Params = parseParams(1);
	auto& Val = Params[0];
	Val.toString();
	return PassValue(Val);
}

// S=StrWrap(Text,Width[,Break[,Flags]])
int FarMacroApi::strwrapFunc()
{
	auto Params = parseParams(4);
	auto& Text  = Params[0];
	int Width   = (int)Params[1].asInteger();
	auto& Break = Params[2];
	//DWORD Flags = (DWORD)Params[3].asInteger();

	if (!Break.isInteger() && !Break.asInteger())
	{
		Break=L"";
		Break.toString();
	}

	const wchar_t* pBreak = *Break.s()==0 ? L"\n" : Break.s();
	FARString strDest;
	FarFormatText(Text.toString(), Width,strDest, pBreak, 1); // 1 == FFTM_BREAKLONGWORD
	return PassString(strDest);
}

int FarMacroApi::intFunc()
{
	auto Params = parseParams(1);
	auto& Val = Params[0];
	Val.toInteger();
	return PassValue(Val);
}

int FarMacroApi::floatFunc()
{
	auto Params = parseParams(1);
	auto& Val = Params[0];
	Val.toDouble();
	return PassValue(Val);
}

int FarMacroApi::absFunc()
{
	auto Params = parseParams(1);

	TVar Result;

	switch(Params[0].type())
	{
	case vtInteger:
		{
			auto i = Params[0].asInteger();
			if (i < 0)
				Result = -i;
			else
				Result = Params[0];
		}
		break;

	case vtDouble:
		{
			auto d = Params[0].asDouble();
			if (d < 0)
				Result = -d;
			else
				Result = Params[0];
		}
		break;

	default:
		break;
	}

	return PassValue(Result);
}

int FarMacroApi::ascFunc()
{
	auto Params = parseParams(1);
	auto& tmpVar = Params[0];

	if (tmpVar.isString())
	{
		tmpVar = (int64_t)((DWORD)((WORD)*tmpVar.toString()));
		tmpVar.toInteger();
	}
	return PassValue(tmpVar);
}

// N=FMatch(S,Mask)
int FarMacroApi::fmatchFunc()
{
	auto Params = parseParams(2);
	auto& Mask(Params[1]);
	auto& S(Params[0]);
	CFileMask FileMask;

	if (FileMask.Set(Mask.toString(), FMF_SILENT))
		PassNumber(FileMask.Compare(S.toString()));
	else
		PassNumber(-1);
	return 0;
}

// V=Editor.Sel(Action[,Opt])
int FarMacroApi::editorselFunc()
{
	/*
	 MCODE_F_EDITOR_SEL
	  Action: 0 = Get Param
	              Opt:  0 = return FirstLine
	                    1 = return FirstPos
	                    2 = return LastLine
	                    3 = return LastPos
	                    4 = return block type (0=nothing 1=stream, 2=column)
	              return: 0 = failure, 1... request value

	          1 = Set Pos
	              Opt:  0 = begin block (FirstLine & FirstPos)
	                    1 = end block (LastLine & LastPos)
	              return: 0 = failure, 1 = success

	          2 = Set Stream Selection Edge
	              Opt:  0 = selection start
	                    1 = selection finish
	              return: 0 = failure, 1 = success

	          3 = Set Column Selection Edge
	              Opt:  0 = selection start
	                    1 = selection finish
	              return: 0 = failure, 1 = success
	          4 = Unmark selected block
	              Opt: ignore
	              return 1
	*/
	auto Params = parseParams(2);
	TVar Ret = 0;
	auto& Opt = Params[1];
	auto& Action = Params[0];
	int Mode=CtrlObject->Macro.GetArea();
	Frame* CurFrame=FrameManager->GetCurrentFrame();
	int NeedType = Mode == MACROAREA_EDITOR?MODALTYPE_EDITOR:(Mode == MACROAREA_VIEWER?MODALTYPE_VIEWER:(Mode == MACROAREA_DIALOG?MODALTYPE_DIALOG:MODALTYPE_PANELS)); // MACROAREA_SHELL?

	if (CurFrame && CurFrame->GetType()==NeedType)
	{
		if (Mode==MACROAREA_SHELL && CtrlObject->CmdLine->IsVisible())
			Ret=CtrlObject->CmdLine->VMProcess(MCODE_F_EDITOR_SEL,(void*)Action.toInteger(),Opt.i());
		else
			Ret=CurFrame->VMProcess(MCODE_F_EDITOR_SEL,(void*)Action.toInteger(),Opt.i());
	}

	return PassValue(Ret);
}

// V=Editor.Undo(N)
int FarMacroApi::editorundoFunc()
{
	auto Params = parseParams(1);
	auto& Action = Params[0];
	TVar Ret = 0;

	if (CtrlObject->Macro.GetArea()==MACROAREA_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
	{
		EditorUndoRedo eur;
		eur.Command=(int)Action.toInteger();
		Ret=CtrlObject->Plugins.CurEditor->EditorControl(ECTL_UNDOREDO,&eur);
	}

	return Ret.i() ? 1:0;
}

// N=Editor.SetTitle([Title])
int FarMacroApi::editorsettitleFunc()
{
	auto Params = parseParams(1);
	auto& Title = Params[0];
	TVar Ret = 0;

	if (CtrlObject->Macro.GetArea()==MACROAREA_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
	{
		if (Title.isInteger() && !Title.i())
		{
			Title=L"";
			Title.toString();
		}
		Ret=CtrlObject->Plugins.CurEditor->EditorControl(ECTL_SETTITLE,(void*)Title.s());
	}

	return Ret.i() ? 1:0;
}

// N=Plugin.Exist(SysId)
int FarMacroApi::pluginexistFunc()
{
	bool Ret = false;
	if (mData->Count>0 && mData->Values[0].Type==FMVT_DOUBLE)
	{
		if (CtrlObject->Plugins.FindPlugin(static_cast<DWORD>(mData->Values[0].Double)))
			Ret = true;
	}
	return PassBoolean(Ret);
}

// N=testfolder(S)
/*
возвращает одно состояний тестируемого каталога:

TSTFLD_NOTFOUND   (2) - нет такого
TSTFLD_NOTEMPTY   (1) - не пусто
TSTFLD_EMPTY      (0) - пусто
TSTFLD_NOTACCESS (-1) - нет доступа
TSTFLD_ERROR     (-2) - ошибка (кривые параметры или нехватило памяти для выделения промежуточных буферов)
*/
int FarMacroApi::testfolderFunc()
{
	auto Params = parseParams(1);
	auto& tmpVar = Params[0];
	int Ret=TSTFLD_ERROR;

	if (tmpVar.isString())
	{
		Ret=TestFolder(tmpVar.s());
		switch(Ret) // перекодируем в значения MacroAPI Far3
		{
			case 0: Ret=1; break;
			case 1: Ret=2; break;
			case 2: Ret=0; break;
		}
	}
	return Ret;
}

/*
Res=kbdLayout([N])

Параметр N:
а) конкретика: 0x0409 или 0x0419 или...
б) 1 - следующую системную (по кругу)
в) -1 - предыдущую системную (по кругу)
г) 0 или не указан - вернуть текущую раскладку.

Возвращает предыдущую раскладку (для N=0 текущую)
*/
// N=kbdLayout([N])
int FarMacroApi::kbdLayoutFunc()
{
	//auto Params = parseParams(1);
	//DWORD dwLayout = (DWORD)Params[0].getInteger();

	BOOL Ret=TRUE;
	HKL  RetLayout=(HKL)0; //Layout=(HKL)0,

	return PassValue(Ret?TVar(static_cast<INT64>(reinterpret_cast<INT_PTR>(RetLayout))):0);
}

//### temporary function, for test only
int FarMacroApi::UDList_Split()
{
	auto Params = parseParams(2);
	auto Flags = (unsigned)Params[0].getInteger();
	auto Subj = Params[1].toString();

	UserDefinedList udl(Flags);
	if (udl.Set(Subj) && udl.Size())
	{
		const wchar_t* str;
		for (int i=0; (str=udl.Get(i)); i++)
			PassString(str);
	}
	else
	{
		PassBoolean(0);
	}
	return 0;
}

static DWORD LayoutKey(DWORD key)
{
	if ((key&0x00FFFFFF) > 0x7F && (key&0x00FFFFFF) < 0xFFFF)
		key = KeyToKeyLayout(key&0x0000FFFF) | (key&(~0x0000FFFF));
	return key;
}

bool KeyMacro::ProcessKey(FarKey dwKey)
{
	if (m_InternalInput || dwKey==KEY_IDLE || dwKey==KEY_NONE || !FrameManager->GetCurrentFrame())
		return false;

	FARString textKey;
	if (!KeyToText(dwKey, textKey) || textKey.IsEmpty())
		return false;

	const bool ctrldot = dwKey == Opt.Macro.KeyMacroCtrlDot;
	const bool ctrlshiftdot = dwKey == Opt.Macro.KeyMacroCtrlShiftDot;

	if (m_Recording == MACROSTATE_NOMACRO)
	{
		if ((ctrldot||ctrlshiftdot) && !IsExecuting())
		{
			if (!LuafarLoaded || !CtrlObject->Plugins.FindPlugin(SYSID_LUAMACRO)) //### || LuaMacro->IsPendingRemove())
			{
					Message(MSG_WARNING,1,Msg::Error,
					   Msg::MacroPluginLuamacroNotLoaded,
					   Msg::MacroRecordingIsDisabled,
					   Msg::HOk);
					return false;
			}

			// Где мы?
			m_StartMode=m_Area;
			// В зависимости от того, КАК НАЧАЛИ писать макрос, различаем общий режим (Ctrl-.
			// с передачей плагину кеев) или специальный (Ctrl-Shift-. - без передачи клавиш плагину)
			m_Recording=ctrldot?MACROSTATE_RECORDING_COMMON:MACROSTATE_RECORDING;

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

				if (key<0xFFFF)
					key=Upper(static_cast<wchar_t>(key));

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
	else // m_Recording!=MACROSTATE_NOMACRO
	{
		if (ctrldot||ctrlshiftdot) // признак конца записи?
		{
			m_InternalInput=1;
			DWORD MacroKey;
			// выставляем флаги по умолчанию.
			DWORD Flags = 0;
			int AssignRet = AssignMacroKey(MacroKey,Flags);

			if (AssignRet == AMK_MODIFY && !m_RecCode.IsEmpty())
			{
				m_RecCode = L"Keys(\"" + m_RecCode + L"\")";
				if (ctrlshiftdot && !GetMacroSettings(MacroKey,Flags))
				{
					AssignRet = AMK_CANCEL;
				}
			}
			m_InternalInput=0;
			if (AssignRet == AMK_MODIFY || AssignRet == AMK_DELETE)
			{
				FARString strKey;
				KeyToText(MacroKey, strKey);
				Flags |= m_Recording == MACROSTATE_RECORDING_COMMON? MFLAGS_NONE : MFLAGS_NOSENDKEYSTOPLUGINS;
				LM_ProcessRecordedMacro(m_StartMode, strKey, m_RecCode, Flags, m_RecDescription);
			}

			m_Recording=MACROSTATE_NOMACRO;
			m_RecCode.Clear();
			m_RecDescription.Clear();
			ScrBuf.Flush(true);

			if (Opt.AutoSaveSetup)
				SaveMacros(false); // записать только изменения!

			return true;
		}
		else
		{
			if (!IsProcessAssignMacroKey)
			{
				if (!m_RecCode.IsEmpty())
					m_RecCode += L' ';

				m_RecCode += textKey == L"\""? L"\\\"" : textKey;
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
				if (m_Area==MACROAREA_EDITOR &&
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

			case MPRT_PLUGINMENU:   // N=Plugin.Menu(Uuid[,MenuUuid])
			case MPRT_PLUGINCONFIG: // N=Plugin.Config(Uuid[,MenuUuid])
			case MPRT_PLUGINCOMMAND: // N=Plugin.Command(Uuid[,Command])
			{
				SetMacroValue(false);

				if (!mpr.Count || mpr.Values[0].Type != FMVT_DOUBLE)
					break;

				DWORD SysID = static_cast<DWORD>(mpr.Values[0].Double);
				if (!CtrlObject->Plugins.FindPlugin(SysID))
					break;

				PluginManager::CallPluginInfo cpInfo = { CPT_CHECKONLY };
				if (mpr.ReturnType==MPRT_PLUGINMENU || mpr.ReturnType==MPRT_PLUGINCONFIG)
				{
					cpInfo.ItemUuid = mpr.Count > 1 && mpr.Values[1].Type == FMVT_DOUBLE ?
						static_cast<DWORD>(mpr.Values[1].Double) : 0;
				}

				if (mpr.ReturnType == MPRT_PLUGINMENU)
					cpInfo.CallFlags |= CPT_MENU;
				else if (mpr.ReturnType == MPRT_PLUGINCONFIG)
					cpInfo.CallFlags |= CPT_CONFIGURE;
				else if (mpr.ReturnType == MPRT_PLUGINCOMMAND)
				{
					cpInfo.CallFlags |= CPT_CMDLINE;
					cpInfo.Command = mpr.Count > 1 && mpr.Values[1].Type == FMVT_STRING ? mpr.Values[1].String : L"";
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
				FrameManager->PluginCommit();

				break;
			}

			//~ case MPRT_USERMENU:
				//~ ShowUserMenu(mpr.Count,mpr.Values);
				//~ break;
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
	FarMacroValue values[]={StrArea.CPtr(),!Pos};
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_ENUMMACROS,&fmc};

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

// Функция, запускающая макросы при старте ФАРа
void KeyMacro::RunStartMacro()
{
	if (Opt.Macro.DisableMacro & (MDOL_ALL|MDOL_AUTOSTART))
		return;

	if (!CtrlObject || !CtrlObject->Cp() || !CtrlObject->Cp()->ActivePanel || !CtrlObject->Plugins.IsPluginsLoaded())
		return;

	static bool IsRunStartMacro=false, IsInside=false;

	if (!IsRunStartMacro && !IsInside)
	{
		IsInside = true;
		OpenMacroPluginInfo info = {MCT_RUNSTARTMACRO,nullptr};
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
	FarMacroCall fmc = {sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info = {MCT_ADDMACRO,&fmc};
	return CallMacroPlugin(&info);
}

bool KeyMacro::DelMacro(DWORD PluginId, void* Id)
{
	FarMacroValue values[]={PluginId,Id};
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_DELMACRO,&fmc};
	return CallMacroPlugin(&info);
}

// обработчик диалогового окна назначения клавиши
LONG_PTR WINAPI KeyMacro::AssignMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	DWORD dw;
	FARString strKeyText;
	static int LastKey=0;
	static DlgParam *KMParam=nullptr;

	if (Msg == DN_INITDIALOG)
	{
		KMParam=reinterpret_cast<DlgParam*>(Param2);
		LastKey=0;
		// <Клавиши, которые не введешь в диалоге назначения>
		DWORD PreDefKeyMain[]=
		{
			KEY_CTRLDOWN,KEY_ENTER,KEY_NUMENTER,KEY_ESC,KEY_F1,KEY_CTRLF5,
		};

		for (size_t i=0; i<ARRAYSIZE(PreDefKeyMain); i++)
		{
			KeyToText(PreDefKeyMain[i],strKeyText);
			SendDlgMessage(hDlg,DM_LISTADDSTR,2,reinterpret_cast<LONG_PTR>(strKeyText.CPtr()));
		}

		DWORD PreDefKey[]=
		{
			KEY_MSWHEEL_UP,KEY_MSWHEEL_DOWN,KEY_MSWHEEL_LEFT,KEY_MSWHEEL_RIGHT,
			KEY_MSLCLICK,KEY_MSRCLICK,KEY_MSM1CLICK,KEY_MSM2CLICK,KEY_MSM3CLICK,
#if 0
			KEY_MSLDBLCLICK,KEY_MSRDBLCLICK,KEY_MSM1DBLCLICK,KEY_MSM2DBLCLICK,KEY_MSM3DBLCLICK,
#endif
		};
		DWORD PreDefModKey[]=
		{
			0,KEY_CTRL,KEY_SHIFT,KEY_ALT,KEY_CTRLSHIFT,KEY_CTRLALT,KEY_ALTSHIFT,
		};

		for (size_t i=0; i<ARRAYSIZE(PreDefKey); i++)
		{
			SendDlgMessage(hDlg,DM_LISTADDSTR,2,reinterpret_cast<LONG_PTR>(L"\1"));

			for (size_t j=0; j<ARRAYSIZE(PreDefModKey); j++)
			{
				KeyToText(PreDefKey[i]|PreDefModKey[j],strKeyText);
				SendDlgMessage(hDlg,DM_LISTADDSTR,2,reinterpret_cast<LONG_PTR>(strKeyText.CPtr()));
			}
		}

		SendDlgMessage(hDlg,DM_SETTEXTPTR,2,reinterpret_cast<LONG_PTR>(L""));
		// </Клавиши, которые не введешь в диалоге назначения>
	}
	else if (Param1 == 2 && Msg == DN_EDITCHANGE)
	{
		LastKey = 0;
		auto KeyCode = KeyNameToKey(((FarDialogItem*)Param2)->PtrData);

		if (KeyCode != KEY_INVALID && KMParam && !KMParam->Recurse)
		{
			Param2 = KeyCode;
			goto M1;
		}
	}
	else if (Msg == DN_KEY && (dw = (Param2 & KEY_END_SKEY), dw < KEY_END_FKEY ||
	                           (dw > INTERNAL_KEY_BASE && dw < INTERNAL_KEY_BASE_2)))
	{
		// <Обработка особых клавиш: F1 & Enter>
		// Esc & (Enter и предыдущий Enter) - не обрабатываем
		if (Param2 == KEY_ESC ||
		        ((Param2 == KEY_ENTER||Param2 == KEY_NUMENTER) && (LastKey == KEY_ENTER||LastKey == KEY_NUMENTER)) ||
		        Param2 == KEY_CTRLDOWN ||
		        Param2 == KEY_F1)
		{
			return FALSE;
		}

		// Было что-то уже нажато и Enter`ом подтверждаем
		if ((Param2 == KEY_ENTER||Param2 == KEY_NUMENTER) && LastKey && !(LastKey == KEY_ENTER||LastKey == KEY_NUMENTER))
			return FALSE;

		// </Обработка особых клавиш: F1 & Enter>
M1:
		Param2 = LayoutKey(Param2);

		//косметика
		if (Param2<0xFFFF)
			Param2=Upper((wchar_t)Param2);

		KMParam->Key=(DWORD)Param2;
		KeyToText((uint32_t)Param2,strKeyText);

		// если УЖЕ есть такой макрос...
		GetMacroData Data;
		if (LM_GetMacro(&Data,KMParam->Area,strKeyText,true) && Data.IsKeyboardMacro)
		{
			// общие макросы учитываем только при удалении.
			if (m_RecCode.IsEmpty() || Data.Area!=MACROAREA_COMMON)
			{
				FARString strBufKey;
				bool SetDelete = m_RecCode.IsEmpty();
				if (Data.Code)
				{
					strBufKey=Data.Code;
					InsertQuote(strBufKey);
				}

				FARString strBuf;
				if (Data.Area==MACROAREA_COMMON)
					strBuf.Format(SetDelete ? Msg::MacroCommonDeleteKey : Msg::MacroCommonReDefinedKey, strKeyText.CPtr());
				else
					strBuf.Format(SetDelete ? Msg::MacroDeleteKey : Msg::MacroReDefinedKey, strKeyText.CPtr());

				int	Result = SetDelete ?
					Message(MSG_WARNING,3,Msg::Warning,strBuf,Msg::MacroSequence,strBufKey,
							Msg::MacroDeleteKey2,
							Msg::Yes, Msg::MacroEditKey, Msg::No) :
					Message(MSG_WARNING,2,Msg::Warning,strBuf,Msg::MacroSequence,strBufKey,
							Msg::MacroReDefinedKey2,
							Msg::Yes, Msg::No);

				if (Result == 0)
				{
					SendDlgMessage(hDlg,DM_CLOSE,1,0);
					return TRUE;
				}

				if (SetDelete && Result == 1)
				{
					auto key = (DWORD)Param2;
					FARString strDescription;

					if ( *Data.Code )
						strBufKey=Data.Code;

					if ( *Data.Description )
						strDescription=Data.Description;

					if (GetMacroSettings(key, Data.Flags, strBufKey, strDescription))
					{
						KMParam->Flags = Data.Flags;
						KMParam->Deleted = true;
						SendDlgMessage(hDlg, DM_CLOSE, 1, 0);
						return TRUE;
					}
				}

				// здесь - здесь мы нажимали "Нет", ну а на нет и суда нет
				//  и значит очистим поле ввода.
				strKeyText.Clear();
			}
		}

		KMParam->Recurse++;
		SendDlgMessage(hDlg,DM_SETTEXTPTR,2,(LONG_PTR)strKeyText.CPtr());
		KMParam->Recurse--;
		LastKey=(int)Param2;
		return TRUE;
	}
	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

int KeyMacro::AssignMacroKey(FarKey& MacroKey, DWORD& Flags)
{
	/*
	  +------ Define macro ------+
	  | Press the desired key    |
	  | ________________________ |
	  +--------------------------+
	*/
	DialogDataEx MacroAssignDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,30,4,{},0,Msg::DefineMacroTitle},
		{DI_TEXT,-1,2,0,2,{},0,Msg::DefineMacro},
		{DI_COMBOBOX,5,3,28,3,{},DIF_FOCUS|DIF_DEFAULT,L""}
	};
	MakeDialogItemsEx(MacroAssignDlgData,MacroAssignDlg);
	DlgParam Param={Flags, 0, m_StartMode, false};
	IsProcessAssignMacroKey++;
	Dialog Dlg(MacroAssignDlg,ARRAYSIZE(MacroAssignDlg),AssignMacroDlgProc,(LONG_PTR)&Param);
	Dlg.SetPosition(-1,-1,34,6);
	Dlg.SetHelp(L"KeyMacro");
	Dlg.Process();
	IsProcessAssignMacroKey--;

	if (Dlg.GetExitCode() == -1)
		return AMK_CANCEL;

	MacroKey = Param.Key;
	Flags = Param.Flags;
	return Param.Deleted ? AMK_DELETE : AMK_MODIFY;
}

static int Set3State(DWORD Flags,DWORD Chk1,DWORD Chk2)
{
	bool b1 = (Flags & Chk1) != 0;
	bool b2 = (Flags & Chk2) != 0;
	return b1==b2 ? 2 : b1 ? 1 : 0;
}

static DWORD Get3State(int Selected,DWORD Chk1,DWORD Chk2)
{
	return Selected==2 ? 0 : Selected==0 ? Chk1 : Chk2;
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

LONG_PTR WINAPI KeyMacro::ParamMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	switch (Msg)
	{
		case DN_BTNCLICK:

			if (Param1==MS_CHECKBOX_A_PANEL || Param1==MS_CHECKBOX_P_PANEL)
				for (int i=1; i<=3; i++)
					SendDlgMessage(hDlg,DM_ENABLE,Param1+i,Param2);

			break;
		case DN_CLOSE:

			if (Param1==MS_BUTTON_OK)
			{
				LPCWSTR Sequence=(LPCWSTR)SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,MS_EDIT_SEQUENCE,0);

				if (*Sequence)
				{
					if (ParseMacroString(Sequence,KMFLAGS_LUA,true))
					{
						m_RecCode=Sequence;
						m_RecDescription = (LPCWSTR)SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,MS_EDIT_DESCR,0);
						return TRUE;
					}
				}
				return FALSE;
			}

			break;
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

int KeyMacro::GetMacroSettings(FarKey Key,DWORD &Flags, const wchar_t* Src, const wchar_t* Descr)
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
		{DI_DOUBLEBOX,3,1,69,19,{},0,L""},
		{DI_TEXT,5,2,0,2,{},0,Msg::MacroSequence},
		{DI_EDIT,5,3,67,3,{reinterpret_cast<DWORD_PTR>(L"MacroSequence")},DIF_HISTORY|DIF_FOCUS,L""},
		{DI_TEXT,5,4,0,4,{},0,Msg::MacroDescription},
		{DI_EDIT,5,5,67,5,{reinterpret_cast<DWORD_PTR>(L"MacroDescription")},DIF_HISTORY,L""},
		{DI_TEXT,3,6,0,6,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,5,7,0,7,{},0,Msg::MacroSettingsEnableOutput},
		{DI_CHECKBOX,5,8,0,8,{},0,Msg::MacroSettingsRunAfterStart},
		{DI_TEXT,3,9,0,9,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,5,10,0,10,{},0,Msg::MacroSettingsActivePanel},
		{DI_CHECKBOX,7,11,0,11,{2},DIF_3STATE,Msg::MacroSettingsPluginPanel},
		{DI_CHECKBOX,7,12,0,12,{2},DIF_3STATE,Msg::MacroSettingsFolders},
		{DI_CHECKBOX,7,13,0,13,{2},DIF_3STATE,Msg::MacroSettingsSelectionPresent},
		{DI_CHECKBOX,37,10,0,10,{},0,Msg::MacroSettingsPassivePanel},
		{DI_CHECKBOX,39,11,0,11,{2},DIF_3STATE,Msg::MacroSettingsPluginPanel},
		{DI_CHECKBOX,39,12,0,12,{2},DIF_3STATE,Msg::MacroSettingsFolders},
		{DI_CHECKBOX,39,13,0,13,{2},DIF_3STATE,Msg::MacroSettingsSelectionPresent},
		{DI_TEXT,3,14,0,14,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,5,15,0,15,{2},DIF_3STATE,Msg::MacroSettingsCommandLine},
		{DI_CHECKBOX,5,16,0,16,{2},DIF_3STATE,Msg::MacroSettingsSelectionBlockPresent},
		{DI_TEXT,3,17,0,17,{},DIF_SEPARATOR,L""},
		{DI_BUTTON,0,18,0,18,{},DIF_DEFAULT|DIF_CENTERGROUP,Msg::Ok},
		{DI_BUTTON,0,18,0,18,{},DIF_CENTERGROUP,Msg::Cancel}
	};
	MakeDialogItemsEx(MacroSettingsDlgData,MacroSettingsDlg);
	FARString strKeyText;
	KeyToText(Key,strKeyText);
	MacroSettingsDlg[MS_DOUBLEBOX].strData.Format(Msg::MacroSettingsTitle, strKeyText.CPtr());
	MacroSettingsDlg[MS_CHECKBOX_OUTPUT].Selected=Flags&MFLAGS_ENABLEOUTPUT?1:0;
	MacroSettingsDlg[MS_CHECKBOX_START].Selected=Flags&MFLAGS_RUNAFTERFARSTART?1:0;

	int a = Set3State(Flags,MFLAGS_NOFILEPANELS,MFLAGS_NOPLUGINPANELS);
	int b = Set3State(Flags,MFLAGS_NOFILES,MFLAGS_NOFOLDERS);
	int c = Set3State(Flags,MFLAGS_SELECTION,MFLAGS_NOSELECTION);
	MacroSettingsDlg[MS_CHECKBOX_A_PLUGINPANEL].Selected = a;
	MacroSettingsDlg[MS_CHECKBOX_A_FOLDERS].Selected = b;
	MacroSettingsDlg[MS_CHECKBOX_A_SELECTION].Selected = c;
	MacroSettingsDlg[MS_CHECKBOX_A_PANEL].Selected = (a!=2 || b!=2 || c!= 2) ? 1 : 0;
	if (0 == MacroSettingsDlg[MS_CHECKBOX_A_PANEL].Selected)
	{
		MacroSettingsDlg[MS_CHECKBOX_A_PLUGINPANEL].Flags |= DIF_DISABLE;
		MacroSettingsDlg[MS_CHECKBOX_A_FOLDERS].Flags |= DIF_DISABLE;
		MacroSettingsDlg[MS_CHECKBOX_A_SELECTION].Flags |= DIF_DISABLE;
	}

	a = Set3State(Flags,MFLAGS_PNOFILEPANELS,MFLAGS_PNOPLUGINPANELS);
	b = Set3State(Flags,MFLAGS_PNOFILES,MFLAGS_PNOFOLDERS);
	c = Set3State(Flags,MFLAGS_PSELECTION,MFLAGS_PNOSELECTION);
	MacroSettingsDlg[MS_CHECKBOX_P_PLUGINPANEL].Selected = a;
	MacroSettingsDlg[MS_CHECKBOX_P_FOLDERS].Selected = b;
	MacroSettingsDlg[MS_CHECKBOX_P_SELECTION].Selected = c;
	MacroSettingsDlg[MS_CHECKBOX_P_PANEL].Selected = (a!=2 || b!=2 || c!= 2) ? 1 : 0;
	if (0 == MacroSettingsDlg[MS_CHECKBOX_P_PANEL].Selected)
	{
		MacroSettingsDlg[MS_CHECKBOX_P_PLUGINPANEL].Flags |= DIF_DISABLE;
		MacroSettingsDlg[MS_CHECKBOX_P_FOLDERS].Flags |= DIF_DISABLE;
		MacroSettingsDlg[MS_CHECKBOX_P_SELECTION].Flags |= DIF_DISABLE;
	}

	MacroSettingsDlg[MS_CHECKBOX_CMDLINE].Selected=Set3State(Flags,MFLAGS_EMPTYCOMMANDLINE,MFLAGS_NOTEMPTYCOMMANDLINE);
	MacroSettingsDlg[MS_CHECKBOX_SELBLOCK].Selected=Set3State(Flags,MFLAGS_EDITSELECTION,MFLAGS_EDITNOSELECTION);
	MacroSettingsDlg[MS_EDIT_SEQUENCE].strData = *Src ? Src : m_RecCode.CPtr();
	MacroSettingsDlg[MS_EDIT_DESCR].strData = *Descr ? Descr : m_RecDescription.CPtr();
	DlgParam Param={0, 0, MACROAREA_OTHER, 0, false};
	Dialog Dlg(MacroSettingsDlg,ARRAYSIZE(MacroSettingsDlg),ParamMacroDlgProc,(LONG_PTR)&Param);
	Dlg.SetPosition(-1,-1,73,21);
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
		return FALSE;

	Flags=MacroSettingsDlg[MS_CHECKBOX_OUTPUT].Selected?MFLAGS_ENABLEOUTPUT:0;
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
	return TRUE;
}

bool KeyMacro::PostNewMacro(const wchar_t* Sequence, DWORD InputFlags, FarKey AKey)
{
	const wchar_t* Lang = GetMacroLanguage(InputFlags);
	const auto onlyCheck = (InputFlags & KMFLAGS_SILENTCHECK) != 0;
	MACROFLAGS_MFLAGS Flags = MFLAGS_POSTFROMPLUGIN;
	if (InputFlags & KMFLAGS_ENABLEOUTPUT)        Flags |= MFLAGS_ENABLEOUTPUT;
	if (InputFlags & KMFLAGS_NOSENDKEYSTOPLUGINS) Flags |= MFLAGS_NOSENDKEYSTOPLUGINS;

	FarMacroValue values[] = { 7.0, Lang, Sequence, static_cast<double>(Flags), static_cast<double>(AKey), onlyCheck };
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_KEYMACRO,&fmc};
	return CallMacroPlugin(&info);
}

bool KeyMacro::CheckEditSelected(DWORD CurFlags)
{
	if (m_Area==MACROAREA_EDITOR || m_Area==MACROAREA_DIALOG || m_Area==MACROAREA_VIEWER ||
		(m_Area==MACROAREA_SHELL && CtrlObject->CmdLine->IsVisible()))
	{
		int NeedType = m_Area == MACROAREA_EDITOR ? MODALTYPE_EDITOR :
			(m_Area == MACROAREA_VIEWER ? MODALTYPE_VIEWER :
			(m_Area == MACROAREA_DIALOG ? MODALTYPE_DIALOG : MODALTYPE_PANELS));
		Frame* CurFrame=FrameManager->GetCurrentFrame();

		if (CurFrame && CurFrame->GetType()==NeedType)
		{
			int CurSelected;

			if (m_Area==MACROAREA_SHELL && CtrlObject->CmdLine->IsVisible())
				CurSelected=(int)CtrlObject->CmdLine->VMProcess(MCODE_C_SELECTED);
			else
				CurSelected=(int)CurFrame->VMProcess(MCODE_C_SELECTED);

			if (((CurFlags&MFLAGS_EDITSELECTION) && !CurSelected) ||	((CurFlags&MFLAGS_EDITNOSELECTION) && CurSelected))
				return false;
		}
	}

	return true;
}

bool KeyMacro::CheckInsidePlugin(DWORD CurFlags)
{
	if (CtrlObject && CtrlObject->Plugins.CurPluginItem && (CurFlags&MFLAGS_NOSENDKEYSTOPLUGINS)) // ?????
		//if(CtrlObject && CtrlObject->Plugins.CurEditor && (CurFlags&MFLAGS_NOSENDKEYSTOPLUGINS))
		return false;

	return true;
}

bool KeyMacro::CheckCmdLine(int CmdLength,DWORD CurFlags)
{
	if (((CurFlags&MFLAGS_EMPTYCOMMANDLINE) && CmdLength) || ((CurFlags&MFLAGS_NOTEMPTYCOMMANDLINE) && CmdLength==0))
		return false;

	return true;
}

bool KeyMacro::CheckPanel(int PanelMode,DWORD CurFlags,bool IsPassivePanel)
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

bool KeyMacro::CheckFileFolder(Panel *CheckPanel,DWORD CurFlags, bool IsPassivePanel)
{
	FARString strFileName;
	DWORD FileAttr=INVALID_FILE_ATTRIBUTES;
	CheckPanel->GetFileName(strFileName,CheckPanel->GetCurrentPos(),FileAttr);

	if (FileAttr != INVALID_FILE_ATTRIBUTES)
	{
		if (IsPassivePanel)
		{
			if (((FileAttr&FILE_ATTRIBUTE_DIRECTORY) && (CurFlags&MFLAGS_PNOFOLDERS)) || (!(FileAttr&FILE_ATTRIBUTE_DIRECTORY) && (CurFlags&MFLAGS_PNOFILES)))
				return false;
		}
		else
		{
			if (((FileAttr&FILE_ATTRIBUTE_DIRECTORY) && (CurFlags&MFLAGS_NOFOLDERS)) || (!(FileAttr&FILE_ATTRIBUTE_DIRECTORY) && (CurFlags&MFLAGS_NOFILES)))
				return false;
		}
	}

	return true;
}

bool KeyMacro::CheckAll(int /*CheckMode*/,DWORD CurFlags)
{
	/* $TODO:
		Здесь вместо Check*() попробовать заюзать IfCondition()
		для исключения повторяющегося кода.
	*/
	if (!CheckInsidePlugin(CurFlags))
		return false;

	// проверка на пусто/не пусто в ком.строке (а в редакторе? :-)
	if (CurFlags&(MFLAGS_EMPTYCOMMANDLINE|MFLAGS_NOTEMPTYCOMMANDLINE))
		if (CtrlObject->CmdLine && !CheckCmdLine(CtrlObject->CmdLine->GetLength(),CurFlags))
			return false;

	FilePanels *Cp=CtrlObject->Cp();

	if (!Cp)
		return false;

	// проверки панели и типа файла
	Panel *ActivePanel=Cp->ActivePanel;
	Panel *PassivePanel=Cp->GetAnotherPanel(Cp->ActivePanel);

	if (ActivePanel && PassivePanel)// && (CurFlags&MFLAGS_MODEMASK)==MACROAREA_SHELL)
	{
		if (CurFlags&(MFLAGS_NOPLUGINPANELS|MFLAGS_NOFILEPANELS))
			if (!CheckPanel(ActivePanel->GetMode(),CurFlags,false))
				return false;

		if (CurFlags&(MFLAGS_PNOPLUGINPANELS|MFLAGS_PNOFILEPANELS))
			if (!CheckPanel(PassivePanel->GetMode(),CurFlags,true))
				return false;

		if (CurFlags&(MFLAGS_NOFOLDERS|MFLAGS_NOFILES))
			if (!CheckFileFolder(ActivePanel,CurFlags,false))
				return false;

		if (CurFlags&(MFLAGS_PNOFOLDERS|MFLAGS_PNOFILES))
			if (!CheckFileFolder(PassivePanel,CurFlags,true))
				return false;

		if (CurFlags&(MFLAGS_SELECTION|MFLAGS_NOSELECTION|MFLAGS_PSELECTION|MFLAGS_PNOSELECTION))
			if (m_Area!=MACROAREA_EDITOR && m_Area != MACROAREA_DIALOG && m_Area!=MACROAREA_VIEWER)
			{
				int SelCount=ActivePanel->GetRealSelCount();

				if (((CurFlags&MFLAGS_SELECTION) && SelCount < 1) || ((CurFlags&MFLAGS_NOSELECTION) && SelCount >= 1))
					return false;

				SelCount=PassivePanel->GetRealSelCount();

				if (((CurFlags&MFLAGS_PSELECTION) && SelCount < 1) || ((CurFlags&MFLAGS_PNOSELECTION) && SelCount >= 1))
					return false;
			}
	}

	if (!CheckEditSelected(CurFlags))
		return false;

	return true;
}

bool KeyMacro::CheckWaitKeyFunc() const
{
	return m_WaitKey != 0;
}
