#include <strings.h>

#include <WinCompat.h>
#include "../WinPort/WinPort.h"

#include "DetectCodepage.h"

#ifdef USEUCD
# include <uchardet.h>

static bool IsDecimalNumber(const char *s)
{
	for (;*s;++s) {
		if (*s < '0' || *s > '9') {
			return false;
		}
	}
	return true;
}

static int TranslateUDCharset(const char *cs)
{
	if (strncasecmp(cs, "windows-", 8) == 0) {
		if (IsDecimalNumber(cs + 8)) {
			return atoi(cs + 8);
		}
		if (strcasecmp(cs + 8, "31j") == 0) {
				return 932;
		}
	}

	if (strncasecmp(cs, "CP", 2) == 0 && IsDecimalNumber(cs + 2)) {
		return atoi(cs + 2);
	}

	if (strncasecmp(cs, "IBM", 3) == 0 && IsDecimalNumber(cs + 3)) {
		return atoi(cs + 3);
	}

	if (!strcasecmp(cs, "UTF16-LE") || !strcasecmp(cs, "UTF16"))
		return CP_UTF16LE;
	if (!strcasecmp(cs, "UTF16-BE"))
		return CP_UTF16BE;
	if (!strcasecmp(cs, "UTF32-LE") || !strcasecmp(cs, "UTF32"))
		return CP_UTF32LE;
	if (!strcasecmp(cs, "UTF32-BE"))
		return CP_UTF32BE;
	if (!strcasecmp(cs, "UTF-8"))
		return CP_UTF8;
	if (!strcasecmp(cs, "UTF-7"))
		return CP_UTF7;
	if (!strcasecmp(cs, "ASCII"))
		return CP_UTF8;   //it's 20127 but CP_UTF8 is better in the editor
	if (!strcasecmp(cs, "KOI8-R"))
		return 20866;
	if (!strcasecmp(cs, "KOI8-U"))
		return 21866;
	if (!strncasecmp(cs, "ISO-8859-", 9) && IsDecimalNumber(cs + 9))
		return 28590 + atoi(cs + 9);
	if (!strcasecmp(cs, "ISO-8859-8-I"))
		return 38598;
	if (!strcasecmp(cs, "x-mac-hebrew") || !strcasecmp(cs, "MS-MAC-HEBREW"))
		return 10005;
	if (!strcasecmp(cs, "mac-cyrillic") || !strcasecmp(cs, "x-mac-cyrillic") || !strcasecmp(cs, "MS-MAC-CYRILLIC"))
		return 10007;
	if (!strcasecmp(cs, "EUC-JP"))
		return 20932;
	if (!strcasecmp(cs, "EUC-KR"))
		return 51949;
	if (!strcasecmp(cs, "ISO-2022-KR"))
		return 50225;
	if (!strcasecmp(cs, "GB18030"))
		return 54936;
	if (!strcasecmp(cs, "Shift_JIS"))
		return 932;

	fprintf(stderr, "TranslateUDCharset: unknown charset '%s'\n", cs);

	/*
		and the rest:
		"Big5"
		"x-euc-tw"
		"X-ISO-10646-UCS-4-3412" - UCS-4, unusual octet order BOM (3412)
		"X-ISO-10646-UCS-4-2143" - UCS-4, unusual octet order BOM (2143)
		ISO-2022-CN
		ISO-2022-JP
		"TIS-620"
		*/
	return -1;
}

int DetectCodePage(const char *data, size_t len)
{
	uchardet_t ud = uchardet_new();
	uchardet_handle_data(ud, data, len);
	uchardet_data_end(ud);
	const char *cs = uchardet_get_charset(ud);
	int out = cs ? TranslateUDCharset(cs) : -1;
//	fprintf(stderr, "DetectCodePage: '%s' -> %d\n", cs, out);
	uchardet_delete(ud);
	return out;
}

#else
int DetectCodePage(const char *data, size_t len)
{
	return -1;
}
#endif
