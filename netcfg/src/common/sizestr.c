#include "sizestr.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%llu"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%lu"
#else
    #error "Environment not 32 or 64-bit."
#endif

extern const char * size_to_str(unsigned long long size)
{
	static const char * units[] = {
		"B",
		"Kb",
		"Mb",
		"Gb",
		"Tb",
		"Eb",
		"Zb"
	};
	static char buf[sizeof("3.1415926535897932384626433832795028841971693993751058209749445923078 Zb")] = {0};
	int offset = 0;
	long double print_size = size;
	while( (size/1024) && (print_size = print_size/1024.0) && (size = size/1024) && ++offset );

	if( offset && snprintf(buf, sizeof(buf), "%.3Lg %s", print_size, units[offset]) > 0 )
		return buf;

	if( snprintf(buf, sizeof(buf), "%llu %s", size, units[offset]) > 0 )
		return buf;

	return "ERROR";

}

const char * msec_to_str(uint64_t msec)
{
	static char buf[sizeof("213503982334 days 24 hours 60 min 999 sec 999 ms")] = {0};
	const static struct {
		const char * fmt;
		uint64_t divider;
	} prtime[] = {
		{LLFMT " days ", 86400000},
		{LLFMT " hours ", 3600000},
		{LLFMT " min ", 60000},
		{LLFMT " sec ", 1000},
		{LLFMT " ms", 1}
	};
	int i, offset = 0;

	buf[0] = 0;
	for( i=0; i < sizeof(prtime)/sizeof(prtime[0]); i++ ) {
		if( offset >= 0 && (msec / prtime[i].divider) ) {
			offset += snprintf(buf + offset,
					   sizeof(buf) - offset,
					   prtime[i].fmt,
					   msec/prtime[i].divider);
			msec = msec % prtime[i].divider;
		}
	}
	return buf;
}

#ifdef MAIN_COMMON_SIZESTR

#define LOG_FILE "/tmp/log.log"
#define LOG_SOURCE_FILE "log.c"

#include "log.h"

int main(int argc, char * argv[])
{
	uint64_t num = (uint64_t)-1;
	long double numf = 3.0;
	LOG_INFO_CONSOLE("sizeof(unsigned long long) %u sizeof(double) %u sizeof(long double) %u sizeof(size_t) %u\n", sizeof(unsigned long long), sizeof(double), sizeof(long double), sizeof(size_t));
	LOG_INFO_CONSOLE("\"%llu\"\n", num);
	LOG_INFO_CONSOLE("\"%.100Lf\"\n", numf/7.0);
	LOG_INFO_CONSOLE("\"%.100Le\"\n", numf/7.0);
	LOG_INFO_CONSOLE("%s\n", size_to_str(0));
	LOG_INFO_CONSOLE("%s\n", size_to_str(1000));
	LOG_INFO_CONSOLE("%s\n", size_to_str(1024));
	LOG_INFO_CONSOLE("%s\n", size_to_str(1024*1024));
	LOG_INFO_CONSOLE("%s\n", size_to_str(1024*1024*1024));
	LOG_INFO_CONSOLE("%s\n", size_to_str(100000000000ull));
	LOG_INFO_CONSOLE("%s\n", size_to_str(17792387055516ull));
	LOG_INFO_CONSOLE("%s\n", size_to_str((1024ull)*1024*1024*1024));
	LOG_INFO_CONSOLE("%s\n", size_to_str((1024ull)*1024*1024*1024*1024));
	LOG_INFO_CONSOLE("%s\n", size_to_str(18446744073709551615ull));

	LOG_INFO_CONSOLE("%s\n", msec_to_str(101));
	LOG_INFO_CONSOLE("%s\n", msec_to_str(1000));
	LOG_INFO_CONSOLE("%s\n", msec_to_str(100001));
	LOG_INFO_CONSOLE("%s\n", msec_to_str(10000002));
	LOG_INFO_CONSOLE("%s\n", msec_to_str(100000000003ull));
	LOG_INFO_CONSOLE("%s\n", msec_to_str(17792387055516ull));
	LOG_INFO_CONSOLE("%s\n", msec_to_str(18446744073709551615ull));
	return 0;
}
#endif // MAIN_COMMON_SIZESTR
