/*
namelist.cpp

Список имен файлов, используется в viewer при нажатии Gray+/Gray-
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


#include "namelist.hpp"
#include "pathmix.hpp"

NamesList::NamesList()
{
	Init();
}

NamesList::~NamesList()
{
}

void NamesList::AddName(const wchar_t *Name)
{
	Names.push_back(OneName());
	auto pName = Names.back();
	pName.Value.strName = Name?Name:L"";
	CurrentName = --Names.end();
}


bool NamesList::GetNextName(FARString &strName)
{
	auto pName = CurrentName;
	if (++pName == Names.end())
		return false;

	CurrentName = pName;
	strName = CurrentName->Value.strName;
	return true;
}


bool NamesList::GetPrevName(FARString &strName)
{
	if (CurrentName == Names.begin())
		return false;

	--CurrentName;
	strName = CurrentName->Value.strName;
	return true;
}


void NamesList::SetCurName(const wchar_t *Name)
{
	for (auto pCurName=Names.cbegin(); pCurName!=Names.cend(); pCurName++)
	{
		if (!StrCmp(Name, pCurName->Value.strName))
		{
			CurrentName=pCurName;
			return;
		}
	}
}


void NamesList::MoveData(NamesList &Dest)
{
	Dest.Names.swap(Names);
	Dest.strCurrentDir = strCurrentDir;
	Dest.CurrentName = CurrentName;
	Init();
}


void NamesList::GetCurDir(FARString &strDir)
{
	strDir = strCurrentDir;
}


void NamesList::SetCurDir(const wchar_t *Dir)
{
	if (StrCmp(strCurrentDir,Dir) || !TestCurrentDirectory(Dir))
	{
		strCurrentDir = Dir;
	}
}

void NamesList::Init()
{
	Names.clear();
	strCurrentDir.Clear();
	CurrentName=Names.cend();
}
