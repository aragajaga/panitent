#include "precomp.h"

#include "crashhandler.h"

#include <DbgHelp.h>
#include <stdarg.h>

static const WCHAR g_szCrashWindowClassName[] = L"PanitentCrashReportWindow";
static const int g_nCrashButtonWidth = 96;
static const int g_nCrashButtonHeight = 28;
static const int g_nCrashPadding = 12;

#define PANITENT_CRASH_REPORT_CCH 65536
#define PANITENT_CRASH_MAX_FRAMES 64
#define IDC_CRASH_EDIT 1001
#define IDC_CRASH_COPY 1002
#define IDC_CRASH_CLOSE 1003

typedef struct PanitentCrashWindowState PanitentCrashWindowState;
struct PanitentCrashWindowState
{
    HWND hWndEdit;
    HWND hWndCopyButton;
    HWND hWndCloseButton;
    HFONT hFont;
    const WCHAR* pszReport;
};

typedef struct PanitentCrashResolvedAddress PanitentCrashResolvedAddress;
struct PanitentCrashResolvedAddress
{
    DWORD64 dwAddress;
    DWORD64 dwSymbolDisplacement;
    DWORD dwLineDisplacement;
    DWORD64 dwBaseOfImage;
    DWORD dwImageSize;
    DWORD dwSymType;
    DWORD dwLineNumber;
    BOOL fHasSymbol;
    BOOL fHasLine;
    BOOL fHasModule;
    WCHAR szModule[256];
    WCHAR szSymbol[512];
    WCHAR szSourceFile[MAX_PATH];
    WCHAR szImagePath[MAX_PATH];
    WCHAR szPdbPath[MAX_PATH];
};

static LONG g_fCrashHandling = 0;
static WCHAR g_szCrashReport[PANITENT_CRASH_REPORT_CCH];

static BOOL Panitent_CrashHandler_CreateDumpInternal(EXCEPTION_POINTERS* exceptionPointers, PWSTR pszDumpPath, size_t cchDumpPath);
static void Panitent_CrashHandler_AppendFormat(PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset, PCWSTR pszFormat, ...);
static PCWSTR Panitent_CrashHandler_GetExceptionCodeName(DWORD dwExceptionCode);
static BOOL Panitent_CrashHandler_InitializeSymbols(HANDLE hProcess);
static void Panitent_CrashHandler_ResolveAddress(HANDLE hProcess, DWORD64 dwAddress, PanitentCrashResolvedAddress* pResolvedAddress);
static void Panitent_CrashHandler_AppendResolvedAddress(PCWSTR pszPrefix, const PanitentCrashResolvedAddress* pResolvedAddress, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset);
static void Panitent_CrashHandler_FormatExceptionRecord(EXCEPTION_RECORD* pExceptionRecord, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset);
static void Panitent_CrashHandler_FormatRegisters(EXCEPTION_POINTERS* exceptionPointers, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset);
static void Panitent_CrashHandler_AppendFrame(HANDLE hProcess, DWORD64 dwAddress, int nFrameIndex, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset);
static void Panitent_CrashHandler_FormatStackTrace(EXCEPTION_POINTERS* exceptionPointers, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset);
static void Panitent_CrashHandler_FormatStackSnapshot(EXCEPTION_POINTERS* exceptionPointers, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset);
static void Panitent_CrashHandler_BuildReport(EXCEPTION_POINTERS* exceptionPointers, PCWSTR pszDumpPath, PWSTR pszBuffer, size_t cchBuffer);
static BOOL Panitent_CrashHandler_CopyTextToClipboard(HWND hWndOwner, PCWSTR pszText);
static BOOL Panitent_CrashHandler_EnsureWindowClass(void);
static void Panitent_CrashHandler_ShowReportWindow(PCWSTR pszReport);
static LRESULT CALLBACK Panitent_CrashHandler_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void CreateDump(EXCEPTION_POINTERS* exceptionPointers)
{
    WCHAR szDumpPath[MAX_PATH] = L"";
    Panitent_CrashHandler_CreateDumpInternal(exceptionPointers, szDumpPath, ARRAYSIZE(szDumpPath));
}

BOOL Panitent_InstallCrashHandler(void)
{
    UINT uErrorMode = SetErrorMode(0);
    SetErrorMode(uErrorMode | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(PanitentUnhandledExceptionFilter);
    return TRUE;
}

LONG WINAPI PanitentUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
    WCHAR szDumpPath[MAX_PATH] = L"";

    if (InterlockedCompareExchange(&g_fCrashHandling, 1, 0) != 0)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    __try
    {
        Panitent_CrashHandler_CreateDumpInternal(exceptionPointers, szDumpPath, ARRAYSIZE(szDumpPath));
        Panitent_CrashHandler_BuildReport(exceptionPointers, szDumpPath, g_szCrashReport, ARRAYSIZE(g_szCrashReport));
        Panitent_CrashHandler_ShowReportWindow(g_szCrashReport);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        MessageBoxW(
            NULL,
            L"Panit.ent crashed, and the crash handler also failed while building the report.",
            L"Panit.ent Crash Handler",
            MB_OK | MB_ICONERROR | MB_TASKMODAL);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void Panitent_RaiseException(PCWSTR pszExceptionMessage)
{
    MessageBoxW(NULL, pszExceptionMessage, L"Panit.ent", MB_OK | MB_ICONERROR | MB_TASKMODAL);
}

static BOOL Panitent_CrashHandler_CreateDumpInternal(EXCEPTION_POINTERS* exceptionPointers, PWSTR pszDumpPath, size_t cchDumpPath)
{
    SYSTEMTIME st = { 0 };
    WCHAR szTempPath[MAX_PATH] = L"";
    HANDLE hDumpFile = INVALID_HANDLE_VALUE;
    BOOL fDumpWritten = FALSE;

    if (pszDumpPath && cchDumpPath > 0)
    {
        pszDumpPath[0] = L'\0';
    }

    GetLocalTime(&st);
    if (!GetTempPathW(ARRAYSIZE(szTempPath), szTempPath))
    {
        StringCchCopyW(szTempPath, ARRAYSIZE(szTempPath), L".\\");
    }

    if (pszDumpPath && cchDumpPath > 0)
    {
        StringCchPrintfW(
            pszDumpPath,
            cchDumpPath,
            L"%spanitent_%04u%02u%02u_%02u%02u%02u_%lu.dmp",
            szTempPath,
            st.wYear,
            st.wMonth,
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond,
            GetCurrentProcessId());
    }

    hDumpFile = CreateFileW(
        (pszDumpPath && pszDumpPath[0] != L'\0') ? pszDumpPath : L"panitent_crash.dmp",
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDumpFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = { 0 };
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = exceptionPointers;
        exceptionInfo.ClientPointers = TRUE;

        fDumpWritten = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hDumpFile,
            MiniDumpWithDataSegs |
                MiniDumpWithHandleData |
                MiniDumpWithIndirectlyReferencedMemory |
                MiniDumpWithThreadInfo |
                MiniDumpWithUnloadedModules,
            &exceptionInfo,
            NULL,
            NULL);
        FlushFileBuffers(hDumpFile);
        CloseHandle(hDumpFile);
    }

    return fDumpWritten;
}

static void Panitent_CrashHandler_AppendFormat(PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset, PCWSTR pszFormat, ...)
{
    HRESULT hr = S_OK;
    WCHAR* pszWrite = NULL;
    size_t cchRemaining = 0;
    va_list args;

    if (!pszBuffer || !pnOffset || *pnOffset >= cchBuffer)
    {
        return;
    }

    pszWrite = pszBuffer + *pnOffset;
    cchRemaining = cchBuffer - *pnOffset;

    va_start(args, pszFormat);
    hr = StringCchVPrintfExW(pszWrite, cchRemaining, &pszWrite, &cchRemaining, 0, pszFormat, args);
    va_end(args);

    if (SUCCEEDED(hr))
    {
        *pnOffset = (size_t)(pszWrite - pszBuffer);
    }
    else
    {
        pszBuffer[cchBuffer - 1] = L'\0';
        *pnOffset = wcslen(pszBuffer);
    }
}

static PCWSTR Panitent_CrashHandler_GetExceptionCodeName(DWORD dwExceptionCode)
{
    switch (dwExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        return L"EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:
        return L"EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return L"EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        return L"EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return L"EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:
        return L"EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:
        return L"EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:
        return L"EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:
        return L"EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:
        return L"EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return L"EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:
        return L"EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return L"EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:
        return L"EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:
        return L"EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        return L"EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION:
        return L"EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP:
        return L"EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:
        return L"EXCEPTION_STACK_OVERFLOW";
    default:
        return L"UNKNOWN_EXCEPTION";
    }
}

static BOOL Panitent_CrashHandler_InitializeSymbols(HANDLE hProcess)
{
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_FAIL_CRITICAL_ERRORS);
    return SymInitialize(hProcess, NULL, TRUE);
}

static void Panitent_CrashHandler_ResolveAddress(HANDLE hProcess, DWORD64 dwAddress, PanitentCrashResolvedAddress* pResolvedAddress)
{
    BYTE symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = { 0 };
    SYMBOL_INFO* pSymbol = (SYMBOL_INFO*)symbolBuffer;
    IMAGEHLP_LINE64 lineInfo = { 0 };
    IMAGEHLP_MODULE64 moduleInfo = { 0 };

    if (!pResolvedAddress)
    {
        return;
    }

    memset(pResolvedAddress, 0, sizeof(*pResolvedAddress));
    pResolvedAddress->dwAddress = dwAddress;

    if (!hProcess || !dwAddress)
    {
        return;
    }

    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;
    if (SymFromAddr(hProcess, dwAddress, &pResolvedAddress->dwSymbolDisplacement, pSymbol))
    {
        MultiByteToWideChar(CP_ACP, 0, pSymbol->Name, -1, pResolvedAddress->szSymbol, ARRAYSIZE(pResolvedAddress->szSymbol));
        pResolvedAddress->fHasSymbol = TRUE;
    }

    lineInfo.SizeOfStruct = sizeof(lineInfo);
    if (SymGetLineFromAddr64(hProcess, dwAddress, &pResolvedAddress->dwLineDisplacement, &lineInfo))
    {
        MultiByteToWideChar(CP_ACP, 0, lineInfo.FileName, -1, pResolvedAddress->szSourceFile, ARRAYSIZE(pResolvedAddress->szSourceFile));
        pResolvedAddress->dwLineNumber = lineInfo.LineNumber;
        pResolvedAddress->fHasLine = TRUE;
    }

    moduleInfo.SizeOfStruct = sizeof(moduleInfo);
    if (SymGetModuleInfo64(hProcess, dwAddress, &moduleInfo))
    {
        const char* pszImagePath = moduleInfo.LoadedImageName[0] ? moduleInfo.LoadedImageName :
            (moduleInfo.ImageName[0] ? moduleInfo.ImageName : NULL);
        const char* pszPdbPath = moduleInfo.LoadedPdbName[0] ? moduleInfo.LoadedPdbName : NULL;
        const char* pszModuleName = moduleInfo.ModuleName[0] ? moduleInfo.ModuleName :
            (moduleInfo.ImageName[0] ? moduleInfo.ImageName : NULL);

        if (pszModuleName)
        {
            MultiByteToWideChar(CP_ACP, 0, pszModuleName, -1, pResolvedAddress->szModule, ARRAYSIZE(pResolvedAddress->szModule));
        }
        if (pszImagePath)
        {
            MultiByteToWideChar(CP_ACP, 0, pszImagePath, -1, pResolvedAddress->szImagePath, ARRAYSIZE(pResolvedAddress->szImagePath));
        }
        if (pszPdbPath)
        {
            MultiByteToWideChar(CP_ACP, 0, pszPdbPath, -1, pResolvedAddress->szPdbPath, ARRAYSIZE(pResolvedAddress->szPdbPath));
        }

        pResolvedAddress->dwBaseOfImage = moduleInfo.BaseOfImage;
        pResolvedAddress->dwImageSize = moduleInfo.ImageSize;
        pResolvedAddress->dwSymType = moduleInfo.SymType;
        pResolvedAddress->fHasModule = TRUE;
    }
}

static void Panitent_CrashHandler_AppendResolvedAddress(PCWSTR pszPrefix, const PanitentCrashResolvedAddress* pResolvedAddress, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset)
{
    if (!pResolvedAddress)
    {
        return;
    }

    Panitent_CrashHandler_AppendFormat(
        pszBuffer,
        cchBuffer,
        pnOffset,
        L"%s%p",
        pszPrefix ? pszPrefix : L"",
        (void*)(UINT_PTR)pResolvedAddress->dwAddress);

    if (pResolvedAddress->fHasModule || pResolvedAddress->fHasSymbol)
    {
        Panitent_CrashHandler_AppendFormat(
            pszBuffer,
            cchBuffer,
            pnOffset,
            L"  %s!%s",
            pResolvedAddress->fHasModule && pResolvedAddress->szModule[0] ? pResolvedAddress->szModule : L"?",
            pResolvedAddress->fHasSymbol && pResolvedAddress->szSymbol[0] ? pResolvedAddress->szSymbol : L"?");

        if (pResolvedAddress->fHasSymbol && pResolvedAddress->dwSymbolDisplacement > 0)
        {
            Panitent_CrashHandler_AppendFormat(
                pszBuffer,
                cchBuffer,
                pnOffset,
                L" + 0x%I64x",
                pResolvedAddress->dwSymbolDisplacement);
        }
    }
    Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, pnOffset, L"\r\n");

    if (pResolvedAddress->fHasModule)
    {
        Panitent_CrashHandler_AppendFormat(
            pszBuffer,
            cchBuffer,
            pnOffset,
            L"      module: %s [base=%p size=0x%08lX symtype=%lu]\r\n",
            pResolvedAddress->szImagePath[0] ? pResolvedAddress->szImagePath : pResolvedAddress->szModule,
            (void*)(UINT_PTR)pResolvedAddress->dwBaseOfImage,
            pResolvedAddress->dwImageSize,
            pResolvedAddress->dwSymType);

        if (pResolvedAddress->szPdbPath[0] != L'\0')
        {
            Panitent_CrashHandler_AppendFormat(
                pszBuffer,
                cchBuffer,
                pnOffset,
                L"      pdb:    %s\r\n",
                pResolvedAddress->szPdbPath);
        }
    }

    if (pResolvedAddress->fHasLine)
    {
        Panitent_CrashHandler_AppendFormat(
            pszBuffer,
            cchBuffer,
            pnOffset,
            L"      source: %s:%lu",
            pResolvedAddress->szSourceFile,
            pResolvedAddress->dwLineNumber);
        if (pResolvedAddress->dwLineDisplacement > 0)
        {
            Panitent_CrashHandler_AppendFormat(
                pszBuffer,
                cchBuffer,
                pnOffset,
                L" + 0x%lX",
                pResolvedAddress->dwLineDisplacement);
        }
        Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, pnOffset, L"\r\n");
    }
}

static void Panitent_CrashHandler_FormatExceptionRecord(EXCEPTION_RECORD* pExceptionRecord, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset)
{
    DWORD i = 0;

    if (!pExceptionRecord)
    {
        return;
    }

    Panitent_CrashHandler_AppendFormat(
        pszBuffer,
        cchBuffer,
        pnOffset,
        L"Exception:\r\n"
        L"  code:    %s (0x%08lX)\r\n"
        L"  flags:   0x%08lX\r\n"
        L"  address: %p\r\n",
        Panitent_CrashHandler_GetExceptionCodeName(pExceptionRecord->ExceptionCode),
        pExceptionRecord->ExceptionCode,
        pExceptionRecord->ExceptionFlags,
        pExceptionRecord->ExceptionAddress);

    if (pExceptionRecord->ExceptionRecord)
    {
        Panitent_CrashHandler_AppendFormat(
            pszBuffer,
            cchBuffer,
            pnOffset,
            L"  nested:  %p\r\n",
            pExceptionRecord->ExceptionRecord);
    }

    if ((pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
            pExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) &&
        pExceptionRecord->NumberParameters >= 2)
    {
        PCWSTR pszViolationKind = L"read";
        if (pExceptionRecord->ExceptionInformation[0] == 1)
        {
            pszViolationKind = L"write";
        }
        else if (pExceptionRecord->ExceptionInformation[0] == 8)
        {
            pszViolationKind = L"execute";
        }

        Panitent_CrashHandler_AppendFormat(
            pszBuffer,
            cchBuffer,
            pnOffset,
            L"  violation: attempted to %s address %p\r\n",
            pszViolationKind,
            (void*)(UINT_PTR)pExceptionRecord->ExceptionInformation[1]);
    }

    if (pExceptionRecord->NumberParameters > 0)
    {
        Panitent_CrashHandler_AppendFormat(
            pszBuffer,
            cchBuffer,
            pnOffset,
            L"  params (%lu):\r\n",
            pExceptionRecord->NumberParameters);
        for (i = 0; i < pExceptionRecord->NumberParameters; ++i)
        {
            Panitent_CrashHandler_AppendFormat(
                pszBuffer,
                cchBuffer,
                pnOffset,
                L"    [%lu] = %p\r\n",
                i,
                (void*)(UINT_PTR)pExceptionRecord->ExceptionInformation[i]);
        }
    }

    Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, pnOffset, L"\r\n");
}

static void Panitent_CrashHandler_FormatRegisters(EXCEPTION_POINTERS* exceptionPointers, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset)
{
    CONTEXT* pContext = NULL;

    if (!exceptionPointers || !exceptionPointers->ContextRecord)
    {
        return;
    }

    pContext = exceptionPointers->ContextRecord;

#if defined(_M_X64)
    Panitent_CrashHandler_AppendFormat(
        pszBuffer,
        cchBuffer,
        pnOffset,
        L"Registers:\r\n"
        L"  RIP=%p RSP=%p RBP=%p\r\n"
        L"  RAX=%p RBX=%p RCX=%p RDX=%p\r\n"
        L"  RSI=%p RDI=%p R8=%p R9=%p\r\n"
        L"  R10=%p R11=%p R12=%p R13=%p\r\n"
        L"  R14=%p R15=%p\r\n\r\n",
        (void*)(UINT_PTR)pContext->Rip,
        (void*)(UINT_PTR)pContext->Rsp,
        (void*)(UINT_PTR)pContext->Rbp,
        (void*)(UINT_PTR)pContext->Rax,
        (void*)(UINT_PTR)pContext->Rbx,
        (void*)(UINT_PTR)pContext->Rcx,
        (void*)(UINT_PTR)pContext->Rdx,
        (void*)(UINT_PTR)pContext->Rsi,
        (void*)(UINT_PTR)pContext->Rdi,
        (void*)(UINT_PTR)pContext->R8,
        (void*)(UINT_PTR)pContext->R9,
        (void*)(UINT_PTR)pContext->R10,
        (void*)(UINT_PTR)pContext->R11,
        (void*)(UINT_PTR)pContext->R12,
        (void*)(UINT_PTR)pContext->R13,
        (void*)(UINT_PTR)pContext->R14,
        (void*)(UINT_PTR)pContext->R15);
#elif defined(_M_IX86)
    Panitent_CrashHandler_AppendFormat(
        pszBuffer,
        cchBuffer,
        pnOffset,
        L"Registers:\r\n"
        L"  EIP=%p ESP=%p EBP=%p\r\n"
        L"  EAX=%p EBX=%p ECX=%p EDX=%p\r\n"
        L"  ESI=%p EDI=%p\r\n\r\n",
        (void*)(UINT_PTR)pContext->Eip,
        (void*)(UINT_PTR)pContext->Esp,
        (void*)(UINT_PTR)pContext->Ebp,
        (void*)(UINT_PTR)pContext->Eax,
        (void*)(UINT_PTR)pContext->Ebx,
        (void*)(UINT_PTR)pContext->Ecx,
        (void*)(UINT_PTR)pContext->Edx,
        (void*)(UINT_PTR)pContext->Esi,
        (void*)(UINT_PTR)pContext->Edi);
#endif
}

static void Panitent_CrashHandler_AppendFrame(HANDLE hProcess, DWORD64 dwAddress, int nFrameIndex, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset)
{
    PanitentCrashResolvedAddress resolvedAddress = { 0 };

    if (!dwAddress)
    {
        return;
    }

    Panitent_CrashHandler_ResolveAddress(hProcess, dwAddress, &resolvedAddress);
    {
        WCHAR szPrefix[32] = L"";
        StringCchPrintfW(szPrefix, ARRAYSIZE(szPrefix), L"#%02d ", nFrameIndex);
        Panitent_CrashHandler_AppendResolvedAddress(szPrefix, &resolvedAddress, pszBuffer, cchBuffer, pnOffset);
    }
}

static void Panitent_CrashHandler_FormatStackTrace(EXCEPTION_POINTERS* exceptionPointers, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset)
{
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();
    CONTEXT context = { 0 };
    STACKFRAME64 stackFrame = { 0 };
    DWORD dwMachineType = 0;
    DWORD64 dwPreviousAddress = 0;
    int i = 0;
    BOOL fSymbolsInitialized = FALSE;

    if (!exceptionPointers || !exceptionPointers->ContextRecord)
    {
        return;
    }

    context = *exceptionPointers->ContextRecord;
    fSymbolsInitialized = Panitent_CrashHandler_InitializeSymbols(hProcess);

    Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, pnOffset, L"Stack trace:\r\n");
    if (!fSymbolsInitialized)
    {
        Panitent_CrashHandler_AppendFormat(
            pszBuffer,
            cchBuffer,
            pnOffset,
            L"  Symbol initialization failed. GetLastError=%lu\r\n",
            GetLastError());
        return;
    }

#if defined(_M_X64)
    dwMachineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rbp ? context.Rbp : context.Rsp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_IX86)
    dwMachineType = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#else
    Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, pnOffset, L"  Unsupported architecture.\r\n");
    SymCleanup(hProcess);
    return;
#endif

    for (i = 0; i < PANITENT_CRASH_MAX_FRAMES; ++i)
    {
        DWORD64 dwAddress = stackFrame.AddrPC.Offset;

        if (!dwAddress || dwAddress == dwPreviousAddress)
        {
            break;
        }

        Panitent_CrashHandler_AppendFrame(hProcess, dwAddress, i, pszBuffer, cchBuffer, pnOffset);
        dwPreviousAddress = dwAddress;

        if (!StackWalk64(
                dwMachineType,
                hProcess,
                hThread,
                &stackFrame,
                &context,
                NULL,
                SymFunctionTableAccess64,
                SymGetModuleBase64,
                NULL))
        {
            break;
        }
    }

    Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, pnOffset, L"\r\n");
    SymCleanup(hProcess);
}

static void Panitent_CrashHandler_FormatStackSnapshot(EXCEPTION_POINTERS* exceptionPointers, PWSTR pszBuffer, size_t cchBuffer, size_t* pnOffset)
{
    HANDLE hProcess = GetCurrentProcess();
    ULONG_PTR nStackPointer = 0;
    DWORD i = 0;
    BOOL fSymbolsInitialized = FALSE;

    if (!exceptionPointers || !exceptionPointers->ContextRecord)
    {
        return;
    }

#if defined(_M_X64)
    nStackPointer = (ULONG_PTR)exceptionPointers->ContextRecord->Rsp;
#elif defined(_M_IX86)
    nStackPointer = (ULONG_PTR)exceptionPointers->ContextRecord->Esp;
#endif

    if (!nStackPointer)
    {
        return;
    }

    fSymbolsInitialized = Panitent_CrashHandler_InitializeSymbols(hProcess);

    Panitent_CrashHandler_AppendFormat(
        pszBuffer,
        cchBuffer,
        pnOffset,
        L"Stack memory snapshot:\r\n");

    for (i = 0; i < 12; ++i)
    {
        ULONG_PTR nValue = 0;
        SIZE_T nBytesRead = 0;
        ULONG_PTR nSlotAddress = nStackPointer + i * sizeof(ULONG_PTR);

        if (!ReadProcessMemory(hProcess, (LPCVOID)nSlotAddress, &nValue, sizeof(nValue), &nBytesRead) ||
            nBytesRead != sizeof(nValue))
        {
            Panitent_CrashHandler_AppendFormat(
                pszBuffer,
                cchBuffer,
                pnOffset,
                L"  [%p] = <unreadable>\r\n",
                (void*)nSlotAddress);
            continue;
        }

        Panitent_CrashHandler_AppendFormat(
            pszBuffer,
            cchBuffer,
            pnOffset,
            L"  [%p] = %p",
            (void*)nSlotAddress,
            (void*)nValue);

        if (fSymbolsInitialized && nValue != 0 && SymGetModuleBase64(hProcess, (DWORD64)nValue) != 0)
        {
            PanitentCrashResolvedAddress resolvedAddress = { 0 };
            Panitent_CrashHandler_ResolveAddress(hProcess, (DWORD64)nValue, &resolvedAddress);
            if (resolvedAddress.fHasModule || resolvedAddress.fHasSymbol)
            {
                Panitent_CrashHandler_AppendFormat(
                    pszBuffer,
                    cchBuffer,
                    pnOffset,
                    L"  -> %s!%s",
                    resolvedAddress.fHasModule && resolvedAddress.szModule[0] ? resolvedAddress.szModule : L"?",
                    resolvedAddress.fHasSymbol && resolvedAddress.szSymbol[0] ? resolvedAddress.szSymbol : L"?");
                if (resolvedAddress.fHasSymbol && resolvedAddress.dwSymbolDisplacement > 0)
                {
                    Panitent_CrashHandler_AppendFormat(
                        pszBuffer,
                        cchBuffer,
                        pnOffset,
                        L" + 0x%I64x",
                        resolvedAddress.dwSymbolDisplacement);
                }
            }
        }

        Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, pnOffset, L"\r\n");
    }

    Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, pnOffset, L"\r\n");
    if (fSymbolsInitialized)
    {
        SymCleanup(hProcess);
    }
}

static void Panitent_CrashHandler_BuildReport(EXCEPTION_POINTERS* exceptionPointers, PCWSTR pszDumpPath, PWSTR pszBuffer, size_t cchBuffer)
{
    SYSTEMTIME st = { 0 };
    EXCEPTION_RECORD* pExceptionRecord = NULL;
    size_t nOffset = 0;
    WCHAR szExePath[MAX_PATH] = L"";
    PanitentCrashResolvedAddress faultAddress = { 0 };
    HANDLE hProcess = GetCurrentProcess();

    if (!pszBuffer || cchBuffer == 0)
    {
        return;
    }

    pszBuffer[0] = L'\0';
    GetLocalTime(&st);
    GetModuleFileNameW(NULL, szExePath, ARRAYSIZE(szExePath));

    pExceptionRecord = exceptionPointers ? exceptionPointers->ExceptionRecord : NULL;
    Panitent_CrashHandler_AppendFormat(
        pszBuffer,
        cchBuffer,
        &nOffset,
        L"Panit.ent crashed and intercepted an unhandled exception.\r\n\r\n");
    Panitent_CrashHandler_AppendFormat(
        pszBuffer,
        cchBuffer,
        &nOffset,
        L"Timestamp: %04u-%02u-%02u %02u:%02u:%02u\r\n"
        L"Process ID: %lu\r\n"
        L"Thread ID: %lu\r\n"
        L"Executable: %s\r\n",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        GetCurrentProcessId(),
        GetCurrentThreadId(),
        szExePath);

    if (pszDumpPath && pszDumpPath[0] != L'\0')
    {
        Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, &nOffset, L"Minidump: %s\r\n", pszDumpPath);
    }
    Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, &nOffset, L"\r\n");

    if (pExceptionRecord)
    {
        Panitent_CrashHandler_FormatExceptionRecord(pExceptionRecord, pszBuffer, cchBuffer, &nOffset);
        if (Panitent_CrashHandler_InitializeSymbols(hProcess))
        {
            Panitent_CrashHandler_ResolveAddress(hProcess, (DWORD64)(UINT_PTR)pExceptionRecord->ExceptionAddress, &faultAddress);
            SymCleanup(hProcess);
        }
        Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, &nOffset, L"Faulting address details:\r\n");
        Panitent_CrashHandler_AppendResolvedAddress(L"  ", &faultAddress, pszBuffer, cchBuffer, &nOffset);
        Panitent_CrashHandler_AppendFormat(pszBuffer, cchBuffer, &nOffset, L"\r\n");
    }

    Panitent_CrashHandler_FormatRegisters(exceptionPointers, pszBuffer, cchBuffer, &nOffset);
    Panitent_CrashHandler_FormatStackTrace(exceptionPointers, pszBuffer, cchBuffer, &nOffset);
    Panitent_CrashHandler_FormatStackSnapshot(exceptionPointers, pszBuffer, cchBuffer, &nOffset);
    Panitent_CrashHandler_AppendFormat(
        pszBuffer,
        cchBuffer,
        &nOffset,
        L"Close this window to terminate the process.\r\n");
}

static BOOL Panitent_CrashHandler_CopyTextToClipboard(HWND hWndOwner, PCWSTR pszText)
{
    SIZE_T cbText = 0;
    HGLOBAL hGlobal = NULL;
    WCHAR* pszClipboardText = NULL;

    if (!pszText)
    {
        return FALSE;
    }

    cbText = (wcslen(pszText) + 1) * sizeof(WCHAR);
    hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, cbText);
    if (!hGlobal)
    {
        return FALSE;
    }

    pszClipboardText = (WCHAR*)GlobalLock(hGlobal);
    if (!pszClipboardText)
    {
        GlobalFree(hGlobal);
        return FALSE;
    }

    memcpy(pszClipboardText, pszText, cbText);
    GlobalUnlock(hGlobal);

    if (!OpenClipboard(hWndOwner))
    {
        GlobalFree(hGlobal);
        return FALSE;
    }

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hGlobal);
    CloseClipboard();
    return TRUE;
}

static BOOL Panitent_CrashHandler_EnsureWindowClass(void)
{
    WNDCLASSEXW wcex = { 0 };

    if (GetClassInfoExW(GetModuleHandleW(NULL), g_szCrashWindowClassName, &wcex))
    {
        return TRUE;
    }

    wcex.cbSize = sizeof(wcex);
    wcex.lpfnWndProc = Panitent_CrashHandler_WndProc;
    wcex.hInstance = GetModuleHandleW(NULL);
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hIcon = LoadIconW(NULL, IDI_ERROR);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = g_szCrashWindowClassName;
    wcex.style = CS_HREDRAW | CS_VREDRAW;

    return RegisterClassExW(&wcex) != 0;
}

static void Panitent_CrashHandler_ShowReportWindow(PCWSTR pszReport)
{
    HWND hWnd = NULL;
    RECT rcWindow = { 0, 0, 900, 640 };
    MSG msg = { 0 };

    if (!Panitent_CrashHandler_EnsureWindowClass())
    {
        MessageBoxW(NULL, pszReport, L"Panit.ent Crash Handler", MB_OK | MB_ICONERROR | MB_TASKMODAL);
        return;
    }

    AdjustWindowRectEx(&rcWindow, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
    hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        g_szCrashWindowClassName,
        L"Panit.ent Crash Handler",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rcWindow.right - rcWindow.left,
        rcWindow.bottom - rcWindow.top,
        NULL,
        NULL,
        GetModuleHandleW(NULL),
        (LPVOID)pszReport);

    if (!hWnd)
    {
        MessageBoxW(NULL, pszReport, L"Panit.ent Crash Handler", MB_OK | MB_ICONERROR | MB_TASKMODAL);
        return;
    }

    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);

    while (IsWindow(hWnd) && GetMessageW(&msg, hWnd, 0, 0) > 0)
    {
        if (!IsDialogMessageW(hWnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

static LRESULT CALLBACK Panitent_CrashHandler_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PanitentCrashWindowState* pState = (PanitentCrashWindowState*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (message)
    {
    case WM_CREATE:
    {
        CREATESTRUCTW* pCreateStruct = (CREATESTRUCTW*)lParam;
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        pState = (PanitentCrashWindowState*)calloc(1, sizeof(PanitentCrashWindowState));
        if (!pState)
        {
            return -1;
        }

        pState->pszReport = (const WCHAR*)pCreateStruct->lpCreateParams;
        pState->hFont = CreateFontW(
            -14,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN,
            L"Consolas");
        if (!pState->hFont)
        {
            pState->hFont = hGuiFont;
        }

        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pState);

        pState->hWndEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_EDITW,
            pState->pszReport,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
            0,
            0,
            0,
            0,
            hWnd,
            (HMENU)(INT_PTR)IDC_CRASH_EDIT,
            GetModuleHandleW(NULL),
            NULL);
        pState->hWndCopyButton = CreateWindowExW(
            0,
            WC_BUTTONW,
            L"Copy",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            0,
            0,
            0,
            0,
            hWnd,
            (HMENU)(INT_PTR)IDC_CRASH_COPY,
            GetModuleHandleW(NULL),
            NULL);
        pState->hWndCloseButton = CreateWindowExW(
            0,
            WC_BUTTONW,
            L"Close",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            0,
            0,
            0,
            0,
            hWnd,
            (HMENU)(INT_PTR)IDC_CRASH_CLOSE,
            GetModuleHandleW(NULL),
            NULL);

        if (pState->hWndEdit)
        {
            SendMessageW(pState->hWndEdit, WM_SETFONT, (WPARAM)pState->hFont, TRUE);
            SendMessageW(pState->hWndEdit, EM_SETSEL, 0, 0);
        }
        if (pState->hWndCopyButton)
        {
            SendMessageW(pState->hWndCopyButton, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        }
        if (pState->hWndCloseButton)
        {
            SendMessageW(pState->hWndCloseButton, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        }

        return 0;
    }
    case WM_SIZE:
        if (pState)
        {
            RECT rcClient = { 0 };
            int nClientWidth = 0;
            int nClientHeight = 0;
            int nButtonTop = 0;

            GetClientRect(hWnd, &rcClient);
            nClientWidth = rcClient.right - rcClient.left;
            nClientHeight = rcClient.bottom - rcClient.top;
            nButtonTop = nClientHeight - g_nCrashPadding - g_nCrashButtonHeight;

            MoveWindow(
                pState->hWndEdit,
                g_nCrashPadding,
                g_nCrashPadding,
                nClientWidth - g_nCrashPadding * 2,
                max(0, nButtonTop - g_nCrashPadding * 2),
                TRUE);
            MoveWindow(
                pState->hWndCopyButton,
                nClientWidth - g_nCrashPadding * 2 - g_nCrashButtonWidth * 2,
                nButtonTop,
                g_nCrashButtonWidth,
                g_nCrashButtonHeight,
                TRUE);
            MoveWindow(
                pState->hWndCloseButton,
                nClientWidth - g_nCrashPadding - g_nCrashButtonWidth,
                nButtonTop,
                g_nCrashButtonWidth,
                g_nCrashButtonHeight,
                TRUE);
        }
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CRASH_COPY:
            if (pState)
            {
                Panitent_CrashHandler_CopyTextToClipboard(hWnd, pState->pszReport);
            }
            return 0;
        case IDC_CRASH_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_NCDESTROY:
        if (pState)
        {
            if (pState->hFont && pState->hFont != GetStockObject(DEFAULT_GUI_FONT))
            {
                DeleteObject(pState->hFont);
            }
            free(pState);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}
