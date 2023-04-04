/*
frame.cpp

Parent class для немодальных объектов
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


#include "frame.hpp"
#include "keybar.hpp"
#include "manager.hpp"
#include "syslog.hpp"

Frame::Frame():
	FrameToBack(nullptr),
	NextModal(nullptr),
	DynamicallyBorn(true),
	CanLoseFocus(false),
	ExitCode(-1),
	KeyBarVisible(0),
	TitleBarVisible(0),
	FrameKeyBar(nullptr),
	MacroArea(MACROAREA_OTHER)
{
	_OT(SysLog(L"[%p] Frame::Frame()", this));
}

Frame::~Frame()
{
	_OT(SysLog(L"[%p] Frame::~Frame()", this));
	SetRegularIdle(false);
	DestroyAllModal();
}

void Frame::SetRegularIdle(bool enabled)
{
	if (enabled != RegularIdle) {
		RegularIdle = enabled;
		if (enabled) {
			FrameManager->RegularIdleWantersAdd();
		} else {
			FrameManager->RegularIdleWantersRemove();
		}
	}
}

void Frame::SetKeyBar(KeyBar *aFrameKeyBar)
{
	FrameKeyBar = aFrameKeyBar;
}

void Frame::UpdateKeyBar()
{
	if (FrameKeyBar && KeyBarVisible)
		FrameKeyBar->RedrawIfChanged();
}

int Frame::IsTopFrame()
{
	return FrameManager->GetCurrentFrame() == this;
}

void Frame::OnChangeFocus(int focus)
{
	if (focus)
	{
		Show();

		for (Frame *iModal=NextModal; iModal; iModal=iModal->NextModal)
		{
			if (iModal->GetType()!=MODALTYPE_COMBOBOX && iModal->IsVisible())
				iModal->Show();
		}
	}
	else
	{
		Hide();
	}
}

void Frame::PushFrame(Frame* Modalized)
{
	Frame* f = this;
	while (f->NextModal)
	{
		f = f->NextModal;
	}
	f->NextModal = Modalized;
}

void Frame::DestroyAllModal()
{
	Frame* f = this;
	while (f->NextModal)
	{
		Frame *Prev = f;
		f = f->NextModal;
		Prev->NextModal = nullptr;
	}
}

int Frame::FastHide()
{
	return TRUE;
}

void Frame::OnDestroy()
{
	DestroyAllModal();
}

bool Frame::RemoveModal(Frame *aFrame)
{
	if (!aFrame)
		return false;

	for (Frame* f=this; f->NextModal; f=f->NextModal)
	{
		if (f->NextModal == aFrame)
		{
			f->DestroyAllModal();
			return true;
		}
	}
	return false;
}

void Frame::ResizeConsole()
{
	FrameManager->ResizeAllModal(this);
}

bool Frame::HasSaveScreen()
{
	return this->SaveScr || this->ShadowSaveScr;
}
