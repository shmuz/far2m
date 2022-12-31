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
	if (Str)
		free(Str);
}

bool UserDefinedListItem::operator==(const UserDefinedListItem &rhs) const
{
	return (Str && rhs.Str) ? !(CaseSensitive ? StrCmp:StrCmpI)(Str, rhs.Str) : false;
}

int UserDefinedListItem::operator<(const UserDefinedListItem &rhs) const
{
	if (!Str)
		return 1;
	else if (!rhs.Str)
		return -1;
	else
		return (CaseSensitive ? StrCmp:StrCmpI)(Str, rhs.Str) < 0;
}

const UserDefinedListItem& UserDefinedListItem::operator=(const
        UserDefinedListItem &rhs)
{
	if (this!=&rhs)
	{
		if (Str)
		{
			free(Str);
			Str=nullptr;
		}

		if (rhs.Str)
			Str=wcsdup(rhs.Str);

		index=rhs.index;
		CaseSensitive=rhs.CaseSensitive;
	}

	return *this;
}

const UserDefinedListItem& UserDefinedListItem::operator=(const wchar_t *rhs)
{
	if (Str!=rhs)
	{
		if (Str)
		{
			free(Str);
			Str=nullptr;
		}

		if (rhs)
			Str=wcsdup(rhs);
	}

	return *this;
}

wchar_t *UserDefinedListItem::set(const wchar_t *Src, size_t Len)
{
	if (Str!=Src)
	{
		if (Str)
		{
			free(Str);
			Str=nullptr;
		}

		Str=static_cast<wchar_t*>(malloc((Len+1)*sizeof(wchar_t)));

		if (Str)
		{
			wmemcpy(Str,Src,Len);
			Str[Len]=0;
		}
	}

	return Str;
}

UserDefinedList::UserDefinedList()
{
	SetParameters(0,0,0);
}

UserDefinedList::UserDefinedList(WORD separator1, WORD separator2,
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
	return !((mUnQuote && (Separator1==L'\"' || Separator2==L'\"')) ||
	         (mProcessBrackets && (Separator1==L'[' || Separator2==L'[' ||
	                              Separator1==L']' || Separator2==L']'))
	        );
}

bool UserDefinedList::SetParameters(WORD separator1, WORD separator2,
                                    DWORD Flags)
{
	Free();
	Separator1 = separator1;
	Separator2 = separator2;
	mProcessBrackets = (Flags & ULF_PROCESSBRACKETS)?true:false;
	mAddAsterisk = (Flags & ULF_ADDASTERISK)?true:false;
	mPackAsterisks = (Flags & ULF_PACKASTERISKS)?true:false;
	mUnique = (Flags & ULF_UNIQUE)?true:false;
	mSort = (Flags & ULF_SORT)?true:false;
	mTrim = (Flags & ULF_NOTTRIM)?false:true;
	mUnQuote = (Flags & ULF_NOTUNQUOTES)?false:true;
	mAccountEmptyLine = (Flags & ULF_ACCOUNTEMPTYLINE)?true:false;
	mCaseSensitive = (Flags & ULF_CASESENSITIVE)?true:false;

	if (!Separator1 && Separator2)
	{
		Separator1 = Separator2;
		Separator2 = 0;
	}

	if (!Separator1 && !Separator2) SetDefaultSeparators();

	return CheckSeparators();
}

void UserDefinedList::Free()
{
	Array.Free();
}

bool UserDefinedList::Set(const wchar_t* const List, bool AddToList)
{
	if (AddToList)
	{
		if (List && !*List) // пусто, нечего добавлять
			return true;
	}
	else
		Free();

	bool rc=false;

	if (CheckSeparators() && List && *List)
	{
		UserDefinedListItem item(mCaseSensitive);
		item.index=Array.getSize();

		int Length, RealLength;
		bool Error=false;
		const wchar_t *CurList=List;

		while (!Error &&
						nullptr!=(CurList=Skip(CurList, Length, RealLength, Error)))
		{
			if (Length > 0)
			{
				item.set(CurList, Length);

				if (item.Str)
				{
					if (mPackAsterisks)
					{
						bool lastAsterisk=false;

						for (int i=0; item.Str[i]; ++i)
						{
							if (item.Str[i]==L'*')
							{
								if (lastAsterisk)
								{
									wmemmove(item.Str+i, item.Str+i+1, StrLength(item.Str+i));
									--i;
								}
								lastAsterisk=true;
							}
							else
								lastAsterisk=false;
						}
					}

					if (mAddAsterisk && !FindAnyOfChars(item.Str, "?*."))
					{
						Length=StrLength(item.Str);
						/* $ 18.09.2002 DJ
							 выделялось на 1 байт меньше, чем надо
						*/
						item.Str=static_cast<wchar_t*>(realloc(item.Str, (Length+2)*sizeof(wchar_t)));

						/* DJ $ */
						if (item.Str)
						{
							item.Str[Length]=L'*';
							item.Str[Length+1]=0;
						}
						else
							Error=true;
					}

					if (!Error && !Array.addItem(item))
						Error=true;
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
		if (mUnique)
		{
			Array.Sort();
			Array.Pack();
		}

		if (!mSort)
			Array.Sort(reinterpret_cast<TARRAYCMPFUNC>(CmpItems));
		else if (!mUnique) // чтобы не сортировать уже отсортированное
			Array.Sort();

		size_t i=0, maxI=Array.getSize();

		for (; i<maxI; ++i)
			Array.getItem(i)->index=i;
	}
	else
		Free();

	return rc;
}

int __cdecl UserDefinedList::CmpItems(const UserDefinedListItem **el1,
                                      const UserDefinedListItem **el2)
{
	if (el1==el2)
		return 0;
	else if ((**el1).index==(**el2).index)
		return 0;
	else if ((**el1).index<(**el2).index)
		return -1;
	else
		return 1;
}

const wchar_t *UserDefinedList::Skip(const wchar_t *Str, int &Length, int &RealLength, bool &Error)
{
	Length=RealLength=0;
	Error=false;
	auto Str0=Str;

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

	if ( mUnQuote )
	{
		// мы в кавычках - захватим все отсюда и до следующих кавычек
		++cur;
		const wchar_t *QuoteEnd=wcschr(cur, L'\"');

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

	return Str;
}

bool UserDefinedList::IsEmpty() const
{
	return Array.getSize() == 0;
}

const wchar_t *UserDefinedList::Get(size_t Index) const
{
	const UserDefinedListItem *item=Array.getConstItem(Index);
	return item ? item->Str : nullptr;
}
