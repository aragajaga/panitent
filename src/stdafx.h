#ifndef STDAFX_H
#define STDAFX_H

#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#define NTDDI_VERSION NTDDI_VISTA
#define _WINNT_WIN32 _WIN32_WINNT_VISTA

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#ifdef __MINGW32__
#include <mingw_missing.h>
#endif

#include <shobjidl.h>
#include <commctrl.h>
#include <shlobj.h>

#endif /* STDAFX_H */
