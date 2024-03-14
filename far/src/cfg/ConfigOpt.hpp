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

#define REG_BOOLEAN 0x100
#define REG_3STATE  0x101

struct GetConfig {
	int IsSave;
	DWORD Type;
	FARString Key;
	FARString Name;

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
bool ConfigOptGetValue(int Index, GetConfig& Data);
void ConfigOptLoad();
void ConfigOptSave(bool Ask);
bool ConfigOptSetBinary(int Index, const void *Data, DWORD Size);
bool ConfigOptSetInteger(int Index, DWORD Value);
bool ConfigOptSetString(int Index, const wchar_t *Value);
