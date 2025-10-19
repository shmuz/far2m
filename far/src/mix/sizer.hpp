#pragma once

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
	static const size_t BIG = 0xFFFF'0000u; // an arbitrary big value

	void* AddBytes(const void *Data, size_t NumBytes, size_t Alignment);
	wchar_t* AddFARString(const FARString& Str);
	wchar_t* AddCString(const wchar_t* Str);
	size_t AddStrArray(const wchar_t* const* &Strings, const std::vector<FARString>& NamesArray);
	size_t AddStrArray(const wchar_t* const* &Strings, wchar_t** NamesArray, size_t Count);
	size_t GetSize() const { return (uintptr_t)mCurPtr - (uintptr_t)mBuf; }
};
