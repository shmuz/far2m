#pragma once

/*
array.hpp

Шаблон работы с массивом

 TPointerArray<Object> Array;
 Object должен иметь конструктор по умолчанию.
 Класс для тупой но прозрачной работы с массивом понтеров на класс Object
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
#include <new> // for std::nothrow

template <class Object>
class TPointerArray
{
	private:
		std::vector<Object*> items;

	public:
		TPointerArray() {}
		~TPointerArray() { Free(); }

		void Free()
		{
			for (auto& el: items)
			{
				delete el;
			}
			items.clear();
		}

		Object *getItem(size_t index) { return index<items.size() ? items[index]:nullptr; }

		const Object *getConstItem(size_t index) const { return index<items.size() ? items[index]:nullptr; }

		Object *lastItem() { return items.empty() ? nullptr:items.back(); }

		Object *addItem() { return insertItem(items.size()); }

		Object *insertItem(size_t index)
		{
			if (index <= items.size())
			{
				Object *newItem = new(std::nothrow) Object;
				if (newItem)
				{
					auto I = items.cbegin() + index;
					items.insert(I, newItem);
					return newItem;
				}
			}

			return nullptr;
		}

		bool deleteItem(size_t index)
		{
			if (index < items.size())
			{
				delete items[index];
				auto I = items.cbegin() + index;
				items.erase(I);
				return true;
			}

			return false;
		}

		bool swapItems(size_t index1, size_t index2)
		{
			if (index1<items.size() && index2<items.size())
			{
				Object *temp = items[index1];
				items[index1] = items[index2];
				items[index2] = temp;
				return true;
			}

			return false;
		}

		size_t getCount() const { return items.size(); }
};
