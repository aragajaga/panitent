#include "precomp.h"

#include <windowsx.h>
#include <commctrl.h>
#include <strsafe.h>
#include <assert.h>

#include "dockhost.h"

dockhost_t g_dockhost;

POINT captionPos;
POINT dragPos;
BOOL fDrag;
HBRUSH hCaptionBrush;

BOOL fSuggestTop;

int iCaptionHeight = 16;
int iBorderWidth = 2;

dock_window_t g_dock_window;

/*
void suggest(dock_side_e side)
{
  eSuggest = side; 
}

void unsuggest()
{
  eSuggest = 0; 
}
*/

dock_side_e g_dock_side;
dock_side_e eSuggest;
binary_tree_t* root;

void Dock_DestroyInner(binary_tree_t* node)
{
  if (!node)
    return;

  if (node->node1)
  {
    DestroyWindow(node->node1->hwnd);
    Dock_DestroyInner(node->node1);
    free(node->node1);
    node->node1 = NULL;
  }

  if (node->node2)
  {
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
  rcClient.left   += iBorderWidth;
  rcClient.top    += iBorderWidth;
  rcClient.right  -= iBorderWidth;
  rcClient.bottom -= iBorderWidth;

  if (root->bShowCaption)
    rcClient.top  += iCaptionHeight + 1;

  /* TODO: memcpy? */
  *rc = rcClient;

  return TRUE;
}

binary_tree_t* DockNode_Create(HWND hWnd, binary_tree_t* parent)
{
  HWND hButton = CreateWindowEx(0, WC_BUTTON, L"Button",
      WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL,
      GetModuleHandle(NULL), NULL);

  parent->posFixedGrip = 128;
  parent->gripAlign = GRIP_ALIGN_END;
  // TODO Use significant bit
  parent->gripPosType = GRIP_POS_ABSOLUTE;
  parent->bShowCaption = FALSE;

  binary_tree_t* node1 = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* node2 = calloc(1, sizeof(binary_tree_t));
  node2->bShowCaption = TRUE;
  node2->lpszCaption = L"Palette";

  node1->posFixedGrip = 64;
  node1->gripAlign = GRIP_ALIGN_START;
  // TODO Use significant bit
  node1->gripPosType = GRIP_POS_ABSOLUTE;
  node1->lpszCaption = L"Working Area";

  binary_tree_t* subnode1 = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* subnode2 = calloc(1, sizeof(binary_tree_t));
  subnode1->bShowCaption = TRUE;
  subnode1->lpszCaption = L"Tools";
  subnode1->hwnd = hButton;
  subnode2->bShowCaption = TRUE;
  subnode2->lpszCaption = L"Untitled-1";
  node1->node1 = subnode1;
  node1->node2 = subnode2;

  parent->node1 = node1;
  parent->node2 = node2;

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

binary_tree_t* Dock_CaptionHitTest(binary_tree_t* root, int x, int y)
{
  if (!root)
    return NULL;

  RECT rcCaption = {0};
  Dock_GetCaptionRect(root, &rcCaption);

  if (x >= rcCaption.left  && y >= rcCaption.top &&
      x <  rcCaption.right && y <  rcCaption.bottom)
  {
    return root;  
  }

  binary_tree_t* found = NULL; 
  if (root->node1)
  {
    found = Dock_CaptionHitTest(root->node1, x, y);
  }

  if (!found && root->node2)
  {
    found = Dock_CaptionHitTest(root->node2, x, y);
  }

  return found;
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

  if (parent->bShowCaption)
  {
    // Fill caption
    RECT rcCapt = {
        rc->left  + iBorderWidth,
        rc->top   + iBorderWidth,
        rc->right - iBorderWidth,
        rc->top   + iBorderWidth + iCaptionHeight};
    FillRect(hDC, &rcCapt, hCaptionBrush); 

    // Print caption text
    SetBkColor(hDC, RGB(0xFF, 0xCC, 0x00));
    SetTextColor(hDC, RGB(0xFF, 0xFF, 0xFF));

    size_t chCount = 0;
    StringCchLength(parent->lpszCaption, STRSAFE_MAX_CCH, &chCount);
    TextOut(hDC, rc->left + iBorderWidth + 4, rc->top + iBorderWidth,
        parent->lpszCaption, chCount);
  }

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
    RECT rcClient = {0};
    Dock_GetClientRect(parent, &rcClient);

    RECT rcNode1 = rcClient;
    if (parent->gripPosType == GRIP_POS_ABSOLUTE) {
      if (parent->gripAlign == GRIP_ALIGN_START)
      {
        rcNode1.right = rcNode1.left + parent->posFixedGrip - iBorderWidth/2;
      }
      else {
        rcNode1.right = rcNode1.right - parent->posFixedGrip - iBorderWidth/2;
      }

    } else {
      rcNode1.right = (rcNode1.right - rcNode1.left) / 2 - iBorderWidth/2;
    }
    node1->rc = rcNode1; 

    // Second part
    RECT rcNode2 = rcClient;
    if (parent->gripPosType == GRIP_POS_ABSOLUTE) {
      if (parent->gripAlign == GRIP_ALIGN_START)
      {
        rcNode2.left = rcNode2.left + parent->posFixedGrip + iBorderWidth/2;
      }
      else {
        rcNode2.left = rcNode2.right - parent->posFixedGrip + iBorderWidth/2;
      }
    } else {
      rcNode2.left += (rcNode2.right - rcNode2.left) / 2 + iBorderWidth/2;
    }
    node2->rc = rcNode2; 
  }
  else if (node1 || node2)
  {
    binary_tree_t* child = node1 ? node1 : node2; 

    RECT rcClient = {0};
    Dock_GetClientRect(parent, &rcClient);

    child->rc = rcClient; 
  }

  if (parent->hwnd)
  {
    RECT rc = {0};
    Dock_GetClientRect(parent, &rc);

    SetWindowPos(parent->hwnd, NULL, rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top, 0);
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

binary_tree_t* Dock_FindParent(binary_tree_t* root, binary_tree_t* node)
{
  if (!(root && node))
    return NULL;

  binary_tree_t* found = NULL;

  if (root->node1)
  {
    if (root->node1 == node)
    {
      return root;
    }
    else {
      found = Dock_FindParent(root->node1, node);
    }
  }

  if (!found && root->node2)
  {
    if (root->node2 == node)
    {
      return root;
    }
    else {
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
  if (parent)
  {
    if (node == parent->node1)
    {
      free(parent->node1);
      parent->node1 = NULL;
      detached = parent->node2;
    }
    else if (node == parent->node2) {
      free(parent->node2);
      parent->node2 = NULL;
      detached = parent->node1;
    }

    binary_tree_t* grandparent = Dock_FindParent(root, parent);
    if (grandparent)
    {
      if (parent == grandparent->node1)
      {
        grandparent->node1 = detached;
        free(parent);
        parent = NULL;
      }
      else if (parent == grandparent->node2)
      {
        grandparent->node2 = detached;
        free(parent);
        parent = NULL;
      }
    }

  }
  else {
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
  if (node == parent->node1)
  {
    detached = parent->node1;
    parent->node1 = NULL;
  }
  else if (node == parent->node2)
  {
    detached = parent->node2;
    parent->node2 = NULL;
  }

  binary_tree_t* grandparent = Dock_FindParent(root, parent);
  if (grandparent)
  {
    if (grandparent->node1 == parent) {
      grandparent->node1 = detached;
      free(parent);
    }
    else if (grandparent->node2 == parent)
    {
      grandparent->node2 = detached;
      free(parent);
    } 
  }

  DockNode_arrange(root);
}

LRESULT CALLBACK DockHost_WndProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      {
        hCaptionBrush = CreateSolidBrush(RGB(0xFF, 0xCC, 0x00));

/*
        RECT rc = {10, 10, 200, 200};
        g_dock_window.rc = rc;
        */
      }
      break;
    case WM_SIZE:
      {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        if (root)
        {
          RECT rcRoot = {0, 0, width, height};
          root->rc = rcRoot;
          
          DockNode_arrange(root);
        }

        if (g_dock_window.fDock)
        {
          g_dock_window.rc.right = width;
        }
      }
      break;
    case WM_PAINT:
      {
        PAINTSTRUCT ps = {0};
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
            */

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
       /*
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
          
          RECT hostRc = {0};
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
        */
      }
      break;
    case WM_LBUTTONUP:
      {
        signed short x = GET_X_LPARAM(lParam);
        signed short y = GET_Y_LPARAM(lParam);


        if (root)
        {
          binary_tree_t* t = Dock_CaptionHitTest(root, x, y);
          if (t)
          {
            /*wchar_t message[64];
            binary_tree_t* parent = Dock_FindParent(root, t);

            StringCchPrintfW(message, 64, L"Target: %s\nParent: %s",
                t->lpszCaption ? t->lpszCaption : L"< NULL >",
                parent ? (parent->lpszCaption ? parent->lpszCaption : L"< NULL CAPTION >") : L"< NULL PARENT >");
            MessageBox(NULL, message, L"HitTest", MB_OK);
            */
            Dock_DestroyInclusive(root, t);
            //Dock_Undock(root, t);
            InvalidateRect(hWnd, NULL, TRUE);
          }
        }
        /*

        if (fDrag && eSuggest)
        {
          g_dock_window.undockedRc = g_dock_window.rc;

          RECT rc = {0};
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
        */

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
  WNDCLASSEX wcex = {0};
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
