#ifndef PANITENT_NEW_H_
#define PANITENT_NEW_H_

#include "precomp.h"

#define IDB_OK 1001

typedef struct _PanitentWindow PanitentWindow;

void NewFileDialog(HWND hwnd);
void RegisterNewFileDialog();
LRESULT CALLBACK NewFileDialogWndProc(HWND hWnd,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam);
void NewFileDialog(PanitentWindow* pPanitentWindow);

#endif /* PANITENT_NEW_H_ */
