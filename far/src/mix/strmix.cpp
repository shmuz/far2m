/*
strmix.cpp

Куча разных вспомогательных функций по работе со строками
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


#include "strmix.hpp"
#include "lang.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "StackHeapArray.hpp"

FARString &FormatNumber(const wchar_t *Src, FARString &strDest, int NumDigits)
{
	FARString result;//can't use strDest cuz Src may point to its internal buffer

	const wchar_t *dot = wcschr(Src, L'.');
	const wchar_t *part = dot ? dot : Src + wcslen(Src);
	if (part == Src) {
		result = L"0";
	} else {
		size_t i = 0;
		for (;;) {
			--part;
			result.Insert(0, *part);
			if (part == Src) break;
			++i;
			if ((i % 3)==0)
				result.Insert(0, L' ');
		}
	}
	if (dot) {
		result.Append(dot, std::min(wcslen(dot), (size_t)NumDigits + 1) );
	}
	strDest = std::move(result);
	return strDest;

	/*
	static bool first = true;
	static NUMBERFMT fmt;
	static wchar_t DecimalSep[4];
	static wchar_t ThousandSep[4];

	if (first)
	{
		GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STHOUSAND,ThousandSep,ARRAYSIZE(ThousandSep));
		GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SDECIMAL,DecimalSep,ARRAYSIZE(DecimalSep));
		DecimalSep[1]=0;  //В винде сепараторы цифр могут быть больше одного символа
		ThousandSep[1]=0; //но для нас это будет не очень хорошо

		if (LOWORD(Opt.FormatNumberSeparators))
			*DecimalSep=LOWORD(Opt.FormatNumberSeparators);

		if (HIWORD(Opt.FormatNumberSeparators))
			*ThousandSep=HIWORD(Opt.FormatNumberSeparators);

		fmt.LeadingZero = 1;
		fmt.Grouping = 3;
		fmt.lpDecimalSep = DecimalSep;
		fmt.lpThousandSep = ThousandSep;
		fmt.NegativeOrder = 1;
		first = false;
	}

	fmt.NumDigits = NumDigits;
	FARString strSrc=Src;
	int Size=GetNumberFormat(LOCALE_USER_DEFAULT,0,strSrc,&fmt,nullptr,0);
	wchar_t* lpwszDest=strDest.GetBuffer(Size);
	GetNumberFormat(LOCALE_USER_DEFAULT,0,strSrc,&fmt,lpwszDest,Size);
	strDest.ReleaseBuffer();
	return strDest;*/
}

FARString &InsertCommas(uint64_t li,FARString &strDest)
{
	strDest.Format(L"%llu", li);
	return FormatNumber(strDest,strDest);
}

static wchar_t * WINAPI InsertCustomQuote(wchar_t *Str,wchar_t QuoteChar)
{
	size_t l = StrLength(Str);

	if (*Str != QuoteChar)
	{
		wmemmove(Str+1,Str,++l);
		*Str=QuoteChar;
	}

	if (l==1 || Str[l-1] != QuoteChar)
	{
		Str[l++] = QuoteChar;
		Str[l] = 0;
	}

	return Str;
}

static FARString& InsertCustomQuote(FARString &strStr,wchar_t QuoteChar)
{
	size_t l = strStr.GetLength();

	if (strStr.At(0) != QuoteChar)
	{
		strStr.Insert(0,QuoteChar);
		l++;
	}

	if (l==1 || strStr.At(l-1) != QuoteChar)
	{
		strStr += QuoteChar;
	}

	return strStr;
}

wchar_t * WINAPI InsertQuote(wchar_t *Str)
{
	return InsertCustomQuote(Str,L'\"');
}

wchar_t * WINAPI InsertRegexpQuote(wchar_t *Str)
{
	if (Str && *Str != L'/')
		return InsertCustomQuote(Str,L'/');
	else          //выражение вида /regexp/i не дополняем слэшем
		return Str;
}

FARString& InsertQuote(FARString &strStr)
{
	return InsertCustomQuote(strStr,L'\"');
}

FARString& InsertRegexpQuote(FARString &strStr)
{
	if (strStr.IsEmpty() || strStr[0] != L'/')
		return InsertCustomQuote(strStr,L'/');
	else          //выражение вида /regexp/i не дополняем слэшем
		return strStr;
}


static FARString escapeSpace(const wchar_t* str) {
	if (*str == L'\0')
		return "''";
	FARString result;
	for (const wchar_t *cur = str; *cur; ++cur) {
		if (wcschr(Opt.strQuotedSymbols, *cur) != nullptr)
			result.Append('\\');
		result.Append(*cur);
	}
	return result;
}

FARString &EscapeSpace(FARString &strStr)
{
	if (strStr.IsEmpty() || strStr.ContainsAnyOf(Opt.strQuotedSymbols.CPtr())) {
		strStr.Copy(escapeSpace(strStr.CPtr()));
	}

	return strStr;
}

static FARString unEscapeSpace(const wchar_t *str)
{
	if (*str == L'\0')
		return "''";
	FARString result;
	for (const wchar_t *cur = str; *cur; ++cur) {
		if (*cur == L'\\' && *(cur+1) != L'\\')
			continue;
		result.Append(*cur);
	}
	return result;
}

FARString &UnEscapeSpace(FARString &strStr)
{
	if (strStr.IsEmpty() || strStr.Contains(L'\\')) {
		strStr.Copy(unEscapeSpace(strStr.CPtr()));
	}

	return strStr;
}

wchar_t*  WINAPI QuoteSpaceOnly(wchar_t *Str)
{
	if (wcschr(Str,L' '))
		InsertQuote(Str);

	return Str;
}


FARString& WINAPI QuoteSpaceOnly(FARString &strStr)
{
	if (strStr.Contains(L' '))
		InsertQuote(strStr);

	return(strStr);
}


FARString& WINAPI TruncStrFromEnd(FARString &strStr, int MaxLength)
{
	wchar_t *lpwszBuffer = strStr.GetBuffer();
	TruncStrFromEnd(lpwszBuffer, MaxLength);
	strStr.ReleaseBuffer();
	return strStr;
}

wchar_t* WINAPI TruncStrFromEnd(wchar_t *Str,int MaxLength)
{
	assert(MaxLength >= 0);

	MaxLength=Max(0, MaxLength);

	const size_t Len = StrLength(Str);
	size_t n = Len;
	StrCellsTruncateRight(Str, n, MaxLength);
	assert(n <= Len);
	Str[n] = 0;

	return Str;
}


wchar_t* WINAPI TruncStr(wchar_t *Str,int MaxLength)
{
	assert(MaxLength >= 0);

	MaxLength=Max(0, MaxLength);

	const size_t Len = StrLength(Str);
	size_t n = Len;
	StrCellsTruncateLeft(Str, n, MaxLength);
	assert(n <= Len);
	Str[n] = 0;

	return Str;
}


FARString& WINAPI TruncStr(FARString &strStr, int MaxLength)
{
	wchar_t *lpwszBuffer = strStr.GetBuffer();
	TruncStr(lpwszBuffer, MaxLength);
	strStr.ReleaseBuffer();
	return strStr;
}

wchar_t* TruncStrFromCenter(wchar_t *Str, int MaxLength)
{
	assert(MaxLength >= 0);

	MaxLength=Max(0, MaxLength);

	const size_t Len = StrLength(Str);
	size_t n = Len;
	StrCellsTruncateCenter(Str, n, MaxLength);
	assert(n <= Len);
	Str[n] = 0;
	return Str;
}

FARString& TruncStrFromCenter(FARString &strStr, int MaxLength)
{
	wchar_t *lpwszBuffer = strStr.GetBuffer();
	TruncStrFromCenter(lpwszBuffer, MaxLength);
	strStr.ReleaseBuffer();
	return strStr;
}

wchar_t* WINAPI TruncPathStr(wchar_t *Str, int MaxLength)
{
	//  TODO
				return TruncStr(Str, MaxLength);
}


FARString& WINAPI TruncPathStr(FARString &strStr, int MaxLength)
{
	wchar_t *lpwszStr = strStr.GetBuffer();
	TruncPathStr(lpwszStr, MaxLength);
	strStr.ReleaseBuffer();
	return strStr;
}


wchar_t* WINAPI RemoveLeadingSpaces(wchar_t *Str)
{
	wchar_t *ChPtr = Str;

	if (!ChPtr)
		return nullptr;

	for (; IsSpace(*ChPtr) || IsEol(*ChPtr); ChPtr++)
		;

	if (ChPtr!=Str)
		wmemmove(Str,ChPtr,StrLength(ChPtr)+1);

	return Str;
}


FARString& WINAPI RemoveLeadingSpaces(FARString &strStr)
{
	const wchar_t *ChPtr = strStr;

	for (; IsSpace(*ChPtr) || IsEol(*ChPtr); ChPtr++)
		;

	strStr.Remove(0,ChPtr-strStr.CPtr());
	return strStr;
}


// удалить конечные пробелы
wchar_t* WINAPI RemoveTrailingSpaces(wchar_t *Str)
{
	if (!Str)
		return nullptr;

	if (!*Str)
		return Str;

	for (wchar_t *ChPtr=Str+StrLength(Str)-1; ChPtr >= Str; ChPtr--)
	{
		if (IsSpace(*ChPtr) || IsEol(*ChPtr))
			*ChPtr=0;
		else
			break;
	}

	return Str;
}


FARString& WINAPI RemoveTrailingSpaces(FARString &strStr, bool keep_escaping)
{
	const wchar_t *Str = strStr;
	const wchar_t *ChPtr = Str + strStr.GetLength() - 1;
	int nSpaces = 0;

	for (; ChPtr >= Str && (IsSpace(*ChPtr) || IsEol(*ChPtr)); ChPtr--)
		++nSpaces;

	if (nSpaces) {
		if (keep_escaping) {
			bool bEscape = false;
			for (; (ChPtr >= Str && *ChPtr == L'\\'); --ChPtr) {
				bEscape = !bEscape;
			}
			if (bEscape)
				--nSpaces;
		}
		if (nSpaces)
			strStr.Truncate(strStr.GetLength() - nSpaces);
	}
	return strStr;
}


wchar_t* WINAPI RemoveExternalSpaces(wchar_t *Str)
{
	return RemoveTrailingSpaces(RemoveLeadingSpaces(Str));
}

FARString&  WINAPI RemoveExternalSpaces(FARString &strStr)
{
	return RemoveTrailingSpaces(RemoveLeadingSpaces(strStr));
}


/* $ 02.02.2001 IS
   Заменяет пробелами непечатные символы в строке. В настоящий момент
   обрабатываются только cr и lf.
*/
FARString& WINAPI RemoveUnprintableCharacters(FARString &strStr)
{
	wchar_t *p = strStr.GetBuffer();

	while (*p)
	{
		if (IsEol(*p))
			*p=L' ';

		p++;
	}

	strStr.ReleaseBuffer(strStr.GetLength());
	return RemoveExternalSpaces(strStr);
}


// Удалить символ Target из строки Str (везде!)
FARString &RemoveChar(FARString &strStr,wchar_t Target,bool Dup)
{
	wchar_t *Ptr = strStr.GetBuffer();
	wchar_t *Str = Ptr, Chr;

	while ((Chr=*Str++) )
	{
		if (Chr == Target)
		{
			if (Dup && *Str == Target)
			{
				*Ptr++ = Chr;
				++Str;
			}

			continue;
		}

		*Ptr++ = Chr;
	}

	*Ptr = L'\0';
	strStr.ReleaseBuffer();
	return strStr;
}

FARString& CenterStr(const wchar_t *Src, FARString &strDest, int Length)
{
	FARString strTempStr = Src; //если Src == strDest, то надо копировать Src!
	int SrcLength = strTempStr.GetLength();

	if (SrcLength >= Length)
	{
		/* Здесь не надо отнимать 1 от длины, т.к. strlen не учитывает \0
		   и мы получали обрезанные строки */
		strDest = std::move(strTempStr);
		strDest.Truncate(Length);
	}
	else
	{
		int Space = (Length - SrcLength) / 2;
		FormatString FString;
		FString << fmt::Expand(Space) << L"" << strTempStr << fmt::Expand(Length - Space - SrcLength) << L"";
		strDest = std::move(FString.strValue());
	}

	return strDest;
}

FARString FixedSizeStr(FARString str, size_t Cells, bool RAlign, bool TruncateCenter)
{
	const size_t InitialStrCells = str.CellsCount();
	if (InitialStrCells > Cells)
	{
		if (TruncateCenter)
			TruncStrFromCenter(str, Cells);
		else
			TruncStr(str, Cells);
	}
	else if (InitialStrCells < Cells)
	{
		if (RAlign)
			str.Insert(0, L' ', Cells - InitialStrCells);
		else
			str.Append(L' ', Cells - InitialStrCells);
	}
	return str;
}


const wchar_t *GetCommaWord(const wchar_t *Src, FARString &strWord, wchar_t Separator, wchar_t Separator2)
{
	if (!*Src)
		return nullptr;

	const wchar_t *StartPtr = Src;
	size_t WordLen;
	bool SkipBrackets=false;

	for (WordLen=0; *Src; Src++,WordLen++)
	{
		if (*Src==L'[' && wcschr(Src+1,L']'))
			SkipBrackets=true;

		if (*Src==L']')
			SkipBrackets=false;

		if ((*Src==Separator || *Src==Separator2) && !SkipBrackets)
		{
			Src++;

			while (IsSpace(*Src))
				Src++;

			strWord.Copy(StartPtr,WordLen);
			return Src;
		}
	}

	strWord.Copy(StartPtr,WordLen);
	return Src;
}


bool IsCaseMixed(const FARString &strSrc)
{
	const wchar_t *lpwszSrc = strSrc;

	while (*lpwszSrc && !IsAlpha(*lpwszSrc))
		lpwszSrc++;

	int Case = IsLower(*lpwszSrc);

	while (*(lpwszSrc++))
		if (IsAlpha(*lpwszSrc) && (IsLower(*lpwszSrc) != Case))
			return true;

	return false;
}

bool IsCaseLower(const FARString &strSrc)
{
	const wchar_t *lpwszSrc = strSrc;

	while (*lpwszSrc)
	{
		if (!IsLower(*lpwszSrc))
			return false;

		lpwszSrc++;
	}

	return true;
}



void WINAPI Unquote(wchar_t *Str)
{
	if (!Str)
		return;

	wchar_t *Dst=Str;

	while (*Str)
	{
		if (*Str!=L'\"')
			*Dst++=*Str;

		Str++;
	}

	*Dst=0;
}


void WINAPI Unquote(FARString &strStr)
{
	wchar_t *Dst = strStr.GetBuffer();
	const wchar_t *Str = Dst;
	const wchar_t *StartPtr = Dst;

	while (*Str)
	{
		if (*Str!=L'\"')
			*Dst++=*Str;

		Str++;
	}

	strStr.ReleaseBuffer(Dst-StartPtr);
}


void UnquoteExternal(FARString &strStr)
{
	size_t len = strStr.GetLength();

	if (len > 1 && strStr.At(0) == L'\"' && strStr.At(len-1) == L'\"')
	{
		strStr.Truncate(len-1);
		strStr.LShift(1);
	}
}


/* FileSizeToStr()
   Форматирование размера файла в удобочитаемый вид.
*/
#define MAX_UNITSTR_SIZE 16

#define UNIT_COUNT 7 // byte, kilobyte, megabyte, gigabyte, terabyte, petabyte, exabyte.

static wchar_t UnitStr[UNIT_COUNT][2][MAX_UNITSTR_SIZE]={};

void PrepareUnitStr()
{
	for (int i=0; i<UNIT_COUNT; i++)
	{
		far_wcsncpy(UnitStr[i][0], (Msg::ListBytes+i), MAX_UNITSTR_SIZE);
		wcscpy(UnitStr[i][1],UnitStr[i][0]);
		WINPORT(CharLower)(UnitStr[i][0]);
		WINPORT(CharUpper)(UnitStr[i][1]);
	}
}

FARString & WINAPI FileSizeToStr(FARString &strDestStr, uint64_t Size, int Width, int ViewFlags)
{
	FARString strStr;
	uint64_t Divider;
	int IndexDiv, IndexB;

	// подготовительные мероприятия
	if (!UnitStr[0][0][0])
	{
		PrepareUnitStr();
	}

	int Commas=(ViewFlags & COLUMN_COMMAS);
	int FloatSize=(ViewFlags & COLUMN_FLOATSIZE);
	int Economic=(ViewFlags & COLUMN_ECONOMIC);
	int UseMinSizeIndex=(ViewFlags & COLUMN_MINSIZEINDEX);
	int MinSizeIndex=(ViewFlags & COLUMN_MINSIZEINDEX_MASK)+1;
	int ShowBytesIndex=(ViewFlags & COLUMN_SHOWBYTESINDEX);

	if (ViewFlags & COLUMN_THOUSAND)
	{
		Divider=1000;
		IndexDiv=0;
	}
	else
	{
		Divider=1024;
		IndexDiv=1;
	}

	uint64_t Sz = Size, Divider2 = Divider/2, Divider64 = Divider, OldSize;

	if (FloatSize)
	{
		uint64_t Divider64F = 1, Divider64F_mul = 1000, Divider64F2 = 1, Divider64F2_mul = Divider;

		//выравнивание идёт по 1000 но само деление происходит на Divider
		//например 999 bytes покажутся как 999 а вот 1000 bytes уже покажутся как 0.97 K
		for (IndexB=0; IndexB<UNIT_COUNT-1; IndexB++)
		{
			if (Sz < Divider64F*Divider64F_mul)
				break;

			Divider64F = Divider64F*Divider64F_mul;
			Divider64F2  = Divider64F2*Divider64F2_mul;
		}

		if (!IndexB)
			strStr.Format(L"%d", (DWORD)Sz);
		else
		{
			Sz = (OldSize=Sz) / Divider64F2;
			OldSize = (OldSize % Divider64F2) / (Divider64F2 / Divider64F2_mul);
			DWORD Decimal = (DWORD)(0.5+(double)(DWORD)OldSize/(double)Divider*100.0);

			if (Decimal >= 100)
			{
				Decimal -= 100;
				Sz++;
			}

			strStr.Format(L"%d.%02d", (DWORD)Sz,Decimal);
			FormatNumber(strStr, strStr, (Economic && Sz > 9) ? 1 : 2);
		}

		if (IndexB>0 || ShowBytesIndex)
		{
			Width-=(Economic?1:2);

			if (Width<0)
				Width=strStr.GetLength();

			if (Economic)
				strDestStr.Format(L"%*.*ls%1.1ls",Width,Width,strStr.CPtr(),UnitStr[IndexB][IndexDiv]);
			else
				strDestStr.Format(L"%*.*ls %1.1ls",Width,Width,strStr.CPtr(),UnitStr[IndexB][IndexDiv]);
		}
		else
			strDestStr.Format(L"%*.*ls",Width,Width,strStr.CPtr());

		return strDestStr;

	}

	if (Commas)
		InsertCommas(Sz,strStr);
	else
		strStr.Format(L"%llu", Sz);

	if ((!UseMinSizeIndex && strStr.GetLength()<=static_cast<size_t>(Width)) || Width<5)
	{
		if (ShowBytesIndex)
		{
			Width-=(Economic?1:2);

			if (Width<0)
				Width=strStr.GetLength();

			if (Economic)
				strDestStr.Format(L"%*.*ls%1.1ls",Width,Width,strStr.CPtr(),UnitStr[0][IndexDiv]);
			else
				strDestStr.Format(L"%*.*ls %1.1ls",Width,Width,strStr.CPtr(),UnitStr[0][IndexDiv]);
		}
		else
			strDestStr.Format(L"%*.*ls",Width,Width,strStr.CPtr());
	}
	else
	{
		Width-=(Economic?1:2);
		IndexB=0;

		do
		{
			//Sz=(Sz+Divider2)/Divider64;
			Sz = (OldSize=Sz) / Divider64;

			if ((OldSize % Divider64) > Divider2)
				++Sz;

			IndexB++;

			if (Commas)
				InsertCommas(Sz,strStr);
			else
				strStr.Format(L"%llu",Sz);
		}
		while ((UseMinSizeIndex && IndexB<MinSizeIndex) || strStr.GetLength() > static_cast<size_t>(Width));

		if (Economic)
			strDestStr.Format(L"%*.*ls%1.1ls",Width,Width,strStr.CPtr(),UnitStr[IndexB][IndexDiv]);
		else
			strDestStr.Format(L"%*.*ls %1.1ls",Width,Width,strStr.CPtr(),UnitStr[IndexB][IndexDiv]);
	}

	return strDestStr;
}



// вставить с позиции Pos в Str строку InsStr (размером InsSize байт)
// если InsSize = 0, то... вставлять все строку InsStr
// возвращает указатель на Str

wchar_t *InsertString(wchar_t *Str,int Pos,const wchar_t *InsStr,int InsSize)
{
	int InsLen=StrLength(InsStr);

	if (InsSize && InsSize < InsLen)
		InsLen=InsSize;

	wmemmove(Str+Pos+InsLen, Str+Pos, (StrLength(Str+Pos)+1));
	wmemcpy(Str+Pos, InsStr, InsLen);
	return Str;
}


// Заменить в строке Str Count вхождений подстроки FindStr на подстроку ReplStr
// Если Count < 0 - заменять "до полной победы"
// Return - количество замен
int ReplaceStrings(FARString &strStr,const wchar_t *FindStr,const wchar_t *ReplStr,int Count,bool IgnoreCase)
{
	if (!Count)
		return 0;
	const int LenFindStr = StrLength(FindStr);
	if ( !LenFindStr)
		return 0;
	const int LenReplStr = StrLength(ReplStr);

	int ReplacedCount = 0;
	FARString strResult;
	size_t StartPos = 0, FoundPos;
	while ( (IgnoreCase ? strStr.PosI(FoundPos, FindStr, StartPos) : strStr.Pos(FoundPos, FindStr, StartPos)) && (Count == -1 || ReplacedCount < Count))
	{
		strResult.Append(strStr.CPtr() + StartPos, FoundPos - StartPos);
		strResult.Append(ReplStr, LenReplStr);
		StartPos = FoundPos + LenFindStr;
		++ReplacedCount;
	}
	if (ReplacedCount)
	{
		 if (StartPos < strStr.GetLength())
			strResult.Append(strStr.CPtr() + StartPos, strStr.GetLength() - StartPos);
		strStr = std::move(strResult);
	}
	return ReplacedCount;
}

FARString &ReplaceNulls(FARString &Str, wchar_t Char)
{
	size_t Len = Str.GetLength();
	wchar_t *Buf = Str.GetBuffer();
	for (size_t I = 0; I < Len; I++) {
		if (Buf[I] == 0)
			Buf[I] = Char;
	}
	Str.ReleaseBuffer();
	return Str;
}

/*
From PHP 4.x.x
Форматирует исходный текст по заданной ширине, используя
разделительную строку. Возвращает строку SrcText свёрнутую
в колонке, заданной параметром Width. Строка рубится при
помощи строки Break.

Разбивает на строки с выравниваением влево.

Если параметр Flags & FFTM_BREAKLONGWORD, то строка всегда
сворачивается по заданной ширине. Так если у вас есть слово,
которое больше заданной ширины, то оно будет разрезано на части.

Example 1.
FarFormatText("Пример строки, которая будет разбита на несколько строк по ширине в 20 символов.", 20 ,Dest, "\n", 0);
Этот пример вернет:
---
Пример строки,
которая будет
разбита на
несколько строк по
ширине в 20
символов.
---

Example 2.
FarFormatText( "Эта строка содержит оооооооооооооччччччччеееень длиное слово", 9, Dest, nullptr, FFTM_BREAKLONGWORD);
Этот пример вернет:

---
Эта
строка
содержит
ооооооооо
ооооччччч
чччеееень
длиное
слово
---

*/

FARString& WINAPI FarFormatText(const wchar_t *SrcText,     // источник
                             int Width,               // заданная ширина
                             FARString &strDestText,          // приемник
                             const wchar_t* Break,       // брик, если = nullptr, то принимается '\n'
                             DWORD Flags)             // один из FFTM_*
{
	const wchar_t *breakchar;
	breakchar = Break?Break:L"\n";

	if (!SrcText || !*SrcText)
	{
		strDestText.Clear();
		return strDestText;
	}

	FARString strSrc = SrcText; //copy FARString in case of SrcText == strDestText

	if (!strSrc.ContainsAnyOf(breakchar) && strSrc.GetLength() <= static_cast<size_t>(Width))
	{
		strDestText = strSrc;
		return strDestText;
	}

	long i=0, l=0, pgr=0, last=0;
	wchar_t *newtext;
	const wchar_t *text= strSrc;
	long linelength = Width;
	int breakcharlen = StrLength(breakchar);
	int docut = Flags&FFTM_BREAKLONGWORD?1:0;
	/* Special case for a single-character break as it needs no
	   additional storage space */

	if (breakcharlen == 1 && !docut)
	{
		newtext = wcsdup(text);

		if (!newtext)
		{
			strDestText.Clear();
			return strDestText;
		}

		while (newtext[i] != L'\0')
		{
			/* prescan line to see if it is greater than linelength */
			l = 0;

			while (newtext[i+l] != breakchar[0])
			{
				if (newtext[i+l] == L'\0')
				{
					l--;
					break;
				}

				l++;
			}

			if (l >= linelength)
			{
				pgr = l;
				l = linelength;

				/* needs breaking; work backwards to find previous word */
				while (l >= 0)
				{
					if (newtext[i+l] == L' ')
					{
						newtext[i+l] = breakchar[0];
						break;
					}

					l--;
				}

				if (l == -1)
				{
					/* couldn't break is backwards, try looking forwards */
					l = linelength;

					while (l <= pgr)
					{
						if (newtext[i+l] == L' ')
						{
							newtext[i+l] = breakchar[0];
							break;
						}

						l++;
					}
				}
			}

			i += l+1;
		}
	}
	else
	{
		/* Multiple character line break */
		newtext = (wchar_t*)malloc((strSrc.GetLength() * (breakcharlen+1)+1)*sizeof(wchar_t));

		if (!newtext)
		{
			strDestText.Clear();
			return strDestText;
		}

		newtext[0] = L'\0';
		i = 0;

		while (text[i] != L'\0')
		{
			/* prescan line to see if it is greater than linelength */
			l = 0;

			while (text[i+l] != L'\0')
			{
				if (text[i+l] == breakchar[0])
				{
					if (breakcharlen == 1 || !StrCmpN(text+i+l, breakchar, breakcharlen))
						break;
				}

				l++;
			}

			if (l >= linelength)
			{
				pgr = l;
				l = linelength;

				/* needs breaking; work backwards to find previous word */
				while (l >= 0)
				{
					if (text[i+l] == L' ')
					{
						wcsncat(newtext, text+last, i+l-last);
						wcscat(newtext, breakchar);
						last = i + l + 1;
						break;
					}

					l--;
				}

				if (l == -1)
				{
					/* couldn't break it backwards, try looking forwards */
					l = linelength - 1;

					while (l <= pgr)
					{
						if (!docut)
						{
							if (text[i+l] == L' ')
							{
								wcsncat(newtext, text+last, i+l-last);
								wcscat(newtext, breakchar);
								last = i + l + 1;
								break;
							}
						}

						if (docut == 1)
						{
							if (text[i+l] == L' ' || l > i-last)
							{
								wcsncat(newtext, text+last, i+l-last+1);
								wcscat(newtext, breakchar);
								last = i + l + 1;
								break;
							}
						}

						l++;
					}
				}

				i += l+1;
			}
			else
			{
				i += (l ? l : 1);
			}
		}

		if (i+l > last)
		{
			wcscat(newtext, text+last);
		}
	}

	strDestText = newtext;
	free(newtext);
	return strDestText;
}

/*
  Ptr=CalcWordFromString(Str,I,&Start,&End);
  far_strncpy(Dest,Ptr,End-Start+1);
  Dest[End-Start+1]=0;

// Параметры:
//   WordDiv  - набор разделителей слова в кодировке OEM
  возвращает указатель на начало слова
*/

const wchar_t * CalcWordFromString(const wchar_t *Str,int CurPos,int *Start,int *End, const wchar_t *WordDiv0)
{
	int StartWPos, EndWPos;

	const int StrSize = StrLength(Str);

	if (CurPos < 0 || CurPos > StrSize)
		return nullptr;

	if (IsWordDivSTNR(WordDiv0, Str[CurPos]))
	{
		// вычисляем дистанцию - куда копать, где ближе слово - слева или справа
		int L, R;

		// копаем влево
		for (L = CurPos - 1; (L >= 0 && IsWordDivSTNR(WordDiv0, Str[L])); --L);

		// копаем вправо
		for (R = CurPos + 1; (R < StrSize && IsWordDivSTNR(WordDiv0, Str[R])); ++R);

		if ( L < 0) {
			if (R >= StrSize)
				return nullptr;

			StartWPos = EndWPos = R;

		} else if (R >= StrSize) {
			StartWPos = EndWPos = L;

		} else if (CurPos - L > R - CurPos) { // ?? >=
			EndWPos = StartWPos = R;

		} else {
			StartWPos = EndWPos = L;
		}

	} else {// здесь все оби, т.е. стоим на буковке
		EndWPos = StartWPos = CurPos;
	}

	for ( ; (StartWPos > 0 && !IsWordDivSTNR(WordDiv0, Str[StartWPos - 1])); --StartWPos);

	for ( ; (EndWPos + 1 < StrSize && !IsWordDivSTNR(WordDiv0, Str[EndWPos + 1])); ++EndWPos);


	*Start = StartWPos;
	*End = EndWPos;

	return Str + StartWPos;
}


bool CheckFileSizeStringFormat(const wchar_t *FileSizeStr)
{
//проверяет если формат строки такой: [0-9]+[BbKkMmGgTtPpEe]?
	const wchar_t *p = FileSizeStr;

	while (iswdigit(*p))
		p++;

	if (p == FileSizeStr)
		return false;

	if (*p)
	{
		if (*(p+1))
			return false;

		if (!StrStrI(L"BKMGTPE", p))
			return false;
	}

	return true;
}

uint64_t ConvertFileSizeString(const wchar_t *FileSizeStr)
{
	if (!CheckFileSizeStringFormat(FileSizeStr))
		return 0;

	uint64_t n = _wtoi64(FileSizeStr);
	wchar_t c = Upper(FileSizeStr[StrLength(FileSizeStr)-1]);

	// http://en.wikipedia.org/wiki/SI_prefix
	switch (c)
	{
		case L'K':		// kilo 10x3
			n <<= 10;
			break;
		case L'M':		// mega 10x6
			n <<= 20;
			break;
		case L'G':		// giga 10x9
			n <<= 30;
			break;
		case L'T':		// tera 10x12
			n <<= 40;
			break;
		case L'P':		// peta 10x15
			n <<= 50;
			break;
		case L'E':		// exa  10x18
			n <<= 60;
			break;
			// Z - zetta 10x21
			// Y - yotta 10x24
	}

	return n;
}

/* $ 21.09.2003 KM
   Трансформация строки по заданному типу.
*/
void Transform(FARString &strBuffer,const wchar_t *ConvStr,wchar_t TransformType)
{
	FARString strTemp;

	switch (TransformType)
	{
		case L'X': // Convert common FARString to hexadecimal FARString representation
		{
			FARString strHex;

			while (*ConvStr)
			{
				strHex.Format(L"%02X",*ConvStr);
				strTemp += strHex;
				ConvStr++;
			}

			break;
		}
		case L'S': // Convert hexadecimal FARString representation to common string
		{
			const wchar_t *ptrConvStr=ConvStr;

			while (*ptrConvStr)
			{
				if (*ptrConvStr != L' ')
				{
					WCHAR Hex[]={ptrConvStr[0],ptrConvStr[1],0};
					size_t l=strTemp.GetLength();
					wchar_t *Temp=strTemp.GetBuffer(l+2);
					Temp[l]=(wchar_t)wcstoul(Hex,nullptr,16) & ((wchar_t)-1);
					strTemp.ReleaseBuffer(l+1);
					ptrConvStr++;
				}

				ptrConvStr++;
			}

			break;
		}
		default:
			break;
	}

	strBuffer=strTemp;
}

wchar_t GetDecimalSeparator()
{
	//wchar_t Separator[4];
	//GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SDECIMAL,Separator,ARRAYSIZE(Separator));
	//return *Separator;
	//return L'.';
	return Opt.strDecimalSeparator.IsEmpty() ? GetDecimalSeparatorDefault() : Opt.strDecimalSeparator.At(0);
}


static FARString ReplaceBrackets(
		const wchar_t* SearchStr,
		const FARString& ReplaceStr,
		const regex_match& Match,
		std::unordered_map<std::wstring, size_t> const& NamedGroups)
{
	enum ST {
		ST_COMMON, ST_HEXCHAR, ST_ORDERED_1, ST_ORDERED_2, ST_NAMED,
	};
	ST State = ST_COMMON;
	FARString strResult, strPart;
	const size_t length = ReplaceStr.GetLength();
	const wchar_t *Arr = ReplaceStr;
	const wchar_t *start = Arr;
	size_t index = 0, count = 0;
	wchar_t hexChar;
	std::vector<wchar_t> stack;
	stack.reserve(256);

	for (size_t pos=0; pos <= length; ) // use '<=' to process all within the loop
	{
		strPart.Clear();
		const wchar_t *p = Arr + pos;

		switch(State)
		{
			case ST_COMMON:
				start = p; // all other states are switched to from this state

				if (pos == length) // end of input
				{
					pos++;
				}
				else if (p[0] == L'\\')
				{
					if (p[1] == L'L' || p[1] == L'U' || p[1] == L'l'  || p[1] == L'u')
					{
						stack.push_back(p[1]);
						pos += 2;
					}
					else if (p[1] == L'E')
					{
						if (!stack.empty()) stack.pop_back();
						pos += 2;
					}
					else if (Upper(p[1]) == L'X' && (hexChar = ParseHexDigit(p[2])) != 0xFF)
					{
						State = ST_HEXCHAR; // process hexadecimal codes, e.g. \x7ABC, \x00, etc.
						count = 1;
						pos += 3;
					}
					else if (pos + 1 < length)
					{
						switch(p[1])
						{
							case L'r' :
							case L'n' : strPart = L'\r'; break;
							case L't' : strPart = L'\t'; break;
							case L'\\': strPart = L'\\'; break;
							case L'$' : strPart = L'$' ; break;
							default   : strPart = L'\\'; strPart = p[1]; break;
						}
						pos += 2;
					}
					else
					{
						strPart = L'\\';
						pos++;
					}
				}
				else if (p[0] == L'$' && p[1] >= L'0' && p[1] <= L'9')
				{
					State = ST_ORDERED_1;
					index = p[1] - L'0';
					pos += 2;
				}
				else if (p[0] == L'$' && p[1] == L'{' && p[2] >= L'0' && p[2] <= L'9')
				{
					State = ST_ORDERED_2;
					index = p[2] - L'0';
					pos += 3;
				}
				else if (p[0] == L'$' && p[1] == L'{' && (p[2] == L'_' || IsAlpha(p[2])))
				{
					State = ST_NAMED;
					pos += 2;
				}
				else
				{
					strPart = p[0];
					pos++;
				}
				break;

			case ST_HEXCHAR:
				if (auto uc = ParseHexDigit(p[0]); uc != 0xFF)
				{
					pos++;
					hexChar = hexChar*16 + uc;
					if (++count == 4) // 4 hex chars at most
					{
						strPart = hexChar;
						State = ST_COMMON;
					}
				}
				else
				{
					strPart = hexChar;
					State = ST_COMMON;
				}
				break;

			case ST_ORDERED_1:
			case ST_ORDERED_2:
				if (p[0] >= L'0' && p[0] <= L'9')
				{
					index = 10*index + (p[0] - L'0');
					pos++;
				}
				else if (State == ST_ORDERED_2 && p[0] != L'}') // alien char; insert the "buffer" as is;
				{                                               // don't increment 'pos'
					strPart = FARString(start, p - start);
					State = ST_COMMON;
				}
				else // valid group syntax found
				{
					if (index < Match.Matches.size())
					{
						const auto& match = Match.Matches[index];
						strPart = FARString(SearchStr + match.start, match.end - match.start);
					}
					else // invalid index; insert the "buffer" as is
					{
						auto end = (State == ST_ORDERED_1) ? p : p + 1;
						strPart = FARString(start, end - start);
					}
					pos += (State == ST_ORDERED_1) ? 0 : 1;
					State = ST_COMMON;
				}
				break;

			case ST_NAMED:
				if (p[0] == L'_' || IsAlphaNum(p[0]))
					pos++;
				else if (p[0] == L'}') // valid named group syntax found
				{
					std::wstring Name(start+2, p - (start+2));
					const auto found = NamedGroups.find(Name);
					if (found != NamedGroups.end())
					{
						const auto idx = found->second;
						if (idx < Match.Matches.size())
						{
							const auto& match = Match.Matches[idx];
							strPart = FARString(SearchStr + match.start, match.end - match.start);
						}
					}
					else //insert named group "as is"
					{
						strPart = FARString(start, p - start);
					}
					State = ST_COMMON;
					pos++;
				}
				else // named group not closed; don't increment 'pos'
				{
					strPart = FARString(start, (p-1) - start);
					State = ST_COMMON;
				}
				break;
		}

		// remove all \l and \u from stack top
		while(!strPart.Empty() && !stack.empty())
		{
			auto Ch = stack.back();
			if (Ch == L'l' || Ch == L'u')
			{
				stack.pop_back();
				strResult += (Ch == L'l' ? Lower(strPart[0]) : Upper(strPart[0]));
				strPart = strPart.SubStr(1);
			}
			else
				break;
		}

		// process \L and \U
		if (!strPart.Empty())
		{
			if (!stack.empty())
			{
				switch(stack.back())
				{
					case L'L': strPart.Lower(); break;
					case L'U': strPart.Upper(); break;
				}
			}
			strResult += strPart;
		}
	}

	return strResult;
}


std::string EscapeUnprintable(const std::string &str)
{
	std::string out;
	out.reserve(str.size());
	for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
		unsigned char c = (unsigned char)*i;
		if (c <= 0x20 || c > 0x7e || c=='\\') {
			char buf[32];
			snprintf(buf, sizeof(buf), "\\x%02x", c);
			out+= buf;
		} else
			out+= c;
	}
	return out;
}

std::string UnescapeUnprintable(const std::string &str)
{
	std::string out;
	out.reserve(str.size());
	for (size_t i = 0; i < str.size(); ++i) {
		char c = str[i];
		if (c == '\\' && (i + 3) < str.size() && str[i+1] == 'x') {
			char tmp[4] = {str[i+2], str[i+3]};
			unsigned int x = 0;
			sscanf(tmp, "%x", &x);
			c = (unsigned char)x;
			i+= 3;
		}
		out+= c;
	}
	return out;
}

bool SearchString(const wchar_t *Source, int StrSize, const FARString& Str, FARString& ReplaceStr,
		int& CurPos, int Position, int Case, int WholeWords, int Reverse, RegExp *Re, int& SearchLength,
		const wchar_t* WordDiv)
{
	SearchLength = 0;

	if (!WordDiv)
		WordDiv=Opt.strWordDiv;

	if (Reverse)
	{
		Position = Min(Position, StrSize);

		if (Position < 0)
			return false;
	}

	if ((Position <= StrSize) && !Str.IsEmpty())
	{
		if (Re)
		{
			regex_match rmatch;

			bool found = false;
			if (!Reverse)
			{
				if (Re->SearchEx(ReStringView(Source, StrSize),Position,rmatch))
					found = true;
			}
			else
			{
				auto View = ReStringView(Source, Min(Position + 1, StrSize));
				for (int pos=Position; pos >= 0; --pos)
				{
					if (Re->SearchEx(View, pos, rmatch))
					{
						if (rmatch.Matches[0].start <= Position)
						{
							found = true;
							break;
						}
					}
				}
			}
			if (found)
			{
				SearchLength = rmatch.Matches[0].end - rmatch.Matches[0].start;
				CurPos = rmatch.Matches[0].start;
				ReplaceStr = ReplaceBrackets(Source, ReplaceStr, rmatch, Re->GetNamedGroups());
			}

			return found;
		}

		int Length = SearchLength = (int)Str.GetLength();

		for (int I=Position; (Reverse && I>=0) || (!Reverse && I<StrSize); Reverse ? I--:I++)
		{
			for (int J=0;; J++)
			{
				if (!Str[J])
				{
					CurPos = I;
					return true;
				}

				if (WholeWords)
				{
					if (I > 0)
					{
						wchar_t ChLeft = Source[I-1];
						if (!IsSpace(ChLeft) && !wcschr(WordDiv,ChLeft))
							break;
					}
					if (I+Length < StrSize)
					{
						wchar_t ChRight = Source[I+Length];
						if (!IsSpace(ChRight) && !wcschr(WordDiv,ChRight))
							break;
					}
				}

				wchar_t Ch = Source[I+J];
				if ((Case && Ch != Str[J]) || (!Case && Upper(Ch) != Upper(Str[J])))
					break;
			}
		}
	}

	return false;
}

bool CompileRegexp(const wchar_t *Str, int CaseSensitive, RegExp *Re)
{
	FARString strSlash(Str);
	InsertRegexpQuote(strSlash);
	return Re->Compile(strSlash, OP_PERLSTYLE | OP_OPTIMIZE | (CaseSensitive ? 0 : OP_IGNORECASE));
}
