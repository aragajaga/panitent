#pragma once

#include <stdint.h> /* uint16_t */

typedef struct PntString PntString;

/**
 * [PUBLIC]
 * 
 * Create the PntString object
 * 
 * @return PntString instance
 */
PntString* PntString_Create();

/**
 * [PUBLIC]
 *
 * Create the PntString object from UTF16 string
 *
 * @return PntString instance
 */
PntString* PntString_CreateFromUTF16(const uint16_t* pStr);

/**
 * [PUBLIC]
 * 
 * Re-initialize existing PntString object with UTF16 string
 * 
 * @param pPntString PntString instance
 */
void PntString_SetFromUTF16(PntString* pPntString, const uint16_t* pStr);

/**
 * [PUBLIC]
 * 
 * Get raw pointer to internal UTF8 string
 * 
 * @param pPntString PntString instance
 * 
 * @return Pointer to UTF8 string buffer
 */
const uint8_t* PntString_GetAsUTF8(const PntString* pPntString);

/**
 * [PUBLIC]
 *
 * Convert to UTF16 string and copy it to destination buffer
 *
 * @param pPntString PntString instance
 * @param pDest Destination buffer
 *
 * @return Length of data copied
 */
size_t PntString_CopyToUTF16String(PntString* pPntString, uint16_t* pDest);

/**
 * [PUBLIC]
 * 
 * Get the length of the string
 * 
 * @param pPntString PntString instance to get length from
 * 
 * @return String length
 */
size_t PntString_GetLength(const PntString* pPntString);

/**
 * [PUBLIC]
 *
 * Compare one PntString with another PntString
 *
 * returns -1 if the first string is lower, 1 if the first string is higher and 0 if both equal
 * 
 * @param pPntString PntString to compare with
 * @param pCompareWith PntString to compare to
 * 
 * @return Comparison result
 */
int PntString_CompareWithAnother(const PntString* pPntString, const PntString* pCompareWith);

/**
 * [PUBLIC]
 *
 * Destroy the PntString object and it's internal buffer
 *
 * @param pPntString PntString instance
 */
void PntString_Destroy(PntString* pPntString);
