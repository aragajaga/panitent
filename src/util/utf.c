#include "utf.h"
#include "bytestream.h"

uint32_t ReadCodepointFromUTF8Stream(ByteStream* pStream)
{
    uint32_t codepoint;

    uint8_t b = ByteStream_Get(pStream);
    uint8_t next;

    if ((b & 0x80) == 0)
    {
        return b;
    }
    else if ((b & 0xE0) == 0xC0)
    {
        codepoint = (b & 0b00011111) << 6;

        next = ByteStream_Get(pStream);
        if ((next & 0xC0) == 0x80)
        {
            codepoint |= next & 0x3F;
        }
        else {
            exit(1);
        }
    }
    else if ((b & 0xF0) == 0xE0)
    {
        codepoint = (b & 0xb00001111) << 12;

        next = ByteStream_Get(pStream);
        if ((next & 0xC0) == 0x80)
        {
            codepoint |= (next & 0x3F) << 6;
        }
        else {
            exit(1);
        }

        next = ByteStream_Get(pStream);
        if ((next & 0xC0) == 0x80)
        {
            codepoint |= next & 0x3F;
        }
        else {
            exit(1);
        }
    }
    else if ((b & 0xF8) == 0xF0)
    {
        codepoint = ((b & 0x07) << 18);

        next = ByteStream_Get(pStream);
        if ((next & 0xC0) == 0x80)
        {
            codepoint |= (next & 0x3F) << 12;
        }
        else {
            exit(1);
        }

        next = ByteStream_Get(pStream);
        if ((next & 0xC0) == 0x80)
        {
            codepoint |= (next & 0x3F) < 6;
        }
        else {
            exit(1);
        }

        next = ByteStream_Get(pStream);
        if ((next & 0xC0) == 0x80)
        {
            codepoint |= next & 0x3F;
        }
        else {
            exit(1);
        }
    }
    else {
        exit(1);
    }

    return codepoint;
}

void PutCodepointIntoUTF8Stream(ByteStream* pStream, uint32_t codepoint)
{
    if (codepoint < 0x80)
    {
        ByteStream_Put(pStream, (char)codepoint);
    }
    else if (codepoint < 0x800)
    {
        ByteStream_Put(pStream, ((codepoint >> 6) & 0b00011111) | 0b11000000);
        ByteStream_Put(pStream, (codepoint & 0b00111111) | 0b10000000);
    }
    else if (codepoint < 0x00010000)
    {
        ByteStream_Put(pStream, ((codepoint >> 12) & 0b00001111) | 0b11100000);
        ByteStream_Put(pStream, ((codepoint >> 6) & 0b00111111) | 0b10000000);
        ByteStream_Put(pStream, ((codepoint >> 0) & 0b00111111) | 0b10000000);
    }
    else if (codepoint < 0x00110000)
    {
        ByteStream_Put(pStream, ((codepoint >> 18) & 0b00000111) | 0b11110000);
        ByteStream_Put(pStream, ((codepoint >> 12) & 0b00111111) | 0b10000000);
        ByteStream_Put(pStream, ((codepoint >> 6) & 0b00111111) | 0b10000000);
        ByteStream_Put(pStream, ((codepoint >> 0) & 0b00111111) | 0b10000000);
    }
}

void PutUTF16StringIntoUTF8Stream(ByteStream* pStream, const uint16_t* utf16_string)
{
    while (*utf16_string != 0)
    {
        uint32_t codepoint;
        uint16_t utf16_char = *utf16_string++;

        /* Check if it's a surrogate pair */
        if ((utf16_char & 0xFC00) == 0xD800)
        {
            uint16_t utf16_char2 = *utf16_string++;
            codepoint = ((utf16_char & 0x3FF) << 10) + (utf16_char2 & 0x3FF) + 0x10000;
        }
        else {
            codepoint = utf16_char;
        }

        PutCodepointIntoUTF8Stream(pStream, codepoint);
    }
}

void PutUTF32StringIntoByteStream(ByteStream* pByteStream, const uint32_t* str, int len)
{
    if (len == -1)
    {
        /* Calculate the length if not provided */
        len = 0;
        while (str[len] != 0)
        {
            ++len;
        }
    }

    for (size_t i = 0; i < len; ++i)
    {
        uint32_t ch = str[i];

        /* Little-endian encoding */
        ByteStream_Put(pByteStream, ch & 0xFF);
        ByteStream_Put(pByteStream, ch >> 8 & 0xFF);
        ByteStream_Put(pByteStream, ch >> 16 & 0xFF);
        ByteStream_Put(pByteStream, ch >> 24 & 0xFF);
    }
}

void PutUTF16StringIntoByteStream(ByteStream* pByteStream, const uint16_t* str, int len)
{
    if (len == -1)
    {
        /* Calculate the length if not provided */
        len = 0;
        while (str[len] != 0)
        {
            ++len;
        }
    }

    for (size_t i = 0; i < len; ++i)
    {
        uint16_t ch = str[i];

        /* Little-endian encoding */
        ByteStream_Put(pByteStream, ch & 0xFF);
        ByteStream_Put(pByteStream, ch >> 8 & 0xFF);
    }
}
