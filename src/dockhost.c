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

void Dock_DestroyInclusive(TreeNode*, TreeNode*);
BOOL DockHostWindow_EnsureAutoHideOverlayHost(DockHostWindow* pDockHostWindow);

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
