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

FileMasksProcessor::FileMasksProcessor() {}

FileMasksProcessor::~FileMasksProcessor()
{
	Reset();
}

void FileMasksProcessor::Reset()
{
	for (auto I: Masks)
		I->Reset();

	Masks.clear();
}

bool FileMasksProcessor::IsEmpty() const
{
	return Masks.empty();
}

/*
 Инициализирует список масок. Принимает список, разделенных запятой.
 Возвращает FALSE при неудаче (например, одна из
 длина одной из масок равна 0)
*/
bool FileMasksProcessor::Set(const wchar_t *masks, DWORD Flags)
{
	Reset();

	// разделителем масок является не только запятая, но и точка с запятой!
	DWORD flags=ULF_PACKASTERISKS|ULF_PROCESSBRACKETS|ULF_SORT|ULF_UNIQUE;

	if (Flags&FMPF_ADDASTERISK)
		flags|=ULF_ADDASTERISK;

	UserDefinedList UdList(flags);

	if (UdList.Set(masks))
	{
		const wchar_t *onemask;
		for (int I=0; (onemask=UdList.Get(I)); I++)
		{
			BaseFileMask *base;
			if (*onemask == L'/')
				base = new RegexMask();
			else
				base = new SingleFileMask();

			if (base->Set(onemask,0))
			{
				Masks.push_back(base);
			}
			else
			{
				Reset();
				delete base;
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
	for (auto I: Masks)
	{
		if (I->Compare(FileName))
			return true;
	}
	return false;
}
