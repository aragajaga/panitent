#ifndef PANITENT_TREE_VIEW_H_
#define PANITENT_TREE_VIEW_H_

#include "window.h"

typedef struct TreeViewCtl TreeViewCtl;

struct TreeViewCtl {
  struct Window base;
};

void TreeViewCtl_Init(TreeViewCtl*);
TreeViewCtl* TreeViewCtl_Create();

BOOL TreeViewCtl_DeleteAllItems(TreeViewCtl*);
BOOL TreeViewCtl_DeleteItem(TreeViewCtl*, HTREEITEM);
BOOL TreeViewCtl_Expand(TreeViewCtl*, HTREEITEM, BOOL);
UINT TreeViewCtl_GetCheckState(TreeViewCtl*, HTREEITEM);
UINT TreeViewCtl_GetCount(TreeViewCtl*);
BOOL TreeViewCtl_GetItem(TreeViewCtl*, LPTVITEM);
LPARAM TreeViewCtl_GetItemData(TreeViewCtl*, HTREEITEM);
HTREEITEM TreeViewCtl_GetNextItem(TreeViewCtl*, HTREEITEM, UINT);
HTREEITEM TreeViewCtl_GetSelection(TreeViewCtl*);
HTREEITEM TreeViewCtl_HitTest(TreeViewCtl*, LPTVHITTESTINFO, BOOL);
HTREEITEM TreeViewCtl_InsertItem(TreeViewCtl* pTreeViewCtl, PCWSTR pcszText, int nImage, LPARAM lParam, HTREEITEM htiParent, HTREEITEM htiInsertAfter);
BOOL TreeViewCtl_SelectItem(TreeViewCtl*, HTREEITEM);
UINT TreeViewCtl_SetCheckState(TreeViewCtl*, HTREEITEM, BOOL);
HIMAGELIST TreeViewCtl_SetImageList(TreeViewCtl*, HIMAGELIST, INT);
BOOL TreeViewCtl_SetItem(TreeViewCtl*, HTREEITEM, PCWSTR);
int TreeViewCtl_SetItemHeight(TreeViewCtl*, SHORT);

#endif  /* PANITENT_TREE_VIEW_H_ */
