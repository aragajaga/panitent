#pragma once

#include "precomp.h" // Assuming this includes necessary headers like windows.h
#include "util/list.h" // Assuming a generic list implementation exists

// Forward declarations
typedef struct _DockManager DockManager;
typedef struct _DockSite DockSite;
typedef struct _DockGroup DockGroup;
typedef struct _DockPane DockPane;
typedef struct _DockContent DockContent;
typedef struct _FloatingWindow FloatingWindow;
typedef struct _AutoHideArea AutoHideArea;

// Enums
typedef enum {
    CONTENT_STATE_DOCKED,
    CONTENT_STATE_FLOATING,
    CONTENT_STATE_AUTO_HIDDEN
} ContentState;

typedef enum {
    DOCK_POSITION_TABBED,
    DOCK_POSITION_LEFT,
    DOCK_POSITION_RIGHT,
    DOCK_POSITION_TOP,
    DOCK_POSITION_BOTTOM,
    DOCK_POSITION_FILL // Fill the parent group/site
} DockPosition;

typedef enum {
    AUTO_HIDE_SIDE_LEFT,
    AUTO_HIDE_SIDE_RIGHT,
    AUTO_HIDE_SIDE_TOP,
    AUTO_HIDE_SIDE_BOTTOM
} AutoHideSide;

typedef enum {
    PANE_TYPE_DOCUMENT,
    PANE_TYPE_TOOL
} PaneType;

typedef enum {
    GROUP_ORIENTATION_HORIZONTAL,
    GROUP_ORIENTATION_VERTICAL
} GroupOrientation;

// Callback for the app to provide/recreate content during layout load
typedef HWND(*AppCreateContentCallback)(const wchar_t* contentId, PaneType contentType, void* userContext);

// --- Structures ---

struct _DockContent {
    HWND hWnd;
    wchar_t title[MAX_PATH];
    wchar_t id[MAX_PATH]; // Unique ID for layout saving/loading
    ContentState state;
    PaneType contentType; // Simplified: is it a document or a tool?

    BOOL canFloat;
    BOOL canAutoHide;
    BOOL canClose;
    // Add more capabilities as needed

    DockPane* parentPane; // Back-pointer to its container
    void* userData;       // For application-specific data
};

struct _DockPane {
    PaneType type;
    List* contents; // List of DockContent* items (tabs)
    int activeContentIndex;
    HWND hTabControl; // HWND for the tab control UI, if applicable

    DockGroup* parentGroup; // Back-pointer
    RECT rect; // Current rectangle of this pane

    // Styling/behavior flags
    BOOL showTabs;
    BOOL showCaption; // If the pane itself has a caption when it's the only one in a group for example
    wchar_t caption[MAX_PATH]; // Pane caption if shown
};

struct _DockGroup {
    List* children; // List of DockPane* or DockGroup*
                    // For simplicity, let's assume a DockGroup always splits into two children for now,
                    // one DockPane/DockGroup and another DockPane/DockGroup, or it contains just one DockPane.
                    // A more flexible model might have a list of children and splitters.

    void* child1; // Can be DockPane* or DockGroup*
    void* child2; // Can be DockPane* or DockGroup*
    BOOL isChild1Group; // TRUE if child1 is DockGroup*, FALSE if DockPane*
    BOOL isChild2Group; // TRUE if child2 is DockGroup*, FALSE if DockPane*

    GroupOrientation orientation; // HORIZONTAL or VERTICAL split
    float splitRatio; // 0.0 to 1.0, percentage for the first child
    int splitterWidth; // Width of the splitter bar

    DockGroup* parentGroup; // Back-pointer (NULL if root group in a DockSite or FloatingWindow)
    RECT rect; // Current rectangle of this group
};

struct _AutoHideArea {
    AutoHideSide side;
    List* hiddenTools; // List of DockContent* (tools)
    HWND hTabStripWnd; // For displaying tabs of hidden tools
    BOOL isVisible;
    RECT rect;
};

struct _FloatingWindow {
    HWND hFloatWnd;      // The OS window for the floating container
    DockSite* dockSite;  // The content of a floating window is a mini-DockSite
    wchar_t id[MAX_PATH]; // Unique ID for layout persistence
};

struct _DockSite {
    HWND hWnd;              // The HWND this dock site is attached to (e.g., main window client area, or FloatingWindow client area)
    DockGroup* rootGroup;
    AutoHideArea autoHideAreas[4]; // Top, Bottom, Left, Right
    List* allPanes;         // Flat list of all panes for easier iteration
    List* allContents;      // Flat list of all contents for easier lookup
};

struct _DockManager {
    DockSite* mainDockSite;
    List* floatingWindows; // List of FloatingWindow*

    // Drag/Drop state
    BOOL isDragging;
    DockContent* draggedContent;
    HWND hDragOverlayWnd; // Window for showing drag previews/targets
    POINT ptDragStart;

    // Global settings
    int defaultSplitterWidth;
    HFONT uiFont;
    // Colors, etc.

    // Tab Drag State
    BOOL isDraggingTab;
    BOOL isFloatingTab; // Has the tab drag turned into a float operation?
    DockPane* draggedTabPane;   // The pane from which a tab is being dragged
    int draggedTabIndexOriginal; // Original index of the tab being dragged
    POINT ptTabDragStart;        // Screen coordinates of tab drag start

    // Splitter Drag State
    BOOL isDraggingSplitter;
    DockGroup* draggedGroup; // The group whose splitter is being dragged
    POINT ptSplitterDragStart; // Screen coordinates of splitter drag start

    FloatingWindow* draggedFloatingWindow; // The floating window being dragged after a tab float
    HWND hTabDragFeedbackWnd;   // Optional: A window for visual feedback during drag

    // Layout persistence
    AppCreateContentCallback appCreateContentCallback;
    void* appCreateContentUserContext;
};

// --- API Prototypes (Example) ---

// Manager Initialization
DockManager* DockManager_Init();
void DockManager_SetMainDockSite(DockManager* pMgr, HWND hMainWnd);
void DockManager_Destroy(DockManager* pMgr);

// Content Management
DockContent* DockManager_CreateContent(DockManager* pMgr, HWND hContentWnd, const wchar_t* title, const wchar_t* id, PaneType contentType);
void DockManager_AddContent(DockManager* pMgr, DockContent* pContent, DockPane* pTargetPane /*optional*/, DockPosition position /*optional*/);
BOOL DockManager_RemoveContent(DockManager* pMgr, DockContent* pContentToRemove, BOOL bDestroyContentHwnd); // This should also handle cleanup of empty panes/groups
// ... other content functions

// Layout and Operations
void DockManager_LayoutDockSite(DockManager* pMgr, DockSite* pSite);
void DockManager_UpdateContentWindowPositions(DockManager* pMgr, DockSite* pSite); // Iterates and calls SetWindowPos

// Utility functions for creating panes and groups could also be here or be internal
DockPane* DockPane_Create(PaneType type, DockGroup* parentGroup);
DockGroup* DockGroup_Create(DockGroup* parentGroup, GroupOrientation orientation);
void DockManager_RemovePane(DockManager* pMgr, DockPane* pPane);

// TODO: Add functions for list creation/destruction if not part of "util/list.h"
// e.g. List* List_Create(); void List_Add(List* pList, void* pItem); etc.

// Helper to get the manager instance (if it's a singleton)
DockManager* GetDockManager();
DockSite* GetSiteForPane(DockManager* pMgr, DockPane* pPane);

// Layout Serialization
BOOL DockManager_SaveLayout(DockManager* pMgr, const wchar_t* filePath);
BOOL DockManager_LoadLayout(DockManager* pMgr, const wchar_t* filePath);
void DockManager_SetAppCreateContentCallback(DockManager* pMgr, AppCreateContentCallback callback, void* userContext);

// --- Drag & Drop / Hit Testing ---

typedef enum {
	DOCK_DROP_AREA_NONE,
	DOCK_DROP_AREA_TAB_STRIP,
	DOCK_DROP_AREA_CENTER,
	DOCK_DROP_AREA_LEFT,
	DOCK_DROP_AREA_RIGHT,
	DOCK_DROP_AREA_TOP,
	DOCK_DROP_AREA_BOTTOM
} DockDropArea;

typedef struct _DockDropTarget {
	DockPane* pane;       // The pane that is the target
	int tabIndex;         // Index for tab strip drop, -1 otherwise
	DockDropArea area;    // The specific area within the pane
	RECT feedbackRect;    // The rectangle for visual feedback
} DockDropTarget;

DockDropTarget DockManager_HitTest(DockManager* pMgr, POINT screenPt);
DockGroup* DockManager_HitTestSplitter(DockManager* pMgr, POINT screenPt);
DockPane* DockGroup_GetFirstPane(DockGroup* pGroup);


#define DEFAULT_SPLITTER_WIDTH 5
#define DEFAULT_CAPTION_HEIGHT 24
#define DEFAULT_BORDER_WIDTH 4
#define DEFAULT_TAB_HEIGHT 20
