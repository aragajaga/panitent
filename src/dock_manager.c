#include "precomp.h"
#include "dock_system.h"
#include "util/list.h" // Assuming List_Create, List_Add, List_IndexOf, List_RemoveAt, List_Destroy

#include <assert.h>
#include <strsafe.h> // For StringCchCopy

// --- DockManager Implementation ---

DockManager* DockManager_Init() {
    DockManager* pMgr = (DockManager*)calloc(1, sizeof(DockManager));
    if (!pMgr) {
        return NULL;
    }

    pMgr->floatingWindows = List_Create();
    if (!pMgr->floatingWindows) {
        free(pMgr);
        return NULL;
    }

    pMgr->isDragging = FALSE;
    pMgr->draggedContent = NULL;
    pMgr->hDragOverlayWnd = NULL;
    pMgr->defaultSplitterWidth = DEFAULT_SPLITTER_WIDTH;

    // Consider getting font from a more central place if PanitentApp is available
    // For now, use system default or allow it to be set later
    pMgr->uiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);


    return pMgr;
}

void DockManager_SetMainDockSite(DockManager* pMgr, HWND hMainWndAsDockSite) {
    if (!pMgr || !hMainWndAsDockSite) {
        return;
    }

    if (pMgr->mainDockSite) {
        // TODO: Handle if a main dock site already exists.
        // Maybe destroy the old one or log an error.
        // For now, let's assume this is called once.
        free(pMgr->mainDockSite->allPanes); // Assuming List_Destroy handles NULL or is not needed if empty
        free(pMgr->mainDockSite->allContents);
        // If rootGroup and its children are dynamically allocated, they need deep destruction here.
        free(pMgr->mainDockSite);
    }

    DockSite* pSite = (DockSite*)calloc(1, sizeof(DockSite));
    if (!pSite) {
        // Error: Could not allocate main dock site
        return;
    }

    pSite->hWnd = hMainWndAsDockSite;
    pSite->rootGroup = NULL; // Will be created when first content is added or layout loaded
    pSite->allPanes = List_Create();
    pSite->allContents = List_Create();

    if (!pSite->allPanes || !pSite->allContents) {
        if (pSite->allPanes) List_Destroy(pSite->allPanes);
        if (pSite->allContents) List_Destroy(pSite->allContents);
        free(pSite);
        // Error: Could not allocate lists for dock site
        return;
    }

    // Initialize AutoHideAreas (rects will be set during layout)
    for (int i = 0; i < 4; ++i) {
        pSite->autoHideAreas[i].side = (AutoHideSide)i;
        pSite->autoHideAreas[i].hiddenTools = List_Create();
        pSite->autoHideAreas[i].isVisible = FALSE;
    }

    pMgr->mainDockSite = pSite;
}

void DockManager_Destroy(DockManager* pMgr) {
    if (!pMgr) {
        return;
    }

    // Destroy main dock site
    if (pMgr->mainDockSite) {
        // TODO: Deep clean of mainDockSite (groups, panes, contents, lists)
        List_Destroy(pMgr->mainDockSite->allPanes); // Assuming items themselves are freed elsewhere or not owned by list
        List_Destroy(pMgr->mainDockSite->allContents);
        for(int i=0; i<4; ++i) {
            List_Destroy(pMgr->mainDockSite->autoHideAreas[i].hiddenTools);
        }
        // Need to recursively free rootGroup and its children (panes/groups)
        // DockGroup_DestroyRecursive(pMgr->mainDockSite->rootGroup);
        free(pMgr->mainDockSite);
    }

    // Destroy floating windows
    if (pMgr->floatingWindows) {
        // TODO: Iterate and destroy each FloatingWindow and its associated DockSite
        // for (size_t i = 0; i < List_GetCount(pMgr->floatingWindows); ++i) {
        //     FloatingWindow* fw = (FloatingWindow*)List_GetAt(pMgr->floatingWindows, i);
        //     DockSite_Destroy(fw->dockSite); // Assuming DockSite_Destroy handles its internal cleanup
        //     DestroyWindow(fw->hFloatWnd);
        //     free(fw);
        // }
        List_Destroy(pMgr->floatingWindows);
    }

    free(pMgr);
}


// --- Content Management Implementation ---

DockContent* DockManager_CreateContent(DockManager* pMgr, HWND hContentWnd, const wchar_t* title, const wchar_t* id, PaneType contentType) {
    if (!pMgr || !hContentWnd) {
        return NULL;
    }

    DockContent* pContent = (DockContent*)calloc(1, sizeof(DockContent));
    if (!pContent) {
        return NULL;
    }

    pContent->hWnd = hContentWnd;
    StringCchCopy(pContent->title, MAX_PATH, title ? title : L"Untitled");
    StringCchCopy(pContent->id, MAX_PATH, id ? id : title); // Use title as ID if ID is null, ensure uniqueness later

    pContent->state = CONTENT_STATE_DOCKED; // Default state
    pContent->contentType = contentType;
    pContent->canFloat = TRUE;
    pContent->canAutoHide = (contentType == PANE_TYPE_TOOL); // Documents typically don't auto-hide
    pContent->canClose = TRUE;
    pContent->parentPane = NULL;
    pContent->userData = NULL;

    return pContent;
}

void DockManager_AddContent(DockManager* pMgr, DockContent* pContent, DockPane* pTargetPane, DockPosition position) {
    if (!pMgr || !pContent || !pTargetPane) { // For now, pTargetPane is mandatory
        // TODO: Handle cases where pTargetPane is NULL (e.g., add to a default location)
        if (pContent && !pTargetPane) {
             OutputDebugString(L"DockManager_AddContent: Target pane is NULL. Content not added.\n");
        }
        return;
    }

    // Check for type compatibility (simplified)
    if (pContent->contentType == PANE_TYPE_DOCUMENT && pTargetPane->type != PANE_TYPE_DOCUMENT) {
        OutputDebugString(L"DockManager_AddContent: Cannot add Document content to a Tool pane.\n");
        // In a real scenario, might try to find/create a document pane.
        return;
    }
    // Tools can often be added to document panes in some systems, but let's be strict for now or make it configurable.
    /*
    if (pContent->contentType == PANE_TYPE_TOOL && pTargetPane->type != PANE_TYPE_TOOL) {
        OutputDebugString(L"DockManager_AddContent: Cannot add Tool content to a Document pane (strict mode).\n");
        return;
    }
    */


    if (!pTargetPane->contents) {
        pTargetPane->contents = List_Create();
    }

    if (!pTargetPane->contents) {
         OutputDebugString(L"DockManager_AddContent: Failed to create contents list for target pane.\n");
        return; // Failed to create list
    }

    List_Add(pTargetPane->contents, pContent);
    pContent->parentPane = pTargetPane;
    pContent->state = CONTENT_STATE_DOCKED;
    pTargetPane->activeContentIndex = List_GetCount(pTargetPane->contents) - 1; // Make new content active

    // Add to global list in the site for easier management if not already there
    DockSite* site = NULL;
    if (pTargetPane->parentGroup && pTargetPane->parentGroup->parentGroup == NULL) { // Assuming root group's parent is NULL for main site
        site = pMgr->mainDockSite; // This logic to find the site is naive, needs improvement
    }
    // TODO: More robust way to find which site the pTargetPane belongs to (main or floating)

    if (site && List_IndexOf(site->allContents, pContent) == -1) {
        List_Add(site->allContents, pContent);
    }


    // TODO: Handle different DockPositions (splitting, etc.)
    // For now, only DOCK_POSITION_TABBED is implicitly handled by adding to pane's list.

    // Update tab control if it exists
    if (pTargetPane->hTabControl) {
        TCITEM tcItem = { 0 };
        tcItem.mask = TCIF_TEXT;
        tcItem.pszText = pContent->title;
        TabCtrl_InsertItem(pTargetPane->hTabControl, TabCtrl_GetItemCount(pTargetPane->hTabControl), &tcItem);
        TabCtrl_SetCurSel(pTargetPane->hTabControl, pTargetPane->activeContentIndex);
    }

    ShowWindow(pContent->hWnd, SW_SHOW); // Ensure child window is visible
    UpdateWindow(pContent->hWnd);
}

// --- Pane and Group Creation ---

DockPane* DockPane_Create(PaneType type, DockGroup* parentGroup) {
    DockPane* pPane = (DockPane*)calloc(1, sizeof(DockPane));
    if (!pPane) {
        return NULL;
    }

    pPane->type = type;
    pPane->contents = List_Create(); // Initialize contents list
    if (!pPane->contents) {
        free(pPane);
        return NULL;
    }
    pPane->activeContentIndex = -1;
    pPane->parentGroup = parentGroup;
    pPane->showTabs = TRUE; // Default
    pPane->showCaption = FALSE; // Default, caption might show if it's the only thing in a group or floating

    // hTabControl would be created when the pane is actually displayed and needs tabs.
    // rect will be set by layout logic.

    return pPane;
}

DockGroup* DockGroup_Create(DockGroup* parentGroup, GroupOrientation orientation) {
    DockGroup* pGroup = (DockGroup*)calloc(1, sizeof(DockGroup));
    if (!pGroup) {
        return NULL;
    }

    pGroup->parentGroup = parentGroup;
    pGroup->orientation = orientation;
    pGroup->splitRatio = 0.5f; // Default 50/50 split
    pGroup->splitterWidth = DEFAULT_SPLITTER_WIDTH;
    // children list might be used if a group can have more than 2 children.
    // For a binary split, child1 & child2 are enough.
    pGroup->child1 = NULL;
    pGroup->child2 = NULL;
    pGroup->isChild1Group = FALSE;
    pGroup->isChild2Group = FALSE;

    // rect will be set by layout logic.
    return pGroup;
}

// Forward declarations
LRESULT CALLBACK FloatingWindow_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
DockSite* DockSite_Create(HWND hWndOwner); // Helper to create and init a DockSite
void DockSite_Destroy(DockSite* pSite);   // Helper to clean up a DockSite

#define FLOATING_WINDOW_CLASS_NAME L"__FloatingDockSiteClass"

// --- Floating Window Management ---

BOOL RegisterFloatingWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = {0};
    if (!GetClassInfoExW(hInstance, FLOATING_WINDOW_CLASS_NAME, &wcex)) {
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcex.lpfnWndProc = FloatingWindow_WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(FloatingWindow*); // Store pointer to FloatingWindow struct
        wcex.hInstance = hInstance;
        wcex.hIcon = NULL; // TODO: Set an icon
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = FLOATING_WINDOW_CLASS_NAME;
        wcex.hIconSm = NULL; // TODO: Set an icon

        if (!RegisterClassExW(&wcex)) {
            return FALSE;
        }
    }
    return TRUE;
}

FloatingWindow* FloatingWindow_Create(DockManager* pMgr, DockContent* pContentToHost, RECT initialScreenRect) {
    if (!pMgr || !pContentToHost) return NULL;

    HINSTANCE hInstance = GetModuleHandle(NULL);
    RegisterFloatingWindowClass(hInstance);

    FloatingWindow* pFltWnd = (FloatingWindow*)calloc(1, sizeof(FloatingWindow));
    if (!pFltWnd) return NULL;

    // Create the OS window
    pFltWnd->hFloatWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW, // Makes it not appear in Alt-Tab, suitable for tool windows
        FLOATING_WINDOW_CLASS_NAME,
        pContentToHost->title, // Window title
        WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_VISIBLE,
        initialScreenRect.left, initialScreenRect.top,
        initialScreenRect.right - initialScreenRect.left,
        initialScreenRect.bottom - initialScreenRect.top,
        pMgr->mainDockSite->hWnd, // Owner window (main app window)
        NULL,
        hInstance,
        pFltWnd // Pass 'this' to WM_NCCREATE/WM_CREATE
    );

    if (!pFltWnd->hFloatWnd) {
        free(pFltWnd);
        return NULL;
    }

    // Associate FloatingWindow struct with HWND
    SetWindowLongPtr(pFltWnd->hFloatWnd, GWLP_USERDATA, (LONG_PTR)pFltWnd);


    // Create and initialize the DockSite for this floating window
    pFltWnd->dockSite = DockSite_Create(pFltWnd->hFloatWnd);
    if (!pFltWnd->dockSite) {
        DestroyWindow(pFltWnd->hFloatWnd);
        free(pFltWnd);
        return NULL;
    }
    // The DockSite's hWnd should be the floating window itself.
    // pFltWnd->dockSite->hWnd = pFltWnd->hFloatWnd; // Done in DockSite_Create

    StringCchCopy(pFltWnd->id, MAX_PATH, pContentToHost->id); // Or generate a unique ID for the float window itself

    // Add the content to this new floating site
    // This requires adapting AddContent or having a helper to set up initial pane/group
    DockPane* initialPane = DockPane_Create(pContentToHost->contentType, NULL); // No parent group initially for the pane
    if (!initialPane) { /* error handling */ DockSite_Destroy(pFltWnd->dockSite); DestroyWindow(pFltWnd->hFloatWnd); free(pFltWnd); return NULL; }

    DockGroup* rootGroup = DockGroup_Create(NULL, GROUP_ORIENTATION_HORIZONTAL); // Root group for the floating site
    if (!rootGroup) { /* error handling */ free(initialPane); DockSite_Destroy(pFltWnd->dockSite); DestroyWindow(pFltWnd->hFloatWnd); free(pFltWnd); return NULL; }

    rootGroup->child1 = initialPane;
    rootGroup->isChild1Group = FALSE;
    initialPane->parentGroup = rootGroup;

    pFltWnd->dockSite->rootGroup = rootGroup;
    List_Add(pFltWnd->dockSite->allPanes, initialPane);

    // Parent the actual content HWND to the floating window
    SetParent(pContentToHost->hWnd, pFltWnd->hFloatWnd);
    DockManager_AddContent(pMgr, pContentToHost, initialPane, DOCK_POSITION_TABBED);
    pContentToHost->state = CONTENT_STATE_FLOATING; // Update state

    List_Add(pMgr->floatingWindows, pFltWnd);
    DockManager_LayoutDockSite(pMgr, pFltWnd->dockSite); // Layout the new floating window
    ShowWindow(pFltWnd->hFloatWnd, SW_SHOW);
    UpdateWindow(pFltWnd->hFloatWnd);

    return pFltWnd;
}


DockSite* DockSite_Create(HWND hWndOwner) {
    DockSite* pSite = (DockSite*)calloc(1, sizeof(DockSite));
    if (!pSite) return NULL;

    pSite->hWnd = hWndOwner;
    pSite->allPanes = List_Create();
    pSite->allContents = List_Create();
    if (!pSite->allPanes || !pSite->allContents) {
        if(pSite->allPanes) List_Destroy(pSite->allPanes);
        if(pSite->allContents) List_Destroy(pSite->allContents);
        free(pSite);
        return NULL;
    }
    for (int i = 0; i < 4; ++i) { // Initialize AutoHideAreas
        pSite->autoHideAreas[i].side = (AutoHideSide)i;
        pSite->autoHideAreas[i].hiddenTools = List_Create();
    }
    return pSite;
}

void DockSite_Destroy(DockSite* pSite) {
    if (!pSite) return;
    // TODO: Full recursive destruction of groups, panes, contents within this site
    if (pSite->rootGroup) {
        DockGroup_DestroyRecursive(pSite->rootGroup); // Call recursive destruction
        pSite->rootGroup = NULL;
    }

    // Panes in allPanes are owned by groups, so they should be freed via group destruction.
    // We just destroy the list itself.
    if (pSite->allPanes) {
        List_Destroy(pSite->allPanes);
        pSite->allPanes = NULL;
    }

    // Contents in allContents are typically pointers to app-managed or DockManager-managed items.
    // Destroying this list doesn't free the DockContent structs themselves here.
    // True DockContent cleanup happens when DockManager_RemoveContent is fully implemented
    // or when the DockManager itself is destroyed.
    if (pSite->allContents) {
        List_Destroy(pSite->allContents);
        pSite->allContents = NULL;
    }

    for (int i = 0; i < 4; ++i) {
        if (pSite->autoHideAreas[i].hiddenTools) {
            // Similar to allContents, this list doesn't own the DockContent structs.
            List_Destroy(pSite->autoHideAreas[i].hiddenTools);
            pSite->autoHideAreas[i].hiddenTools = NULL;
        }
    }
    free(pSite);
}

void DockPane_Destroy(DockPane* pPane) {
    if (!pPane) return;

    if (pPane->hTabControl && IsWindow(pPane->hTabControl)) {
        DestroyWindow(pPane->hTabControl);
        pPane->hTabControl = NULL;
    }

    // The 'contents' list in DockPane stores DockContent pointers.
    // The DockContent structures themselves are typically managed globally by DockManager
    // or by the application. Destroying the pane should not free the DockContent structs
    // unless the pane explicitly owns them (which is not the current design).
    // The content HWNDs are reparented or destroyed by other logic (e.g. when content is closed/moved).
    if (pPane->contents) {
        List_Destroy(pPane->contents); // Just destroy the list container.
        pPane->contents = NULL;
    }
    free(pPane);
}

void DockGroup_DestroyRecursive(DockGroup* pGroup) {
    if (!pGroup) return;

    if (pGroup->child1) {
        if (pGroup->isChild1Group) {
            DockGroup_DestroyRecursive((DockGroup*)pGroup->child1);
        } else {
            DockPane_Destroy((DockPane*)pGroup->child1);
        }
        pGroup->child1 = NULL;
    }

    if (pGroup->child2) {
        if (pGroup->isChild2Group) {
            DockGroup_DestroyRecursive((DockGroup*)pGroup->child2);
        } else {
            DockPane_Destroy((DockPane*)pGroup->child2);
        }
        pGroup->child2 = NULL;
    }
    free(pGroup);
}


LRESULT CALLBACK FloatingWindow_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    FloatingWindow* pFltWnd = (FloatingWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    DockManager* pMgr = GetDockManager(); // Assuming GetDockManager() is accessible

    switch (message) {
        case WM_NCCREATE:
        {
            // Associate FloatingWindow struct with HWND early
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreate->lpCreateParams);
            return TRUE; // Continue creation
        }

        case WM_SIZE:
            if (pFltWnd && pFltWnd->dockSite && pMgr) {
                DockManager_LayoutDockSite(pMgr, pFltWnd->dockSite);
            }
            return 0;

        case WM_CLOSE:
            if (pFltWnd && pMgr) {
                // TODO: Decide behavior: hide, destroy, or try to re-dock content.
                // For now, just destroy the floating window and its site.
                // Content might be lost or need to be re-parented/re-docked.
                // This is a simplified close.
                List_Remove(pMgr->floatingWindows, pFltWnd); // Remove from manager's list
                DockSite_Destroy(pFltWnd->dockSite);
                pFltWnd->dockSite = NULL; // Avoid double free if DestroyWindow calls this again
                // Any content HWNDs that were parented to this floating window will be destroyed with it.
                // The DockContent structs themselves need to be handled (e.g. removed from global lists or freed).
                // For now, assume content HWNDs are destroyed.
                DestroyWindow(hWnd);
                free(pFltWnd);
            }
            return 0;

        case WM_DESTROY:
            // If pFltWnd is still valid, it means WM_CLOSE didn't fully clean it up,
            // or it's being destroyed for other reasons.
            // SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)NULL); // Disassociate
            break;

        // TODO: Add WM_NCLBUTTONDOWN for HTCAPTION to allow dragging the window.
        // TODO: Add WM_GETMINMAXINFO if needed.
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}


// --- Undocking and Floating ---

// Forward declaration for helper
static DockSite* GetSiteForPane(DockManager* pMgr, DockPane* pPane);

BOOL DockManager_RemoveContent(DockManager* pMgr, DockContent* pContentToRemove, BOOL bDestroyContentHwnd) {
    if (!pMgr || !pContentToRemove) return FALSE;

    DockPane* parentPane = pContentToRemove->parentPane;
    DockSite* parentSite = NULL;

    if (parentPane) {
        parentSite = GetSiteForPane(pMgr, parentPane);
        List_Remove(parentPane->contents, pContentToRemove);
        pContentToRemove->parentPane = NULL; // Unlink

        // Update active index and tab control
        if (List_GetCount(parentPane->contents) > 0) {
            if (parentPane->activeContentIndex >= (int)List_GetCount(parentPane->contents) ||
                parentPane->activeContentIndex == List_IndexOf(parentPane->contents, pContentToRemove)) { // If active was removed or index out of bounds
                parentPane->activeContentIndex = 0;
            }
        } else {
            parentPane->activeContentIndex = -1;
        }

        if (parentPane->hTabControl) { // Refresh tab control
            TabCtrl_DeleteAllItems(parentPane->hTabControl);
            for (size_t i = 0; i < List_GetCount(parentPane->contents); ++i) {
                DockContent* dc = (DockContent*)List_GetAt(parentPane->contents, i);
                TCITEMW tcItem = {0};
                tcItem.mask = TCIF_TEXT | TCIF_PARAM;
                tcItem.pszText = dc->title;
                tcItem.lParam = (LPARAM)dc;
                TabCtrl_InsertItem(parentPane->hTabControl, i, &tcItem);
            }
            if (parentPane->activeContentIndex != -1) {
                TabCtrl_SetCurSel(parentPane->hTabControl, parentPane->activeContentIndex);
            }
        }

        // TODO: Handle empty pane removal and group collapsing. This is complex.
        // For now, an empty pane will remain.
        // if (List_GetCount(parentPane->contents) == 0) {
        //     DockManager_RemovePane(pMgr, parentPane); // This function would handle group logic
        // }

    }

    // Remove from the global list of the relevant site
    // If parentSite is NULL, it might be content that was never fully added or already removed from a pane.
    // We should try to find it in *any* site's allContents list.
    BOOL removedFromGlobalList = FALSE;
    if (parentSite && List_IndexOf(parentSite->allContents, pContentToRemove) != -1) {
        List_Remove(parentSite->allContents, pContentToRemove);
        removedFromGlobalList = TRUE;
    } else { // Check all sites if not found in the presumed parentSite
        if (pMgr->mainDockSite && List_IndexOf(pMgr->mainDockSite->allContents, pContentToRemove) != -1) {
            List_Remove(pMgr->mainDockSite->allContents, pContentToRemove);
            parentSite = pMgr->mainDockSite; // Found its site
            removedFromGlobalList = TRUE;
        } else {
            for (size_t i = 0; i < List_GetCount(pMgr->floatingWindows); ++i) {
                FloatingWindow* fw = (FloatingWindow*)List_GetAt(pMgr->floatingWindows, i);
                if (fw->dockSite && List_IndexOf(fw->dockSite->allContents, pContentToRemove) != -1) {
                    List_Remove(fw->dockSite->allContents, pContentToRemove);
                    parentSite = fw->dockSite; // Found its site
                    removedFromGlobalList = TRUE;
                    break;
                }
            }
        }
    }

    if (bDestroyContentHwnd && pContentToRemove->hWnd && IsWindow(pContentToRemove->hWnd)) {
        DestroyWindow(pContentToRemove->hWnd);
        pContentToRemove->hWnd = NULL;
    }

    // The DockContent struct itself is not freed here. The application or a higher-level
    // content closing mechanism should decide when to free the DockContent struct.
    // This function primarily removes it from the layout.

    if (parentSite) { // Re-layout the site where content was removed from
        DockManager_LayoutDockSite(pMgr, parentSite);
    }

    return TRUE;
}


BOOL DockManager_UndockContent(DockManager* pMgr, DockContent* pContent) {
    if (!pMgr || !pContent || pContent->state != CONTENT_STATE_DOCKED || !pContent->parentPane) {
        return FALSE; // Not docked or no parent pane
    }

    DockPane* oldPane = pContent->parentPane;
    DockSite* siteOfOldPane = GetSiteForPane(pMgr, oldPane);

    List_Remove(oldPane->contents, pContent);
    pContent->parentPane = NULL;

    // Refresh original pane's tab control
    if (oldPane->hTabControl) {
        TabCtrl_DeleteAllItems(oldPane->hTabControl);
        for (size_t i = 0; i < List_GetCount(oldPane->contents); ++i) {
            DockContent* dc = (DockContent*)List_GetAt(oldPane->contents, i);
            TCITEMW tcItem = { 0 };
            tcItem.mask = TCIF_TEXT | TCIF_PARAM;
            tcItem.pszText = dc->title;
            tcItem.lParam = (LPARAM)dc;
            TabCtrl_InsertItem(oldPane->hTabControl, i, &tcItem);
        }
        if (List_GetCount(oldPane->contents) > 0) {
            oldPane->activeContentIndex = 0; // Default to first tab
            TabCtrl_SetCurSel(oldPane->hTabControl, oldPane->activeContentIndex);
        } else {
            oldPane->activeContentIndex = -1;
            // TODO: If pane is empty, it might need to be removed/collapsed.
        }
    }

    if (siteOfOldPane) {
         DockManager_LayoutDockSite(pMgr, siteOfOldPane);
    }
    // Content HWND is not destroyed, just unparented from layout system logic here.
    // Its actual SetParent(NULL) or to the floating window happens in FloatContent.
    return TRUE;
}

void DockManager_FloatContent(DockManager* pMgr, DockContent* pContentToFloat, RECT initialScreenRect) {
    if (!pMgr || !pContentToFloat) return;

    if (pContentToFloat->state == CONTENT_STATE_DOCKED) {
        if (!DockManager_UndockContent(pMgr, pContentToFloat)) {
            // Failed to undock, cannot float
            return;
        }
    } else if (pContentToFloat->state == CONTENT_STATE_FLOATING) {
        // Already floating, maybe just move/resize? For now, do nothing.
        return;
    }
    // TODO: Handle CONTENT_STATE_AUTO_HIDDEN

    FloatingWindow_Create(pMgr, pContentToFloat, initialScreenRect);
    // The FloatingWindow_Create function now handles changing state and layout.
}

// --- Layout Persistence ---

void DockManager_SetAppCreateContentCallback(DockManager* pMgr, AppCreateContentCallback callback, void* userContext) {
    if (!pMgr) return;
    pMgr->appCreateContentCallback = callback;
    pMgr->appCreateContentUserContext = userContext;
}

// Helper function prototypes for recursive saving (to be defined below or in a separate .c file)
static void SaveDockGroupRecursive(FILE* f, DockGroup* pGroup, int indentLevel);
static void SaveDockPane(FILE* f, DockPane* pPane, int indentLevel);
static void SaveDockContentInfo(FILE* f, DockContent* pContent, int indentLevel);

BOOL DockManager_SaveLayout(DockManager* pMgr, const wchar_t* filePath) {
    if (!pMgr || !filePath) return FALSE;

    FILE* f = NULL;
    errno_t err = _wfopen_s(&f, filePath, L"w, ccs=UTF-8"); // Open in UTF-8 text mode
    if (err != 0 || !f) {
        // OutputDebugString(L"Failed to open layout file for writing.\n");
        return FALSE;
    }

    fwprintf(f, L"DockLayout Version: 1.0\n");

    // Save Main DockSite
    if (pMgr->mainDockSite && pMgr->mainDockSite->rootGroup) {
        fwprintf(f, L"MainDockSite {\n");
        SaveDockGroupRecursive(f, pMgr->mainDockSite->rootGroup, 1);
        fwprintf(f, L"}\n");
    }

    // Save Floating Windows
    fwprintf(f, L"FloatingWindows: %zu {\n", List_GetCount(pMgr->floatingWindows));
    for (size_t i = 0; i < List_GetCount(pMgr->floatingWindows); ++i) {
        FloatingWindow* pFltWnd = (FloatingWindow*)List_GetAt(pMgr->floatingWindows, i);
        RECT rcFloatWnd;
        GetWindowRect(pFltWnd->hFloatWnd, &rcFloatWnd);

        fwprintf(f, L"\tFloatingWindow ID: %s Rect: %ld,%ld,%ld,%ld {\n",
                 pFltWnd->id, rcFloatWnd.left, rcFloatWnd.top, rcFloatWnd.right, rcFloatWnd.bottom);
        if (pFltWnd->dockSite && pFltWnd->dockSite->rootGroup) {
            SaveDockGroupRecursive(f, pFltWnd->dockSite->rootGroup, 2);
        }
        fwprintf(f, L"\t}\n");
    }
    fwprintf(f, L"}\n");

    // TODO: Save AutoHideArea contents

    fclose(f);
    // OutputDebugString(L"Layout saved.\n");
    return TRUE;
}

static void SaveIndent(FILE* f, int level) {
    for (int i = 0; i < level; ++i) fwprintf(f, L"\t");
}

static void SaveDockGroupRecursive(FILE* f, DockGroup* pGroup, int indentLevel) {
    if (!pGroup) return;
    SaveIndent(f, indentLevel);
    fwprintf(f, L"DockGroup Orientation: %s SplitRatio: %.2f {\n",
             pGroup->orientation == GROUP_ORIENTATION_HORIZONTAL ? L"Horizontal" : L"Vertical",
             pGroup->splitRatio);

    if (pGroup->child1) {
        SaveIndent(f, indentLevel + 1);
        fwprintf(f, L"Child1 {\n");
        if (pGroup->isChild1Group) {
            SaveDockGroupRecursive(f, (DockGroup*)pGroup->child1, indentLevel + 2);
        } else {
            SaveDockPane(f, (DockPane*)pGroup->child1, indentLevel + 2);
        }
        SaveIndent(f, indentLevel + 1);
        fwprintf(f, L"}\n");
    }

    if (pGroup->child2) {
        SaveIndent(f, indentLevel + 1);
        fwprintf(f, L"Child2 {\n");
        if (pGroup->isChild2Group) {
            SaveDockGroupRecursive(f, (DockGroup*)pGroup->child2, indentLevel + 2);
        } else {
            SaveDockPane(f, (DockPane*)pGroup->child2, indentLevel + 2);
        }
        SaveIndent(f, indentLevel + 1);
        fwprintf(f, L"}\n");
    }

    SaveIndent(f, indentLevel);
    fwprintf(f, L"}\n");
}

static void SaveDockPane(FILE* f, DockPane* pPane, int indentLevel) {
    if (!pPane) return;
    SaveIndent(f, indentLevel);
    fwprintf(f, L"DockPane Type: %s ActiveIndex: %d Contents: %zu {\n",
             pPane->type == PANE_TYPE_DOCUMENT ? L"Document" : L"Tool",
             pPane->activeContentIndex,
             List_GetCount(pPane->contents));

    for (size_t i = 0; i < List_GetCount(pPane->contents); ++i) {
        DockContent* pContent = (DockContent*)List_GetAt(pPane->contents, i);
        SaveDockContentInfo(f, pContent, indentLevel + 1);
    }

    SaveIndent(f, indentLevel);
    fwprintf(f, L"}\n");
}

static void SaveDockContentInfo(FILE* f, DockContent* pContent, int indentLevel) {
    if (!pContent) return;
    SaveIndent(f, indentLevel);
    // Basic info: ID and Title. Could add more like visible state if needed.
    fwprintf(f, L"DockContent ID: \"%s\" Title: \"%s\" Type: %s\n",
        pContent->id, pContent->title,
        pContent->contentType == PANE_TYPE_DOCUMENT ? L"Document" : L"Tool");
}


// Helper function prototypes for recursive loading
static DockGroup* LoadDockGroupRecursive(FILE* f, DockManager* pMgr, DockGroup* pParentGroup, int indentLevel);
static DockPane* LoadDockPane(FILE* f, DockManager* pMgr, DockGroup* pParentGroup, int indentLevel);
// static DockContent* LoadDockContentInfo(FILE* f, DockManager* pMgr, int indentLevel); // Content is loaded via callback

// Simplified line reading helper
static BOOL ReadLine(FILE* f, wchar_t* buffer, int bufferSize) {
    if (fgetws(buffer, bufferSize, f) == NULL) return FALSE;
    // Trim newline characters
    size_t len = wcslen(buffer);
    if (len > 0 && buffer[len - 1] == L'\n') buffer[len - 1] = L'\0';
    if (len > 1 && buffer[len - 2] == L'\r') buffer[len - 2] = L'\0';
    return TRUE;
}


BOOL DockManager_LoadLayout(DockManager* pMgr, const wchar_t* filePath) {
    if (!pMgr || !filePath || !pMgr->appCreateContentCallback) {
        // OutputDebugString(L"LoadLayout: Missing manager, path, or callback.\n");
        return FALSE;
    }

    FILE* f = NULL;
    errno_t err = _wfopen_s(&f, filePath, L"r, ccs=UTF-8");
    if (err != 0 || !f) {
        // OutputDebugString(L"Failed to open layout file for reading.\n");
        return FALSE;
    }

    wchar_t lineBuffer[1024];

    // TODO: Clear current layout (close all windows, destroy all sites/panes/groups)
    // For now, assuming a clean state or that the manager handles it.
    // A proper implementation would be:
    // DockManager_CloseAllContent(pMgr);
    // DockManager_DestroyAllFloatingWindows(pMgr);
    // if (pMgr->mainDockSite) { DockSite_Reset(pMgr->mainDockSite); }


    if (!ReadLine(f, lineBuffer, _countof(lineBuffer)) || wcsncmp(lineBuffer, L"DockLayout Version: 1.0", 24) != 0) {
        // OutputDebugString(L"Invalid layout file format or version.\n");
        fclose(f);
        return FALSE;
    }

    // Load Main DockSite
    if (ReadLine(f, lineBuffer, _countof(lineBuffer)) && wcscmp(lineBuffer, L"MainDockSite {") == 0) {
        pMgr->mainDockSite->rootGroup = LoadDockGroupRecursive(f, pMgr, NULL, 1);
        ReadLine(f, lineBuffer, _countof(lineBuffer)); // Read closing "}"
    }

    // Load Floating Windows
    if (ReadLine(f, lineBuffer, _countof(lineBuffer))) {
        size_t numFloating = 0;
        if (swscanf_s(lineBuffer, L"FloatingWindows: %zu {", &numFloating) == 1) {
            for (size_t i = 0; i < numFloating; ++i) {
                if (!ReadLine(f, lineBuffer, _countof(lineBuffer))) break; // Expected "\tFloatingWindow..."

                wchar_t floatId[MAX_PATH];
                RECT rcFloat = {0};
                if (swscanf_s(lineBuffer, L"\tFloatingWindow ID: %s Rect: %ld,%ld,%ld,%ld {",
                    floatId, _countof(floatId), &rcFloat.left, &rcFloat.top, &rcFloat.right, &rcFloat.bottom) == 6)
                {
                    // Create a placeholder content to initiate floating window creation
                    // The actual content will be loaded by LoadDockGroupRecursive
                    DockContent placeholderContent = {0};
                    StringCchCopy(placeholderContent.id, MAX_PATH, floatId);
                    StringCchCopy(placeholderContent.title, MAX_PATH, L"Floating"); // Temp title
                    placeholderContent.contentType = PANE_TYPE_TOOL; // Default, doesn't matter much for placeholder

                    FloatingWindow* pFltWnd = FloatingWindow_Create(pMgr, &placeholderContent, rcFloat);
                    // FloatingWindow_Create adds a default pane and group. We need to replace its rootGroup.
                    if (pFltWnd && pFltWnd->dockSite) {
                        // Clear the auto-generated simple layout in the new floating window
                        if (pFltWnd->dockSite->rootGroup) {
                            // TODO: Proper recursive free of the temp group/pane
                            // For now, assuming it's simple and can be overwritten
                            if(pFltWnd->dockSite->rootGroup->child1 && !pFltWnd->dockSite->rootGroup->isChild1Group) {
                                List_Remove(pFltWnd->dockSite->allPanes, pFltWnd->dockSite->rootGroup->child1);
                                free(pFltWnd->dockSite->rootGroup->child1);
                            }
                            free(pFltWnd->dockSite->rootGroup);
                        }
                        pFltWnd->dockSite->rootGroup = LoadDockGroupRecursive(f, pMgr, NULL, 2); // Load actual layout
                        // The content that was used as placeholder is now part of the loaded layout if its ID matched
                        // We need to remove the placeholder from its temporary pane if it wasn't loaded by ID
                        List_Remove(List_GetAt(pFltWnd->dockSite->allPanes, 0), &placeholderContent); // This is risky
                    }
                    ReadLine(f, lineBuffer, _countof(lineBuffer)); // Read closing "\t}"
                } else { ReadLine(f, lineBuffer, _countof(lineBuffer)); /* Consume closing brace if format error */ }
            }
            ReadLine(f, lineBuffer, _countof(lineBuffer)); // Read closing "}" for FloatingWindows
        }
    }

    // TODO: Load AutoHideArea contents

    fclose(f);

    // Final layout pass
    DockManager_LayoutDockSite(pMgr, pMgr->mainDockSite);
    for(size_t i=0; i < List_GetCount(pMgr->floatingWindows); ++i) {
        FloatingWindow* fw = (FloatingWindow*)List_GetAt(pMgr->floatingWindows, i);
        DockManager_LayoutDockSite(pMgr, fw->dockSite);
    }

    // OutputDebugString(L"Layout loaded.\n");
    return TRUE;
}


static DockGroup* LoadDockGroupRecursive(FILE* f, DockManager* pMgr, DockGroup* pParentGroup, int indentLevel) {
    wchar_t lineBuffer[1024];
    wchar_t orientationStr[20];
    float splitRatio;

    if (!ReadLine(f, lineBuffer, _countof(lineBuffer))) return NULL; // Expected DockGroup line

    // Example parse: "\tDockGroup Orientation: Horizontal SplitRatio: 0.50 {"
    wchar_t* token;
    wchar_t* context = NULL;
    wchar_t localLineBuffer[1024]; // Use a local copy for strtok
    StringCchCopy(localLineBuffer, _countof(localLineBuffer), lineBuffer);

    BOOL foundOrientation = FALSE;
    BOOL foundRatio = FALSE;

    token = wcstok_s(localLineBuffer, L" :{", &context); // Skip "DockGroup"
    while (token != NULL) {
        if (wcscmp(token, L"Orientation") == 0) {
            token = wcstok_s(NULL, L" :{,}", &context); // Get value
            if (token) {
                StringCchCopy(orientationStr, _countof(orientationStr), token);
                foundOrientation = TRUE;
            }
        } else if (wcscmp(token, L"SplitRatio") == 0) {
            token = wcstok_s(NULL, L" :{,}", &context); // Get value
            if (token) {
                splitRatio = (float)_wtof(token);
                foundRatio = TRUE;
            }
        }
        token = wcstok_s(NULL, L" :{,}", &context);
    }

    if (!foundOrientation || !foundRatio) {
        OutputDebugString(L"LoadDockGroupRecursive: Failed to parse DockGroup line for orientation/ratio.\n");
        OutputDebugString(lineBuffer); // Print the problematic line
        // Consume child lines until closing brace to attempt recovery for subsequent items
        int braceLevel = 1;
        while (ReadLine(f, lineBuffer, _countof(lineBuffer)) && braceLevel > 0) {
            if (wcsstr(lineBuffer, L"{")) braceLevel++;
            if (wcsstr(lineBuffer, L"}")) braceLevel--;
        }
        return NULL;
    }

    DockGroup* pGroup = DockGroup_Create(pParentGroup,
        (wcscmp(orientationStr, L"Horizontal") == 0) ? GROUP_ORIENTATION_HORIZONTAL : GROUP_ORIENTATION_VERTICAL);
    if (!pGroup) return NULL;
    pGroup->splitRatio = splitRatio;

    // Load Child1
    if (ReadLine(f, lineBuffer, _countof(lineBuffer)) && wcsstr(lineBuffer, L"Child1 {")) {
        if (ReadLine(f, lineBuffer, _countof(lineBuffer))) { // Read the actual type line
            if (wcsstr(lineBuffer, L"DockGroup")) {
                pGroup->child1 = LoadDockGroupRecursive(f, pMgr, pGroup, indentLevel + 2);
                pGroup->isChild1Group = TRUE;
            } else if (wcsstr(lineBuffer, L"DockPane")) {
                pGroup->child1 = LoadDockPane(f, pMgr, pGroup, indentLevel + 2);
                pGroup->isChild1Group = FALSE;
                if(pGroup->child1) List_Add(pMgr->mainDockSite->allPanes, pGroup->child1); // Add to global list (assuming main site for now)
            }
        }
        ReadLine(f, lineBuffer, _countof(lineBuffer)); // Read closing "}" for Child1
    }

    // Load Child2
    if (ReadLine(f, lineBuffer, _countof(lineBuffer)) && wcsstr(lineBuffer, L"Child2 {")) {
         if (ReadLine(f, lineBuffer, _countof(lineBuffer))) { // Read the actual type line
            if (wcsstr(lineBuffer, L"DockGroup")) {
                pGroup->child2 = LoadDockGroupRecursive(f, pMgr, pGroup, indentLevel + 2);
                pGroup->isChild2Group = TRUE;
            } else if (wcsstr(lineBuffer, L"DockPane")) {
                pGroup->child2 = LoadDockPane(f, pMgr, pGroup, indentLevel + 2);
                pGroup->isChild2Group = FALSE;
                 if(pGroup->child2) List_Add(pMgr->mainDockSite->allPanes, pGroup->child2); // Add to global list
            }
        }
        ReadLine(f, lineBuffer, _countof(lineBuffer)); // Read closing "}" for Child2
    }

    ReadLine(f, lineBuffer, _countof(lineBuffer)); // Read closing "}" for DockGroup
    return pGroup;
}

static DockPane* LoadDockPane(FILE* f, DockManager* pMgr, DockGroup* pParentGroup, int indentLevel) {
    wchar_t lineBuffer[1024];
    wchar_t typeStr[20];
    int activeIndex;
    size_t numContents;

    // The first line for DockPane was already read by LoadDockGroupRecursive to determine type.
    // The lineBuffer should contain the "DockPane Type:..." line, read by the caller (LoadDockGroupRecursive)
    wchar_t* token;
    wchar_t* context = NULL;
    wchar_t localLineBuffer[1024];
    StringCchCopy(localLineBuffer, _countof(localLineBuffer), lineBuffer);

    BOOL foundType = FALSE;
    BOOL foundActiveIndex = FALSE;
    BOOL foundNumContents = FALSE;

    token = wcstok_s(localLineBuffer, L" :{", &context); // Skip "DockPane"
    while (token != NULL) {
        if (wcscmp(token, L"Type") == 0) {
            token = wcstok_s(NULL, L" :{,}", &context);
            if (token) { StringCchCopy(typeStr, _countof(typeStr), token); foundType = TRUE; }
        } else if (wcscmp(token, L"ActiveIndex") == 0) {
            token = wcstok_s(NULL, L" :{,}", &context);
            if (token) { activeIndex = _wtoi(token); foundActiveIndex = TRUE; }
        } else if (wcscmp(token, L"Contents") == 0) {
            token = wcstok_s(NULL, L" :{,}", &context);
            if (token) { numContents = (size_t)_wtoi(token); foundNumContents = TRUE; }
        }
        token = wcstok_s(NULL, L" :{,}", &context);
    }

    if (!foundType || !foundActiveIndex || !foundNumContents) {
        OutputDebugString(L"LoadDockPane: Failed to parse DockPane line for type/activeIndex/numContents.\n");
        OutputDebugString(lineBuffer);
        // Consume child lines until closing brace
        int braceLevel = 1; // Already inside the Pane's {
        while (ReadLine(f, lineBuffer, _countof(lineBuffer)) && braceLevel > 0) {
            if (wcsstr(lineBuffer, L"{")) braceLevel++;
            if (wcsstr(lineBuffer, L"}")) braceLevel--;
        }
        return NULL;
    }

    DockPane* pPane = DockPane_Create((wcscmp(typeStr, L"Document") == 0) ? PANE_TYPE_DOCUMENT : PANE_TYPE_TOOL, pParentGroup);
    if (!pPane) return NULL;
    pPane->activeContentIndex = activeIndex;

    for (size_t i = 0; i < numContents; ++i) {
        if (!ReadLine(f, lineBuffer, _countof(lineBuffer))) break; // Expected DockContent line
        wchar_t contentId[MAX_PATH];
        wchar_t contentTitle[MAX_PATH]; // Title also saved for recreate if needed
        wchar_t contentTypeStr[20];

        // Example: "\t\t\tDockContent ID: "some_id" Title: "Some Title" Type: Document"
        if (swscanf_s(lineBuffer, L"%*[^I]ID: \"%[^\"]\" Title: \"%[^\"]\" Type: %s",
            contentId, _countof(contentId), contentTitle, _countof(contentTitle), contentTypeStr, _countof(contentTypeStr)) == 3)
        {
            PaneType contentType = (wcscmp(contentTypeStr, L"Document") == 0) ? PANE_TYPE_DOCUMENT : PANE_TYPE_TOOL;
            HWND hWndContent = pMgr->appCreateContentCallback(contentId, contentType, pMgr->appCreateContentUserContext);
            if (hWndContent) {
                DockContent* pContent = DockManager_CreateContent(pMgr, hWndContent, contentTitle, contentId, contentType);
                if (pContent) {
                    // Parent HWND to the correct site (main or floating)
                    // This needs to be determined based on pParentGroup's hierarchy
                    HWND hParentSiteWnd = pMgr->mainDockSite->hWnd; // Default
                    // TODO: Find correct site HWND if in floating window
                    SetParent(hWndContent, hParentSiteWnd);

                    DockManager_AddContent(pMgr, pContent, pPane, DOCK_POSITION_TABBED);
                } else { /* Log error, couldn't create DockContent struct */ }
            } else { /* Log error, app couldn't provide HWND for contentID */ }
        } else { /* Log parse error for DockContent line */ }
    }

    ReadLine(f, lineBuffer, _countof(lineBuffer)); // Read closing "}" for DockPane
    return pPane;
}


// TODO: Implement DockManager_RemoveContent, which will involve:
// - Removing content from its parent pane's list.
// - If pane becomes empty, potentially removing the pane from its parent group.
// - If group becomes empty or has only one child, collapsing/restructuring the group.
// - Updating tab controls.
// - Removing from global content list.
// - Potentially freeing the DockContent structure itself if it's not just being hidden.

// TODO: Implement layout functions (DockManager_LayoutDockSite, DockManager_UpdateContentWindowPositions)
// This will be the most complex part, recursively calculating rectangles for groups, panes, and content.
// And then calling SetWindowPos on the actual HWNDs.
// The version in dockhost.c is a very early placeholder.
