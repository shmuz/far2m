#pragma once

/*
tvar.hpp

Реализация класса TVar ("кастрированый" вариант - только целое и строковое значение)
(для макросов)

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

//---------------------------------------------------------------
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.
//---------------------------------------------------------------

#include <WinCompat.h>
#include "locale.hpp"

enum TVarType
{
	vtUnknown = -1,
	vtInteger = 0,
	vtString  = 1,
	vtDouble  = 2,
};

typedef int (*TVarFuncCmp)(TVarType vt,const void *, const void *);

class TVar
{
	private:
		TVarType vType;
		int64_t inum;
		double  dnum;
		wchar_t *str;

	private:
		static int CompAB(const TVar& a, const TVar& b, TVarFuncCmp fcmp);

	public:
		TVar();
		TVar(int64_t);
		TVar(const wchar_t*);
		TVar(int);
		TVar(double);
		TVar(const TVar&);
		~TVar();

	public:
		TVar& operator=(const TVar&);
		TVar& operator=(const int&);
		TVar& operator=(const int64_t&);
		TVar& operator=(const double&);

		TVar operator+();
		TVar operator-();
		TVar operator!();
		TVar operator~();

		friend int operator==(const TVar&, const TVar&);
		friend int operator!=(const TVar&, const TVar&);
		friend int operator<(const TVar&, const TVar&);
		friend int operator<=(const TVar&, const TVar&);
		friend int operator>(const TVar&, const TVar&);
		friend int operator>=(const TVar&, const TVar&);

		TVarType type() const { return vType; }

		int isString()   const { return vType == vtString;  }
		int isInteger()  const { return vType == vtInteger; }
		int isDouble()   const { return vType == vtDouble;  }
		int isUnknown()  const { return vType == vtUnknown;  }

		double d()         const;
		int64_t i()        const;
		const wchar_t *s() const;

		const wchar_t *toString();
		double toDouble();
		int64_t toInteger();

		int64_t getInteger() const;
		int32_t getInt32() const;
		double getDouble() const;

		double asDouble() const;
		int64_t asInteger() const;
};
