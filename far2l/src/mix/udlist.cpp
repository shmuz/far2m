/*
udlist.cpp

Список чего-либо, перечисленного через символ-разделитель. Если нужно, чтобы
элемент списка содержал разделитель, то этот элемент следует заключить в
кавычки. Если кроме разделителя ничего больше в строке нет, то считается, что
это не разделитель, а простой символ.
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

#include "udlist.hpp"

UserDefinedListItem::~UserDefinedListItem()
{
}

bool UserDefinedListItem::operator==(const UserDefinedListItem &rhs) const
{
	return 0 == (CaseSensitive ? StrCmp:StrCmpI)(Str.CPtr(), rhs.Str.CPtr());
}

bool UserDefinedListItem::operator<(const UserDefinedListItem &rhs) const
{
	return (CaseSensitive ? StrCmp:StrCmpI)(Str.CPtr(), rhs.Str.CPtr()) < 0;
}

const UserDefinedListItem& UserDefinedListItem::operator=(const UserDefinedListItem &rhs)
{
	if (this!=&rhs)
	{
		Str=rhs.Str;
		index=rhs.index;
		CaseSensitive=rhs.CaseSensitive;
	}

	return *this;
}

const UserDefinedListItem& UserDefinedListItem::operator=(const wchar_t *rhs)
{
	Str=rhs;
	return *this;
}

void UserDefinedListItem::Compact(wchar_t Char, bool ByPairs)
{
	bool lastFound=false;
	wchar_t *Txt=wcsdup(Str.CPtr());

	for (int i=0; Txt[i]; ++i)
	{
		if (Txt[i]==Char)
		{
			if (lastFound)
			{
				if (ByPairs) lastFound=false;
				wmemmove(Txt+i, Txt+i+1, StrLength(Txt+i));
				--i;
			}
			else
				lastFound=true;
		}
		else
			lastFound=false;
	}
	Str=Txt;
	free(Txt);
}

static bool __cdecl CmpIndexes(const UserDefinedListItem &el1, const UserDefinedListItem &el2)
{
	return el1.index < el2.index;
}

UserDefinedList::UserDefinedList()
{
	SetParameters(0,0,0);
}

UserDefinedList::UserDefinedList(wchar_t separator1, wchar_t separator2,
                                 DWORD Flags)
{
	SetParameters(separator1, separator2, Flags);
}

void UserDefinedList::SetDefaultSeparators()
{
	Separator1=L';';
	Separator2=L',';
}

bool UserDefinedList::CheckSeparators() const
{
	return !((Separator1==L'\"' || Separator2==L'\"') ||
	         (mProcessBrackets && (Separator1==L'[' || Separator2==L'[' ||
	                              Separator1==L']' || Separator2==L']'))
	        );
}

bool UserDefinedList::SetParameters(wchar_t separator1, wchar_t separator2, DWORD Flags)
{
	Array.clear();
	Separator1 = separator1;
	Separator2 = separator2;
	mProcessBrackets = (Flags & ULF_PROCESSBRACKETS) != 0;
	mAddAsterisk = (Flags & ULF_ADDASTERISK) != 0;
	mPackAsterisks = (Flags & ULF_PACKASTERISKS) != 0;
	mUnique = (Flags & ULF_UNIQUE) != 0;
	mSort = (Flags & ULF_SORT) != 0;
	mTrim = (Flags & ULF_NOTTRIM) == 0;
	mAccountEmptyLine = (Flags & ULF_ACCOUNTEMPTYLINE) != 0;
	mCaseSensitive = (Flags & ULF_CASESENSITIVE) != 0;

	if (!Separator1 && Separator2)
	{
		Separator1 = Separator2;
		Separator2 = 0;
	}

	if (!Separator1 && !Separator2) SetDefaultSeparators();

	return CheckSeparators();
}

bool UserDefinedList::SetAsIs(const wchar_t* const List)
{
	if (List && *List)
	{
		Array.clear();
		Array.emplace_back(mCaseSensitive);
		Array.back() = List;
		return true;
	}
	return false;
}

bool UserDefinedList::Set(const wchar_t* const List, bool AddToList)
{
	if (AddToList)
	{
		if (List && !*List) // пусто, нечего добавлять
			return true;
	}
	else
		Array.clear();

	bool rc=false;

	if (CheckSeparators() && List && *List)
	{
		UserDefinedListItem item(mCaseSensitive);
		item.index=Array.size();

		int Length, RealLength;
		bool Error=false;
		const wchar_t *CurList=List;

		while (!Error && nullptr!=(CurList=Skip(CurList, Length, RealLength, Error)))
		{
			if (Length > 0)
			{
				item.Str=FARString(CurList,Length);

				if (item.Str)
				{
					if (mPackAsterisks)
						item.Compact(L'*', false);

					item.Compact(L'\"', true);

					if (mAddAsterisk && !FindAnyOfChars(item.Str.CPtr(), "?*."))
					{
						Length=StrLength(item.Str);
						item.Str += L"*";
					}

					if (!Error)
						Array.push_back(item);
				}
				else
					Error=true;

				CurList+=RealLength;
			}
			else
			{
				if (!mAccountEmptyLine)
					Error=true;
			}

			++item.index;
		}

		rc=!Error;
	}

	if (rc)
	{
		if (mUnique) // Array.Pack();
		{
			std::sort(Array.begin(), Array.end());
			for (auto it=Array.cbegin(); it != Array.cend(); )
			{
				auto curr = it;
				if (++it != Array.cend())
				{
					if (*it == *curr)
						it = Array.erase(curr);
				}
			}
		}

		if (!mSort)
			std::sort(Array.begin(), Array.end(), CmpIndexes);
		else if (!mUnique) // чтобы не сортировать уже отсортированное
			std::sort(Array.begin(), Array.end());

		size_t i=0;
		for (auto& el: Array)
		{
			el.index=i++;
		}
	}
	else
		Array.clear();

	return rc;
}

const wchar_t *UserDefinedList::Skip(const wchar_t *Str, int &Length, int &RealLength, bool &Error)
{
	Length=RealLength=0;
	Error=false;
	const wchar_t* Str0=Str;

	if ( mTrim )
		while (IsSpace(*Str)) ++Str;

	if (*Str==Separator1 || *Str==Separator2)
	{
		++Str;
		if ( mTrim )
			while (IsSpace(*Str)) ++Str;
	}

	if (!*Str)
		return Str==Str0 ? nullptr:Str;

	const wchar_t *cur=Str;
	bool InBrackets=false, InQuotes = (*cur==L'\"');

	if (!InQuotes) // если мы в кавычках, то обработка будет позже и чуть сложнее
		while (*cur) // важно! проверка *cur должна стоять первой
		{
			if (mProcessBrackets)
			{
				if (*cur==L']')
					InBrackets=false;

				if (*cur==L'[' && nullptr!=wcschr(cur+1, L']'))
					InBrackets=true;
			}

			if (!InBrackets && (*cur==Separator1 || *cur==Separator2))
				break;

			++cur;
		}

	if (!InQuotes || !*cur)
	{
		RealLength=Length=(int)(cur-Str);
		--cur;

		if ( mTrim )
			while (IsSpace(*cur))
			{
				--Length;
				--cur;
			}

		return Str;
	}

	// мы в кавычках - захватим все отсюда и до следующих кавычек
	++cur;

	const wchar_t *QuoteEnd = nullptr;
	for (auto ptr=cur; *ptr; ++ptr)
	{
		if (*ptr == L'\"')
		{
			if (*(ptr+1) == L'\"') // 2 кавычки подряд будут потом заменены на одну
				++ptr;
			else
			{
				QuoteEnd=ptr;
				break;
			}
		}
	}

	if (!QuoteEnd)
	{
		Error=true;
		return nullptr;
	}

	const wchar_t *End=QuoteEnd+1;

	if ( mTrim )
		while (IsSpace(*End)) ++End;

	if (!*End || *End==Separator1 || *End==Separator2)
	{
		Length=(int)(QuoteEnd-cur);
		RealLength=(int)(End-cur);
		return cur;
	}

	Error=true;
	return nullptr;
}

const wchar_t *UserDefinedList::Get(size_t Index) const
{
	if (Index < Array.size())
	{
		return Array[Index].Str.CPtr();
	}
	return nullptr;
}
