#ifndef _WIN32_DIALOG_H_INCLUDED
#define _WIN32_DIALOG_H_INCLUDED

#include "window.h"

typedef struct Dialog Dialog;

typedef INT_PTR(*FnDialogDlgUserProc)(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam);
typedef void (*FnDialogOnOK)(Dialog* pDialog);
typedef void (*FnDialogOnCancel)(Dialog* pDialog);
typedef BOOL (*FnDialogOnInitDialog)(Dialog* pDialog);

struct Dialog {
    Window base;

    FnDialogDlgUserProc DlgUserProc;
    FnDialogOnOK OnOK;
    FnDialogOnCancel OnCancel;
    FnDialogOnInitDialog OnInitDialog;
};

Dialog* Dialog_Create(Application* pApp);
void Dialog_Init(Dialog* pDialog, Application* pApp);
INT_PTR Dialog_DefaultDialogProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR Dialog_CreateWindow(Dialog* pDialog, UINT uResourceId, HWND hWndParent, BOOL bModal);

#endif  // _WIN32_DIALOG_H_INCLUDED
