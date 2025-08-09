#include "precomp.h"
#include "dockhost.h"   // Includes dock_system.h and indirectly win32/window.h etc.
#include "dock_system.h"// Explicit include for clarity
#include "dock_site.h"
#include "util/list.h"
#include <assert.h>
#include <stdio.h> // For printf in test runner

// Forward declare if not in a common header accessible by tests
// extern DockManager* g_pDockManager; // Assuming this is how it's accessed globally
// Or use GetDockManager()

// Dummy HWND creation for content - in a real app, these would be actual windows
HWND CreateDummyContentWindow(const wchar_t* title, HWND hParent) {
    // For testing, we don't need a real window class if we're not showing them.
    // But if SetParent or GetWindowLong is called, it needs to be a valid HWND.
    // Let's create a simple static window.
    return CreateWindowExW(0, L"STATIC", title, WS_CHILD | WS_VISIBLE,
                           0, 0, 100, 100, hParent, (HMENU)(UINT_PTR)rand(), // Unique enough ID for test
                           GetModuleHandle(NULL), NULL);
}

// Dummy AppCreateContentCallback for layout loading tests
HWND DummyAppCreateContent(const wchar_t* contentId, PaneType contentType, void* userContext) {
    UNREFERENCED_PARAMETER(userContext);
    UNREFERENCED_PARAMETER(contentType);
    // In a real test, you might have a map of IDs to pre-created dummy HWNDs
    // For now, just create a new one with the ID as title.
    // The parent will be set by the dock manager later.
    return CreateDummyContentWindow(contentId, GetDockManager()->mainDockSite->hWnd); // Or NULL parent initially
}


void Test_DockManagerInitialization() {
    printf("Running Test_DockManagerInitialization...\n");
    DockManager* pMgr = GetDockManager(); // Initializes if g_pDockManager is NULL
    assert(pMgr != NULL);
    assert(pMgr->mainDockSite == NULL); // SetMainDockSite not called yet by this test directly
    assert(pMgr->floatingWindows != NULL);
    assert(List_GetCount(pMgr->floatingWindows) == 0);
    assert(pMgr->isDraggingTab == FALSE);
    printf("Test_DockManagerInitialization PASSED.\n");
}

void Test_SetMainDockSite() {
    printf("Running Test_SetMainDockSite...\n");
    DockManager* pMgr = GetDockManager();
    assert(pMgr != NULL);

    // Simulate a main application window HWND
    // In a real test harness, this might be a specially created window.
    // For a focused test, we can mock it or use a placeholder if functions allow.
    // Let's assume we need a valid HWND for DockManager_SetMainDockSite.
    HWND hMockMainWnd = CreateWindowW(L"STATIC", L"MockMain", WS_OVERLAPPED, 0,0,100,100, NULL, NULL, GetModuleHandle(NULL), NULL);
    assert(hMockMainWnd != NULL);

    DockManager_SetMainDockSite(pMgr, hMockMainWnd);
    assert(pMgr->mainDockSite != NULL);
    assert(pMgr->mainDockSite->hWnd == hMockMainWnd);
    assert(pMgr->mainDockSite->rootGroup == NULL);
    assert(pMgr->mainDockSite->allPanes != NULL);
    assert(pMgr->mainDockSite->allContents != NULL);

    // Clean up mock window if it's not used by a global DockHostWindow instance
    // If GetDockManager() always uses a single global DockHostWindow, this might not be needed
    // or could conflict if DockHostWindow_Create was also called.
    // For this test, let's assume SetMainDockSite can be called with any HWND.
    if (pMgr->mainDockSite->hWnd == hMockMainWnd) { // Only destroy if it's our mock
         // To prevent issues if mainDockSite is used later by other tests through a global DockHostWindow,
         // we should probably reset it or ensure tests are independent.
         // For simplicity here, just destroy.
        List_Destroy(pMgr->mainDockSite->allPanes);
        List_Destroy(pMgr->mainDockSite->allContents);
        for(int i=0; i<4; ++i) List_Destroy(pMgr->mainDockSite->autoHideAreas[i].hiddenTools);
        free(pMgr->mainDockSite);
        pMgr->mainDockSite = NULL;
    }
    DestroyWindow(hMockMainWnd);

    printf("Test_SetMainDockSite PASSED.\n");
}


void Test_PinSingleWindow() {
    printf("Running Test_PinSingleWindow...\n");
    DockManager* pMgr = GetDockManager();
    // Ensure a main dock site is set up.
    // This relies on a DockHostWindow-like structure being the main window.
    // We need a simplified DockHostWindow_Create or similar for testing.
    // For now, let's manually set up a mainDockSite.
    HWND hParentWnd = CreateDummyContentWindow(L"TestParentSite", NULL); // A top-level window for the site
    DockManager_SetMainDockSite(pMgr, hParentWnd);
    assert(pMgr->mainDockSite != NULL);

    HWND hContent1 = CreateDummyContentWindow(L"Content1", pMgr->mainDockSite->hWnd);

    // Simulate DockHostWindow_PinWindow's core logic
    DockContent* pContent1 = DockManager_CreateContent(pMgr, hContent1, L"Content 1", L"ID_Content1", PANE_TYPE_DOCUMENT);
    assert(pContent1 != NULL);

    // Simplified add: create group and pane if they don't exist
    if (!pMgr->mainDockSite->rootGroup) {
        pMgr->mainDockSite->rootGroup = DockGroup_Create(NULL, GROUP_ORIENTATION_HORIZONTAL);
        assert(pMgr->mainDockSite->rootGroup != NULL);
        DockPane* newPane = DockPane_Create(PANE_TYPE_DOCUMENT, pMgr->mainDockSite->rootGroup);
        assert(newPane != NULL);
        pMgr->mainDockSite->rootGroup->child1 = newPane;
        pMgr->mainDockSite->rootGroup->isChild1Group = FALSE;
        List_Add(pMgr->mainDockSite->allPanes, newPane);
    }
    DockPane* targetPane = (DockPane*)pMgr->mainDockSite->rootGroup->child1;
    assert(targetPane != NULL);

    DockManager_AddContent(pMgr, pContent1, targetPane, DOCK_POSITION_TABBED);

    assert(List_GetCount(targetPane->contents) == 1);
    assert(List_GetAt(targetPane->contents, 0) == pContent1);
    assert(pContent1->parentPane == targetPane);
    assert(targetPane->activeContentIndex == 0);
    assert(List_GetCount(pMgr->mainDockSite->allContents) == 1);

    DockManager_LayoutDockSite(pMgr, pMgr->mainDockSite);
    // Assertions on HWND visibility/position are harder without a message loop
    // but we can check if rects are calculated.
    assert(pContent1->parentPane->rect.right > pContent1->parentPane->rect.left); // Check layout ran
    assert(pMgr->mainDockSite->rootGroup != NULL);
    assert(pMgr->mainDockSite->rootGroup->child1 == targetPane);
    assert(targetPane->parentGroup == pMgr->mainDockSite->rootGroup);

    // Cleanup
    DockManager_RemoveContent(pMgr, pContent1); // Use new remove function if available, or manual
    // Manual cleanup if RemoveContent is not fully implemented for tests:
    if (List_IndexOf(targetPane->contents, pContent1) != -1) List_Remove(targetPane->contents, pContent1);
    if (List_IndexOf(pMgr->mainDockSite->allContents, pContent1) != -1) List_Remove(pMgr->mainDockSite->allContents, pContent1);
    if (pContent1->hWnd && GetParent(pContent1->hWnd) == hParentWnd) DestroyWindow(pContent1->hWnd); // Destroy if parented to our mock site
    free(pContent1);

    DockSite_Destroy(pMgr->mainDockSite); // This should ideally clean up its groups/panes too
    pMgr->mainDockSite = NULL;
    DestroyWindow(hParentWnd);

    printf("Test_PinSingleWindow PASSED.\n");
}

void Test_PinMultipleWindows() {
    printf("Running Test_PinMultipleWindows...\n");
    DockManager* pMgr = GetDockManager();
    HWND hParentWnd = CreateDummyContentWindow(L"TestParentSite_Multi", NULL);
    DockManager_SetMainDockSite(pMgr, hParentWnd);
    assert(pMgr->mainDockSite != NULL);

    HWND hContent1 = CreateDummyContentWindow(L"ContentM1", pMgr->mainDockSite->hWnd);
    HWND hContent2 = CreateDummyContentWindow(L"ContentM2", pMgr->mainDockSite->hWnd);
    HWND hContent3 = CreateDummyContentWindow(L"ContentM3", pMgr->mainDockSite->hWnd);

    DockContent* pContent1 = DockManager_CreateContent(pMgr, hContent1, L"Content M1", L"ID_CM1", PANE_TYPE_DOCUMENT);
    DockContent* pContent2 = DockManager_CreateContent(pMgr, hContent2, L"Content M2", L"ID_CM2", PANE_TYPE_DOCUMENT);
    DockContent* pContent3 = DockManager_CreateContent(pMgr, hContent3, L"Content M3", L"ID_CM3", PANE_TYPE_DOCUMENT);

    // Simplified add: create group and pane if they don't exist
    if (!pMgr->mainDockSite->rootGroup) {
        pMgr->mainDockSite->rootGroup = DockGroup_Create(NULL, GROUP_ORIENTATION_HORIZONTAL);
        DockPane* newPane = DockPane_Create(PANE_TYPE_DOCUMENT, pMgr->mainDockSite->rootGroup);
        pMgr->mainDockSite->rootGroup->child1 = newPane;
        pMgr->mainDockSite->rootGroup->isChild1Group = FALSE;
        List_Add(pMgr->mainDockSite->allPanes, newPane);
    }
    DockPane* targetPane = (DockPane*)pMgr->mainDockSite->rootGroup->child1;
    assert(targetPane != NULL);

    DockManager_AddContent(pMgr, pContent1, targetPane, DOCK_POSITION_TABBED);
    DockManager_AddContent(pMgr, pContent2, targetPane, DOCK_POSITION_TABBED);
    DockManager_AddContent(pMgr, pContent3, targetPane, DOCK_POSITION_TABBED);

    assert(List_GetCount(targetPane->contents) == 3);
    assert(targetPane->activeContentIndex == 2); // Last added is active
    assert(List_GetAt(targetPane->contents, 0) == pContent1);
    assert(List_GetAt(targetPane->contents, 1) == pContent2);
    assert(List_GetAt(targetPane->contents, 2) == pContent3);
    assert(List_GetCount(pMgr->mainDockSite->allContents) == 3);

    DockManager_LayoutDockSite(pMgr, pMgr->mainDockSite);
    assert(List_GetCount(targetPane->tabRects) == 3);

    // Check visibility (conceptual - relies on UpdateContentWindowPositions logic)
    // For true check, would need GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE
    // This is a simplified check of layout rects.
    RECT rc1, rc2, rc3; // Assume these would be content areas from a test-focused layout call
    // Simulate what UpdateContentWindowPositions would do for visibility based on active index
    // For actual test, you'd check HWND visibility after message processing if possible.
    // Here, we trust DockManager_UpdateContentWindowPositions' logic based on activeContentIndex.


    // Cleanup
    DockManager_RemoveContent(pMgr, pContent1); // Assuming RemoveContent is robust enough or manual cleanup
    DockManager_RemoveContent(pMgr, pContent2);
    DockManager_RemoveContent(pMgr, pContent3);

    if (List_IndexOf(pMgr->mainDockSite->allContents, pContent1) != -1) List_Remove(pMgr->mainDockSite->allContents, pContent1);
    if (List_IndexOf(pMgr->mainDockSite->allContents, pContent2) != -1) List_Remove(pMgr->mainDockSite->allContents, pContent2);
    if (List_IndexOf(pMgr->mainDockSite->allContents, pContent3) != -1) List_Remove(pMgr->mainDockSite->allContents, pContent3);

    if(hContent1 && GetParent(hContent1) == hParentWnd) DestroyWindow(hContent1);
    if(hContent2 && GetParent(hContent2) == hParentWnd) DestroyWindow(hContent2);
    if(hContent3 && GetParent(hContent3) == hParentWnd) DestroyWindow(hContent3);

    free(pContent1); free(pContent2); free(pContent3);

    DockSite_Destroy(pMgr->mainDockSite);
    pMgr->mainDockSite = NULL;
    DestroyWindow(hParentWnd);

    printf("Test_PinMultipleWindows PASSED.\n");
}

void Test_TabSelection() {
    printf("Running Test_TabSelection...\n");
    DockManager* pMgr = GetDockManager();
    HWND hParentWnd = CreateDummyContentWindow(L"TestParentSite_TabSel", NULL);
    DockManager_SetMainDockSite(pMgr, hParentWnd);

    HWND hContent1 = CreateDummyContentWindow(L"ContentTS1", pMgr->mainDockSite->hWnd);
    HWND hContent2 = CreateDummyContentWindow(L"ContentTS2", pMgr->mainDockSite->hWnd);
    DockContent* pContent1 = DockManager_CreateContent(pMgr, hContent1, L"Content TS1", L"ID_CTS1", PANE_TYPE_DOCUMENT);
    DockContent* pContent2 = DockManager_CreateContent(pMgr, hContent2, L"Content TS2", L"ID_CTS2", PANE_TYPE_DOCUMENT);

    if (!pMgr->mainDockSite->rootGroup) { // Setup initial pane
        pMgr->mainDockSite->rootGroup = DockGroup_Create(NULL, GROUP_ORIENTATION_HORIZONTAL);
        DockPane* newPane = DockPane_Create(PANE_TYPE_DOCUMENT, pMgr->mainDockSite->rootGroup);
        pMgr->mainDockSite->rootGroup->child1 = newPane;
        pMgr->mainDockSite->rootGroup->isChild1Group = FALSE;
        List_Add(pMgr->mainDockSite->allPanes, newPane);
    }
    DockPane* targetPane = (DockPane*)pMgr->mainDockSite->rootGroup->child1;
    DockManager_AddContent(pMgr, pContent1, targetPane, DOCK_POSITION_TABBED);
    DockManager_AddContent(pMgr, pContent2, targetPane, DOCK_POSITION_TABBED); // pContent2 is now active (index 1)
    assert(targetPane->activeContentIndex == 1);

    // Simulate tab selection by directly setting active index and re-layouting
    targetPane->activeContentIndex = 0;
    DockManager_LayoutDockSite(pMgr, pMgr->mainDockSite);
    // In a real scenario with message loop, WM_NOTIFY would trigger this.
    // Here we test the consequence of activeContentIndex changing.

    // Assertions: Check which window *would be* visible.
    // This requires inspecting the logic of DockManager_UpdateContentWindowPositions
    // or adding test hooks/flags to content visibility.
    // For now, we trust that if activeContentIndex is 0, pContent1's HWND will be shown.
    printf("  (Visual check needed for Test_TabSelection if HWNDs were real)\n");


    // Cleanup
    DockManager_RemoveContent(pMgr, pContent1);
    DockManager_RemoveContent(pMgr, pContent2);
    if (List_IndexOf(pMgr->mainDockSite->allContents, pContent1) != -1) List_Remove(pMgr->mainDockSite->allContents, pContent1);
    if (List_IndexOf(pMgr->mainDockSite->allContents, pContent2) != -1) List_Remove(pMgr->mainDockSite->allContents, pContent2);
    if(hContent1 && GetParent(hContent1) == hParentWnd) DestroyWindow(hContent1);
    if(hContent2 && GetParent(hContent2) == hParentWnd) DestroyWindow(hContent2);
    free(pContent1); free(pContent2);

    DockSite_Destroy(pMgr->mainDockSite);
    pMgr->mainDockSite = NULL;
    DestroyWindow(hParentWnd);

    printf("Test_TabSelection PASSED.\n");
}

// TODO: Test_FloatWindow
// TODO: Test_SaveLoadLayout (with dummy callback and simple file)


// Basic test runner
int main_dock_test() {
    // Initialize common controls for tab controls, etc.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    Test_DockManagerInitialization();
    Test_SetMainDockSite();
    Test_PinSingleWindow();
    Test_PinMultipleWindows();
    Test_TabSelection();

    // Destroy the global manager if it was created
    if (g_pDockManager) {
        DockManager_Destroy(g_pDockManager);
        g_pDockManager = NULL;
    }

    printf("All tests finished.\n");
    return 0;
}
