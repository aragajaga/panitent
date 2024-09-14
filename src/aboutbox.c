#include "precomp.h"

#include "aboutbox.h"
#include "resource.h"

#include "panitentapp.h"
#include "sharing/activitysharingmanager.h"

INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        PanitentApp_SetActivityStatus(PanitentApp_Instance(), L"Seeking About dialog 👀");
    }
        return TRUE;
        break;

    case WM_COMMAND:
        EndDialog(hwndDlg, wParam);
        return TRUE;
        break;
    }

    return FALSE;
}

void AboutBox_Run(HWND hWndParent)
{
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTBOX), hWndParent, AboutDlgProc);
}
