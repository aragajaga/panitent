#include "precomp.h"

#include <windowsx.h>
#include <commctrl.h>
#include <strsafe.h>
#include <assert.h>

#include "dockhost.h"
#include "panitent.h"
#include "resource.h"

dockhost_t g_dockhost;

POINT captionPos;
HBRUSH hCaptionBrush;

BOOL fSuggestTop;

int iCaptionHeight = 16;
int iBorderWidth   = 2;

dock_window_t g_dock_window;

dock_side_e g_dock_side;
dock_side_e eSuggest;

binary_tree_t* Dock_CaptionHitTest(binary_tree_t*, int, int);
binary_tree_t* Dock_CloseButtonHitTest(binary_tree_t*, int, int);
void Dock_DestroyInclusive(binary_tree_t*, binary_tree_t*);
void DockNode_paint(binary_tree_t*, HDC, HBRUSH);

/* Window message handlers */
BOOL DockHost_OnCreate(PDOCKHOSTDATA, LPCREATESTRUCT);
void DockHost_OnSize(PDOCKHOSTDATA, UINT, int, int);
void DockHost_OnDestroy(PDOCKHOSTDATA);
void DockHost_OnPaint(PDOCKHOSTDATA);
void DockHost_OnMouseMove(PDOCKHOSTDATA, int, int, UINT);
void DockHost_OnLButtonDown(PDOCKHOSTDATA, BOOL, int, int, UINT);
void DockHost_OnLButtonUp(PDOCKHOSTDATA, int, int, UINT);

BOOL DockHost_OnCreate(PDOCKHOSTDATA pDat, LPCREATESTRUCT lpcs) {

  pDat->hCaptionBrush_ = CreateSolidBrush(RGB(0x22, 0x22, 0x22));

  return TRUE;
}

void DockHost_OnPaint(PDOCKHOSTDATA pDat) {

  PAINTSTRUCT ps = {0};
  HDC hDC = BeginPaint(pDat->hWnd_, &ps);

  /* RECT rc = g_dock_window.rc; */

  if (pDat->pRoot_) {
    DockNode_paint(pDat->pRoot_, hDC, pDat->hCaptionBrush_);
  }

  EndPaint(pDat->hWnd_, &ps);
}

void DockHost_OnSize(PDOCKHOSTDATA pDat, UINT state, int cx, int cy) {

  if (pDat->pRoot_) {
    RECT rcRoot = { 0, 0, cx, cy };
    pDat->pRoot_->rc    = rcRoot;

    DockNode_arrange(pDat->pRoot_);
  }

  if (g_dock_window.fDock) {
    g_dock_window.rc.right = cx;
  }
}

void DockHost_OnDestroy(PDOCKHOSTDATA pDat) {
  DeleteObject(pDat->hCaptionBrush_);
}

void DockHost_OnMouseMove(PDOCKHOSTDATA pDat, int x, int y, UINT keyFlags) {

}

void DockHost_OnLButtonDown(PDOCKHOSTDATA pDat, BOOL fDoubleClick, int x, int y,
    UINT keyFlags) {

  RECT rc = g_dock_window.rc;

  /* If cursor in caption */
  if (x >= rc.left + 2 && y >= rc.top + 2 && x < rc.left + rc.right - 2 &&
      y < rc.top + 2 + 24) {

    SetCapture(pDat->hWnd_);
    /* Save click position */
    pDat->ptDragPos_.x = x - (rc.left + 2);
    pDat->ptDragPos_.y = y - (rc.top + 2);
    pDat->fDrag_ = TRUE;
  }
}

void DockHost_OnLButtonUp(PDOCKHOSTDATA pDat, int x, int y, UINT keyFlags) {

  if (pDat->pRoot_) {
    binary_tree_t* t = Dock_CloseButtonHitTest(pDat->pRoot_, x, y);

    if (t) {
      Dock_DestroyInclusive(pDat->pRoot_, t);
      InvalidateRect(pDat->hWnd_, NULL, TRUE);
    }
  }

  /*
  if (pDat->fDrag_ && eSuggest) {
    g_dock_window.undockedRc = g_dock_window.rc;

    RECT rc = {0};
    GetClientRect(hWnd, &rc);

    switch (eSuggest) {
    case DOCK_RIGHT:
      rc.left     = rc.right - g_dock_window.rc.right;
      g_dock_side = DOCK_RIGHT;
      break;
    case DOCK_TOP:
      rc.bottom   = g_dock_window.rc.bottom;
      g_dock_side = DOCK_TOP;
      break;
    case DOCK_LEFT:
      rc.right    = g_dock_window.rc.right;
      g_dock_side = DOCK_LEFT;
      break;
    case DOCK_BOTTOM:
      rc.top      = rc.bottom - g_dock_window.rc.bottom;
      g_dock_side = DOCK_BOTTOM;
      break;
    }
    g_dock_window.rc    = rc;
    g_dock_window.fDock = TRUE;

    unsuggest();
    InvalidateRect(hWnd, NULL, TRUE);
  }
  */

  pDat->fDrag_ = FALSE;
  ReleaseCapture();
}

void Dock_DestroyInner(binary_tree_t* node)
{
  if (!node)
    return;

  if (node->node1) {
    DestroyWindow(node->node1->hwnd);
    Dock_DestroyInner(node->node1);
    free(node->node1);
    node->node1 = NULL;
  }

  if (node->node2) {
    DestroyWindow(node->node2->hwnd);
    Dock_DestroyInner(node->node2);
    free(node->node2);
    node->node2 = NULL;
  }
}

BOOL Dock_GetClientRect(binary_tree_t* root, RECT* rc)
{
  if (!root)
    return FALSE;

  RECT rcClient = root->rc;

  /* clang-format off */
  rcClient.left   += iBorderWidth;
  rcClient.top    += iBorderWidth;
  rcClient.right  -= iBorderWidth;
  rcClient.bottom -= iBorderWidth;
  /* clang-format on */

  if (root->bShowCaption)
    rcClient.top += iCaptionHeight + 1;

  /* TODO: memcpy? */
  *rc = rcClient;

  return TRUE;
}

binary_tree_t* DockNode_Create(HWND hWnd, binary_tree_t* parent)
{
  HWND hButton = CreateWindowEx(0, WC_BUTTON, L"Button", WS_CHILD | WS_VISIBLE,
      0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL);

  parent->posFixedGrip = 128;
  parent->gripAlign    = GRIP_ALIGN_END;
  /* TODO Use significant bit */
  parent->gripPosType  = GRIP_POS_ABSOLUTE;
  parent->bShowCaption = FALSE;

  binary_tree_t* node1 = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* node2 = calloc(1, sizeof(binary_tree_t));
  if (node1 && node2)
  {
    node2->bShowCaption = TRUE;
    node2->lpszCaption = L"Palette";

    node1->posFixedGrip = 64;
    node1->gripAlign = GRIP_ALIGN_START;
    /* TODO Use significant bit */
    node1->gripPosType = GRIP_POS_ABSOLUTE;
    node1->lpszCaption = L"Working Area";

    binary_tree_t* subnode1 = calloc(1, sizeof(binary_tree_t));
    binary_tree_t* subnode2 = calloc(1, sizeof(binary_tree_t));
    if (subnode1 && subnode2)
    {
      subnode1->bShowCaption = TRUE;
      subnode1->lpszCaption = L"Tools";
      subnode1->hwnd = hButton;
      subnode2->bShowCaption = TRUE;
      subnode2->lpszCaption = L"Untitled-1";
      node1->node1 = subnode1;
      node1->node2 = subnode2;

      parent->node1 = node1;
      parent->node2 = node2;
    }
  }

  return NULL;
}

BOOL Dock_GetCaptionRect(binary_tree_t* node, RECT* rc)
{
  if (!node->bShowCaption)
    return FALSE;

  RECT rcCaption = node->rc;
  rcCaption.top += iBorderWidth;
  rcCaption.left += iBorderWidth;
  rcCaption.right -= iBorderWidth;
  rcCaption.bottom = rcCaption.top + iBorderWidth + iCaptionHeight;

  *rc = rcCaption;

  return TRUE;
}

binary_tree_t* Dock_CloseButtonHitTest(binary_tree_t* root, int x, int y)
{
  if (!root)
    return NULL;

  RECT rcCaption = {0};
  Dock_GetCaptionRect(root, &rcCaption);

  if (x >= (rcCaption.right - 16)
      && y >= rcCaption.top && x < rcCaption.right &&
      y < rcCaption.bottom) {
    return root;
  }

  binary_tree_t* found = NULL;
  if (root->node1) {
    found = Dock_CloseButtonHitTest(root->node1, x, y);
  }

  if (!found && root->node2) {
    found = Dock_CloseButtonHitTest(root->node2, x, y);
  }

  return found;
}

binary_tree_t* Dock_CaptionHitTest(binary_tree_t* root, int x, int y)
{
  if (!root)
    return NULL;

  RECT rcCaption = {0};
  Dock_GetCaptionRect(root, &rcCaption);

  if (x >= rcCaption.left && y >= rcCaption.top && x < rcCaption.right &&
      y < rcCaption.bottom) {
    return root;
  }

  binary_tree_t* found = NULL;
  if (root->node1) {
    found = Dock_CaptionHitTest(root->node1, x, y);
  }

  if (!found && root->node2) {
    found = Dock_CaptionHitTest(root->node2, x, y);
  }

  return found;
}

binary_tree_t* BT_HitTest(binary_tree_t* root, int x, int y)
{
  if (root->node1) {
    return BT_HitTest(root->node1, x, y);
  }

  if (root->node2) {
    return BT_HitTest(root->node2, x, y);
  }

  return NULL;
}

void DockNode_paint(binary_tree_t* parent, HDC hDC, HBRUSH hCaptionBrush)
{
  if (!parent)
    return;

  RECT* rc = &parent->rc;
  Rectangle(hDC, rc->left, rc->top, rc->right, rc->bottom);

  if (parent->bShowCaption) {
    /* Fill caption */
    RECT rcCapt = {
        rc->left + iBorderWidth,
        rc->top + iBorderWidth,
        rc->right - iBorderWidth,
        rc->top + iBorderWidth + iCaptionHeight,
    };
    FillRect(hDC, &rcCapt, hCaptionBrush);

    /* Print caption text */

    SetBkColor(hDC, RGB(0x22, 0x22, 0x22));
    SetTextColor(hDC, RGB(0xFF, 0xFF, 0xFF));

    size_t chCount = 0;
    HRESULT hr = StringCchLength(parent->lpszCaption, STRSAFE_MAX_CCH, &chCount);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
      return;
    }

    HBITMAP hbmCloseBtn = LoadBitmap(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_CLOSEBTN));
    HDC hBmDC = CreateCompatibleDC(hDC);
    HGDIOBJ hGdiPrevObj = SelectObject(hBmDC, hbmCloseBtn);
    BitBlt(hDC, rc->left + (rc->right - rc->left) - 14 - 3, rc->top + 3, 14, 14, hBmDC, 0, 0, SRCCOPY);
    SelectObject(hBmDC, hGdiPrevObj);

    HFONT guiFont = GetGuiFont();
    hGdiPrevObj = SelectObject(hDC, guiFont);

    TextOut(hDC,
            rc->left + iBorderWidth + 4,
            rc->top + iBorderWidth,
            parent->lpszCaption,
            (int)chCount);

    SelectObject(hDC, hGdiPrevObj);

    if (parent->hwnd)
      InvalidateRect(parent->hwnd, NULL, FALSE);
  }

  if (parent->node1) {
    DockNode_paint(parent->node1, hDC, hCaptionBrush);
  }

  if (parent->node2) {
    DockNode_paint(parent->node2, hDC, hCaptionBrush);
  }
}

void DockNode_arrange(binary_tree_t* parent)
{
  if (!parent)
    return;

  binary_tree_t* node1 = parent->node1;
  binary_tree_t* node2 = parent->node2;

  if (node1 && node2) {
    RECT rcClient = {0};
    Dock_GetClientRect(parent, &rcClient);

    RECT rcNode1 = rcClient;
    if (parent->gripPosType == GRIP_POS_ABSOLUTE) {
      if (parent->gripAlign == GRIP_ALIGN_START) {
        if (parent->splitDirection == SPLIT_DIRECTION_VERTICAL)
        {
          rcNode1.bottom = rcNode1.top + parent->posFixedGrip - iBorderWidth / 2;
        }
        else {
          rcNode1.right = rcNode1.left + parent->posFixedGrip - iBorderWidth / 2;
        }
      }
      else {
        if (parent->splitDirection == SPLIT_DIRECTION_VERTICAL)
        {
          rcNode1.bottom = rcNode1.bottom - parent->posFixedGrip - iBorderWidth / 2;
        }
        else {
          rcNode1.right = rcNode1.right - parent->posFixedGrip - iBorderWidth / 2;
        }
      }

    } else {
      rcNode1.right = (rcNode1.right - rcNode1.left) / 2 - iBorderWidth / 2;
    }
    node1->rc = rcNode1;

    /* Second part */
    RECT rcNode2 = rcClient;
    if (parent->gripPosType == GRIP_POS_ABSOLUTE) {
      if (parent->gripAlign == GRIP_ALIGN_START) {
        if (parent->splitDirection == SPLIT_DIRECTION_VERTICAL)
        {
          rcNode2.top = rcNode2.top + parent->posFixedGrip + iBorderWidth / 2;
        }
        else {
          rcNode2.left = rcNode2.left + parent->posFixedGrip + iBorderWidth / 2;
        }
      } else {
        if (parent->splitDirection == SPLIT_DIRECTION_VERTICAL)
        {
          rcNode2.top = rcNode2.bottom - parent->posFixedGrip + iBorderWidth / 2;
        }
        else {
          rcNode2.left = rcNode2.right - parent->posFixedGrip + iBorderWidth / 2;
        }
      }
    } else {
      rcNode2.left += (rcNode2.right - rcNode2.left) / 2 + iBorderWidth / 2;
    }
    node2->rc = rcNode2;
  } else if (node1 || node2) {
    binary_tree_t* child = node1 ? node1 : node2;

    RECT rcClient = {0};
    Dock_GetClientRect(parent, &rcClient);

    child->rc = rcClient;
  }

  if (parent->hwnd) {
    RECT rc = {0};
    Dock_GetClientRect(parent, &rc);

    SetWindowPos(parent->hwnd,
                 NULL,
                 rc.left,
                 rc.top,
                 rc.right - rc.left,
                 rc.bottom - rc.top,
                 0);
    UpdateWindow(parent->hwnd);
  }

  if (parent->node1) {
    DockNode_arrange(parent->node1);
  }

  if (parent->node2) {
    DockNode_arrange(parent->node2);
  }
}

WCHAR szSampleText[] = L"SampleText";

binary_tree_t* Dock_FindParent(binary_tree_t* root, binary_tree_t* node)
{
  if (!(root && node))
    return NULL;

  binary_tree_t* found = NULL;

  if (root->node1) {
    if (root->node1 == node) {
      return root;
    } else {
      found = Dock_FindParent(root->node1, node);
    }
  }

  if (!found && root->node2) {
    if (root->node2 == node) {
      return root;
    } else {
      found = Dock_FindParent(root->node2, node);
    }
  }

  return found;
}

void Dock_DestroyInclusive(binary_tree_t* root, binary_tree_t* node)
{
  Dock_DestroyInner(node);

  DestroyWindow(node->hwnd);
  binary_tree_t* parent = Dock_FindParent(root, node);

  binary_tree_t* detached = NULL;
  if (parent) {
    if (node == parent->node1) {
      free(parent->node1);
      parent->node1 = NULL;

      detached = parent->node2;
    } else if (node == parent->node2) {
      free(parent->node2);
      parent->node2 = NULL;

      detached = parent->node1;
    }

    binary_tree_t* grandparent = Dock_FindParent(root, parent);
    if (grandparent) {
      if (parent == grandparent->node1) {
        grandparent->node1 = detached;
        free(parent);
        parent = NULL;
      } else if (parent == grandparent->node2) {
        grandparent->node2 = detached;
        free(parent);
        parent = NULL;
      }
    }

  } else {
    free(node);
  }

  DockNode_arrange(root);
}

void Dock_Undock(binary_tree_t* root, binary_tree_t* node)
{
  binary_tree_t* parent = Dock_FindParent(root, node);
  DestroyWindow(node->hwnd);

  if (!parent)
    return;

  binary_tree_t* detached = NULL;
  if (node == parent->node1) {
    detached = parent->node1;

    parent->node1 = NULL;
  } else if (node == parent->node2) {
    detached = parent->node2;

    parent->node2 = NULL;
  }

  binary_tree_t* grandparent = Dock_FindParent(root, parent);
  if (grandparent) {
    if (grandparent->node1 == parent) {
      grandparent->node1 = detached;
      free(parent);
    } else if (grandparent->node2 == parent) {
      grandparent->node2 = detached;
      free(parent);
    }
  }

  DockNode_arrange(root);
}

LRESULT CALLBACK DockHost_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

  PDOCKHOSTDATA pDat = NULL;

  if (message == WM_CREATE || message == WM_NCCREATE) {
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
    pDat = (PDOCKHOSTDATA) lpcs->lpCreateParams;

    pDat->hWnd_ = hWnd;
    SetWindowLongPtr(hWnd, 0, (LONG_PTR) pDat);
  }
  else {
    pDat = (PDOCKHOSTDATA) GetWindowLongPtr(hWnd, 0);
  }

  switch (message) {

    case WM_CREATE:
      return DockHost_OnCreate(pDat, (LPCREATESTRUCT)lParam);
      break;

    case WM_SIZE:
      DockHost_OnSize(pDat, (UINT)wParam, (int)(short)LOWORD(lParam),
          (int)(short)HIWORD(lParam));
      return 0;
      break;

    case WM_PAINT:
      DockHost_OnPaint(pDat);
      return 0;
      break;

    case WM_DESTROY:
      DockHost_OnDestroy(pDat);
      return 0;
      break;

    case WM_LBUTTONDOWN:
      DockHost_OnLButtonDown(pDat, FALSE, (int)(short)LOWORD(lParam),
          (int)(short)HIWORD(lParam), (UINT)wParam);
      return 0;
      break;

    case WM_MOUSEMOVE:
      DockHost_OnMouseMove(pDat, (int)(short)LOWORD(lParam),
          (int)(short)HIWORD(lParam), (UINT)wParam);
      return 0;
      break;

    case WM_LBUTTONUP:
      DockHost_OnLButtonUp(pDat, (int)(short)LOWORD(lParam),
          (int)(short)HIWORD(lParam), (UINT)wParam);
      return 0;
      break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void DockHost_Init(PDOCKHOSTDATA pDat) {
  HINSTANCE hInstance = NULL;
  ZeroMemory(pDat, sizeof(DOCKHOSTDATA));

  hInstance = GetModuleHandle(NULL);

  pDat->hInstance_ = hInstance;

  pDat->cs_.style = WS_BORDER | WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN;
  pDat->cs_.x = 0;
  pDat->cs_.y = 0;
  pDat->cs_.cx = 640;
  pDat->cs_.cy = 480;
  pDat->cs_.hInstance = hInstance;
}

BOOL DockHost_RegisterClass(PDOCKHOSTDATA pDat) {

  HINSTANCE hInstance;
  WNDCLASSEX wcex = { 0 };

  if (GetClassInfoEx(pDat->hInstance_, pDat->wcex_.lpszClassName, &wcex)) {
    memcpy(&pDat->wcex_, &wcex, sizeof(WNDCLASSEX));
    return TRUE;
  }

  pDat->wcex_.cbSize = sizeof(wcex);
  pDat->wcex_.style = CS_HREDRAW | CS_VREDRAW;
  pDat->wcex_.lpfnWndProc = (WNDPROC) DockHost_WndProc;
  pDat->wcex_.cbWndExtra = sizeof(PDOCKHOSTDATA);
  pDat->wcex_.hInstance = pDat->hInstance_;
  pDat->wcex_.hCursor = LoadCursor(NULL, IDC_ARROW);
  pDat->wcex_.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
  pDat->wcex_.lpszClassName = L"Panitent_DockHost3_";

  return (BOOL) RegisterClassEx(&pDat->wcex_);
}

void DockHost_Create(PDOCKHOSTDATA pDat, HWND hParent) {
  DockHost_RegisterClass(pDat);

  pDat->cs_.lpszClass = pDat->wcex_.lpszClassName;

  if (IsWindow(hParent)) {
    RECT rcClient = { 0 };
    GetClientRect(hParent, &rcClient);

    pDat->cs_.cx = rcClient.right;
    pDat->cs_.cy = rcClient.bottom;
  }

  pDat->hWnd_ = CreateWindowEx(pDat->cs_.dwExStyle, pDat->cs_.lpszClass,
      L"DockHost",
      pDat->cs_.style,
      pDat->cs_.x, pDat->cs_.y, pDat->cs_.cx, pDat->cs_.cy,
      hParent,
      (HMENU) NULL,
      pDat->hInstance_,
      (LPVOID) pDat);
}
