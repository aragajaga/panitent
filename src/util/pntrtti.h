#pragma once

typedef struct PntRTTI PntRTTI;

PntRTTI* PntRTTI_Create();
void PntRTTI_SetTypeName(PntRTTI* pPntRTTI, PCWSTR pszString);
DWORD PntRTTI_GetTypeName(PntRTTI* pPntRTTI, PWSTR pszString);
