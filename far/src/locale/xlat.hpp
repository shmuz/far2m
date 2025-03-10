#pragma once

/*
xlat.hpp

XLat - перекодировка
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

#include <WinCompat.h>
#include <KeyFileHelper.h>
#include <string>
#include <vector>

class Xlator
{
	std::wstring _latin, _local;
	struct Rules : std::vector<std::pair<wchar_t, wchar_t>>
	{
		void InitFromValue(const std::wstring &v);

	} _after_latin, _after_local, _after_other;

	size_t _min_len_table{0};
	enum {
		UNKNOWN,
		LATIN,
		LOCAL,
	} _cur_lang {UNKNOWN};

	void InitFromValues(KeyFileValues &kfv);

public:
	Xlator(DWORD flags);
	bool Valid() const { return _min_len_table != 0; }
	wchar_t Transcode(wchar_t chr);
};

wchar_t* WINAPI Xlat(wchar_t *Line, int StartPos, int EndPos, DWORD Flags);
bool Xlat(std::wstring &Target, const wchar_t *Line, DWORD Flags);
