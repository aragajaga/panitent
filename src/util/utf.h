#ifndef _UTF_H_
#define _UTF_H_

#pragma once

#include <stdint.h>

#include "bytestream.h"

uint32_t ReadCodepointFromUTF8Stream(ByteStream* pStream);
void PutCodepointIntoUTF8Stream(ByteStream* pStream, uint32_t codepoint);
void PutUTF16StringIntoUTF8Stream(ByteStream* pStream, const uint16_t* utf16_string);
void PutUTF32StringIntoByteStream(ByteStream* pByteStream, const uint32_t* str, int len);
void PutUTF16StringIntoByteStream(ByteStream* pByteStream, const uint16_t* str, int len);

#endif  /* _UTF_H_ */
