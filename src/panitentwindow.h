#pragma once

#include "dockhost.h"

typedef struct PanitentApp PanitentApp;
typedef struct MSTheme MSTheme;

typedef struct _PanitentWindow PanitentWindow;
struct _PanitentWindow
{
	Window base;

	DockHostWindow* m_pDockHostWindow;
	HWND m_hWndOptionBar;
	HWND m_hWndPalette;
	HWND m_hWndToolbox;
	TreeNode* m_viewportNode;
	BOOL fCallDWP;
	BOOL bCustomFrame;

	MSTheme* m_pMSTheme;
};

PanitentWindow* PanitentWindow_Create();
