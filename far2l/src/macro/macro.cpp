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
#include <farplug-wide.h>
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

static void Log(const char* str)
{
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
        fprintf(fp, "%d: %s\n", N, str);
        fclose(fp);
      }
      free(buf);
    }
  }
}

typedef unsigned int MACROFLAGS_MFLAGS;
static const MACROFLAGS_MFLAGS
	MFLAGS_NONE                    = 0,
	// public flags, read from/saved to config
	MFLAGS_MODEMASK                = 0x000000FF, // ### этот флаг подлежит удалению в будущем
	MFLAGS_DISABLEOUTPUT           = 0,          // ###
	MFLAGS_NEEDSAVEMACRO           = 0x40000000, // ###
	MFLAGS_DISABLEMACRO            = 0x80000000, // ###

	MFLAGS_ENABLEOUTPUT            = 0x00000001, // не подавлять обновление экрана во время выполнения макроса
	MFLAGS_NOSENDKEYSTOPLUGINS     = 0x00000002, // НЕ передавать плагинам клавиши во время записи/воспроизведения макроса
	MFLAGS_RUNAFTERFARSTART        = 0x00000008, // этот макрос запускается при старте ФАРа
	MFLAGS_EMPTYCOMMANDLINE        = 0x00000010, // запускать, если командная линия пуста
	MFLAGS_NOTEMPTYCOMMANDLINE     = 0x00000020, // запускать, если командная линия не пуста
	MFLAGS_EDITSELECTION           = 0x00000040, // запускать, если есть выделение в редакторе
	MFLAGS_EDITNOSELECTION         = 0x00000080, // запускать, если есть нет выделения в редакторе
	MFLAGS_SELECTION               = 0x00000100, // активная:  запускать, если есть выделение
	MFLAGS_PSELECTION              = 0x00000200, // пассивная: запускать, если есть выделение
	MFLAGS_NOSELECTION             = 0x00000400, // активная:  запускать, если есть нет выделения
	MFLAGS_PNOSELECTION            = 0x00000800, // пассивная: запускать, если есть нет выделения
	MFLAGS_NOFILEPANELS            = 0x00001000, // активная:  запускать, если это плагиновая панель
	MFLAGS_PNOFILEPANELS           = 0x00002000, // пассивная: запускать, если это плагиновая панель
	MFLAGS_NOPLUGINPANELS          = 0x00004000, // активная:  запускать, если это файловая панель
	MFLAGS_PNOPLUGINPANELS         = 0x00008000, // пассивная: запускать, если это файловая панель
	MFLAGS_NOFOLDERS               = 0x00010000, // активная:  запускать, если текущий объект "файл"
	MFLAGS_PNOFOLDERS              = 0x00020000, // пассивная: запускать, если текущий объект "файл"
	MFLAGS_NOFILES                 = 0x00040000, // активная:  запускать, если текущий объект "папка"
	MFLAGS_PNOFILES                = 0x00080000, // пассивная: запускать, если текущий объект "папка"
	MFLAGS_PUBLIC_MASK             = 0x10000000 - 1,
	// private flags, for runtime purposes only
	MFLAGS_PRIVATE_MASK            = ~MFLAGS_PUBLIC_MASK,
	MFLAGS_POSTFROMPLUGIN          = 0x10000000; // последовательность пришла от АПИ

// для диалога назначения клавиши
struct DlgParam
{
	KeyMacro *Handle;
	DWORD Key;
	int Mode;
	int Recurse;
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

static bool ToDouble(long long v, double *d)
{
	if ((v >= 0 && v <= 0x1FFFFFFFFFFFFFLL) || (v < 0 && v >= -0x1FFFFFFFFFFFFFLL))
	{
		*d = (double)v;
		return true;
	}
	return false;
}

static const wchar_t* GetMacroLanguage(FARKEYMACROFLAGS Flags)
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
	int ret;
	int result = CtrlObject->Plugins.CallPlugin(SYSID_LUAMACRO, OPEN_LUAMACRO, Info, &ret) != 0;
	return result && ret;
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
	//print_opcodes();
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

static bool LM_GetMacro(GetMacroData* Data, FARMACROAREA Area, const FARString& TextKey, bool UseCommon)
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

bool KeyMacro::MacroExists(int Key, FARMACROAREA Area, bool UseCommon)
{
	GetMacroData dummy;
	FARString KeyName;
	return KeyToText(Key,KeyName) && LM_GetMacro(&dummy, Area, KeyName, UseCommon);
}

static void LM_ProcessRecordedMacro(int Area, const FARString& TextKey, const FARString& Code,
	MACROFLAGS_MFLAGS Flags, const FARString& Description)
{
	FarMacroValue values[] = { static_cast<double>(Area), TextKey.CPtr(), Code.CPtr(), Flags, Description.CPtr() };
	FarMacroCall fmc={sizeof(FarMacroCall),ARRAYSIZE(values),values,nullptr,nullptr};
	OpenMacroPluginInfo info={MCT_RECORDEDMACRO,&fmc};
	CallMacroPlugin(&info);
}

bool KeyMacro::ParseMacroString(const wchar_t* Sequence, FARKEYMACROFLAGS Flags, bool skipFile) const
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

DWORD KeyMacro::GetMacroParseError(point& ErrPos, FARString& ErrSrc)
{
	MacroPluginReturn Ret;
	if (MacroPluginOp(OP_GETLASTERROR, false, &Ret))
	{
		ErrSrc = Ret.Values[0].String;
		ErrPos.y = static_cast<int>(Ret.Values[1].Double);
		ErrPos.x = static_cast<int>(Ret.Values[2].Double);
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

	int PassBoolean(int b);
	int PassError(const wchar_t* str);
	int PassInteger(long long Int);
	int PassNumber(double dbl);
	int PassString(const wchar_t* str);
	int PassString(const FARString& str);
	int PassValue(const TVar& Var);

	int absFunc();
	int ascFunc();
	int atoiFunc();
	int beepFunc();
	int chrFunc();
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
	int farcfggetFunc();
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
	int menushowFunc();
	int minFunc();
	int modFunc();
	int msgBoxFunc();
	int panelfattrFunc();
	int panelfexistFunc();
	int panelitemFunc();
	int panelselectFunc();
	int panelsetposFunc();
	int panelsetposidxFunc();
	int pluginexistFunc();
	int pluginloadFunc();
	int pluginunloadFunc();
	int promptFunc();
	int replaceFunc();
	int rindexFunc();
	int size2strFunc();
	int sleepFunc();
	int stringFunc();
	int strpadFunc();
	int strwrapFunc();
	int substrFunc();
	int testfolderFunc();
	int trimFunc();
	int ucaseFunc();
	int waitkeyFunc();
	int windowscrollFunc();
	int xlatFunc();

private:
	int fattrFuncImpl(int Type);

	FarMacroCall* mData;
};

int FarMacroApi::PassString(const wchar_t *str)
{
	FarMacroValue val = NullToEmpty(str);
	mData->Callback(mData->CallbackData, &val, 1);
	return 1;
}

int FarMacroApi::PassString(const FARString& str)
{
	return PassString(str.CPtr());
}

int FarMacroApi::PassError(const wchar_t *str)
{
	FarMacroValue val = NullToEmpty(str);
	val.Type = FMVT_ERROR;
	mData->Callback(mData->CallbackData, &val, 1);
	return 1;
}

int FarMacroApi::PassNumber(double dbl)
{
	FarMacroValue val = dbl;
	mData->Callback(mData->CallbackData, &val, 1);
	return 1;
}

int FarMacroApi::PassInteger(long long Int)
{
	FarMacroValue val = Int;
	mData->Callback(mData->CallbackData, &val, 1);
	return 1;
}

int FarMacroApi::PassBoolean(int b)
{
	FarMacroValue val = (b != 0);
	mData->Callback(mData->CallbackData, &val, 1);
	return 1;
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

	mData->Callback(mData->CallbackData, &val, 1);
	return 1;
}

intptr_t KeyMacro::CallFar(intptr_t CheckCode, FarMacroCall* Data)
{
	intptr_t ret=0;
	DWORD FileAttr = INVALID_FILE_ATTRIBUTES;
	FarMacroApi api(Data);

	// проверка на область
	if (CheckCode == 0)
	{
		return api.PassNumber (FrameManager->GetCurrentFrame()->GetMacroMode());
	}

	const auto ActivePanel = CtrlObject->Cp() ? CtrlObject->Cp()->ActivePanel : nullptr;
	const auto PassivePanel = CtrlObject->Cp() ? CtrlObject->Cp()->GetAnotherPanel(ActivePanel) : nullptr;

	auto CurrentWindow = FrameManager->GetCurrentFrame();

	switch (CheckCode)
	{
		//~ case MCODE_F_GETOPTIONS:
		//~ {
			//~ DWORD Options = Opt.OnlyEditorViewerUsed; // bits 0x1 and 0x2
			//~ if (Opt.Macro.DisableMacro&MDOL_ALL)       Options |= 0x4;
			//~ if (Opt.Macro.DisableMacro&MDOL_AUTOSTART) Options |= 0x8;
			//~ if (Opt.ReadOnlyConfig)                    Options |= 0x10;
			//~ api.PassNumber(Options);
			//~ break;
		//~ }
	}
	return 0; //### TODO
}

struct TMacroKeywords
{
	int Type;              // Тип: 0=Area, 1=Flags, 2=Condition
	const wchar_t *Name;   // Наименование
	DWORD Value;           // Значение
	DWORD Reserved;
};

TMacroKeywords MKeywords[] =
{
	{0,  L"Other",              MCODE_C_AREA_OTHER,0},
	{0,  L"Shell",              MCODE_C_AREA_SHELL,0},
	{0,  L"Viewer",             MCODE_C_AREA_VIEWER,0},
	{0,  L"Editor",             MCODE_C_AREA_EDITOR,0},
	{0,  L"Dialog",             MCODE_C_AREA_DIALOG,0},
	{0,  L"Search",             MCODE_C_AREA_SEARCH,0},
	{0,  L"Disks",              MCODE_C_AREA_DISKS,0},
	{0,  L"MainMenu",           MCODE_C_AREA_MAINMENU,0},
	{0,  L"Menu",               MCODE_C_AREA_MENU,0},
	{0,  L"Help",               MCODE_C_AREA_HELP,0},
	{0,  L"Info",               MCODE_C_AREA_INFOPANEL,0},
	{0,  L"QView",              MCODE_C_AREA_QVIEWPANEL,0},
	{0,  L"Tree",               MCODE_C_AREA_TREEPANEL,0},
	{0,  L"FindFolder",         MCODE_C_AREA_FINDFOLDER,0},
	{0,  L"UserMenu",           MCODE_C_AREA_USERMENU,0},
	{0,  L"AutoCompletion",     MCODE_C_AREA_AUTOCOMPLETION,0},

	// ПРОЧЕЕ
	{2,  L"Bof",                MCODE_C_BOF,0},
	{2,  L"Eof",                MCODE_C_EOF,0},
	{2,  L"Empty",              MCODE_C_EMPTY,0},
	{2,  L"Selected",           MCODE_C_SELECTED,0},

	{2,  L"Far.Width",          MCODE_V_FAR_WIDTH,0},
	{2,  L"Far.Height",         MCODE_V_FAR_HEIGHT,0},
	{2,  L"Far.Title",          MCODE_V_FAR_TITLE,0},
	{2,  L"MacroArea",          MCODE_V_MACROAREA,0},

	{2,  L"ItemCount",          MCODE_V_ITEMCOUNT,0},  // ItemCount - число элементов в текущем объекте
	{2,  L"CurPos",             MCODE_V_CURPOS,0},    // CurPos - текущий индекс в текущем объекте
	{2,  L"Title",              MCODE_V_TITLE,0},
	{2,  L"Height",             MCODE_V_HEIGHT,0},
	{2,  L"Width",              MCODE_V_WIDTH,0},

	{2,  L"APanel.Empty",       MCODE_C_APANEL_ISEMPTY,0},
	{2,  L"PPanel.Empty",       MCODE_C_PPANEL_ISEMPTY,0},
	{2,  L"APanel.Bof",         MCODE_C_APANEL_BOF,0},
	{2,  L"PPanel.Bof",         MCODE_C_PPANEL_BOF,0},
	{2,  L"APanel.Eof",         MCODE_C_APANEL_EOF,0},
	{2,  L"PPanel.Eof",         MCODE_C_PPANEL_EOF,0},
	{2,  L"APanel.Root",        MCODE_C_APANEL_ROOT,0},
	{2,  L"PPanel.Root",        MCODE_C_PPANEL_ROOT,0},
	{2,  L"APanel.Visible",     MCODE_C_APANEL_VISIBLE,0},
	{2,  L"PPanel.Visible",     MCODE_C_PPANEL_VISIBLE,0},
	{2,  L"APanel.Plugin",      MCODE_C_APANEL_PLUGIN,0},
	{2,  L"PPanel.Plugin",      MCODE_C_PPANEL_PLUGIN,0},
	{2,  L"APanel.FilePanel",   MCODE_C_APANEL_FILEPANEL,0},
	{2,  L"PPanel.FilePanel",   MCODE_C_PPANEL_FILEPANEL,0},
	{2,  L"APanel.Folder",      MCODE_C_APANEL_FOLDER,0},
	{2,  L"PPanel.Folder",      MCODE_C_PPANEL_FOLDER,0},
	{2,  L"APanel.Selected",    MCODE_C_APANEL_SELECTED,0},
	{2,  L"PPanel.Selected",    MCODE_C_PPANEL_SELECTED,0},
	{2,  L"APanel.Left",        MCODE_C_APANEL_LEFT,0},
	{2,  L"PPanel.Left",        MCODE_C_PPANEL_LEFT,0},
	{2,  L"APanel.LFN",         MCODE_C_APANEL_LFN,0},
	{2,  L"PPanel.LFN",         MCODE_C_PPANEL_LFN,0},
	{2,  L"APanel.Filter",      MCODE_C_APANEL_FILTER,0},
	{2,  L"PPanel.Filter",      MCODE_C_PPANEL_FILTER,0},

	{2,  L"APanel.Type",        MCODE_V_APANEL_TYPE,0},
	{2,  L"PPanel.Type",        MCODE_V_PPANEL_TYPE,0},
	{2,  L"APanel.ItemCount",   MCODE_V_APANEL_ITEMCOUNT,0},
	{2,  L"PPanel.ItemCount",   MCODE_V_PPANEL_ITEMCOUNT,0},
	{2,  L"APanel.CurPos",      MCODE_V_APANEL_CURPOS,0},
	{2,  L"PPanel.CurPos",      MCODE_V_PPANEL_CURPOS,0},
	{2,  L"APanel.Current",     MCODE_V_APANEL_CURRENT,0},
	{2,  L"PPanel.Current",     MCODE_V_PPANEL_CURRENT,0},
	{2,  L"APanel.SelCount",    MCODE_V_APANEL_SELCOUNT,0},
	{2,  L"PPanel.SelCount",    MCODE_V_PPANEL_SELCOUNT,0},
	{2,  L"APanel.Path",        MCODE_V_APANEL_PATH,0},
	{2,  L"PPanel.Path",        MCODE_V_PPANEL_PATH,0},
	{2,  L"APanel.Path0",       MCODE_V_APANEL_PATH0,0},
	{2,  L"PPanel.Path0",       MCODE_V_PPANEL_PATH0,0},
	{2,  L"APanel.UNCPath",     MCODE_V_APANEL_UNCPATH,0},
	{2,  L"PPanel.UNCPath",     MCODE_V_PPANEL_UNCPATH,0},
	{2,  L"APanel.Height",      MCODE_V_APANEL_HEIGHT,0},
	{2,  L"PPanel.Height",      MCODE_V_PPANEL_HEIGHT,0},
	{2,  L"APanel.Width",       MCODE_V_APANEL_WIDTH,0},
	{2,  L"PPanel.Width",       MCODE_V_PPANEL_WIDTH,0},
	{2,  L"APanel.OPIFlags",    MCODE_V_APANEL_OPIFLAGS,0},
	{2,  L"PPanel.OPIFlags",    MCODE_V_PPANEL_OPIFLAGS,0},
	{2,  L"APanel.DriveType",   MCODE_V_APANEL_DRIVETYPE,0}, // APanel.DriveType - активная панель: тип привода
	{2,  L"PPanel.DriveType",   MCODE_V_PPANEL_DRIVETYPE,0}, // PPanel.DriveType - пассивная панель: тип привода
	{2,  L"APanel.ColumnCount", MCODE_V_APANEL_COLUMNCOUNT,0}, // APanel.ColumnCount - активная панель:  количество колонок
	{2,  L"PPanel.ColumnCount", MCODE_V_PPANEL_COLUMNCOUNT,0}, // PPanel.ColumnCount - пассивная панель: количество колонок
	{2,  L"APanel.HostFile",    MCODE_V_APANEL_HOSTFILE,0},
	{2,  L"PPanel.HostFile",    MCODE_V_PPANEL_HOSTFILE,0},
	{2,  L"APanel.Prefix",      MCODE_V_APANEL_PREFIX,0},
	{2,  L"PPanel.Prefix",      MCODE_V_PPANEL_PREFIX,0},

	{2,  L"CmdLine.Bof",        MCODE_C_CMDLINE_BOF,0}, // курсор в начале cmd-строки редактирования?
	{2,  L"CmdLine.Eof",        MCODE_C_CMDLINE_EOF,0}, // курсор в конеце cmd-строки редактирования?
	{2,  L"CmdLine.Empty",      MCODE_C_CMDLINE_EMPTY,0},
	{2,  L"CmdLine.Selected",   MCODE_C_CMDLINE_SELECTED,0},
	{2,  L"CmdLine.ItemCount",  MCODE_V_CMDLINE_ITEMCOUNT,0},
	{2,  L"CmdLine.CurPos",     MCODE_V_CMDLINE_CURPOS,0},
	{2,  L"CmdLine.Value",      MCODE_V_CMDLINE_VALUE,0},

	{2,  L"Editor.FileName",    MCODE_V_EDITORFILENAME,0},
	{2,  L"Editor.CurLine",     MCODE_V_EDITORCURLINE,0},  // текущая линия в редакторе (в дополнении к Count)
	{2,  L"Editor.Lines",       MCODE_V_EDITORLINES,0},
	{2,  L"Editor.CurPos",      MCODE_V_EDITORCURPOS,0},
	{2,  L"Editor.RealPos",     MCODE_V_EDITORREALPOS,0},
	{2,  L"Editor.State",       MCODE_V_EDITORSTATE,0},
	{2,  L"Editor.Value",       MCODE_V_EDITORVALUE,0},
	{2,  L"Editor.SelValue",    MCODE_V_EDITORSELVALUE,0},

	{2,  L"Dlg.ItemType",       MCODE_V_DLGITEMTYPE,0},
	{2,  L"Dlg.ItemCount",      MCODE_V_DLGITEMCOUNT,0},
	{2,  L"Dlg.CurPos",         MCODE_V_DLGCURPOS,0},
	{2,  L"Dlg.Info.Id",        MCODE_V_DLGINFOID,0},

	{2,  L"Help.FileName",      MCODE_V_HELPFILENAME, 0},
	{2,  L"Help.Topic",         MCODE_V_HELPTOPIC, 0},
	{2,  L"Help.SelTopic",      MCODE_V_HELPSELTOPIC, 0},

	{2,  L"Drv.ShowPos",        MCODE_V_DRVSHOWPOS,0},
	{2,  L"Drv.ShowMode",       MCODE_V_DRVSHOWMODE,0},

	{2,  L"Viewer.FileName",    MCODE_V_VIEWERFILENAME,0},
	{2,  L"Viewer.State",       MCODE_V_VIEWERSTATE,0},

	{2,  L"Menu.Value",         MCODE_V_MENU_VALUE,0},

	{2,  L"Fullscreen",         MCODE_C_FULLSCREENMODE,0},
	{2,  L"IsUserAdmin",        MCODE_C_ISUSERADMIN,0},
};

TMacroKeywords MKeywordsArea[] =
{
	{0,  L"Funcs",              (DWORD)MACRO_FUNCS,0},
	{0,  L"Consts",             (DWORD)MACRO_CONSTS,0},
	{0,  L"Vars",               (DWORD)MACRO_VARS,0},
	{0,  L"Other",              (DWORD)MACRO_OTHER,0},
	{0,  L"Shell",              (DWORD)MACRO_SHELL,0},
	{0,  L"Viewer",             (DWORD)MACRO_VIEWER,0},
	{0,  L"Editor",             (DWORD)MACRO_EDITOR,0},
	{0,  L"Dialog",             (DWORD)MACRO_DIALOG,0},
	{0,  L"Search",             (DWORD)MACRO_SEARCH,0},
	{0,  L"Disks",              (DWORD)MACRO_DISKS,0},
	{0,  L"MainMenu",           (DWORD)MACRO_MAINMENU,0},
	{0,  L"Menu",               (DWORD)MACRO_MENU,0},
	{0,  L"Help",               (DWORD)MACRO_HELP,0},
	{0,  L"Info",               (DWORD)MACRO_INFOPANEL,0},
	{0,  L"QView",              (DWORD)MACRO_QVIEWPANEL,0},
	{0,  L"Tree",               (DWORD)MACRO_TREEPANEL,0},
	{0,  L"FindFolder",         (DWORD)MACRO_FINDFOLDER,0},
	{0,  L"UserMenu",           (DWORD)MACRO_USERMENU,0},
	{0,  L"AutoCompletion",     (DWORD)MACRO_AUTOCOMPLETION,0},
	{0,  L"Common",             (DWORD)MACRO_COMMON,0},
};

TMacroKeywords MKeywordsFlags[] =
{
	// ФЛАГИ
	{1,  L"DisableOutput",      MFLAGS_DISABLEOUTPUT,0},
	{1,  L"RunAfterFARStart",   MFLAGS_RUNAFTERFARSTART,0},
	{1,  L"EmptyCommandLine",   MFLAGS_EMPTYCOMMANDLINE,0},
	{1,  L"NotEmptyCommandLine",MFLAGS_NOTEMPTYCOMMANDLINE,0},
	{1,  L"EVSelection",        MFLAGS_EDITSELECTION,0},
	{1,  L"NoEVSelection",      MFLAGS_EDITNOSELECTION,0},

	{1,  L"NoFilePanels",       MFLAGS_NOFILEPANELS,0},
	{1,  L"NoPluginPanels",     MFLAGS_NOPLUGINPANELS,0},
	{1,  L"NoFolders",          MFLAGS_NOFOLDERS,0},
	{1,  L"NoFiles",            MFLAGS_NOFILES,0},
	{1,  L"Selection",          MFLAGS_SELECTION,0},
	{1,  L"NoSelection",        MFLAGS_NOSELECTION,0},

	{1,  L"NoFilePPanels",      MFLAGS_PNOFILEPANELS,0},
	{1,  L"NoPluginPPanels",    MFLAGS_PNOPLUGINPANELS,0},
	{1,  L"NoPFolders",         MFLAGS_PNOFOLDERS,0},
	{1,  L"NoPFiles",           MFLAGS_PNOFILES,0},
	{1,  L"PSelection",         MFLAGS_PSELECTION,0},
	{1,  L"NoPSelection",       MFLAGS_PNOSELECTION,0},

	{1,  L"NoSendKeysToPlugins",MFLAGS_NOSENDKEYSTOPLUGINS,0},
};

static bool absFunc(const TMacroFunction*);
static bool ascFunc(const TMacroFunction*);
static bool atoiFunc(const TMacroFunction*);
static bool beepFunc(const TMacroFunction*);
static bool callpluginFunc(const TMacroFunction*);
static bool chrFunc(const TMacroFunction*);
static bool clipFunc(const TMacroFunction*);
static bool dateFunc(const TMacroFunction*);
static bool dlggetvalueFunc(const TMacroFunction*);
static bool editorposFunc(const TMacroFunction*);
static bool editorselFunc(const TMacroFunction*);
static bool editorsetFunc(const TMacroFunction*);
static bool editorsettitleFunc(const TMacroFunction*);
static bool editorundoFunc(const TMacroFunction*);
static bool environFunc(const TMacroFunction*);
static bool fattrFunc(const TMacroFunction*);
static bool fexistFunc(const TMacroFunction*);
static bool floatFunc(const TMacroFunction*);
static bool flockFunc(const TMacroFunction*);
static bool fsplitFunc(const TMacroFunction*);
static bool iifFunc(const TMacroFunction*);
static bool indexFunc(const TMacroFunction*);
static bool intFunc(const TMacroFunction*);
static bool itowFunc(const TMacroFunction*);
static bool lcaseFunc(const TMacroFunction*);
static bool kbdLayoutFunc(const TMacroFunction*);
static bool keyFunc(const TMacroFunction*);
static bool lenFunc(const TMacroFunction*);
static bool maxFunc(const TMacroFunction*);
static bool mloadFunc(const TMacroFunction*);
static bool modFunc(const TMacroFunction*);
static bool msaveFunc(const TMacroFunction*);
static bool msgBoxFunc(const TMacroFunction*);
static bool minFunc(const TMacroFunction*);
static bool panelfattrFunc(const TMacroFunction*);
static bool panelfexistFunc(const TMacroFunction*);
static bool panelitemFunc(const TMacroFunction*);
static bool panelselectFunc(const TMacroFunction*);
static bool panelsetpathFunc(const TMacroFunction*);
static bool panelsetposFunc(const TMacroFunction*);
static bool panelsetposidxFunc(const TMacroFunction*);
static bool panelitemFunc(const TMacroFunction*);
static bool promptFunc(const TMacroFunction*);
static bool replaceFunc(const TMacroFunction*);
static bool rindexFunc(const TMacroFunction*);
static bool sleepFunc(const TMacroFunction*);
static bool stringFunc(const TMacroFunction*);
static bool substrFunc(const TMacroFunction*);
static bool testfolderFunc(const TMacroFunction*);
static bool trimFunc(const TMacroFunction*);
static bool ucaseFunc(const TMacroFunction*);
static bool waitkeyFunc(const TMacroFunction*);
static bool xlatFunc(const TMacroFunction*);
static bool pluginsFunc(const TMacroFunction*);
static bool usersFunc(const TMacroFunction*);
static bool windowscrollFunc(const TMacroFunction*);

int MKeywordsSize = ARRAYSIZE(MKeywords);
int MKeywordsFlagsSize = ARRAYSIZE(MKeywordsFlags);

TVarTable glbVarTable;
TVarTable glbConstTable;

static TVar __varTextDate;
const TVar tviZero {static_cast<int64_t>(0)};

class TVMStack: public TStack<TVar>
{
	private:
		const TVar Error;

	public:
		TVMStack() {}
		~TVMStack() {}

	public:
		const TVar &Pop()
		{
			static TVar temp; //чтоб можно было вернуть по референс.

			if (TStack<TVar>::Pop(temp))
				return temp;

			return Error;
		}

		TVar &Pop(TVar &dest)
		{
			if (!TStack<TVar>::Pop(dest))
				dest=Error;

			return dest;
		}

		const TVar &Peek()
		{
			TVar *var = TStack<TVar>::Peek();

			if (var)
				return *var;

			return Error;
		}
};

TVMStack VMStack;

bool KeyMacro::ProcessKey(DWORD IntKey)
{
	if (m_InternalInput || IntKey==KEY_IDLE || IntKey==KEY_NONE || !FrameManager->GetCurrentFrame()) //FIXME: избавиться от IntKey
		return false;

	FARString textKey;
	if (!KeyToText(IntKey, textKey) || textKey.IsEmpty())
		return false;

	const bool ctrldot = IntKey == Opt.Macro.KeyMacroCtrlDot;           //###  || IntKey == Opt.Macro.KeyMacroRCtrlDot;
	const bool ctrlshiftdot = IntKey == Opt.Macro.KeyMacroCtrlShiftDot; //###  || IntKey == Opt.Macro.KeyMacroRCtrlShiftDot;

	if (m_Recording == MACROSTATE_NOMACRO)
	{
		if ((ctrldot||ctrlshiftdot) && !IsExecuting())
		{
			const Plugin* LuaMacro = CtrlObject->Plugins.FindPlugin(SYSID_LUAMACRO);
			if (!LuaMacro) //###  || LuaMacro->IsPendingRemove())
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
			ScrBuf.Flush();
			return true;
		}
		else
		{
			if (!m_WaitKey && IsPostMacroEnabled())
			{
				auto key = IntKey;
				if ((key&0x00FFFFFF) > 0x7F && (key&0x00FFFFFF) < 0xFFFF)
					key=KeyToKeyLayout(key&0x0000FFFF)|(key&~0x0000FFFF);

				if (key<0xFFFF)
					key=Upper(static_cast<wchar_t>(key)); //### was: upper

				FARString str = textKey;
				if (key != IntKey)
					KeyToText(key, str);
				if (TryToPostMacro(m_Area, str, IntKey))
					return true;
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
			int AssignRet=0; //### AssignMacroKey(MacroKey,Flags);

			if (AssignRet && AssignRet!=2 && !m_RecCode.IsEmpty())
			{
				m_RecCode = L"Keys(\"" + m_RecCode + L"\")";
				// добавим проверку на удаление
				// если удаляем или был вызван диалог изменения, то не нужно выдавать диалог настройки.
				//if (MacroKey != (DWORD)-1 && (Key==KEY_CTRLSHIFTDOT || Recording==2) && RecBufferSize)
				if (ctrlshiftdot && !GetMacroSettings(MacroKey,Flags))
				{
					AssignRet=0;
				}
			}
			m_InternalInput=0;
			if (AssignRet)
			{
				FARString strKey;
				KeyToText(MacroKey, strKey);
				Flags |= m_Recording == MACROSTATE_RECORDING_COMMON? MFLAGS_NONE : MFLAGS_NOSENDKEYSTOPLUGINS;
				LM_ProcessRecordedMacro(m_StartMode, strKey, m_RecCode, Flags, m_RecDescription);
			}

			m_Recording=MACROSTATE_NOMACRO;
			m_RecCode.Clear();
			m_RecDescription.Clear();
			ScrBuf.RestoreMacroChar();

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

// S=trim(S[,N])
static bool trimFunc(const TMacroFunction*)
{
	int  mode = VMStack.Pop().getInt32();
	TVar Val;
	VMStack.Pop(Val);
	wchar_t *p = (wchar_t *)Val.toString();
	bool Ret=true;

	switch (mode)
	{
		case 0: p=RemoveExternalSpaces(p); break;  // alltrim
		case 1: p=RemoveLeadingSpaces(p); break;   // ltrim
		case 2: p=RemoveTrailingSpaces(p); break;  // rtrim
		default: Ret=false;
	}

	VMStack.Push(p);
	return Ret;
}

// S=substr(S,start[,length])
static bool substrFunc(const TMacroFunction*)
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
	bool Ret=false;

	TVar VarLength;  VMStack.Pop(VarLength);
	int length = VarLength.getInt32();
	int  start = VMStack.Pop().getInt32();
	TVar Val;        VMStack.Pop(Val);

	wchar_t *p = (wchar_t *)Val.toString();
	int length_str = StrLength(p);

	// TODO: MCODE_OP_PUSHUNKNOWN!
	if ((uint64_t)VarLength.getInteger() == (((uint64_t)1)<<63))
		length=length_str;


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

	if (!length)
	{
		VMStack.Push(L"");
	}
	else
	{
		p += start;
		p[length] = 0;
		Ret=true;
		VMStack.Push(p);
	}

	return Ret;
}

static BOOL SplitFileName(const wchar_t *lpFullName,FARString &strDest,int nFlags)
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
		return FALSE;

	if ((*p == L'/') && (*(p+1) == L'/'))   //share
	{
		p += 2;
		p = wcschr(p, L'/');

		if (!p)
			return FALSE; //invalid share (\\server\)

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

	return TRUE;
}


// S=fsplit(S,N)
static bool fsplitFunc(const TMacroFunction*)
{
	int m = VMStack.Pop().getInt32();
	TVar Val;
	VMStack.Pop(Val);
	const wchar_t *s = Val.toString();
	bool Ret=false;
	FARString strPath;

	if (!SplitFileName(s,strPath,m))
		strPath.Clear();
	else
		Ret=true;

	VMStack.Push(strPath.CPtr());
	return Ret;
}

#if 0
// S=Meta("!.!") - в макросах юзаем ФАРовы метасимволы
static bool metaFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	const wchar_t *s = Val.toString();

	if (s && *s)
	{
		char SubstText[512];
		char Name[NM],ShortName[NM];
		far_strncpy(SubstText,s,sizeof(SubstText));
		SubstFileName(SubstText,sizeof(SubstText),Name,ShortName,nullptr,nullptr,TRUE);
		return TVar(SubstText);
	}

	return TVar(L"");
}
#endif


// N=atoi(S[,radix])
static bool atoiFunc(const TMacroFunction*)
{
	bool Ret=true;
	wchar_t *endptr;
	TVar R, S;
	VMStack.Pop(R);
	VMStack.Pop(S);
	VMStack.Push(TVar((int64_t)_wcstoi64(S.toString(),&endptr,(int)R.toInteger())));
	return Ret;
}


// N=Window.Scroll(Lines[,Axis])
static bool windowscrollFunc(const TMacroFunction*)
{
	bool Ret=false;
	TVar A, L;
	VMStack.Pop(A); // 0 - вертикаль (по умолчанию), 1 - горизонталь.
	VMStack.Pop(L); // Положительное число - вперёд (вниз/вправо), отрицательное - назад (вверх/влево).

	if (Opt.WindowMode)
	{
		int Lines=(int)L.i(), Columns=0;
		L=0;
		if (A.i())
		{
			Columns=Lines;
			Lines=0;
		}

		if (Console.ScrollWindow(Lines, Columns))
		{
			Ret=true;
			L=1;
		}
	}
	else
		L=0;

	VMStack.Push(L);
	return Ret;
}

// S=itoa(N[,radix])
static bool itowFunc(const TMacroFunction*)
{
	bool Ret=false;
	TVar R, N;
	VMStack.Pop(R);
	VMStack.Pop(N);

	if (N.isInteger())
	{
		wchar_t value[65];
		int Radix=(int)R.toInteger();

		if (!Radix)
			Radix=10;

		Ret=true;
		N=TVar(_i64tow(N.toInteger(),value,Radix));
	}

	VMStack.Push(N);
	return Ret;
}

// N=sleep(N)
static bool sleepFunc(const TMacroFunction*)
{
	long Period=(long)VMStack.Pop().getInteger();

	if (Period > 0)
	{
		WINPORT(Sleep)((DWORD)Period);
		VMStack.Push(1);
		return true;
	}

	VMStack.Push(tviZero);
	return false;
}

// S=key(V)
static bool keyFunc(const TMacroFunction*)
{
	TVar VarKey;
	VMStack.Pop(VarKey);
	FARString strKeyText;

	if (VarKey.isInteger())
	{
		if (VarKey.i())
			KeyToText((DWORD)VarKey.i(),strKeyText);
	}
	else
	{
		// Проверим...
		DWORD Key = KeyNameToKey(VarKey.s());

		if (Key != KEY_INVALID && Key==(DWORD)VarKey.i())
			strKeyText=VarKey.s();
	}

	VMStack.Push(strKeyText.CPtr());
	return !strKeyText.IsEmpty()?true:false;
}

// V=waitkey([N,[T]])
static bool waitkeyFunc(const TMacroFunction*)
{
	int64_t Type=VMStack.Pop().getInteger();
	int64_t Period=VMStack.Pop().getInteger();
	DWORD Key=WaitKey((DWORD)-1,Period);

	if (!Type)
	{
		FARString strKeyText;

		if (Key != KEY_NONE)
			if (!KeyToText(Key,strKeyText))
				strKeyText.Clear();

		VMStack.Push(strKeyText.CPtr());
		return !strKeyText.IsEmpty()?true:false;
	}

	if (Key == KEY_NONE)
		Key=KEY_INVALID;

	VMStack.Push((int64_t)Key);
	return Key != KEY_INVALID;
}

// n=min(n1,n2)
static bool minFunc(const TMacroFunction*)
{
	TVar V2, V1;
	VMStack.Pop(V2);
	VMStack.Pop(V1);
	VMStack.Push(V2 < V1 ? V2 : V1);
	return true;
}

// n=max(n1.n2)
static bool maxFunc(const TMacroFunction*)
{
	TVar V2, V1;
	VMStack.Pop(V2);
	VMStack.Pop(V1);
	VMStack.Push(V2 > V1  ? V2 : V1);
	return true;
}

// n=mod(n1,n2)
static bool modFunc(const TMacroFunction*)
{
	TVar V2, V1;
	VMStack.Pop(V2);
	VMStack.Pop(V1);

	if (!V2.i())
	{
		_KEYMACRO(___FILEFUNCLINE___;SysLog(L"Error: Divide (mod) by zero"));
		VMStack.Push(tviZero);
		return false;
	}

	VMStack.Push(V1 % V2);
	return true;
}

// n=iif(expression,n1,n2)
static bool iifFunc(const TMacroFunction*)
{
	//### to be implemented in Lua
	return true;
}

// N=index(S1,S2[,Mode])
static bool indexFunc(const TMacroFunction*)
{
	TVar Mode;  VMStack.Pop(Mode);
	TVar S2;    VMStack.Pop(S2);
	TVar S1;    VMStack.Pop(S1);

	const wchar_t *s = S1.toString();
	const wchar_t *p = S2.toString();
	const wchar_t *i = !Mode.getInteger() ? StrStrI(s,p) : StrStr(s,p);
	bool Ret= i ? true : false;
	VMStack.Push(TVar((int64_t)(i ? i-s : -1)));
	return Ret;
}

// S=rindex(S1,S2[,Mode])
static bool rindexFunc(const TMacroFunction*)
{
	TVar Mode;  VMStack.Pop(Mode);
	TVar S2;    VMStack.Pop(S2);
	TVar S1;    VMStack.Pop(S1);

	const wchar_t *s = S1.toString();
	const wchar_t *p = S2.toString();
	const wchar_t *i = !Mode.getInteger() ? RevStrStrI(s,p) : RevStrStr(s,p);
	bool Ret= i ? true : false;
	VMStack.Push(TVar((int64_t)(i ? i-s : -1)));
	return Ret;
}

// S=date([S])
static bool dateFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);

	if (Val.isInteger() && !Val.i())
		Val=L"";

	const wchar_t *s = Val.toString();
	bool Ret=false;
	FARString strTStr;

	if (MkStrFTime(strTStr,s))
		Ret=true;
	else
		strTStr.Clear();

	VMStack.Push(TVar(strTStr.CPtr()));
	return Ret;
}

// S=xlat(S)
static bool xlatFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	wchar_t *Str = (wchar_t *)Val.toString();
	bool Ret=::Xlat(Str,0,StrLength(Str),Opt.XLat.Flags)?true:false;
	VMStack.Push(TVar(Str));
	return Ret;
}

// N=beep([N])
static bool beepFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	/*
		MB_ICONASTERISK = 0x00000040
			Звук Звездочка
		MB_ICONEXCLAMATION = 0x00000030
		    Звук Восклицание
		MB_ICONHAND = 0x00000010
		    Звук Критическая ошибка
		MB_ICONQUESTION = 0x00000020
		    Звук Вопрос
		MB_OK = 0x0
		    Стандартный звук
		SIMPLE_BEEP = 0xffffffff
		    Встроенный динамик
	*/
	bool Ret=false;//MessageBeep((UINT)Val.i())?true:false;

	/*
		http://msdn.microsoft.com/en-us/library/dd743680%28VS.85%29.aspx
		BOOL PlaySound(
	    	LPCTSTR pszSound,
	    	HMODULE hmod,
	    	DWORD fdwSound
		);

		http://msdn.microsoft.com/en-us/library/dd798676%28VS.85%29.aspx
		BOOL sndPlaySound(
	    	LPCTSTR lpszSound,
	    	UINT fuSound
		);
	*/

	VMStack.Push(Ret?1:0);
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
static bool kbdLayoutFunc(const TMacroFunction*)
{
//	DWORD dwLayout = (DWORD)VMStack.Pop().getInteger();

	BOOL Ret=TRUE;
	HKL  RetLayout=(HKL)0; //Layout=(HKL)0,

	VMStack.Push(Ret?TVar(static_cast<INT64>(reinterpret_cast<INT_PTR>(RetLayout))):tviZero);

	return Ret?true:false;
}

// S=prompt("Title"[,"Prompt"[,flags[, "Src"[, "History"]]]])
static bool promptFunc(const TMacroFunction*)
{
	TVar ValHistory;
	VMStack.Pop(ValHistory);
	TVar ValSrc;
	VMStack.Pop(ValSrc);
	DWORD Flags = (DWORD)VMStack.Pop().getInteger();
	TVar ValPrompt;
	VMStack.Pop(ValPrompt);
	TVar ValTitle;
	VMStack.Pop(ValTitle);
	TVar Result(L"");
	bool Ret=false;

	if (!(ValTitle.isInteger() && !ValTitle.i()))
	{
		const wchar_t *history=nullptr;

		if (!(ValHistory.isInteger() && !ValHistory.i()))
			history=ValHistory.s();

		const wchar_t *src=L"";

		if (!(ValSrc.isInteger() && !ValSrc.i()))
			src=ValSrc.s();

		const wchar_t *prompt=L"";

		if (!(ValPrompt.isInteger() && !ValPrompt.i()))
			prompt=ValPrompt.s();

		const wchar_t *title=NullToEmpty(ValTitle.toString());
		FARString strDest;

		if (GetString(title,prompt,history,src,strDest,nullptr,Flags&~FIB_CHECKBOX,nullptr,nullptr))
		{
			Result=strDest.CPtr();
			Result.toString();
			Ret=true;
		}
	}

	VMStack.Push(Result);
	return Ret;
}

// N=msgbox(["Title"[,"Text"[,flags]]])
static bool msgBoxFunc(const TMacroFunction*)
{
	DWORD Flags = (DWORD)VMStack.Pop().getInteger();
	TVar ValB, ValT;
	VMStack.Pop(ValB);
	VMStack.Pop(ValT);
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
	TVar Result=FarMessageFn(-1,Flags,nullptr,(const wchar_t * const *)TempBuf.CPtr(),0,0)+1;
	VMStack.Push(Result);
	return true;
}


// S=env(S)
static bool environFunc(const TMacroFunction*)
{
	TVar S;
	VMStack.Pop(S);
	bool Ret=false;
	FARString strEnv;

	if (apiGetEnvironmentVariable(S.toString(), strEnv))
		Ret=true;
	else
		strEnv.Clear();

	VMStack.Push(strEnv.CPtr());
	return Ret;
}

// V=Panel.Select(panelType,Action[,Mode[,Items]])
static bool panelselectFunc(const TMacroFunction*)
{
	TVar ValItems;  VMStack.Pop(ValItems);
	int Mode = VMStack.Pop().getInt32();
	DWORD Action=(int)VMStack.Pop().getInteger();
	int typePanel = VMStack.Pop().getInt32();
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

		if (Mode == 2 || Mode == 3)
		{
			FARString strStr=ValItems.s();
			ReplaceStrings(strStr,L"\r\n",L";");
			ValItems=strStr.CPtr();
		}

		MacroPanelSelect mps;
		mps.Action      = Action & 0xF;
		mps.ActionFlags = (Action & (~0xF)) >> 4;
		mps.Mode        = Mode;
		mps.Index       = Index;
		mps.Item        = &ValItems;
		Result=SelPanel->VMProcess(MCODE_F_PANEL_SELECT,&mps,0);
	}

	VMStack.Push(Result);
	return Result==-1?false:true;
}

static bool _fattrFunc(int Type)
{
	bool Ret=false;
	DWORD FileAttr=INVALID_FILE_ATTRIBUTES;
	long Pos=-1;

	if (!Type || Type == 2) // не панели: fattr(0) & fexist(2)
	{
		TVar Str;
		VMStack.Pop(Str);
		FAR_FIND_DATA_EX FindData;
		apiGetFindDataEx(Str.toString(), FindData);
		FileAttr=FindData.dwFileAttributes;
		Ret=true;
	}
	else // panel.fattr(1) & panel.fexist(3)
	{
		TVar S;
		VMStack.Pop(S);
		int typePanel = VMStack.Pop().getInt32();
		const wchar_t *Str = S.toString();
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
				Ret=true;
			}
		}
	}

	if (Type == 2) // fexist(2)
		FileAttr=(FileAttr!=INVALID_FILE_ATTRIBUTES)?1:0;
	else if (Type == 3) // panel.fexist(3)
		FileAttr=(DWORD)Pos+1;

	VMStack.Push(TVar((int64_t)FileAttr));
	return Ret;
}

// N=fattr(S)
static bool fattrFunc(const TMacroFunction*)
{
	return _fattrFunc(0);
}

// N=fexist(S)
static bool fexistFunc(const TMacroFunction*)
{
	return _fattrFunc(2);
}

// N=panel.fattr(S)
static bool panelfattrFunc(const TMacroFunction*)
{
	return _fattrFunc(1);
}

// N=panel.fexist(S)
static bool panelfexistFunc(const TMacroFunction*)
{
	return _fattrFunc(3);
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
static bool flockFunc(const TMacroFunction*)
{
	TVar Ret(-1);
	int stateFLock = VMStack.Pop().getInt32();
	UINT vkKey=(UINT)VMStack.Pop().getInteger();

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

	VMStack.Push(Ret);
	return Ret.i()!=-1;
}

// V=Dlg.GetValue(ID,N)
static bool dlggetvalueFunc(const TMacroFunction*)
{
	TVar Ret(-1);
	int TypeInf = VMStack.Pop().getInt32();
	unsigned Index=(unsigned)VMStack.Pop().getInteger()-1;
	Frame* CurFrame=FrameManager->GetCurrentFrame();

	if (CtrlObject->Macro.GetMode()==MACRO_DIALOG && CurFrame && CurFrame->GetType()==MODALTYPE_DIALOG)
	{
		unsigned DlgItemCount=((Dialog*)CurFrame)->GetAllItemCount();
		const DialogItemEx **DlgItem=((Dialog*)CurFrame)->GetAllItem();

		if (Index == std::numeric_limits<unsigned>::max())
		{
			SMALL_RECT Rect;

			if (SendDlgMessage((HANDLE)CurFrame,DM_GETDLGRECT,0,(LONG_PTR)&Rect))
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

					if (SendDlgMessage((HANDLE)CurFrame,DM_LISTGETITEM,Index,(LONG_PTR)&ListItem))
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
						Ret = tviZero;
						/*
						int Item->Selected;
						const char *Item->History;
						const char *Item->Mask;
						FarList *Item->ListItems;
						int  Item->ListPos;
						CHAR_INFO *Item->VBuf;
						*/
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

	VMStack.Push(Ret);
	return Ret.i()!=-1;
}

// N=Editor.Pos(Op,What[,Where])
// Op: 0 - get, 1 - set
static bool editorposFunc(const TMacroFunction*)
{
	TVar Ret(-1);
	int Where = VMStack.Pop().getInt32();
	int What  = VMStack.Pop().getInt32();
	int Op    = VMStack.Pop().getInt32();

	if (CtrlObject->Macro.GetMode()==MACRO_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
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

	VMStack.Push(Ret);
	return Ret.i() != -1;
}

// OldVar=Editor.Set(Idx,Var)
static bool editorsetFunc(const TMacroFunction*)
{
	TVar Ret(-1);
	TVar _longState;
	VMStack.Pop(_longState);
	int Index = VMStack.Pop().getInt32();

	if (CtrlObject->Macro.GetMode()==MACRO_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
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

	VMStack.Push(Ret);
	return Ret.i()==-1;
}

static bool DeserializeVar(TVar &v, FARString &ValSerialized)
{
	if (ValSerialized.Begins(L"INT:"))
	{
		v = (int64_t)wcstoll(ValSerialized.CPtr() + 4, nullptr, 10);
		return true;
	}
	if (ValSerialized.Begins(L"STR:"))
	{
		v = ValSerialized.CPtr() + 4;
		return true;
	}
	if (ValSerialized.Begins(L"DBL:"))
	{
		v = wcstod(ValSerialized.CPtr() + 4, nullptr);
		return true;
	}
	return false;
}

static bool SerializeVar(TVar &v, FARString &ValSerialized)
{
	if (v.isInteger())
	{
		ValSerialized.Format(L"INT:%lld", (long long)v.i());
		return true;
	}
	if (v.isString())
	{
		ValSerialized.Format(L"STR:%ls", v.s());
		return true;
	}
	if (v.isDouble())
	{
		ValSerialized.Format(L"DBL:%f", v.d());
		return true;
	}
	return false;
}

// b=mload(var)
static bool mloadFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	TVarTable *t = &glbVarTable;
	const wchar_t *Name = Val.s();

	if (!Name || *Name != L'%')
	{
		VMStack.Push(tviZero);
		return false;
	}

	bool Ret = false;
	FARString ValSerialized = ConfigReader("KeyMacros/Vars").GetString(Wide2MB(Name), L"");
	if (ValSerialized != L"")
	{
		Ret = DeserializeVar(varInsert(*t, Name+1)->value, ValSerialized);
	}
	VMStack.Push(TVar(Ret ? 1 : 0));
	return Ret;
}

// b=msave(var)
static bool msaveFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	TVarTable *t = &glbVarTable;
	const wchar_t *Name=Val.s();

	if (!Name || *Name!= L'%')
	{
		VMStack.Push(tviZero);
		return false;
	}

	TVarSet *tmpVarSet=varLook(*t, Name+1);

	if (!tmpVarSet)
	{
		VMStack.Push(tviZero);
		return false;
	}

	TVar Result = tmpVarSet->value;
	bool Ret = false;
	FARString strValueName = Val.s();
	FARString strValueData;
	if (SerializeVar(Result, strValueData))
	{
		ConfigWriter cfg_writer("KeyMacros/Vars");
		cfg_writer.SetString(strValueName.GetMB(), strValueData);
		Ret = cfg_writer.Save();
	}

	VMStack.Push(TVar(Ret ? 1 : 0));

	return Ret;
}

// V=Clip(N[,V])
static bool clipFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	int cmdType = VMStack.Pop().getInt32();

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
				VMStack.Push(varClip);
				return true;
			}

			break;
		}
		case 1: // Put "S" into Clipboard
		{
			Ret=CopyToClipboard(Val.s());
			VMStack.Push(TVar((int64_t)Ret)); // 0!  ???
			return Ret?true:false;
		}
		case 2: // Add "S" into Clipboard
		{
			TVar varClip(Val.s());
			Clipboard clip;

			Ret=FALSE;

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
			VMStack.Push(TVar((int64_t)Ret)); // 0!  ???
			return Ret?true:false;
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
			VMStack.Push(TVar((int64_t)Ret)); // 0!  ???
			return Ret?true:false;
		}
		case 5: // ClipMode
		{
			// 0 - flip, 1 - виндовый буфер, 2 - внутренний, -1 - что сейчас?
			int Action = Val.getInt32();
			bool mode=Clipboard::GetUseInternalClipboardState();
			if (Action >= 0)
			{
				switch (Action)
				{
					case 0: mode=!mode; break;
					case 1: mode=false; break;
					case 2: mode=true;  break;
				}
				mode=Clipboard::SetUseInternalClipboardState(mode);
			}
			VMStack.Push((int64_t)(mode?2:1)); // 0!  ???
			return Ret?true:false;
		}
	}

	return Ret?true:false;
}


// N=Panel.SetPosIdx(panelType,Idx[,InSelection])
/*
*/
static bool panelsetposidxFunc(const TMacroFunction*)
{
	int InSelection = VMStack.Pop().getInt32();
	long idxItem=(long)VMStack.Pop().getInteger();
	int typePanel = VMStack.Pop().getInt32();
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

	VMStack.Push(Ret);
	return Ret?true:false;
}

// N=panel.SetPath(panelType,pathName[,fileName])
static bool panelsetpathFunc(const TMacroFunction*)
{
	TVar ValFileName;  VMStack.Pop(ValFileName);
	TVar Val;          VMStack.Pop(Val);
	int typePanel = VMStack.Pop().getInt32();
	int64_t Ret=0;

	if (!(Val.isInteger() && !Val.i()))
	{
		const wchar_t *pathName=Val.s();
		const wchar_t *fileName=L"";

		if (!ValFileName.isInteger())
			fileName=ValFileName.s();

		Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
		Panel *PassivePanel=nullptr;

		if (ActivePanel)
			PassivePanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

		//Frame* CurFrame=FrameManager->GetCurrentFrame();
		Panel *SelPanel = typePanel? (typePanel == 1?PassivePanel:nullptr):ActivePanel;

		if (SelPanel)
		{
			if (SelPanel->SetCurDir(pathName,TRUE))
			{
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

	VMStack.Push(Ret);
	return Ret?true:false;
}

// N=Panel.SetPos(panelType,fileName)
static bool panelsetposFunc(const TMacroFunction*)
{
	TVar Val; VMStack.Pop(Val);
	int typePanel = VMStack.Pop().getInt32();
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

	VMStack.Push(Ret);
	return Ret?true:false;
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
static bool replaceFunc(const TMacroFunction*)
{
	int Mode = VMStack.Pop().getInt32();
	TVar Count; VMStack.Pop(Count);
	TVar Repl;  VMStack.Pop(Repl);
	TVar Find;  VMStack.Pop(Find);
	TVar Src;   VMStack.Pop(Src);
	int64_t Ret=1;
	// TODO: Здесь нужно проверить в соответствии с УНИХОДОМ!
	FARString strStr;
	int lenS=StrLength(Src.s());
	int lenF=StrLength(Find.s());
	int lenR=StrLength(Repl.s());
	int cnt=0;

	if( lenF )
	{
		const wchar_t *Ptr=Src.s();
		if( !Mode )
		{
			while ((Ptr=StrStrI(Ptr,Find.s())) )
			{
				cnt++;
				Ptr+=lenF;
			}
		}
		else
		{
			while ((Ptr=StrStr(Ptr,Find.s())) )
			{
				cnt++;
				Ptr+=lenF;
			}
		}
	}

	if (cnt)
	{
		if (lenR > lenF)
			lenS+=cnt*(lenR-lenF+1); //???

		strStr=Src.s();
		cnt=(int)Count.i();

		if (cnt <= 0)
			cnt=-1;

		ReplaceStrings(strStr,Find.s(),Repl.s(),cnt,!Mode);
		VMStack.Push(strStr.CPtr());
	}
	else
		VMStack.Push(Src);

	return Ret?true:false;
}

// V=Panel.Item(typePanel,Index,TypeInfo)
static bool panelitemFunc(const TMacroFunction*)
{
	TVar P2; VMStack.Pop(P2);
	TVar P1; VMStack.Pop(P1);
	int typePanel = VMStack.Pop().getInt32();
	TVar Ret{tviZero};
	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
	Panel *PassivePanel=nullptr;

	if (ActivePanel)
		PassivePanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);

	//Frame* CurFrame=FrameManager->GetCurrentFrame();
	Panel *SelPanel = typePanel? (typePanel == 1?PassivePanel:nullptr):ActivePanel;

	if (!SelPanel)
	{
		VMStack.Push(Ret);
		return false;
	}

	int TypePanel=SelPanel->GetType(); //FILE_PANEL,TREE_PANEL,QVIEW_PANEL,INFO_PANEL

	if (!(TypePanel == FILE_PANEL || TypePanel ==TREE_PANEL))
	{
		VMStack.Push(Ret);
		return false;
	}

	int Index=(int)(P1.toInteger())-1;
	int TypeInfo=(int)P2.toInteger();
	FileListItem filelistItem;

	if (TypePanel == TREE_PANEL)
	{
		TreeItem treeItem;

		if (SelPanel->GetItem(Index,&treeItem) && !TypeInfo)
		{
			VMStack.Push(TVar(treeItem.strName));
			return true;
		}
	}
	else
	{
		FARString strDate, strTime;

		if (TypeInfo == 11)
			SelPanel->ReadDiz();

		if (!SelPanel->GetItem(Index,&filelistItem))
			TypeInfo=-1;

		switch (TypeInfo)
		{
			case 0:  // Name
				Ret=TVar(filelistItem.strName);
				break;
			case 1:  // ShortName obsolete, use Name
				Ret=TVar(filelistItem.strName);
				break;
			case 2:  // FileAttr
				Ret=TVar((int64_t)filelistItem.FileAttr);
				break;
			case 3:  // CreationTime
				ConvertDate(filelistItem.CreationTime,strDate,strTime,8,FALSE,FALSE,TRUE,TRUE);
				strDate += L" ";
				strDate += strTime;
				Ret=TVar(strDate.CPtr());
				break;
			case 4:  // AccessTime
				ConvertDate(filelistItem.AccessTime,strDate,strTime,8,FALSE,FALSE,TRUE,TRUE);
				strDate += L" ";
				strDate += strTime;
				Ret=TVar(strDate.CPtr());
				break;
			case 5:  // WriteTime
				ConvertDate(filelistItem.WriteTime,strDate,strTime,8,FALSE,FALSE,TRUE,TRUE);
				strDate += L" ";
				strDate += strTime;
				Ret=TVar(strDate.CPtr());
				break;
			case 6:  // FileSize
				Ret=TVar((int64_t)filelistItem.FileSize);
				break;
			case 7:  // PhysicalSize
				Ret=TVar((int64_t)filelistItem.PhysicalSize);
				break;
			case 8:  // Selected
				Ret=TVar((int64_t)((DWORD)filelistItem.Selected));
				break;
			case 9:  // NumberOfLinks
				Ret=TVar((int64_t)filelistItem.NumberOfLinks);
				break;
			case 10:  // SortGroup
				Ret=filelistItem.SortGroup;
				break;
			case 11:  // DizText
			{
				const wchar_t *LPtr=filelistItem.DizText;
				Ret=TVar(LPtr);
				break;
			}
			case 12:  // Owner
				Ret=TVar(filelistItem.strOwner);
				break;
			case 13:  // CRC32
				Ret=TVar((int64_t)filelistItem.CRC32);
				break;
			case 14:  // Position
				Ret=filelistItem.Position;
				break;
			case 15:  // CreationTime (FILETIME)
				Ret=TVar((int64_t)FileTimeToUI64(&filelistItem.CreationTime));
				break;
			case 16:  // AccessTime (FILETIME)
				Ret=TVar((int64_t)FileTimeToUI64(&filelistItem.AccessTime));
				break;
			case 17:  // WriteTime (FILETIME)
				Ret=TVar((int64_t)FileTimeToUI64(&filelistItem.WriteTime));
				break;
			case 18: // NumberOfStreams (deprecated)
				Ret=TVar((int64_t)((filelistItem.FileAttr & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1));
				break;
			case 19: // StreamsSize (deprecated)
				Ret=TVar((int64_t)0);
				break;
			case 20:  // ChangeTime
				ConvertDate(filelistItem.ChangeTime,strDate,strTime,8,FALSE,FALSE,TRUE,TRUE);
				strDate += L" ";
				strDate += strTime;
				Ret=TVar(strDate.CPtr());
				break;
			case 21:  // ChangeTime (FILETIME)
				Ret=TVar((int64_t)FileTimeToUI64(&filelistItem.ChangeTime));
				break;
		}
	}

	VMStack.Push(Ret);
	return false;
}

// N=len(V)
static bool lenFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	VMStack.Push(TVar(StrLength(Val.toString())));
	return true;
}

static bool ucaseFunc(const TMacroFunction*)
{
	TVar Val; VMStack.Pop(Val);
	StrUpper((wchar_t *)Val.toString());
	VMStack.Push(Val);
	return true;
}

static bool lcaseFunc(const TMacroFunction*)
{
	TVar Val; VMStack.Pop(Val);
	StrLower((wchar_t *)Val.toString());
	VMStack.Push(Val);
	return true;
}

static bool stringFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	Val.toString();
	VMStack.Push(Val);
	return true;
}

static bool intFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	Val.toInteger();
	VMStack.Push(Val);
	return true;
}

static bool floatFunc(const TMacroFunction*)
{
	TVar Val;
	VMStack.Pop(Val);
	//Val.toDouble();
	VMStack.Push(Val);
	return true;
}

static bool absFunc(const TMacroFunction*)
{
	TVar tmpVar;
	VMStack.Pop(tmpVar);

	if (tmpVar < tviZero)
		tmpVar=-tmpVar;

	VMStack.Push(tmpVar);
	return true;
}

static bool ascFunc(const TMacroFunction*)
{
	TVar tmpVar;
	VMStack.Pop(tmpVar);

	if (tmpVar.isString())
	{
		tmpVar = (int64_t)((DWORD)((WORD)*tmpVar.toString()));
		tmpVar.toInteger();
	}

	VMStack.Push(tmpVar);
	return true;
}

static bool chrFunc(const TMacroFunction*)
{
	TVar tmpVar;
	VMStack.Pop(tmpVar);

	if (tmpVar.isInteger())
	{
		const wchar_t tmp[]={(wchar_t) (tmpVar.i() & (wchar_t)-1),L'\0'};
		tmpVar = tmp;
		tmpVar.toString();
	}

	VMStack.Push(tmpVar);
	return true;
}


// V=Editor.Sel(Action[,Opt])
static bool editorselFunc(const TMacroFunction*)
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
	TVar Ret{tviZero};
	TVar Opt; VMStack.Pop(Opt);
	TVar Action; VMStack.Pop(Action);
	int Mode=CtrlObject->Macro.GetMode();
	Frame* CurFrame=FrameManager->GetCurrentFrame();
	int NeedType = Mode == MACRO_EDITOR?MODALTYPE_EDITOR:(Mode == MACRO_VIEWER?MODALTYPE_VIEWER:(Mode == MACRO_DIALOG?MODALTYPE_DIALOG:MODALTYPE_PANELS)); // MACRO_SHELL?

	if (CurFrame && CurFrame->GetType()==NeedType)
	{
		if (Mode==MACRO_SHELL && CtrlObject->CmdLine->IsVisible())
			Ret=CtrlObject->CmdLine->VMProcess(MCODE_F_EDITOR_SEL,(void*)Action.toInteger(),Opt.i());
		else
			Ret=CurFrame->VMProcess(MCODE_F_EDITOR_SEL,(void*)Action.toInteger(),Opt.i());
	}

	VMStack.Push(Ret);
	return Ret.i() == 1;
}

// V=Editor.Undo(N)
static bool editorundoFunc(const TMacroFunction*)
{
	TVar Ret{tviZero};
	TVar Action; VMStack.Pop(Action);

	if (CtrlObject->Macro.GetMode()==MACRO_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
	{
		EditorUndoRedo eur;
		eur.Command=(int)Action.toInteger();
		Ret=CtrlObject->Plugins.CurEditor->EditorControl(ECTL_UNDOREDO,&eur);
	}

	VMStack.Push(Ret);
	return Ret.i()!=0;
}

// N=Editor.SetTitle([Title])
static bool editorsettitleFunc(const TMacroFunction*)
{
	TVar Ret{tviZero};
	TVar Title; VMStack.Pop(Title);

	if (CtrlObject->Macro.GetMode()==MACRO_EDITOR && CtrlObject->Plugins.CurEditor && CtrlObject->Plugins.CurEditor->IsVisible())
	{
		if (Title.isInteger() && !Title.i())
		{
			Title=L"";
			Title.toString();
		}
		Ret=CtrlObject->Plugins.CurEditor->EditorControl(ECTL_SETTITLE,(void*)Title.s());
	}

	VMStack.Push(Ret);
	return Ret.i()!=0;
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
static bool testfolderFunc(const TMacroFunction*)
{
	TVar tmpVar;
	VMStack.Pop(tmpVar);
	int64_t Ret=TSTFLD_ERROR;

	if (tmpVar.isString())
	{
		Ret=TestFolder(tmpVar.s());
	}

	VMStack.Push(Ret);
	return Ret?true:false;
}

// вызов плагиновой функции
static bool pluginsFunc(const TMacroFunction *thisFunc)
{
	TVar V;
	bool Ret=false;
	int nParam=thisFunc->nParam;
/*
enum FARMACROVARTYPE
{
	FMVT_INTEGER                = 0,
	FMVT_STRING                 = 1,
	FMVT_DOUBLE                 = 2,
};

struct FarMacroValue
{
	FARMACROVARTYPE type;
	union
	{
		int64_t  i;
		double   d;
		const wchar_t *s;
	} v;
};
*/
#if defined(PROCPLUGINMACROFUNC)
	int I;

	FarMacroValue *vParams=new FarMacroValue[nParam];
	if (vParams)
	{
		memset(vParams,0,sizeof(FarMacroValue) * nParam);

		for (I=nParam-1; I >= 0; --I)
		{
			VMStack.Pop(V);
			(vParams+I)->type=(FARMACROVARTYPE)V.type();
			switch(V.type())
			{
				case vtInteger:
					(vParams+I)->v.i=V.i();
					break;
				case vtString:
					(vParams+I)->v.s=wcsdup(V.s());
					break;
				case vtDouble:
					(vParams+I)->v.d=V.d();
					break;
			}
		}

		FarMacroValue *Results;
		int nResults=0;
		// fnGUID ???
		if (CtrlObject->Plugins.ProcessMacroFunc(thisFunc->Name,vParams,thisFunc->nParam,&Results,&nResults))
		{
			if (Results)
			{
				for (I=0; I < nResults; ++I)
				//for (I=nResults-1; I >= 0; --I)
				{
					//V.type()=(TVarType)(Results+I)->type;
					switch((Results+I)->type)
					{
						case FMVT_INTEGER:
							V=(Results+I)->v.i;
							break;
						case FMVT_STRING:
							V=(Results+I)->v.s;
							break;
						case FMVT_DOUBLE:
							V=(Results+I)->v.d;
							break;
					}
					VMStack.Push(V);
				}
			}
		}

		for (I=0; I < nParam; ++I)
			if((vParams+I)->type == vtString && (vParams+I)->v.s)
				free((void*)(vParams+I)->v.s);

		delete[] vParams;
	}
	else
		VMStack.Push(0);
#else
	/* времянка */ while(--nParam >= 0) VMStack.Pop(V);
#endif
	return Ret;
}

// вызов пользовательской функции
static bool usersFunc(const TMacroFunction *thisFunc)
{
	TVar V;
	bool Ret=false;

	int nParam=thisFunc->nParam;
	/* времянка */ while(--nParam >= 0) VMStack.Pop(V);

	VMStack.Push(tviZero);
	return Ret;
}


const wchar_t *eStackAsString(int)
{
	const wchar_t *s=__varTextDate.toString();
	return !s?L"":s;
}

int KeyMacro::GetKey()
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
						return static_cast<int>(mpr.Values[1].Double);
				}
			}

			case MPRT_PRINT:
			{
				m_StringToPrint = mpr.Values[0].String;
				return KEY_OP_PLAINTEXT;
			}

			//~ case MPRT_PLUGINMENU:   // N=Plugin.Menu(Uuid[,MenuUuid])
			//~ case MPRT_PLUGINCONFIG: // N=Plugin.Config(Uuid[,MenuUuid])
			//~ case MPRT_PLUGINCOMMAND: // N=Plugin.Command(Uuid[,Command])
			//~ {
				//~ SetMacroValue(false);

				//~ if (!mpr.Count || mpr.Values[0].Type != FMVT_STRING)
					//~ break;

				//~ const auto Uuid = uuid::try_parse(string_view(mpr.Values[0].String));
				//~ if (!Uuid)
					//~ break;

				//~ if (!Global->CtrlObject->Plugins->FindPlugin(*Uuid))
					//~ break;

				//~ PluginManager::CallPluginInfo cpInfo = { CPT_CHECKONLY };
				//~ const auto Arg = mpr.Count > 1 && mpr.Values[1].Type == FMVT_STRING? mpr.Values[1].String : L"";

				//~ UUID MenuUuid;
				//~ if (*Arg && (mpr.ReturnType==MPRT_PLUGINMENU || mpr.ReturnType==MPRT_PLUGINCONFIG))
				//~ {
					//~ if (const auto MenuUuidOpt = uuid::try_parse(string_view(Arg)))
					//~ {
						//~ MenuUuid = *MenuUuidOpt;
						//~ cpInfo.ItemUuid = &MenuUuid;
					//~ }
					//~ else
						//~ break;
				//~ }

				//~ if (mpr.ReturnType == MPRT_PLUGINMENU)
					//~ cpInfo.CallFlags |= CPT_MENU;
				//~ else if (mpr.ReturnType == MPRT_PLUGINCONFIG)
					//~ cpInfo.CallFlags |= CPT_CONFIGURE;
				//~ else if (mpr.ReturnType == MPRT_PLUGINCOMMAND)
				//~ {
					//~ cpInfo.CallFlags |= CPT_CMDLINE;
					//~ cpInfo.Command = Arg;
				//~ }

				//~ // Чтобы вернуть результат "выполнения" нужно проверить наличие плагина/пункта
				//~ if (Global->CtrlObject->Plugins->CallPluginItem(*Uuid, &cpInfo))
				//~ {
					//~ // Если нашли успешно - то теперь выполнение
					//~ SetMacroValue(true);
					//~ cpInfo.CallFlags&=~CPT_CHECKONLY;
					//~ Global->CtrlObject->Plugins->CallPluginItem(*Uuid, &cpInfo);
				//~ }
				//~ Global->WindowManager->RefreshWindow();
				//~ //с текущим переключением окон могут быть проблемы с заголовком консоли.
				//~ Global->WindowManager->PluginCommit();

				//~ break;
			//~ }

			//~ case MPRT_USERMENU:
				//~ ShowUserMenu(mpr.Count,mpr.Values);
				//~ break;
		}
	}

	return 0;
}

// Проверить - есть ли еще клавиша?
int KeyMacro::PeekKey() const
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

/*
  после вызова этой функции нужно удалить память!!!
  функция декомпилит только простые последовательности, т.к.... клавиши
  в противном случае возвращает Src
*/
wchar_t *KeyMacro::MkTextSequence(DWORD *Buffer,int BufferSize,const wchar_t *Src)
{
	int J, Key;
	FARString strMacroKeyText;
	FARString strTextBuffer;

	if (!Buffer)
		return nullptr;

#if 0

	if (BufferSize == 1)
	{
		if (
		    (((DWORD)(DWORD_PTR)Buffer)&KEY_MACRO_ENDBASE) >= KEY_MACRO_BASE && (((DWORD)(DWORD_PTR)Buffer)&KEY_MACRO_ENDBASE) <= KEY_MACRO_ENDBASE ||
		    (((DWORD)(DWORD_PTR)Buffer)&KEY_OP_ENDBASE) >= KEY_OP_BASE && (((DWORD)(DWORD_PTR)Buffer)&KEY_OP_ENDBASE) <= KEY_OP_ENDBASE
		)
		{
			return Src?wcsdup(Src):nullptr;
		}

		if (KeyToText((DWORD)(DWORD_PTR)Buffer,strMacroKeyText))
			return wcsdup(strMacroKeyText.CPtr());

		return nullptr;
	}

#endif
	strTextBuffer.Clear();

	//~ if (Buffer[0] == MCODE_OP_KEYS)
		//~ for (J=1; J < BufferSize; J++)
		//~ {
			//~ Key=Buffer[J];

			//~ if (Key == MCODE_OP_ENDKEYS || Key == MCODE_OP_KEYS)
				//~ continue;

			//~ if (/*
				//~ (Key&KEY_MACRO_ENDBASE) >= KEY_MACRO_BASE && (Key&KEY_MACRO_ENDBASE) <= KEY_MACRO_ENDBASE ||
				//~ (Key&KEY_OP_ENDBASE) >= KEY_OP_BASE && (Key&KEY_OP_ENDBASE) <= KEY_OP_ENDBASE ||
				//~ */
			    //~ !KeyToText(Key,strMacroKeyText)
			//~ )
			//~ {
				//~ return Src?wcsdup(Src):nullptr;
			//~ }

			//~ if (J > 1)
				//~ strTextBuffer += L" ";

			//~ strTextBuffer += strMacroKeyText;
		//~ }

	if (!strTextBuffer.IsEmpty())
		return wcsdup(strTextBuffer.CPtr());

	return nullptr;
}

void KeyMacro::SetMacroConst(const wchar_t *ConstName, const TVar Value)
{
	varLook(glbConstTable, ConstName,1)->value = Value;
}

// эта функция будет вызываться из тех классов, которым нужен перезапуск макросов
void KeyMacro::RestartAutoMacro(int /*Mode*/)
{
#if 0
	/*
	Область      Рестарт
	-------------------------------------------------------
	Other         0
	Shell         1 раз, при запуске ФАРа
	Viewer        для каждой новой копии вьювера
	Editor        для каждой новой копии редатора
	Dialog        0
	Search        0
	Disks         0
	MainMenu      0
	Menu          0
	Help          0
	Info          1 раз, при запуске ФАРа и выставлении такой панели
	QView         1 раз, при запуске ФАРа и выставлении такой панели
	Tree          1 раз, при запуске ФАРа и выставлении такой панели
	Common        0
	*/
#endif
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

// обработчик диалогового окна назначения клавиши
LONG_PTR WINAPI KeyMacro::AssignMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	FARString strKeyText;
	static int LastKey=0;
	static DlgParam *KMParam=nullptr;
	int Index;

	//_SVS(SysLog(L"LastKey=%d Msg=%ls",LastKey,_DLGMSG_ToName(Msg)));
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

		/*
		int KeySize=GetRegKeySize("KeyMacros","DlgKeys");
		char *KeyStr;
		if(KeySize &&
			(KeyStr=(char*)malloc(KeySize+1))  &&
			GetRegKey("KeyMacros","DlgKeys",KeyStr,"",KeySize)
		)
		{
			UserDefinedList KeybList;
			if(KeybList.Set(KeyStr))
			{
				KeybList.Start();
				const char *OneKey;
				*KeyText=0;
				while(nullptr!=(OneKey=KeybList.GetNext()))
				{
					far_strncpy(KeyText, OneKey, sizeof(KeyText));
					SendDlgMessage(hDlg,DM_LISTADDSTR,2,(long)KeyText);
				}
			}
			free(KeyStr);
		}
		*/
		SendDlgMessage(hDlg,DM_SETTEXTPTR,2,reinterpret_cast<LONG_PTR>(L""));
		// </Клавиши, которые не введешь в диалоге назначения>
	}
	else if (Param1 == 2 && Msg == DN_EDITCHANGE)
	{
		LastKey = 0;
		_SVS(SysLog(L"[%d] ((FarDialogItem*)Param2)->PtrData='%ls'",__LINE__,((FarDialogItem*)Param2)->PtrData));
		auto KeyCode = KeyNameToKey(((FarDialogItem*)Param2)->PtrData);

		if (KeyCode != KEY_INVALID && KMParam != nullptr && !KMParam->Recurse)
		{
			Param2 = KeyCode;
			goto M1;
		}
	}
	else if (Msg == DN_KEY && (((Param2&KEY_END_SKEY) < KEY_END_FKEY) ||
	                           (((Param2&KEY_END_SKEY) > INTERNAL_KEY_BASE) && (Param2&KEY_END_SKEY) < INTERNAL_KEY_BASE_2)))
	{
		//if((Param2&0x00FFFFFF) >= 'A' && (Param2&0x00FFFFFF) <= 'Z' && ShiftPressed)
		//Param2|=KEY_SHIFT;

		//_SVS(SysLog(L"Macro: Key=%ls",_FARKEY_ToName(Param2)));
		// <Обработка особых клавиш: F1 & Enter>
		// Esc & (Enter и предыдущий Enter) - не обрабатываем
		if (Param2 == KEY_ESC ||
		        ((Param2 == KEY_ENTER||Param2 == KEY_NUMENTER) && (LastKey == KEY_ENTER||LastKey == KEY_NUMENTER)) ||
		        Param2 == KEY_CTRLDOWN ||
		        Param2 == KEY_F1)
		{
			return FALSE;
		}

		/*
		// F1 - особый случай - нужно жать 2 раза
		// первый раз будет выведен хелп,
		// а второй раз - второй раз уже назначение
		if(Param2 == KEY_F1 && LastKey!=KEY_F1)
		{
		  LastKey=KEY_F1;
		  return FALSE;
		}
		*/
		// Было что-то уже нажато и Enter`ом подтверждаем
		_SVS(SysLog(L"[%d] Assign ==> Param2='%ls',LastKey='%ls'",__LINE__,_FARKEY_ToName((DWORD)Param2),(LastKey?_FARKEY_ToName(LastKey):L"")));

		if ((Param2 == KEY_ENTER||Param2 == KEY_NUMENTER) && LastKey && !(LastKey == KEY_ENTER||LastKey == KEY_NUMENTER))
			return FALSE;

		// </Обработка особых клавиш: F1 & Enter>
M1:
		_SVS(SysLog(L"[%d] Assign ==> Param2='%ls',LastKey='%ls'",__LINE__,_FARKEY_ToName((DWORD)Param2),LastKey?_FARKEY_ToName(LastKey):L""));
		KeyMacro *MacroDlg=KMParam->Handle;

		if ((Param2&0x00FFFFFF) > 0x7F && (Param2&0x00FFFFFF) < 0xFFFF)
			Param2=KeyToKeyLayout((int)(Param2&0x0000FFFF))|(DWORD)(Param2&(~0x0000FFFF));

		//косметика
		if (Param2<0xFFFF)
			Param2=Upper((wchar_t)(Param2&0x0000FFFF))|(Param2&(~0x0000FFFF));

		_SVS(SysLog(L"[%d] Assign ==> Param2='%ls',LastKey='%ls'",__LINE__,_FARKEY_ToName((DWORD)Param2),LastKey?_FARKEY_ToName(LastKey):L""));
		KMParam->Key=(DWORD)Param2;
		KeyToText((uint32_t)Param2,strKeyText);

		// если УЖЕ есть такой макрос...
		//~ if ((Index=MacroDlg->GetIndex((uint32_t)Param2,KMParam->Mode)) != -1)
		//~ {
			//~ MacroRecord *Mac=MacroDlg->MacroLIB+Index;

			//~ // общие макросы учитываем только при удалении.
			//~ if (!MacroDlg->RecBuffer || !MacroDlg->RecBufferSize || (Mac->Flags&0xFF)!=MACRO_COMMON)
			//~ {
				//~ FARString strRegKeyName;
				//~ //### MacroDlg->MkRegKeyName(Index, strRegKeyName);

				//~ FARString strBufKey;
				//~ if (Mac->Src )
				//~ {
					//~ strBufKey=Mac->Src;
					//~ InsertQuote(strBufKey);
				//~ }

				//~ DWORD DisFlags=Mac->Flags&MFLAGS_DISABLEMACRO;
				//~ FARString strBuf;
				//~ if ((Mac->Flags&0xFF)==MACRO_COMMON)
					//~ strBuf.Format((!MacroDlg->RecBufferSize
					                  //~ ? (DisFlags ? Msg::MacroCommonDeleteAssign : Msg::MacroCommonDeleteKey)
					                  //~ : Msg::MacroCommonReDefinedKey), strKeyText.CPtr());
				//~ else
					//~ strBuf.Format((!MacroDlg->RecBufferSize
					                  //~ ? (DisFlags ? Msg::MacroDeleteAssign : Msg::MacroDeleteKey)
					                  //~ : Msg::MacroReDefinedKey), strKeyText.CPtr());

				//~ // проверим "а не совпадает ли всё?"
				//~ int Result=0;
				//~ if (!(!DisFlags &&
				        //~ Mac->Buffer && MacroDlg->RecBuffer &&
				        //~ Mac->BufferSize == MacroDlg->RecBufferSize &&
				        //~ (
				            //~ (Mac->BufferSize >  1 && !memcmp(Mac->Buffer,MacroDlg->RecBuffer,MacroDlg->RecBufferSize*sizeof(DWORD))) ||
				            //~ (Mac->BufferSize == 1 && (DWORD)(DWORD_PTR)Mac->Buffer == (DWORD)(DWORD_PTR)MacroDlg->RecBuffer)
				        //~ )
				   //~ ))
					//~ Result=Message(MSG_WARNING,2,Msg::Warning,
					          //~ strBuf,
					          //~ Msg::MacroSequence,
					          //~ strBufKey,
					          //~ (!MacroDlg->RecBufferSize?Msg::MacroDeleteKey2:
					              //~ (DisFlags?Msg::MacroDisDisabledKey:Msg::MacroReDefinedKey2)),
					          //~ (DisFlags && MacroDlg->RecBufferSize?Msg::MacroDisOverwrite:Msg::Yes),
					          //~ (DisFlags && MacroDlg->RecBufferSize?Msg::MacroDisAnotherKey:Msg::No));

				//~ if (!Result)
				//~ {
					//~ if (DisFlags)
					//~ {
						//~ // удаляем из реестра только если включен автосейв
						//~ if (Opt.AutoSaveSetup)
						//~ {
							//~ // удалим старую запись из реестра
							//~ ConfigWriter(strRegKeyName.GetMB()).RemoveSection();
						//~ }
						//~ // раздисаблим
						//~ Mac->Flags&=~MFLAGS_DISABLEMACRO;
					//~ }

					//~ // в любом случае - вываливаемся
					//~ SendDlgMessage(hDlg,DM_CLOSE,1,0);
					//~ return TRUE;
				//~ }

				//~ // здесь - здесь мы нажимали "Нет", ну а на нет и суда нет
				//~ //  и значит очистим поле ввода.
				//~ strKeyText.Clear();
			//~ }
		//~ }

		KMParam->Recurse++;
		SendDlgMessage(hDlg,DM_SETTEXTPTR,2,(LONG_PTR)strKeyText.CPtr());
		KMParam->Recurse--;
		//if(Param2 == KEY_F1 && LastKey == KEY_F1)
		//LastKey=-1;
		//else
		LastKey=(int)Param2;
		return TRUE;
	}
	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

DWORD KeyMacro::AssignMacroKey()
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
	DlgParam Param={this,0,m_StartMode,0};
	//_SVS(SysLog(L"StartMode=%d",m_StartMode));
	IsProcessAssignMacroKey++;
	Dialog Dlg(MacroAssignDlg,ARRAYSIZE(MacroAssignDlg),AssignMacroDlgProc,(LONG_PTR)&Param);
	Dlg.SetPosition(-1,-1,34,6);
	Dlg.SetHelp(L"KeyMacro");
	Dlg.Process();
	IsProcessAssignMacroKey--;

	if (Dlg.GetExitCode() == -1)
		return KEY_INVALID;

	return Param.Key;
}

static int Set3State(DWORD Flags,DWORD Chk1,DWORD Chk2)
{
	DWORD Chk12=Chk1|Chk2, FlagsChk12=Flags&Chk12;

	if (FlagsChk12 == Chk12 || !FlagsChk12)
		return (2);
	else
		return (Flags&Chk1?1:0);
}

enum MACROSETTINGSDLG
{
	MS_DOUBLEBOX,
	MS_TEXT_SEQUENCE,
	MS_EDIT_SEQUENCE,
	MS_SEPARATOR1,
	MS_CHECKBOX_OUPUT,
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
	static DlgParam *KMParam=nullptr;

	switch (Msg)
	{
		case DN_INITDIALOG:
			KMParam=(DlgParam *)Param2;
			break;
		case DN_BTNCLICK:

			if (Param1==MS_CHECKBOX_A_PANEL || Param1==MS_CHECKBOX_P_PANEL)
				for (int i=1; i<=3; i++)
					SendDlgMessage(hDlg,DM_ENABLE,Param1+i,Param2);

			break;
		case DN_CLOSE:

			if (Param1==MS_BUTTON_OK)
			{
				MacroRecord mr{};
				KeyMacro *Macro=KMParam->Handle;
				LPCWSTR Sequence=(LPCWSTR)SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,MS_EDIT_SEQUENCE,0);

				if (*Sequence)
				{
					//### TODO if (Macro->ParseMacroString(&mr,Sequence))
					{
						free(Macro->RecBuffer);
						Macro->RecBufferSize=mr.BufferSize;
						Macro->RecBuffer=mr.Buffer;
						Macro->RecSrc=wcsdup(Sequence);
						return TRUE;
					}
				}

				return FALSE;
			}

			break;
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

int KeyMacro::GetMacroSettings(uint32_t Key,DWORD &Flags)
{
	/*
	          1         2         3         4         5         6
	   3456789012345678901234567890123456789012345678901234567890123456789
	 1 г=========== Параметры макрокоманды для 'CtrlP' ==================¬
	 2 | Последовательность:                                             |
	 3 | _______________________________________________________________ |
	 4 |-----------------------------------------------------------------|
	 5 | [ ] Разрешить во время выполнения вывод на экран                |
	 6 | [ ] Выполнять после запуска FAR                                 |
	 7 |-----------------------------------------------------------------|
	 8 | [ ] Активная панель             [ ] Пассивная панель            |
	 9 |   [?] На панели плагина           [?] На панели плагина         |
	10 |   [?] Выполнять для папок         [?] Выполнять для папок       |
	11 |   [?] Отмечены файлы              [?] Отмечены файлы            |
	12 |-----------------------------------------------------------------|
	13 | [?] Пустая командная строка                                     |
	14 | [?] Отмечен блок                                                |
	15 |-----------------------------------------------------------------|
	16 |               [ Продолжить ]  [ Отменить ]                      |
	17 L=================================================================+

	*/
	DialogDataEx MacroSettingsDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,69,17,{},0,L""},
		{DI_TEXT,5,2,0,2,{},0,Msg::MacroSequence},
		{DI_EDIT,5,3,67,3,{},DIF_FOCUS,L""},
		{DI_TEXT,3,4,0,4,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,5,5,0,5,{},0,Msg::MacroSettingsEnableOutput},
		{DI_CHECKBOX,5,6,0,6,{},0,Msg::MacroSettingsRunAfterStart},
		{DI_TEXT,3,7,0,7,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,5,8,0,8,{},0,Msg::MacroSettingsActivePanel},
		{DI_CHECKBOX,7,9,0,9,{2},DIF_3STATE|DIF_DISABLE,Msg::MacroSettingsPluginPanel},
		{DI_CHECKBOX,7,10,0,10,{2},DIF_3STATE|DIF_DISABLE,Msg::MacroSettingsFolders},
		{DI_CHECKBOX,7,11,0,11,{2},DIF_3STATE|DIF_DISABLE,Msg::MacroSettingsSelectionPresent},
		{DI_CHECKBOX,37,8,0,8,{},0,Msg::MacroSettingsPassivePanel},
		{DI_CHECKBOX,39,9,0,9,{2},DIF_3STATE|DIF_DISABLE,Msg::MacroSettingsPluginPanel},
		{DI_CHECKBOX,39,10,0,10,{2},DIF_3STATE|DIF_DISABLE,Msg::MacroSettingsFolders},
		{DI_CHECKBOX,39,11,0,11,{2},DIF_3STATE|DIF_DISABLE,Msg::MacroSettingsSelectionPresent},
		{DI_TEXT,3,12,0,12,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,5,13,0,13,{2},DIF_3STATE,Msg::MacroSettingsCommandLine},
		{DI_CHECKBOX,5,14,0,14,{2},DIF_3STATE,Msg::MacroSettingsSelectionBlockPresent},
		{DI_TEXT,3,15,0,15,{},DIF_SEPARATOR,L""},
		{DI_BUTTON,0,16,0,16,{},DIF_DEFAULT|DIF_CENTERGROUP,Msg::Ok},
		{DI_BUTTON,0,16,0,16,{},DIF_CENTERGROUP,Msg::Cancel}
	};
	MakeDialogItemsEx(MacroSettingsDlgData,MacroSettingsDlg);
	FARString strKeyText;
	KeyToText(Key,strKeyText);
	MacroSettingsDlg[MS_DOUBLEBOX].strData.Format(Msg::MacroSettingsTitle, strKeyText.CPtr());
	//if(!(Key&0x7F000000))
	//MacroSettingsDlg[3].Flags|=DIF_DISABLE;
	MacroSettingsDlg[MS_CHECKBOX_OUPUT].Selected=Flags&MFLAGS_DISABLEOUTPUT?0:1;
	MacroSettingsDlg[MS_CHECKBOX_START].Selected=Flags&MFLAGS_RUNAFTERFARSTART?1:0;
	MacroSettingsDlg[MS_CHECKBOX_A_PLUGINPANEL].Selected=Set3State(Flags,MFLAGS_NOFILEPANELS,MFLAGS_NOPLUGINPANELS);
	MacroSettingsDlg[MS_CHECKBOX_A_FOLDERS].Selected=Set3State(Flags,MFLAGS_NOFILES,MFLAGS_NOFOLDERS);
	MacroSettingsDlg[MS_CHECKBOX_A_SELECTION].Selected=Set3State(Flags,MFLAGS_SELECTION,MFLAGS_NOSELECTION);
	MacroSettingsDlg[MS_CHECKBOX_P_PLUGINPANEL].Selected=Set3State(Flags,MFLAGS_PNOFILEPANELS,MFLAGS_PNOPLUGINPANELS);
	MacroSettingsDlg[MS_CHECKBOX_P_FOLDERS].Selected=Set3State(Flags,MFLAGS_PNOFILES,MFLAGS_PNOFOLDERS);
	MacroSettingsDlg[MS_CHECKBOX_P_SELECTION].Selected=Set3State(Flags,MFLAGS_PSELECTION,MFLAGS_PNOSELECTION);
	MacroSettingsDlg[MS_CHECKBOX_CMDLINE].Selected=Set3State(Flags,MFLAGS_EMPTYCOMMANDLINE,MFLAGS_NOTEMPTYCOMMANDLINE);
	MacroSettingsDlg[MS_CHECKBOX_SELBLOCK].Selected=Set3State(Flags,MFLAGS_EDITSELECTION,MFLAGS_EDITNOSELECTION);
	LPWSTR Sequence=MkTextSequence(RecBuffer,RecBufferSize);
	MacroSettingsDlg[MS_EDIT_SEQUENCE].strData=Sequence;
	free(Sequence);
	DlgParam Param={this,0,0,0};
	Dialog Dlg(MacroSettingsDlg,ARRAYSIZE(MacroSettingsDlg),ParamMacroDlgProc,(LONG_PTR)&Param);
	Dlg.SetPosition(-1,-1,73,19);
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

	Flags=MacroSettingsDlg[MS_CHECKBOX_OUPUT].Selected?0:MFLAGS_DISABLEOUTPUT;
	Flags|=MacroSettingsDlg[MS_CHECKBOX_START].Selected?MFLAGS_RUNAFTERFARSTART:0;

	if (MacroSettingsDlg[MS_CHECKBOX_A_PANEL].Selected)
	{
		Flags|=MacroSettingsDlg[MS_CHECKBOX_A_PLUGINPANEL].Selected==2?0:
		       (MacroSettingsDlg[MS_CHECKBOX_A_PLUGINPANEL].Selected==0?MFLAGS_NOPLUGINPANELS:MFLAGS_NOFILEPANELS);
		Flags|=MacroSettingsDlg[MS_CHECKBOX_A_FOLDERS].Selected==2?0:
		       (MacroSettingsDlg[MS_CHECKBOX_A_FOLDERS].Selected==0?MFLAGS_NOFOLDERS:MFLAGS_NOFILES);
		Flags|=MacroSettingsDlg[MS_CHECKBOX_A_SELECTION].Selected==2?0:
		       (MacroSettingsDlg[MS_CHECKBOX_A_SELECTION].Selected==0?MFLAGS_NOSELECTION:MFLAGS_SELECTION);
	}

	if (MacroSettingsDlg[MS_CHECKBOX_P_PANEL].Selected)
	{
		Flags|=MacroSettingsDlg[MS_CHECKBOX_P_PLUGINPANEL].Selected==2?0:
		       (MacroSettingsDlg[MS_CHECKBOX_P_PLUGINPANEL].Selected==0?MFLAGS_PNOPLUGINPANELS:MFLAGS_PNOFILEPANELS);
		Flags|=MacroSettingsDlg[MS_CHECKBOX_P_FOLDERS].Selected==2?0:
		       (MacroSettingsDlg[MS_CHECKBOX_P_FOLDERS].Selected==0?MFLAGS_PNOFOLDERS:MFLAGS_PNOFILES);
		Flags|=MacroSettingsDlg[MS_CHECKBOX_P_SELECTION].Selected==2?0:
		       (MacroSettingsDlg[MS_CHECKBOX_P_SELECTION].Selected==0?MFLAGS_PNOSELECTION:MFLAGS_PSELECTION);
	}

	Flags|=MacroSettingsDlg[MS_CHECKBOX_CMDLINE].Selected==2?0:
	       (MacroSettingsDlg[MS_CHECKBOX_CMDLINE].Selected==0?MFLAGS_NOTEMPTYCOMMANDLINE:MFLAGS_EMPTYCOMMANDLINE);
	Flags|=MacroSettingsDlg[MS_CHECKBOX_SELBLOCK].Selected==2?0:
	       (MacroSettingsDlg[MS_CHECKBOX_SELBLOCK].Selected==0?MFLAGS_EDITNOSELECTION:MFLAGS_EDITSELECTION);
	return TRUE;
}

bool KeyMacro::PostNewMacro(const wchar_t* Sequence,FARKEYMACROFLAGS InputFlags,DWORD AKey)
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

void MacroState::Init(TVarTable *tbl)
{
	KeyProcess=Executing=MacroPC=ExecLIBPos=MacroWORKCount=0;
	MacroWORK=nullptr;

	if (!tbl)
	{
		AllocVarTable=true;
		locVarTable=(TVarTable*)malloc(sizeof(TVarTable));
		initVTable(*locVarTable);
	}
	else
	{
		AllocVarTable=false;
		locVarTable=tbl;
	}
}

// получить название моды по коду
const wchar_t* KeyMacro::GetSubKey(int Mode)
{
	return (Mode >= MACRO_FUNCS && Mode < MACRO_LAST)?MKeywordsArea[Mode+3].Name:L"";
}

// получить код моды по имени
int KeyMacro::GetSubKey(const wchar_t *Mode)
{
	for (int i=MACRO_FUNCS; i < MACRO_LAST; i++)
		if (!StrCmpI(MKeywordsArea[i+3].Name,Mode))
			return i;

	return MACRO_FUNCS-1;
}

BOOL KeyMacro::CheckEditSelected(DWORD CurFlags)
{
	if (Mode==MACRO_EDITOR || Mode==MACRO_DIALOG || Mode==MACRO_VIEWER || (Mode==MACRO_SHELL&&CtrlObject->CmdLine->IsVisible()))
	{
		int NeedType = Mode == MACRO_EDITOR?MODALTYPE_EDITOR:(Mode == MACRO_VIEWER?MODALTYPE_VIEWER:(Mode == MACRO_DIALOG?MODALTYPE_DIALOG:MODALTYPE_PANELS));
		Frame* CurFrame=FrameManager->GetCurrentFrame();

		if (CurFrame && CurFrame->GetType()==NeedType)
		{
			int CurSelected;

			if (Mode==MACRO_SHELL && CtrlObject->CmdLine->IsVisible())
				CurSelected=(int)CtrlObject->CmdLine->VMProcess(MCODE_C_SELECTED);
			else
				CurSelected=(int)CurFrame->VMProcess(MCODE_C_SELECTED);

			if (((CurFlags&MFLAGS_EDITSELECTION) && !CurSelected) ||	((CurFlags&MFLAGS_EDITNOSELECTION) && CurSelected))
				return FALSE;
		}
	}

	return TRUE;
}

BOOL KeyMacro::CheckInsidePlugin(DWORD CurFlags)
{
	if (CtrlObject && CtrlObject->Plugins.CurPluginItem && (CurFlags&MFLAGS_NOSENDKEYSTOPLUGINS)) // ?????
		//if(CtrlObject && CtrlObject->Plugins.CurEditor && (CurFlags&MFLAGS_NOSENDKEYSTOPLUGINS))
		return FALSE;

	return TRUE;
}

BOOL KeyMacro::CheckCmdLine(int CmdLength,DWORD CurFlags)
{
	if (((CurFlags&MFLAGS_EMPTYCOMMANDLINE) && CmdLength) || ((CurFlags&MFLAGS_NOTEMPTYCOMMANDLINE) && CmdLength==0))
		return FALSE;

	return TRUE;
}

BOOL KeyMacro::CheckPanel(int PanelMode,DWORD CurFlags,BOOL IsPassivePanel)
{
	if (IsPassivePanel)
	{
		if ((PanelMode == PLUGIN_PANEL && (CurFlags&MFLAGS_PNOPLUGINPANELS)) || (PanelMode == NORMAL_PANEL && (CurFlags&MFLAGS_PNOFILEPANELS)))
			return FALSE;
	}
	else
	{
		if ((PanelMode == PLUGIN_PANEL && (CurFlags&MFLAGS_NOPLUGINPANELS)) || (PanelMode == NORMAL_PANEL && (CurFlags&MFLAGS_NOFILEPANELS)))
			return FALSE;
	}

	return TRUE;
}

BOOL KeyMacro::CheckFileFolder(Panel *CheckPanel,DWORD CurFlags, BOOL IsPassivePanel)
{
	FARString strFileName;
	DWORD FileAttr=INVALID_FILE_ATTRIBUTES;
	CheckPanel->GetFileName(strFileName,CheckPanel->GetCurrentPos(),FileAttr);

	if (FileAttr != INVALID_FILE_ATTRIBUTES)
	{
		if (IsPassivePanel)
		{
			if (((FileAttr&FILE_ATTRIBUTE_DIRECTORY) && (CurFlags&MFLAGS_PNOFOLDERS)) || (!(FileAttr&FILE_ATTRIBUTE_DIRECTORY) && (CurFlags&MFLAGS_PNOFILES)))
				return FALSE;
		}
		else
		{
			if (((FileAttr&FILE_ATTRIBUTE_DIRECTORY) && (CurFlags&MFLAGS_NOFOLDERS)) || (!(FileAttr&FILE_ATTRIBUTE_DIRECTORY) && (CurFlags&MFLAGS_NOFILES)))
				return FALSE;
		}
	}

	return TRUE;
}

BOOL KeyMacro::CheckAll(int /*CheckMode*/,DWORD CurFlags)
{
	/* $TODO:
		Здесь вместо Check*() попробовать заюзать IfCondition()
		для исключения повторяющегося кода.
	*/
	if (!CheckInsidePlugin(CurFlags))
		return FALSE;

	// проверка на пусто/не пусто в ком.строке (а в редакторе? :-)
	if (CurFlags&(MFLAGS_EMPTYCOMMANDLINE|MFLAGS_NOTEMPTYCOMMANDLINE))
		if (CtrlObject->CmdLine && !CheckCmdLine(CtrlObject->CmdLine->GetLength(),CurFlags))
			return FALSE;

	FilePanels *Cp=CtrlObject->Cp();

	if (!Cp)
		return FALSE;

	// проверки панели и типа файла
	Panel *ActivePanel=Cp->ActivePanel;
	Panel *PassivePanel=Cp->GetAnotherPanel(Cp->ActivePanel);

	if (ActivePanel && PassivePanel)// && (CurFlags&MFLAGS_MODEMASK)==MACRO_SHELL)
	{
		if (CurFlags&(MFLAGS_NOPLUGINPANELS|MFLAGS_NOFILEPANELS))
			if (!CheckPanel(ActivePanel->GetMode(),CurFlags,FALSE))
				return FALSE;

		if (CurFlags&(MFLAGS_PNOPLUGINPANELS|MFLAGS_PNOFILEPANELS))
			if (!CheckPanel(PassivePanel->GetMode(),CurFlags,TRUE))
				return FALSE;

		if (CurFlags&(MFLAGS_NOFOLDERS|MFLAGS_NOFILES))
			if (!CheckFileFolder(ActivePanel,CurFlags,FALSE))
				return FALSE;

		if (CurFlags&(MFLAGS_PNOFOLDERS|MFLAGS_PNOFILES))
			if (!CheckFileFolder(PassivePanel,CurFlags,TRUE))
				return FALSE;

		if (CurFlags&(MFLAGS_SELECTION|MFLAGS_NOSELECTION|MFLAGS_PSELECTION|MFLAGS_PNOSELECTION))
			if (Mode!=MACRO_EDITOR && Mode != MACRO_DIALOG && Mode!=MACRO_VIEWER)
			{
				int SelCount=ActivePanel->GetRealSelCount();

				if (((CurFlags&MFLAGS_SELECTION) && SelCount < 1) || ((CurFlags&MFLAGS_NOSELECTION) && SelCount >= 1))
					return FALSE;

				SelCount=PassivePanel->GetRealSelCount();

				if (((CurFlags&MFLAGS_PSELECTION) && SelCount < 1) || ((CurFlags&MFLAGS_PNOSELECTION) && SelCount >= 1))
					return FALSE;
			}
	}

	if (!CheckEditSelected(CurFlags))
		return FALSE;

	return TRUE;
}

static int __cdecl SortMacros(const MacroRecord *el1,const MacroRecord *el2)
{
	int Mode1, Mode2;

	if ((Mode1=(el1->Flags&MFLAGS_MODEMASK)) == (Mode2=(el2->Flags&MFLAGS_MODEMASK)))
		return 0;

	if (Mode1 < Mode2)
		return -1;

	return 1;
}

DWORD KeyMacro::GetOpCode(MacroRecord *MR,int PC)
{
	DWORD OpCode=(MR->BufferSize > 1)?MR->Buffer[PC]:(DWORD)(DWORD_PTR)MR->Buffer;
	return OpCode;
}

bool KeyMacro::CheckWaitKeyFunc() const
{
	return m_WaitKey != 0;
}
