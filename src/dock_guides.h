#pragma once

#include "precomp.h"
#include "dock_system.h"

// --- Structures ---

typedef struct _DockGuide {
    HWND hWnd;
    DockDropArea area; // What drop area this guide represents (e.g., DOCK_DROP_AREA_LEFT)
    RECT rect;         // Position in screen coordinates
    BOOL isHot;        // Is the mouse hovering over this guide?
} DockGuide;

typedef struct _DockGuideCompass {
    DockGuide center;
    DockGuide left;
    DockGuide right;
    DockGuide top;
    DockGuide bottom;
    HBITMAP hBitmap;
} DockGuideCompass;

typedef struct _DockGuideManager {
    DockGuideCompass compass;
    DockGuide siteLeft;
    DockGuide siteRight;
    DockGuide siteTop;
    DockGuide siteBottom;
    BOOL isVisible;
} DockGuideManager;


// --- API ---

DockGuideManager* DockGuideManager_Create();
void DockGuideManager_Destroy(DockGuideManager* pMgr);

// Shows and positions the guides relative to a target pane or the main site
void DockGuideManager_Show(DockGuideManager* pMgr, DockPane* pTargetPane, DockSite* pTargetSite);
void DockGuideManager_Hide(DockGuideManager* pMgr);

// Updates which guide is "hot" based on the cursor position
void DockGuideManager_UpdateHighlight(DockGuideManager* pMgr, POINT screenPt);

// Returns the drop area the user is currently indicating with the mouse
DockDropArea DockGuideManager_GetDropTarget(DockGuideManager* pMgr);
