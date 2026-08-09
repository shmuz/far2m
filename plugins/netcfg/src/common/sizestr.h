#ifndef __COMMON_SIZESTR_H__
#define __COMMON_SIZESTR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern const char * size_to_str(unsigned long long size);
extern const char * msec_to_str(uint64_t msec);

#define SEC_TO_MS(sec) (((uint64_t)sec)*1000)

#ifdef __cplusplus
}
#endif

#endif // __COMMON_SIZESTR_H__
