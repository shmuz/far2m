#pragma once

/*
stddlg.hpp

Куча разных стандартных диалогов
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

#include <WinCompat.h>
#include "FARString.hpp"


struct SearchReplaceDlgParams {
	FARString SearchStr;
	FARString ReplaceStr;
	int CaseSens = 0;    // -1 means the checkbox should not be shown in the dialog
	int WholeWords = 0;  // +++
	int Reverse = 0;     // +++
	int SelectFound = 0; // +++
	int Regexp = 0;      // +++
};

/*
	Функция GetSearchReplaceParams выводит диалог поиска или замены.

	Параметры:
		IsReplaceMode
				true - если хотим заменять; false - если хотим искать.
		Params
				Структура, содержащая входные параметры диалога. Если диалог не был отменён,
				то выходные параметры диалога будут записаны в эту же структуру.
		TextHistoryName
				Имя истории строки поиска.
				Если установлено в nullptr, то по умолчанию принимается значение "SearchText".
				Если установлено в пустую строку, то история вестись не будет.
		ReplaceHistoryName
				Имя истории строки замены.
				Если установлено в nullptr, то по умолчанию принимается значение "ReplaceText".
				Если установлено в пустую строку, то история вестись не будет.
		HelpTopic
				Имя темы помощи. Если nullptr или пустая строка - тема помощи не назначается.

	Возвращаемое значение:
		TRUE  - пользователь подтвердил свои намерения
		FALSE - пользователь отказался от диалога (Esc)
*/
int WINAPI GetSearchReplaceParams(
    bool IsReplaceMode,
		SearchReplaceDlgParams &Params,
    const wchar_t *TextHistoryName,
    const wchar_t *ReplaceHistoryName,
    const wchar_t *HelpTopic = nullptr
);

int WINAPI GetString(
    const wchar_t *Title,
    const wchar_t *SubTitle,
    const wchar_t *HistoryName,
    const wchar_t *SrcText,
    FARString &strDestText,
    const wchar_t *HelpTopic = nullptr,
    DWORD Flags = 0,
    int *CheckBoxValue = nullptr,
    const wchar_t *CheckBoxText = nullptr,
    const GUID *Guid = nullptr
);

// для диалога GetNameAndPassword()
enum FlagsNameAndPassword
{
	GNP_USELAST      = 0x00000001UL, // использовать последние введенные данные
};

int WINAPI GetNameAndPassword(const wchar_t *Title,FARString &strUserName, FARString &strPassword, const wchar_t *HelpTopic,DWORD Flags);
