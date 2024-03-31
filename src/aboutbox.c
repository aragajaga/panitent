#include "precomp.h"

#include "aboutbox.h"
#include "resource.h"

INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT message, WPARAM wParam,
    LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
#ifdef HAS_DISCORDSDK
        Discord_SetActivityStatus(g_panitent.discord, L"Seeking About dialog 👀");
#endif /* HAS_DISCORDSDK */
        return TRUE;
    case WM_COMMAND:
        EndDialog(hwndDlg, wParam);
        return TRUE;
    }

    return FALSE;
}

void AboutBox_Run(HWND hWndParent)
{
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTBOX), hWndParent, AboutDlgProc);
}
