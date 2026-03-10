#include "precomp.h"

#include "dockhostmodelapply.h"

#include "dockhosttree.h"

BOOL DockHostModelApply_RemoveDockedWindow(DockHostWindow* pDockHostWindow, HWND hWnd, BOOL bKeepWindowAlive)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    TreeNode* pLiveNode = DockNode_FindByHWND(DockHostWindow_GetRoot(pDockHostWindow), hWnd);
    DockData* pLiveData = pLiveNode ? (DockData*)pLiveNode->data : NULL;
    if (!pLiveData)
    {
        return FALSE;
    }

    if (pLiveData->nPaneKind == DOCK_PANE_TOOL)
    {
        return DockHostModelApply_RemoveToolWindow(pDockHostWindow, hWnd, bKeepWindowAlive);
    }
    if (pLiveData->nPaneKind == DOCK_PANE_DOCUMENT)
    {
        return DockHostModelApply_RemoveDocumentWindow(pDockHostWindow, hWnd, bKeepWindowAlive);
    }

    return FALSE;
}
