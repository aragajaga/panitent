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
	TreeNode* m_viewportNode;
	BOOL bCustomFrame;
	BOOL bNcTracking;
	int nCaptionButtonHot;
	int nCaptionButtonPressed;
};

PanitentWindow* PanitentWindow_Create();
void PanitentWindow_SetUseStandardFrame(PanitentWindow* pPanitentWindow, BOOL fUseStandardFrame);
