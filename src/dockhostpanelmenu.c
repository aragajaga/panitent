#include "precomp.h"

#include "dockhostpanelmenu.h"

#include "dockhostautohide.h"
#include "dockhostmodelapply.h"
#include "dockhosttree.h"
#include "dockhostzone.h"
#include "dockpolicy.h"
#include "floatingwindowcontainer.h"
#include "win32/window.h"
#include "win32/util.h"

#define IDM_PANEL_DOCK 4101
#define IDM_PANEL_AUTOHIDE 4102
#define IDM_PANEL_MOVETONEW 4103
#define IDM_PANEL_CLOSE 4104

BOOL DockHostPanelMenu_TogglePanelPinned(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode)
{
	if (!pDockHostWindow || !pPanelNode || !pPanelNode->data)
	{
		return FALSE;
	}

	DockData* pPanelData = (DockData*)pPanelNode->data;
	if (!DockPolicy_CanPinPanel(pPanelData->nRole, pPanelData->lpszName))
	{
		return FALSE;
	}

	TreeNode* pZoneNode = DockHostWindow_FindOwningZoneNode(pDockHostWindow, pPanelNode);
	if (!pZoneNode || !pZoneNode->data)
	{
		return FALSE;
	}

	DockData* pZoneData = (DockData*)pZoneNode->data;
	BOOL bCollapse = pPanelData->bCollapsed ? FALSE : TRUE;
	pPanelData->bCollapsed = bCollapse;

	if (pPanelData->hWnd && IsWindow(pPanelData->hWnd) && !bCollapse)
	{
		pZoneData->hWndActiveTab = pPanelData->hWnd;
	}

	if (pPanelData->hWnd && IsWindow(pPanelData->hWnd) &&
		bCollapse &&
		pZoneData->hWndActiveTab == pPanelData->hWnd)
	{
		TreeNode* visibleTabs[DOCK_ZONE_MAX_TABS] = { 0 };
		int nVisibleTabs = DockZone_GetPanelsByCollapsed(pZoneNode, visibleTabs, ARRAYSIZE(visibleTabs), FALSE);
		if (nVisibleTabs > 0 && visibleTabs[0] && visibleTabs[0]->data)
		{
			pZoneData->hWndActiveTab = ((DockData*)visibleTabs[0]->data)->hWnd;
		}
		else {
			pZoneData->hWndActiveTab = NULL;
		}
	}

	DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
	DockHostWindow_Rearrange(pDockHostWindow);
	return TRUE;
}

BOOL DockHostPanelMenu_MovePanelToNewWindow(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode)
{
	if (!pDockHostWindow || !pPanelNode || !pPanelNode->data)
	{
		return FALSE;
	}

	DockData* pPanelData = (DockData*)pPanelNode->data;
	HWND hWndPanel = pPanelData->hWnd;
	if (!hWndPanel || !IsWindow(hWndPanel))
	{
		return FALSE;
	}

	RECT rcPanelLocal = pPanelData->rc;
	RECT rcPanelScreen = rcPanelLocal;
	MapWindowPoints(Window_GetHWND((Window*)pDockHostWindow), HWND_DESKTOP, (POINT*)&rcPanelScreen, 2);

	DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
	if (pPanelData->nPaneKind == DOCK_PANE_TOOL)
	{
		if (!DockHostModelApply_RemoveToolWindow(pDockHostWindow, hWndPanel, TRUE))
		{
			return FALSE;
		}
	}
	else {
		DockHostWindow_Undock(pDockHostWindow, pPanelNode);
	}

	FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
	HWND hWndFloating = Window_CreateWindow((Window*)pFloatingWindowContainer, NULL);
	FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostWindow);
	FloatingWindowContainer_PinWindow(pFloatingWindowContainer, hWndPanel);

	int width = max(Win32_Rect_GetWidth(&rcPanelScreen), 260);
	int height = max(Win32_Rect_GetHeight(&rcPanelScreen), 220);
	int x = rcPanelScreen.left;
	int y = rcPanelScreen.top;
	SetWindowPos(hWndFloating, NULL, x, y, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);

	DockHostWindow_Rearrange(pDockHostWindow);
	return TRUE;
}

void DockHostPanelMenu_Show(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode, POINT ptScreen)
{
	if (!pDockHostWindow || !pPanelNode || !pPanelNode->data)
	{
		return;
	}

	DockData* pPanelData = (DockData*)pPanelNode->data;
	BOOL bCanPin = DockPolicy_CanPinPanel(pPanelData->nRole, pPanelData->lpszName);
	BOOL bCanUndock = DockPolicy_CanUndockPanel(pPanelData->nRole, pPanelData->lpszName);
	BOOL bCanClose = DockPolicy_CanClosePanel(pPanelData->nRole, pPanelData->lpszName);
	BOOL bAutoHide = pPanelData->bCollapsed ? TRUE : FALSE;

	HMENU hMenu = CreatePopupMenu();
	if (!hMenu)
	{
		return;
	}

	AppendMenu(hMenu, MF_STRING | (bAutoHide ? MF_ENABLED : MF_GRAYED), IDM_PANEL_DOCK, L"Doc&k");
	AppendMenu(hMenu, MF_STRING | ((bCanPin && !bAutoHide) ? MF_ENABLED : MF_GRAYED), IDM_PANEL_AUTOHIDE, L"&Auto Hide");
	AppendMenu(hMenu, MF_STRING | (bCanUndock ? MF_ENABLED : MF_GRAYED), IDM_PANEL_MOVETONEW, L"Move To &New Window");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | (bCanClose ? MF_ENABLED : MF_GRAYED), IDM_PANEL_CLOSE, L"&Close\tShift+Esc");

	UINT uCmd = (UINT)TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN,
		ptScreen.x, ptScreen.y, 0, Window_GetHWND((Window*)pDockHostWindow), NULL);
	DestroyMenu(hMenu);

	switch (uCmd)
	{
	case IDM_PANEL_DOCK:
		if (bAutoHide)
		{
			DockHostPanelMenu_TogglePanelPinned(pDockHostWindow, pPanelNode);
		}
		break;

	case IDM_PANEL_AUTOHIDE:
		if (bCanPin && !bAutoHide)
		{
			DockHostPanelMenu_TogglePanelPinned(pDockHostWindow, pPanelNode);
		}
		break;

	case IDM_PANEL_MOVETONEW:
		if (bCanUndock)
		{
			DockHostPanelMenu_MovePanelToNewWindow(pDockHostWindow, pPanelNode);
		}
		break;

	case IDM_PANEL_CLOSE:
		if (bCanClose)
		{
			DockHostWindow_DestroyInclusive(pDockHostWindow, pPanelNode);
		}
		break;
	}
}
