#include "precomp.h"
#include <assert.h>
#include <uxtheme.h>

#include "log.h"
#include "log_window.h"
#include "resource.h"

#define IDC_LOGVIEW 1701

typedef struct _tagLOGWINDOWDATA {
  HWND hList;
  int nIDLogObserver;
} LOGWINDOWDATA, *LPLOGWINDOWDATA;

LRESULT CALLBACK LogWindow_WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL LogWindow_OnCreate(HWND, LPCREATESTRUCT);
void LogWindow_OnSize(HWND, UINT, int, int);
LRESULT LogWindow_OnNotify(HWND, int, NMHDR *);
void LogWindow_OnDestroy(HWND);
void PopupError(DWORD);

#define LOGWINDOW_WIDTH 360
#define LOGWINDOW_HEIGHT 480

const WCHAR szLogWindowClassName[] = L"Win32Class_LogWindow";

BOOL LogWindow_Register(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = { 0 };
  wcex.cbSize = sizeof(wcex);
  wcex.style = CS_VREDRAW | CS_HREDRAW;
  wcex.lpfnWndProc = (WNDPROC) LogWindow_WndProc;
  wcex.cbWndExtra = sizeof(LPLOGWINDOWDATA);
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LOG));
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
  wcex.lpszClassName = szLogWindowClassName;

  return RegisterClassEx(&wcex);
}

void LogWindow_LogEventObserverCallback(LPLOGEVENT lEvent)
{
  switch (lEvent->iType)
  {
    case LOGALTEREVENT_ADD:
      {
        LPLOGWINDOWDATA wndData = (LPLOGWINDOWDATA) lEvent->userData;
        assert(wndData);

        ListView_SetItemCount(wndData->hList, lEvent->extra);
      }
      break;
  }
}

HWND LogWindow_Create(HWND hParent)
{
  RECT rc = { 0 };
  rc.right = LOGWINDOW_WIDTH;
  rc.bottom = LOGWINDOW_HEIGHT;
  AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);

  HWND hLogWindow = CreateWindowEx(0, szLogWindowClassName,
      L"Log", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
      rc.right - rc.left,
      rc.bottom - rc.top,
      hParent,
      NULL,
      GetModuleHandle(NULL), NULL);
  assert(hLogWindow);

  ShowWindow(hLogWindow, SW_SHOWNORMAL);
  UpdateWindow(hLogWindow);

  return hLogWindow;
}

LRESULT CALLBACK LogWindow_WndProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    HANDLE_MSG(hWnd, WM_CREATE, LogWindow_OnCreate);
    HANDLE_MSG(hWnd, WM_SIZE, LogWindow_OnSize);
    HANDLE_MSG(hWnd, WM_DESTROY, LogWindow_OnDestroy);
    HANDLE_MSG(hWnd, WM_NOTIFY, LogWindow_OnNotify);
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL LogWindow_OnCreate(HWND hWnd, LPCREATESTRUCT lpcs)
{
  LPLOGWINDOWDATA wndData = (LPLOGWINDOWDATA) calloc(1, sizeof(LOGWINDOWDATA));
  assert(wndData);
  SetWindowLongPtr(hWnd, 0, (LONG_PTR) wndData);

  DWORD dwError;

  HWND hList = CreateWindowEx(0, WC_LISTVIEW, L"",
      WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA,
      0, 0, 0, 0,
      hWnd, (HMENU) IDC_LOGVIEW, GetModuleHandle(NULL), NULL);
  dwError = GetLastError();

  if (!hList) {
    PopupError(dwError);
    assert(hList);
  }

  ListView_SetExtendedListViewStyle(hList,
      LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | 
      LVS_EX_HEADERDRAGDROP | LVS_EX_JUSTIFYCOLUMNS);
  SetWindowTheme(hList, L"Explorer", NULL);

  WCHAR szText[256];
  LVCOLUMN lvc;
  int iCol = 0;

  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

  lvc.pszText = szText;
  lvc.fmt = LVCFMT_LEFT;

  lvc.cx = 100;
  lvc.iSubItem = iCol;
  StringCchCopy(szText, 256, L"Time");
  ListView_InsertColumn(hList, iCol++, &lvc);

  lvc.cx = 70;
  lvc.iSubItem = iCol;
  StringCchCopy(szText, 256, L"Type");
  ListView_InsertColumn(hList, iCol++, &lvc);

  lvc.iSubItem = iCol;
  StringCchCopy(szText, 256, L"Module");
  ListView_InsertColumn(hList, iCol++, &lvc);

  lvc.cx = 200;
  lvc.iSubItem = iCol;
  StringCchCopy(szText, 256, L"Message");
  ListView_InsertColumn(hList, iCol++, &lvc);

  wndData->hList = hList;

  wndData->nIDLogObserver =
      LogRegisterObserver(LogWindow_LogEventObserverCallback, (LPVOID)wndData);

  int nEntries = LogGetSize();
  ListView_SetItemCount(hList, nEntries);

  return TRUE;
}

void LogWindow_OnSize(HWND hWnd, UINT state, int cx, int cy)
{
  LPLOGWINDOWDATA wndData = (LPLOGWINDOWDATA) GetWindowLongPtr(hWnd, 0);
  assert(wndData);

  RECT rcClient = { 0 };
  GetClientRect(hWnd, &rcClient);

  SetWindowPos(wndData->hList, NULL, 0, 0,
      rcClient.right - rcClient.left,
      rcClient.bottom - rcClient.top,
      SWP_NOMOVE);
}

void PopupError(DWORD dwError)
{
  DWORD dwFormatError;
  WCHAR errMsg[256] = { 0 };
  const DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  dwFormatError = FormatMessage(dwFlags, 0, dwError, 0, errMsg, 256, NULL); 
  if (!dwFormatError) {
    /* Ooops, we got a serious problem */
    StringCchPrintf(errMsg, 256, L"Unbelievable. Can't format "
        L"error message in human-readable format. "
        L"FormatMessage error code: %d\r\n\r\n"
        L"Originally reported error code: %d\r\n");
  }

  MessageBox(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
}

LRESULT LogWindow_OnNotify(HWND hwnd, int idCtrl, NMHDR *pnm)
{
  if (idCtrl == IDC_LOGVIEW) {
    switch (pnm->code)
    {
      case LVN_GETDISPINFO:
        {
          LOGENTRY logEntry;
          NMLVDISPINFO *plvdi = (NMLVDISPINFO *) pnm; /* ??? */

          if (plvdi->item.iItem == -1) {
            OutputDebugString(L"LVOWNER: Request for -1 item?\n");
            DebugBreak();
          }

          LPLOGENTRY pEntry;
          pEntry = RetrieveLogEntry(plvdi->item.iItem);
          if (!pEntry) {
            assert(FALSE);
          }

          memcpy(&logEntry, pEntry, sizeof(LOGENTRY));

          if (plvdi->item.mask & LVIF_TEXT) {
            switch (plvdi->item.iSubItem) {
              case 0:
                {
                  DWORD dwError;
                  int cchCount;
                  WCHAR szTimeString[256] = { 0 };

                  cchCount = GetTimeFormatEx(LOCALE_NAME_INVARIANT,
                      TIME_FORCE24HOURFORMAT,
                      &logEntry.timestamp,
                      NULL,
                      szTimeString,
                      256);

                  if (!cchCount) {
                    dwError = GetLastError();
                    assert(FALSE);
                    PopupError(dwError);
                  }

                  StringCchCopy(plvdi->item.pszText, 80, szTimeString);
                }
                break;

              case 1:
                {
                  switch (logEntry.iType) {
                    case LOGENTRY_TYPE_DEBUG: 
                      StringCchCopy(plvdi->item.pszText, 80, L"Debug");
                      break;
                    case LOGENTRY_TYPE_INFO: 
                      StringCchCopy(plvdi->item.pszText, 80, L"Info");
                      break;
                    case LOGENTRY_TYPE_WARNING:
                      StringCchCopy(plvdi->item.pszText, 80, L"Warning");
                      break;
                    case LOGENTRY_TYPE_ERROR:
                      StringCchCopy(plvdi->item.pszText, 80, L"Error");
                      break;
                  }
                }
                break;

              case 2:
                StringCchCopy(plvdi->item.pszText, 80, logEntry.szModule);
                break;

              case 3:
                StringCchCopy(plvdi->item.pszText, 80, logEntry.szMessage);
                break;

              default:
                break;
            }
          }
          return FALSE;
        }
        break;

      case LVN_ODCACHEHINT:
        // NMLVCACHEHINT *pCacheHint = (NMLVCACHEHINT *) pnm;
        break;
    }
  }

  return FALSE;
}

void LogWindow_OnDestroy(HWND hWnd)
{
  LPLOGWINDOWDATA wndData = (LPLOGWINDOWDATA) GetWindowLongPtr(hWnd, 0);
  assert(wndData);

  LogUnregisterObserver(wndData->nIDLogObserver);

  free(wndData);
}
