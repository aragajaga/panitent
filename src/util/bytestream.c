#include "bytestream.h"

#include <string.h>
#include <stdlib.h>

typedef struct ByteStream ByteStream;
struct ByteStream {
    uint8_t* pData;
    uint8_t* pReadPtr;
    uint8_t* pWritePtr;
    size_t capacity;
    size_t length;
    int state;
};

#define MIN_CAPACITY_WINDOW 16

void ByteStream_Init(ByteStream* pByteStream);

/**
 * [PUBLIC]
 * 
 * Create the ByteStream object
 * 
 * @return Pointer to new ByteStream instance
 */
ByteStream* ByteStream_Create()
{
    ByteStream *pByteStream = (ByteStream*)malloc(sizeof(ByteStream));
    if (pByteStream)
    {
        ByteStream_Init(pByteStream);
    }

    return pByteStream;
}

/**
 * [PRIVATE]
 * 
 * Initialize the ByteStream structure
 * 
 * @param pByteStream Pointer to ByteStream memory location
 */
void ByteStream_Init(ByteStream* pByteStream)
{
    memset(pByteStream, 0, sizeof(ByteStream));

    pByteStream->pData = malloc(MIN_CAPACITY_WINDOW);
    if (pByteStream->pData)
    {
        memset(pByteStream->pData, 0, MIN_CAPACITY_WINDOW);
        pByteStream->capacity = MIN_CAPACITY_WINDOW;
        pByteStream->pReadPtr = pByteStream->pData;
        pByteStream->pWritePtr = pByteStream->pData;
    }
}

uint8_t* ByteStream_GetBuffer(const ByteStream* pByteStream)
{
    return pByteStream->pReadPtr;
}

uint8_t ByteStream_Get(ByteStream* pByteStream)
{
    return *pByteStream->pReadPtr++;
}

void ByteStream_Put(ByteStream* pByteStream, uint8_t b)
{
    if (!pByteStream)
    {
        return;
    }

    if (pByteStream->pWritePtr == (pByteStream->pData + pByteStream->capacity))
    {
        if ((pByteStream->pWritePtr - pByteStream->pReadPtr) < pByteStream->capacity)
        {
            ptrdiff_t writeDiff = pByteStream->pWritePtr - pByteStream->pData;

            uint8_t* pData = memmove(pByteStream->pData, pByteStream->pReadPtr, pByteStream->pWritePtr - pByteStream->pReadPtr);
            pByteStream->pData = pData;
            pByteStream->pReadPtr = pData;
            pByteStream->pWritePtr = pData + writeDiff;
        }
        else {
            size_t newCapacity = pByteStream->capacity * 2;
            ptrdiff_t readDiff = pByteStream->pReadPtr - pByteStream->pData;
            ptrdiff_t writeDiff = pByteStream->pWritePtr - pByteStream->pData;
            uint8_t* pData = realloc(pByteStream->pData, newCapacity);
            if (!pData)
            {
                exit(1);
            }

            pByteStream->capacity = newCapacity;
            pByteStream->pData = pData;
            pByteStream->pReadPtr = pData + readDiff;
            pByteStream->pWritePtr = pData + writeDiff;
        }
    }

    *pByteStream->pWritePtr++ = b;
    pByteStream->length++;
}

/**
 * [PUBLIC]
 *
 * Destroy the ByteStream object and internal buffer
 *
 * @param pByteStream ByteStream instance
 */
void ByteStream_Destroy(ByteStream* pByteStream)
{
    free(pByteStream->pData);
    free(pByteStream);
}

/**
 * [PUBLIC]
 * 
 * Get the length of ByteStream internal buffer
 *
 * Length is a difference between reading and writing locations
 * 
 * @param pByteStream ByteStream instance
 * 
 * @return the length of the stream
 */
size_t ByteStream_Length(ByteStream* pByteStream)
{
    return pByteStream->pWritePtr - pByteStream->pReadPtr;
}

/**
 * [PUBLIC]
 *
 * Copy the actual buffer into plain memory pointed by pDest
 * 
 * pDest should be large enough to successfully copy the buffer contents.
 * Use ByteStream_Length to acknowledge the length of the data to be copied
 * 
 * @param pByteStream ByteStream instance
 * @param pDest The destination buffer
 */
void ByteStream_Copy(ByteStream* pByteStream, uint8_t* pDest)
{
    if (!pByteStream || !pDest)
    {
        return;
    }

    memcpy(pDest, pByteStream->pReadPtr, pByteStream->pWritePtr - pByteStream->pReadPtr);
}

/**
 * [PUBLIC]
 *
 * Replace buffer contents with provided one
 *
 * Read pointer will be set at the start of the buffer
 * and write pointer will be set at the end
 *
 * @param pByteStream ByteStream instance
 * @param pSrc New buffer data
 * @param len Length of the data to be copied
 */
void ByteStream_SetBuffer(ByteStream* pByteStream, uint8_t* pSrc, size_t len)
{
    /* Calculate the new capacity */
    size_t newCapacity = (len / MIN_CAPACITY_WINDOW + 1) * MIN_CAPACITY_WINDOW;

    uint8_t* pData;
    if (pByteStream->pData)
    {
        if (pByteStream->capacity != newCapacity)
        {
            pData = (uint8_t)realloc(pByteStream->pData, newCapacity);
        }
        else {
            pData = pByteStream->pData;
        }
        
    }
    else {
        pData = (uint8_t)malloc(newCapacity);
    }

    if (pData)
    {
        memcpy(pData, pSrc, len);
        pByteStream->pData = pData;
        pByteStream->capacity = newCapacity;
        pByteStream->pReadPtr = pData;
        pByteStream->pWritePtr = pData + len;
    }
}
