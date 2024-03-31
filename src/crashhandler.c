#include "precomp.h"

#include "crashhandler.h"

#include <DbgHelp.h>

void CreateDump(EXCEPTION_POINTERS* exceptionPointers)
{
    SYSTEMTIME st;
    GetSystemTime(&st);

    WCHAR szDumpFileName[MAX_PATH];
    StringCchPrintf(szDumpFileName, MAX_PATH, L"panitent_%04d%02d%02d_%02d%02d%02d.dmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);


    HANDLE dumpFile = CreateFile(szDumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (dumpFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = { 0 };
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = exceptionPointers;
        exceptionInfo.ClientPointers = TRUE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpWithFullMemory, &exceptionInfo, NULL, NULL);
        CloseHandle(dumpFile);
    }
}

LONG WINAPI PanitentUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
    CreateDump(exceptionPointers);
    return EXCEPTION_EXECUTE_HANDLER;
}
