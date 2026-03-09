#include "precomp.h"

#include "dockdefaultlayoutmodel.h"

#include "docklayout.h"
#include "dockmodelops.h"

typedef struct DockDefaultLayoutBuildContext
{
    uint32_t uNextNodeId;
} DockDefaultLayoutBuildContext;

static DockModelNode* DockDefaultLayoutModel_CreateNode(
    DockDefaultLayoutBuildContext* pContext,
    DockNodeRole nRole,
    DockPaneKind nPaneKind,
    int nDockSide,
    DWORD dwStyle,
    int iGripPos,
    BOOL bShowCaption,
    PCWSTR pszName,
    PCWSTR pszCaption,
    PanitentDockViewId nViewId)
{
    DockModelNode* pNode = (DockModelNode*)calloc(1, sizeof(DockModelNode));
    if (!pNode || !pContext)
    {
        free(pNode);
        return NULL;
    }

    pNode->uNodeId = pContext->uNextNodeId++;
    pNode->uViewId = (uint32_t)nViewId;
    pNode->nRole = nRole;
    pNode->nPaneKind = nPaneKind;
    pNode->nDockSide = nDockSide;
    pNode->dwStyle = dwStyle;
    pNode->iGripPos = (short)iGripPos;
    pNode->fGripPos = -1.0f;
    pNode->bShowCaption = bShowCaption;
    pNode->bCollapsed = FALSE;
    if (pszName)
    {
        wcscpy_s(pNode->szName, ARRAYSIZE(pNode->szName), pszName);
    }
    if (pszCaption)
    {
        wcscpy_s(pNode->szCaption, ARRAYSIZE(pNode->szCaption), pszCaption);
    }
    return pNode;
}

static DockModelNode* DockDefaultLayoutModel_CreatePanel(DockDefaultLayoutBuildContext* pContext, PCWSTR pszName, PanitentDockViewId nViewId)
{
    return DockDefaultLayoutModel_CreateNode(
        pContext,
        DOCK_ROLE_PANEL,
        DOCK_PANE_TOOL,
        DKS_NONE,
        DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL,
        64,
        TRUE,
        pszName,
        pszName,
        nViewId);
}

static DockModelNode* DockDefaultLayoutModel_CreateZone(DockDefaultLayoutBuildContext* pContext, int nDockSide, PCWSTR pszName)
{
    return DockDefaultLayoutModel_CreateNode(
        pContext,
        DOCK_ROLE_ZONE,
        DOCK_PANE_NONE,
        nDockSide,
        DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL,
        64,
        FALSE,
        pszName,
        NULL,
        PNT_DOCK_VIEW_NONE);
}

static DockModelNode* DockDefaultLayoutModel_CreateShellSplit(
    DockDefaultLayoutBuildContext* pContext,
    PCWSTR pszName,
    DWORD dwStyle,
    int iGripPos,
    DockModelNode* pChild1,
    DockModelNode* pChild2)
{
    DockModelNode* pNode = DockDefaultLayoutModel_CreateNode(
        pContext,
        DOCK_ROLE_SHELL_SPLIT,
        DOCK_PANE_NONE,
        DKS_NONE,
        dwStyle,
        iGripPos,
        FALSE,
        pszName,
        NULL,
        PNT_DOCK_VIEW_NONE);
    if (!pNode)
    {
        return NULL;
    }
    pNode->pChild1 = pChild1;
    pNode->pChild2 = pChild2;
    return pNode;
}

DockModelNode* DockDefaultLayoutModel_CreateMain(void)
{
    DockDefaultLayoutBuildContext context = { 1 };

    DockModelNode* pRoot = DockDefaultLayoutModel_CreateNode(&context, DOCK_ROLE_ROOT, DOCK_PANE_NONE, DKS_NONE, DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL, 64, FALSE, L"Root", NULL, PNT_DOCK_VIEW_NONE);
    DockModelNode* pWorkspace = DockDefaultLayoutModel_CreateNode(&context, DOCK_ROLE_WORKSPACE, DOCK_PANE_DOCUMENT, DKS_NONE, DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL, 64, FALSE, L"WorkspaceContainer", NULL, PNT_DOCK_VIEW_WORKSPACE);
    DockModelNode* pZoneLeft = DockDefaultLayoutModel_CreateZone(&context, DKS_LEFT, L"DockZone.Left");
    DockModelNode* pZoneRight = DockDefaultLayoutModel_CreateZone(&context, DKS_RIGHT, L"DockZone.Right");
    DockModelNode* pZoneTop = DockDefaultLayoutModel_CreateZone(&context, DKS_TOP, L"DockZone.Top");
    DockModelNode* pZoneBottom = DockDefaultLayoutModel_CreateZone(&context, DKS_BOTTOM, L"DockZone.Bottom");
    DockModelNode* pToolbox = DockDefaultLayoutModel_CreatePanel(&context, L"Toolbox", PNT_DOCK_VIEW_TOOLBOX);
    DockModelNode* pGLWindow = DockDefaultLayoutModel_CreatePanel(&context, L"GLWindow", PNT_DOCK_VIEW_GLWINDOW);
    DockModelNode* pPalette = DockDefaultLayoutModel_CreatePanel(&context, L"Palette", PNT_DOCK_VIEW_PALETTE);
    DockModelNode* pLayers = DockDefaultLayoutModel_CreatePanel(&context, L"Layers", PNT_DOCK_VIEW_LAYERS);
    DockModelNode* pOptionBar = DockDefaultLayoutModel_CreatePanel(&context, L"Option Bar", PNT_DOCK_VIEW_OPTIONBAR);

    if (!pRoot || !pWorkspace || !pZoneLeft || !pZoneRight || !pZoneTop || !pZoneBottom ||
        !pToolbox || !pGLWindow || !pPalette || !pLayers || !pOptionBar)
    {
        DockModel_Destroy(pRoot);
        DockModel_Destroy(pWorkspace);
        DockModel_Destroy(pZoneLeft);
        DockModel_Destroy(pZoneRight);
        DockModel_Destroy(pZoneTop);
        DockModel_Destroy(pZoneBottom);
        DockModel_Destroy(pToolbox);
        DockModel_Destroy(pGLWindow);
        DockModel_Destroy(pPalette);
        DockModel_Destroy(pLayers);
        DockModel_Destroy(pOptionBar);
        return NULL;
    }

    if (!DockModelOps_AppendPanelToZone(pZoneLeft, DKS_LEFT, pToolbox) ||
        !DockModelOps_AppendPanelToZone(pZoneRight, DKS_RIGHT, pGLWindow) ||
        !DockModelOps_AppendPanelToZone(pZoneRight, DKS_RIGHT, pPalette) ||
        !DockModelOps_AppendPanelToZone(pZoneRight, DKS_RIGHT, pLayers) ||
        !DockModelOps_AppendPanelToZone(pZoneBottom, DKS_BOTTOM, pOptionBar))
    {
        DockModel_Destroy(pRoot);
        DockModel_Destroy(pWorkspace);
        DockModel_Destroy(pZoneLeft);
        DockModel_Destroy(pZoneRight);
        DockModel_Destroy(pZoneTop);
        DockModel_Destroy(pZoneBottom);
        return NULL;
    }

    DockModelNode* pCenterRight = DockDefaultLayoutModel_CreateShellSplit(
        &context,
        L"DockShell.CenterRight",
        DGA_END | DGP_ABSOLUTE | DGD_HORIZONTAL,
        300,
        pWorkspace,
        pZoneRight);
    DockModelNode* pMiddleBand = DockDefaultLayoutModel_CreateShellSplit(
        &context,
        L"DockShell.MiddleBand",
        DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL,
        220,
        pZoneLeft,
        pCenterRight);
    DockModelNode* pTopMiddle = DockDefaultLayoutModel_CreateShellSplit(
        &context,
        L"DockShell.TopMiddle",
        DGA_START | DGP_ABSOLUTE | DGD_VERTICAL,
        72,
        pZoneTop,
        pMiddleBand);
    DockModelNode* pShellRoot = DockDefaultLayoutModel_CreateShellSplit(
        &context,
        L"DockShell.Root",
        DGA_END | DGP_ABSOLUTE | DGD_VERTICAL,
        72,
        pTopMiddle,
        pZoneBottom);

    if (!pCenterRight || !pMiddleBand || !pTopMiddle || !pShellRoot)
    {
        DockModel_Destroy(pRoot);
        return NULL;
    }

    pRoot->pChild1 = pShellRoot;
    return pRoot;
}
