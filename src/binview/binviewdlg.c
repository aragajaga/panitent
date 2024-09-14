#include "../precomp.h"
#include "binviewdlg.h"
#include "../util/assert.h"

void BinViewDialog_Init(BinViewDialog* pBinViewDialog);
void BinViewDialog_OnInitDialog(BinViewDialog* pBinViewDialog);
void BinViewDialog_OnOK(BinViewDialog* pBinViewDialog);
void BinViewDialog_OnCancel(BinViewDialog* pBinViewDialog);
INT_PTR CALLBACK BinViewDialog_DlgUserProc(BinViewDialog* pBinViewDialog, UINT message, WPARAM wParam, LPARAM lParam);

void BinViewDialog_Init(BinViewDialog* pBinViewDialog)
{
    ASSERT(pBinViewDialog);
    memset(pBinViewDialog, 0, sizeof(BinViewDialog));

    Dialog_Init((Dialog*)&pBinViewDialog);

    pBinViewDialog->base.OnInitDialog = BinViewDialog_OnInitDialog;
    pBinViewDialog->base.OnOK = BinViewDialog_OnOK;
    pBinViewDialog->base.OnCancel = BinViewDialog_OnCancel;
    pBinViewDialog->base.DlgUserProc = BinViewDialog_DlgUserProc;
}

void BinViewDialog_OnInitDialog(BinViewDialog* pBinViewDialog)
{
    ASSERT(pBinViewDialog);
}

void BinViewDialog_OnOK(BinViewDialog* pBinViewDialog)
{
    ASSERT(pBinViewDialog);
}

void BinViewDialog_OnCancel(BinViewDialog* pBinViewDialog)
{
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

INT_PTR CALLBACK BinViewDialog_DlgUserProc(BinViewDialog* pBinViewDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    }

    return FALSE;
}
