#include "rdrand.h"
#include <intrin.h>

#define RDRAND_MASK	0x40000000
#define RETRY_LIMIT 10

#ifdef _WIN64
typedef uint64_t _wordlen_t;
#else
typedef uint32_t _wordlen_t;
#endif

int rdrandCpuid()
{
	int info[4] = {-1, -1, -1, -1};
	int ecx;

	__cpuid(info, 0);

	if ( memcmp((void *) &info[1], (void *) "Genu", 4) != 0 ||
		memcmp((void *) &info[3], (void *) "ineI", 4) != 0 ||
		memcmp((void *) &info[2], (void *) "ntel", 4) != 0 ) {

			return 0;
	}

	__cpuid(info, 1);

	ecx = info[2];
	if ((ecx & RDRAND_MASK) == RDRAND_MASK)
		return 1;
	else
		return 0;
}

int rdrandSupported()
{
	static int supported = RDRAND_SUPPORT_UNKNOWN;

	if (supported == RDRAND_SUPPORT_UNKNOWN) {
		if (rdrandCpuid())
			supported = RDRAND_SUPPORTED;
		else
			supported = RDRAND_UNSUPPORTED;
	}

	return (supported == RDRAND_SUPPORTED) ? 1 : 0;
}

int rdrand_16(uint16_t* x, int retry)
{
	int i;
	if (rdrandSupported()) {
		if (retry) {
			for (i = 0; i < RETRY_LIMIT; i++) {
				int flag = _rdrand16_step(x);
				if (flag)
					return RDRAND_SUCCESS;
			}

			return RDRAND_NOT_READY;
		} else {
			int flag = _rdrand16_step(x);
			if (flag)
				return RDRAND_SUCCESS;
			else
				return RDRAND_NOT_READY;
		}
	} else {
		return RDRAND_UNSUPPORTED;
	}
}

int rdrand_32(uint32_t* x, int retry)
{
	int i;
	if (rdrandSupported()) {
		if (retry) {
			for (i = 0; i < RETRY_LIMIT; i++) {
				int flag = _rdrand32_step(x);
				if (flag)
					return RDRAND_SUCCESS;
			}

			return RDRAND_NOT_READY;
		} else {
			int flag = _rdrand32_step(x);
			if (flag)
				return RDRAND_SUCCESS;
			else
				return RDRAND_NOT_READY;
		}
	} else {
		return RDRAND_UNSUPPORTED;
	}
}

#ifdef _WIN64
int rdrand_get_n_64(unsigned int n, uint64_t *dest)
{
	int success;
	int count;
	unsigned int i;

	for (i=0; i<n; i++) {
		count = 0;
		do {
			success= rdrand_64(dest, 1);
		} while((success == 0) && (count++ < RETRY_LIMIT));
		if (success != RDRAND_SUCCESS) return success;
		dest= &(dest[1]);
	}
	return RDRAND_SUCCESS; 
}
#endif

int rdrand_get_n_32(unsigned int n, uint32_t *dest)
{
	int success;
	int count;
	unsigned int i;

	for (i=0; i<n; i++) {
		count = 0;
		do {
			success= rdrand_32(dest, 1);
		} while((success == 0) && (count++ < RETRY_LIMIT));
		if (success != RDRAND_SUCCESS) return success;
		dest= &(dest[1]);
	}
	return RDRAND_SUCCESS; 
}

#ifdef _WIN64
int rdrand_64(uint64_t* x, int retry)
{
	int i;
	if (rdrandSupported()) {
		if (retry) {
			for (i = 0; i < RETRY_LIMIT; i++) {
				int flag = _rdrand64_step(x);
				if (flag)
					return RDRAND_SUCCESS;
			}

			return RDRAND_NOT_READY;
		} else {
			int flag = _rdrand64_step(x);
			if (flag)
				return RDRAND_SUCCESS;
			else
				return RDRAND_NOT_READY;
		}
	} else {
		return RDRAND_UNSUPPORTED;
	}
}
#endif

int rdrand_get_bytes(unsigned int n, unsigned char *dest)
{
	unsigned char *start;
	unsigned char *residualstart;
	_wordlen_t *blockstart;
	_wordlen_t i, temprand;
	unsigned int count;
	unsigned int residual;
	unsigned int startlen;
	unsigned int length;
	int success;

	start = dest;
	if (((uint32_t)start % (uint32_t) sizeof(_wordlen_t)) == 0) {
		blockstart = (_wordlen_t *)start;
		count = n;
		startlen = 0;
	} else {
		blockstart = (_wordlen_t *)(((_wordlen_t)start & ~(_wordlen_t) (sizeof(_wordlen_t)-1) )+(_wordlen_t)sizeof(_wordlen_t));
		count = n - (sizeof(_wordlen_t) - (unsigned int)((_wordlen_t)start % sizeof(_wordlen_t)));
		startlen = (unsigned int)((_wordlen_t)blockstart - (_wordlen_t)start);
	}

	residual = count % sizeof(_wordlen_t);
	length = count/sizeof(_wordlen_t);
	if (residual != 0) {
		residualstart = (unsigned char *)(blockstart + length);
	}

	if (startlen > 0) {
#ifdef _WIN64
		if ( (success= rdrand_64((uint64_t *) &temprand, 1)) != RDRAND_SUCCESS) return success;
#else
		if ( (success= rdrand_32((uint32_t *) &temprand, 1)) != RDRAND_SUCCESS) return success;
#endif
	}

	for (i = 0; i<startlen; i++) {
		start[i] = (unsigned char)(temprand & 0xff);
		temprand = temprand >> 8;
	}


#ifdef _WIN64
	if ( (success= rdrand_get_n_64(length, (uint64_t *)(blockstart))) != RDRAND_SUCCESS) return success;
#else
	if ( (success= rdrand_get_n_32(length, (uint32_t *)(blockstart))) != RDRAND_SUCCESS) return success;
#endif
	/* populate the final misaligned block */
	if (residual > 0) {
#ifdef _WIN64
		if ((success= rdrand_64((uint64_t *)&temprand, 1)) != RDRAND_SUCCESS) return success;
#else
		if ((success= rdrand_32((uint32_t *)&temprand, 1)) != RDRAND_SUCCESS) return success;
#endif

		for (i = 0; i<residual; i++) {
			residualstart[i] = (unsigned char)(temprand & 0xff);
			temprand = temprand >> 8;
		}
	}
	return RDRAND_SUCCESS;
}