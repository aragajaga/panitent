#ifndef MINGW_MISSING_H
#define MINGW_MISSING_H

#include <windows.h>

#ifndef OFFSETOFCLASS

#define OFFSETOFCLASS(base, derived) \
    ((DWORD)(DWORD_PTR)((base*)((derived*)8))-8)

#endif

typedef struct
{
    const IID * piid;
    int         dwOffset;
} QITAB, *LPQITAB;
typedef const QITAB *LPCQITAB;

#define QITABENTMULTI(Cthis, Ifoo, Iimpl) \
    { (IID*) &IID_##Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }
#define QITABENT(Cthis, Ifoo) QITABENTMULTI(Cthis, Ifoo, Ifoo)

STDAPI QISearch(void* that, LPCQITAB pqit, REFIID riid, void **ppv);

#endif /* MINGW_MISSING_H */
