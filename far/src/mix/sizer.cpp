#include "headers.hpp"

#include "sizer.hpp"

void* Sizer::AddBytes(const void *Data, size_t NumBytes, size_t Alignment)
{
	size_t Space = BIG;
	std::align(Alignment, NumBytes, mCurPtr, Space);
	size_t RequiredSize = NumBytes + (BIG - Space);

	if (mHasSpace)
	{
		if (mAvail >= RequiredSize)
		{
			mAvail -= RequiredSize;
			if (Data)
				memmove(mCurPtr, Data, NumBytes);
			else
				memset(mCurPtr, 0, NumBytes);
		}
		else
			mHasSpace = false;
	}

	void *Ret = mCurPtr;
	mCurPtr = (char*)mCurPtr + NumBytes;
	return Ret;
}

wchar_t* Sizer::AddFARString(const FARString& Str)
{
	const auto numBytes = sizeof(wchar_t) * (Str.GetLength() + 1);
	return (wchar_t*)AddBytes(Str.CPtr(), numBytes, alignof(wchar_t));
}

wchar_t* Sizer::AddCString(const wchar_t* Str)
{
	if (Str == nullptr)
		return nullptr;
	const auto numBytes = sizeof(wchar_t) * (wcslen(Str) + 1);
	return (wchar_t*)AddBytes(Str, numBytes, alignof(wchar_t));
}

size_t Sizer::AddStrArray(const wchar_t* const* &Strings, const std::vector<FARString>& NamesArray)
{
	size_t Count = NamesArray.size();
	Strings = nullptr;

	if (Count)
	{
		size_t numBytes = Count * sizeof(wchar_t*);
		const auto Items = (wchar_t**)AddBytes(nullptr, numBytes, alignof(wchar_t*));
		Strings = mHasSpace ? Items : nullptr;

		for (size_t i = 0; i < Count; ++i)
		{
			wchar_t* pStr = AddFARString(NamesArray[i]);
			if (mHasSpace)
				Items[i] = pStr;
		}
	}

	return Count;
}

size_t Sizer::AddStrArray(const wchar_t* const* &Strings, wchar_t** NamesArray, size_t Count)
{
	Strings = nullptr;

	if (Count)
	{
		size_t numBytes = Count * sizeof(wchar_t*);
		const auto Items = (wchar_t**)AddBytes(nullptr, numBytes, alignof(wchar_t*));
		Strings = mHasSpace ? Items : nullptr;

		for (size_t i = 0; i < Count; ++i)
		{
			wchar_t* pStr = AddCString(NamesArray[i]);
			if (mHasSpace)
				Items[i] = pStr;
		}
	}

	return Count;
}
