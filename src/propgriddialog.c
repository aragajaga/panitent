#include "precomp.h"

#include "win32/propertygridimpl.h"
#include "propgriddialog.h"
#include "resource.h"

INT_PTR PropertyGridDialog_DlgUserProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam);
BOOL PropertyGridDialog_OnInitDialog(Dialog* pDialog);
void PropertyGridDialog_OnOK(Dialog* pDialog);
void PropertyGridDialog_OnCancel(Dialog* pDialog);


PropertyGridDialog* PropertyGridDialog_Create()
{
    PropertyGridDialog* pPropertyGridDialog = (PropertyGridDialog*)malloc(sizeof(PropertyGridDialog));

    if (pPropertyGridDialog)
    {
        memset(pPropertyGridDialog, 0, sizeof(PropertyGridDialog));

        PropertyGridDialog_Init(pPropertyGridDialog);
    }

    return pPropertyGridDialog;
}

void PropertyGridDialog_Init(PropertyGridDialog* pPropertyGridDialog)
{
    Dialog_Init(&pPropertyGridDialog->base);

    pPropertyGridDialog->base.DlgUserProc = PropertyGridDialog_DlgUserProc;
    pPropertyGridDialog->base.OnInitDialog = PropertyGridDialog_OnInitDialog;
    pPropertyGridDialog->base.OnCancel = PropertyGridDialog_OnCancel;
    pPropertyGridDialog->base.OnOK = PropertyGridDialog_OnOK;

    pPropertyGridDialog->m_pPropertyGrid = PropertyGridCtl_Create();
}

INT_PTR PropertyGridDialog_DlgUserProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    PropertyGridDialog* pPropertyGridDialog = (PropertyGridDialog*)pDialog;
    switch (message)
    {
    }

    return Dialog_DefaultDialogProc(pDialog, message, wParam, lParam);
}

BOOL PropertyGridDialog_OnInitDialog(Dialog* pDialog)
{
    PropertyGridDialog* pPropertyGridDialog = (PropertyGridDialog*)pDialog;
    HWND hDlg = Window_GetHWND(&pPropertyGridDialog->base.base);

    HWND hPropertyGrid = GetDlgItem(hDlg, IDC_PROPERTYGRID);
    Window_Attach((Window*)pPropertyGridDialog->m_pPropertyGrid, hPropertyGrid);

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

    return TRUE;
}

void PropertyGridDialog_OnOK(Dialog* pDialog)
{
    PropertyGridDialog* pPropertyGridDialog = (PropertyGridDialog*)pDialog;
    EndDialog(Window_GetHWND(&pPropertyGridDialog->base.base), 0);
}

void PropertyGridDialog_OnCancel(Dialog* pDialog)
{
    PropertyGridDialog* pPropertyGridDialog = (PropertyGridDialog*)pDialog;
    EndDialog(Window_GetHWND(&pPropertyGridDialog->base.base), 0);
}
