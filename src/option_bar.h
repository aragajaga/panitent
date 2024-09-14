#pragma once

typedef struct OptionBarWindow OptionBarWindow;

struct OptionBarWindow {
	Window base;
	HWND textstring_handle;
};

BOOL BrushSel_RegisterClass(HINSTANCE hInstance);

OptionBarWindow* OptionBarWindow_Create();
