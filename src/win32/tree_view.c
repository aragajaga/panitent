#include "tree_view.h"

void TreeViewCtl_PreCreate(TreeViewCtl*);
BOOL TreeViewCtl_OnCreate(TreeViewCtl*, LPCREATESTRUCT);

void TreeViewCtl_Init(TreeViewCtl* window, struct Application* app)
{
  Window_Init(&window->base, app);

  window->base.PreRegister = NULL;
  window->base.PreCreate = TreeViewCtl_PreCreate;

  window->base.OnCreate = TreeViewCtl_OnCreate;
}

TreeViewCtl* TreeViewCtl_Create(struct Application* app)
{
  TreeViewCtl* window = calloc(1, sizeof(TreeViewCtl));
  if (window)
  {
    TreeViewCtl_Init(window, app);
  }

  return window;
}

void TreeViewCtl_PreCreate(LPCREATESTRUCT lpcs)
{
  lpcs->dwExStyle = WS_EX_CLIENTEDGE;
  lpcs->lpszClass = WC_TREEVIEW;
  lpcs->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
}

BOOL TreeViewCtl_OnCreate(TreeViewCtl* window, LPCREATESTRUCT lpcs)
{
  TreeView_SetExtendedStyle(window->base.hWnd, TVS_EX_DOUBLEBUFFER, 0);
  Window_OnCreate(&window->base, lpcs);
}

BOOL TreeViewCtl_DeleteAllItems(TreeViewCtl* window)
{
  return TreeView_DeleteAllItems(window->base.hWnd);
}

BOOL TreeViewCtl_DeleteItem(TreeViewCtl* window, HTREEITEM item)
{
  return TreeView_DeleteItem(window->base.hWnd, item);
}

BOOL TreeViewCtl_Expand(TreeViewCtl* window, HTREEITEM item, BOOL expand)
{
  return TreeView_Expand(window, item, expand ? TVE_EXPAND : TVE_COLLAPSE);
}

UINT TreeViewCtl_GetCheckState(TreeViewCtl* window, HTREEITEM item)
{
  return TreeView_GetCheckState(window->base.hWnd, item);
}

UINT TreeViewCtl_GetCount(TreeViewCtl* window)
{
  return TreeView_GetCount(window->base.hWnd);
}

BOOL TreeViewCtl_GetItem(TreeViewCtl* window, LPTVITEM item)
{
  return TreeView_GetItem(window->base.hWnd, item);
}

LPARAM TreeViewCtl_GetItemData(TreeViewCtl* window, HTREEITEM item)
{
  TVITEM tvi = { 0 };
  tvi.mask = TVIF_PARAM;
  tvi.hItem = item;
  TreeViewCtl_GetItem(window, &tvi);

  return tvi.lParam;
}

HTREEITEM TreeViewCtl_GetNextItem(TreeViewCtl* window, HTREEITEM item, UINT flag)
{
  return TreeViewCtl_GetNextItem(window, item, flag);
}

HTREEITEM TreeViewCtl_GetSelection(TreeViewCtl* window)
{
  return TreeView_GetSelection(window->base.hWnd);
}

HTREEITEM TreeViewCtl_HitTest(TreeViewCtl* window, LPTVHITTESTINFO hti, BOOL bGetCursorPos)
{
  if (bGetCursorPos)
  {
    GetCursorPos(&hti->pt);
    ScreenToClient(window->base.hWnd, &hti->pt);
  }

  return TreeView_HitTest(window->base.hWnd, hti);
}

HTREEITEM TreeViewCtl_InsertItem(TreeViewCtl* window, PCWSTR text, int image, LPARAM lParam, HTREEITEM parent, HTREEITEM insertAfter)
{
  TVITEM tvi = { 0 };
  tvi.mask = TVIF_TEXT | TVIF_PARAM;
  tvi.pszText = (PWSTR)text;
  tvi.lParam = lParam;

  if (image > -1)
  {
    tvi.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.iImage = image;
    tvi.iSelectedImage = image;
  }

  TVINSERTSTRUCT tvis = { 0 };
  tvis.item = tvi;
  tvis.hInsertAfter = insertAfter;
  tvis.hParent = parent;

  return TreeView_InsertItem(window->base.hWnd, &tvis);
}

BOOL TreeViewCtl_SelectItem(TreeViewCtl* window, HTREEITEM item)
{
  return TreeView_Select(window->base.hWnd, item, TVGN_CARET);
}

UINT TreeViewCtl_SetCheckState(TreeViewCtl* window, HTREEITEM item, BOOL check)
{
  TVITEM tvi = { 0 };
  tvi.mask = TVIF_HANDLE | TVIF_STATE;
  tvi.hItem = item;
  tvi.stateMask = TVIS_STATEIMAGEMASK;
  tvi.state = INDEXTOSTATEIMAGEMASK(check + 1);

  return TreeView_SetItem(window->base.hWnd, &tvi);
}

HIMAGELIST TreeViewCtl_SetImageList(TreeViewCtl* window, HIMAGELIST himl, INT image)
{
  return TreeView_SetImageList(window->base.hWnd, himl, image);
}

BOOL TreeViewCtl_SetItem(TreeViewCtl* window, HTREEITEM item, PCWSTR text)
{
  TVITEM tvi = { 0 };
  tvi.mask = TVIF_HANDLE;
  tvi.hItem = item;

  if (!TreeViewCtl_GetItem(window, &tvi))
  {
    return FALSE;
  }

  tvi.mask |= TVIF_TEXT;
  tvi.pszText = (PWSTR)text;
  return TreeView_SetItem(window->base.hWnd, &tvi);
}

int TreeViewCtl_SetItemHeight(TreeViewCtl* window, SHORT cyItem)
{
  return TreeView_SetItemHeight(window->base.hWnd, cyItem);
}
