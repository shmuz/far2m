/*
panelmix.cpp

Commonly used panel related functions
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


#include "panelmix.hpp"
#include "strmix.hpp"
#include "filepanels.hpp"
#include "config.hpp"
#include "panel.hpp"
#include "ctrlobj.hpp"
#include "keys.hpp"
#include "treelist.hpp"
#include "filelist.hpp"
#include "pathmix.hpp"
#include "lang.hpp"
#include "datetime.hpp"

static const struct { const wchar_t *Symbol; int Width; } ColumnTypes[] =
{
	{ L"N",   0 },      // NAME_COLUMN
	{ L"S",   6 },      // SIZE_COLUMN
	{ L"P",   6 },      // PHYSICAL_COLUMN
	{ L"D",   8 },      // DATE_COLUMN
	{ L"T",   5 },      // TIME_COLUMN,
	{ L"DM", 14 },      // WDATE_COLUMN
	{ L"DC", 14 },      // CDATE_COLUMN
	{ L"DA", 14 },      // ADATE_COLUMN
	{ L"DE", 14 },      // CHDATE_COLUMN
	{ L"A",  10 },      // ATTR_COLUMN
	{ L"Z",   0 },      // DIZ_COLUMN
	{ L"O",   0 },      // OWNER_COLUMN
	{ L"U",   3 },      // GROUP_COLUMN
	{ L"LN",  3 },      // NUMLINK_COLUMN
	{ L"F",   6 },      // RESERVED_COLUMN1 (was: number of streams)
	{ L"G",   0 },      // RESERVED_COLUMN2 (was: size of file streams)
};

int GetColumnTypeWidth(unsigned ColIndex)
{
	return ColIndex < ARRAYSIZE(ColumnTypes) ? ColumnTypes[ColIndex].Width : 0;
}

void ShellUpdatePanels(Panel *SrcPanel,BOOL NeedSetUpADir)
{
	if (!SrcPanel)
		SrcPanel=CtrlObject->Cp()->ActivePanel;

	Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(SrcPanel);

	switch (SrcPanel->GetType())
	{
		case QVIEW_PANEL:
		case INFO_PANEL:
			SrcPanel=CtrlObject->Cp()->GetAnotherPanel(AnotherPanel=SrcPanel);
	}

	int AnotherType=AnotherPanel->GetType();

	if (AnotherType!=QVIEW_PANEL && AnotherType!=INFO_PANEL)
	{
		if (NeedSetUpADir)
		{
			FARString strCurDir;
			SrcPanel->GetCurDir(strCurDir);
			AnotherPanel->SetCurDir(strCurDir,true);
			AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
		}
		else
		{
			// TODO: ???
			//if(AnotherPanel->NeedUpdatePanel(SrcPanel))
			//  AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
			//else
			{
				// Сбросим время обновления панели. Если там есть нотификация - обновится сама.
				if (AnotherType==FILE_PANEL)
					((FileList *)AnotherPanel)->ResetLastUpdateTime();

				AnotherPanel->UpdateIfChanged(UIC_UPDATE_NORMAL);
			}
		}
	}

	SrcPanel->Update(UPDATE_KEEP_SELECTION);

	if (AnotherType==QVIEW_PANEL)
		AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);

	CtrlObject->Cp()->Redraw();
}

int CheckUpdateAnotherPanel(Panel *SrcPanel,const wchar_t *SelName)
{
	if (!SrcPanel)
		SrcPanel=CtrlObject->Cp()->ActivePanel;

	Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(SrcPanel);
	AnotherPanel->CloseFile();

	if (AnotherPanel->GetMode() == NORMAL_PANEL)
	{
		FARString strAnotherCurDir;
		FARString strFullName;
		AnotherPanel->GetCurDir(strAnotherCurDir);
		AddEndSlash(strAnotherCurDir);
		ConvertNameToFull(SelName, strFullName);
		AddEndSlash(strFullName);

		if (wcsstr(strAnotherCurDir,strFullName))
		{
			((FileList*)AnotherPanel)->CloseChangeNotification();
			return TRUE;
		}
	}

	return FALSE;
}

int _MakePath1(DWORD Key, FARString &strPathName, const wchar_t *Param2, int escaping)
{
	int RetCode = FALSE;
	int NeedRealName = FALSE;
	strPathName.Clear();

	switch (Key) {
		case KEY_CTRLALTBRACKET:		// Вставить сетевое (UNC) путь из левой панели
		case KEY_CTRLALTBACKBRACKET:	// Вставить сетевое (UNC) путь из правой панели
		case KEY_ALTSHIFTBRACKET:		// Вставить сетевое (UNC) путь из активной панели
		case KEY_ALTSHIFTBACKBRACKET:	// Вставить сетевое (UNC) путь из пассивной панели
			NeedRealName = TRUE;
		case KEY_CTRLBRACKET:			// Вставить путь из левой панели
		case KEY_CTRLBACKBRACKET:		// Вставить путь из правой панели
		case KEY_CTRLSHIFTBRACKET:		// Вставить путь из активной панели
		case KEY_CTRLSHIFTBACKBRACKET:	// Вставить путь из пассивной панели
		case KEY_CTRLSHIFTNUMENTER:		// Текущий файл с пасс.панели
		case KEY_SHIFTNUMENTER:			// Текущий файл с актив.панели
		case KEY_CTRLSHIFTENTER:		// Текущий файл с пасс.панели
		case KEY_SHIFTENTER:			// Текущий файл с актив.панели
		{
			Panel *SrcPanel = nullptr;
			FilePanels *Cp = CtrlObject->Cp();

			switch (Key) {
				case KEY_CTRLALTBRACKET:
				case KEY_CTRLBRACKET:
					SrcPanel = Cp->LeftPanel;
					break;
				case KEY_CTRLALTBACKBRACKET:
				case KEY_CTRLBACKBRACKET:
					SrcPanel = Cp->RightPanel;
					break;
				case KEY_SHIFTNUMENTER:
				case KEY_SHIFTENTER:
				case KEY_ALTSHIFTBRACKET:
				case KEY_CTRLSHIFTBRACKET:
					SrcPanel = Cp->ActivePanel;
					break;
				case KEY_CTRLSHIFTNUMENTER:
				case KEY_CTRLSHIFTENTER:
				case KEY_ALTSHIFTBACKBRACKET:
				case KEY_CTRLSHIFTBACKBRACKET:
					SrcPanel = Cp->GetAnotherPanel(Cp->ActivePanel);
					break;
			}

			if (SrcPanel) {
				if (Key == KEY_SHIFTENTER || Key == KEY_CTRLSHIFTENTER || Key == KEY_SHIFTNUMENTER
						|| Key == KEY_CTRLSHIFTNUMENTER) {
					SrcPanel->GetCurName(strPathName);
				} else {
					/* TODO: Здесь нужно учесть, что у TreeList тоже есть путь :-) */
					if (!(SrcPanel->GetType() == FILE_PANEL || SrcPanel->GetType() == TREE_PANEL))
						return FALSE;

					SrcPanel->GetCurDirPluginAware(strPathName);
					if (NeedRealName && SrcPanel->GetType() == FILE_PANEL
							&& SrcPanel->GetMode() != PLUGIN_PANEL) {
						FileList *SrcFilePanel = (FileList *)SrcPanel;
						SrcFilePanel->CreateFullPathName(strPathName, FILE_ATTRIBUTE_DIRECTORY, strPathName,
								TRUE);
					}

					AddEndSlash(strPathName);
				}

				if (escaping & Opt.QuotedName & QUOTEDNAME_INSERT)
					EscapeSpace(strPathName);

				if (Param2)
					strPathName+= Param2;

				RetCode = TRUE;
			}
		} break;
	}

	return RetCode;
}

void TextToViewSettings(const wchar_t *ColumnTitles, const wchar_t *ColumnWidths, std::vector<Column> &Columns)
{
	const wchar_t *TextPtr=ColumnTitles;
	FARString strArgName;

	Columns.clear();
	for (int ColumnCount=0; (TextPtr=GetCommaWord(TextPtr,strArgName)); ColumnCount++)
	{
		Columns.emplace_back(Column());
		strArgName.Upper();
		unsigned int &ColumnType=Columns[ColumnCount].Type;

		if (strArgName.At(0)==L'N')
		{
			ColumnType=NAME_COLUMN;

			for (auto Ptr=strArgName.CPtr()+1; *Ptr; Ptr++)
			{
				switch (*Ptr)
				{
					case L'M':
						ColumnType|=COLUMN_MARK;
						break;
					case L'O':
						ColumnType|=COLUMN_NAMEONLY;
						break;
					case L'R':
						ColumnType|=COLUMN_RIGHTALIGN;
						break;
				}
			}
		}
		else
		{
			if (strArgName.At(0)==L'S' || strArgName.At(0)==L'P' || strArgName.At(0)==L'G')
			{
				ColumnType=(strArgName.At(0)==L'S') ? SIZE_COLUMN:PHYSICAL_COLUMN;

				for (auto Ptr=strArgName.CPtr()+1; *Ptr; Ptr++)
				{
					switch (*Ptr)
					{
						case L'C':
							ColumnType|=COLUMN_COMMAS;
							break;
						case L'E':
							ColumnType|=COLUMN_ECONOMIC;
							break;
						case L'F':
							ColumnType|=COLUMN_FLOATSIZE;
							break;
						case L'T':
							ColumnType|=COLUMN_THOUSAND;
							break;
					}
				}
			}
			else
			{
				if (!StrCmpN(strArgName,L"DM",2) || !StrCmpN(strArgName,L"DC",2) || !StrCmpN(strArgName,L"DA",2) || !StrCmpN(strArgName,L"DE",2))
				{
					switch (strArgName.At(1))
					{
						case L'M':
							ColumnType=WDATE_COLUMN;
							break;
						case L'C':
							ColumnType=CDATE_COLUMN;
							break;
						case L'A':
							ColumnType=ADATE_COLUMN;
							break;
						case L'E':
							ColumnType=CHDATE_COLUMN;
							break;
					}

					for (auto Ptr=strArgName.CPtr()+2; *Ptr; Ptr++)
					{
						switch (*Ptr)
						{
							case L'B':
								ColumnType|=COLUMN_BRIEF;
								break;
							case L'M':
								ColumnType|=COLUMN_MONTH;
								break;
						}
					}
				}
				else
				{
					if (strArgName.At(0)==L'U')
					{
						ColumnType=GROUP_COLUMN;
					}
					else if (strArgName.At(0)==L'O')
					{
						ColumnType=OWNER_COLUMN;

						if (strArgName.At(1)==L'L')
							ColumnType|=COLUMN_FULLOWNER;
					}
					else if (strArgName.At(0)==L'C')
					{
						size_t len=strArgName.GetLength();
						if (len>=2 && len<=3)
						{
							if (iswdigit(strArgName.At(1)) && (len==2 || iswdigit(strArgName.At(2))))
								ColumnType=CUSTOM_COLUMN0 + _wtoi(strArgName.CPtr()+1);
						}
					}
					else
					{
						for (unsigned I=0; I<ARRAYSIZE(ColumnTypes); I++)
						{
							if (!StrCmp(strArgName,ColumnTypes[I].Symbol))
							{
								ColumnType=I;
								break;
							}
						}
					}
				}
			}
		}
	}

	TextPtr=ColumnWidths;

	for (size_t I=0; I<Columns.size(); I++)
	{
		FARString strArgName;

		if (!(TextPtr=GetCommaWord(TextPtr,strArgName)))
			break;

		Columns[I].Width=_wtoi(strArgName);
		Columns[I].WidthType=COUNT_WIDTH;

		if (strArgName.GetLength()>1)
		{
			switch (strArgName.At(strArgName.GetLength()-1))
			{
				case L'%':
					Columns[I].WidthType=PERCENT_WIDTH;
					break;
			}
		}
	}
}


void ViewSettingsToText(const std::vector<Column> &Columns, FARString &strColumnTitles, FARString &strColumnWidths)
{
	strColumnTitles.Clear();
	strColumnWidths.Clear();

	for (size_t I=0; I<Columns.size(); I++)
	{
		unsigned ColumnType = Columns[I].Type & 0xff;
		FARString strType = ColumnType<ARRAYSIZE(ColumnTypes) ? ColumnTypes[ColumnType].Symbol : L"";

		if (ColumnType==NAME_COLUMN)
		{
			if (Columns[I].Type & COLUMN_MARK)
				strType += L"M";

			if (Columns[I].Type & COLUMN_NAMEONLY)
				strType += L"O";

			if (Columns[I].Type & COLUMN_RIGHTALIGN)
				strType += L"R";
		}

		else if (ColumnType==SIZE_COLUMN || ColumnType==PHYSICAL_COLUMN)
		{
			if (Columns[I].Type & COLUMN_COMMAS)
				strType += L"C";

			if (Columns[I].Type & COLUMN_ECONOMIC)
				strType += L"E";

			if (Columns[I].Type & COLUMN_FLOATSIZE)
				strType += L"F";

			if (Columns[I].Type & COLUMN_THOUSAND)
				strType += L"T";
		}

		else if (ColumnType==WDATE_COLUMN || ColumnType==ADATE_COLUMN || ColumnType==CDATE_COLUMN  || ColumnType==CHDATE_COLUMN)
		{
			if (Columns[I].Type & COLUMN_BRIEF)
				strType += L"B";

			if (Columns[I].Type & COLUMN_MONTH)
				strType += L"M";
		}

		else if (ColumnType==OWNER_COLUMN)
		{
			if (Columns[I].Type & COLUMN_FULLOWNER)
				strType += L"L";
		}

		else if (ColumnType>=CUSTOM_COLUMN0 && ColumnType<=CUSTOM_COLUMN_LAST)
		{
			wchar_t buf[8];
			swprintf(buf, ARRAYSIZE(buf), L"C%d", int(ColumnType-CUSTOM_COLUMN0));
			strType = buf;
		}

		strColumnTitles += strType;
		wchar_t *lpwszWidth = strType.GetBuffer(20);
		_itow(Columns[I].Width,lpwszWidth,10);
		strType.ReleaseBuffer();
		strColumnWidths += strType;

		switch (Columns[I].WidthType)
		{
			case PERCENT_WIDTH:
				strColumnWidths += L"%";
				break;
		}

		if (I < Columns.size()-1)
		{
			strColumnTitles += L",";
			strColumnWidths += L",";
		}
	}
}

const FARString FormatStr_Attribute(DWORD FileAttributes, DWORD UnixMode, int Width)
{
	FormatString strResult;
	wchar_t OutStr[16] = {};
	if (UnixMode != 0) {
		if (FileAttributes & FILE_ATTRIBUTE_BROKEN)
			OutStr[0] = L'B';
		else if (FileAttributes & FILE_ATTRIBUTE_DEVICE_CHAR)
			OutStr[0] = L'c';
		else if (FileAttributes & FILE_ATTRIBUTE_DEVICE_BLOCK)
			OutStr[0] = L'b';
		else if (FileAttributes & FILE_ATTRIBUTE_DEVICE_FIFO)
			OutStr[0] = L'p';
		else if (FileAttributes & FILE_ATTRIBUTE_DEVICE_SOCK)
			OutStr[0] = L's';
		/*else if (FileAttributes & FILE_ATTRIBUTE_DEVICE)
			OutStr[0] = L'V';*/
		else if (FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
			OutStr[0] = L'l';
		else if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			OutStr[0] = L'd';
		else
			OutStr[0] = L'-';

		OutStr[1] = UnixMode & S_IRUSR ? L'r' : L'-';
		OutStr[2] = UnixMode & S_IWUSR ? L'w' : L'-';
		OutStr[3] = UnixMode & S_IXUSR ? (UnixMode & S_ISUID ? L's' : L'x') : (UnixMode & S_ISUID ? L'S' : L'-');
		OutStr[4] = UnixMode & S_IRGRP ? L'r' : L'-';
		OutStr[5] = UnixMode & S_IWGRP ? L'w' : L'-';
		OutStr[6] = UnixMode & S_IXGRP ? (UnixMode & S_ISGID ? L's' : L'x') : (UnixMode & S_ISGID ? L'S' : L'-');
		OutStr[7] = UnixMode & S_IROTH ? L'r' : L'-';
		OutStr[8] = UnixMode & S_IWOTH ? L'w' : L'-';
		OutStr[9] = UnixMode & S_IXOTH ? (UnixMode & S_ISVTX ? L't' : L'x') : (UnixMode & S_ISVTX ? L'T' : L'-');
	} else {
		OutStr[0] = FileAttributes & FILE_ATTRIBUTE_EXECUTABLE ? L'X' : L' ';
		OutStr[1] = FileAttributes & FILE_ATTRIBUTE_READONLY ? L'R' : L' ';
		OutStr[2] = FileAttributes & FILE_ATTRIBUTE_SYSTEM ? L'S' : L' ';
		OutStr[3] = FileAttributes & FILE_ATTRIBUTE_HIDDEN ? L'H' : L' ';
		OutStr[4] = FileAttributes & FILE_ATTRIBUTE_ARCHIVE ? L'A' : L' ';
		OutStr[5] = FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? L'L'
				: FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE
				? L'$'
				: L' ';
		OutStr[6] = FileAttributes & FILE_ATTRIBUTE_COMPRESSED ? L'C'
				: FileAttributes & FILE_ATTRIBUTE_ENCRYPTED
				? L'E'
				: L' ';
		OutStr[7] = FileAttributes & FILE_ATTRIBUTE_TEMPORARY ? L'T' : L' ';
		OutStr[8] = FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ? L'I' : L' ';
		OutStr[9] = FileAttributes & FILE_ATTRIBUTE_OFFLINE ? L'O' : L' ';
		OutStr[10] = FileAttributes & FILE_ATTRIBUTE_VIRTUAL ? L'V' : L' ';
	}

	if (Width > 0)
		strResult << fmt::Size(Width);

	strResult << OutStr;

	return std::move(strResult.strValue());
}

const FARString FormatStr_DateTime(const FILETIME *FileTime,int ColumnType,DWORD Flags,int Width)
{
	FormatString strResult;

	if (Width < 0)
	{
		if (ColumnType == DATE_COLUMN)
			Width=0;
		else
			return std::move(strResult.strValue());
	}

	int ColumnWidth=Width;
	int Brief=Flags & COLUMN_BRIEF;
	int TextMonth=Flags & COLUMN_MONTH;
	int FullYear=FALSE;

	switch(ColumnType)
	{
		case DATE_COLUMN:
		case TIME_COLUMN:
		{
			Brief=FALSE;
			TextMonth=FALSE;
			if (ColumnType == DATE_COLUMN)
				FullYear=ColumnWidth>9;
			break;
		}
		case WDATE_COLUMN:
		case CDATE_COLUMN:
		case ADATE_COLUMN:
		case CHDATE_COLUMN:
		{
			if (!Brief)
			{
				int CmpWidth=ColumnWidth-TextMonth;

				if (CmpWidth==15 || CmpWidth==16 || CmpWidth==18 || CmpWidth==19 || CmpWidth>21)
					FullYear=TRUE;
			}
			ColumnWidth-=9;
			break;
		}
	}

	FARString strDateStr,strTimeStr;

	ConvertDate(*FileTime,strDateStr,strTimeStr,ColumnWidth,Brief,TextMonth,FullYear);

	strResult << fmt::Size(Width);
	switch(ColumnType)
	{
		case DATE_COLUMN:
			strResult<<strDateStr;
			break;
		case TIME_COLUMN:
			strResult<<strTimeStr;
			break;
		default:
			strResult << (strDateStr + L" " + strTimeStr);
			break;
	}

	return std::move(strResult.strValue());
}

const FARString FormatStr_Size(int64_t FileSize, int64_t PhysicalSize, const FARString &strName,DWORD FileAttributes,uint8_t ShowFolderSize,int ColumnType,DWORD Flags,int Width)
{
	FormatString strResult;

	bool Physical=(ColumnType==PHYSICAL_COLUMN);

	if (ShowFolderSize==2)
	{
		Width--;
		strResult<<L"~";
	}

	if (!Physical && (FileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)) && !ShowFolderSize)
	{
		const wchar_t *PtrName=Msg::ListFolder;

		if (TestParentFolderName(strName))
		{
			PtrName=Msg::ListUp;
		}
		else if (FileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)
		{
			PtrName=Msg::ListSymLink;
		}

		strResult<<fmt::Expand(Width)<<fmt::Truncate(Width);
		if (StrLength(PtrName) <= Width-2) {
			// precombine into tmp string to avoid miseffect of fmt::Expand etc (#1137)
			strResult<<FARString(L"<").Append(PtrName).Append(L">");
		} else {
			strResult<<PtrName;
		}

	}
	else
	{
		FARString strOutStr;
		strResult<<FileSizeToStr(strOutStr,Physical?PhysicalSize:FileSize,Width,Flags).CPtr();
	}

	return std::move(strResult.strValue());
}
