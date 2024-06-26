/*
TPreRedrawFunc.cpp

Фоновый апдейт

*/
/*
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


#include "TPreRedrawFunc.hpp"

TPreRedrawFunc PreRedraw;

PreRedrawItem TPreRedrawFunc::errorStack{};

PreRedrawItem TPreRedrawFunc::Pop()
{
	if (!Items.empty())
	{
		PreRedrawItem Destination = Items.back();
		Items.pop_back();
		return Destination;
	}
	return errorStack;
}

PreRedrawItem TPreRedrawFunc::Peek() const
{
	if (!Items.empty())
		return Items.back();

	return errorStack;
}

PreRedrawItem TPreRedrawFunc::SetParam(const PreRedrawParamStruct &Param)
{
	if (!Items.empty())
	{
		Items.back().Param = Param;
		return Items.back();
	}
	return errorStack;
}

PreRedrawItem TPreRedrawFunc::Push(const PreRedrawItem &Source)
{
	Items.push_back(Source);
	return Items.back();
}

PreRedrawItem TPreRedrawFunc::Push(PREREDRAWFUNC Func, PreRedrawParamStruct *Param)
{
	PreRedrawItem Source{};
	Source.PreRedrawFunc = Func;

	if (Param)
		Source.Param = *Param;

	return Push(Source);
}

TPreRedrawFuncGuard::TPreRedrawFuncGuard(PREREDRAWFUNC Func)
{
	PreRedraw.Push(Func);
}

TPreRedrawFuncGuard::~TPreRedrawFuncGuard()
{
	PreRedraw.Pop();
}
