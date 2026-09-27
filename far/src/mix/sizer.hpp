#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>
#include <limits>
#include "FARString.hpp"

class Sizer {
private:
	void*  mBuf;
	void*  mCurPtr;
	size_t mAvail;

	// A dummy base address for Pass 1 (sizing pass).
	// Using a non-null, strongly-aligned address (e.g., 64-byte aligned) avoids
	// nullptr arithmetic UB and ensures identical alignment padding across passes.
	static constexpr uintptr_t DUMMY_BASE = 0x10000;

public:
	Sizer(void* Buf, size_t BufSize)
		: mBuf(Buf ? Buf : reinterpret_cast<void*>(DUMMY_BASE)),
		  mCurPtr(mBuf),
		  mAvail(Buf ? BufSize : 0)
	{}

	void* AddBytes(size_t NumBytes, const void* Data = nullptr, size_t Alignment = 1);
	wchar_t* AddFARString(const FARString& Str);
	wchar_t* AddWString(const wchar_t* Str);
	size_t AddStrArray(const wchar_t* const*& Strings, const std::vector<FARString>& NamesArray);
	size_t AddStrArray(const wchar_t* const*& Strings, const wchar_t* const* NamesArray, size_t Count);

	size_t GetSize() const { return static_cast<const char*>(mCurPtr) - static_cast<const char*>(mBuf); }

	template <typename T>
	T* AddObject(size_t Count = 1, const void* Data = nullptr, size_t Alignment = alignof(T))
	{
		if (Count > 0 && (std::numeric_limits<size_t>::max() / sizeof(T) < Count))
			return nullptr;

		return static_cast<T*>(AddBytes(Count * sizeof(T), Data, Alignment));
	}
};
