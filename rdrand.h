#ifndef RDRAND_H
#define RDRAND_H

#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
#else
/* MSVC specific */
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

#define RDRAND_SUCCESS 1
#define RDRAND_NOT_READY -1
#define RDRAND_SUPPORTED -2
#define RDRAND_UNSUPPORTED -3
#define RDRAND_SUPPORT_UNKNOWN -4

__declspec(dllexport) int rdrand_16(uint16_t* x, int retry);
__declspec(dllexport) int rdrand_32(uint32_t* x, int retry);
#ifdef _WIN64
__declspec(dllexport) int rdrand_64(uint64_t* x, int retry);
#endif

__declspec(dllexport) int rdrand_get_n_32(unsigned int n, uint32_t* x);
#ifdef _WIN64
__declspec(dllexport) int rdrand_get_n_64(unsigned int n, uint64_t* x);
#endif

__declspec(dllexport) int rdrand_get_bytes(unsigned int n, unsigned char *buffer);

#endif // RDRAND_H
