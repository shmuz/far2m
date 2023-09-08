#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <alloca.h>

extern void common_log(const char * filename, const char * prefix, const char * file, const char *function, unsigned int line, const char *format, ...)
{
	va_list args;
	const char out_format[] = "%s %s:%u %s %s%s";
	char *xformat = (char *)alloca(strlen(prefix) + strlen(file) + strlen(format) + strlen(function) + sizeof(out_format));
	sprintf(xformat, out_format, prefix, file, line, function, (*format != '\n') ? " - " : "", format);

	FILE *stream = 0;
	if( filename && filename[0] )
		stream = fopen(filename, "at");

	if( !stream )
		stream = stderr;

	va_start(args, format);
	vfprintf(stream, xformat, args);
	va_end(args);

	if( stream != stderr )
		fclose(stream);
}

#ifdef MAIN_COMMON_LOG

#define LOG_FILE "/tmp/log.log"
#define LOG_SOURCE_FILE "log.c"

int main(int argc, char * argv[])
{
	LOG_INFO("\n");
	LOG_WARN("\n");
	LOG_ERROR("\n");
	LOG_INFO_CONSOLE("\n");
	LOG_WARN_CONSOLE("\n");
	LOG_ERROR_CONSOLE("\n");
	return 0;
}
#endif // MAIN_COMMON_LOG
