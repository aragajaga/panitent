#include "precomp.h"

#include "util.h"

extern int __float2int_s_check_safecast(float);
extern int float2int_s(int*, float);

uint32_t ABGRToARGB(uint32_t abgr)
{
	uint32_t b = (abgr >> 16) & 0xFF;
	uint32_t r = abgr & 0xFF;
	return (abgr & 0xFF00FF00) | (r << 16) | b;
}
