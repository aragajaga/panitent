#include <assert.h>
#include <windowsx.h>
#include <strsafe.h>
#include "dockhost.h"

dockhost_t g_dockhost;

POINT captionPos;
POINT dragPos;
BOOL fDrag;
HBRUSH hCaptionBrush;

BOOL fSuggestTop;

typedef enum {
  DOCK_RIGHT = 1,
  DOCK_TOP,
  DOCK_LEFT,
  DOCK_BOTTOM
} dock_side_e;

// TODO Use significant bit
typedef enum {
  GRIP_ALIGN_START,
  GRIP_ALIGN_END
} grip_align_e;

typedef enum {
  GRIP_POS_UNIFORM,
  GRIP_POS_ABSOLUTE,
  GRIP_POS_RELATIVE
} grip_pos_type_e;

dock_side_e g_dock_side;
dock_side_e eSuggest;

typedef struct _dock_window {
  RECT rc;
  RECT pins;
  RECT undockedRc;
  HWND hwnd;
  LPWSTR caption;
  BOOL fDock;
} dock_window_t;

dock_window_t g_dock_window;

struct _binary_tree {
  struct _binary_tree* node1;
  struct _binary_tree* node2;
  int delimPos;
  RECT rc;
  LPWSTR lpszCaption;
  grip_pos_type_e gripPosType;
  float fGrip;
  grip_align_e gripAlign;
  int posFixedGrip;
};

typedef struct _binary_tree binary_tree_t;

void suggest(dock_side_e side)
{
  eSuggest = side; 
}

void unsuggest()
{
  eSuggest = 0; 
}

binary_tree_t* root;

binary_tree_t* DockNode_Create(binary_tree_t* parent)
{
  parent->posFixedGrip = 128;
  parent->gripAlign = GRIP_ALIGN_END;
  // TODO Use significant bit
  parent->gripPosType = GRIP_POS_ABSOLUTE;

  binary_tree_t* node1 = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* node2 = calloc(1, sizeof(binary_tree_t));

  node1->posFixedGrip = 64;
  node1->gripAlign = GRIP_ALIGN_START;
  // TODO Use significant bit
  node1->gripPosType = GRIP_POS_ABSOLUTE;

  binary_tree_t* subnode1 = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* subnode2 = calloc(1, sizeof(binary_tree_t));
  node1->node1 = subnode1;
  node1->node2 = subnode2;

  parent->node1 = node1;
  parent->node2 = node2;
}

binary_tree_t* BT_HitTest(binary_tree_t* root, int x, int y)
{
  if (root->node1)
  {
    return BT_HitTest(root->node1, x, y);
  }

  if (root->node2)
  {
    return BT_HitTest(root->node2, x, y);
  }

  return NULL;
}

void DockNode_paint(binary_tree_t* parent, HDC hDC)
{
  if (!parent)
    return;

  RECT* rc = &parent->rc;
  Rectangle(hDC, rc->left, rc->top, rc->right, rc->bottom);

  // Fill caption
  RECT rcCapt = {
      rc->left + 2,
      rc->top  + 2,
      rc->right - 2,
      rc->top  + 2 + 24};
  FillRect(hDC, &rcCapt, hCaptionBrush); 

  // Print caption text
  SetBkColor(hDC, RGB(0xFF, 0x00, 0x00));
  SetTextColor(hDC, RGB(0xFF, 0xFF, 0xFF));

  size_t chCount = 0;
  StringCchLength(parent->lpszCaption, STRSAFE_MAX_CCH, &chCount);
  TextOut(hDC, rc->left + 6, rc->top + 6,
      parent->lpszCaption, chCount);

  if (parent->node1)
  {
    DockNode_paint(parent->node1, hDC);
  }

  if (parent->node2)
  {
    DockNode_paint(parent->node2, hDC);
  }
}

void DockNode_arrange(binary_tree_t* parent)
{
  if (!parent)
    return;

  binary_tree_t* node1 = parent->node1;
  binary_tree_t* node2 = parent->node2;

  if (node1 && node2)
  {
    RECT rcNode1 = parent->rc;
    rcNode1.top    += 2 + 24 + 1;
    rcNode1.left   += 2;
    rcNode1.bottom -= 2;
    if (parent->gripPosType == GRIP_POS_ABSOLUTE) {
      if (parent->gripAlign == GRIP_ALIGN_START)
      {
        rcNode1.right = rcNode1.left + parent->posFixedGrip - 2;
      }
      else {
        rcNode1.right = rcNode1.right - parent->posFixedGrip - 2;
      }

    } else {
      rcNode1.right  -= (rcNode1.right - rcNode1.left) / 2 + 2;
    }
    node1->rc = rcNode1; 

    RECT rcNode2 = parent->rc;
    rcNode2.right  -= 2;
    rcNode2.top    += 2 + 24 + 1;
    rcNode2.bottom -= 2;
    if (parent->gripPosType == GRIP_POS_ABSOLUTE) {
      if (parent->gripAlign == GRIP_ALIGN_START)
      {
        rcNode2.left = parent->posFixedGrip + 2;
      }
      else {
        rcNode2.left = rcNode2.right - parent->posFixedGrip + 2;
      }
    } else {
      rcNode2.left += (rcNode2.right - rcNode2.left) / 2 + 2;
    }
    node2->rc = rcNode2; 
  }
  else if (node1 || node2)
  {
    binary_tree_t* child = node1 ? node1 : node2; 

    RECT rcNode = parent->rc;
    rcNode.right  -= 2;
    rcNode.top    += 2 + 24 + 1;
    rcNode.bottom -= 2;
    rcNode.left   += 2;
    child->rc = rcNode; 
  }

  if (parent->node1)
  {
    DockNode_arrange(parent->node1);
  }

  if (parent->node2)
  {
    DockNode_arrange(parent->node2);
  }
}

WCHAR szSampleText[] = L"SampleText";

LRESULT CALLBACK DockHost_WndProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      {
        RECT rcRoot = {};
        GetClientRect(hWnd, &rcRoot);
        root = calloc(1, sizeof(binary_tree_t));
        root->lpszCaption = szSampleText;
        root->rc = rcRoot;

        binary_tree_t* node;
        node = DockNode_Create(root); 


        RECT rc = {10, 10, 200, 200};
        hCaptionBrush = CreateSolidBrush(RGB(0xFF, 0x00, 0x00));
        g_dock_window.rc = rc;
      }
      break;
    case WM_SIZE:
      {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        RECT rcRoot = {0, 0, width, height};
        root->rc = rcRoot;
        
        DockNode_arrange(root);

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

        if (root)
        {
          DockNode_paint(root, hDC);
        }


        /*
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
            */

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
