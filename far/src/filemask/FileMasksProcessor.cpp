/*
FileMasksProcessor.cpp

Класс для работы с простыми масками файлов (не учитывается наличие масок
исключения).
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

#include "FileMasksProcessor.hpp"
#include "processname.hpp"
#include "StackHeapArray.hpp"
#include "udlist.hpp"
#include "KeyFileHelper.h"

#define EXCLUDEMASKSEPARATOR (L'|')

// Protect against cases like a=<a> or a=<b>, b=<a>, etc.
// It also restricts valid nesting depth but the limit is high enough to cover all practical cases.
// This method was chosen due to its simplicity.
#define MAXCALLDEPTH 64

static const wchar_t *FindExcludeChar(const wchar_t *masks)
{
	for (bool regexp=false; *masks; masks++)
	{
		if (!regexp)
		{
			if (*masks == EXCLUDEMASKSEPARATOR)
				return masks;
			if (*masks == L'/')
				regexp = true;
		}
		else
		{
			if (*masks == L'\\')
			{
				if (*(++masks) == 0) // skip the next char
					break;
			}
			else if (*masks == L'/')
				regexp = false;
		}
	}
	return nullptr;
}

static bool IsExcludeMask(const wchar_t *masks)
{
	return FindExcludeChar(masks)!=nullptr;
}

bool SingleFileMask::Set(const wchar_t *Masks, DWORD Flags)
{
	Mask=Masks;
	return !Mask.IsEmpty();
}

bool SingleFileMask::Compare(const wchar_t *Name) const
{
	return CmpName(Mask.CPtr(), Name, false);
}

bool SingleFileMask::IsEmpty() const
{
	return Mask.IsEmpty();
}

void SingleFileMask::Reset()
{
	Mask.Clear();
}

RegexMask::RegexMask(): BaseFileMask(), re(nullptr), n(0)
{
}

RegexMask::~RegexMask()
{
	re.reset();
}

bool RegexMask::IsEmpty() const
{
	return re ? !n : true;
}

void RegexMask::Reset()
{
	re.reset();
	n = 0;
}

bool RegexMask::Set(const wchar_t *masks, DWORD Flags)
{
	Reset();

	if (*masks == L'/')
	{
		re.reset(new(std::nothrow) RegExp);
		if (re && re->Compile(masks, OP_PERLSTYLE|OP_OPTIMIZE))
		{
			n = re->GetBracketsCount();
			return true;
		}
	}

	Reset();
	return false;
}

/* сравнить имя файла со списком масок
   Возвращает TRUE в случае успеха.
   Путь к файлу в FileName НЕ игнорируется */
bool RegexMask::Compare(const wchar_t *FileName) const
{
	if (re)
	{
		StackHeapArray<RegExpMatch> m(n);
		int i = n;
		return re->Search(ReStringView(FileName), m.Get(), i);
	}

	return false;
}

FileMasksProcessor::FileMasksProcessor() : CallDepth(0)
{
	IniReader = new KeyFileReadSection(InMyConfig("settings/masks.ini"), "Masks");
}

FileMasksProcessor::FileMasksProcessor(int aCallDepth, KeyFileReadSection *aIniReader)
	: CallDepth(aCallDepth), IniReader(aIniReader)
{
}

FileMasksProcessor::~FileMasksProcessor()
{
	Reset();
	if (CallDepth==0)
		delete IniReader;
}

void FileMasksProcessor::Reset()
{
	for (auto I: IncludeMasks) { I->Reset(); delete I; }
	for (auto I: ExcludeMasks) { I->Reset(); delete I; }

	IncludeMasks.clear();
	ExcludeMasks.clear();
}

bool FileMasksProcessor::IsEmpty() const
{
	return IncludeMasks.empty();
}

FARString FileMasksProcessor::GetNamedMask(const wchar_t *Name)
{
	return IniReader->SectionLoaded() ? IniReader->GetString(Wide2MB(Name)) : "";
}

/*
 Инициализирует список масок. Принимает список, разделенных запятой.
 Возвращает FALSE при неудаче (например, одна из
 длина одной из масок равна 0)
*/
bool FileMasksProcessor::Set(const wchar_t *masks, DWORD Flags)
{
	Reset();

	if (CallDepth >= MAXCALLDEPTH) return false;

	if (!*masks) return false;

	bool rc=false;
	wchar_t *MasksStr=wcsdup(masks);

	if (MasksStr)
	{
		rc=true;
		wchar_t *pExclude = (wchar_t *) FindExcludeChar(MasksStr);

		if (pExclude)
		{
			*pExclude++ = 0;

			if (*pExclude!=L'/' && wcschr(pExclude, EXCLUDEMASKSEPARATOR))
				rc=false;
		}

		if (rc)
		{
			rc = SetPart(*MasksStr ? MasksStr:L"*", Flags&FMPF_ADDASTERISK, IncludeMasks);

			if (rc && pExclude)
				rc = SetPart(pExclude, 0, ExcludeMasks);
		}

		free(MasksStr);
	}

	if (!rc)
		Reset();

	return rc;
}

/*
 Инициализирует список масок. Принимает список, разделенных запятой.
 Возвращает FALSE при неудаче (например, одна из
 длина одной из масок равна 0)
*/
bool FileMasksProcessor::SetPart(const wchar_t *masks, DWORD Flags, std::vector<BaseFileMask*> &Target)
{
	// разделителем масок является не только запятая, но и точка с запятой!
	DWORD flags=ULF_PACKASTERISKS|ULF_PROCESSBRACKETS|ULF_SORT|ULF_UNIQUE;

	if (Flags&FMPF_ADDASTERISK)
		flags|=ULF_ADDASTERISK;

	UserDefinedList UdList(flags);

	if (UdList.Set(masks))
	{
		FARString strMask;
		const wchar_t *onemask;

		for (int I=0; (onemask=UdList.Get(I)); I++)
		{
			BaseFileMask *baseMask = nullptr;

			auto pStart=onemask;

			if (*pStart == L'<')
			{
				auto pEnd = wcschr(++pStart, L'>');
				if (pEnd && pEnd!=pStart && pEnd[1]==0)
				{
					FARString strKey(pStart, pEnd-pStart);
					strMask = GetNamedMask(strKey);

					if (!strMask.IsEmpty())
					{
						baseMask = new(std::nothrow) FileMasksProcessor(CallDepth+1,IniReader);
						onemask = strMask.CPtr();
					}
				}
			}
			else if (*onemask == L'/')
			{
				baseMask = new(std::nothrow) RegexMask;
			}
			else
			{
				baseMask = new(std::nothrow) SingleFileMask;
			}

			if (baseMask && baseMask->Set(onemask,0))
			{
				Target.push_back(baseMask);
			}
			else
			{
				Reset();
				delete baseMask;
				return false;
			}
		}
		return true;
	}

	return false;
}

/* сравнить имя файла со списком масок
   Возвращает TRUE в случае успеха.
   Путь к файлу в FileName НЕ игнорируется */
bool FileMasksProcessor::Compare(const wchar_t *FileName) const
{
	bool OK=false;
	for (auto I: IncludeMasks)
	{
		if (I->Compare(FileName))	{ OK=true; break; }
	}
	if (OK)
	{
		for (auto I: ExcludeMasks)
		{
			if (I->Compare(FileName))	{ OK=false; break; }
		}
	}
	return OK;
}
