#include "precomp.h"

#include "docklayout.h"

DWORD DockLayout_GetZoneStackStyle(int nDockSide)
{
	DWORD dwStyle = DGA_END | DGP_ABSOLUTE;
	if (nDockSide == DKS_LEFT || nDockSide == DKS_RIGHT)
	{
		dwStyle |= DGD_VERTICAL;
	}
	else {
		dwStyle |= DGD_HORIZONTAL;
	}

	return dwStyle;
}

int DockLayout_GetZoneStackGrip(int nDockSide, int iDockSize)
{
	if (nDockSide == DKS_LEFT || nDockSide == DKS_RIGHT)
	{
		return 220;
	}

	iDockSize = iDockSize > 0 ? iDockSize : 260;
	return max(160, min(iDockSize, 520));
}

int DockLayout_GetZoneSplitGrip(int nDockSide, int iDockSize)
{
	int iGrip = iDockSize > 0 ? iDockSize : 220;
	if (nDockSide == DKS_LEFT || nDockSide == DKS_RIGHT)
	{
		iGrip = max(160, min(iGrip, 500));
	}
	else {
		iGrip = max(56, min(iGrip, 360));
	}

	return iGrip;
}

BOOL DockLayout_GetZoneTabRect(const RECT* pClientRect, int nDockSide, int iTabIndex, int nTabs, RECT* pRect)
{
	if (!pClientRect || !pRect)
	{
		return FALSE;
	}

	int cx = pClientRect->right - pClientRect->left;
	int cy = pClientRect->bottom - pClientRect->top;
	if (cx <= 0 || cy <= 0)
	{
		return FALSE;
	}

	int nSafeTabs = max(1, nTabs);
	int iSafeIndex = max(0, min(iTabIndex, nSafeTabs - 1));
	int iOffset = iSafeIndex * (DOCKLAYOUT_ZONE_TAB_LENGTH + DOCKLAYOUT_ZONE_TAB_GAP);
	int iInset = min(DOCKLAYOUT_ZONE_TAB_INSET, min(cx, cy) / 2);
	int iUsableWidth = max(0, cx - iInset * 2);
	int iUsableHeight = max(0, cy - iInset * 2);

	switch (nDockSide)
	{
	case DKS_LEFT:
		pRect->left = 0;
		pRect->right = DOCKLAYOUT_ZONE_TAB_THICKNESS;
		pRect->top = iInset + min(iOffset, iUsableHeight);
		pRect->bottom = min(pRect->top + DOCKLAYOUT_ZONE_TAB_LENGTH, cy - iInset);
		pRect->top = min(pRect->top, pRect->bottom);
		return TRUE;

	case DKS_RIGHT:
		pRect->right = cx;
		pRect->left = max(cx - DOCKLAYOUT_ZONE_TAB_THICKNESS, 0);
		pRect->top = iInset + min(iOffset, iUsableHeight);
		pRect->bottom = min(pRect->top + DOCKLAYOUT_ZONE_TAB_LENGTH, cy - iInset);
		pRect->top = min(pRect->top, pRect->bottom);
		return TRUE;

	case DKS_TOP:
		pRect->top = 0;
		pRect->bottom = DOCKLAYOUT_ZONE_TAB_THICKNESS;
		pRect->left = iInset + min(iOffset, iUsableWidth);
		pRect->right = min(pRect->left + DOCKLAYOUT_ZONE_TAB_LENGTH, cx - iInset);
		pRect->left = min(pRect->left, pRect->right);
		return TRUE;

	case DKS_BOTTOM:
		pRect->bottom = cy;
		pRect->top = max(cy - DOCKLAYOUT_ZONE_TAB_THICKNESS, 0);
		pRect->left = iInset + min(iOffset, iUsableWidth);
		pRect->right = min(pRect->left + DOCKLAYOUT_ZONE_TAB_LENGTH, cx - iInset);
		pRect->left = min(pRect->left, pRect->right);
		return TRUE;
	}

	return FALSE;
}

BOOL DockLayout_GetDockPreviewRect(const RECT* pHostRect, int nDockSide, RECT* pRect)
{
	if (!pHostRect || !pRect)
	{
		return FALSE;
	}

	int width = pHostRect->right - pHostRect->left;
	int height = pHostRect->bottom - pHostRect->top;
	if (width <= 0 || height <= 0)
	{
		return FALSE;
	}

	int edge = min(width, height) / 3;
	edge = max(edge, 96);
	edge = min(edge, 420);

	*pRect = *pHostRect;
	switch (nDockSide)
	{
	case DKS_LEFT:
		pRect->right = min(pRect->left + edge, pHostRect->right);
		return TRUE;

	case DKS_RIGHT:
		pRect->left = max(pRect->right - edge, pHostRect->left);
		return TRUE;

	case DKS_TOP:
		pRect->bottom = min(pRect->top + edge, pHostRect->bottom);
		return TRUE;

	case DKS_BOTTOM:
		pRect->top = max(pRect->bottom - edge, pHostRect->top);
		return TRUE;
	}

	return FALSE;
}

int DockLayout_ClampSplitGrip(int iSpan, int iGrip, int iMinPaneSize)
{
	if (iSpan <= 0)
	{
		return 0;
	}

	int iMinGrip = max(iMinPaneSize, 0);
	int iMaxGrip = iSpan - iMinGrip;
	if (iMaxGrip < iMinGrip)
	{
		iMinGrip = 0;
		iMaxGrip = iSpan;
	}

	return max(iMinGrip, min(iGrip, iMaxGrip));
}

int DockLayout_AdjustSplitGripFromDelta(DWORD dwStyle, int iStartGrip, int iDelta, int iSpan, int iMinPaneSize)
{
	int iNext = iStartGrip;
	if (dwStyle & DGA_END)
	{
		iNext -= iDelta;
	}
	else {
		iNext += iDelta;
	}

	return DockLayout_ClampSplitGrip(iSpan, iNext, iMinPaneSize);
}
