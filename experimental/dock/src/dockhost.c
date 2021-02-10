#include <assert.h>
#include <windowsx.h>
#include <strsafe.h>
#include <commctrl.h>
#include "dockhost.h"

HBRUSH hCaptionBrush;

typedef struct _dock_frame {
  BOOL fShow;
  BOOL fCaption;
  HWND hWnd;
  WCHAR* lpszCaption;
} dock_frame_t;

struct _linked_tree_node {
  struct _linked_tree_node* parent;
  struct _linked_tree_node* nodeA;
  struct _linked_tree_node* nodeB;
  unsigned int weight;
  dock_frame_t dockFrame;
};

typedef struct _linked_tree_node linked_tree_node_t;

typedef struct _linked_tree {
  linked_tree_node_t* root;
  void (*node_destroy_callback)(linked_tree_node_t*);
} linked_tree_t;

typedef struct _dock_host {
  unsigned int captionHeight;
  unsigned int borderWidth;
  linked_tree_t* tree;
} dock_host_t;

typedef struct _lifo {
  size_t capacity;
  size_t size;  
  void **data;
} lifo_t;

typedef enum _rbtree_color {
  RBTREE_COLOR_BLACK,
  RBTREE_COLOR_RED
} rbtree_color_t;

typedef struct _rbtree_node {
  struct _rbtree_node *parent;
  struct _rbtree_node *nodeA;
  struct _rbtree_node *nodeB;
  rbtree_color_t color;
  int weight;
} rbtree_node_t;

typedef struct _dictonary {
  size_t size;
  size_t capacity;
} dict_t;

rbtree_node_t* rbtree_get_parent(rbtree_node_t* node)
{
  return !node ? NULL : node->parent;
}

void rbtree_push(rbtree_node_t* root, rbtree_node_t* inserting)
{
  rbtree_node_t* next_node;
  rbtree_node_t* this_node;

  this_node = root;

  do {
    next_node = NULL;

    next_node = (this_node->weight <= inserting->weight)
      ? this_node->nodeA
      : this_node->nodeB;

    if (this_node->weight <= inserting->weight)
    {
      rbtree_node_t* temp;
      
      temp = this_node->nodeA;
      this_node->nodeA = inserting;
    }

  } while (this_node->nodeA);
}

#define CAPACITY_ALIGN 16
#define CHOP(a, b) ((a) / b * b)

void lifo_init(lifo_t* lifo)
{
  assert(!lifo->data);
  lifo->data = calloc(CAPACITY_ALIGN, sizeof(void*));
  lifo->capacity = CAPACITY_ALIGN;
}

void lifo_push(lifo_t* lifo, void *data)
{
  if (lifo->capacity <= lifo->size)
  {
    size_t newcap = CHOP(lifo->capacity + 1, CAPACITY_ALIGN);
    lifo->data = realloc(lifo->data, newcap);
    lifo->capacity = newcap;
  }

  lifo->data[lifo->size++] = data;
}

void *lifo_pop(lifo_t* lifo)
{
  /*
   *  Do not decide on substraction because it may cause unsigned underflow:
   *  if (lifo->size <= lifo->capacity - CAPACITY_ALIGN)
   *  if (lifo->capacity - lifo->size >= CAPACITY_ALIGN)
   */
  if (lifo->size + CAPACITY_ALIGN <= lifo->capacity)
  {
    /* Same for (lifo->size - 1)... */
    size_t newcap = CHOP(lifo->size + CAPACITY_ALIGN - 1, CAPACITY_ALIGN) - 1;
    lifo->data = realloc(lifo->data, newcap);
    lifo->capacity = newcap;
  }

  return lifo->data[--lifo->size];   
}

void ltn_set_nodeA(linked_tree_node_t* parent, linked_tree_node_t* child)
{
  parent->nodeA = child;
  child->parent = parent;
}

void ltn_set_nodeB(linked_tree_node_t* parent, linked_tree_node_t* child)
{
  parent->nodeB = child;
  child->parent = parent;
}

void demo_init_layout(HWND hWnd, linked_tree_node_t* root)
{
  linked_tree_node_t* nodeA = calloc(1, sizeof(linked_tree_node_t));
  linked_tree_node_t* nodeB = calloc(1, sizeof(linked_tree_node_t));

  linked_tree_node_t* nodeAA = calloc(1, sizeof(linked_tree_node_t));
  linked_tree_node_t* nodeAB = calloc(1, sizeof(linked_tree_node_t)); 


  /* Set toolbox mock */
  HWND hWndToolboxMock = CreateWindowEx(0, WC_STATIC, L"Toolbox",
      WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL),
      NULL);
  nodeAA->dockFrame.hWnd = hWndToolboxMock;
  nodeAA->dockFrame.fShow = TRUE;
  nodeAA->dockFrame.fCaption = TRUE;


  /* Set viewport mock */
  HWND hWndViewportMock = CreateWindowEx(0, WC_STATIC, L"Viewport",
      WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL),
      NULL);
  nodeAB->dockFrame.hWnd = hWndViewportMock;
  nodeAB->dockFrame.fShow = TRUE;
  nodeAB->dockFrame.fCaption = TRUE;


  /* Set subnodes for nodeA */
  ltn_set_nodeA(nodeA, nodeAA);
  ltn_set_nodeB(nodeA, nodeAB);


  /* Set palette mock */
  HWND hWndPaletteMock = CreateWindowEx(0, WC_STATIC, L"Palette",
      WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL),
      NULL);
  nodeB->dockFrame.hWnd = hWndPaletteMock;
  nodeB->dockFrame.fShow = TRUE;
  nodeB->dockFrame.fCaption = TRUE;


  /* Set subnodes for root */
  ltn_set_nodeA(root, nodeA);
  ltn_set_nodeB(root, nodeB);
}

void dockframe_destroy(dock_frame_t* frame)
{
  if (frame->hWnd)
    DestroyWindow(frame->hWnd);
}

void docknode_destroy(linked_tree_node_t* node)
{
  dockframe_destroy(&node->dockFrame);

  free(node);
}

void dock_draw(linked_tree_node_t* node)
{
  (void)node;
}

void dockhost_paint(dock_host_t* host)
{
  assert(host);

  linked_tree_node_t* node = host->tree->root;
  linked_tree_node_t* prev = NULL;

  lifo_t lifo = {0};
  lifo_init(&lifo);

  do {
    if (node->nodeA && node->nodeA != prev)
    {
      dock_draw(node->nodeA);
      lifo_push(&lifo, (void *) node->nodeA);
      prev = node->nodeA;
      continue;
    }

    if (node->nodeB && node->nodeB != prev)
    {
      dock_draw(node->nodeB);
      lifo_push(&lifo, (void *) node->nodeB);
      prev = node->nodeB;
      continue;
    }

  } while (node = (linked_tree_node_t *) lifo_pop(&lifo));
}

void dockhost_init_tree(linked_tree_t* tree)
{
  tree->root = calloc(1, sizeof(linked_tree_node_t)); 
  tree->node_destroy_callback = docknode_destroy;
}

void dockhost_init(dock_host_t* host)
{
  host->tree->root = calloc(1, sizeof(linked_tree_node_t));
  host->captionHeight = 24;
  host->borderWidth = 2;
  dockhost_init_tree(host->tree);
}

void ltn_detach_from_parent(linked_tree_node_t* node)
{
  linked_tree_node_t* parent = node->parent;

  if (parent)
  {
    if (parent->nodeA == node)
    {
      parent->nodeA = NULL;
    }
    else if (parent->nodeB == node)
    {
      parent->nodeB = NULL;
    }

    node->parent = NULL;
  }
}

void ltn_destroy(linked_tree_t* tree, linked_tree_node_t* node)
{
  assert(node);

  lifo_t node_stack = {0};
  lifo_init(&node_stack);

  do {
    if (node->nodeA)
    {
      lifo_push(&node_stack, node->nodeB);
      continue;
    }

    if (node->nodeB)
    {
      lifo_push(&node_stack, node->nodeB);
      continue;
    }

    ltn_detach_from_parent(node);
    tree->node_destroy_callback(node);

  } while (node = (linked_tree_node_t*) lifo_pop(&node_stack));
}

void Dock_DestroyInner(linked_tree_node_t* node)
{
  if (!node)
    return;

  if (node->nodeA)
  {
    DestroyWindow(node->nodeA->dockFrame.hWnd);
    Dock_DestroyInner(node->nodeA);
    free(node->nodeA);
    node->nodeA = NULL;
  }

  if (node->nodeB)
  {
    DestroyWindow(node->nodeB->dockFrame.hWnd);
    Dock_DestroyInner(node->nodeB);
    free(node->nodeB);
    node->nodeB = NULL;
  }
}

void DockFrame_Paint(dock_frame_t* frame, HDC hdc)
{
  TextOut(hdc, 10, 10, frame->lpszCaption, 80);
}

/**
 * Paint dock containers using in-order traversion

 * @param host Pointer to instance of DockHost
 * @param hdc Handle to valid Win32 screen device context
 */
void DockHost_Paint(dock_host_t* host, HDC hdc)
{
  linked_tree_node_t* current = host->tree->root;
  lifo_t stack = {0};
  lifo_init(&stack);
  BOOL done = FALSE;

  while (!done)
  {
    /* Reach the left most node of the current */
    if (current != NULL)
    {
      /* place pointer to a tree node on the stack before traversing the node's
         left subtree */
      lifo_push(&stack, current);
      current = current->nodeA;
    }

    /* Backtrack from the empty subtree and visit the node at the top of the
       stack; however, if the stack is empty, you are done */
    else {
      if (!stack.size)
      {
        current = lifo_pop(&stack);
        DockFrame_Paint(&current->dockFrame, hdc);

        current = current->nodeB;
      }
      else {
        done = TRUE;
      }
    }
  }

}

BOOL Dock_GetClientRect(HWND hWnd, dock_host_t* host, linked_tree_node_t* node,
    RECT* rcResult)
{
  if (!host || !node)
    return FALSE;

  host->borderWidth;

  RECT rect = {0};
  GetClientRect(hWnd, &rect);

  linked_tree_node_t* prev = host->tree->root;
  for (;;)
  {
    if (node->nodeA)
    {
      prev = node->nodeA;
    }

    if (node->nodeB)
    {
      prev = node->nodeB;
    }
    
    if (prev == node)
    {
      memcpy(rcResult, &rect, sizeof(RECT));
      return TRUE;
    }
  }

  rcResult = NULL;
  return FALSE;
}

void DockNode_paint_$recursive(HWND hWnd, dock_host_t* host,
    linked_tree_node_t* node, HDC hDC)
{
  if (!(host && node))
    return;

  int borderWidth = host->borderWidth;

  RECT rc = {0};
  GetClientRect(hWnd, &rc); 

  if (node->dockFrame.fCaption)
  {
    /* Draw caption background */
    RECT rcCaption = {
      rc.left  + borderWidth,
      rc.top   + borderWidth,
      rc.right - borderWidth, 
      rc.top   + borderWidth + host->captionHeight
    };

    FillRect(hDC, &rcCaption, hCaptionBrush); 

    /* Draw caption text */
    size_t len = 0;
    StringCchLength(node->dockFrame.lpszCaption, STRSAFE_MAX_CCH, &len);

    SetBkColor(hDC, RGB(0xFF, 0x00, 0x00)); /* TODO: unhardcode */
    SetTextColor(hDC, RGB(0xFF, 0xFF, 0xFF));

    TextOut(hDC, rcCaption.left + 4, rcCaption.top + borderWidth,
        node->dockFrame.lpszCaption, len);
  }

  if (node->nodeA)
  {
    DockNode_paint_$recursive(hWnd, host, node->nodeA, hDC);
  }

  if (node->nodeB)
  {
    DockNode_paint_$recursive(hWnd, host, node->nodeB, hDC);
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
        hCaptionBrush = CreateSolidBrush(RGB(0xFF, 0x00, 0x00));

        linked_tree_node_t node;
        demo_init_layout(hWnd, &node);
      }
      break;
    case WM_SIZE:
      {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
      }
      break;
    case WM_PAINT:
      {
        PAINTSTRUCT ps = {0};
        HDC hDC = BeginPaint(hWnd, &ps);

        DockHost_Paint();

        EndPaint(hWnd, &ps);
      }
      break;
    case WM_DESTROY:
      DeleteObject(hCaptionBrush);
      break;
    case WM_LBUTTONDOWN:
      {
      }
      break;

    case WM_MOUSEMOVE:
      {
      }
      break;

    case WM_LBUTTONUP:
      {
        signed short x = GET_X_LPARAM(lParam);
        signed short y = GET_Y_LPARAM(lParam);
      }
      break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
      break;
  }

  return 0;
}

const WCHAR szDockHostClassName[] = L"Win32Class_DockHost";

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
  wcex.lpszClassName = szDockHostClassName;
  wcex.lpszMenuName = NULL;
  wcex.hIconSm = NULL;
  ATOM atom = RegisterClassEx(&wcex);
  assert(atom);
  return atom;
}

HWND DockHost_Create(HWND hParent)
{
  dock_host_t* host = calloc(1, sizeof(dock_host_t));

  HWND hWnd = CreateWindowEx(0, szDockHostClassName, L"DockHost",
      WS_CHILD | WS_VISIBLE, 0, 0, 320, 240, hParent, (HMENU)NULL,
      GetModuleHandle(NULL), (LPVOID)host);
  assert(hWnd);

  return hWnd;
}
