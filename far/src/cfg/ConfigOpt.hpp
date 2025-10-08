#pragma once

/*
ConfigOpt.hpp

Конфигурация
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

enum OPT_TYPE {
	OPT_3STATE,
	OPT_BINARY,
	OPT_BOOLEAN,
	OPT_DWORD,
	OPT_REALBOOLEAN,
	OPT_SZ,
};

struct GetConfig {
	bool IsSave;
	OPT_TYPE ValType;
	FARString KeyName;
	FARString ValName;

	DWORD dwDefault;
	DWORD dwValue;

	FARString strDefault;
	FARString strValue;

	const void *binDefault;
	const void *binData;
	DWORD binSize;
};

void ConfigOptAssertLoaded();
int  ConfigOptGetIndex(const wchar_t *wKeyName);
bool ConfigOptGetValue(size_t Index, GetConfig& Data);
void ConfigOptLoad();
void ConfigOptSave(bool Ask);
bool ConfigOptSetBinary(size_t Index, const void *Data, DWORD Size);
bool ConfigOptSetInteger(size_t Index, DWORD Value);
bool ConfigOptSetString(size_t Index, const wchar_t *Value);
size_t ConfigOptGetSize();
