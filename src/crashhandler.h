#pragma once

#include "precomp.h"

void CreateDump(EXCEPTION_POINTERS* exceptionPointers);
BOOL Panitent_InstallCrashHandler(void);
LONG WINAPI PanitentUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers);
void Panitent_RaiseException(PCWSTR pszExceptionMessage);
