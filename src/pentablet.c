#include "precomp.h"

#include "pentablet.h"

WTINFOW gpWTInfoW;

/* HCTX (WINAPI *pfnWTOpenW) (HWND, LPLOGCONTEXTW, BOOL); */

BOOL LoadWintab()
{
  HANDLE hWintab = LoadLibrary(L"Wintab32.dll");
  assert(hWintab);

  if (!hWintab)
  {
    MessageBox(NULL, L"Wintab failed to load", NULL, MB_OK | MB_ICONERROR);
    return FALSE;
  }
  else
    MessageBox(NULL, L"Wintab loaded successfully", L"Status", MB_OK | MB_ICONINFORMATION);

  /*
  pfnWTOpenW = GetProcAddress(hWintab, L"WTOpenW");
  assert(pfnWTOpenW);
  */

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  /* ifdef __GNUC__ */

  gpWTInfoW = (WTINFOW)GetProcAddress(hWintab, "WTInfoW");

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  /* ifdef __GNUC__ */

  assert(gpWTInfoW);

  WCHAR szTabletName[50] = { 0 };
  (*gpWTInfoW)(WTI_DEVICES, DVC_NAME, szTabletName);
  MessageBox(NULL, szTabletName, L"Status", MB_OK | MB_ICONINFORMATION);
  (*gpWTInfoW)(WTI_INTERFACE, IFC_WINTABID, szTabletName);
  MessageBox(NULL, szTabletName, L"Status", MB_OK | MB_ICONINFORMATION);

  if (!(*gpWTInfoW)(0, 0, NULL))
  {
    return FALSE;
  }

  /*
  GetProcAddress(hWintab, WTInfoA);
  GetProcAddress(hWintab, WTGetA);
  GetProcAddress(hWintab, WTSetA);
  GetProcAddress(hWintab, WTPacket);
  GetProcAddress(hWintab, WTClose);
  GetProcAddress(hWintab, WTEnable);
  GetProcAddress(hWintab, WTOverlap);
  GetProcAddress(hWintab, WTSave);
  GetProcAddress(hWintab, WTConfig);
  GetProcAddress(hWintab, WTRestore);
  GetProcAddress(hWintab, WTExtSet);
  GetProcAddress(hWintab, WTExtGet);
  GetProcAddress(hWintab, WTQueueSizeSet);
  GetProcAddress(hWintab, WTDataPeek);
  GetProcAddress(hWintab, WTPacketsGet);
  */

  return TRUE;
}
