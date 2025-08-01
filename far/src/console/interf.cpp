/*
interf.cpp

Консольные функции ввода-вывода
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

#include <stdarg.h>
#include <VT256ColorTable.h>

#include "interf.hpp"
#include "keyboard.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "ctrlobj.hpp"
#include "ConfigOpt.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "fileedit.hpp"
#include "manager.hpp"
#include "scrbuf.hpp"
#include "syslog.hpp"
#include "farcolors.hpp"
#include "strmix.hpp"
#include "console.hpp"
#include "vtshell.h"

BOOL WINAPI CtrlHandler(DWORD CtrlType);

static int CurX, CurY;
static DWORD64 CurColor;
static volatile DWORD CtrlHandlerEvent = std::numeric_limits<uint32_t>::max();

static CONSOLE_CURSOR_INFO InitialCursorInfo;

static SMALL_RECT windowholder_rect;

WCHAR Oem2Unicode[256];
WCHAR BoxSymbols[64];

COORD CurSize{};
SHORT ScrX = 0, ScrY = 0;
SHORT PrevScrX = -1, PrevScrY = -1;
DWORD InitialConsoleMode = 0;
FARString strInitTitle;
SMALL_RECT InitWindowRect;
COORD InitialSize;

// static HICON hOldLargeIcon=nullptr, hOldSmallIcon=nullptr;

const size_t StackBufferSize = 0x2000;

void InitConsole()
{
	InitRecodeOutTable();
	Console.GetCursorInfo(InitialCursorInfo);
	Console.SetControlHandler(CtrlHandler, TRUE);
	Console.GetMode(Console.GetInputHandle(), InitialConsoleMode);
	Console.GetTitle(strInitTitle);
	Console.GetWindowRect(InitWindowRect);
	Console.GetSize(InitialSize);
	CONSOLE_CURSOR_INFO InitCursorInfo;
	Console.GetCursorInfo(InitCursorInfo);

	ConfigOptAssertLoaded();

	// размер клавиатурной очереди = 1024 кода клавиши
	if (!KeyQueue)
		KeyQueue = new FarQueue<DWORD>(1024);

	SetFarConsoleMode();

	/*
		$ 09.04.2002 DJ
		если размер консольного буфера больше размера окна, выставим
		их равными
	*/
	SMALL_RECT WindowRect;
	Console.GetWindowRect(WindowRect);

	COORD InitSize{};
	GetVideoMode(InitSize);
	if (WindowRect.Left || WindowRect.Top || WindowRect.Right != InitSize.X - 1
			|| WindowRect.Bottom != InitSize.Y - 1) {
		COORD newSize;
		newSize.X = WindowRect.Right - WindowRect.Left + 1;
		newSize.Y = WindowRect.Bottom - WindowRect.Top + 1;
		Console.SetSize(newSize);
		GetVideoMode(InitSize);
	}

	GetVideoMode(CurSize);
	ScrBuf.FillBuf();
}

void CloseConsole()
{
	ScrBuf.Flush();
	Console.SetCursorInfo(InitialCursorInfo);
	ChangeConsoleMode(InitialConsoleMode);

	Console.SetTitle(strInitTitle);
	Console.SetSize(InitialSize);
	Console.SetWindowRect(InitWindowRect);
	Console.SetSize(InitialSize);

	delete KeyQueue;
	KeyQueue = nullptr;
}

void SetFarConsoleMode(BOOL SetsActiveBuffer)
{
	int Mode = ENABLE_WINDOW_INPUT;

	if (Opt.Mouse) {
		// ENABLE_EXTENDED_FLAGS actually disables all the extended flags.
		Mode|= ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
	} else {
		// если вдруг изменили опцию во время работы фара, то включим то что надо
		Mode|= InitialConsoleMode & (ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE);
	}

	if (SetsActiveBuffer)
		Console.SetActiveScreenBuffer(Console.GetOutputHandle());

	ChangeConsoleMode(Mode);
}

void ChangeConsoleMode(int Mode)
{
	DWORD CurrentConsoleMode;
	HANDLE hConIn = Console.GetInputHandle();
	Console.GetMode(hConIn, CurrentConsoleMode);

	if (CurrentConsoleMode != (DWORD)Mode)
		Console.SetMode(hConIn, Mode);
}

void SaveConsoleWindowRect()
{
	Console.GetWindowRect(windowholder_rect);
}

void RestoreConsoleWindowRect()
{
	SMALL_RECT WindowRect;
	Console.GetWindowRect(WindowRect);
	if (WindowRect.Right - WindowRect.Left < windowholder_rect.Right - windowholder_rect.Left
			|| WindowRect.Bottom - WindowRect.Top < windowholder_rect.Bottom - windowholder_rect.Top) {
		Console.SetWindowRect(windowholder_rect);
	}
}

void FlushInputBuffer()
{
	Console.FlushInputBuffer();
	MouseButtonState = 0;
	MouseEventFlags = 0;
}

void ToggleVideoMode()
{
	const COORD LargestSize = Console.GetLargestWindowSize();
	WINPORT(SetConsoleWindowMaximized)(LargestSize.X != CurSize.X || LargestSize.Y != CurSize.Y);
}

void GenerateWINDOW_BUFFER_SIZE_EVENT(int Sx, int Sy, bool Damaged)
{
	COORD Size;
	if (Sx == -1 || Sy == -1) {
		Console.GetSize(Size);
	}
	INPUT_RECORD Rec;
	Rec.EventType = WINDOW_BUFFER_SIZE_EVENT;
	Rec.Event.WindowBufferSizeEvent.dwSize.X = Sx == -1 ? Size.X : Sx;
	Rec.Event.WindowBufferSizeEvent.dwSize.Y = Sy == -1 ? Size.Y : Sy;
	Rec.Event.WindowBufferSizeEvent.bDamaged = Damaged ? TRUE : FALSE;
	DWORD Writes;
	Console.WriteInput(Rec, 1, Writes);
}

void GetVideoMode(COORD &Size)
{
	// чтоб решить баг винды приводящий к появлению скролов и т.п. после потери фокуса
	SaveConsoleWindowRect();
	Size.X = 0;
	Size.Y = 0;
	Console.GetSize(Size);
	ScrX = Size.X - 1;
	ScrY = Size.Y - 1;
	ASSERT(ScrX > 0);
	ASSERT(ScrY > 0);
	WidthNameForMessage = (ScrX * 38) / 100 + 1;

	if (PrevScrX == -1)
		PrevScrX = ScrX;

	if (PrevScrY == -1)
		PrevScrY = ScrY;

	_OT(SysLog(L"ScrX=%d ScrY=%d", ScrX, ScrY));
	ScrBuf.AllocBuf(Size.X, Size.Y);
	_OT(ViewConsoleInfo());
}

BOOL WINAPI CtrlHandler(DWORD CtrlType)
{
	// write some dummy console input to kick any pending ReadConsoleInput
	CtrlHandlerEvent = CtrlType;
	INPUT_RECORD ir = {};
	ir.EventType = NOOP_EVENT;
	DWORD dw = 0;
	WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
	return TRUE;
}

void CheckForPendingCtrlHandleEvent()
{
	const DWORD CtrlType = CtrlHandlerEvent;

	if (CtrlType == std::numeric_limits<uint32_t>::max())
		return;

	CtrlHandlerEvent = std::numeric_limits<uint32_t>::max();

	/*
		TODO: need handle
			CTRL_CLOSE_EVENT
			CTRL_LOGOFF_EVENT
			CTRL_SHUTDOWN_EVENT
	*/
	if (CtrlType == CTRL_C_EVENT || CtrlType == CTRL_BREAK_EVENT) {
		if (CtrlType == CTRL_BREAK_EVENT)
			WriteInput(KEY_BREAK);

		if (CtrlObject && CtrlObject->Cp()) {
			if (CtrlObject->Cp()->LeftPanel && CtrlObject->Cp()->LeftPanel->GetMode() == PLUGIN_PANEL)
				CtrlObject->Plugins.ProcessEvent(CtrlObject->Cp()->LeftPanel->GetPluginHandle(), FE_BREAK,
						(void *)(DWORD_PTR)CtrlType);

			if (CtrlObject->Cp()->RightPanel && CtrlObject->Cp()->RightPanel->GetMode() == PLUGIN_PANEL)
				CtrlObject->Plugins.ProcessEvent(CtrlObject->Cp()->RightPanel->GetPluginHandle(), FE_BREAK,
						(void *)(DWORD_PTR)CtrlType);
		}

		return;
	}

	if (CtrlObject && !CtrlObject->Plugins.MayExitFar()) {
		return;
	}

	CloseFAR = TRUE;
}

void ShowTime(int ShowAlways)
{
	FARString strClockText;
	static SYSTEMTIME lasttm = {0, 0, 0, 0, 0, 0, 0, 0};
	SYSTEMTIME tm;
	WINPORT(GetLocalTime)(&tm);
	CHAR_INFO ScreenClockText[5];
	GetText(ScrX - 4, 0, ScrX, 0, ScreenClockText, sizeof(ScreenClockText));

	if (ShowAlways == 2) {
		memset(&lasttm, 0, sizeof(lasttm));
		return;
	}

	if ((!ShowAlways && lasttm.wMinute == tm.wMinute && lasttm.wHour == tm.wHour
				&& ScreenClockText[2].Char.UnicodeChar == L':')
			|| ScreenSaverActive)
		return;

	ProcessShowClock++;
	lasttm = tm;
	strClockText.Format(L"%02d:%02d", tm.wHour, tm.wMinute);
	GotoXY(ScrX - 4, 0);
	// Здесь хрень какая-то получается с ModType - все время не верное значение!
	Frame *CurFrame = FrameManager->GetCurrentFrame();

	if (CurFrame) {
		int ModType = CurFrame->GetType();
		SetColor(FarColorToReal(ModType == MODALTYPE_VIEWER
						? COL_VIEWERCLOCK
						: (ModType == MODALTYPE_EDITOR ? COL_EDITORCLOCK : COL_CLOCK)));
		Text(strClockText);
		// ScrBuf.Flush();
	}

	ProcessShowClock--;
}

void GotoXY(int X, int Y)
{
	CurX = X;
	CurY = Y;
}

int WhereX()
{
	return (CurX);
}

int WhereY()
{
	return (CurY);
}

void MoveCursor(int X, int Y)
{
	ScrBuf.MoveCursor(X, Y);
}

void GetCursorPos(SHORT &X, SHORT &Y)
{
	ScrBuf.GetCursorPos(X, Y);
}

void SetCursorType(bool Visible, DWORD Size)
{
	if (Size == (DWORD)-1 || !Visible)
		Size = (Opt.CursorSize[0] ? Opt.CursorSize[0] : InitialCursorInfo.dwSize);

	ScrBuf.SetCursorType(Visible, Size);
}

void SetInitialCursorType()
{
	ScrBuf.SetCursorType(InitialCursorInfo.bVisible != FALSE, InitialCursorInfo.dwSize);
}

void GetCursorType(bool &Visible, DWORD &Size)
{
	ScrBuf.GetCursorType(Visible, Size);
}

void MoveRealCursor(int X, int Y)
{
	COORD C = {(SHORT)X, (SHORT)Y};
	Console.SetCursorPosition(C);
}

void GetRealCursorPos(SHORT &X, SHORT &Y)
{
	COORD CursorPosition;
	Console.GetCursorPosition(CursorPosition);
	X = CursorPosition.X;
	Y = CursorPosition.Y;
}

void InitRecodeOutTable()
{
	for (size_t i = 0; i < ARRAYSIZE(Oem2Unicode); i++) {
		char c = static_cast<char>(i);
		WINPORT(MultiByteToWideChar)(CP_OEMCP, MB_USEGLYPHCHARS, &c, 1, &Oem2Unicode[i], 1);
	}

	if (Opt.CleanAscii) {
		for (size_t i = 0; i < 0x20; i++)
			Oem2Unicode[i] = L'.';

		Oem2Unicode[0x07] = L'*';
		Oem2Unicode[0x10] = L'>';
		Oem2Unicode[0x11] = L'<';
		Oem2Unicode[0x15] = L'$';
		Oem2Unicode[0x16] = L'-';
		Oem2Unicode[0x18] = L'|';
		Oem2Unicode[0x19] = L'V';
		Oem2Unicode[0x1A] = L'>';
		Oem2Unicode[0x1B] = L'<';
		Oem2Unicode[0x1E] = L'X';
		Oem2Unicode[0x1F] = L'X';

		Oem2Unicode[0x7F] = L'.';
	}

	if (Opt.NoGraphics) {
		for (int i = 0xB3; i <= 0xDA; i++) {
			Oem2Unicode[i] = L'+';
		}
		Oem2Unicode[0xB3] = L'|';
		Oem2Unicode[0xBA] = L'|';
		Oem2Unicode[0xC4] = L'-';
		Oem2Unicode[0xCD] = L'=';
	}

	{
		static WCHAR _BoxSymbols[48] = {
				0x2591,
				0x2592,
				0x2593,
				0x2502,
				0x2524,
				0x2561,
				0x2562,
				0x2556,
				0x2555,
				0x2563,
				0x2551,
				0x2557,
				0x255D,
				0x255C,
				0x255B,
				0x2510,
				0x2514,
				0x2534,
				0x252C,
				0x251C,
				0x2500,
				0x253C,
				0x255E,
				0x255F,
				0x255A,
				0x2554,
				0x2569,
				0x2566,
				0x2560,
				0x2550,
				0x256C,
				0x2567,
				0x2568,
				0x2564,
				0x2565,
				0x2559,
				0x2558,
				0x2552,
				0x2553,
				0x256B,
				0x256A,
				0x2518,
				0x250C,
				0x2588,
				0x2584,
				0x258C,
				0x2590,
				0x2580,
		};
		// перед [пере]инициализацией восстановим буфер
		memcpy(BoxSymbols, (BYTE *)_BoxSymbols, sizeof(_BoxSymbols));

		if (Opt.NoGraphics) {
			for (int i = BS_V1; i <= BS_LT_H1V1; i++)
				BoxSymbols[i] = L'+';

			BoxSymbols[BS_V1] = BoxSymbols[BS_V2] = L'|';
			BoxSymbols[BS_H1] = L'-';
			BoxSymbols[BS_H2] = L'=';
		}

		if (Opt.NoBoxes) {
			for (int i = BS_V1; i <= BS_LT_H1V1; i++)
				BoxSymbols[i] = L' ';

			BoxSymbols[BS_V1] = BoxSymbols[BS_V2] = L' ';
			BoxSymbols[BS_H1] = L' ';
			BoxSymbols[BS_H2] = L' ';
		}
	}

	//_SVS(SysLogDump("Oem2Unicode",0,(LPBYTE)Oem2Unicode,sizeof(Oem2Unicode),nullptr));
}

void Text(int X, int Y, uint64_t Color, const WCHAR *Str, size_t Length)
{
	CurColor = Color;
	CurX = X;
	CurY = Y;
	Text(Str, Length);
}

void Text(int X, int Y, uint64_t Color, const WCHAR *Str)
{
	CurColor = Color;
	CurX = X;
	CurY = Y;
	Text(Str);
}

void Text(const WCHAR Ch, uint64_t Color, size_t Length)
{
	if ( !Length )
		return;

	int X1 = CurX;
	int Y1 = CurY;
	int X2 = CurX + Length;
	int Y2 = CurY;

	if (X1 < 0)
		X1 = 0;
	if (Y1 < 0)
		Y1 = 0;

	if (X2 > ScrX)
		X2 = ScrX;
	if (Y2 > ScrY)
		Y2 = ScrY;

	ScrBuf.FillRect(X1, Y1, X2, Y2, Ch, Color);
	CurX += Length;
}

void Text(const WCHAR Ch, size_t Length)
{
	Text(Ch, CurColor, Length);
}

void Text(const WCHAR *Str, size_t Length)
{
	if (Length == (size_t)-1)
		Length = StrLength(Str);

	if (Length == 0)
		return;

	CHAR_INFO StackBuffer[StackBufferSize];
	PCHAR_INFO HeapBuffer = nullptr;
	PCHAR_INFO BufPtr = StackBuffer;

	if (Length * 2 >= StackBufferSize) {
		HeapBuffer = new CHAR_INFO[Length * 2 + 1];
		BufPtr = HeapBuffer;
	}

	int nCells = 0, Skipped = 0;
	std::wstring wstr;
	for (size_t i = 0; i < Length; ++nCells) {
		const size_t nG = StrSizeOfCell(&Str[i], Length - i);
		if (nG > 1) {
			wstr.assign(&Str[i], nG);
			CI_SET_COMPOSITE(BufPtr[nCells], wstr.c_str());
		} else {
			CI_SET_WCHAR(BufPtr[nCells], Str[i]);
		}
		CI_SET_ATTR(BufPtr[nCells], CurColor);
		CharClasses cc(Str[i]);
		if (cc.FullWidth()) {
			++nCells;
			CI_SET_WCATTR(BufPtr[nCells], 0, CurColor);
		} else if (cc.Xxxfix()) {
			++Skipped;
		}
		i+= nG;
	}

	ScrBuf.Write(CurX, CurY, BufPtr, nCells + Skipped);
	if (HeapBuffer) {
		delete[] HeapBuffer;
	}
	CurX+= nCells;
}

void TextEx(const WCHAR *Str, size_t Length)
{
	if (Length == (size_t)-1)
		Length = StrLength(Str);

	if (Length == 0)
		return;

	CHAR_INFO StackBuffer[StackBufferSize];
	PCHAR_INFO HeapBuffer = nullptr;
	PCHAR_INFO BufPtr = StackBuffer;

	if (Length * 2 >= StackBufferSize) {
		HeapBuffer = new CHAR_INFO[Length * 2 + 1];
		BufPtr = HeapBuffer;
	}

	int nCells = 0, Skipped = 0;
	std::wstring wstr;
	for (size_t i = 0; i < Length; ++nCells) {

		const size_t nG = StrSizeOfCell(&Str[i], Length - i);

		if (nG > 1) {
			wstr.assign(&Str[i], nG);
			CI_SET_COMPOSITE(BufPtr[nCells], wstr.c_str());
			CI_SET_ATTR(BufPtr[nCells], CurColor);
		} else {
			CI_SET_WCHAR(BufPtr[nCells], Str[i]);
			CI_SET_ATTR(BufPtr[nCells], CurColor & (0xFFFFFFFFFFFFFFFF ^ (COMMON_LVB_STRIKEOUT | COMMON_LVB_UNDERSCORE)) );
		}

//		CI_SET_ATTR(BufPtr[nCells], CurColor);
		CharClasses cc(Str[i]);
		if (cc.FullWidth()) {
			++nCells;
			CI_SET_WCATTR(BufPtr[nCells], 0, CurColor);
		} else	if (cc.Xxxfix()) {
			++Skipped;
		}
		i+= nG;
	}

	ScrBuf.Write(CurX, CurY, BufPtr, nCells + Skipped);
	if (HeapBuffer) {
		delete[] HeapBuffer;
	}
	CurX+= nCells;
}

void Text(FarLangMsg MsgId)
{
	Text(MsgId.CPtr());
}

void VText(const WCHAR *Str)
{
	int Length = StrLength(Str);

	if (Length <= 0)
		return;

	int StartCurX = CurX;
	WCHAR ChrStr[2] = {0, 0};

	for (int I = 0; I < Length; I++) {
		GotoXY(CurX, CurY);
		ChrStr[0] = Str[I];
		Text(ChrStr);
		CurY++;
		CurX = StartCurX;
	}
}

void HiText(const wchar_t *Str, uint64_t HiColor, int isVertText)
{
	FARString strTextStr;
	uint64_t SaveColor;
	size_t pos;
	strTextStr = Str;

	if (!strTextStr.Pos(pos, L'&')) {
		if (isVertText)
			VText(strTextStr);
		else
			Text(strTextStr);
	} else {
		//   &&      = '&'
		//   &&&     = '&'
		//              ^H
		//   &&&&    = '&&'
		//   &&&&&   = '&&'
		//              ^H
		//   &&&&&&  = '&&&'

		wchar_t *ChPtr = strTextStr.GetBuffer() + pos;
		int I = 0;
		wchar_t *ChPtr2 = ChPtr;

		while (*ChPtr2++ == L'&')
			++I;

		if (I & 1)		// нечет?
		{
			*ChPtr = 0;

			if (isVertText)
				VText(strTextStr);
			else
				Text(strTextStr);	// BUGBUG BAD!!!

			if (ChPtr[1]) {
				wchar_t Chr[] = {ChPtr[1], 0};
				SaveColor = CurColor;
				SetColor(HiColor);

				if (isVertText)
					VText(Chr);
				else
					Text(Chr);

				SetColor(SaveColor);
				FARString strText = (ChPtr + 1);
				strTextStr.ReleaseBuffer();
				ReplaceStrings(strText, L"&&", L"&", -1);

				if (isVertText)
					VText(strText.CPtr() + 1);
				else
					Text(strText.CPtr() + 1);
			}
		} else {
			strTextStr.ReleaseBuffer();
			ReplaceStrings(strTextStr, L"&&", L"&", -1);

			if (isVertText)
				VText(strTextStr);
			else
				Text(strTextStr);	// BUGBUG BAD!!!
		}
	}
}

void SetScreen(int X1, int Y1, int X2, int Y2, wchar_t Ch, uint64_t Color)
{
	if (X1 < 0)
		X1 = 0;

	if (Y1 < 0)
		Y1 = 0;

	if (X2 > ScrX)
		X2 = ScrX;

	if (Y2 > ScrY)
		Y2 = ScrY;

	ScrBuf.FillRect(X1, Y1, X2, Y2, Ch, Color);
}

void MakeShadow(int X1, int Y1, int X2, int Y2, SaveScreen *ss)
{
	if (X1 < 0)
		X1 = 0;

	if (Y1 < 0)
		Y1 = 0;

	if (X2 > ScrX)
		X2 = ScrX;

	if (Y2 > ScrY)
		Y2 = ScrY;

//	ScrBuf.ApplyColorMask(X1, Y1, X2, Y2, 0xFFFFFFFFFFFFFFF8);
	ScrBuf.ApplyShadow(X1, Y1, X2, Y2, ss);
}

void ChangeBlockColor(int X1, int Y1, int X2, int Y2, uint64_t Color)
{
	if (X1 < 0)
		X1 = 0;

	if (Y1 < 0)
		Y1 = 0;

	if (X2 > ScrX)
		X2 = ScrX;

	if (Y2 > ScrY)
		Y2 = ScrY;

	ScrBuf.ApplyColor(X1, Y1, X2, Y2, Color);
}

void mprintf(const wchar_t *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	wchar_t OutStr[2048];
	vswprintf(OutStr, ARRAYSIZE(OutStr) - 1, fmt, argptr);
	Text(OutStr);
	va_end(argptr);
}

void vmprintf(const wchar_t *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	wchar_t OutStr[2048];
	vswprintf(OutStr, ARRAYSIZE(OutStr) - 1, fmt, argptr);
	VText(OutStr);
	va_end(argptr);
}

void SetColor(uint64_t Color, bool ApplyToConsole)
{
	CurColor = Color;

	if (ApplyToConsole) {
		Console.SetTextAttributes(CurColor);
	}
}

void SetFarColor(uint16_t Color, bool ApplyToConsole)
{
	CurColor = FarColorToReal(Color);

	if (ApplyToConsole) {
		Console.SetTextAttributes(CurColor);
	}
}

void FarTrueColorFromRGB(FarTrueColor &out, DWORD rgb, bool used)
{
	out.Flags = used ? 1 : 0;
	out.R = rgb & 0xff;
	out.G = (rgb >> 8) & 0xff;
	out.B = (rgb >> 16) & 0xff;
}

void FarTrueColorFromAttributes(FarTrueColorForeAndBack &TFB, DWORD64 Attrs)
{
	FarTrueColorFromRGB(TFB.Fore, (Attrs >> 16) & 0xffffff, (Attrs & FOREGROUND_TRUECOLOR) != 0);
	FarTrueColorFromRGB(TFB.Back, (Attrs >> 40) & 0xffffff, (Attrs & BACKGROUND_TRUECOLOR) != 0);
}

void FarTrueColorToAttributes(DWORD64 &Attrs, const FarTrueColorForeAndBack &TFB)
{
	if (TFB.Fore.Flags & 1) {
		SET_RGB_FORE(Attrs, COMPOSE_RGB(TFB.Fore.R, TFB.Fore.G, TFB.Fore.B));
	}
	if (TFB.Back.Flags & 1) {
		SET_RGB_BACK(Attrs, COMPOSE_RGB(TFB.Back.R, TFB.Back.G, TFB.Back.B));
	}
}

void FarTrueColorFromRGB(FarTrueColor &out, DWORD rgb)
{
	FarTrueColorFromRGB(out, rgb, rgb != 0);
}

DWORD64 ComposeColor(WORD BaseColor, const FarTrueColorForeAndBack *TFB)
{
	DWORD64 Attrs = FarColorToReal(BaseColor);
	if (TFB) {
		FarTrueColorToAttributes(Attrs, *TFB);
	}
	return Attrs;
}

void ComposeAndSetColor(WORD BaseColor, const FarTrueColorForeAndBack *TrueColor, bool ApplyToConsole)
{
	CurColor = ComposeColor(BaseColor, TrueColor);
	if (ApplyToConsole) {
		Console.SetTextAttributes(CurColor);
	}
}

void ClearScreen(uint64_t Color)
{
	SetColor(Color);
	ScrBuf.FillRect(0, 0, ScrX, ScrY, L' ', Color);
	ScrBuf.ResetShadow();
	ScrBuf.Flush();
	Console.SetTextAttributes(Color);
}

uint64_t GetColor()
{
	return CurColor;
}

void ScrollScreen(int Count)
{
	ScrBuf.Scroll(Count);
	ScrBuf.FillRect(0, ScrY + 1 - Count, ScrX, ScrY, L' ', FarColorToReal(COL_COMMANDLINEUSERSCREEN));
}

void GetText(int X1, int Y1, int X2, int Y2, void *Dest, int DestSize)
{
	ScrBuf.Read(X1, Y1, X2, Y2, (CHAR_INFO *)Dest, DestSize);
}

void PutText(int X1, int Y1, int X2, int Y2, const void *Src)
{
	int Width = X2 - X1 + 1;
	int Y;
	CHAR_INFO *SrcPtr = (CHAR_INFO *)Src;

	for (Y = Y1; Y <= Y2; ++Y, SrcPtr+= Width)
		ScrBuf.Write(X1, Y, SrcPtr, Width);
}

void BoxText(wchar_t Chr)
{
	wchar_t Str[] = {Chr, L'\0'};
	BoxText(Str);
}

void BoxText(const wchar_t *Str, int IsVert)
{
	if (IsVert)
		VText(Str);
	else
		Text(Str);
}

/*
	Отрисовка прямоугольника.
*/
void Box(int x1, int y1, int x2, int y2, uint64_t Color, int Type)
{
	if (x1 >= x2 || y1 >= y2)
		return;

//	SetColor(Color);
	CurColor = Color;
	Type = (Type == DOUBLE_BOX || Type == SHORT_DOUBLE_BOX);

	WCHAR StackBuffer[StackBufferSize];
	LPWSTR HeapBuffer = nullptr;
	LPWSTR BufPtr = StackBuffer;

	const size_t height = y2 - y1;
	if (height > StackBufferSize) {
		HeapBuffer = new WCHAR[height];
		BufPtr = HeapBuffer;
	}
	wmemset(BufPtr, BoxSymbols[Type ? BS_V2 : BS_V1], height - 1);
	BufPtr[height - 1] = 0;
	GotoXY(x1, y1 + 1);
	VText(BufPtr);
	GotoXY(x2, y1 + 1);
	VText(BufPtr);
	const size_t width = x2 - x1 + 2;
	if (width > StackBufferSize) {
		if (width > height) {
			if (HeapBuffer) {
				delete[] HeapBuffer;
			}
			HeapBuffer = new WCHAR[width];
		}
		BufPtr = HeapBuffer;
	}
	BufPtr[0] = BoxSymbols[Type ? BS_LT_H2V2 : BS_LT_H1V1];
	wmemset(BufPtr + 1, BoxSymbols[Type ? BS_H2 : BS_H1], width - 3);
	BufPtr[width - 2] = BoxSymbols[Type ? BS_RT_H2V2 : BS_RT_H1V1];
	BufPtr[width - 1] = 0;
	GotoXY(x1, y1);
	Text(BufPtr);
	BufPtr[0] = BoxSymbols[Type ? BS_LB_H2V2 : BS_LB_H1V1];
	BufPtr[width - 2] = BoxSymbols[Type ? BS_RB_H2V2 : BS_RB_H1V1];
	GotoXY(x1, y2);
	Text(BufPtr);

	if (HeapBuffer) {
		delete[] HeapBuffer;
	}
}

bool ScrollBarRequired(UINT Length, UINT64 ItemsCount)
{
	return Length > 2 && ItemsCount && Length < ItemsCount;
}

bool ScrollBarEx(UINT X1, UINT Y1, UINT Length, UINT64 TopItem, UINT64 ItemsCount)
{
	if (ScrollBarRequired(Length, ItemsCount)) {
		Length-= 2;
		ItemsCount-= 2;
		UINT CaretPos = static_cast<UINT>(Round(Length * TopItem, ItemsCount));
		UINT CaretLength =
				Max(1U, static_cast<UINT>(Round(static_cast<UINT64>(Length * Length), ItemsCount)));

		if (!CaretPos && TopItem) {
			CaretPos++;
		} else if (CaretPos + CaretLength == Length && TopItem + 2 + Length < ItemsCount) {
			CaretPos--;
		}

		CaretPos = Min(CaretPos, Length - CaretLength);
		WCHAR StackBuffer[StackBufferSize];
		LPWSTR HeapBuffer = nullptr;
		LPWSTR BufPtr = StackBuffer;
		if (Length + 3 >= StackBufferSize) {
			HeapBuffer = new WCHAR[Length + 3];
			BufPtr = HeapBuffer;
		}
		wmemset(BufPtr + 1, BoxSymbols[BS_X_B0], Length);
		BufPtr[0] = Oem2Unicode[0x1E];

		for (size_t i = 0; i < CaretLength; i++)
			BufPtr[CaretPos + 1 + i] = BoxSymbols[BS_X_B2];

		BufPtr[Length + 1] = Oem2Unicode[0x1F];
		BufPtr[Length + 2] = 0;
		GotoXY(X1, Y1);
		VText(BufPtr);
		if (HeapBuffer) {
			delete[] HeapBuffer;
		}
		return true;
	}

	return false;
}

void ScrollBar(int X1, int Y1, int Length, unsigned int Current, unsigned int Total)
{
	int ThumbPos;

	if ((Length-= 2) < 1)
		return;

	if (Total > 0)
		ThumbPos = Length * Current / Total;
	else
		ThumbPos = 0;

	if (ThumbPos >= Length)
		ThumbPos = Length - 1;

	GotoXY(X1, Y1);
	{
		WCHAR StackBuffer[StackBufferSize];
		LPWSTR HeapBuffer = nullptr;
		LPWSTR BufPtr = StackBuffer;
		if (static_cast<size_t>(Length + 3) >= StackBufferSize) {
			HeapBuffer = new WCHAR[Length + 3];
			BufPtr = HeapBuffer;
		}
		wmemset(BufPtr + 1, BoxSymbols[BS_X_B0], Length);
		BufPtr[ThumbPos + 1] = BoxSymbols[BS_X_B2];
		BufPtr[0] = Oem2Unicode[0x1E];
		BufPtr[Length + 1] = Oem2Unicode[0x1F];
		BufPtr[Length + 2] = 0;
		VText(BufPtr);
		if (HeapBuffer) {
			delete[] HeapBuffer;
		}
	}
}

void DrawLine(int Length, int Type, const wchar_t *UserSep)
{
	if (Length > 1) {
		WCHAR StackBuffer[StackBufferSize];
		LPWSTR HeapBuffer = nullptr;
		LPWSTR BufPtr = StackBuffer;
		if (static_cast<size_t>(Length) >= StackBufferSize) {
			HeapBuffer = new WCHAR[Length + 1];
			BufPtr = HeapBuffer;
		}
		MakeSeparator(Length, BufPtr, Type, UserSep);

		(Type >= 4 && Type <= 7) || (Type >= 10 && Type <= 11) ? VText(BufPtr) : Text(BufPtr);
		if (HeapBuffer) {
			delete[] HeapBuffer;
		}
	}
}

// "Нарисовать" сепаратор в памяти.
WCHAR *MakeSeparator(int Length, WCHAR *DestStr, int Type, const wchar_t *UserSep)
{
	wchar_t BoxType[12][3] = {
			// h-horiz, s-space, v-vert, b-border, 1-one line, 2-two line
			/* 00 */ {L' ', L' ', BoxSymbols[BS_H1]},										//  -     h1s
			/* 01 */ {BoxSymbols[BS_L_H1V2], BoxSymbols[BS_R_H1V2], BoxSymbols[BS_H1]},		// ||-||  h1b2
			/* 02 */ {BoxSymbols[BS_L_H1V1], BoxSymbols[BS_R_H1V1], BoxSymbols[BS_H1]},		// |-|    h1b1
			/* 03 */ {BoxSymbols[BS_L_H2V2], BoxSymbols[BS_R_H2V2], BoxSymbols[BS_H2]},		// ||=||  h2b2

			/* 04 */ {L' ', L' ', BoxSymbols[BS_V1]},										//  |     v1s
			/* 05 */ {BoxSymbols[BS_T_H2V1], BoxSymbols[BS_B_H2V1], BoxSymbols[BS_V1]},		// =|=    v1b2
			/* 06 */ {BoxSymbols[BS_T_H1V1], BoxSymbols[BS_B_H1V1], BoxSymbols[BS_V1]},		// -|-    v1b1
			/* 07 */ {BoxSymbols[BS_T_H2V2], BoxSymbols[BS_B_H2V2], BoxSymbols[BS_V2]},		// =||=   v2b2

			/* 08 */ {BoxSymbols[BS_H1], BoxSymbols[BS_H1], BoxSymbols[BS_H1]},				// -      h1
			/* 09 */ {BoxSymbols[BS_H2], BoxSymbols[BS_H2], BoxSymbols[BS_H2]},				// =      h2
			/* 10 */ {BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1]},				// |      v1
			/* 11 */ {BoxSymbols[BS_V2], BoxSymbols[BS_V2], BoxSymbols[BS_V2]},				// ||     v2
	};

	if (Length > 1 && DestStr) {
		Type%= ARRAYSIZE(BoxType);
		wmemset(DestStr, BoxType[Type][2], Length);
		DestStr[0] = BoxType[Type][0];
		DestStr[Length - 1] = BoxType[Type][1];
		DestStr[Length] = 0;
	}

	return DestStr;
}

FARString &HiText2Str(FARString &strDest, const wchar_t *Str)
{
	const wchar_t *ChPtr;
	strDest = Str;

	if ((ChPtr = wcschr(Str, L'&'))) {
		//   &&      = '&'
		//   &&&     = '&'
		//              ^H
		//   &&&&    = '&&'
		//   &&&&&   = '&&'
		//              ^H
		//   &&&&&&  = '&&&'

		int I = 0;
		const wchar_t *ChPtr2 = ChPtr;

		while (*ChPtr2++ == L'&')
			++I;

		if (I & 1)		// нечет?
		{
			strDest.Truncate(ChPtr - Str);

			if (ChPtr[1]) {
				wchar_t Chr[] = {ChPtr[1], 0};
				strDest+= Chr;
				FARString strText = (ChPtr + 1);
				ReplaceStrings(strText, L"&&", L"&", -1);
				strDest+= strText.CPtr() + 1;
			}
		} else {
			ReplaceStrings(strDest, L"&&", L"&", -1);
		}
	}

	return strDest;
}

int HiStrCellsCount(const wchar_t *Str)
{
	/*
			&&      = '&'
			&&&     = '&'
					   ^H
			&&&&    = '&&'
			&&&&&   = '&&'
					   ^H
			&&&&&&  = '&&&'
	*/

	int Length = 0;
	bool Hi = false;

	if (Str) {
		while (*Str) {
			if (*Str == L'&') {
				int Count = 0;

				while (*Str == L'&') {
					Str++;
					Count++;
				}

				if (Count & 1)		// нечёт?
				{
					if (Hi)
						Length+= +1;
					else
						Hi = true;
				}

				Length+= Count / 2;
			} else {
				CharClasses cc(*Str);
				if (cc.FullWidth())
					Length+= 2;
				else if (!cc.Xxxfix())
					Length+= 1;
				Str++;
			}
		}
	}

	return Length;
}

int HiFindRealPos(const wchar_t *Str, int Pos, BOOL ShowAmp)
{
	/*
			&&      = '&'
			&&&     = '&'
					   ^H
			&&&&    = '&&'
			&&&&&   = '&&'
					   ^H
			&&&&&&  = '&&&'
	*/

	if (ShowAmp) {
		return Pos;
	}

	int RealPos = 0;
	int VisPos = 0;

	if (Str) {
		while (VisPos < Pos && *Str) {
			if (*Str == L'&') {
				Str++;
				RealPos++;

				if (*Str == L'&' && *(Str + 1) == L'&' && *(Str + 2) != L'&') {
					Str++;
					RealPos++;
				}
			}

			CharClasses cc(*Str);
			if (cc.FullWidth())
				VisPos+= 2;
			else if (!cc.Xxxfix())
				VisPos+= 1;
			Str++;
			RealPos++;
		}
	}

	return RealPos;
}

int HiFindNextVisualPos(const wchar_t *Str, int Pos, int Direct)
{
	/*
			&&      = '&'
			&&&     = '&'
					   ^H
			&&&&    = '&&'
			&&&&&   = '&&'
					   ^H
			&&&&&&  = '&&&'
	*/

	if (Str) {
		if (Direct < 0) {
			if (!Pos || Pos == 1)
				return 0;

			if (Str[Pos - 1] != L'&') {
				if (Str[Pos - 2] == L'&') {
					if (Pos - 3 >= 0 && Str[Pos - 3] == L'&')
						return Pos - 1;

					return Pos - 2;
				}

				if (Pos > 1 && CharClasses(Str[Pos - 1]).FullWidth())
					return Pos - 2;

				return Pos - 1;
			} else {
				if (Pos - 3 >= 0 && Str[Pos - 3] == L'&')
					return Pos - 3;

				return Pos - 2;
			}
		} else {
			if (!Str[Pos])
				return Pos + 1;

			if (Str[Pos] == L'&') {
				if (Str[Pos + 1] == L'&' && Str[Pos + 2] == L'&')
					return Pos + 3;

				return Pos + 2;
			} else {
				return CharClasses(*Str).FullWidth() ? Pos + 2 : Pos + 1;
			}
		}
	}

	return 0;
}

bool CheckForInactivityExit()
{
	if (Opt.InactivityExit && Opt.InactivityExitTime > 0
			&& GetProcessUptimeMSec() - StartIdleTime > Opt.InactivityExitTime * 60000 && FrameManager
			&& FrameManager->GetFrameCount() == 1
			&& (!CtrlObject || !CtrlObject->Plugins.HasBackgroundTasks())) {
		FrameManager->ExitMainLoop(FALSE);
		return true;
	}

	return false;
}
