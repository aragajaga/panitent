#include <assert.h>
#include <windowsx.h>
#include "dockhost.h"

dockhost_t g_dockhost;

POINT captionPos;
POINT dragPos;
BOOL fDrag;
HBRUSH hCaptionBrush;

BOOL fSuggestTop;

typedef struct _dock_window {
  RECT rc;
  RECT pins;
  RECT undockedRc;
  HWND hwnd;
  LPWSTR caption;
  BOOL fDock;
} dock_window_t;

dock_window_t g_dock_window;

typedef enum {
  DOCK_RIGHT = 1,
  DOCK_TOP,
  DOCK_LEFT,
  DOCK_BOTTOM
} dock_side_e;

dock_side_e g_dock_side;
dock_side_e eSuggest;

void suggest(dock_side_e side)
{
  eSuggest = side; 
}

void unsuggest()
{
  eSuggest = 0; 
}

LRESULT CALLBACK DockHost_WndProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      {
        RECT rc = {10, 10, 200, 200};
        hCaptionBrush = CreateSolidBrush(RGB(0xFF, 0x00, 0x00));
        g_dock_window.rc = rc;
      }
      break;
    case WM_SIZE:
      {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        if (g_dock_window.fDock)
        {
          g_dock_window.rc.right = width;
        }
      }
      break;
    case WM_PAINT:
      {
        PAINTSTRUCT ps = {};
        HDC hDC = BeginPaint(hWnd, &ps);

        RECT rc = g_dock_window.rc;
        // Fill window border
        Rectangle(hDC, rc.left, rc.top, rc.left + rc.right,
            rc.top + rc.bottom);

        // Fill caption
        RECT rcCapt = {rc.left + 2, rc.top + 2,
            rc.left+rc.right-2, rc.top + 2 + 24};
        FillRect(hDC, &rcCapt, hCaptionBrush); 

        // Print caption text
        SetBkColor(hDC, RGB(0xFF, 0x00, 0x00));
        SetTextColor(hDC, RGB(0xFF, 0xFF, 0xFF));
        TextOut(hDC, rc.left + 6, rc.top + 6,
            L"Main window", 11);

        if (eSuggest)
        {
          RECT rc;
          GetClientRect(hWnd, &rc);

          switch (eSuggest)
          {
            case DOCK_RIGHT:
              rc.left = rc.right - 24;
              FillRect(hDC, &rc, hCaptionBrush); 
              break;
            case DOCK_TOP:
              rc.bottom = 24;
              FillRect(hDC, &rc, hCaptionBrush); 
              break;
            case DOCK_LEFT:
              rc.right = 24;
              FillRect(hDC, &rc, hCaptionBrush); 
              break;
            case DOCK_BOTTOM:
              rc.top = rc.bottom - 24;
              FillRect(hDC, &rc, hCaptionBrush); 
              break;
          }
          // SIZE resSize;
          // GetTextExtentPoint32(hDC, L"Dock top", 8, &resSize);

          // TextOut(hDC, (sugRc.right-resSize.cx)/2, 4, L"Dock top", 8);
        }

        EndPaint(hWnd, &ps);
      }
      break;
    case WM_DESTROY:
      DeleteObject(hCaptionBrush);
      break;
    case WM_LBUTTONDOWN:
      {
        signed short x = GET_X_LPARAM(lParam);
        signed short y = GET_Y_LPARAM(lParam);

        RECT rc = g_dock_window.rc;

        // If cursor in caption
        if (x >= rc.left + 2 && y >= rc.top + 2 &&
            x < rc.left + rc.right - 2 &&
            y < rc.top + 2 + 24)
        {

          SetCapture(hWnd);
          // Save click position
          dragPos.x = x - (rc.left + 2);
          dragPos.y = y - (rc.top + 2);
          fDrag = TRUE;
        }
      }
      break;

    case WM_MOUSEMOVE:
      {
        if (fDrag)
        {
          signed short x = GET_X_LPARAM(lParam);
          signed short y = GET_Y_LPARAM(lParam);

          if (g_dock_window.fDock)
          {
            g_dock_window.rc = g_dock_window.undockedRc;
            g_dock_window.fDock = FALSE;
          }

          g_dock_window.rc.left = x - dragPos.x;
          g_dock_window.rc.top  = y - dragPos.y;
          
          RECT hostRc = {};
          GetClientRect(hWnd, &hostRc);

          if (y < 10) {
            suggest(DOCK_TOP);
          }
          else if (x >= hostRc.right - 10) {
            suggest(DOCK_RIGHT);
          }
          else if (y >= hostRc.bottom - 10) {
            suggest(DOCK_BOTTOM);
          }
          else if (x < 10) {
            suggest(DOCK_LEFT);
          }
          else {
            unsuggest();
          }

          InvalidateRect(hWnd, NULL, TRUE);
        }
      }
      break;
    case WM_LBUTTONUP:
      {
        signed short x = GET_X_LPARAM(lParam);
        signed short y = GET_Y_LPARAM(lParam);

        if (fDrag && eSuggest)
        {
          g_dock_window.undockedRc = g_dock_window.rc;

          RECT rc = {};
          GetClientRect(hWnd, &rc);

          switch (eSuggest)
          {
            case DOCK_RIGHT:
              rc.left = rc.right - g_dock_window.rc.right;
              g_dock_side = DOCK_RIGHT;
              break;
            case DOCK_TOP:
              rc.bottom = g_dock_window.rc.bottom;
              g_dock_side = DOCK_TOP;
              break;
            case DOCK_LEFT:
              rc.right = g_dock_window.rc.right;
              g_dock_side = DOCK_LEFT;
              break;
            case DOCK_BOTTOM:
              rc.top = rc.bottom - g_dock_window.rc.bottom;
              g_dock_side = DOCK_BOTTOM;
              break;
          }
          g_dock_window.rc = rc;
          g_dock_window.fDock = TRUE;

          unsuggest();
          InvalidateRect(hWnd, NULL, TRUE);
        }

        fDrag = FALSE;
        ReleaseCapture();
      }
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
      break;
  }

  return 0;
}

ATOM DockHost_Register(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)DockHost_WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = L"Win32Class_DockHost";
  wcex.lpszMenuName = NULL;
  wcex.hIconSm = NULL;
  ATOM atom = RegisterClassEx(&wcex);
  assert(atom);
  g_dockhost.wndClass = atom;
  return atom;
}

HWND DockHost_Create(HWND hParent)
{
  HWND hWnd = CreateWindowEx(0, MAKEINTATOM(g_dockhost.wndClass),
      L"DockHost", WS_CHILD | WS_VISIBLE, 0, 0, 320, 240, hParent,
      (HMENU)NULL, GetModuleHandle(NULL), NULL);
  assert(hWnd);

  return hWnd;
}
