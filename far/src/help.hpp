#pragma once

/*
help.hpp

Помощь
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
#include "chgmmode.hpp"
#include "stddlg.hpp"

class CallBackStack;

#define HelpBeginLink L'<'
#define HelpEndLink L'>'
#define HelpFormatLink L"<%ls/>%ls"

#define HELPMODE_CLICKOUTSIDE  0x20000000 // было нажатие мыши вне хелпа?

struct StackHelpData
{
	DWORD Flags;                  // флаги
	int   TopStr;                 // номер верхней видимой строки темы
	int   CurX,CurY;              // координаты (???)

	FARString strHelpMask;           // значение маски
	FARString strHelpPath;           // путь к хелпам
	FARString strHelpTopic;         // текущий топик
	FARString strSelTopic;          // выделенный топик (???)

	void Clear()
	{
		Flags=0;
		TopStr=0;
		CurX=CurY=0;
		strHelpMask.Clear();
		strHelpPath.Clear();
		strHelpTopic.Clear();
		strSelTopic.Clear();
	}
};

enum HELPDOCUMENTSHELPTYPE
{
	HIDX_PLUGINS,                 // Индекс плагинов
	HIDX_DOCUMS,                  // Индекс документов
};

enum
{
	FHELPOBJ_ERRCANNOTOPENHELP  = 0x80000000,
};

class HelpRecord
{
	public:
		FARString HelpStr;

		HelpRecord(const wchar_t *HStr=nullptr):HelpStr(HStr){};

		HelpRecord& operator=(const HelpRecord &rhs)
		{
			if (this != &rhs)
			{
				HelpStr = rhs.HelpStr;
			}
			return *this;
		}

		bool operator==(const HelpRecord &rhs) const
		{
			return !StrCmpI(HelpStr,rhs.HelpStr);
		}

		bool operator <(const HelpRecord &rhs) const
		{
			return StrCmpI(HelpStr,rhs.HelpStr) < 0;
		}
};

class Help:public Frame
{
private:
	ChangeMacroArea Cma;
	bool  ErrorHelp;            // true - ошибка! Например - нет такого топика
	SaveScreen *TopScreen;      // область сохранения под хелпом
	KeyBar      HelpKeyBar;     // кейбар
	CallBackStack *Stack;       // стек возврата
	FARString  strFullHelpPathName;

	StackHelpData StackData;
	std::vector<HelpRecord> HelpList; // "хелп" в памяти.

	int   StrCount;             // количество строк в теме
	int   FixCount;             // количество строк непрокручиваемой области
	int   FixSize;              // Размер непрокручиваемой области
	bool  TopicFound;           // true - топик найден
	bool  IsNewTopic;           // это новый топик?
	bool  MouseDown;

	FARString strCtrlColorChar;    // CtrlColorChar - опция! для спецсимвола-
	//   символа - для атрибутов
	uint64_t CurColor;			// CurColor - текущий цвет отрисовки
	int   CtrlTabSize;          // CtrlTabSize - опция! размер табуляции

	FARString strCurPluginContents; // помним PluginContents (для отображения в заголовке)

	DWORD LastStartPos;
	DWORD StartPos;

	FARString strCtrlStartPosChar;

	SearchReplaceDlgParams LastSearch;
private:
	void DisplayObject() override;
	bool ReadHelp(const wchar_t *Mask=nullptr);
	void AddLine(const wchar_t *Line);
	void AddTitle(const wchar_t *Title);
	void HighlightsCorrection(FARString &strStr);
	void FastShow();
	void DrawWindowFrame();
	void OutString(const wchar_t *Str);
	int  StringLen(const wchar_t *Str);
	void CorrectPosition();
	bool IsReferencePresent();
	void MoveToReference(int Forward,int CurScreen);
	void ReadDocumentsHelp(int TypeIndex);
	void Search(FILE *HelpFile,uintptr_t nCodePage);
	int  JumpTopic(const wchar_t *JumpTopic=nullptr);
	const HelpRecord* GetHelpItem(int Pos);

public:
	Help(const wchar_t *Topic,const wchar_t *Mask=nullptr,DWORD Flags=0);
	~Help() override;

public:
	void Hide() override;
	int  ProcessKey(FarKey Key) override;
	int  ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent) override;
	void InitKeyBar() override;
	bool GetError() {return ErrorHelp;}
	void SetScreenPosition() override;
	void OnChangeFocus(bool focus) override; // вызывается при смене фокуса
	void ResizeConsole() override;

	int  FastHide() override; // Введена для нужд CtrlAltShift

	const wchar_t *GetTypeName() const override {return L"[Help]";}
	int GetTypeAndName(FARString &strType, FARString &strName) override;
	int GetType() const override { return MODALTYPE_HELP; }

	int64_t VMProcess(int OpCode,void *vParam,int64_t iParam) override;

	static FARString &MkTopic(INT_PTR PluginNumber,const wchar_t *HelpTopic,FARString &strTopic);
};
