#include "precomp.h"

#include <windowsx.h>
#include <commctrl.h>
#include <strsafe.h>
#include <assert.h>

#include "dockhost.h"
#include "panitent.h"
#include "resource.h"
#include "docknode.h"
#include "commontypes.h"

static const int CAPTION_HEIGHT = 16;
static const int BORDER_WIDTH = 2;

const WCHAR szDockHostWndClass[] = L"Win32Class_DockHost";

struct _DockBase
{
  DockType type; 
  Rect rect;
};

struct _DockContainer
{
  /* Base struct: */
  DockBase super;

  /* Members: */
  DockBase* child[2];
  DockContainerType containerType;
  DockSplitDirection direction;
  DockSplitAlign align;
  int distance;
};

struct _DockWindow
{
  /* Base struct: */
  DockBase super;

  /* Members: */
  HWND hwnd;
  BOOL bShowCaption;
};

struct _DockHost
{
  DockBase* root;
  HWND hwnd;
  HFONT hCaptionFont;
};

struct _DockDrawCtx
{
  DockHost* host;
  HDC hdc;
};

DockContainer* DockContainer_Create(int distance, DockSplitDirection direction,
    DockSplitAlign align)
{
  DockContainer* split = calloc(1, sizeof(DockContainer));
  ((DockBase*)split)->type = E_DOCK_CONTAINER;
  split->distance = distance;
  split->direction = direction;
  split->align = align;
  return split;
}

DockWindow* DockWindow_Create(HWND hwnd)
{
  DockWindow* d = calloc(1, sizeof(DockWindow));
  ((DockBase*)d)->type = E_DOCK_WINDOW;
  d->hwnd = hwnd;
  d->bShowCaption;

  return d;
}

void DockContainer_Attach(DockContainer* split, DockBase* d)
{
  assert(split);

  if (!split->child[0])
  {
    split->child[0] = d;
  }
  else if (!split->child[1])
  {
    split->child[1] = d;
  } 
  else {
    assert(FALSE);
  }
}

Rect GetContainerRect(DockBase* container)
{
  return container->rect;
}

const DockHost* DockHost_Create(HWND hwndParent, DockBase* root)
{
  DockHost* host = calloc(1, sizeof(DockHost));

  host->root = root;
  host->hwnd = CreateWindowEx(0, szDockHostWndClass, L"", WS_CHILD | WS_VISIBLE,
      0, 0, 0, 0, hwndParent, NULL,
      (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE), (LPVOID)host);
  host->hCaptionFont = GetGuiFont();

  return host;
}

HWND DockHost_GetHWND(const DockHost* host)
{
  return host->hwnd;
}

RECT RectToWin32RECT(Rect rect)
{
  RECT rc = {0};
  rc.left = rect.left;
  rc.top = rect.top;
  rc.right = rect.right;
  rc.bottom = rect.bottom; 
  return rc;
}

void DockWindow_DrawCaption(const DockDrawCtx* ctx, const DockWindow* window)
{
  WCHAR szWindowCaption[128] = {0};
  GetWindowText(window->hwnd, szWindowCaption, 128);

  Rect rect = GetContainerRect((DockBase*)window);
  rect.top += 2;
  rect.left += 2;

  Rect captionRc = GetContainerRect((DockBase*)window);
  captionRc.left += 3;
  captionRc.top += 3;
  captionRc.bottom = captionRc.top + 19;
  captionRc.right -= 3;

  RECT rcCaption = RectToWin32RECT(captionRc);

  FillRect(ctx->hdc, &rcCaption, CreateSolidBrush(RGB(63, 63, 63)));

  SetBkMode(ctx->hdc, TRANSPARENT);
  SetTextColor(ctx->hdc, RGB(255, 255, 255));

  HFONT hCaptionFont = ctx->host->hCaptionFont;
  HGDIOBJ hOldObj = SelectObject(ctx->hdc, hCaptionFont);
  TextOut(ctx->hdc, rect.left + 3, rect.top +3, szWindowCaption,
      wcslen(szWindowCaption));
  SelectObject(ctx->hdc, hOldObj);
}

void DrawContainer(const DockDrawCtx* ctx, DockBase* container)
{
  if (container->type == E_DOCK_CONTAINER)
  {
    const DockContainer* split = (DockContainer*)container;
    if (split->child[0])
    {
      DrawContainer(ctx, split->child[0]);
    }

    if (split->child[1])
    {
      DrawContainer(ctx, split->child[1]);
    }
  }

  if (container->type == E_DOCK_WINDOW)
  {
    Rect rect = GetContainerRect(container);

    RECT rc = RectToWin32RECT(rect);
    FrameRect(ctx->hdc, &rc, GetStockBrush(BLACK_BRUSH));

    const DockWindow* dock = (DockWindow*)container; 
    DockWindow_DrawCaption(ctx, dock);
  }
}

static void OnPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DockHost* host;

  PAINTSTRUCT ps = {0};
  HDC hdc;

  host = (DockHost*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  assert(host);
  if (!host)
    return;

  assert(host->root);
  if (!host->root)
    return;

  DockBase* root = host->root;

  DockDrawCtx* ctx = calloc(1, sizeof(DockDrawCtx));

  hdc = BeginPaint(hwnd, &ps);

  ctx->hdc = hdc; 
  ctx->host = host;

  DrawContainer(ctx, root);

  EndPaint(hwnd, &ps);

  free(ctx);
}

DockContainer* GripHitTest(DockHost* host, DockBase* dock, int x, int y)
{
  if (dock->type == E_DOCK_CONTAINER)
  {
    DockContainer* container = (DockContainer*)dock;

    if (container->direction == E_DIRECTION_VERTICAL)
    {
      int dist = container->distance;

      HDC hdc = GetDC(NULL);

      Rect rect = GetContainerRect(dock);

      RECT appRc;
      GetWindowRect(DockHost_GetHWND(host), &appRc);

      RECT overlayRc;
      overlayRc.left = rect.left + appRc.left;
      overlayRc.top = rect.bottom + appRc.top - dist - 2;
      overlayRc.right = rect.right + appRc.left;
      overlayRc.bottom = rect.bottom + appRc.top - dist + 2;
      FrameRect(hdc, &overlayRc, CreateSolidBrush(RGB(0, 127, 0)));

      if (x > rect.left && y > rect.bottom - dist- 2 && x <= rect.right &&
          y <= rect.bottom - dist + 2)
      {
        MessageBox(NULL, L"Split", NULL, MB_OK);
        return container;
      }
    }
  }
  return NULL;
}

DockBase* DockCaptionHitTest(DockHost* host, DockBase* dock, int x, int y)
{
  if (dock->type == E_DOCK_CONTAINER)
  {
    DockBase* dest1 = NULL;
    DockBase* dest2 = NULL;

    if (!((DockContainer*)dock)->child[0])
    {
      return NULL;
    }
    dest1 = DockCaptionHitTest(host, ((DockContainer*)dock)->child[0], x, y);

    if (!((DockContainer*)dock)->child[1])
    {
      return dest1;
    }

    dest2 = DockCaptionHitTest(host, ((DockContainer*)dock)->child[1], x, y);
    return dest2 ? dest2 : dest1;
  }
  else if (dock->type == E_DOCK_WINDOW)
  {
    Rect rect;
    rect = GetContainerRect(dock); 

    HDC hdc = GetDC(NULL);

    RECT appRc;
    GetWindowRect(DockHost_GetHWND(host), &appRc);

    RECT overlayRc;
    overlayRc.left = rect.left + appRc.left + 3;
    overlayRc.top = rect.top + appRc.top + 3;
    overlayRc.right = rect.right + appRc.left - 3;
    overlayRc.bottom = rect.top + 24 + appRc.top - 2;

    FrameRect(hdc, &overlayRc, CreateSolidBrush(RGB(255, 0, 0)));

    if (x > rect.left + 3 && y > rect.top + 3 && x <= rect.right - 3 && y <= (rect.top + 24 - 3))
    {
      return dock;
    }
  }

  return NULL;
}

void OnLButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  unsigned short x = GET_X_LPARAM(lParam);
  unsigned short y = GET_Y_LPARAM(lParam);

  DockHost* host;
  host = (DockHost*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  assert(host);
  if (!host)
    return;

  assert(host->root);
  if (!host->root)
    return;

  GripHitTest(host, host->root, x, y);
  DockBase* dockHit = DockCaptionHitTest(host, host->root, x, y);
  if (dockHit)
  {
    WCHAR szWindowText[80];

    if (dockHit->type == E_DOCK_WINDOW)
    {
      GetWindowText(((DockWindow*)dockHit)->hwnd, szWindowText, 80);
      MessageBox(NULL, szWindowText, NULL, MB_OK);
    }
    MessageBox(NULL, L"Caption clicked", L"DockCaptionHitTest", MB_OK);
  }
}

typedef struct _DockSizeCtx {
  DockHost* host;
  Rect size;
} DockSizeCtx;

void Rect_Contract(Rect* rc, int d)
{
  rc->left   += d;
  rc->top    += d;
  rc->right  -= d;
  rc->bottom -= d;
}

void UpdateContainerLayout(DockHost* host, Rect rect, DockBase* container)
{
  if (container->type == E_DOCK_CONTAINER)
  {
    DockContainer* split = (DockContainer*)container;
    container->rect = rect;

    /* If first child is NULL, there should not be another childs */
    if (!split->child[0])
      return;

    /* Draw only first child, if second is missing */
    if (!split->child[1])
    {
      UpdateContainerLayout(host, rect, split->child[0]);
      return;
    }

    Rect rectChild1 = rect;
    Rect rectChild2 = rect;

    if (split->direction == E_DIRECTION_HORIZONTAL)
    {
      if (split->align == E_ALIGN_START)
     {
        rectChild1.right = split->distance - 2;
        rectChild2.left = split->distance + 2;
      }
      else {
        rectChild1.right = rect.right - split->distance - 2;
        rectChild2.left = rect.right - split->distance + 2;
      }
    }

    if (split->direction == E_DIRECTION_VERTICAL)
    {
      if (split->align == E_ALIGN_START)
      {
        rectChild1.bottom = split->distance - 2;
        rectChild2.top = split->distance + 2;
      }
      else {
        rectChild1.bottom = rect.bottom - split->distance - 2;
        rectChild2.top = rect.bottom - split->distance + 2;
      }
    }

    UpdateContainerLayout(host, rectChild1, split->child[0]);
    UpdateContainerLayout(host, rectChild2, split->child[1]);
  }

  if (container->type == E_DOCK_WINDOW)
  {
    DockWindow* window = (DockWindow*)container;

    Rect rc = rect;
    container->rect = rc;

    SetWindowPos(window->hwnd, HWND_TOP, rc.left + 4, rc.top + 3 + 16 + 4, (rc.right - rc.left) - 8, (rc.bottom -rc.top) - 8 - 16 - 3, 0);
    UpdateWindow(window->hwnd);
  }
}

static void OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DockHost* host;

  UINT width = LOWORD(lParam);
  UINT height = HIWORD(lParam);

  host = (DockHost*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  assert(host);
  if (!host)
    return;

  assert(host->root);
  if (!host->root)
    return;

  DockBase* root = host->root;

  Rect size = {0};
  size.right = width;
  size.bottom = height;

  UpdateContainerLayout(host, size, root);
}

static void OnCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
  DockHost* host = (DockHost*)lpcs->lpCreateParams;
  assert(host);

  host->hwnd = hwnd;
  SetWindowLong(hwnd, GWLP_USERDATA, (LONG_PTR)host);
}

LRESULT CALLBACK DockHost_WndProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      OnCreate(hwnd, wParam, lParam);
      return 0;
    case WM_SIZE:
      OnSize(hwnd, wParam, lParam);
      return 0;
    case WM_PAINT:
      OnPaint(hwnd, wParam, lParam);
      return 0;
    case WM_LBUTTONDOWN:
      OnLButtonDown(hwnd, wParam, lParam);
      return 0;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL DockHost_Register(HINSTANCE hInstance)
{
  WNDCLASSEX wcex    = {0};
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = (WNDPROC)DockHost_WndProc;
  wcex.hInstance     = hInstance;
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = szDockHostWndClass;

  return RegisterClassEx(&wcex);
}
