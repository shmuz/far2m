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
	auto buf=new wchar_t[Str.GetLength()+1], trg=buf;
	bool found=false;

	for (auto src=Str.CPtr(); *src; src++)
	{
		if (!found)
		{
			found=(*src==Char);
		}
		else
		{
			if (*src==Char)
			{
				found=!ByPairs;
				continue;
			}
			found=false;
		}
		*trg++=*src;
	}

	*trg=0;
	Str=buf;
	delete[] buf;
}

static bool __cdecl CmpIndexes(const UserDefinedListItem &el1, const UserDefinedListItem &el2)
{
	return el1.index < el2.index;
}

UserDefinedList::UserDefinedList(DWORD Flags, wchar_t separator1, wchar_t separator2)
{
	SetParameters(Flags, separator1, separator2);
}

void UserDefinedList::SetDefaultSeparators()
{
	Separator1=L';';
	Separator2=L',';
}

bool UserDefinedList::CheckSeparators() const
{
	return !(Separator1==L'\"' || Separator2==L'\"' ||
		(mProcessBrackets && (Separator1==L'[' || Separator2==L'[' || Separator1==L']' || Separator2==L']')) ||
		(mProcessRegexp   && (Separator1==L'/' || Separator2==L'/')));
}

bool UserDefinedList::SetParameters(DWORD Flags, wchar_t separator1, wchar_t separator2)
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
	mProcessRegexp = (Flags & ULF_PROCESSREGEXP) != 0;

	if (!Separator1 && Separator2)
	{
		Separator1 = Separator2;
		Separator2 = 0;
	}

	if (!Separator1 && !Separator2) SetDefaultSeparators();

	return CheckSeparators();
}

bool UserDefinedList::SetAsIs(const wchar_t* List)
{
	if (*List)
	{
		Array.clear();
		Array.emplace_back(mCaseSensitive);
		Array.back() = List;
		return true;
	}
	return false;
}

bool UserDefinedList::Set(const wchar_t* List, bool AddToList)
{
	if (!CheckSeparators())
		return false;

	if (!*List)
		return AddToList; // пусто, нечего добавлять

	if (!AddToList)
		Array.clear();

	bool rc=false;

	{
		UserDefinedListItem item(mCaseSensitive);
		item.index=Array.size();

		int Length, RealLength;
		bool Error=false;
		const wchar_t *CurList=List;
		bool InQuotes=false;

		while (!Error && (CurList=Skip(CurList, Length, RealLength, Error, InQuotes)))
		{
			if (Length > 0)
			{
				item.Str=FARString(CurList,Length);

				if (item.Str)
				{
					if (!(mProcessRegexp && *CurList==L'/'))
					{
						if (mPackAsterisks)
							item.Compact(L'*', false);

						if (InQuotes)
							item.Compact(L'\"', true);

						if (mAddAsterisk && !FindAnyOfChars(item.Str.CPtr(), "?*."))
						{
							Length=StrLength(item.Str);
							item.Str += L"*";
						}
					}

					Array.push_back(item);
					CurList+=RealLength;
				}
				else
					Error=true;
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
		if (mUnique)
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

const wchar_t *UserDefinedList::Skip(const wchar_t *Str, int &Length, int &RealLength, bool &Error,
	bool &InQuotes)
{
	InQuotes = false;
	Length=RealLength=0;
	Error=false;

	if (!*Str)
		return nullptr;

	if ( mTrim )
		while (IsSpace(*Str)) ++Str;

	if (*Str==Separator1 || *Str==Separator2)
	{
		++Str;
		if ( mTrim )
			while (IsSpace(*Str)) ++Str;
	}

	if (!*Str)
		return Str;

	const wchar_t *cur=Str;
	InQuotes = (*cur==L'\"');
	bool IsRegexp = mProcessRegexp && (*cur==L'/');

	if (InQuotes)
	{
		const wchar_t *End = nullptr;
		for (auto ptr=++cur; *ptr; ++ptr)
		{
			if (*ptr == L'\"')
			{
				if (*(ptr+1) == L'\"') // 2 кавычки подряд будут потом заменены на одну
					++ptr;
				else
				{
					End=ptr;
					break;
				}
			}
		}

		if (End)
		{
			auto RealEnd=End+1;

			if ( mTrim )
				while (IsSpace(*RealEnd)) ++RealEnd;

			if (!*RealEnd || *RealEnd==Separator1 || *RealEnd==Separator2)
			{
				Length=(int)(End-cur);
				RealLength=(int)(RealEnd-cur);
				return cur;
			}
		}
	}

	else if (IsRegexp)
	{
		const wchar_t *End = nullptr;
		for (auto ptr=cur+1; *ptr; ++ptr)
		{
			if (*ptr == L'\\') {
				if (!*++ptr)
					goto ErrorLabel;
			}
			else if (*ptr == L'/') {
				End=ptr;
				break;
			}
		}
		if (End)
		{
			++End;
			for (int i=0; i<4; i++,End++) { // allow up to 4 flags, e.g. "ismx"
				if (!*End || *End==Separator1 || *End==Separator2 || !IsAlpha(*End))
					break;
			}

			auto RealEnd=End;
			if ( mTrim )
				while (IsSpace(*RealEnd)) ++RealEnd;

			if (!*RealEnd || *RealEnd==Separator1 || *RealEnd==Separator2)
			{
				Length=(int)(End-cur);
				RealLength=(int)(RealEnd-cur);
				return cur;
			}
		}
	}

	else
	{
		bool InBrackets=false;
		for (; *cur; ++cur) // важно! проверка *cur должна стоять первой
		{
			if (mProcessBrackets)
			{
				if (*cur==L'[')
					InBrackets=true;
				else if (*cur==L']')
					InBrackets=false;
			}
			if (!InBrackets && (*cur==Separator1 || *cur==Separator2))
				break;
		}

		RealLength=Length=(int)(cur-Str);

		if ( mTrim )
		{
			while (Length>0 && IsSpace(*--cur))
				--Length;
		}
		return Str;
	}

ErrorLabel:
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
