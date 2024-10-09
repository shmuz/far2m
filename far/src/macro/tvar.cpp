/*
tvar.cpp

Реализация класса TVar (для макросов)

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

#include "headers.hpp"


#include "tvar.hpp"
#include "config.hpp"

enum TypeString
{
	tsStr,
	tsInt,
	tsFloat,
};

static TypeString checkTypeString(const wchar_t *TestStr)
{
	TypeString typeTestStr=tsStr;

	if (TestStr && *TestStr)
	{
		const wchar_t *ptrTestStr=TestStr;
		wchar_t ch, ch2;
		bool isNum     = true;
		//bool isDec     = false;
		bool isBegDec  = false;
		//bool isHex     = false;
		bool isBegHex  = false;
		//bool isOct     = false;
		bool isBegOct  = false;
		//bool isE       = false;
		bool isExp     = false;
		bool isPoint   = false;
		bool isSign    = false;
		bool isExpSign = false;

		if (*ptrTestStr == L'-' || *ptrTestStr == L'+')
		{
			isSign=true;
			ptrTestStr++;
		}

		if (*ptrTestStr == L'.' && iswdigit(ptrTestStr[1]))
		{
			isPoint=true;
			ptrTestStr++;
		}

		if (*ptrTestStr >= L'1' && *ptrTestStr <=L'9')
			isBegDec=true;
		else if (*ptrTestStr == L'0')
		{
			if ((ptrTestStr[1] == L'x' || ptrTestStr[1] == L'X') && iswxdigit(ptrTestStr[2]))
			{
				isBegHex=true;
				ptrTestStr+=2;
			}
			else
			{
				if (iswdigit(ptrTestStr[1]) || ptrTestStr[1] == L'.')
					isBegDec=true;
				else if (!ptrTestStr[1])
					return tsInt;
				else
					isBegOct=true;
			}
		}

		while ((ch=*ptrTestStr++) )
		{
			switch (ch)
			{
				case L'-':
				case L'+':

					if (ptrTestStr == TestStr+1)
						isSign=true;
					else if (isSign)
					{
						isNum=false;
						break;
					}

					if (isExp)
					{
						if (isExpSign)
						{
							isNum=false;
							break;
						}

						isExpSign=true;
					}

					break;
				case L'.':

					if (isPoint)
					{
						isNum=false;
						break;
					}

					isPoint=true;

					if (!(iswdigit(ptrTestStr[1]) || ptrTestStr[1] == L'e' || ptrTestStr[1] == L'E' || !ptrTestStr[1]))
					{
						isNum=false;
						break;
					}

					break;
				case L'e':
				case L'E':
					//isHex=true;
					//isE=true;
					ch2=*ptrTestStr++;

					if (ch2 == L'-' || ch2 == L'+')  // E+D
					{
						if (isBegHex || isExpSign)  // начало hex или уже был знак у порядка?
						{
							isNum=false;
							break;
						}

						isExpSign=true;
						wchar_t ch3=*ptrTestStr++;

						if (!iswdigit(ch3))   // за знаком идет число?
						{
							isNum=false;
							break;
						}
						else
						{
							isExp=true;
						}
					}
					else if (!iswdigit(ch2))   // ED
					{
						if (isBegDec)
						{
							isNum=false;
							break;
						}

						ptrTestStr--;
					}
					else
					{
						isExp=true;
						ptrTestStr--;
					}

					break;
				case L'a': case L'A': case L'b': case L'B': case L'c': case L'C': case L'd': case L'D': case L'f': case L'F':

					if (isBegDec || isExp)
					{
						isNum=false;
						break;
					}

					//isHex=true;
					break;
				case L'0': case L'1': case L'2': case L'3': case L'4': case L'5': case L'6': case L'7':
					//isOct=true;
				case L'8': case L'9':

					if (isBegOct && (ch == L'8' || ch == L'9'))
					{
						isNum=false;
						break;
					}

					//isDec=true;
					break;
				default:
					isNum=false;
			}

			if (!isNum)
				break;
		}

		if (isNum)
		{
			if (isBegDec && (isExp || isPoint))
				typeTestStr=tsFloat;

			if ((isBegDec || isBegHex || isBegOct) && !(isExp || isPoint))
				typeTestStr=tsInt;
		}
	}

	return typeTestStr;
}

static const wchar_t *toString(int64_t num)
{
	static wchar_t str[128];
	_i64tow(num, str, 10);
	return str;
}

static const wchar_t *toString(double num)
{
	static wchar_t str[256];
	swprintf(str, ARRAYSIZE(str)-1, L"%.14g", num);
	return str;
}

static wchar_t *dubstr(const wchar_t *s)
{
	wchar_t *newStr=nullptr;

	if (s)
	{
		newStr = new(std::nothrow) wchar_t[StrLength(s)+1];

		if (newStr)
			wcscpy(newStr, s);
	}

	return newStr;
}

static TVar addStr(const wchar_t *a, const wchar_t *b)
{
	TVar r(L"");
	wchar_t *c = new(std::nothrow) wchar_t[StrLength(a ? a : L"")+StrLength(b ? b : L"")+1];

	if (c)
	{
		r = wcscat(wcscpy(c, a ? a : L""), b ? b : L"");
		delete [] c;
	}

	return r;
}

TVar::~TVar()
{
	if (str)
		delete [] str;
}

TVar::TVar() :
	vType(vtUnknown),
	inum(0),
	dnum(0.0),
	str(nullptr)
{
}

TVar::TVar(int64_t v) :
	vType(vtInteger),
	inum(v),
	dnum(0.0),
	str(nullptr)
{
}

TVar::TVar(int v) :
	vType(vtInteger),
	inum((int64_t)v),
	dnum(0.0),
	str(nullptr)
{
}

TVar::TVar(double v) :
	vType(vtDouble),
	inum(0),
	dnum(v),
	str(nullptr)
{
}

TVar::TVar(const wchar_t *v) :
	vType(vtString),
	inum(0),
	dnum(0.0),
	str(dubstr(v))
{
}

TVar::TVar(const TVar& v) :
	vType(v.vType),
	inum(v.inum),
	dnum(v.dnum),
	str(dubstr(v.str))
{
}

TVar& TVar::operator=(const TVar& v)
{
	if (this != &v)
	{
		vType = v.vType;
		inum = v.inum;
		dnum = v.dnum;

		if (str)
			delete [] str;

		str=dubstr(v.str);
	}

	return *this;
}

TVar& TVar::operator=(const int& v)
{
	vType = vtInteger;
	inum = static_cast<int64_t>(v);
	dnum = 0.0;
	if (str)
		delete [] str;
	str = nullptr;

	return *this;
}

TVar& TVar::operator=(const int64_t& v)
{
	vType = vtInteger;
	inum = v;
	dnum = 0.0;
	if (str)
		delete [] str;
	str = nullptr;

	return *this;
}

TVar& TVar::operator=(const double& v)
{
	vType = vtDouble;
	inum = static_cast<int64_t>(0);
	dnum = v;
	if (str)
		delete [] str;
	str = nullptr;

	return *this;
}

int64_t TVar::i() const
{
	return isInteger() ? inum : (isDouble() ? (int64_t)dnum : (str ? _wtoi64(str) : 0));
}

double TVar::d() const
{
	wchar_t *endptr;
	return isDouble() ? dnum : (isInteger() ? (double)inum : (str ? wcstod(str,&endptr) : 0));
}

const wchar_t *TVar::s() const
{
	if (isUnknown())
		return L"";

	if (isString())
		return  str ? str : L"";

	return isInteger()? (::toString(inum)) : (::toString(dnum));
}

const wchar_t *TVar::toString()
{
	wchar_t s[256] = {0};

	switch (vType)
	{
		case vtDouble:
			far_wcsncpy(s, ::toString(dnum),ARRAYSIZE(s));
			break;
		case vtInteger:
			far_wcsncpy(s, ::toString(inum),ARRAYSIZE(s));
			break;
		case vtString:
			return str;
		case vtUnknown:
			break;
	}

	if (str)
		delete [] str;

	str = dubstr(s);
	vType = vtString;
	return str;
}

int64_t TVar::toInteger()
{
	if (vType == vtString)
		inum = str ? _wtoi64(str) : 0;
	else if (vType == vtDouble)
		inum=(int64_t)dnum;

	vType = vtInteger;
	return inum;
}

double TVar::toDouble()
{
	if (vType == vtString)
	{
		wchar_t *endptr;
		dnum = str ? wcstod(str,&endptr) : 0;
	}
	else if (vType == vtInteger)
		dnum=(double)inum;

	vType = vtDouble;
	return dnum;
}

int64_t TVar::getInteger() const
{
	int64_t ret = inum;

	if (vType == vtString)
		ret = str ? _wtoi64(str) : 0;
	else if (vType == vtDouble)
		ret=(int64_t)dnum;

	return ret;
}

int32_t TVar::getInt32() const
{
	int64_t ret = inum;

	if (vType == vtString)
		ret = str ? _wtoi64(str) : 0;
	else if (vType == vtDouble)
		ret=static_cast<int64_t>(dnum);

	return static_cast<int32_t>(ret);
}

double TVar::getDouble() const
{
	double ret = dnum;

	if (vType == vtString)
	{
		wchar_t *endptr;
		ret = str ? wcstod(str,&endptr) : 0;
	}
	else if (vType == vtInteger)
		ret=(double)inum;

	return ret;
};

static int _cmp_Ne(TVarType vt,const void *a, const void *b)
{
	int r = 1;

	switch (vt)
	{
		case vtInteger: r = *(int64_t*)a != *(int64_t*)b?1:0; break;
		case vtDouble:  r = *(double*)a != *(double*)b?1:0; break;
		case vtString:  r = StrCmp((const wchar_t*)a, (const wchar_t*)b); break;
		case vtUnknown: break;
	}

	return r;
}

static int _cmp_Eq(TVarType vt,const void *a, const void *b)
{
	int r = 0;

	switch (vt)
	{
		case vtInteger: r = *(int64_t*)a == *(int64_t*)b?1:0; break;
		case vtDouble:  r = *(double*)a == *(double*)b?1:0; break;
		case vtString:  r = !StrCmp((const wchar_t*)a, (const wchar_t*)b); break;
		case vtUnknown: break;
	}

	return r;
}

static int _cmp_Lt(TVarType vt,const void *a, const void *b)
{
	int r = 0;

	switch (vt)
	{
		case vtInteger: r = *(int64_t*)a < *(int64_t*)b?1:0; break;
		case vtDouble:  r = *(double*)a < *(double*)b?1:0; break;
		case vtString:  r = StrCmp((const wchar_t*)a, (const wchar_t*)b) < 0; break;
		case vtUnknown: break;
	}

	return r;
}

static int _cmp_Le(TVarType vt,const void *a, const void *b)
{
	int r = 0;

	switch (vt)
	{
		case vtInteger: r = *(int64_t*)a <= *(int64_t*)b?1:0; break;
		case vtDouble:  r = *(double*)a <= *(double*)b?1:0; break;
		case vtString:  r = StrCmp((const wchar_t*)a, (const wchar_t*)b) <= 0; break;
		case vtUnknown: break;
	}

	return r;
}

static int _cmp_Gt(TVarType vt,const void *a, const void *b)
{
	int r = 0;

	switch (vt)
	{
		case vtInteger: r = *(int64_t*)a > *(int64_t*)b?1:0; break;
		case vtDouble:  r = *(double*)a > *(double*)b?1:0; break;
		case vtString:  r = StrCmp((const wchar_t*)a, (const wchar_t*)b) > 0; break;
		case vtUnknown: break;
	}

	return r;
}

static int _cmp_Ge(TVarType vt,const void *a, const void *b)
{
	int r = 0;

	switch (vt)
	{
		case vtInteger: r = *(int64_t*)a >= *(int64_t*)b?1:0; break;
		case vtDouble:  r = *(double*)a >= *(double*)b?1:0; break;
		case vtString:  r = StrCmp((const wchar_t*)a, (const wchar_t*)b) >= 0; break;
		case vtUnknown: break;
	}

	return r;
}

int TVar::CompAB(const TVar& a, const TVar& b, TVarFuncCmp fcmp)
{
	int r = 1;
	int64_t bi;
	double bd;

	switch (a.vType)
	{
		case vtInteger:

			switch (b.vType)
			{
				case vtInteger: r = fcmp(vtInteger,&a.inum,&b.inum); break;
				case vtDouble:  r = fcmp(vtDouble,&a.inum,&b.dnum); break;
				case vtString:
				{
					switch (checkTypeString(b.s()))
					{
						case tsStr:             r = fcmp(vtString,a.s(),b.str); break;
						case tsInt:   bi=b.i(); r = fcmp(vtInteger,&a.inum,&bi); break;
						case tsFloat: bd=b.d(); r = fcmp(vtDouble,&a.inum,&bd); break;
					}

					break;
				}
				case vtUnknown: break;
			}

			break;
		case vtDouble:

			switch (b.vType)
			{
				case vtInteger: r = fcmp(vtInteger,&a.dnum,&b.inum); break;
				case vtDouble:  r = fcmp(vtDouble,&a.dnum,&b.dnum);  break;
				case vtString:
				{
					switch (checkTypeString(b.str))
					{
						case tsStr:             r = fcmp(vtString,a.s(),b.str); break;
						case tsInt:
						case tsFloat: bd=b.d(); r = fcmp(vtDouble,&a.inum,&bd); break;
					}

					break;
				}
				case vtUnknown: break;
			}

			break;
		case vtString:
		{
			r = fcmp(vtString,a.s(),b.s());
			break;
		}
		case vtUnknown: break;
	}

	return r;
};

int operator!=(const TVar& a, const TVar& b)
{
	return TVar::CompAB(a,b,(TVarFuncCmp)_cmp_Ne);
}

int operator==(const TVar& a, const TVar& b)
{
	return TVar::CompAB(a,b,(TVarFuncCmp)_cmp_Eq);
}

int operator<(const TVar& a, const TVar& b)
{
	return TVar::CompAB(a,b,(TVarFuncCmp)_cmp_Lt);
}

int operator<=(const TVar& a, const TVar& b)
{
	return TVar::CompAB(a,b,(TVarFuncCmp)_cmp_Le);
}

int operator>(const TVar& a, const TVar& b)
{
	return TVar::CompAB(a,b,(TVarFuncCmp)_cmp_Gt);
}

int operator>=(const TVar& a, const TVar& b)
{
	return TVar::CompAB(a,b,(TVarFuncCmp)_cmp_Ge);
}

TVar TVar::operator+()
{
	return *this;
}

TVar TVar::operator-()
{
	switch (vType)
	{
		case vtInteger:
			return TVar(-inum);
		case vtDouble:
			return TVar(-dnum);
		default:
			return *this;
	}
}

TVar TVar::operator!()
{
	switch (vType)
	{
		case vtInteger:
			return TVar((int64_t)!inum);
		case vtDouble:
			return TVar((double)!dnum);
		case vtString:
		default:
			return *this;
	}
}

TVar TVar::operator~()
{
	switch (vType)
	{
		case vtInteger:
			return TVar(~inum);
		case vtDouble:
			return TVar(~((int64_t)dnum));
		case vtString:
		default:
			return *this;
	}
}

int64_t TVar::asInteger() const
{
	switch (vType)
	{
	case vtInteger:
	case vtUnknown:
		return inum;

	case vtDouble:
		return dnum;

	case vtString:
		{
			long long val;
			return 1 == wscanf(str, "%ll", &val) ? val : 0;
		}

	default:
		return 0;
	}
}

double TVar::asDouble() const
{
	switch (vType)
	{
	case vtInteger:
	case vtUnknown:
		return inum;

	case vtDouble:
		return dnum;

	case vtString:
		{
			double val;
			return 1 == wscanf(str, "%lf", &val) ? val : 0;
		}

	default:
		return 0;
	}
}
