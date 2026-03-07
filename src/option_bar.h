#pragma once

#include "win32/window.h"

typedef struct Tool Tool;
typedef struct OptionBarWindow OptionBarWindow;

struct OptionBarWindow {
	Window base;
	HWND hWndStrokeCheck;
	HWND hWndFillCheck;
	HWND hWndAlgorithmCombo;
	HWND hWndThicknessCombo;
	HWND hWndBrushSelector;
	HWND hWndFontCombo;
	HWND hWndTextSizeCombo;
	int nMode;
	BOOL fShowFill;
};

BOOL BrushSel_RegisterClass(HINSTANCE hInstance);

OptionBarWindow* OptionBarWindow_Create();
void OptionBarWindow_SyncTool(OptionBarWindow* pOptionBarWindow, Tool* pTool);
