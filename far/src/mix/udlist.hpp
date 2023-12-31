#pragma once

/*
udlist.hpp

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


#include <vector>
#include "noncopyable.hpp"

enum UDL_FLAGS
{
	ULF_ADDASTERISK      =0x00000001, // добавлять '*' к концу элемента списка, если он не содержит '?', '*' и '.'
	ULF_PACKASTERISKS    =0x00000002, // вместо "***" в список помещать просто "*"
	ULF_PROCESSBRACKETS  =0x00000004, // учитывать квадратные скобки при анализе строки инициализации
	ULF_UNIQUE           =0x00000010, // убирать дублирующиеся элементы
	ULF_SORT             =0x00000020, // отсортировать (с учетом регистра)
	ULF_NOTTRIM          =0x00000040, // не удалять пробелы
	ULF_ACCOUNTEMPTYLINE =0x00000100, // учитывать пустые "строки"
	ULF_CASESENSITIVE    =0x00000200, // регистрозависимый
	ULF_PROCESSREGEXP    =0x00000400, // учитывать регулярные выражения, например /a+,b+;c+/i
};


class UserDefinedListItem
{
	public:
		size_t index;
		FARString Str;
		bool CaseSensitive;
		UserDefinedListItem(bool cs=false):index(0), CaseSensitive(cs) {}
		bool operator==(const UserDefinedListItem &rhs) const;
		bool operator<(const UserDefinedListItem &rhs) const;
		const UserDefinedListItem& operator=(const UserDefinedListItem &rhs);
		const UserDefinedListItem& operator=(const wchar_t *rhs);
		void Compact(wchar_t Char, bool ByPairs);
		~UserDefinedListItem() {}
};

class UserDefinedList : private NonCopyable
{
	private:
		std::vector<UserDefinedListItem> Array;
		wchar_t Separator1, Separator2;
		bool mProcessBrackets, mAddAsterisk, mPackAsterisks, mUnique, mSort, mTrim;
		bool mAccountEmptyLine, mCaseSensitive, mProcessRegexp;

	private:
		bool CheckSeparators() const; // проверка разделителей на корректность
		void SetDefaultSeparators();
		const wchar_t *Skip(const wchar_t *Str, int &Length, int &RealLength, bool &Error, bool &InQuotes);

	public:
		// по умолчанию разделителем считается ';' и ',', а
		// mProcessBrackets=mAddAsterisk=mPackAsterisks=false
		// mUnique=mSort=false

		// Явно указываются разделители. См. описание SetParameters
		UserDefinedList(DWORD Flags=0, wchar_t separator1=L';', wchar_t separator2=L',');
		~UserDefinedList() {}

	public:
		// Сменить символы-разделитель и разрешить или запретить обработку
		// квадратных скобок.
		// Если один из Separator* равен 0x00, то он игнорируется при компиляции
		// (т.е. в Set)
		// Если оба разделителя равны 0x00, то восстанавливаются разделители по
		// умолчанию (';' & ',').
		// Если mAddAsterisk равно true, то к концу элемента списка будет
		// добавляться '*', если этот элемент не содержит '?', '*' и '.'
		// Возвращает false, если один из разделителей является кавычкой или
		// включена обработка скобок и один из разделителей является квадратной
		// скобкой.
		bool SetParameters(DWORD Flags=0, wchar_t Separator1=L';', wchar_t Separator2=L',');

		// Инициализирует список. Принимает список, разделенный разделителями.
		// Возвращает false при неудаче.
		bool Set(const wchar_t* List, bool AddToList=false);

		// Ничего не парсим, устанавливаем 1 элемент "как есть".
		// Возвращает false только при нехватке памяти.
		bool SetAsIs(const wchar_t* List);

		// Добавление к уже существующему списку
		bool AddItem(const wchar_t *NewItem)
		{
			return Set(NewItem,true);
		}

		// Выдает указатель на очередной элемент списка или nullptr
		const wchar_t *Get(size_t Index) const;

		// true, если элементов в списке нет
		bool IsEmpty() const { return Array.empty(); }

		bool IsLastElement(size_t Index) const { return Index + 1 == Array.size(); }

		// Вернуть количество элементов в списке
		size_t Size() const { return Array.size(); }
};
