/*
dirmix.cpp

Misc functions for working with directories
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


#include "dirmix.hpp"
#include "cvtname.hpp"
#include "message.hpp"
#include "lang.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "treelist.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "scantree.hpp"
#include "delete.hpp"
#include <atomic>
#include <signal.h>


bool FarChDir(const wchar_t *NewDir)
{
	if (!NewDir || !*NewDir)
		return false;

	bool rc = false;
	FARString strCurDir;

	// remove redundant slashes
	wchar_t *Str = wcsdup(NewDir), *q=Str;
	for (auto p = NewDir; *p; ) {
		*q++ = *p++;
		if (*(p-1)==L'/') while (*p==L'/') p++;
	}
	*q = 0;
	NewDir = Str;

	if (*NewDir=='/') {
		strCurDir = NewDir;
		rc = apiSetCurrentDirectory(strCurDir); // здесь берем корень
	} else {
		apiGetCurrentDirectory(strCurDir);
		ConvertNameToFull(NewDir,strCurDir);
		rc = apiSetCurrentDirectory(strCurDir);
	}
	if (!rc) {
		fprintf(stderr, "FarChDir: FAILED - '%ls'\n", NewDir);
	}
	free(Str);

	return rc;
}

/*
  Функция TestFolder возвращает одно состояний тестируемого каталога:

    TSTFLD_NOTFOUND   (2) - нет такого
    TSTFLD_NOTEMPTY   (1) - не пусто
    TSTFLD_EMPTY      (0) - пусто
    TSTFLD_NOTACCESS (-1) - нет доступа
    TSTFLD_ERROR     (-2) - ошибка (кривые параметры или нехватило памяти для выделения промежуточных буферов)
*/
TESTFOLDERCONST TestFolder(const wchar_t *Path)
{
	if (!(Path && *Path)) // проверка на вшивость
		return TSTFLD_ERROR;

	std::string mbPath;
	Wide2MB(Path, mbPath);
	while (mbPath.size() > 1 && mbPath.back() == '/')
		mbPath.resize(mbPath.size() - 1);

	struct stat s{};
	int r = sdc_stat(mbPath.c_str(), &s);
	if (r == -1)
	{
		if (errno == EPERM || errno == EACCES)
			return TSTFLD_NOTACCESS;

		return TSTFLD_NOTFOUND;
	}

	if (!S_ISDIR(s.st_mode)) // not directories are always empty
		return TSTFLD_EMPTY;

	DIR *d = sdc_opendir(mbPath.c_str());
	if (!d) switch (errno)
	{
		case EPERM: case EACCES:
			return TSTFLD_NOTACCESS;

		case ENOMEM: case EMFILE: case ENFILE: case EBADF:
			return TSTFLD_ERROR;

		default:
			return TSTFLD_EMPTY;
	}

	TESTFOLDERCONST out = TSTFLD_EMPTY;
	for (;;)
	{
		struct dirent *de = sdc_readdir(d);
		if (!de) break;
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, ".."))
		{
			out = TSTFLD_NOTEMPTY;
			break;
		}
	}
	sdc_closedir(d);

	return out;
}

/*
   Проверка пути или хост-файла на существование
   Если идет проверка пути (IsHostFile=FALSE), то будет
   предпринята попытка найти ближайший путь. Результат попытки
   возвращается в переданном TestPath.

   Return: 0 - бЯда.
           1 - ОБИ!,
          -1 - Почти что ОБИ, но ProcessPluginEvent вернул TRUE
   TestPath может быть пустым, тогда просто исполним ProcessPluginEvent()

*/
int CheckShortcutFolder(FARString &pTestPath, bool IsHostFile, bool Silent)
{
	if (!pTestPath.IsEmpty() && apiGetFileAttributes(pTestPath) == INVALID_FILE_ATTRIBUTES)
	{
		bool FoundPath = false;
		FARString strTarget = pTestPath;
		TruncPathStr(strTarget, ScrX-16);

		if (IsHostFile)
		{
			WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);

			if (!Silent)
				Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, strTarget, Msg::Ok);
		}
		else // попытка найти!
		{
			WINPORT(SetLastError)(ERROR_PATH_NOT_FOUND);

			if (Silent || !Message(MSG_WARNING | MSG_ERRORTYPE, 2, Msg::Error, strTarget, Msg::NeedNearPath, Msg::HYes,Msg::HNo))
			{
				FARString strTemp = pTestPath;

				for (; CutToSlash(strTemp, false); CutToSlash(strTemp, true))
				{
					if (apiGetFileAttributes(strTemp) != INVALID_FILE_ATTRIBUTES)
					{
						int ChkFld=TestFolder(strTemp);

						if (ChkFld == TSTFLD_NOTACCESS || ChkFld == TSTFLD_EMPTY || ChkFld == TSTFLD_NOTEMPTY)
						{
							pTestPath = strTemp;
							FoundPath = true;
							break;
						}
					}
				}
			}
		}

		if (!FoundPath)
			return 0;
	}

	if (CtrlObject->Cp()->ActivePanel->ProcessPluginEvent(FE_CLOSE))
		return -1;

	return 1;
}

void CreatePath(FARString &strPath)
{
	wchar_t *ChPtr = strPath.GetBuffer();
	bool bEnd = false;

	for (;;)
	{
		if (!*ChPtr || IsSlash(*ChPtr))
		{
			if (!*ChPtr)
				bEnd = true;

			*ChPtr = 0;

			if (apiCreateDirectory(strPath, nullptr))
				TreeList::AddTreeName(strPath);

			if (bEnd)
				break;

			*ChPtr = GOOD_SLASH;
		}

		ChPtr++;
	}

	strPath.ReleaseBuffer();
}

std::string GetHelperPathName(const char *name)
{
 	std::string out = g_strFarPath.GetMB();
	if (!out.empty() && out.back() != GOOD_SLASH) {
		out+= GOOD_SLASH;
	}
	out+= name;

	struct stat s;
	if (stat(out.c_str(), &s) == 0)
		return out;

	if (TranslateInstallPath_Share2Lib(out)) {
		if (stat(out.c_str(), &s) == 0)
			return out;
	}

	fprintf(stderr, "GetHelperPathName('%s') - not found\n", name);
	return out;
}

std::string GetMyScriptQuoted(const char *name)
{
	std::string out = "\"";
	out+= EscapeCmdStr(GetHelperPathName(name));
	out+= "\"";
	return out;
}

void CleanupTemporaryOpenPaths(bool clean_ours, bool clean_dead)
{
	std::string open_dir = InMyTemp("open");
	DIR *d = opendir(open_dir.c_str());
	if (!d)
		return;

	const pid_t my_pid = getpid();
	struct dirent *de;
	std::vector<std::string> to_delete;

	while ((de = readdir(d)) != nullptr) {
		if (de->d_name[0] == '.')
			continue;

		unsigned int dir_pid = 0;
		if (sscanf(de->d_name, "%u_", &dir_pid) == 1) {
			if (clean_ours && (pid_t)dir_pid == my_pid) {
				to_delete.push_back(open_dir + "/" + de->d_name);
			} else if (clean_dead && (pid_t)dir_pid != my_pid) {
				// If the owning instance no longer exists, reap it
				if (kill((pid_t)dir_pid, 0) == -1 && errno == ESRCH) {
					to_delete.push_back(open_dir + "/" + de->d_name);
				}
			}
		}
	}
	closedir(d);

	for (const auto &path : to_delete) {
		FARString wPath(path.c_str());
		DeleteDirTree(wPath.CPtr());
	}
}

void PrepareTemporaryOpenPath(FARString &Path)
{
	// Housekeeping: purge leftovers from dead/crashed instances
	CleanupTemporaryOpenPaths(false, true);

	Path = InMyTemp("open");
	apiCreateDirectory(Path, nullptr);

	static std::atomic<unsigned short> s_counter{0};
	const unsigned int cnt = ++s_counter;
	Path.AppendFormat(L"%c%u_%u", GOOD_SLASH, (unsigned int)getpid(), cnt);
	apiCreateDirectory(Path, nullptr);
}

FARString DefaultPanelInitialDirectory()
{
	FARString out;
	const std::string &home = GetMyHome();
	if (!home.empty()) {
		out = home;
	} else {
		out = g_strFarPath;
	}

	DeleteEndSlash(out);
	return out;
}
