/*
cvtname.cpp

Функций для преобразования имен файлов/путей.
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

#include "cvtname.hpp"
#include "cddrv.hpp"
#include "syslog.hpp"
#include "pathmix.hpp"
#include "drivemix.hpp"
#include "strmix.hpp"
#include <errno.h>
#include <set>

#define IsDot(str) (str == L'.')

void MixToFullPath(FARString &strPath)
{
	// Skip all path to root (with slash if exists)
	LPWSTR pstPath = strPath.GetBuffer();
	// size_t PathOffset=0;
	//	Point2Root(pstPath,PathOffset);
	//	pstPath+=PathOffset;

	// Process "." and ".." if exists
	for (int m = 0; pstPath[m];) {
		// fragment "."
		if (IsDot(pstPath[m]) && (!m || IsSlash(pstPath[m - 1]))) {
			LPCWSTR pstSrc;
			LPWSTR pstDst;

			switch (pstPath[m + 1]) {
					// fragment ".\"
				// case L'\\':
				// fragment "./"
				case LGOOD_SLASH: {
					for (pstSrc = pstPath + m + 2, pstDst = pstPath + m; *pstSrc; pstSrc++, pstDst++) {
						*pstDst = *pstSrc;
					}

					*pstDst = 0;
					continue;
				} break;
				// fragment "." at the end
				case 0: {
					pstPath[m] = 0;
					continue;
				} break;
				// fragment "..\" or "../" or ".." at the end
				case L'.': {
					if (IsSlash(pstPath[m + 2]) || !pstPath[m + 2]) {
						int n;

						// Calculate subdir name offset
						for (n = m - 2; (n >= 0) && (!IsSlash(pstPath[n])); n--)
							;

						n = (n < 0) ? 0 : n + 1;

						// fragment "..\" or "../"
						if (pstPath[m + 2]) {
							for (pstSrc = pstPath + m + 3, pstDst = pstPath + n; *pstSrc; pstSrc++,
								pstDst++) {
								*pstDst = *pstSrc;
							}

							*pstDst = 0;
						}
						// fragment ".." at the end
						else if (n > 0) {
							pstPath[n] = 0;
						} else {	// dont go to nowhere
							pstPath[0] = GOOD_SLASH;
							pstPath[1] = 0;
						}

						m = n;
						continue;
					}
				} break;
			}
		}

		m++;
	}
	strPath.ReleaseBuffer();
	if (strPath.GetLength() > 1 && strPath[strPath.GetLength() - 1] == GOOD_SLASH)
		strPath.Truncate(strPath.GetLength() - 1);	// #249
}

bool MixToFullPath(LPCWSTR stPath, FARString &strDest, LPCWSTR stCurrentDir)
{
	if (stPath && *stPath == GOOD_SLASH) {
		strDest = stPath;
		MixToFullPath(strDest);
		return true;
	}

	strDest.Clear();

	if (stCurrentDir && *stCurrentDir) {
		strDest = stCurrentDir;
	}

	if (strDest.IsEmpty()) {
		apiGetCurrentDirectory(strDest);
		if (strDest.IsEmpty()) {	// wtf
			strDest = L"." WGOOD_SLASH;
		}
	}

	if (strDest.At(strDest.GetLength() - 1) != GOOD_SLASH)
		strDest+= GOOD_SLASH;

	if (stPath) {
		while (stPath[0] == '.' && (!stPath[1] || stPath[1] == GOOD_SLASH)) {
			++stPath;
			if (*stPath == GOOD_SLASH)
				++stPath;
		}
		strDest+= stPath;
	}

	MixToFullPath(strDest);
	return true;

	/*
	size_t lPath=wcslen(NullToEmpty(stPath)),
		lCurrentDir=wcslen(NullToEmpty(stCurrentDir)),
		lFullPath=lPath+lCurrentDir;

	if (lFullPath > 0)
	{
		strDest.Clear();
		LPCWSTR pstPath = nullptr, pstCurrentDir = nullptr;
		bool blIgnore = false;
		size_t PathOffset=0;
		PATH_PFX_TYPE PathType=Point2Root(stPath,PathOffset);
		pstPath=stPath+PathOffset;

		switch (PathType)
		{
			case PPT_NONE: //"abc"
			{
				pstCurrentDir=stCurrentDir;
			}
			break;
			case PPT_DRIVE: //"C:" or "C:abc"
			{
				WCHAR DriveVar[]={L'=',*stPath,L':',L'\0'};
				FARString strValue;

				if (apiGetEnvironmentVariable(DriveVar,strValue))
				{
					strDest=strValue;
				}
				else
				{
					if (Upper(*stPath)==Upper(*stCurrentDir))
					{
						strDest=stCurrentDir;
					}
					else
					{
						strDest=DriveVar+1;
					}
				}

				AddEndSlash(strDest);
			}
			break;
			case PPT_ROOT: //"\" or "\abc"
			{
				if (stCurrentDir)
				{
					size_t PathOffset=0;

					if (Point2Root(stCurrentDir,PathOffset)!=PPT_NONE)
					{
						strDest=FARString(stCurrentDir,PathOffset);
					}
				}
			}
			break;
			case PPT_PREFIX: //"C:\abc"
			{
				pstPath=stPath;
			}
			break;
			case PPT_NT: //"\\?\abc"
			{
				blIgnore=true;
				pstPath=stPath;
			}
			break;
		}

		if (pstCurrentDir)
		{
			strDest+=pstCurrentDir;
			AddEndSlash(strDest);
		}

		if (pstPath)
		{
			strDest+=pstPath;
		}

		if (!blIgnore)
			MixToFullPath(strDest);

		return true;
	}

	return false;*/
}

/*
	Преобразует Src в полный РЕАЛЬНЫЙ путь с учетом reparse point.
	Note that Src can be partially non-existent.
*/
void ConvertNameToReal(const wchar_t *Src, FARString &strDest)
{
	char buf[PATH_MAX + 1];
	std::string s = Wide2MB(Src);
	if (*Src == GOOD_SLASH) {
		std::string cutoff;
		for (;;) {
			if (sdc_realpath(s.c_str(), buf)) {
				buf[sizeof(buf) - 1] = 0;
				if (strcmp(buf, s.c_str()) != 0) {
					strDest = buf;
					if (!cutoff.empty())
						strDest.Append(cutoff.c_str());
					return;
				}
				break;
			}

			size_t p = s.rfind(GOOD_SLASH);
			if (p == std::string::npos || p == 0)
				break;
			cutoff.insert(0, s.c_str() + p);
			s.resize(p);
		}
	} else {
		if (sdc_realpath(s.c_str(), buf)) {
			if (strcmp(buf, s.c_str()) != 0) {
				strDest = buf;
				return;
			}
		} else {
			ssize_t r = sdc_readlink(s.c_str(), buf, sizeof(buf) - 1);
			if (r > 0 && r < (ssize_t)sizeof(buf) && buf[0]) {
				buf[r] = 0;
				if (buf[0] != GOOD_SLASH) {
					strDest = s;
					CutToSlash(strDest);
					strDest+= buf;
				} else
					strDest = buf;
				ConvertNameToFull(strDest);
				return;
			}
		}
	}
	strDest = Src;
}

bool ReadSymlink(const wchar_t *lnk, FARString &dest)
{
	char buf[PATH_MAX + 1];
	const auto &lnk_mb = Wide2MB(lnk);
	ssize_t r = sdc_readlink(lnk_mb.c_str(), buf, sizeof(buf) - 1);
	if (r < 0 || r >= (ssize_t)sizeof(buf)) {
		fprintf(stderr, "%s(%ls): error %u\n", __FUNCTION__, lnk, errno);
		return false;
	}
	buf[r] = 0;
	dest = buf;
	return true;
}

void ConvertNameToFull(const wchar_t *lpwszSrc, FARString &strDest)
{
	if (*lpwszSrc != GOOD_SLASH) {
		FARString strCurDir;
		apiGetCurrentDirectory(strCurDir);
		FARString strSrc = lpwszSrc;
		MixToFullPath(strSrc, strDest, strCurDir);
	} else {
		strDest = lpwszSrc;
		MixToFullPath(strDest);
	}
}

void ConvertNameToFull(FARString &strSrcDest)
{
	ConvertNameToFull(strSrcDest, strSrcDest);
}

void ConvertHomePrefixInPath(FARString &strFileName)
{
	if (strFileName.GetLength() > 1 && strFileName[0] == L'~' && strFileName[1] == GOOD_SLASH) {
		strFileName.Replace(0, 1, FARString(GetMyHome()));
	}
}
