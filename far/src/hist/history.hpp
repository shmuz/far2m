#pragma once

/*
history.hpp

История (Alt-F8, Alt-F11, Alt-F12)
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

#include <list>

class Dialog;
class VMenu;

enum enumHISTORYTYPE
{
	HISTORYTYPE_CMD = 0,
	HISTORYTYPE_FOLDER,
	HISTORYTYPE_VIEW,
	HISTORYTYPE_DIALOG
};

enum history_record_type
{
	HR_DEFAULT,
	HR_VIEWER = HR_DEFAULT,
	HR_EDITOR,
	HR_EXTERNAL,
	HR_EXTERNAL_WAIT,
	HR_EDITOR_RO,
};

enum history_return_type
{
	HRT_CANCEL,
	HRT_ENTER,
	HRT_SHIFTENTER,
	HRT_CTRLENTER,
	HRT_F3, //internal
	HRT_F4, //internal
	HRT_CTRLSHIFTENTER,
	HRT_CTRLALTENTER,
};

enum history_remove_dups
{
	HRD_NOREMOVE   = 0,
	HRD_CASESENS   = 1,
	HRD_CASEINSENS = 2,
};

struct HistoryRecord
{
	int Type  = HR_DEFAULT;
	bool Lock = false;
	bool Marked = false;
	FARString strName;
	FARString strExtra;
	FILETIME Timestamp{};

	const HistoryRecord &operator=(const HistoryRecord &rhs)
	{
		if (this != &rhs) {
			strName = rhs.strName;
			strExtra = rhs.strExtra;
			Type = rhs.Type;
			Lock = rhs.Lock;
			Timestamp = rhs.Timestamp;
		}
		return *this;
	}
};

class History
{
private:
	typedef std::list<HistoryRecord>::iterator Iter;

	const enumHISTORYTYPE mTypeHistory;
	const size_t mMaxCount;
	const bool mSaveType;
	const std::string mStrRegKey;
	bool mEnableAdd;
	bool mKeepSelectedPos;
	const int *mEnableSave;
	history_remove_dups mRemoveDups;
	std::list<HistoryRecord> mList;
	Iter mCurrentItem;
	struct stat mLoadedStat {};

private:
	void AddToHistoryLocal(const wchar_t *Str, const wchar_t *Extra, const wchar_t *Prefix, int Type);
	bool EqualType(int Type1, int Type2);
	static const wchar_t *GetTitle(int Type);
	const wchar_t *GetDelTitle() const;
	bool IsAllowedForHistory(const wchar_t *Str) const;
	int ProcessMenu(VMenu &HistoryMenu, const wchar_t *Title, int Height, FARString &strOut,
		int &TypeOut, Dialog *Dlg);
	bool ReadHistory();
	bool SaveHistory();
	void SyncChanges();

public:
	History(enumHISTORYTYPE TypeHistory, size_t HistoryCount, const std::string &RegKey,
		const int *EnableSave, bool SaveType);
	~History() {}

public:
	void AddToHistoryExtra(const wchar_t *Str, const wchar_t *Extra, int Type = HR_DEFAULT,
		const wchar_t *Prefix = nullptr, bool SaveForbid = false);
	void AddToHistory(const wchar_t *Str, int Type = HR_DEFAULT, const wchar_t *Prefix = nullptr,
		bool SaveForbid = false);
	static bool ReadLastItem(const char *RegKey, FARString &strStr);
	int Select(FARString &strOut, int &TypeOut);
	int Select(VMenu &HistoryMenu, int Height, Dialog *Dlg, FARString &strOut);
	void GetPrev(FARString &strStr);
	void GetNext(FARString &strStr);
	bool GetSimilar(FARString &strStr, int LastCmdPartLength, bool bAppend = false);
	bool GetAllSimilar(VMenu &HistoryMenu, const wchar_t *Str);
	bool DeleteMatching(FARString &strStr);
	void SetAddMode(bool EnableAdd, history_remove_dups RemoveDups, bool KeepSelectedPos);
	void ResetPosition() { mCurrentItem = mList.end(); }
};
