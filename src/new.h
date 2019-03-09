#ifndef NEW_H
#define NEW_H

#include "stdafx.h"

#define IDB_OK 1001

void NewFileDialog(HWND hwnd);
void RegisterNewFileDialog();
LRESULT CALLBACK NewFileDialogWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void NewFileDialog(HWND hwnd);

#endif /* NEW_H */
