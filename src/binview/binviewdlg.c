#include "../precomp.h"
#include "binviewdlg.h"
#include "../util/assert.h"

void BinViewDialog_Init(BinViewDialog* pBinViewDialog);
BOOL BinViewDialog_OnInitDialog(Dialog* pDialog);
void BinViewDialog_OnOK(Dialog* pDialog);
void BinViewDialog_OnCancel(Dialog* pDialog);
INT_PTR BinViewDialog_DlgUserProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam);

void BinViewDialog_Init(BinViewDialog* pBinViewDialog)
{
    ASSERT(pBinViewDialog);
    memset(pBinViewDialog, 0, sizeof(BinViewDialog));

    Dialog_Init(&pBinViewDialog->base);

    pBinViewDialog->base.OnInitDialog = BinViewDialog_OnInitDialog;
    pBinViewDialog->base.OnOK = BinViewDialog_OnOK;
    pBinViewDialog->base.OnCancel = BinViewDialog_OnCancel;
    pBinViewDialog->base.DlgUserProc = BinViewDialog_DlgUserProc;
}

BOOL BinViewDialog_OnInitDialog(Dialog* pDialog)
{
    BinViewDialog* pBinViewDialog = (BinViewDialog*)pDialog;
    ASSERT(pBinViewDialog);
    return TRUE;
}

void BinViewDialog_OnOK(Dialog* pDialog)
{
    BinViewDialog* pBinViewDialog = (BinViewDialog*)pDialog;
    ASSERT(pBinViewDialog);
}

void BinViewDialog_OnCancel(Dialog* pDialog)
{
    BinViewDialog* pBinViewDialog = (BinViewDialog*)pDialog;
    ASSERT(pBinViewDialog);
}

void BinViewDialog_OnPlayBtnClick()
{

}

void BinViewDialog_OnPauseBtnClick()
{

}

void BinViewDialog_OnResumeBtnClick()
{

}

void BinViewDialog_OnStopBtnClick()
{

}

void BinViewDialog_OnReadAsPictureBtnClick()
{

}

INT_PTR BinViewDialog_DlgUserProc(Dialog* pDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    BinViewDialog* pBinViewDialog = (BinViewDialog*)pDialog;
    ASSERT(pBinViewDialog);
    switch (message)
    {

    }

    return FALSE;
}
