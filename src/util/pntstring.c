#include <stddef.h> /* size_t */
#include <stdlib.h> /* malloc, realloc etc. memory functions */
#include <string.h>

/* uint16_t defined in inclusion of "stdint.h" in pntstring.h */

#include "bytestream.h"
#include "utf.h"
#include "pntstring.h"

typedef struct PntString PntString;
struct PntString {
    void* data;
    size_t len;
};

/* Private functions forward declaration */
void PntString_Init(PntString* pPntString);
void PntString_InitFromUTF16(PntString* pPntString, const uint16_t* pStr);

/****************************************************************************
 *                             PUBLIC FUNCTIONS                             *
 ****************************************************************************/

/**
 * [PUBLIC]
 *
 * Create the PntString object
 *
 * @return PntString instance
 */
PntString* PntString_Create()
{
    PntString* pPntString = (PntString*)malloc(sizeof(PntString));
    if (pPntString)
    {
        PntString_Init(pPntString);
    }

    return pPntString;
}

/**
 * [PUBLIC]
 *
 * Create the PntString object from UTF16 string
 *
 * @return PntString instance
 */
PntString* PntString_CreateFromUTF16(const uint16_t* pStr)
{
    PntString* pPntString = (PntString*)malloc(sizeof(PntString));
    if (pPntString)
    {
        PntString_InitFromUTF16(pPntString, pStr);
    }

    return pPntString;
}

/**
 * [PUBLIC]
 *
 * Re-initialize existing PntString object with UTF16 string
 *
 * @param pPntString PntString instance
 */
void PntString_SetFromUTF16(PntString* pPntString, const uint16_t* pStr)
{
    /* Check that pPntString is valid */
    if (!pPntString)
    {
        return;
    }

    ByteStream* pByteStream = ByteStream_Create();
    PutUTF16StringIntoUTF8Stream(pByteStream, pStr);

    size_t len = ByteStream_Length(pByteStream);

    uint8_t* pData;
    if (pPntString->data)
    {
        pData = realloc(pPntString->data, len);
    }
    else {
        pData = malloc(pPntString->data, len);
    }

    if (!pData)
    {
        goto error;
    }

    uint8_t* pBuffer = ByteStream_GetBuffer(pByteStream);
    memcpy(pData, pBuffer, len);

    pPntString->data = pData;
    pPntString->len = len;

error:
    ByteStream_Destroy(pByteStream);
}

/**
 * [PUBLIC]
 *
 * Get raw pointer to internal UTF8 string
 *
 * @param pPntString PntString instance
 *
 * @return Pointer to UTF8 string buffer
 */
const uint8_t* PntString_GetAsUTF8(const PntString* pPntString)
{
    return (const uint8_t*)pPntString->data;
}

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
size_t PntString_CopyToUTF16String(PntString* pPntString, uint16_t* pDest)
{
    size_t utf16_len = 0;
    size_t i = 0;

    ByteStream* pByteStream = ByteStream_Create();
    ByteStream_SetBuffer(pByteStream, pPntString->data, pPntString->len);

    uint32_t codepoint;
    while (codepoint = ReadCodepointFromUTF8Stream(pByteStream))
    {
        if (codepoint < 0x10000)
        {
            if (pDest)
            {
                pDest[utf16_len++] = (uint16_t)codepoint;
            }
            /* Dry run */
            else {
                utf16_len++;
            }
        }
        else {
            if (pDest)
            {
                pDest[utf16_len++] = (uint16_t)((codepoint >> 10) + 0xD7C0);
                pDest[utf16_len++] = (uint16_t)((codepoint & 0x3FF) + 0xDC00);
            }
            /* dry run */
            else {
                utf16_len += 2;
            }
            
        }
    }

    if (pDest)
    {
        pDest[utf16_len] = 0;
    }
    
    return utf16_len;
}

/**
 * [PUBLIC]
 *
 * Get the length of the string
 *
 * @param pPntString PntString instance to get length from
 *
 * @return String length
 */
size_t PntString_GetLength(const PntString* pPntString)
{
    return pPntString->len;
}

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
int PntString_CompareWithAnother(const PntString* pPntString, const PntString* pCompareWith)
{
    if (!pPntString || !pCompareWith)
    {
        return -2;
    }

    if (pPntString->len < pCompareWith->len)
    {
        return -1;
    }

    if (pPntString->len > pCompareWith->len)
    {
        return 1;
    }

    ByteStream* pByteStream1 = ByteStream_Create();
    ByteStream* pByteStream2 = ByteStream_Create();
    ByteStream_SetBuffer(pByteStream1, pPntString->data, pPntString->len);
    ByteStream_SetBuffer(pByteStream2, pCompareWith->data, pCompareWith->len);

    int result = 0;

    for (size_t i = 0; i < pPntString->len; ++i)
    {
        uint32_t codepoint1 = ReadCodepointFromUTF8Stream(pByteStream1);
        uint32_t codepoint2 = ReadCodepointFromUTF8Stream(pByteStream2);

        if (codepoint1 == codepoint2)
        {
            continue;
        }

        /* As we deal with unsigned 32-bit values, casting it to
         * signed 64-bit only for a subtraction would be too ambiguous
         */
        if (codepoint1 < codepoint2)
        {
            result = -1;
            goto final;
        }
        else {
            result = 1;
            goto final;
        }
    }

    final:
    ByteStream_Destroy(pByteStream1);
    ByteStream_Destroy(pByteStream2);

    return result;
}

/**
 * [PUBLIC]
 *
 * Destroy the PntString object and it's internal buffer
 *
 * @param pPntString PntString instance
 */
void PntString_Destroy(PntString* pPntString)
{
    free(pPntString->data);
    free(pPntString);
}

/****************************************************************************
 *                             PRIVATE FUNCTIONS                            *
 ****************************************************************************/

 /**
  * [PRIVATE]
  *
  * Init empty PntString with NULL pData buffer and zero length
  *
  * @param pPntString PntString instance
  */
void PntString_Init(PntString* pPntString)
{
    memset(pPntString, 0, sizeof(PntString));
}

/**
 * [PRIVATE]
 *
 * Init a PntString with predefined data copied from provided UTF16 string
 *
 * @param pPntString PntString instance
 * @param pStr UTF16 string
 */
void PntString_InitFromUTF16(PntString* pPntString, const uint16_t* pStr)
{
    memset(pPntString, 0, sizeof(PntString));

    ByteStream* pByteStream = ByteStream_Create();
    PutUTF16StringIntoUTF8Stream(pByteStream, pStr);

    size_t len = ByteStream_Length(pByteStream);
    uint8_t* pData = (uint8_t*)malloc(len * sizeof(uint8_t));
    if (!pData)
    {
        goto error;
    }

    ByteStream_Copy(pByteStream, pData);

    pPntString->data = pData;
    pPntString->len = len;

error:
    ByteStream_Destroy(pByteStream);
}
