#pragma once

#include "dockhost.h"

typedef struct PanitentApp PanitentApp;
typedef struct PanitentWindow PanitentWindow;
struct PanitentWindow
{
	Window base;

	DockHostWindow* m_pDockHostWindow;
	HWND m_hWndOptionBar;
	HWND m_hWndPalette;
	HWND m_hWndToolbox;
	HWND hWndMenuBar;
	HMENU hMainMenu;
	TreeNode* m_viewportNode;
	BOOL bCustomFrame;
	BOOL bCompactMenuBar;
	BOOL bNcTracking;
	BOOL bMenuTracking;
	BOOL bMenuPopupTracking;
	int nCaptionButtonHot;
	int nCaptionButtonPressed;
	int nHotMenuItem;
	int nOpenMenuItem;
	int nPendingMenuItem;
	void* pFileDropTarget;
};

PanitentWindow* PanitentWindow_Create();
void PanitentWindow_RefreshTheme(PanitentWindow* pPanitentWindow);
void PanitentWindow_SetUseStandardFrame(PanitentWindow* pPanitentWindow, BOOL fUseStandardFrame);
void PanitentWindow_SetCompactMenuBar(PanitentWindow* pPanitentWindow, BOOL fCompactMenuBar);
