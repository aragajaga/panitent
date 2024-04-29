#pragma once

#include "dockhost.h"

typedef struct PanitentWindow PanitentWindow;

struct PanitentWindow
{
	Window base;

	DockHostWindow* m_pDockHostWindow;
	HWND m_hWndOptionBar;
	HWND m_hWndPalette;
	HWND m_hWndToolbox;
	TreeNode* m_viewportNode;
	BOOL fCallDWP;
	BOOL bCustomFrame;
};

PanitentWindow* PanitentWindow_Create(struct Application*);
