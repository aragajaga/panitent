#include "precomp.h"

#include "dockhostcaption.h"

#include "dockpolicy.h"
#include "panitentapp.h"
#include "theme.h"
#include "toolwndframe.h"
#include "util/tree.h"

static const int kDockCaptionHeight = 14;
static const int kDockCaptionInset = 3;
static const int kDockWindowButtonSize = 14;
static const int kDockWindowButtonSpacing = 3;

static void DockCaption_GetMetrics(CaptionFrameMetrics* pMetrics)
{
    if (!pMetrics)
    {
        return;
    }

    pMetrics->borderSize = kDockCaptionInset;
    pMetrics->captionHeight = kDockCaptionHeight;
    pMetrics->buttonSpacing = kDockWindowButtonSpacing;
    pMetrics->textPaddingLeft = kDockCaptionInset;
    pMetrics->textPaddingRight = kDockCaptionInset;
    pMetrics->textPaddingY = 0;
}

static int DockCaption_BuildButtons(DockData* pDockData, BOOL bPinFirst, int iPinGlyph, BOOL bIncludeChevron, CaptionButton* pButtons, int cButtons)
{
    if (!pDockData || !pButtons || cButtons <= 0)
    {
        return 0;
    }

    BOOL bCanClose = DockPolicy_CanClosePanel(pDockData->nRole, pDockData->lpszName);
    BOOL bCanPin = DockPolicy_CanPinPanel(pDockData->nRole, pDockData->lpszName);
    int nCount = 0;

    if (bPinFirst && bCanPin && nCount < cButtons)
    {
        pButtons[nCount++] = (CaptionButton){ (SIZE){ kDockWindowButtonSize, kDockWindowButtonSize }, iPinGlyph, DCB_PIN };
    }
    if (bCanClose && nCount < cButtons)
    {
        pButtons[nCount++] = (CaptionButton){ (SIZE){ kDockWindowButtonSize, kDockWindowButtonSize }, CAPTION_GLYPH_CLOSE_TILE, DCB_CLOSE };
    }
    if (!bPinFirst && bCanPin && nCount < cButtons)
    {
        pButtons[nCount++] = (CaptionButton){ (SIZE){ kDockWindowButtonSize, kDockWindowButtonSize }, iPinGlyph, DCB_PIN };
    }
    if (bIncludeChevron && nCount < cButtons)
    {
        pButtons[nCount++] = (CaptionButton){ (SIZE){ kDockWindowButtonSize, kDockWindowButtonSize }, CAPTION_GLYPH_CHEVRON_TILE, DCB_MORE };
    }

    return nCount;
}

static BOOL DockCaption_BuildLayout(DockData* pDockData, BOOL bPinFirst, CaptionFrameLayout* pLayout)
{
    if (!pDockData || !pLayout || !pDockData->bShowCaption)
    {
        return FALSE;
    }

    CaptionFrameMetrics metrics = { 0 };
    DockCaption_GetMetrics(&metrics);

    CaptionButton buttons[3] = { 0 };
    int nButtons = DockCaption_BuildButtons(
        pDockData,
        bPinFirst,
        bPinFirst ? CAPTION_GLYPH_PIN_DIAGONAL_TILE : CAPTION_GLYPH_PIN_VERTICAL_TILE,
        TRUE,
        buttons,
        ARRAYSIZE(buttons));
    return CaptionFrame_BuildLayout(&pDockData->rc, &metrics, buttons, nButtons, pLayout);
}

static BOOL DockCaption_GetButtonRect(DockData* pDockData, int nButtonType, RECT* pRect)
{
    if (!pDockData || !pRect || nButtonType == DCB_NONE)
    {
        return FALSE;
    }

    CaptionFrameLayout layout = { 0 };
    if (!DockCaption_BuildLayout(pDockData, FALSE, &layout))
    {
        return FALSE;
    }

    return CaptionFrame_GetButtonRect(&layout, nButtonType, pRect);
}

BOOL Dock_CaptionHitTest(DockData* pDockData, int x, int y)
{
    CaptionFrameLayout layout = { 0 };
    if (!DockCaption_BuildLayout(pDockData, FALSE, &layout))
    {
        return FALSE;
    }

    POINT pt = { x, y };
    return PtInRect(&layout.rcCaption, pt);
}

BOOL Dock_CloseButtonHitTest(DockData* pDockData, int x, int y)
{
    RECT rcButton = { 0 };
    if (!DockCaption_GetButtonRect(pDockData, DCB_CLOSE, &rcButton))
    {
        return FALSE;
    }

    POINT pt = { x, y };
    return PtInRect(&rcButton, pt);
}

BOOL Dock_PinButtonHitTest(DockData* pDockData, int x, int y)
{
    RECT rcButton = { 0 };
    if (!DockCaption_GetButtonRect(pDockData, DCB_PIN, &rcButton))
    {
        return FALSE;
    }

    POINT pt = { x, y };
    return PtInRect(&rcButton, pt);
}

BOOL Dock_MoreButtonHitTest(DockData* pDockData, int x, int y)
{
    RECT rcButton = { 0 };
    if (!DockCaption_GetButtonRect(pDockData, DCB_MORE, &rcButton))
    {
        return FALSE;
    }

    POINT pt = { x, y };
    return PtInRect(&rcButton, pt);
}

void DockNode_Paint(DockHostWindow* pDockHostWindow, TreeNode* pNodeParent, HDC hdc, HBRUSH hCaptionBrush)
{
    UNREFERENCED_PARAMETER(hCaptionBrush);

    if (!pNodeParent)
    {
        return;
    }

    TreeTraversalRLOT dockNodeTraversal;
    TreeTraversalRLOT_Init(&dockNodeTraversal, pNodeParent);

    TreeNode* pCurrentNode = NULL;
    while (pCurrentNode = TreeTraversalRLOT_GetNext(&dockNodeTraversal))
    {
        DockData* pDockData = (DockData*)pCurrentNode->data;
        if (!pDockData)
        {
            continue;
        }

        if (pDockData->bShowCaption)
        {
            CaptionFrameLayout layout = { 0 };
            if (DockCaption_BuildLayout(pDockData, FALSE, &layout))
            {
                CaptionFramePalette palette = { 0 };
                PanitentTheme_GetCaptionPalette(&palette);

                int nHotButton = DCB_NONE;
                int nPressedButton = DCB_NONE;
                if (pDockHostWindow)
                {
                    if (pDockHostWindow->pCaptionHotNode == pCurrentNode)
                    {
                        nHotButton = pDockHostWindow->nCaptionHotButton;
                    }
                    if (pDockHostWindow->pCaptionPressedNode == pCurrentNode)
                    {
                        nPressedButton = pDockHostWindow->nCaptionPressedButton;
                    }
                }

                HFONT guiFont = PanitentApp_GetUIFont(PanitentApp_Instance());
                CaptionFrame_DrawStateful(hdc, &layout, &palette, pDockData->lpszCaption, guiFont, nHotButton, nPressedButton);
            }

            if (pCurrentNode->data && ((DockData*)pCurrentNode->data)->hWnd)
            {
                InvalidateRect(((DockData*)pCurrentNode->data)->hWnd, NULL, FALSE);
            }
        }
    }

    TreeTraversalRLOT_Destroy(&dockNodeTraversal);
}
