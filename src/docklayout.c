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

	switch (nDockSide)
	{
	case DKS_LEFT:
		pRect->left = 0;
		pRect->right = DOCKLAYOUT_ZONE_TAB_THICKNESS;
		pRect->top = min(iOffset, cy);
		pRect->bottom = min(pRect->top + DOCKLAYOUT_ZONE_TAB_LENGTH, cy);
		return TRUE;

	case DKS_RIGHT:
		pRect->right = cx;
		pRect->left = max(cx - DOCKLAYOUT_ZONE_TAB_THICKNESS, 0);
		pRect->top = min(iOffset, cy);
		pRect->bottom = min(pRect->top + DOCKLAYOUT_ZONE_TAB_LENGTH, cy);
		return TRUE;

	case DKS_TOP:
		pRect->top = 0;
		pRect->bottom = DOCKLAYOUT_ZONE_TAB_THICKNESS;
		pRect->left = min(iOffset, cx);
		pRect->right = min(pRect->left + DOCKLAYOUT_ZONE_TAB_LENGTH, cx);
		return TRUE;

	case DKS_BOTTOM:
		pRect->bottom = cy;
		pRect->top = max(cy - DOCKLAYOUT_ZONE_TAB_THICKNESS, 0);
		pRect->left = min(iOffset, cx);
		pRect->right = min(pRect->left + DOCKLAYOUT_ZONE_TAB_LENGTH, cx);
		return TRUE;
	}

	return FALSE;
}
