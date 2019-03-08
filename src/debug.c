#include "debug.h"

ASSOC memProtectConst[13] = {
    {PAGE_EXECUTE,              "PAGE_EXECUTE"},
    {PAGE_EXECUTE_READ,         "PAGE_EXECUTE_READ"},
    {PAGE_EXECUTE_READWRITE,    "PAGE_EXECUTE_READWRITE"},
    {PAGE_EXECUTE_WRITECOPY,    "PAGE_EXECUTE_WRITECOPY"},
    {PAGE_NOACCESS,             "PAGE_NOACCESS"},
    {PAGE_READONLY,             "PAGE_READONLY"},
    {PAGE_READWRITE,            "PAGE_READWRITE"},
    {PAGE_WRITECOPY,            "PAGE_WRITECOPY"},
    {0x40000000,                "PAGE_TARGETS_INVALID"},
    {0x40000000,                "PAGE_TARGETS_NO_UPDATE"},
    {PAGE_GUARD,                "PAGE_GUARD"},
    {PAGE_NOCACHE,              "PAGE_NOCACHE"},
    {PAGE_WRITECOMBINE,         "PAGE_WRITECOMBINE"}
};

ASSOC mbiStateConst[3] = {
    {MEM_COMMIT,    "MEM_COMMIT"},
    {MEM_FREE,      "MEM_FREE"},
    {MEM_RESERVE,   "MEM_RESERVE"}
};

ASSOC mbiTypeConst[3] = {
    {MEM_IMAGE,     "MEM_IMAGE"},
    {MEM_MAPPED,    "MEM_MAPPED"},
    {MEM_PRIVATE,   "MEM_PRIVATE"}
};        

void DebugVirtualMemoryInfo(void *memPtr)
{
    MEMORY_BASIC_INFORMATION mbi = {0};

    if (VirtualQuery((LPCVOID)memPtr, &mbi, sizeof(mbi)))
    {
        unsigned char bFirst;

        printf("Memory Basic Information\n");

        printf("BaseAddress:\t\t0x%08lX\n",         (unsigned long)mbi.BaseAddress);

        printf("AllocationBase:\t\t0x%08lX\n",      (unsigned long)mbi.AllocationBase);

        printf("AllocationProtect:\t0x%08lX\t(",    (unsigned long)mbi.AllocationProtect);
        EXPLAINMASK(mbi.AllocationProtect, memProtectConst);

        printf("RegionSize:\t\t%10ld\n",            mbi.RegionSize);

        printf("State:\t\t\t%10ld\t(",              mbi.State);
        EXPLAINMASK(mbi.State, mbiStateConst);

        printf("Type:\t\t\t%10ld\t(",               mbi.Type);
        EXPLAINMASK(mbi.Type, mbiTypeConst);
    }
    else {
        printf("[DebugVirtualMemoryInfo] VirtualQuery Error: %ld\n", GetLastError());
    }
}

void DebugPrintRect(RECT *rc)
{
    printf("\tright:\t%ld\n"
           "\ttop:\t%ld\n"
           "\tleft:\t%ld\n"
           "\tbottom:\t%ld\n",
           rc->right, rc->top, rc->left, rc->bottom);
}
