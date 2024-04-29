#ifndef _BYTESTREAM_H_
#define _BYTESTREAM_H_

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct ByteStream ByteStream;

/**
 * [PUBLIC]
 *
 * Create the ByteStream object
 *
 * @return Pointer to new ByteStream instance
 */
ByteStream* ByteStream_Create();

/**
 * [PUBLIC]
 *
 * Get current read pointer
 *
 * @return Pointer to read position
 */
uint8_t* ByteStream_GetBuffer(const ByteStream* pByteStream);

/**
 * [PUBLIC]
 * 
 * Consume one byte from back of the stream
 * 
 * @param pByteStream ByteStream instance
 * 
 * @return byte
 */
uint8_t ByteStream_Get(ByteStream* pByteStream);

/**
 * [PUBLIC]
 *
 * Put one byte in front of the stream
 *
 * @param pByteStream ByteStream instance
 * @param b The byte to put
 */
void ByteStream_Put(ByteStream* pByteStream, uint8_t b);

/**
 * [PUBLIC]
 *
 * Destroy the ByteStream object and internal buffer
 *
 * @param pByteStream ByteStream instance
 */
void ByteStream_Destroy(ByteStream* pByteStream);

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
size_t ByteStream_Length(ByteStream* pByteStream);

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
void ByteStream_Copy(ByteStream* pByteStream, uint8_t* pDest);

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
void ByteStream_SetBuffer(ByteStream* pByteStream, uint8_t* pSrc, size_t len);

#endif  /* _BYTESTREAM_H_ */
