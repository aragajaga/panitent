#include "precomp.h"

#include "win32/propertygridimpl.h"
#include "propgriddialog.h"
#include "resource.h"

INT_PTR PropertyGridDialog_DlgUserProc(PropertyGridDialog* pPropertyGridDialog, UINT message, WPARAM wParam, LPARAM lParam);
void PropertyGridDialog_OnInitDialog(PropertyGridDialog* pPropertyGridDialog);
void PropertyGridDialog_OnOK(PropertyGridDialog* pPropertyGridDialog);
void PropertyGridDialog_OnCancel(PropertyGridDialog* pPropertyGridDialog);


PropertyGridDialog* PropertyGridDialog_Create(Application* pApp)
{
    PropertyGridDialog* pPropertyGridDialog = (PropertyGridDialog*)malloc(sizeof(PropertyGridDialog));

    if (pPropertyGridDialog)
    {
        memset(pPropertyGridDialog, 0, sizeof(PropertyGridDialog));

        PropertyGridDialog_Init(pPropertyGridDialog, pApp);
    }

    return pPropertyGridDialog;
}

void PropertyGridDialog_Init(PropertyGridDialog* pPropertyGridDialog, Application* pApp)
{
    Dialog_Init((Dialog*)&pPropertyGridDialog->base, pApp);

    pPropertyGridDialog->base.DlgUserProc = PropertyGridDialog_DlgUserProc;
    pPropertyGridDialog->base.OnInitDialog = PropertyGridDialog_OnInitDialog;
    pPropertyGridDialog->base.OnCancel = PropertyGridDialog_OnCancel;
    pPropertyGridDialog->base.OnOK = PropertyGridDialog_OnOK;

    pPropertyGridDialog->m_pPropertyGrid = PropertyGridCtl_Create(pApp);
}

INT_PTR PropertyGridDialog_DlgUserProc(PropertyGridDialog* pPropertyGridDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    }

    return Dialog_DefaultDialogProc(pPropertyGridDialog, message, wParam, lParam);
}

void PropertyGridDialog_OnInitDialog(PropertyGridDialog* pPropertyGridDialog)
{
    HWND hDlg = Window_GetHWND(pPropertyGridDialog);

    HWND hPropertyGrid = GetDlgItem(hDlg, IDC_PROPERTYGRID);
    Window_Attach(pPropertyGridDialog->m_pPropertyGrid, hPropertyGrid);

    PROPGRIDITEM pgi;
    PropGrid_ItemInit(pgi);
    
    WCHAR szCurValue[256] = L"";

    pgi.lpszCatalog = L"Edit, Static and combos";
    pgi.lpszPropName = L"Edit Field";
    pgi.lpCurValue = (LPARAM)szCurValue;
    pgi.lpszPropDesc = L"This field is editable";
    pgi.iItemType = PIT_EDIT;
    PropGrid_AddItem(hPropertyGrid, &pgi);

    pgi.lpszPropName = L"Static combo field";
    pgi.lpszPropDesc = L"Press F4 to view drop down.";
    pgi.iItemType = PIT_COMBO;
    PropGrid_AddItem(hPropertyGrid, &pgi);

    pgi.lpszCatalog = L"Check Boxes";
    pgi.lpszPropName = L"Save settings";
    pgi.iItemType = PIT_CHECK;
    PropGrid_AddItem(hPropertyGrid, &pgi);
}

void PropertyGridDialog_OnOK(PropertyGridDialog* pPropertyGridDialog)
{
    EndDialog(Window_GetHWND((Window*)pPropertyGridDialog), 0);
}

void PropertyGridDialog_OnCancel(PropertyGridDialog* pPropertyGridDialog)
{
    EndDialog(Window_GetHWND((Window*)pPropertyGridDialog), 0);
}
