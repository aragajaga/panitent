#ifndef PANITENT_PRECOMP_H_
#define PANITENT_PRECOMP_H_

#define NTDDI_VERSION NTDDI_WIN7
#define _WIN32_WINNT  _WIN32_WINNT_WIN7
#define WINVER        _WIN32_WINNT_WIN7

#ifndef UNICODE
#define UNICODE
#endif /* UNICODE */

#ifndef _UNICODE
#define _UNICODE
#endif /* _UNICODE */

#include <windows.h>
#include <windowsx.h>
#include <initguid.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <strsafe.h>
#include <knownfolders.h>

#include <stdint.h>
#include <assert.h>

#endif /* PANITENT_PRECOMP_H_ */
