#pragma once

#include <vector>
#include "FARString.hpp"

class Sizer {
private:
	void  *mBuf;
	void  *mCurPtr;
	size_t mAvail;
	bool   mHasSpace;

public:
	Sizer(void *aBuf, size_t aAvail)
		: mBuf(aBuf), mCurPtr(aBuf), mAvail(aAvail), mHasSpace(aAvail != 0) {}

public:
	void* AddBytes(size_t NumBytes, const void *Data = nullptr, size_t Alignment = 1);
	wchar_t* AddFARString(const FARString& Str);
	wchar_t* AddWString(const wchar_t* Str);
	size_t AddStrArray(const wchar_t* const* &Strings, const std::vector<FARString>& NamesArray);
	size_t AddStrArray(const wchar_t* const* &Strings, wchar_t** NamesArray, size_t Count);
	size_t GetSize() const { return (uintptr_t)mCurPtr - (uintptr_t)mBuf; }

	template <typename T> T* AddObject(
			size_t Count = 1, const void *Data = nullptr, size_t Alignment = alignof(T))
	{
		return static_cast<T*>(AddBytes(Count * sizeof(T), Data, Alignment));
	}
};
