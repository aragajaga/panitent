#ifndef PANITENT_NEW_H_
#define PANITENT_NEW_H_

#include "precomp.h"

#define IDB_OK 1001

void NewFileDialog(HWND hwnd);
void RegisterNewFileDialog();
LRESULT CALLBACK NewFileDialogWndProc(HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam);
void NewFileDialog(HWND hwnd);

#endif  /* PANITENT_NEW_H_ */
