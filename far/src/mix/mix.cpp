/*
mix.cpp

Куча разных вспомогательных функций
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

#include "mix.hpp"
#include "CFileMask.hpp"
#include "scantree.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "InterThreadCall.hpp"

int ToPercent(uint32_t N1, uint32_t N2)
{
	if (N1 > 10000) {
		N1/= 100;
		N2/= 100;
	}

	if (!N2)
		return 0;

	if (N2 < N1)
		return (100);

	return ((int)(N1 * 100 / N2));
}

int ToPercent64(uint64_t N1, uint64_t N2)
{
	if (N1 > 10000) {
		N1/= 100;
		N2/= 100;
	}

	if (!N2)
		return 0;

	if (N2 < N1)
		return 100;

	return static_cast<int>(N1 * 100 / N2);
}

 /* $ 30.07.2001 IS
	 1. Проверяем правильность параметров.
	 2. Теперь обработка каталогов не зависит от маски файлов
	 3. Маска может быть стандартного фаровского вида (со скобками,
		перечислением и пр.). Может быть несколько масок файлов, разделенных
		запятыми или точкой с запятой, можно указывать маски исключения,
		можно заключать маски в кавычки. Короче, все как и должно быть :-)
*/
void WINAPI
FarRecursiveSearch(const wchar_t *InitDir, const wchar_t *Mask, FRSUSERFUNC Func, DWORD Flags, void *Param)
{
	if (Func && InitDir && *InitDir && Mask && *Mask) {
		SudoClientRegion scr;
		// SudoSilentQueryRegion ssqr;
		CFileMask FMask;

		if (!FMask.Set(Mask, FMF_SILENT))
			return;

		bool CaseSensMask = (Flags & FRS_CASESENSMASK) != 0;
		ScanTree ScTree(Flags & FRS_RETUPDIR, Flags & FRS_RECUR, Flags & FRS_SCANSYMLINK);
		FAR_FIND_DATA_EX FindData;
		FARString strFullName;
		ScTree.SetFindPath(InitDir, L"*");

		while (ScTree.GetNextName(&FindData, strFullName)) {
			if (FMask.Compare(FindData.strFileName, CaseSensMask)) {
				FAR_FIND_DATA fdata;
				apiFindDataExToData(&FindData, &fdata);

				if (!Func(&fdata, strFullName, Param)) {
					apiFreeFindData(&fdata);
					break;
				}

				apiFreeFindData(&fdata);
			}
		}
	}
}

/* $ 14.09.2000 SVS
 + Функция FarMkTemp - получение имени временного файла с полным путем.
	Dest - приемник результата
	Template - шаблон по правилам функции mktemp, например "FarTmpXXXXXX"
	Вернет требуемый размер приемника.
*/
int WINAPI FarMkTemp(wchar_t *Dest, DWORD size, const wchar_t *Prefix)
{
	FARString strDest;
	if (FarMkTempEx(strDest, Prefix, TRUE) && Dest && size) {
		far_wcsncpy(Dest, strDest, size);
	}
	return static_cast<int>(strDest.GetLength() + 1);
}

/*
			 v - точка
   prefXXX X X XXX
	   \ / ^   ^^^\ PID + TID
		|  \------/
		|
		+---------- [0A-Z]
*/
FARString &
FarMkTempEx(FARString &strDest, const wchar_t *Prefix, BOOL WithTempPath, const wchar_t *UserTempPath)
{
	FARString strPath = L".";
	if (WithTempPath) {
		apiGetTempPath(strPath);
	} else if (UserTempPath) {
		strPath = UserTempPath;
	}

	strDest.Clear();

	AddEndSlash(strPath);
	strPath+= (Prefix && *Prefix) ? Prefix : L"FTMP";

	const size_t BasePathLen = strPath.GetLength();
	for (unsigned int uniq = (GetInterThreadID() << 4) ^ RevBytes(WINPORT(GetCurrentProcessId)()), wraps = 0;; ++uniq) {
		strPath.AppendFormat(L"%x.tmp", uniq);
		if (apiGetFileAttributes(strPath) == INVALID_FILE_ATTRIBUTES) {
			strDest = std::move(strPath);
			break;
		}
		strPath.Truncate(BasePathLen);
		if (uniq == 0) {
			if (++wraps == 4) {
				fprintf(stderr, "%s: gave up - '%ls'\n", __FUNCTION__, strPath.CPtr());
				break;
			}
			fprintf(stderr, "%s: wrap around - '%ls'\n", __FUNCTION__, strPath.CPtr());
			usleep(10000);
		}
	}
	// fprintf(stderr, "%s: '%ls'\n", __FUNCTION__, strDest.CPtr());
	return strDest;
}

void DisplayNotification(const wchar_t *action, const char *object)
{
	if (!Opt.NotifOpt.OnlyIfBackground || !WINPORT(IsConsoleActive)()) {
		WINPORT(ConsoleDisplayNotification)(action, MB2Wide(object).c_str());
	}
}

void DisplayNotification(const char *action, const char *object)
{
	if (!Opt.NotifOpt.OnlyIfBackground || !WINPORT(IsConsoleActive)()) {
		DisplayNotification(MB2Wide(action).c_str(), object);
	}
}

void DisplayNotification(const wchar_t *action, const wchar_t *object)
{
	if (!Opt.NotifOpt.OnlyIfBackground || !WINPORT(IsConsoleActive)()) {
		DisplayNotification(action, Wide2MB(object).c_str());
	}
}
