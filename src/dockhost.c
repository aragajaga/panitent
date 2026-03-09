#include "precomp.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "dockhost.h"
#include "dockhostdrag.h"
#include "dockhostinput.h"
#include "dockhostmetrics.h"
#include "dockhostmutate.h"
#include "dockhostruntime.h"
#include "dockhostzone.h"
#include "dockinspectordialog.h"
#include "resource.h"

#define IDM_DOCKINSPECTOR 101

void Dock_DestroyInclusive(TreeNode*, TreeNode*);
BOOL DockHostWindow_EnsureAutoHideOverlayHost(DockHostWindow* pDockHostWindow);

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam);
void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode);
void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode);
void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow);

void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
	DockHostMutate_DestroyInclusive(pDockHostWindow, pTargetNode);
}

void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
	DockHostMutate_Undock(pDockHostWindow, pTargetNode);
}

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDM_DOCKINSPECTOR:
		DockHostInput_InvokeInspectorDialog(pDockHostWindow);
		return TRUE;
	}

	return FALSE;
}

LRESULT DockHostWindow_UserProc(DockHostWindow* pDockHostWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ERASEBKGND:
		/* Full background is painted in OnPaint using a backbuffer. */
		return 1;
		break;

	case WM_MOUSEMOVE:
		DockHostInput_OnMouseMove(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam, DockHostMetrics_GetZoneTabGutter());
		return 0;
		break;

	case WM_MOUSELEAVE:
		DockHostInput_OnMouseLeave(pDockHostWindow);
		return 0;
		break;

	case WM_LBUTTONDOWN:
		DockHostInput_OnLButtonDown(pDockHostWindow, FALSE, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam, DockHostMetrics_GetZoneTabGutter());
		return 0;
		break;

	case WM_LBUTTONUP:
		DockHostInput_OnLButtonUp(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam);
		return 0;
		break;

	case WM_CAPTURECHANGED:
			DockHostInput_OnCaptureChanged(pDockHostWindow);
			return 0;
			break;

	case WM_CONTEXTMENU:
			DockHostInput_OnContextMenu(pDockHostWindow, (HWND)wParam, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam));
			return 0;
			break;
	}

	return Window_UserProcDefault((Window *)pDockHostWindow, hWnd, message, wParam, lParam);
}

int DockHostWindow_HitTestDockSide(DockHostWindow* pDockHostWindow, POINT ptScreen)
{
	return DockHostDrag_HitTestDockSide(pDockHostWindow, ptScreen);
}

BOOL DockHostWindow_HitTestDockTarget(DockHostWindow* pDockHostWindow, POINT ptScreen, DockTargetHit* pTargetHit)
{
	return DockHostDrag_HitTestDockTarget(pDockHostWindow, ptScreen, pTargetHit);
}

BOOL DockHostWindow_DockHWND(DockHostWindow* pDockHostWindow, HWND hWnd, int nDockSide, int iDockSize)
{
	return DockHostMutate_DockHWND(pDockHostWindow, hWnd, nDockSide, iDockSize);
}

BOOL DockHostWindow_DockHWNDToTarget(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize)
{
	return DockHostMutate_DockHWNDToTarget(pDockHostWindow, hWnd, pTargetHit, iDockSize);
}

BOOL DockHostWindow_DestroyDockedHWND(DockHostWindow* pDockHostWindow, HWND hWnd)
{
	if (!pDockHostWindow || !hWnd || !IsWindow(hWnd))
	{
		return FALSE;
	}

	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	if (!pRoot)
	{
		return FALSE;
	}

	TreeNode* pNode = DockNode_FindByHWND(pRoot, hWnd);
	if (!pNode)
	{
		return FALSE;
	}

	DockHostWindow_DestroyInclusive(pDockHostWindow, pNode);
	return TRUE;
}
