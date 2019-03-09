#ifndef STDAFX_H
#define STDAFX_H

//#define WINVER NTDDI_VISTA
// #define _WIN32_WINNT NTDDI_VISTA

#define NTDDI_VERSION NTDDI_VISTA
#define _WINNT_WIN32 _WIN32_WINNT_VISTA

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shobjidl.h>
#include <commctrl.h>
#include <shlobj.h>

typedef struct _tagMOUSEEVENT
{
    HWND hwnd;
    LPARAM lParam;
} MOUSEEVENT;

#endif /* STDAFX_H */
