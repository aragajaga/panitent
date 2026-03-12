#include "../precomp.h"

#include "activitysharingmanager.h"
#include "../win32/dialog.h"
#include "activitystubdialog.h"
#include "../resource.h"
#include "../panitentapp.h"

BOOL ActivityStubDialog_OnInitDialog(Dialog* pDialog);
void ActivityStubDialog_OnOK(Dialog* pDialog);
void ActivityStubDialog_OnCancel(Dialog* pDialog);
INT_PTR ActivityStubDialog_DlgUserProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam);
void ActivityStubDialog_SetStatusMessage(void* handler, PCWSTR pszStatusMessage);

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
    Dialog_Init(&pActivityStubDialog->base);

    pActivityStubDialog->base.DlgUserProc = ActivityStubDialog_DlgUserProc;
    pActivityStubDialog->base.OnInitDialog = ActivityStubDialog_OnInitDialog;
    pActivityStubDialog->base.OnOK = ActivityStubDialog_OnOK;
    pActivityStubDialog->base.OnCancel = ActivityStubDialog_OnCancel;

    pActivityStubDialog->m_activitySharingClient.m_handler = pActivityStubDialog;
    pActivityStubDialog->m_activitySharingClient.SetStatusMessage = ActivityStubDialog_SetStatusMessage;
}

BOOL ActivityStubDialog_OnInitDialog(Dialog* pDialog)
{
    ActivityStubDialog* pActivityStubDialog = (ActivityStubDialog*)pDialog;
    HWND hDlg = Window_GetHWND(&pActivityStubDialog->base.base);

    HWND hStatic = GetDlgItem(hDlg, IDS_ACTIVITYTEXT);
    PCWSTR pszCurrentStatus = ActivitySharingManager_GetStatus(PanitentApp_GetActivitySharingManager(PanitentApp_Instance()));
    SetWindowText(hStatic, pszCurrentStatus);
    return TRUE;
}

void ActivityStubDialog_OnOK(Dialog* pDialog)
{
    ActivityStubDialog* pActivityStubDialog = (ActivityStubDialog*)pDialog;
    EndDialog(Window_GetHWND(&pActivityStubDialog->base.base), 0);
}

void ActivityStubDialog_OnCancel(Dialog* pDialog)
{
    ActivityStubDialog* pActivityStubDialog = (ActivityStubDialog*)pDialog;
    EndDialog(Window_GetHWND(&pActivityStubDialog->base.base), 0);
}

INT_PTR ActivityStubDialog_DlgUserProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    }

    return Dialog_DefaultDialogProc(pDialog, message, wParam, lParam);
}

void ActivityStubDialog_SetStatusMessage(void* handler, PCWSTR pszStatusMessage)
{
    ActivityStubDialog* pActivityStubDialog = (ActivityStubDialog*)handler;
    if (!pActivityStubDialog)
    {
        return;
    }
    HWND hDlg = Window_GetHWND(&pActivityStubDialog->base.base);
    HWND hStatic = GetDlgItem(hDlg, IDS_ACTIVITYTEXT);
    SetWindowText(hStatic, pszStatusMessage);
}
