/* Copyright � 2012, Intel Corporation.  All rights reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

-       Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
-       Redistributions in binary form must reproduce the above copyright 
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
-       Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. */

#include "rdrand.h"

#include <intrin.h>

/*! \def RDRAND_MASK
*    The bit mask used to examine the ecx register returned by cpuid. The 
*   30th bit is set.
*/
#define RDRAND_MASK	0x40000000

#define RETRY_LIMIT 10

#ifdef _WIN64
typedef uint64_t _wordlen_t;
#else
typedef uint32_t _wordlen_t;
#endif

/*! \brief Queries cpuid to see if rdrand is supported
*
* rdrand support in a CPU is determined by examining the 30th bit of the ecx
* register after calling cpuid.
* 
* \return bool of whether or not rdrand is supported
*/
int RdRand_cpuid()
{
	int info[4] = {-1, -1, -1, -1};
	int ecx;

	/* Are we on an Intel processor? */

	__cpuid(info, /*feature bits*/0);

	if ( memcmp((void *) &info[1], (void *) "Genu", 4) != 0 ||
		memcmp((void *) &info[3], (void *) "ineI", 4) != 0 ||
		memcmp((void *) &info[2], (void *) "ntel", 4) != 0 ) {

			return 0;
	}

	/* Do we have RDRAND? */

	__cpuid(info, /*feature bits*/1);

	ecx = info[2];
	if ((ecx & RDRAND_MASK) == RDRAND_MASK)
		return 1;
	else
		return 0;
}

/*! \brief Determines whether or not rdrand is supported by the CPU
*
* This function simply serves as a cache of the result provided by cpuid, 
* since calling cpuid is so expensive. The result is stored in a static 
* variable to save from calling cpuid on each invocation of rdrand.
* 
* \return bool/int of whether or not rdrand is supported
*/
int RdRand_isSupported()
{
	static int supported = RDRAND_SUPPORT_UNKNOWN;

	if (supported == RDRAND_SUPPORT_UNKNOWN)
	{
		if (RdRand_cpuid())
			supported = RDRAND_SUPPORTED;
		else
			supported = RDRAND_UNSUPPORTED;
	}

	return (supported == RDRAND_SUPPORTED) ? 1 : 0;
}

int rdrand_16(uint16_t* x, int retry)
{
	int i;
	if (RdRand_isSupported())
	{
		if (retry)
		{
			for (i = 0; i < RETRY_LIMIT; i++)
			{
				int flag = _rdrand16_step(x);
				if (flag)
					return RDRAND_SUCCESS;
			}

			return RDRAND_NOT_READY;
		}
		else
		{
			int flag = _rdrand16_step(x);
			if (flag)
				return RDRAND_SUCCESS;
			else
				return RDRAND_NOT_READY;
		}
	}
	else
	{
		return RDRAND_UNSUPPORTED;
	}
}

int rdrand_32(uint32_t* x, int retry)
{
	int i;
	if (RdRand_isSupported())
	{
		if (retry)
		{
			for (i = 0; i < RETRY_LIMIT; i++)
			{
				int flag = _rdrand32_step(x);
				if (flag)
					return RDRAND_SUCCESS;
			}

			return RDRAND_NOT_READY;
		}
		else
		{
			int flag = _rdrand32_step(x);
			if (flag)
				return RDRAND_SUCCESS;
			else
				return RDRAND_NOT_READY;
		}
	}
	else
	{
		return RDRAND_UNSUPPORTED;
	}
}

#ifdef _WIN64
int rdrand_64(uint64_t* x, int retry)
{
	int i;
	if (RdRand_isSupported())
	{
		if (retry)
		{
			for (i = 0; i < RETRY_LIMIT; i++)
			{
				int flag = _rdrand64_step(x);
				if (flag)
					return RDRAND_SUCCESS;
			}

			return RDRAND_NOT_READY;
		}
		else
		{
			int flag = _rdrand64_step(x);
			if (flag)
				return RDRAND_SUCCESS;
			else
				return RDRAND_NOT_READY;
		}
	}
	else
	{
		return RDRAND_UNSUPPORTED;
	}
}

int rdrand_get_n_64(unsigned int n, uint64_t *dest)
{
	int success;
	int count;
	unsigned int i;

	for (i=0; i<n; i++)
	{
		count = 0;
		do
		{
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

	for (i=0; i<n; i++)
	{
		count = 0;
		do
		{
			success= rdrand_32(dest, 1);
		} while((success == 0) && (count++ < RETRY_LIMIT));
		if (success != RDRAND_SUCCESS) return success;
		dest= &(dest[1]);
	}
	return RDRAND_SUCCESS; 
}

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

	/* Compute the address of the first 32- or 64- bit aligned block in the destination buffer, depending on whether we are in 32- or 64-bit mode */
	start = dest;
	if (((uint32_t)start % (uint32_t) sizeof(_wordlen_t)) == 0)
	{
		blockstart = (_wordlen_t *)start;
		count = n;
		startlen = 0;
	}
	else
	{
		blockstart = (_wordlen_t *)(((_wordlen_t)start & ~(_wordlen_t) (sizeof(_wordlen_t)-1) )+(_wordlen_t)sizeof(_wordlen_t));
		count = n - (sizeof(_wordlen_t) - (unsigned int)((_wordlen_t)start % sizeof(_wordlen_t)));
		startlen = (unsigned int)((_wordlen_t)blockstart - (_wordlen_t)start);
	}

	/* Compute the number of 32- or 64- bit blocks and the remaining number of bytes */
	residual = count % sizeof(_wordlen_t);
	length = count/sizeof(_wordlen_t);
	if (residual != 0)
	{
		residualstart = (unsigned char *)(blockstart + length);
	}

	/* Get a temporary random number for use in the residuals. Failout if retry fails */
	if (startlen > 0)
	{
#ifdef _WIN64
		if ( (success= rdrand_64((uint64_t *) &temprand, 1)) != RDRAND_SUCCESS) return success;
#else
		if ( (success= rdrand_32((uint32_t *) &temprand, 1)) != RDRAND_SUCCESS) return success;
#endif
	}

	/* populate the starting misaligned block */
	for (i = 0; i<startlen; i++)
	{
		start[i] = (unsigned char)(temprand & 0xff);
		temprand = temprand >> 8;
	}

	/* populate the central aligned block. Fail out if retry fails */

#ifdef _WIN64
	if ( (success= rdrand_get_n_64(length, (uint64_t *)(blockstart))) != RDRAND_SUCCESS) return success;
#else
	if ( (success= rdrand_get_n_32(length, (uint32_t *)(blockstart))) != RDRAND_SUCCESS) return success;
#endif
	/* populate the final misaligned block */
	if (residual > 0)
	{
#ifdef _WIN64
		if ((success= rdrand_64((uint64_t *)&temprand, 1)) != RDRAND_SUCCESS) return success;
#else
		if ((success= rdrand_32((uint32_t *)&temprand, 1)) != RDRAND_SUCCESS) return success;
#endif

		for (i = 0; i<residual; i++)
		{
			residualstart[i] = (unsigned char)(temprand & 0xff);
			temprand = temprand >> 8;
		}
	}

	return RDRAND_SUCCESS;
}