#include "../precomp.h"

#include "../panitent.h"
#include "activitysharingmanager.h"
#include "../win32/dialog.h"
#include "activitystubdialog.h"
#include "../resource.h"

void ActivityStubDialog_OnInitDialog(ActivityStubDialog* pActivityStubDialog);
void ActivityStubDialog_OnOK(ActivityStubDialog* pActivityStubDialog);
void ActivityStubDialog_OnCancel(ActivityStubDialog* pActivityStubDialog);
INT_PTR ActivityStubDialog_DlgUserProc(ActivityStubDialog* pActivityStubDialog, UINT message, WPARAM wParam, LPARAM lParam);
void ActivityStubDialog_SetStatusMessage(ActivityStubDialog* pActivityStubDialog, PCWSTR pszStatusMessage);

ActivityStubDialog* ActivityStubDialog_Create(Application* pApp)
{
    ActivityStubDialog* pActivityStubDialog = (ActivityStubDialog*)malloc(sizeof(ActivityStubDialog));

    if (pActivityStubDialog)
    {
        memset(pActivityStubDialog, 0, sizeof(ActivityStubDialog));
        ActivityStubDialog_Init(pActivityStubDialog, pApp);
    }

    return pActivityStubDialog;
}

void ActivityStubDialog_Init(ActivityStubDialog* pActivityStubDialog, Application* pApp)
{
    Dialog_Init((Dialog*)&pActivityStubDialog->base, pApp);

    pActivityStubDialog->base.DlgUserProc = (FnDialogDlgUserProc)ActivityStubDialog_DlgUserProc;
    pActivityStubDialog->base.OnInitDialog = (FnDialogOnInitDialog)ActivityStubDialog_OnInitDialog;
    pActivityStubDialog->base.OnOK = (FnDialogOnOK)ActivityStubDialog_OnOK;
    pActivityStubDialog->base.OnCancel = (FnDialogOnCancel)ActivityStubDialog_OnCancel;

    pActivityStubDialog->m_activitySharingClient.m_handler = pActivityStubDialog;
    pActivityStubDialog->m_activitySharingClient.SetStatusMessage = ActivityStubDialog_SetStatusMessage;
}

void ActivityStubDialog_OnInitDialog(ActivityStubDialog* pActivityStubDialog)
{
    HWND hDlg = Window_GetHWND((Window*)pActivityStubDialog);

    HWND hStatic = GetDlgItem(hDlg, IDS_ACTIVITYTEXT);
    PCWSTR pszCurrentStatus = ActivitySharingManager_GetStatus(Panitent_GetApp()->m_pActivitySharingManager);
    SetWindowText(hStatic, pszCurrentStatus);
}

void ActivityStubDialog_OnOK(ActivityStubDialog* pActivityStubDialog)
{
    EndDialog(Window_GetHWND((Window*)pActivityStubDialog), 0);
}

void ActivityStubDialog_OnCancel(ActivityStubDialog* pActivityStubDialog)
{
    EndDialog(Window_GetHWND((Window*)pActivityStubDialog), 0);
}

INT_PTR ActivityStubDialog_DlgUserProc(ActivityStubDialog* pActivityStubDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    }

    return Dialog_DefaultDialogProc(pActivityStubDialog, message, wParam, lParam);
}

void ActivityStubDialog_SetStatusMessage(ActivityStubDialog* pActivityStubDialog, PCWSTR pszStatusMessage)
{
    HWND hDlg = Window_GetHWND((Window*)pActivityStubDialog);
    HWND hStatic = GetDlgItem(hDlg, IDS_ACTIVITYTEXT);
    SetWindowText(hStatic, pszStatusMessage);
}
