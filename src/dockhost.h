#pragma once

#include "precomp.h"
#include "dock_system.h" // Include the new docking system definitions
#include "win32/window.h" // Ensure Window base class is known

// Forward declare PanitentApp if it's used in function signatures
typedef struct PanitentApp PanitentApp;
typedef struct DockInspectorDialog DockInspectorDialog; // Keep if still used

// DockHostWindow will now act as the main DockSite or be closely tied to it.
// The old tree structure (TreeNode, DockData) is being replaced by DockGroup, DockPane, DockContent.
struct DockHostWindow {
	Window base; // Inherits from a base Window class

	DockManager* dockManager; // Pointer to the global dock manager
	DockSite* dockSite;       // This window represents the main dock site

	// Keep UI elements if they are still relevant at this level
	HBRUSH hCaptionBrush_; // Example: if DockHostWindow still paints some global elements

	// Fields related to drag operations might move to DockManager or be handled via it
	// BOOL fCaptionDrag;
	// DockContent* m_pSubjectContent; // Instead of DockData*
	// POINT ptDragPos_;
	// BOOL fDrag_;

	DockInspectorDialog* m_pDockInspectorDialog; // If the inspector is still part of this window
};

// Global instance of the DockManager will likely replace g_pRoot.
// extern DockManager* g_pDockManager; // To be defined in a .c file

// --- Function Prototypes ---

// DockHostWindow now primarily manages the DockSite and interacts with DockManager
DockHostWindow* DockHostWindow_Create(PanitentApp* pApp); // pApp might be needed to get main HWND for DockManager

// The old PinWindow function will be replaced by DockManager_AddContent or similar.
// void DockHostWindow_PinWindow(DockHostWindow* pDockHostWindow, HWND hWndToPin, const wchar_t* title, PaneType contentType);

// Functions for creating nodes and data are now part of DockManager or helpers in dock_system.c
// DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
// TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);

// Rearrange and other layout logic will be driven by DockManager_LayoutDockSite
void DockHostWindow_OnSize(DockHostWindow* pDockHostWindow, UINT state, int cx, int cy); // Will likely trigger DockManager_LayoutDockSite

// Other event handlers (OnPaint, OnLButtonDown, etc.) will need to be adapted
// to use the new structures and delegate logic to DockManager where appropriate.
