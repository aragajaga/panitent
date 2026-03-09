#include "precomp.h"

#include "dockhostlayout.h"

#include "docklayout.h"
#include "docktypes.h"
#include "win32/util.h"

static BOOL DockHostLayout_NodeHasVisibleWindowInZone(TreeNode* pNode, DockData* pZoneData)
{
	if (!pNode)
	{
		return FALSE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData && DockNodeRole_IsZone(pDockData->nRole, pDockData->lpszName))
	{
		pZoneData = pDockData;
	}

	if (pDockData && pDockData->hWnd && IsWindow(pDockData->hWnd))
	{
		if (pDockData->bCollapsed)
		{
			return FALSE;
		}

		if (!pZoneData)
		{
			return TRUE;
		}

		if (!pZoneData->hWndActiveTab || !IsWindow(pZoneData->hWndActiveTab))
		{
			pZoneData->hWndActiveTab = pDockData->hWnd;
		}

		return pDockData->hWnd == pZoneData->hWndActiveTab;
	}

	return DockHostLayout_NodeHasVisibleWindowInZone(pNode->node1, pZoneData) ||
		DockHostLayout_NodeHasVisibleWindowInZone(pNode->node2, pZoneData);
}

BOOL DockHostLayout_NodeHasVisibleWindow(TreeNode* pNode)
{
	return DockHostLayout_NodeHasVisibleWindowInZone(pNode, NULL);
}

static BOOL DockHostLayout_NodeUsesProportionalGrip(TreeNode* pNode)
{
	if (!pNode || !pNode->data)
	{
		return FALSE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	return DockNodeRole_UsesProportionalGrip(pDockData->nRole, pDockData->lpszName);
}

BOOL DockHostLayout_IsSplitVertical(TreeNode* pNode)
{
    if (!pNode || !pNode->data)
    {
        return FALSE;
    }

    return ((((DockData*)pNode->data)->dwStyle & DGD_VERTICAL) != 0) ? TRUE : FALSE;
}

BOOL DockHostLayout_GetSplitRect(TreeNode* pNode, RECT* pRect, int iBorderWidth)
{
    if (!pNode || !pRect || !pNode->data || !pNode->node1 || !pNode->node2)
    {
        return FALSE;
    }

    DockData* pDockData = (DockData*)pNode->data;
    if (pDockData->dwStyle & DGP_RELATIVE)
    {
        return FALSE;
    }

    if (!DockHostLayout_NodeHasVisibleWindow(pNode->node1) || !DockHostLayout_NodeHasVisibleWindow(pNode->node2))
    {
        return FALSE;
    }

    RECT rcClient = { 0 };
    if (!DockData_GetClientRect(pDockData, &rcClient))
    {
        return FALSE;
    }

    BOOL bVertical = DockHostLayout_IsSplitVertical(pNode);
    int iSpan = bVertical ? Win32_Rect_GetHeight(&rcClient) : Win32_Rect_GetWidth(&rcClient);
    if (iSpan <= 0)
    {
        return FALSE;
    }

    int iGrip = DockLayout_ClampSplitGrip(iSpan, pDockData->iGripPos, 0);
    int iPos = 0;
    if (pDockData->dwStyle & DGA_END)
    {
        iPos = bVertical ? (rcClient.bottom - iGrip) : (rcClient.right - iGrip);
    }
    else {
        iPos = bVertical ? (rcClient.top + iGrip) : (rcClient.left + iGrip);
    }

    int iThickness = max(iBorderWidth, 4);
    int iHalf = iThickness / 2;

    RECT rcSplit = rcClient;
    if (bVertical)
    {
        rcSplit.top = iPos - iHalf;
        rcSplit.bottom = rcSplit.top + iThickness;
    }
    else {
        rcSplit.left = iPos - iHalf;
        rcSplit.right = rcSplit.left + iThickness;
    }

    if (!IntersectRect(pRect, &rcSplit, &rcClient))
    {
        return FALSE;
    }

    return Win32_Rect_GetWidth(pRect) > 0 && Win32_Rect_GetHeight(pRect) > 0;
}

static void DockHostLayout_AssignRectsRecursive(TreeNode* pNode, int iBorderWidth, int iMinPaneSize)
{
	if (!pNode || !pNode->data)
	{
		return;
	}

	DockData* pDockData = (DockData*)pNode->data;
	TreeNode* pChildNode1 = pNode->node1;
	TreeNode* pChildNode2 = pNode->node2;

	if (pChildNode1 && pChildNode2)
	{
		RECT rcClient = { 0 };
		DockData_GetClientRect(pDockData, &rcClient);

		RECT rcNode1 = rcClient;
		DWORD dwStyle = pDockData->dwStyle;
		int iGripPos = pDockData->iGripPos;

		BOOL bHasContentNode1 = DockHostLayout_NodeHasVisibleWindow(pChildNode1);
		BOOL bHasContentNode2 = DockHostLayout_NodeHasVisibleWindow(pChildNode2);
		int iSpan = (dwStyle & DGD_VERTICAL) ? Win32_Rect_GetHeight(&rcClient) : Win32_Rect_GetWidth(&rcClient);
		if (!(dwStyle & DGP_RELATIVE))
		{
			iGripPos = DockLayout_ClampSplitGrip(iSpan, iGripPos, iMinPaneSize);
			if (DockHostLayout_NodeUsesProportionalGrip(pNode) &&
				bHasContentNode1 &&
				bHasContentNode2)
			{
				if (pDockData->fGripPos >= 0.0f)
				{
					iGripPos = DockLayout_ScaleGripFromRatio(iSpan, pDockData->fGripPos, iMinPaneSize);
				}
				else {
					pDockData->fGripPos = DockLayout_GetGripRatio(iSpan, iGripPos, iMinPaneSize);
				}
			}

			pDockData->iGripPos = (short)iGripPos;
		}

		if (!bHasContentNode1)
		{
			iGripPos = (dwStyle & DGA_END) ? iSpan : 0;
		}
		else if (!bHasContentNode2)
		{
			iGripPos = (dwStyle & DGA_END) ? 0 : iSpan;
		}

		if (dwStyle & DGP_RELATIVE)
		{
			rcNode1.right = (rcNode1.right - rcNode1.left) / 2 - iBorderWidth / 2;
		}
		else if (dwStyle & DGA_END)
		{
			if (dwStyle & DGD_VERTICAL)
			{
				rcNode1.bottom = rcNode1.bottom - iGripPos - iBorderWidth / 2;
			}
			else {
				rcNode1.right = rcNode1.right - iGripPos - iBorderWidth / 2;
			}
		}
		else {
			if (dwStyle & DGD_VERTICAL)
			{
				rcNode1.bottom = rcNode1.top + iGripPos - iBorderWidth / 2;
			}
			else {
				rcNode1.right = rcNode1.left + iGripPos - iBorderWidth / 2;
			}
		}

		if (pChildNode1->data)
		{
			((DockData*)pChildNode1->data)->rc = rcNode1;
		}

		RECT rcNode2 = rcClient;
		if (dwStyle & DGP_RELATIVE)
		{
			rcNode2.left += (rcNode2.right - rcNode2.left) / 2 + iBorderWidth / 2;
		}
		else if (dwStyle & DGA_END)
		{
			if (dwStyle & DGD_VERTICAL)
			{
				rcNode2.top = rcNode2.bottom - iGripPos + iBorderWidth / 2;
			}
			else {
				rcNode2.left = rcNode2.right - iGripPos + iBorderWidth / 2;
			}
		}
		else {
			if (dwStyle & DGD_VERTICAL)
			{
				rcNode2.top = rcNode2.top + iGripPos + iBorderWidth / 2;
			}
			else {
				rcNode2.left = rcNode2.left + iGripPos + iBorderWidth / 2;
			}
		}

		if (pChildNode2->data)
		{
			((DockData*)pChildNode2->data)->rc = rcNode2;
		}
	}
	else if (pChildNode1 || pChildNode2) {
		TreeNode* pChild = pChildNode1 ? pChildNode1 : pChildNode2;
		RECT rcClient = { 0 };
		DockData_GetClientRect(pDockData, &rcClient);
		if (pChild && pChild->data)
		{
			((DockData*)pChild->data)->rc = rcClient;
		}
	}

	DockHostLayout_AssignRectsRecursive(pChildNode1, iBorderWidth, iMinPaneSize);
	DockHostLayout_AssignRectsRecursive(pChildNode2, iBorderWidth, iMinPaneSize);
}

void DockHostLayout_AssignRects(TreeNode* pRootNode, int iBorderWidth, int iMinPaneSize)
{
	DockHostLayout_AssignRectsRecursive(pRootNode, iBorderWidth, iMinPaneSize);
}
