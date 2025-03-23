/*
constitle.cpp

Заголовок консоли
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

#include "constitle.hpp"
#include "lang.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "ctrlobj.hpp"
#include "CriticalSections.hpp"
#include "console.hpp"
#include <stdarg.h>

bool ConsoleTitle::m_TitleModified = false;
DWORD ConsoleTitle::m_LastSetTime = 0;
FARString ConsoleTitle::m_FarTitle;

CriticalSection TitleCS;

static FARString FormatFarTitle(const FARString &CurrentTitle)
{
	static const FARString strVer(FAR_BUILD);
	static const FARString strPlatform(FAR_PLATFORM);
	/*
		%State    - current window title
		%Ver      - 2.3.102-beta
		%Platform - x86
		%Backend  - gui
		%Admin    - Msg::FarTitleAddonsAdmin
	*/
	const auto &wsBackend = MB2Wide(WinPortBackendInfo(-1));
	FARString hostname, username;
	apiGetEnvironmentVariable("HOSTNAME", hostname);
	apiGetEnvironmentVariable("USER", username);

	FARString Target = Opt.strWindowTitle;
	ReplaceStrings(Target, L"%Ver", strVer);
	ReplaceStrings(Target, L"%Platform", strPlatform);
	ReplaceStrings(Target, L"%Backend", wsBackend.c_str());
	ReplaceStrings(Target, L"%Admin", (Opt.IsUserAdmin ? Msg::FarTitleAddonsAdmin : L""));
	ReplaceStrings(Target, L"%Host", hostname);
	ReplaceStrings(Target, L"%User", username);
	// сделаем эту замену последней во избежание случайных совпадений
	// подстрок из CurrentTitle с другими переменными
	ReplaceStrings(Target, L"%State", CurrentTitle);
	RemoveExternalSpaces(Target);

	return Target;
}

ConsoleTitle::ConsoleTitle(const wchar_t *title)
{
	CriticalSectionLock Lock(TitleCS);
	Console.GetTitle(m_OldTitle);

	if (title)
		SetFarTitle(title, true);
}

ConsoleTitle::~ConsoleTitle()
{
	CriticalSectionLock Lock(TitleCS);
	SetFarTitle(m_OldTitle, true, true);
}

void ConsoleTitle::Set(const wchar_t *fmt, ...)
{
	CriticalSectionLock Lock(TitleCS);
	wchar_t msg[2048];
	va_list argptr;
	va_start(argptr, fmt);
	vswprintf(msg, ARRAYSIZE(msg)-1, fmt, argptr);
	va_end(argptr);
	SetFarTitle(msg);
}

void ConsoleTitle::SetFarTitle(const wchar_t *Title, bool Force, bool Restoring)
{
	CriticalSectionLock Lock(TitleCS);

	FARString CurrentTitle(NullToEmpty(Title), 0x100);
	m_FarTitle = Restoring ? CurrentTitle : FormatFarTitle(CurrentTitle);
	m_TitleModified = true;

	FARString ConsTitle;
	Console.GetTitle(ConsTitle);

	if (ConsTitle != m_FarTitle && !CtrlObject->Macro.IsOutputDisabled())
	{
		auto CurTime = WINPORT(GetTickCount)();
		// if (CurTime - m_LastSetTime > RedrawTimeout || Force)
		{
			m_LastSetTime = CurTime;
			Console.SetTitle(m_FarTitle);
			m_TitleModified = true;
		}
	}
}

/*
	Title=nullptr для случая, когда нужно выставить пред.заголовок
	SetFarTitle(nullptr) - это не для всех!
	Этот вызов имеет право делать только макро-движок!
*/
void ConsoleTitle::SetFarTitle()
{
	CriticalSectionLock Lock(TitleCS);
	if (m_TitleModified) {
		Console.SetTitle(m_FarTitle);
		m_TitleModified = false;
		//_SVS(SysLog(L"  (nullptr)FarTitle='%s'",FarTitle));
	}
}
