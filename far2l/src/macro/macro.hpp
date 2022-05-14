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

#include "tvar.hpp"
#include "macroopcode.hpp"

template <typename T>
bool CheckStructSize(const T* s)
{
	return s && (s->StructSize >= sizeof(T));
}

#define ALIGNAS(value, alignment) ((value+(alignment-1))&~(alignment-1))
#define ALIGN(value) ALIGNAS(value, sizeof(void*))

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

// области действия макросов (начало исполнения) -  НЕ БОЛЕЕ 0xFF областей!
enum MACROMODEAREA
{
	// see also plugin.hpp # FARMACROAREA
	MACRO_OTHER                =   0, // Режим копирования текста с экрана, вертикальные меню
	MACRO_SHELL                =   1, // Файловые панели
	MACRO_VIEWER               =   2, // Внутренняя программа просмотра
	MACRO_EDITOR               =   3, // Редактор
	MACRO_DIALOG               =   4, // Диалоги
	MACRO_SEARCH               =   5, // Быстрый поиск в панелях
	MACRO_DISKS                =   6, // Меню выбора дисков
	MACRO_MAINMENU             =   7, // Основное меню
	MACRO_MENU                 =   8, // Прочие меню
	MACRO_HELP                 =   9, // Система помощи
	MACRO_INFOPANEL            =  10, // Информационная панель
	MACRO_QVIEWPANEL           =  11, // Панель быстрого просмотра
	MACRO_TREEPANEL            =  12, // Панель дерева папок
	MACRO_FINDFOLDER           =  13, // Поиск папок
	MACRO_USERMENU             =  14, // Меню пользователя
	MACRO_AUTOCOMPLETION       =  15, // Список автодополнения

	MACRO_COMMON,                     // ВЕЗДЕ! - должен быть предпоследним, т.к. приоритет самый низший !!!
	MACRO_LAST                        // Должен быть всегда последним! Используется в циклах
};

// коды возврата для KeyMacro::GetCurRecord()
enum MACRORECORDANDEXECUTETYPE
{
	MACROMODE_NOMACRO          =0,  // не в режиме макро
	MACROMODE_EXECUTING        =1,  // исполнение: без передачи плагину пимп
	MACROMODE_EXECUTING_COMMON =2,  // исполнение: с передачей плагину пимп
	MACROMODE_RECORDING        =3,  // запись: без передачи плагину пимп
	MACROMODE_RECORDING_COMMON =4,  // запись: с передачей плагину пимп
};

class Panel;

enum INTMF_FLAGS{
	IMFF_UNLOCKSCREEN               =0x00000001,
	IMFF_DISABLEINTINPUT            =0x00000002,
};

struct MacroRecord
{
	DWORD  Flags;         // Флаги макропоследовательности
	uint32_t    Key;           // Назначенная клавиша
	int    BufferSize;    // Размер буфера компилированной последовательности
	DWORD *Buffer;        // компилированная последовательность (OpCode) макроса
	wchar_t  *Src;           // оригинальный "текст" макроса
	wchar_t  *Description;   // описание макроса
	DWORD  Reserved[2];   // зарезервировано
};

#define STACKLEVEL      32

struct MacroPanelSelect {
	int     Action;
	DWORD   ActionFlags;
	int     Mode;
	int64_t Index;
	TVar    *Item;
};

class KeyMacro
{
public:
	KeyMacro();

	//static bool AddMacro(const UUID& PluginId, const MacroAddMacroV1* Data);
	//static bool DelMacro(const UUID& PluginId, void* Id);
	static bool ExecuteString(MacroExecuteString *Data);
	static bool GetMacroKeyInfo(const FARString& StrArea, int Pos, FARString &strKeyName, FARString &strDescription);
	static bool IsOutputDisabled();
	static bool IsExecuting() { return GetExecutingState() != MACROSTATE_NOMACRO; }
	static bool IsHistoryDisabled(int TypeHistory);
	static bool MacroExists(int Key, int Area, bool UseCommon);
	static void RunStartMacro();
	static bool SaveMacros(bool always);
	static void SetMacroConst(int ConstIndex, long long Value);
	static bool PostNewMacro(const wchar_t* Sequence, DWORD InputFlags, DWORD AKey = 0);

	intptr_t CallFar(intptr_t CheckCode, FarMacroCall* Data);
	bool CheckWaitKeyFunc() const;
	int  GetState() const;
	int  GetKey();
	static DWORD GetMacroParseError(COORD& ErrPos, FARString& ErrSrc);
	int GetArea() const { return m_Area; }
	const wchar_t* GetStringToPrint() const { return m_StringToPrint.CPtr(); }
	bool IsRecording() const { return m_Recording != MACROSTATE_NOMACRO; }
	bool LoadMacros(bool FromFar, bool InitedRAM=true, const FarMacroLoad *Data=nullptr);
	bool ParseMacroString(const wchar_t* Sequence,DWORD Flags,bool skipFile) const;
	int  PeekKey() const;
	//bool ProcessEvent(const FAR_INPUT_RECORD *Rec);
	void SetArea(int Area) { m_Area=Area; }
	void SuspendMacros(bool Suspend) { Suspend ? ++m_InternalInput : --m_InternalInput; }

private:
	static int GetExecutingState();
	//intptr_t AssignMacroDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2);
	int AssignMacroKey(DWORD& MacroKey);
	//bool GetMacroSettings(int Key, unsigned long long &Flags, string_view Src = {}, string_view Descr = {});
	//intptr_t ParamMacroDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2);
	void RestoreMacroChar() const;

	int m_Area;
	int m_StartMode;
	int m_Recording;
	static FARString m_RecCode;
	static FARString m_RecDescription;
	int m_InternalInput;
	int m_WaitKey;
	FARString m_StringToPrint;

	private:
		int RecBufferSize;
		DWORD *RecBuffer;
		wchar_t *RecSrc;

		class LockScreen *LockScr;

	private:
		int GetMacroSettings(uint32_t Key,DWORD &Flags);

		BOOL CheckEditSelected(DWORD CurFlags);
		BOOL CheckInsidePlugin(DWORD CurFlags);
		BOOL CheckPanel(int PanelMode,DWORD CurFlags, BOOL IsPassivePanel);
		BOOL CheckCmdLine(int CmdLength,DWORD Flags);
		BOOL CheckFileFolder(Panel *ActivePanel,DWORD CurFlags, BOOL IsPassivePanel);
		BOOL CheckAll(int CheckMode,DWORD CurFlags);

	private:
		static LONG_PTR WINAPI AssignMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
		static LONG_PTR WINAPI ParamMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);

	public:
		bool ProcessKey(DWORD Key);

		static wchar_t *MkTextSequence(DWORD *Buffer,int BufferSize,const wchar_t *Src=nullptr);
};

BOOL WINAPI KeyMacroToText(uint32_t Key,FARString &strKeyText0);
uint32_t WINAPI KeyNameMacroToKey(const wchar_t *Name);

inline bool IsMenuArea(int Area){return Area==MACRO_MAINMENU || Area==MACRO_MENU || Area==MACRO_DISKS || Area==MACRO_USERMENU || Area==MACRO_AUTOCOMPLETION;}
void Log(const char* str);
