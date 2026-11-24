#include "headers.hpp"

#include "fastfind.hpp"
#include "panel.hpp"
#include "keys.hpp"
#include "keyboard.hpp"
#include "lang.hpp"
#include "interf.hpp"
#include "farcolors.hpp"
#include "clipboard.hpp"
#include "help.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "pathmix.hpp"

static FarKey _CorrectFastFindKbdLayout(const INPUT_RECORD *rec, FarKey Key)
{
	if ((Key & (KEY_ALT | KEY_RALT)))
	{
		if ((Key & KEY_SHIFT))
		{
			switch (Key)
			{
				case KEY_ALT + KEY_SHIFT + '`':
				case KEY_ALT + KEY_SHIFT + '_':
				case KEY_ALT + KEY_SHIFT + '=':
					return Key;
			}
		}

		if (rec && rec->Event.KeyEvent.uChar.UnicodeChar
				&& WCHAR(Key & KEY_MASKF) != rec->Event.KeyEvent.uChar.UnicodeChar)
		{
			Key = (Key & 0xFFF10000) | rec->Event.KeyEvent.uChar.UnicodeChar;
		}
	}

	return Key;
}

FastFind::FastFind(Panel *Owner, FarKey FirstKey)
	:
	OwnerPanel(Owner),
	FirstKey(FirstKey),
	KeyToProcess(KEY_NONE),
	Closed(false)
{
	SetDynamicallyBorn(false);
	SetMacroArea(MACROAREA_SEARCH);
	SetRestoreScreenMode(true);

	FindEdit.SetEditBeyondEnd(false);
	FindEdit.SetObjectColor(FarColorToReal(COL_DIALOGEDIT));

	InitPositionAndSize();
}

void FastFind::InitPositionAndSize()
{
	int PX1, PY1, PX2, PY2;
	OwnerPanel->GetPosition(PX1, PY1, PX2, PY2);
	int FindX = Min(PX1 + 9, ScrX - 22);
	int FindY = Min(PY2, ScrY - 2);
	SetPosition(FindX, FindY, FindX + 21, FindY + 2);
	FindEdit.SetPosition(FindX + 2, FindY + 1, FindX + 19, FindY + 1);
}

void FastFind::ResizeConsole()
{
	Hide();
	InitPositionAndSize();
	if (FrameManager->GetBottomFrame())
		FrameManager->GetBottomFrame()->Show();
	Show();
}

void FastFind::DisplayObject()
{
	ShowBorder();
	FindEdit.Show();

	if (FirstKey)
	{
		FarKey Key = FirstKey;
		FirstKey = 0;
		ProcessKey(Key);
	}
}

void FastFind::ShowBorder()
{
	SetFarColor(COL_DIALOGTEXT);
	GotoXY(X1 + 1, Y1 + 1);
	Text(L" ");
	GotoXY(X1 + 20, Y1 + 1);
	Text(L" ");
	Box(X1, Y1, X1 + 21, Y1 + 2, FarColorToReal(COL_DIALOGBOX), DOUBLE_BOX);
	GotoXY(X1 + 7, Y1);
	SetFarColor(COL_DIALOGBOXTITLE);
	Text(Msg::SearchFileTitle);
}

void FastFind::ProcessName(const wchar_t *Src)
{
	FARString strBuffer = FindEdit.GetStringAddr();
	int OldLen = strBuffer.GetLength();
	strBuffer += Src;

	// Unquote appended text in place
	wchar_t *Buf = strBuffer.GetBuffer(strBuffer.GetLength() + 1);
	Unquote(Buf + OldLen);
	strBuffer.ReleaseBuffer();

	while (!strBuffer.IsEmpty() && !OwnerPanel->FindPartNameXLat(strBuffer, false, 1, true))
	{
		strBuffer.Truncate(strBuffer.GetLength() - 1);
	}

	if (!strBuffer.IsEmpty())
	{
		FindEdit.SetString(strBuffer);
		FindEdit.Show();
	}
}

int FastFind::ProcessKey(FarKey Key)
{
	if (Key == KEY_CTRLV || /*Key == KEY_RCTRLV ||*/ Key == KEY_SHIFTINS || Key == KEY_SHIFTNUMPAD0) //###
	{
		wchar_t *ClipText = PasteFromClipboard();
		if (ClipText)
		{
			if (*ClipText)
			{
				ProcessName(ClipText);
				ShowBorder();
			}
			free(ClipText);
		}
		return TRUE;
	}
	else if (Key == KEY_OP_XLAT)
	{
		FARString strTempName;
		FindEdit.Xlat();
		FindEdit.GetString(strTempName);
		FindEdit.SetString(L"");
		ProcessName(strTempName);
		Redraw();
		return TRUE;
	}
	else if (Key == KEY_OP_PLAINTEXT)
	{
		FARString strTempName;
		FindEdit.ProcessKey(Key);
		FindEdit.GetString(strTempName);
		FindEdit.SetString(L"");
		ProcessName(strTempName);
		Redraw();
		return TRUE;
	}
	else
	{
		Key = _CorrectFastFindKbdLayout(FrameManager->GetLastInputRecord(), Key);
	}

	if (Key == KEY_ESC || Key == KEY_F10)
	{
		Close(-1);
		return TRUE;
	}

	if (Key >= KEY_ALT_BASE + 0x01 && Key <= KEY_ALT_BASE + 65535)
		Key = Lower(static_cast<WCHAR>(Key - KEY_ALT_BASE));
	//### else if (Key >= KEY_RALT_BASE + 0x01 && Key <= KEY_RALT_BASE + 65535)
	//### 	Key = Lower(static_cast<WCHAR>(Key - KEY_RALT_BASE));

	if (Key >= KEY_ALTSHIFT_BASE + 0x01 && Key <= KEY_ALTSHIFT_BASE + 65535)
		Key = Lower(static_cast<WCHAR>(Key - KEY_ALTSHIFT_BASE));
	//### else if (Key >= KEY_RALTSHIFT_BASE + 0x01 && Key <= KEY_RALTSHIFT_BASE + 65535)
	//### 	Key = Lower(static_cast<WCHAR>(Key - KEY_RALTSHIFT_BASE));

	if (Key == KEY_MULTIPLY)
		Key = L'*';

	switch (Key)
	{
		case KEY_F1:
		{
			Hide();
			{
				Help Hlp(L"FastFind");
			}
			Show();
			break;
		}

		case KEY_CTRLNUMENTER:
		//### case KEY_RCTRLNUMENTER:
		case KEY_CTRLENTER:
		//### case KEY_RCTRLENTER:
			OwnerPanel->FindPartNameXLat(FindEdit.GetStringAddr(), true, 1, true);
			Redraw();
			break;

		case KEY_CTRLSHIFTNUMENTER:
		//### case KEY_RCTRLSHIFTNUMENTER:
		case KEY_CTRLSHIFTENTER:
		//### case KEY_RCTRLSHIFTENTER:
			OwnerPanel->FindPartNameXLat(FindEdit.GetStringAddr(), true, -1, true);
			Redraw();
			break;

		case KEY_NONE:
		case KEY_IDLE:
			break;

		default:
			if ((Key < 32 || Key >= 65536)
					&& Key != KEY_BS && Key != KEY_CTRLY /*&& Key != KEY_RCTRLY*/ //###
					&& Key != KEY_CTRLBS /*&& Key != KEY_RCTRLBS*/ //###
					&& Key != KEY_ALT && Key != KEY_SHIFT && Key != KEY_CTRL
					&& Key != KEY_RALT && Key != KEY_RCTRL
					&& !(Key == KEY_CTRLINS || Key == KEY_CTRLNUMPAD0)
					&& !(Key == KEY_SHIFTINS || Key == KEY_SHIFTNUMPAD0))
			{
				KeyToProcess = Key;
				Close(1);
				return TRUE;
			}

			FARString strLastName = FindEdit.GetStringAddr();
			if (FindEdit.ProcessKey(Key))
			{
				FARString strName;
				FindEdit.GetString(strName);

				if (strName.GetLength() > 1
						&& strName.At(strName.GetLength() - 1) == L'*'
						&& strName.At(strName.GetLength() - 2) == L'*')
				{
					strName.Truncate(strName.GetLength() - 1);
					FindEdit.SetString(strName);
				}

				if (strName.At(0) == L'"')
				{
					strName.LShift(1);
					FindEdit.SetString(strName);
				}

				if (!OwnerPanel->FindPartNameXLat(strName, false, 1, true))
				{
					FindEdit.SetString(strLastName);
				}

				Redraw();
			}
			break;
	}

	return TRUE;
}

int FastFind::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	if (MouseEvent->dwButtonState & 3)
		Close(-1);

	return TRUE;
}

int FastFind::GetTypeAndName(FARString &strType, FARString &strName)
{
	strType = Msg::SearchFileTitle;
	strName = FindEdit.GetStringAddr();
	return MODALTYPE_SEARCH;
}

void FastFind::Process()
{
	FrameManager->ExecuteModal(this);
}

void FastFind::Close(int ExitCode)
{
	if (!Closed)
	{
		Closed = true;
		SetExitCode(ExitCode);
		Hide();
		FrameManager->DeleteFrame(this);
	}
}
