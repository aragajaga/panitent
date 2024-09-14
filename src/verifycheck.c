#include "verifycheck.h"

#include "util/assert.h"

#include "resource.h"

#include "win32/windowmap.h"

BOOL VerifyCheckDlg_OnInitDialog(VerifyCheckDlg* pVerifyCheckDlg);
void VerifyCheckDlg_OnOK(VerifyCheckDlg* pVerifyCheckDlg);
void VerifyCheckDlg_OnCancel(VerifyCheckDlg* pVerifyCheckDlg);
INT_PTR VerifyCheckDlg_UserProcDlg(VerifyCheckDlg* pVerifyCheckDlg, UINT message, WPARAM wParam, LPARAM lParam);

void VerifyCheckDlg_Init(VerifyCheckDlg* pVerifyCheckDlg)
{
    ASSERT(pVerifyCheckDlg);
    memset(pVerifyCheckDlg, 0, sizeof(VerifyCheckDlg));
    Dialog_Init((Dialog*)pVerifyCheckDlg);

    pVerifyCheckDlg->base.OnInitDialog = (FnDialogOnInitDialog)VerifyCheckDlg_OnInitDialog;
    pVerifyCheckDlg->base.OnOK = (FnDialogOnOK)VerifyCheckDlg_OnOK;
    pVerifyCheckDlg->base.OnCancel = (FnDialogOnCancel)VerifyCheckDlg_OnCancel;
    pVerifyCheckDlg->base.DlgUserProc = (FnDialogDlgUserProc)VerifyCheckDlg_UserProcDlg;
}


BOOL VerifyCheckDlg_OnInitDialog(VerifyCheckDlg* pVerifyCheckDlg)
{
    HWND hDlg = Window_GetHWND((Window*)pVerifyCheckDlg);

    HWND hCountryCombo = GetDlgItem(hDlg, IDC_PHONECOUNTRY);
    ComboBox_AddString(hCountryCombo, L"+7");

    return TRUE;
}

void VerifyCheckDlg_OnOK(VerifyCheckDlg* pVerifyCheckDlg)
{
    EndDialog(Window_GetHWND((Window*)pVerifyCheckDlg), 0);
    // Window_Destroy((Window*)pVerifyCheckDlg);
    WindowMap_Erase(Window_GetHWND((Window*)pVerifyCheckDlg));
}

void VerifyCheckDlg_OnCancel(VerifyCheckDlg* pVerifyCheckDlg)
{
    EndDialog(Window_GetHWND((Window*)pVerifyCheckDlg), 0);
    // Window_Destroy((Window*)pVerifyCheckDlg);
    WindowMap_Erase(Window_GetHWND((Window*)pVerifyCheckDlg));
}

INT_PTR VerifyCheckDlg_UserProcDlg(VerifyCheckDlg* pVerifyCheckDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    }

    return Dialog_DefaultDialogProc(pVerifyCheckDlg, message, wParam, lParam);
}
