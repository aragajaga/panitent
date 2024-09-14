#include "../precomp.h"

#include "activitysharingmanager.h"
#include "../win32/dialog.h"
#include "activitystubdialog.h"
#include "../resource.h"

void ActivityStubDialog_OnInitDialog(ActivityStubDialog* pActivityStubDialog);
void ActivityStubDialog_OnOK(ActivityStubDialog* pActivityStubDialog);
void ActivityStubDialog_OnCancel(ActivityStubDialog* pActivityStubDialog);
INT_PTR ActivityStubDialog_DlgUserProc(ActivityStubDialog* pActivityStubDialog, UINT message, WPARAM wParam, LPARAM lParam);
void ActivityStubDialog_SetStatusMessage(ActivityStubDialog* pActivityStubDialog, PCWSTR pszStatusMessage);

ActivityStubDialog* ActivityStubDialog_Create()
{
    ActivityStubDialog* pActivityStubDialog = (ActivityStubDialog*)malloc(sizeof(ActivityStubDialog));

    if (pActivityStubDialog)
    {
        memset(pActivityStubDialog, 0, sizeof(ActivityStubDialog));
        ActivityStubDialog_Init(pActivityStubDialog);
    }

    return pActivityStubDialog;
}

void ActivityStubDialog_Init(ActivityStubDialog* pActivityStubDialog)
{
    Dialog_Init((Dialog*)&pActivityStubDialog->base);

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
    PCWSTR pszCurrentStatus = ActivitySharingManager_GetStatus(PanitentApp_GetActivitySharingManager(PanitentApp_Instance()));
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
