#ifndef __COMMON_LOG_H__
#define __COMMON_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

void common_log(const char * filename, const char * prefix, const char * file, const char *function, unsigned int line, const char *format, ...);

#ifdef __cplusplus
}
#endif

// LOG_FILE and LOG_SOURCE_FILE define in c(cpp) file
// use '#define LOG_FILE ""' for console output

#define LOG_INFO(args...) common_log(LOG_FILE,  "[info] ", LOG_SOURCE_FILE, __FUNCTION__, __LINE__, args)
#define LOG_WARN(args...) common_log(LOG_FILE,  "[warn] ", LOG_SOURCE_FILE, __FUNCTION__, __LINE__, args)
#define LOG_ERROR(args...) common_log(LOG_FILE, "[error]", LOG_SOURCE_FILE, __FUNCTION__, __LINE__, args)

#define LOG_INFO_CONSOLE(args...) common_log(0,  "[info] ", LOG_SOURCE_FILE, __FUNCTION__, __LINE__, args)
#define LOG_WARN_CONSOLE(args...) common_log(0,  "[warn] ", LOG_SOURCE_FILE, __FUNCTION__, __LINE__, args)
#define LOG_ERROR_CONSOLE(args...) common_log(0, "[error]", LOG_SOURCE_FILE, __FUNCTION__, __LINE__, args)

#endif // __COMMON_LOG_H__
