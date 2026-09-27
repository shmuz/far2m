#include "headers.hpp"
#include "sizer.hpp"
#include <memory>
#include <cstring>
#include <cwchar>

void* Sizer::AddBytes(size_t NumBytes, const void* Data, size_t Alignment)
{
	size_t Space = std::numeric_limits<size_t>::max();
	std::align(Alignment, NumBytes, mCurPtr, Space);
	size_t RequiredSize = NumBytes + (std::numeric_limits<size_t>::max() - Space);
	void* Ret = nullptr;

	if (mAvail >= RequiredSize)
	{
		Ret = mCurPtr;
		mAvail -= RequiredSize;
		if (Data)
			std::memmove(mCurPtr, Data, NumBytes);
		else
			std::memset(mCurPtr, 0, NumBytes);
	}
	else
	{
		mAvail = 0; // Disable further writes if buffer space is exhausted
	}

	mCurPtr = static_cast<char*>(mCurPtr) + NumBytes;
	return Ret;
}

wchar_t* Sizer::AddFARString(const FARString& Str)
{
	return AddObject<wchar_t>(Str.GetLength() + 1, Str.CPtr());
}

wchar_t* Sizer::AddWString(const wchar_t* Str)
{
	return Str ? AddObject<wchar_t>(std::wcslen(Str) + 1, Str) : nullptr;
}

size_t Sizer::AddStrArray(const wchar_t* const*& Strings, const std::vector<FARString>& NamesArray)
{
	const size_t Count = NamesArray.size();
	Strings = nullptr;

	if (Count > 0)
	{
		const auto Items = AddObject<wchar_t*>(Count);
		Strings = Items;

		for (size_t i = 0; i < Count; ++i)
		{
			wchar_t* pStr = AddFARString(NamesArray[i]);
			if (Items)
				Items[i] = pStr;
		}
	}

	return Count;
}

size_t Sizer::AddStrArray(const wchar_t* const*& Strings, const wchar_t* const* NamesArray, size_t Count)
{
	Strings = nullptr;

	if (Count > 0 && NamesArray != nullptr)
	{
		const auto Items = AddObject<wchar_t*>(Count);
		Strings = Items;

		for (size_t i = 0; i < Count; ++i)
		{
			wchar_t* pStr = AddWString(NamesArray[i]);
			if (Items)
				Items[i] = pStr;
		}
	}

	return Count;
}
