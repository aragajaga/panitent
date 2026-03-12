#ifndef PANITENT_NEW_H_
#define PANITENT_NEW_H_

#include "precomp.h"

#define IDB_OK 1001

typedef struct PanitentWindow PanitentWindow;

void RegisterNewFileDialog();
BOOL DocumentSetupDialog_RegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK NewFileDialogWndProc(HWND hWnd,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam);
void NewFileDialog(PanitentWindow* pPanitentWindow);

#endif /* PANITENT_NEW_H_ */
