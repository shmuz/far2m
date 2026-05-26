/*
dizlist.cpp

Описания файлов
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

#include "dizlist.hpp"
#include "lang.hpp"
#include "savescr.hpp"
#include "TPreRedrawFunc.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "filestr.hpp"
#include "codepage.hpp"
#include "cache.hpp"

DizList::DizList()
	:
	Modified(false),
	NeedRebuild(true),
	OrigCodePage(CP_AUTODETECT)
{}

DizList::~DizList()
{
	Reset();
}

void DizList::Reset()
{
	DizData.clear();
	Modified = false;
	NeedRebuild = true;
	OrigCodePage = CP_AUTODETECT;
}

void DizList::PR_ReadingMsg()
{
	Message(0, 0, L"", Msg::ReadingDiz);
}

void DizList::Read(const wchar_t *Path, const wchar_t *DizName)
{
	Reset();
	SCOPED_ACTION(TPreRedrawFuncGuard)(DizList::PR_ReadingMsg);
	const wchar_t *NamePtr = Opt.Diz.strListNames;

	for (;;) {
		if (DizName) {
			strDizFileName = DizName;
		}
		else {
			strDizFileName = Path;

			if (!PathCanHoldRegularFile(strDizFileName))
				break;

			FARString strArgName;

			if (!(NamePtr = GetCommaWord(NamePtr, strArgName)))
				break;

			AddEndSlash(strDizFileName);
			strDizFileName += strArgName;
		}

		File DizFile;
		FAR_FIND_DATA_EX FindData;
		if (apiGetFindDataEx(strDizFileName, FindData, FIND_FILE_FLAG_CASE_INSENSITIVE) &&
				DizFile.Open(FindData.strFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING))
		{
			strDizFileName = FindData.strFileName;
			GetFileString GetStr(DizFile);
			wchar_t *DizText;
			int DizLength;
			clock_t StartTime = GetProcessUptimeMSec();
			UINT CodePage = CP_AUTODETECT;

			if (!GetFileFormat(DizFile, CodePage))
				CodePage = Opt.Diz.AnsiByDefault ? CP_ACP : CP_OEMCP;

			while (GetStr.GetString(&DizText, CodePage, DizLength) > 0) {
				if (!(DizData.size() & 127) && GetProcessUptimeMSec() - StartTime > 1000) {
					SetCursorType(false, 0);
					PR_ReadingMsg();

					if (CheckForEsc())
						break;
				}

				RemoveTrailingSpaces(DizText);

				if (*DizText)
					AddRecord(DizText);
			}

			OrigCodePage = CodePage;
			Modified = false;
			return;
		}

		if (DizName)
			break;
	}

	Modified = false;
	strDizFileName.Clear();
}

bool DizList::AddRecord(const wchar_t *DizText)
{
	auto &Rec = DizData.emplace_back(DizRecord{});
	Rec.DizText = DizText;
	Rec.NameStart = 0;
	Rec.NameLength = 0;
	Rec.Deleted = false;

	if (*DizText == L'\"') {
		DizText++;
		Rec.NameStart++;

		while (*DizText && *DizText != L'\"') {
			DizText++;
			Rec.NameLength++;
		}
	}
	else {
		while (!IsSpaceOrEos(*DizText)) {
			DizText++;
			Rec.NameLength++;
		}
	}

	NeedRebuild = true;
	Modified = true;
	return true;
}

const wchar_t *DizList::GetDizTextAddr(const wchar_t *Name)
{
	const wchar_t *DizText = nullptr;
	int TextPos;
	int DizPos = GetDizPosEx(Name, &TextPos);

	if (DizPos != -1) {
		DizText = DizData[DizPos].DizText + TextPos;

		while (*DizText && IsSpace(*DizText))
			DizText++;
	}

	return DizText;
}

int DizList::GetDizPosEx(const wchar_t *Name, int *TextPos)
{
	int DizPos = GetDizPos(Name, TextPos);

	// если файл описаний был в OEM/ANSI то имена файлов могут не совпадать с юникодными
	if (DizPos == -1 && !IsUnicodeOrUtfCodePage(OrigCodePage) && OrigCodePage != CP_AUTODETECT)
	{
		int len = WINPORT(WideCharToMultiByte)(OrigCodePage, 0, Name, -1, nullptr, 0, nullptr, nullptr);
		if (!len)
			return -1;

		std::vector<char> AnsiBuf(len);
		if (!WINPORT(WideCharToMultiByte)(OrigCodePage, 0, Name, -1, AnsiBuf.data(), len, nullptr, nullptr))
			return -1;

		FARString strRecoded(AnsiBuf.data(), OrigCodePage);
		return (strRecoded == Name) ? - 1 : GetDizPos(strRecoded, TextPos);
	}

	return DizPos;
}

static bool CompareDiz(const DizRecord& Rec1, const DizRecord& Rec2)
{
	if (Rec1.Deleted != Rec2.Deleted)
		return !Rec1.Deleted;

	const wchar_t *Diz1 = Rec1.DizText + Rec1.NameStart;
	const wchar_t *Diz2 = Rec2.DizText + Rec2.NameStart;
	int CmpCode = StrCmpN(Diz1, Diz2, Min(Rec1.NameLength, Rec2.NameLength));

	return (CmpCode != 0) ? (CmpCode < 0) : (Rec1.NameLength < Rec2.NameLength);
}

int DizList::GetDizPos(const wchar_t *Name, int *TextPos)
{
	if (DizData.empty() || !*Name)
		return -1;

	if (NeedRebuild) {
		std::sort(DizData.begin(), DizData.end(), CompareDiz);
		NeedRebuild = false;
	}

	DizRecord Key {
		.DizText = Name,
		.NameStart = 0,
		.NameLength = StrLength(Name),
		.Deleted = false
	};

	auto It = std::lower_bound(DizData.begin(), DizData.end(), Key, CompareDiz);

	bool Found = It != DizData.end() // ensure that here >= is actually ==
			&& !It->Deleted
			&& It->NameLength == Key.NameLength
			&& !StrCmpN(It->DizText + It->NameStart, Key.DizText, Key.NameLength);

	if (Found) {
		if (TextPos) {
			*TextPos = It->NameStart + It->NameLength;

			if (It->NameStart && It->DizText[*TextPos] == L'\"')
				(*TextPos)++;
		}
		return It - DizData.begin();
	}

	return -1;
}

bool DizList::DeleteDiz(const wchar_t *Name)
{
	int DizPos = GetDizPosEx(Name, nullptr);

	if (DizPos == -1)
		return false;

	DizData[DizPos].Deleted = true;

	for (DizPos++; DizPos < (int)DizData.size(); DizPos++) {
		auto &Rec = DizData[DizPos];
		if (!Rec.DizText.IsEmpty() && !IsSpace(Rec.DizText[0]))
			break;

		Rec.Deleted = true;
	}

	Modified = true;
	NeedRebuild = true;
	return true;
}

bool DizList::Flush(const wchar_t *Path, const wchar_t *DizName)
{
	if (!Modified)
		return true;

	if (DizName) {
		strDizFileName = DizName;
	}
	else if (strDizFileName.IsEmpty()) {
		if (DizData.empty() || !Path)
			return false;

		strDizFileName = Path;
		AddEndSlash(strDizFileName);
		FARString strArgName;
		GetCommaWord(Opt.Diz.strListNames, strArgName);
		strDizFileName+= strArgName;
	}

	DWORD FileAttr = apiGetFileAttributes(strDizFileName);

	if (FileAttr != INVALID_FILE_ATTRIBUTES) {
		if (FileAttr & FILE_ATTRIBUTE_READONLY) {
			if (Opt.Diz.ROUpdate) {
				if (apiSetFileAttributes(strDizFileName, FileAttr)) {
					FileAttr^= FILE_ATTRIBUTE_READONLY;
				}
			}
		}

		if (!(FileAttr & FILE_ATTRIBUTE_READONLY)) {
			apiSetFileAttributes(strDizFileName, FILE_ATTRIBUTE_ARCHIVE);
		}
		else {
			Message(MSG_WARNING, 1, Msg::Error, Msg::CannotUpdateDiz, Msg::CannotUpdateRODiz, Msg::Ok);
			return false;
		}
	}

	File DizFile;

	bool AnyError = false;

	bool EmptyDiz = true;
	// Don't use CreationDisposition=CREATE_ALWAYS here - it's kills alternate streams
	if (!DizData.empty()
			&& DizFile.Open(strDizFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
					FileAttr == INVALID_FILE_ATTRIBUTES ? CREATE_NEW : TRUNCATE_EXISTING))
	{
		CachedWrite Cache(DizFile);

		DWORD dwSignature = SIGN_UTF8;
		if (!Cache.Write(&dwSignature, 3)) {
			AnyError = true;
		}

		if (!AnyError) {
			for (auto &Rec: DizData) {
				if (!Rec.Deleted) {
					std::string utf8Text = Wide2MB(Rec.DizText);
					if (!utf8Text.empty()) {
						if (Cache.Write(utf8Text.c_str(), utf8Text.size())) {
							EmptyDiz = false;
						}
						else {
							AnyError = true;
							break;
						}
						if (!Cache.Write("\n", 1)) {
							AnyError = true;
							break;
						}
					}
				}
			}
		}

		if (!AnyError) {
			if (!Cache.Flush()) {
				AnyError = true;
			}
		}

		DizFile.Close();
	}

	if (!EmptyDiz && !AnyError) {
		if (FileAttr == INVALID_FILE_ATTRIBUTES) {
			FileAttr = FILE_ATTRIBUTE_ARCHIVE | (Opt.Diz.SetHidden ? FILE_ATTRIBUTE_HIDDEN : 0);
		}
		apiSetFileAttributes(strDizFileName, FileAttr);
	}
	else {
		apiDeleteFile(strDizFileName);
		if (AnyError) {
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, Msg::CannotUpdateDiz, Msg::Ok);
			return false;
		}
	}

	Modified = false;
	return true;
}

bool DizList::AddDizText(const wchar_t *Name, const wchar_t *DizText)
{
	DeleteDiz(Name);
	FARString strQuotedName = Name;
	QuoteSpaceOnly(strQuotedName);
	FormatString FString;
	FString << fmt::LeftAlign() << fmt::Expand(Opt.Diz.StartPos > 1 ? Opt.Diz.StartPos - 2 : 0)
			<< strQuotedName << L" " << DizText;
	return AddRecord(FString);
}

bool DizList::CopyDiz(const wchar_t *Name, const wchar_t *DestName, DizList *DestDiz)
{
	int TextPos;
	int DizPos = GetDizPosEx(Name, &TextPos);

	if (DizPos == -1)
		return false;

	const wchar_t *DizText = DizData[DizPos].DizText;
	while (IsSpace(DizText[TextPos]))
		TextPos++;

	DestDiz->AddDizText(DestName, &DizText[TextPos]);

	while (++DizPos < (int)DizData.size()) {
		DizText = DizData[DizPos].DizText;
		if (*DizText && !IsSpace(*DizText))
			break;

		DestDiz->AddRecord(DizText);
	}

	return true;
}

void DizList::GetDizName(FARString &strDizName)
{
	strDizName = strDizFileName;
}
