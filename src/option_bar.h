#pragma once

typedef struct OptionBarWindow OptionBarWindow;

struct OptionBarWindow {
	Window base;
	HWND textstring_handle;
	HWND hWndStrokeCheck;
	HWND hWndFillCheck;
	HWND hWndAlgorithmCombo;
	HWND hWndThicknessCombo;
	HWND hWndBrushSelector;
};

BOOL BrushSel_RegisterClass(HINSTANCE hInstance);

OptionBarWindow* OptionBarWindow_Create();
