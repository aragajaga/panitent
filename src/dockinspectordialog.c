#include "precomp.h"

#include "dockinspectordialog.h"
#include "panitent.h"
#include "resource.h"
#include "win32/util.h"

#include "dockhost.h"
#include "tree.h"
#include "queue.h"

DockInspectorDialog* DockInspectorDialog_Create();
void DockInspectorDialog_Init(DockInspectorDialog* pDockInspectorDialog, Application *pApp);

INT_PTR DockInspectorDialog_DlgUserProc(DockInspectorDialog* pDockInspectorDialog, UINT message, WPARAM wParam, LPARAM lParam);
void DockInspectorDialog_OnInitDialog(DockInspectorDialog* pDockInspectorDialog);
void DockInspectorDialog_OnOK(DockInspectorDialog* pDockInspectorDialog);
void DockInspectorDialog_OnCancel(DockInspectorDialog* pDockInspectorDialog);

DockInspectorDialog* DockInspectorDialog_Create()
{
    DockInspectorDialog* pDockInspectorDialog = (DockInspectorDialog*)calloc(1, sizeof(DockInspectorDialog));

    if (pDockInspectorDialog)
    {
        DockInspectorDialog_Init(pDockInspectorDialog, (Application*)Panitent_GetApp());
    }    

    return pDockInspectorDialog;
}

void DockInspectorDialog_Init(DockInspectorDialog* pDockInspectorDialog, Application* pApp)
{
    Dialog_Init((Dialog*)pDockInspectorDialog, pApp);

    pDockInspectorDialog->base.DlgUserProc = (FnDialogDlgUserProc)DockInspectorDialog_DlgUserProc;
    pDockInspectorDialog->base.OnInitDialog = (FnDialogOnInitDialog)DockInspectorDialog_OnInitDialog;
    pDockInspectorDialog->base.OnOK = (FnDialogOnOK)DockInspectorDialog_OnOK;
    pDockInspectorDialog->base.OnCancel = (FnDialogOnCancel)DockInspectorDialog_OnCancel;

    pDockInspectorDialog->m_pTreeView = TreeViewCtl_Create(pApp);
}

typedef struct TreeViewNodePair TreeViewNodePair;
struct TreeViewNodePair {
    TreeNode* pTreeNode;
    HTREEITEM hti;
};

void DockInspectorDialog_Update(DockInspectorDialog* pDockInspectorDialog, TreeNode* pTreeRoot)
{
    if (pDockInspectorDialog && pDockInspectorDialog->m_pTreeView)
    {
        TreeViewCtl* pTreeView = pDockInspectorDialog->m_pTreeView;
        TreeViewCtl_DeleteAllItems(pTreeView);

        Queue* pQueue = CreateQueue();

        TreeViewNodePair* pPair = (TreeViewNodePair*)calloc(1, sizeof(TreeViewNodePair));
        pPair->pTreeNode = pTreeRoot;
        pPair->hti = TVI_ROOT;
        Queue_Enqueue(pQueue, pPair);

        while (!Queue_IsEmpty(pQueue))
        {
            pPair = Queue_Dequeue(pQueue);

            TreeNode* pCurrentNode = pPair->pTreeNode;
            HTREEITEM hParentItem = pPair->hti;

            TVINSERTSTRUCT tvInsert = { 0 };
            tvInsert.hParent = hParentItem;
            tvInsert.hInsertAfter = TVI_LAST;
            tvInsert.item.mask = TVIF_TEXT;
            tvInsert.item.lParam = (LPARAM)pCurrentNode->data;

            if (pCurrentNode && pCurrentNode->data && ((DockData*)pCurrentNode->data)->lpszName)
            {
                tvInsert.item.pszText = ((DockData*)pCurrentNode->data)->lpszName;
            }
            else {
                tvInsert.item.pszText = L"< Unknown >";
            }

            HTREEITEM hItem = TreeView_InsertItem(Window_GetHWND((Window*)pTreeView), &tvInsert);

            if (pCurrentNode->node1)
            {
                TreeViewNodePair* pPair = (TreeViewNodePair*)calloc(1, sizeof(TreeViewNodePair));
                pPair->pTreeNode = pCurrentNode->node1;
                pPair->hti = hItem;
                Queue_Enqueue(pQueue, pPair);
            }

            if (pCurrentNode->node2)
            {
                TreeViewNodePair* pPair = (TreeViewNodePair*)calloc(1, sizeof(TreeViewNodePair));
                pPair->pTreeNode = pCurrentNode->node2;
                pPair->hti = hItem;
                Queue_Enqueue(pQueue, pPair);
            }
        }
    }

    pDockInspectorDialog->m_pTreeRoot = pTreeRoot;
}

void DockInspectorDialog_OnInitDialog(DockInspectorDialog* pDockInspectorDialog)
{
    HWND hWnd = Window_GetHWND((Window*)pDockInspectorDialog);
    Window_Attach((Window*)pDockInspectorDialog->m_pTreeView, GetDlgItem(hWnd, IDC_TREE1));

    return;
}

void DockInspectorDialog_OnOK(DockInspectorDialog* pDockInspectorDialog)
{
    EndDialog(Window_GetHWND((Window*)pDockInspectorDialog), 0);
}

void DockInspectorDialog_OnCancel(DockInspectorDialog* pDockInspectorDialog)
{
    EndDialog(Window_GetHWND((Window*)pDockInspectorDialog), 0);
}

INT_PTR DockInspectorDialog_DlgUserProc(DockInspectorDialog* pDockInspectorDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    }

    return Dialog_DefaultDialogProc(pDockInspectorDialog, message, wParam, lParam);
}
