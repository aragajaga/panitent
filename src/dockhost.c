#include "precomp.h"

#include "win32/window.h"
#include "win32/dialog.h" // Keep if DockInspectorDialog or other dialogs are used
#include "win32/util.h"   // For Win32_HexToCOLORREF etc.
#include "util/list.h"    // For List operations

#include <windowsx.h>
#include <commctrl.h> // Keep for common controls if used by pinned windows or UI elements
#include <strsafe.h>
#include <assert.h>

// #include "util/stack.h" // Old, likely not needed
// #include "util/tree.h"  // Old tree structure, replaced by DockGroup/DockPane

#include "dockhost.h"
#include "dock_system.h" // New docking system structures and APIs
#include "resource.h"    // For resource IDs like IDB_DOCKHOSTBG
#include "floatingwindowcontainer.h" // This will likely be refactored or replaced by DockManager's FloatingWindow
#include "dockinspectordialog.h"     // Keep if inspector dialog is retained
#include "toolwndframe.h" // May need to be adapted or its role absorbed

#include "panitentapp.h" // For PanitentApp_Instance, PanitentApp_GetUIFont

static const WCHAR szClassName[] = L"__DockHostWindow";

// Global instance of the DockManager
static DockManager* g_pDockManager = NULL;

DockManager* GetDockManager() {
	if (!g_pDockManager) {
		g_pDockManager = DockManager_Init();
	}
	return g_pDockManager;
}

// Forward declarations for functions to be implemented or adapted
BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam);
// void DockHostWindow_OnMouseMove(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags); // To be refactored
// void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow); // Will be replaced by DockManager_LayoutDockSite
void DockHostWindow_PaintContent(DockSite* pSite, HDC hdc, HBRUSH hCaptionBrush); // New paint helper


// --- Start of functions to be removed or heavily refactored ---
/*
POINT captionPos; // Old global
BOOL fSuggestTop;   // Old global
int iCaptionHeight = 24; // Old global, use DEFAULT_CAPTION_HEIGHT from dock_system.h
int iBorderWidth = 4;  // Old global, use DEFAULT_BORDER_WIDTH from dock_system.h

BOOL Dock_CaptionHitTest(DockData* pDockData, int x, int y); // Old
BOOL Dock_CloseButtonHitTest(DockData* pDockData, int x, int y); // Old
void Dock_DestroyInclusive(TreeNode*, TreeNode*); // Old
void DockNode_Paint(TreeNode*, HDC, HBRUSH); // Old


BOOL DockData_GetClientRect(DockData* pDockData, RECT* rc)
{
	if (!pDockData) { return FALSE; }
	RECT rcClient = pDockData->rc;
	if (pDockData->bShowCaption) { rcClient.top += iCaptionHeight + 1; }
	*rc = rcClient;
	return TRUE;
}

void DockData_Init(DockData* pDockData)
{
	pDockData->iGripPos = 64;
	pDockData->dwStyle = DGA_START | DGD_HORIZONTAL | DGP_ABSOLUTE;
	pDockData->bShowCaption = FALSE;
}

BOOL DockData_GetCaptionRect(DockData* pDockData, RECT* rc)
{
	if (!pDockData->bShowCaption) { return FALSE; }
	RECT rcCaption = pDockData->rc;
	rcCaption.top += iBorderWidth;
	rcCaption.left += iBorderWidth;
	rcCaption.right -= iBorderWidth;
	rcCaption.bottom = rcCaption.top + iBorderWidth + iCaptionHeight;
	*rc = rcCaption;
	return TRUE;
}

#define DHT_UNKNOWN 0 // Old
#define DHT_CAPTION 1 // Old
#define DHT_CLOSEBTN 2 // Old
#define WINDOWBUTTONSIZE 14 // Old

BOOL Dock_CaptionHitTest(DockData* pDockData, int x, int y) { ... } // Old
BOOL Dock_CloseButtonHitTest(DockData* pDockData, int x, int y) { ... } // Old

int DockHostWindow_HitTest(DockHostWindow* pDockHostWindow, TreeNode** ppTreeNode, int x, int y) // Old
{
    // ... old implementation based on pRoot_ and TreeNode ...
    return DHT_UNKNOWN;
}

void DockNode_Paint(TreeNode* pNodeParent, HDC hdc, HBRUSH hCaptionBrush) // Old
{
    // ... old implementation ...
}


void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow) // Old, replaced by DockManager_LayoutDockSite
{
    // ... old implementation ...
	// DockInspectorDialog_Update(pDockHostWindow->m_pDockInspectorDialog, pDockHostWindow->pRoot_); // Old
}

TreeNode* DockNode_FindParent(TreeNode* root, TreeNode* node) // Old
{
    // ... old implementation ...
	return NULL;
}

void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode) // Old
{
    // ... old implementation ...
	// DockHostWindow_Rearrange(pDockHostWindow); // Old
}

void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode) // Old
{
    // ... old implementation ...
	// DockHostWindow_Rearrange(pDockHostWindow); // Old
}
*/
// --- End of functions to be removed or heavily refactored ---


// Forward declaration
void DockManager_LayoutGroupRecursive(DockManager* pMgr, DockGroup* pGroup, RECT groupRect);
void DockManager_LayoutPane(DockManager* pMgr, DockPane* pPane, RECT paneRect);

// Main layout function for a DockSite
void DockManager_LayoutDockSite(DockManager* pMgr, DockSite* pSite) {
    if (!pMgr || !pSite) {
        return;
    }

    RECT rcClient;
    GetClientRect(pSite->hWnd, &rcClient);

    // TODO: Account for AutoHideAreas first, reducing rcClient accordingly.
    // For now, AutoHideAreas are not laid out.

    if (!pSite->rootGroup) {
        // No root group. If there are panes in allPanes (e.g. from a simplified PinWindow), lay out the first one.
        if (pSite->allPanes && List_GetCount(pSite->allPanes) > 0) {
            DockPane* firstPane = (DockPane*)List_GetAt(pSite->allPanes, 0);
            // This pane should ideally be part of a rootGroup. This indicates an incomplete setup.
            // However, to prevent crashes and show something, we'll lay it out.
            DockManager_LayoutPane(pMgr, firstPane, rcClient);
        } else {
            // No root group and no panes, nothing to lay out. Invalidate to draw background.
            InvalidateRect(pSite->hWnd, NULL, TRUE);
        }
    } else {
        // Start recursive layout from the root group
        DockManager_LayoutGroupRecursive(pMgr, pSite->rootGroup, rcClient);
    }

    // After all calculations, update the actual HWND positions
    DockManager_UpdateContentWindowPositions(pMgr, pSite);

    // Redraw the entire dock site to reflect changes, splitters, etc.
    InvalidateRect(pSite->hWnd, NULL, TRUE); // Use TRUE to erase background for full repaint
}

// Recursive function to lay out a DockGroup and its children
void DockManager_LayoutGroupRecursive(DockManager* pMgr, DockGroup* pGroup, RECT groupRect) {
    if (!pGroup) return;
    pGroup->rect = groupRect;

    if (IsRectEmpty(&groupRect)) {
        if(pGroup->child1) { // If group is empty, its children effectively are too
            if (pGroup->isChild1Group) DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child1, groupRect);
            else DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child1, groupRect);
        }
        if(pGroup->child2) {
            if (pGroup->isChild2Group) DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child2, groupRect);
            else DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child2, groupRect);
        }
        return;
    }

    if (!pGroup->child1) { // No children
        return;
    }

    if (!pGroup->child2) { // Only one child, it takes the whole groupRect
        if (pGroup->isChild1Group) {
            DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child1, groupRect);
        } else {
            DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child1, groupRect);
        }
        return;
    }

    // Two children, split the groupRect
    RECT rect1 = groupRect;
    RECT rect2 = groupRect;
    int splitterEffectiveWidth = pGroup->splitterWidth;

    if (pGroup->orientation == GROUP_ORIENTATION_HORIZONTAL) {
        int totalWidth = groupRect.right - groupRect.left;
        if (totalWidth <= splitterEffectiveWidth) {
             if (pGroup->isChild1Group) DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child1, groupRect); // Child1 gets all space
             else DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child1, groupRect);
             // Child2 gets no space
             RECT emptyRect = {0,0,0,0};
             if (pGroup->isChild2Group) DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child2, emptyRect);
             else DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child2, emptyRect);
            return;
        }
        int width1 = (int)((totalWidth - splitterEffectiveWidth) * pGroup->splitRatio);
        if (width1 < 0) width1 = 0;
        if (width1 > totalWidth - splitterEffectiveWidth) width1 = totalWidth - splitterEffectiveWidth;

        rect1.right = rect1.left + width1;
        rect2.left = rect1.right + splitterEffectiveWidth;
        // Ensure rect2 does not exceed groupRect boundaries due to rounding
        if(rect2.left > groupRect.right) rect2.left = groupRect.right;
        if(rect2.right < rect2.left) rect2.right = rect2.left; // Correct if width becomes negative

    } else { // GROUP_ORIENTATION_VERTICAL
        int totalHeight = groupRect.bottom - groupRect.top;
         if (totalHeight <= splitterEffectiveWidth) {
             if (pGroup->isChild1Group) DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child1, groupRect); // Child1 gets all space
             else DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child1, groupRect);
             // Child2 gets no space
             RECT emptyRect = {0,0,0,0};
             if (pGroup->isChild2Group) DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child2, emptyRect);
             else DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child2, emptyRect);
            return;
        }
        int height1 = (int)((totalHeight - splitterEffectiveWidth) * pGroup->splitRatio);
        if (height1 < 0) height1 = 0;
        if (height1 > totalHeight - splitterEffectiveWidth) height1 = totalHeight - splitterEffectiveWidth;

        rect1.bottom = rect1.top + height1;
        rect2.top = rect1.bottom + splitterEffectiveWidth;
        // Ensure rect2 does not exceed groupRect boundaries
        if(rect2.top > groupRect.bottom) rect2.top = groupRect.bottom;
        if(rect2.bottom < rect2.top) rect2.bottom = rect2.top; // Correct if height becomes negative
    }

    if (pGroup->isChild1Group) {
        DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child1, rect1);
    } else {
        DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child1, rect1);
    }

    if (pGroup->isChild2Group) {
        DockManager_LayoutGroupRecursive(pMgr, (DockGroup*)pGroup->child2, rect2);
    } else {
        DockManager_LayoutPane(pMgr, (DockPane*)pGroup->child2, rect2);
    }
}

// Function to lay out a DockPane and its contents
void DockManager_LayoutPane(DockManager* pMgr, DockPane* pPane, RECT paneRect) {
    if (!pPane) return;
    pPane->rect = paneRect;

    if (IsRectEmpty(&paneRect)) {
        if(pPane->hTabControl) ShowWindow(pPane->hTabControl, SW_HIDE);
        if(pPane->contents) { // Hide all content windows if pane is empty
            for(size_t i = 0; i < List_GetCount(pPane->contents); ++i) {
                DockContent* content = (DockContent*)List_GetAt(pPane->contents, i);
                if (content && content->hWnd) ShowWindow(content->hWnd, SW_HIDE);
            }
        }
        return;
    }

    BOOL tabsShouldBeVisible = pPane->showTabs && pPane->contents && List_GetCount(pPane->contents) > 0;

    if (pPane->hTabControl) { // If tab control exists, position or hide it
        if (tabsShouldBeVisible) {
            RECT rcTabCtrl = paneRect; // Tab control takes the top part of the paneRect
            rcTabCtrl.bottom = rcTabCtrl.top + DEFAULT_TAB_HEIGHT;
            if (rcTabCtrl.bottom > paneRect.bottom) rcTabCtrl.bottom = paneRect.bottom; // Clamp to pane bottom

            SetWindowPos(pPane->hTabControl, NULL, rcTabCtrl.left, rcTabCtrl.top,
                         rcTabCtrl.right - rcTabCtrl.left, rcTabCtrl.bottom - rcTabCtrl.top,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        } else {
            ShowWindow(pPane->hTabControl, SW_HIDE);
        }
    } else if (tabsShouldBeVisible) {
        pPane->hTabControl = CreateWindowEx(0, WC_TABCONTROLW, L"", // Use WC_TABCONTROLW for Unicode
                                         WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_FOCUSNEVER | TCS_TOOLTIPS,
                                         paneRect.left, paneRect.top, paneRect.right - paneRect.left, DEFAULT_TAB_HEIGHT,
                                         pMgr->mainDockSite->hWnd,
                                         (HMENU)(UINT_PTR)(pPane), // Using pane pointer as ID, ensure this is safe/unique enough
                                         GetModuleHandle(NULL), NULL);
        if (pPane->hTabControl) {
            SendMessage(pPane->hTabControl, WM_SETFONT, (WPARAM)pMgr->uiFont, TRUE);
            TabCtrl_DeleteAllItems(pPane->hTabControl); // Clear old tabs before repopulating
            for (size_t i = 0; i < List_GetCount(pPane->contents); ++i) {
                DockContent* dc = (DockContent*)List_GetAt(pPane->contents, i);
                TCITEMW tcItem = { 0 };
                tcItem.mask = TCIF_TEXT | TCIF_PARAM;
                tcItem.pszText = dc->title;
                tcItem.lParam = (LPARAM)dc;
                TabCtrl_InsertItem(pPane->hTabControl, i, &tcItem);
            }
            if (pPane->activeContentIndex >=0 && pPane->activeContentIndex < (int)List_GetCount(pPane->contents)) {
                 TabCtrl_SetCurSel(pPane->hTabControl, pPane->activeContentIndex);
            }
        }
    }
    // Actual content HWNDs are positioned in DockManager_UpdateContentWindowPositions
}

void DockManager_UpdateContentWindowPositions(DockManager* pMgr, DockSite* pSite) {
    if (!pMgr || !pSite || !pSite->allContents) return;

    for (size_t i = 0; i < List_GetCount(pSite->allContents); ++i) {
        DockContent* content = (DockContent*)List_GetAt(pSite->allContents, i);
        if (!content || !content->hWnd) continue;

        if (content->state == CONTENT_STATE_DOCKED && content->parentPane) {
            DockPane* pane = content->parentPane;
            if (IsRectEmpty(&pane->rect)) { // If parent pane has no size, hide content
                ShowWindow(content->hWnd, SW_HIDE);
                continue;
            }

            BOOL isActive = (List_IndexOf(pane->contents, content) == pane->activeContentIndex);

            if (isActive) {
                RECT contentAreaRect = pane->rect;
                BOOL tabsVisible = pane->showTabs && pane->contents && List_GetCount(pane->contents) > 0;

                if (tabsVisible) {
                    contentAreaRect.top += DEFAULT_TAB_HEIGHT;
                }
                if (pane->showCaption) {
                    contentAreaRect.top += DEFAULT_CAPTION_HEIGHT;
                }
                InflateRect(&contentAreaRect, -DEFAULT_BORDER_WIDTH, -DEFAULT_BORDER_WIDTH);

                if (contentAreaRect.right < contentAreaRect.left) contentAreaRect.right = contentAreaRect.left;
                if (contentAreaRect.bottom < contentAreaRect.top) contentAreaRect.bottom = contentAreaRect.top;

                if (!IsRectEmpty(&contentAreaRect)) {
                     SetWindowPos(content->hWnd, NULL,
                                 contentAreaRect.left, contentAreaRect.top,
                                 contentAreaRect.right - contentAreaRect.left,
                                 contentAreaRect.bottom - contentAreaRect.top,
                                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
                } else { // Not enough space for content
                    ShowWindow(content->hWnd, SW_HIDE);
                }
            } else {
                ShowWindow(content->hWnd, SW_HIDE);
            }
        } else if (content->state == CONTENT_STATE_FLOATING) {
            // Floating window positioning is handled by the FloatingWindow's own WM_SIZE/WM_MOVE.
        } else if (content->state == CONTENT_STATE_AUTO_HIDDEN) {
            ShowWindow(content->hWnd, SW_HIDE);
        }
    }
}


BOOL DockHostWindow_OnCreate(DockHostWindow* pDockHostWindow, LPCREATESTRUCT lpcs)
{
	UNREFERENCED_PARAMETER(lpcs);
	pDockHostWindow->hCaptionBrush_ = CreateSolidBrush(Win32_HexToCOLORREF(L"#9185be")); // Keep for now

    pDockHostWindow->dockManager = GetDockManager();
    // The DockHostWindow's HWND is the main dock site.
    DockManager_SetMainDockSite(pDockHostWindow->dockManager, Window_GetHWND((Window*)pDockHostWindow));
    pDockHostWindow->dockSite = pDockHostWindow->dockManager->mainDockSite; // Store direct pointer

	return TRUE;
}

void DockHostWindow_OnDestroy(DockHostWindow* pDockHostWindow)
{
	DeleteObject(pDockHostWindow->hCaptionBrush_);
    // Global DockManager is not destroyed here; handle on app exit.
}

#define DOCKHOSTBGMARGIN 16

void DockHostWindow_PaintContent(DockSite* pSite, HDC hdc, HBRUSH hCaptionBrush) {
    if (!pSite) return;

    if (pSite->rootGroup) {
        // TODO: Recursive paint for groups, splitters, panes, tabs, captions
        // For now, very basic: draw border around first pane if it exists
        if (pSite->rootGroup->child1 && !pSite->rootGroup->isChild1Group) {
            DockPane* pane = (DockPane*)pSite->rootGroup->child1;
            FrameRect(hdc, &pane->rect, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

            // Draw caption for active content in this pane
            if (pane->contents && List_GetCount(pane->contents) > 0 && pane->activeContentIndex != -1) {
                 DockContent* activeContent = (DockContent*)List_GetAt(pane->contents, pane->activeContentIndex);
                 if (activeContent) {
                    RECT rcCaptionArea = pane->rect; // Placeholder for caption area
                    if (pane->showTabs && List_GetCount(pane->contents) > 0) {
                        // Caption could be part of tab, or a separate bar
                        // This example assumes separate caption bar for non-tabbed or if pane->showCaption is true
                    }
                    // For simplicity, let's say if NO tabs, pane shows a caption for its active content
                    if (!(pane->showTabs && List_GetCount(pane->contents) > 0) || pane->showCaption) {
                        rcCaptionArea.bottom = rcCaptionArea.top + DEFAULT_CAPTION_HEIGHT;
                        FillRect(hdc, &rcCaptionArea, hCaptionBrush);
                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, RGB(0xff,0xff,0xff));
                        HFONT hOldFont = (HFONT)SelectObject(hdc, GetDockManager()->uiFont);
                        DrawText(hdc, activeContent->title, -1, &rcCaptionArea, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                        SelectObject(hdc, hOldFont);
                        // TODO: Draw close/pin buttons on this caption
                    }
                 }
            }
        }
    }
}

void DockHostWindow_OnPaint(DockHostWindow* pDockHostWindow)
{
	HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);
	PAINTSTRUCT ps = { 0 };
	HDC hdc = BeginPaint(hWnd, &ps);

	if (pDockHostWindow->dockSite && (pDockHostWindow->dockSite->rootGroup || List_GetCount(pDockHostWindow->dockSite->allContents) > 0) ) {
        DockHostWindow_PaintContent(pDockHostWindow->dockSite, hdc, pDockHostWindow->hCaptionBrush_);
	}
	else { // Draw background if no content or site not fully initialized
		HDC hdcLogo = CreateCompatibleDC(hdc);
		HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DOCKHOSTBG));
		BITMAP bm;
		GetObject(hBitmap, sizeof(BITMAP), &bm);
		HBITMAP hOldBitmap = SelectObject(hdcLogo, hBitmap);
		BLENDFUNCTION blendFunc = { AC_SRC_OVER, 0, 255, 0 }; // AC_SRC_ALPHA if bitmap has alpha
		RECT rcClient;
		Window_GetClientRect((Window*)pDockHostWindow, &rcClient);
		int x = (rcClient.right - bm.bmWidth) - DOCKHOSTBGMARGIN;
		int y = (rcClient.bottom - bm.bmHeight) - DOCKHOSTBGMARGIN;
		AlphaBlend(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcLogo, 0, 0, bm.bmWidth, bm.bmHeight, blendFunc);
		SelectObject(hdcLogo, hOldBitmap);
        DeleteObject(hBitmap);
		DeleteDC(hdcLogo);
	}
	EndPaint(hWnd, &ps);
}

void DockHostWindow_OnSize(DockHostWindow* pDockHostWindow, UINT state, int cx, int cy)
{
	UNREFERENCED_PARAMETER(state);
	UNREFERENCED_PARAMETER(cx);
	UNREFERENCED_PARAMETER(cy);

	if (pDockHostWindow->dockManager && pDockHostWindow->dockSite)
	{
		DockManager_LayoutDockSite(pDockHostWindow->dockManager, pDockHostWindow->dockSite);
	}
}

// HWND g_hWndDragOverlay; // Old global, move to DockManager if used for drag visuals

// --- Old Drag/HitTest related functions - To be refactored into DockManager ---
/*
void DockHostWindow_UndockToFloating(DockHostWindow* pDockHostWindow, TreeNode* pNode) { ... }
void DockHostWindow_OnMouseMove(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags) { ... }
LRESULT CALLBACK DragOverlayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { ... }
float smoothstepf(float edge0, float edge1, float x) { ... }
float clampf(float value, int min, int max) { ... }
void DockHostWindow_StartDrag(DockHostWindow* pDockHostWindow, int x, int y) { ... }
void DockHostWindow_OnLButtonDown(DockHostWindow* pDockHostWindow, BOOL fDoubleClick, int x, int y, UINT keyFlags) { ... }
void DockHostWindow_OnLButtonUp(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags) { ... }
void DockHostWindow_OnContextMenu(DockHostWindow* pDockHostWindow, HWND hWndContext, int x, int y) { ... }
*/
// --- End of Old Drag/HitTest related functions ---


void DockHostWindow_InvokeDockInspectorDialog(DockHostWindow* pDockHostWindow)
{
	HWND hWndDialog = Dialog_CreateWindow((Dialog*)pDockHostWindow->m_pDockInspectorDialog, IDD_DOCKINSPECTOR, Window_GetHWND((Window*)pDockHostWindow), FALSE);
	if (hWndDialog && IsWindow(hWndDialog))
	{
		ShowWindow(hWndDialog, SW_SHOW);
	}
}

#define IDM_DOCKINSPECTOR 101 // Should be in resource.h or similar

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
	switch (LOWORD(wParam))
	{
	case IDM_DOCKINSPECTOR:
		DockHostWindow_InvokeDockInspectorDialog(pDockHostWindow);
		break;
    // Handle other commands if necessary
	}
    return FALSE; // Return TRUE if command was handled
}

LRESULT DockHostWindow_UserProc(DockHostWindow* pDockHostWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // TODO: Refactor message handling to delegate to DockManager for relevant messages
    // (e.g., mouse clicks for drag, splitter interaction, tab clicks)
	DockManager* pMgr = pDockHostWindow->dockManager; // Get manager for easier access

	switch (message)
	{
    case WM_LBUTTONDOWN:
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            HWND hClickedWnd = WindowFromPoint(pt); // This gets the direct window, might be the tab control

            if (pMgr && pMgr->mainDockSite && pMgr->mainDockSite->allPanes) {
                for (size_t i = 0; i < List_GetCount(pMgr->mainDockSite->allPanes); ++i) {
                    DockPane* pane = (DockPane*)List_GetAt(pMgr->mainDockSite->allPanes, i);
                    if (pane->hTabControl && pane->hTabControl == hClickedWnd) {
                        TCHITTESTINFO htInfo = { 0 };
                        htInfo.pt = pt;
                        ScreenToClient(pane->hTabControl, &htInfo.pt); // Convert to tab control's client coords
                        int tabIdx = TabCtrl_HitTest(pane->hTabControl, &htInfo);

                        if (tabIdx != -1) { // Hit a tab
                            pMgr->isDraggingTab = TRUE;
                            pMgr->draggedTabPane = pane;
                            pMgr->draggedTabIndexOriginal = tabIdx;
                            ClientToScreen(hClickedWnd, &pt); // Use screen coords for drag start
                            pMgr->ptTabDragStart = pt;
                            SetCapture(hWnd); // Capture mouse for dragging
                            // OutputDebugString(L"Tab drag started.\n");
                            return 0;
                        }
                    }
                }
            }
        }
        break; // Fall through to default if not handled

    case WM_MOUSEMOVE:
        if (pMgr && pMgr->isDraggingTab) {
            // POINT ptCurrent = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            // ClientToScreen(hWnd, &ptCurrent); // Convert to screen
            // TODO: Implement visual feedback for tab dragging (e.g., ghost tab, insertion marker)
            // For now, just keeping it simple.
            // OutputDebugString(L"Tab dragging...\n");
            return 0;
        }
        break;

    case WM_LBUTTONUP:
        if (pMgr && pMgr->isDraggingTab) {
            ReleaseCapture();
            POINT ptDrop = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            HWND hTargetWnd = WindowFromPoint(ptDrop); // Screen coordinates by default from GetMessage
                                                      // If captured, lParam is client. Let's assume screen for now or convert.
            ClientToScreen(hWnd, &ptDrop); // Ensure ptDrop is in screen coordinates

            DockPane* targetPaneForDrop = NULL;
            int targetIndexForDrop = -1;

            // Check if dropped onto a tab control (could be the same or another)
            if (pMgr->mainDockSite && pMgr->mainDockSite->allPanes) {
                 for (size_t i = 0; i < List_GetCount(pMgr->mainDockSite->allPanes); ++i) {
                    DockPane* pane = (DockPane*)List_GetAt(pMgr->mainDockSite->allPanes, i);
                    if (pane->hTabControl && pane->hTabControl == WindowFromPoint(ptDrop) ) { // WindowFromPoint uses screen coords
                        targetPaneForDrop = pane;
                        TCHITTESTINFO htInfo = { 0 };
                        htInfo.pt = ptDrop;
                        ScreenToClient(pane->hTabControl, &htInfo.pt);
                        targetIndexForDrop = TabCtrl_HitTest(pane->hTabControl, &htInfo);
                        if (targetIndexForDrop == -1) { // Dropped on tab control but not on a specific tab, append.
                            targetIndexForDrop = TabCtrl_GetItemCount(pane->hTabControl);
                        }
                        break;
                    }
                }
            }

            // For now, only handle reordering within the SAME pane
            if (targetPaneForDrop == pMgr->draggedTabPane && targetIndexForDrop != -1) {
                DockPane* pane = pMgr->draggedTabPane;
                DockContent* contentToMove = (DockContent*)List_GetAt(pane->contents, pMgr->draggedTabIndexOriginal);

                if (contentToMove) {
                    List_RemoveAt(pane->contents, pMgr->draggedTabIndexOriginal);
                    if (targetIndexForDrop > pMgr->draggedTabIndexOriginal && targetIndexForDrop > 0) {
                        // If moving to a later position, the target index effectively reduces by 1 after removal
                        // targetIndexForDrop--; // This is not needed if we insert before the target index
                    }
                    // Ensure targetIndexForDrop is within bounds for insertion
                    if(targetIndexForDrop < 0) targetIndexForDrop = 0;
                    if(targetIndexForDrop > (int)List_GetCount(pane->contents)) targetIndexForDrop = List_GetCount(pane->contents);

                    List_InsertAt(pane->contents, contentToMove, targetIndexForDrop);

                    // Update active index if the moved tab was active or affected the active one
                    // If the active tab was moved, its new index is targetIndexForDrop
                    // If another tab was active and its index changed, update activeContentIndex
                    if (pane->activeContentIndex == pMgr->draggedTabIndexOriginal) {
                        pane->activeContentIndex = targetIndexForDrop;
                    } else if (pMgr->draggedTabIndexOriginal < pane->activeContentIndex && targetIndexForDrop >= pane->activeContentIndex) {
                        pane->activeContentIndex--;
                    } else if (pMgr->draggedTabIndexOriginal > pane->activeContentIndex && targetIndexForDrop <= pane->activeContentIndex) {
                        pane->activeContentIndex++;
                    }


                    // Refresh tab control
                    TabCtrl_DeleteAllItems(pane->hTabControl);
                    for (size_t i = 0; i < List_GetCount(pane->contents); ++i) {
                        DockContent* dc = (DockContent*)List_GetAt(pane->contents, i);
                        TCITEMW tcItem = { 0 };
                        tcItem.mask = TCIF_TEXT | TCIF_PARAM;
                        tcItem.pszText = dc->title;
                        tcItem.lParam = (LPARAM)dc;
                        TabCtrl_InsertItem(pane->hTabControl, i, &tcItem);
                    }
                    if(pane->activeContentIndex >=0 && pane->activeContentIndex < (int)List_GetCount(pane->contents)) {
                        TabCtrl_SetCurSel(pane->hTabControl, pane->activeContentIndex);
                    } else if (List_GetCount(pane->contents) > 0) {
                        pane->activeContentIndex = 0; // Default to first tab if active index is invalid
                        TabCtrl_SetCurSel(pane->hTabControl, pane->activeContentIndex);
                    } else {
                        pane->activeContentIndex = -1;
                    }

                    DockManager_LayoutDockSite(pMgr, pMgr->mainDockSite);
                    // OutputDebugString(L"Tab reordered.\n");
                }
            } else { // Dropped somewhere else (not on a tab in the original pane)
                // Check if it's a drop onto another existing pane (TODO later)
                // For now, if not on any tab control, consider it a float operation if moved enough
                if (targetPaneForDrop == NULL) { // Not dropped on any known tab control
                    DockContent* contentToFloat = (DockContent*)List_GetAt(pMgr->draggedTabPane->contents, pMgr->draggedTabIndexOriginal);
                    if (contentToFloat) {
                        POINT ptCurrent;
                        GetCursorPos(&ptCurrent); // Current cursor in screen coordinates

                        int dragThreshold = GetSystemMetrics(SM_CXDRAG) * 2; // Minimum drag distance
                        if (abs(ptCurrent.x - pMgr->ptTabDragStart.x) > dragThreshold ||
                            abs(ptCurrent.y - pMgr->ptTabDragStart.y) > dragThreshold) {

                            // Initial rect for the floating window
                            RECT floatRect = { ptCurrent.x - 150, ptCurrent.y - 20, ptCurrent.x + 150, ptCurrent.y + 200 };
                            // Ensure the original content HWND is made visible before floating if it was hidden (e.g. inactive tab)
                            // ShowWindow(contentToFloat->hWnd, SW_SHOW);

                            DockManager_FloatContent(pMgr, contentToFloat, floatRect);
                            // OutputDebugString(L"Tab floated.\n");
                        } else {
                            // OutputDebugString(L"Tab drag too short, no float.\n");
                        }
                    }
                } else {
                     // TODO: Dropped on a *different* pane's tab control. Implement docking to other panes.
                     // OutputDebugString(L"Tab dropped on another pane (not implemented yet).\n");
                }
            }

            pMgr->isDraggingTab = FALSE;
            pMgr->draggedTabPane = NULL;
            return 0;
        }
        break; // Fall through to default if not handled

    case WM_NOTIFY: // For tab control notifications
        if (pDockHostWindow->dockSite && pDockHostWindow->dockSite->allPanes) {
            LPNMHDR lpnmhdr = (LPNMHDR)lParam;
            // Check if it's from one of our pane's tab controls
            for(size_t i=0; i < List_GetCount(pDockHostWindow->dockSite->allPanes); ++i) {
                DockPane* pane = (DockPane*)List_GetAt(pDockHostWindow->dockSite->allPanes, i);
                if (pane->hTabControl && pane->hTabControl == lpnmhdr->hwndFrom) {
                    if (lpnmhdr->code == TCN_SELCHANGE) {
                        int sel = TabCtrl_GetCurSel(pane->hTabControl);
                        if (sel != -1 && sel < (int)List_GetCount(pane->contents)) {
                            pane->activeContentIndex = sel;
                            // DockContent* newActiveContent = (DockContent*)List_GetAt(pane->contents, sel);
                            // TODO: Potentially bring newActiveContent's HWND to top among siblings if needed,
                            // though ShowWindow/HideWindow in UpdateContentWindowPositions might be enough.
                            DockManager_LayoutDockSite(pDockHostWindow->dockManager, pDockHostWindow->dockSite); // Re-layout to show/hide windows
                        }
                    }
                    return 0; // Handled
                }
            }
        }
        break;
	}

	return Window_UserProcDefault((Window *)pDockHostWindow, hWnd, message, wParam, lParam);
}

void DockHostWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
	lpwcex->style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
	lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
	lpwcex->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Changed to COLOR_WINDOW for a more standard background
	lpwcex->lpszClassName = szClassName;
}

void DockHostWindow_PreCreate(LPCREATESTRUCT lpcs)
{
	lpcs->dwExStyle = WS_EX_CONTROLPARENT; // Important for tab navigation to child windows
	lpcs->lpszClass = szClassName;
	lpcs->lpszName = L"Panitent Dock Host"; // More descriptive name
	lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN; // WS_CLIPCHILDREN is important
	lpcs->x = CW_USEDEFAULT;
	lpcs->y = CW_USEDEFAULT;
	lpcs->cx = 800; // Default size
	lpcs->cy = 600; // Default size
}

void DockHostWindow_Init(DockHostWindow* pDockHostWindow, PanitentApp* pPanitentApp)
{
    UNREFERENCED_PARAMETER(pPanitentApp); // pPanitentApp might be used later for global app settings

	Window_Init(&pDockHostWindow->base);
	pDockHostWindow->base.szClassName = szClassName;
	pDockHostWindow->base.OnCreate = (FnWindowOnCreate)DockHostWindow_OnCreate;
	pDockHostWindow->base.OnDestroy = (FnWindowOnDestroy)DockHostWindow_OnDestroy;
	pDockHostWindow->base.OnPaint = (FnWindowOnPaint)DockHostWindow_OnPaint;
	pDockHostWindow->base.OnSize = (FnWindowOnSize)DockHostWindow_OnSize;
	pDockHostWindow->base.OnCommand = (FnWindowOnCommand)DockHostWindow_OnCommand;

	_WindowInitHelper_SetPreRegisterRoutine((Window *)pDockHostWindow, (FnWindowPreRegister)DockHostWindow_PreRegister);
	_WindowInitHelper_SetPreCreateRoutine((Window *)pDockHostWindow, (FnWindowPreCreate)DockHostWindow_PreCreate);
	_WindowInitHelper_SetUserProcRoutine((Window *)pDockHostWindow, (FnWindowUserProc)DockHostWindow_UserProc);

    // DockManager and DockSite are initialized in OnCreate after HWND is available.
	pDockHostWindow->dockManager = NULL;
	pDockHostWindow->dockSite = NULL;

	pDockHostWindow->m_pDockInspectorDialog = DockInspectorDialog_Create();
    // Ensure DockManager's UI font is set, e.g., in GetDockManager() or DockManager_Init()
    // GetDockManager()->uiFont = PanitentApp_GetUIFont(PanitentApp_Instance()); // Example
}

DockHostWindow* DockHostWindow_Create(PanitentApp* pPanitentApp)
{
	DockHostWindow* pDockHostWindow = (DockHostWindow*)calloc(1, sizeof(DockHostWindow)); // Use calloc for zero-init

	if (pDockHostWindow)
	{
		// memset(pDockHostWindow, 0, sizeof(DockHostWindow)); // calloc already zeros
		DockHostWindow_Init(pDockHostWindow, pPanitentApp);
	}
	return pDockHostWindow;
}

// TreeNode* DockHostWindow_SetRoot(...) // Old, replaced by DockManager interaction
// TreeNode* DockHostWindow_GetRoot(...) // Old

// New function to add a window to the docking system.
// Replaces DockData_PinWindow and old TreeNode logic.
void DockHostWindow_PinWindow(DockHostWindow* pDockHostWindow, HWND hWndToPin, const wchar_t* title, const wchar_t* id, PaneType contentType) {
    if (!pDockHostWindow || !pDockHostWindow->dockManager || !hWndToPin) {
        return;
    }

    DockManager* pMgr = pDockHostWindow->dockManager;
    DockContent* pContent = DockManager_CreateContent(pMgr, hWndToPin, title, id, contentType);
    if (!pContent) return;

    SetParent(hWndToPin, Window_GetHWND((Window*)pDockHostWindow)); // Parent to DockSite
    DWORD dwStyle = GetWindowLong(hWndToPin, GWL_STYLE);
    dwStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_POPUP | WS_SYSMENU); // Remove typical frame styles
    dwStyle |= WS_CHILD;
    SetWindowLong(hWndToPin, GWL_STYLE, dwStyle);
    // Ensure WS_CLIPSIBLINGS is on sibling windows if overlap is an issue.
    // WS_CLIPCHILDREN should be on the parent (DockHostWindow).

    // Add content to the docking system.
    // This is a simplified initial add. A real system might have more complex logic
    // for determining the target pane or creating splits.
    DockSite* mainSite = pMgr->mainDockSite;
    if (!mainSite) return; // Should not happen if OnCreate ran.

    DockPane* targetPane = NULL;

    // If no root group, create one with a single pane of the content's type
    if (!mainSite->rootGroup) {
        mainSite->rootGroup = DockGroup_Create(NULL, GROUP_ORIENTATION_HORIZONTAL); // Orientation irrelevant for single child
        if (!mainSite->rootGroup) { free(pContent); return; } // Failed to create group

        targetPane = DockPane_Create(contentType, mainSite->rootGroup);
        if (!targetPane) { free(mainSite->rootGroup); mainSite->rootGroup = NULL; free(pContent); return; }

        mainSite->rootGroup->child1 = targetPane;
        mainSite->rootGroup->isChild1Group = FALSE;
        List_Add(mainSite->allPanes, targetPane); // Add to site's list of panes
    } else {
        // Find an existing pane or decide where to put it.
        // Simplistic: find first pane of matching type, or first document pane for documents, or first tool pane for tools.
        // Or, always add to the first child of rootGroup if it's a pane.
        if (mainSite->rootGroup->child1 && !mainSite->rootGroup->isChild1Group) {
            DockPane* potentialPane = (DockPane*)mainSite->rootGroup->child1;
            // Basic type matching for now
            if ((contentType == PANE_TYPE_DOCUMENT && potentialPane->type == PANE_TYPE_DOCUMENT) ||
                (contentType == PANE_TYPE_TOOL && potentialPane->type == PANE_TYPE_TOOL) ||
                (contentType == PANE_TYPE_TOOL && potentialPane->type == PANE_TYPE_DOCUMENT) ) { // Allow tools in doc panes sometimes
                 targetPane = potentialPane;
            }
        }
        // If no suitable pane found in this simple check, we might need to create a new split or new pane.
        // For now, if targetPane is still NULL, we'll create a new pane and make the root a horizontal split.
        if (!targetPane) {
            DockPane* newPane = DockPane_Create(contentType, mainSite->rootGroup);
            if (!newPane) { free(pContent); return; }

            if (!mainSite->rootGroup->child1) { // Root group was empty, make newPane the first child
                 mainSite->rootGroup->child1 = newPane;
                 mainSite->rootGroup->isChild1Group = FALSE;
            } else { // Root group has one child, make it a split
                void* existingChild = mainSite->rootGroup->child1;
                BOOL existingIsGroup = mainSite->rootGroup->isChild1Group;

                DockGroup* newSplitGroup = DockGroup_Create(mainSite->rootGroup, GROUP_ORIENTATION_HORIZONTAL);
                if(!newSplitGroup) { free(newPane); free(pContent); return;}

                newSplitGroup->child1 = existingChild;
                newSplitGroup->isChild1Group = existingIsGroup;
                if(existingIsGroup) ((DockGroup*)existingChild)->parentGroup = newSplitGroup; else ((DockPane*)existingChild)->parentGroup = newSplitGroup;

                newSplitGroup->child2 = newPane;
                newSplitGroup->isChild2Group = FALSE;
                newPane->parentGroup = newSplitGroup;

                mainSite->rootGroup->child1 = newSplitGroup;
                mainSite->rootGroup->isChild1Group = TRUE;
            }
            targetPane = newPane;
            List_Add(mainSite->allPanes, targetPane);
        }
    }

    if (targetPane) {
        DockManager_AddContent(pMgr, pContent, targetPane, DOCK_POSITION_TABBED);
    } else {
        // Should not be reached if logic above is correct
        OutputDebugString(L"DockHostWindow_PinWindow: Failed to find or create a target pane.\n");
        free(pContent);
        return;
    }

    DockManager_LayoutDockSite(pMgr, mainSite);
    // ShowWindow(hWndToPin, SW_SHOWNA); // Show without activating, LayoutDockSite will handle visibility
    // UpdateWindow(hWndToPin);
}


// DockData* DockData_Create(...) // Old
// TreeNode* DockNode_Create(...) // Old
/*
DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption)
{
	DockData* pDockData = (DockData*)malloc(sizeof(DockData));
	if (pDockData)
	{
		memset(pDockData, 0, sizeof(DockData));
		pDockData->dwStyle = dwStyle;
		pDockData->iGripPos = iGripPos;
		pDockData->bShowCaption = bShowCaption;
		return pDockData;
	}
	return NULL;
}

TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption)
{
	TreeNode* pTreeNode = BinaryTree_AllocEmptyNode();
	DockData* pDockData = DockData_Create(iGripPos, dwStyle, bShowCaption);
	pTreeNode->data = pDockData;
	return pTreeNode;
}
*/
