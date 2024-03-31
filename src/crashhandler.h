#pragma once

void CreateDump(EXCEPTION_POINTERS* exceptionPointers);
LONG WINAPI PanitentUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers);
