#include "precomp.h"

#include "dockhostapplytxn.h"

#include "dockhostpreserve.h"
#include "dockmodel.h"
#include "dockmodelops.h"
#include "panitentapp.h"

BOOL DockHostApplyTxn_Begin(DockHostApplyTxn* pTxn, DockHostWindow* pDockHostWindow, HWND hWndIncludeHidden)
{
    TreeNode* pLiveRoot = NULL;

    if (!pTxn || !pDockHostWindow)
    {
        return FALSE;
    }

    memset(pTxn, 0, sizeof(*pTxn));
    pTxn->pRollbackModel = DockModel_CaptureHostLayout(pDockHostWindow);
    pTxn->pTargetModel = DockModelOps_CloneTree(pTxn->pRollbackModel);
    if (!pTxn->pRollbackModel || !pTxn->pTargetModel)
    {
        DockHostApplyTxn_End(pTxn);
        return FALSE;
    }

    pTxn->context.pPanitentApp = PanitentApp_Instance();
    pLiveRoot = DockHostWindow_GetRoot(pDockHostWindow);
    if (hWndIncludeHidden)
    {
        DockHostPreserve_CollectViewsRecursiveEx(
            &pTxn->context,
            pLiveRoot,
            pTxn->pRollbackModel,
            pLiveRoot,
            hWndIncludeHidden);
    }
    else {
        DockHostPreserve_CollectViewsRecursive(
            &pTxn->context,
            pLiveRoot,
            pTxn->pRollbackModel,
            pLiveRoot);
    }

    return TRUE;
}

void DockHostApplyTxn_Rollback(DockHostWindow* pDockHostWindow, DockHostApplyTxn* pTxn)
{
    if (!pDockHostWindow || !pTxn || !pTxn->pRollbackModel)
    {
        return;
    }

    DockHostApplyCore_ApplyModel(pDockHostWindow, pTxn->pRollbackModel, &pTxn->context);
}

void DockHostApplyTxn_End(DockHostApplyTxn* pTxn)
{
    if (!pTxn)
    {
        return;
    }

    DockModel_Destroy(pTxn->pRollbackModel);
    DockModel_Destroy(pTxn->pTargetModel);
    memset(pTxn, 0, sizeof(*pTxn));
}
