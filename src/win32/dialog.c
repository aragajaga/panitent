#include "common.h"

#include "dialog.h"
#include "windowmap.h"

extern HashMap g_hWndMap;

Dialog* Dialog_Create(Application* pApp);
void Dialog_Init(Dialog* pDialog, Application* pApp);
INT_PTR Dialog_DlgUserProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR Dialog_DefaultDialogProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Dialog_DialogProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR Dialog_CreateWindow(Dialog* pDialog, UINT uResourceId, HWND hWndParent, BOOL bModal);

Dialog* g_pCurrentDialog;

BOOL Dialog_OnCommand(Dialog* pDialog, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

void Dialog_OnInitDialog(Dialog* pDialog)
{
    return;
}

Dialog* Dialog_Create(Application* pApp)
{
    Dialog* pDialog = (Dialog*)malloc(sizeof(Dialog));
    
    if (pDialog)
    {
        memset(pDialog, 0, sizeof(Dialog));

        Dialog_Init(pDialog, pApp);
    }

    return pDialog;
}

void Dialog_Init(Dialog* pDialog, Application* pApp)
{
    Window_Init((Window *)pDialog, pApp);

    pDialog->base.OnCommand = Dialog_OnCommand;
    pDialog->DlgUserProc = Dialog_DlgUserProc;
    pDialog->OnInitDialog = Dialog_OnInitDialog;   
}

INT_PTR Dialog_DlgUserProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    return Dialog_DefaultDialogProc(pDialog, message, wParam, lParam);
}

INT_PTR Dialog_DefaultDialogProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CLOSE:
        ((Window*)pDialog)->OnClose((Window *)pDialog);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            pDialog->OnOK(pDialog);
            return TRUE;
        case IDCANCEL:
            pDialog->OnCancel(pDialog);
            return TRUE;
        default:
            return ((Window*)pDialog)->OnCommand((Window*)pDialog, wParam, lParam);
        }

    case WM_DESTROY:
        ((Window*)pDialog)->OnDestroy((Window*)pDialog);
        break;

    case WM_INITDIALOG:
        return pDialog->OnInitDialog(pDialog);
        break;

    case WM_PAINT:
        ((Window*)pDialog)->OnPaint((Window*)pDialog);
        break;

    case WM_SIZE:
        ((Window*)pDialog)->OnSize((Window*)pDialog, (UINT)wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK Dialog_DialogProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Dialog* pDialog = NULL;
    pDialog = (Dialog*)WindowMap_Get(hWnd);

    if (!pDialog)
    {
        pDialog = (Dialog*)g_pCurrentDialog;
        if (pDialog)
        {
            ((Window*)pDialog)->hWnd = hWnd;
            WindowMap_Insert(hWnd, pDialog);
        }
    }

    if (pDialog)
    {
        return pDialog->DlgUserProc(pDialog, message, wParam, lParam);
    }
}

INT_PTR Dialog_CreateWindow(Dialog* pDialog, UINT uResourceId, HWND hWndParent, BOOL bModal)
{
    g_pCurrentDialog = pDialog;

    if (bModal)
    {
        INT_PTR result = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(uResourceId), hWndParent, Dialog_DialogProcStatic, (LPARAM)pDialog);
        pDialog->base.hWnd = NULL;
        g_pCurrentDialog = NULL;

        return result;
    }
    else {
        HWND hWnd = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(uResourceId), hWndParent, Dialog_DialogProcStatic, (LPARAM)pDialog);
        g_pCurrentDialog = NULL;

        return (INT_PTR)hWnd;
    }
}
