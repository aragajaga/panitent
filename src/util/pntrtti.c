#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "bytestream.h"
#include "utf.h"
#include "pntstring.h"
#include "hashmap.h"

#include <Windows.h>

typedef struct PntRTTI PntRTTI;
struct PntRTTI {
    PntString* typeName;
};

void PntRTTI_Init(PntRTTI* pPntRTTI);

PntRTTI* PntRTTI_Create()
{
    PntRTTI* pPntRTTI = malloc(sizeof(PntRTTI));
    if (pPntRTTI)
    {
        PntRTTI_Init(pPntRTTI);
    }

    return pPntRTTI;
}

void PntRTTI_Init(PntRTTI* pPntRTTI)
{
    memset(pPntRTTI, 0, sizeof(PntRTTI));

    pPntRTTI->typeName = PntString_Create();
}

void PntRTTI_SetTypeName(PntRTTI* pPntRTTI, PCWSTR pszString)
{
    PntString_SetFromUTF16(pPntRTTI->typeName, pszString);
}

DWORD PntRTTI_GetTypeName(PntRTTI* pPntRTTI, PWSTR pszString)
{
    return (DWORD)PntString_CopyToUTF16String(pPntRTTI->typeName, (uint16_t*)pszString);
}
