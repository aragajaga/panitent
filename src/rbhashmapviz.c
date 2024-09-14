#include "precomp.h"

#include "rbhashmapviz.h"

#include "win32/util.h"
#include "util/rbhashmap.h"
#include "resource.h"

INT_PTR RBHashMapVizDialog_DlgUserProc(RBHashMapVizDialog* pRBHashMapVizDialog, UINT message, WPARAM wParam, LPARAM lParam);
void RBHashMapVizDialog_OnInitDialog(RBHashMapVizDialog* pRBHashMapVizDialog);
void RBHashMapVizDialog_OnOK(RBHashMapVizDialog* pRBHashMapVizDialog);
void RBHashMapVizDialog_OnCancel(RBHashMapVizDialog* pRBHashMapVizDialog);
void RBHashMapVizDialog_OnInsertBtn(RBHashMapVizDialog* pRBHashMapVizDialog);
void RBHashMapVizDialog_OnPaint(RBHashMapVizDialog* pRBHashMapVizDialog);

RBHashMapVizDialog* RBHashMapVizDialog_Create()
{
    RBHashMapVizDialog* pRBHashMapVizDialog = (RBHashMapVizDialog*)malloc(sizeof(RBHashMapVizDialog));
    if (pRBHashMapVizDialog)
    {
        memset(pRBHashMapVizDialog, 0, sizeof(RBHashMapVizDialog));
        RBHashMapVizDialog_Init(pRBHashMapVizDialog);
    }

    return pRBHashMapVizDialog;
}

void RBHashMapVizDialog_Init(RBHashMapVizDialog* pRBHashMapVizDialog)
{
    Dialog_Init((Dialog*)pRBHashMapVizDialog);

    pRBHashMapVizDialog->base.DlgUserProc = (FnDialogDlgUserProc)RBHashMapVizDialog_DlgUserProc;
    pRBHashMapVizDialog->base.OnInitDialog = (FnDialogOnInitDialog)RBHashMapVizDialog_OnInitDialog;
    pRBHashMapVizDialog->base.OnOK = (FnDialogOnOK)RBHashMapVizDialog_OnOK;
    pRBHashMapVizDialog->base.OnCancel = (FnDialogOnCancel)RBHashMapVizDialog_OnCancel;

    RBHashMapContext ctx = {
        .pfnKeyComparator = _tcscmp,
        .pfnKeyDeleter = free,
        .pfnValueDeleter = free
    };
    pRBHashMapVizDialog->m_pHashMap = RBHashMap_Create(&ctx);
}

INT_PTR RBHashMapVizDialog_DlgUserProc(RBHashMapVizDialog* pRBHashMapVizDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_INSERTBTN)
        {
            RBHashMapVizDialog_OnInsertBtn(pRBHashMapVizDialog);
        }
        break;

    case WM_PAINT:
        RBHashMapVizDialog_OnPaint(pRBHashMapVizDialog);
        break;
        return 0;
    }

    return Dialog_DefaultDialogProc(pRBHashMapVizDialog, message, wParam, lParam);
}

void RBHashMapVizDialog_OnInitDialog(RBHashMapVizDialog* pRBHashMapVizDialog)
{
}

void RBHashMapVizDialog_OnOK(RBHashMapVizDialog* pRBHashMapVizDialog)
{
    EndDialog(pRBHashMapVizDialog->base.base.hWnd, 0);
}

void RBHashMapVizDialog_OnCancel(RBHashMapVizDialog* pRBHashMapVizDialog)
{
    EndDialog(pRBHashMapVizDialog->base.base.hWnd, 0);
}

static const TCHAR g_kBase64Chars[] = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

void GenerateRandomString(PTSTR pszOut, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        pszOut[i] = g_kBase64Chars[rand() % (ARRAYSIZE(g_kBase64Chars) - 1)];
    }
    /* Null-terminate the string */
    pszOut[length] = '\0';
}

void RBHashMapVizDialog_OnInsertBtn(RBHashMapVizDialog* pRBHashMapVizDialog)
{
    const TCHAR szText[9] = { 0 };
    GenerateRandomString(szText, ARRAYSIZE(szText) - 1);
    size_t len = _tcslen(szText);
    PTSTR pszText = (PTSTR)malloc((len + 1) * sizeof(WCHAR));
    if (!pszText)
    {
        return;
    }

    _tcscpy_s(pszText, len + 1, szText);

    RBHashMap_Insert(pRBHashMapVizDialog->m_pHashMap, pszText, pszText);

    HWND hDlg = pRBHashMapVizDialog->base.base.hWnd;
    RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW);
    // UpdateWindow(hDlg);
    // InvalidateRect(pRBHashMapVizDialog->base.base.hWnd, NULL, TRUE);
}

void DrawNode(HDC hDC, int x, int y, PCTSTR pszText, COLORREF color)
{
    int paddingX = 4;
    int paddingY = 2;

    /* Create and assign font */
    LOGFONT lf = { 0 };
    lf.lfWeight = FW_BOLD;
    lf.lfHeight = -MulDiv(9, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    _tcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Comfortaa");
    HFONT hFont = CreateFontIndirect(&lf);
    HFONT hOldFont = SelectFont(hDC, hFont);

    /* Get string dimensions */
    // const TCHAR szText[] = _T("Red-Black Tree node value text");
    SIZE textSize = { 0 };
    GetTextExtentPoint32(hDC, pszText, _tcslen(pszText), &textSize);

    /* Get solid brush for rectangle fill */
    HBRUSH hBrush = GetStockObject(DC_BRUSH);
    HBRUSH hOldBrush = SelectObject(hDC, hBrush);
    SetDCBrushColor(hDC, color);

    /* Draw rectangle */
    RoundRect(hDC, x - textSize.cx / 2 - paddingX, y - textSize.cy / 2 - paddingY, x + textSize.cx / 2 + paddingX, y + textSize.cy / 2 + paddingY, 10, 10);

    /* Set text color */
    SetTextColor(hDC, RGB(255, 255, 255));
    SetBkMode(hDC, TRANSPARENT);

    /* Draw text */
    TextOut(hDC, x - textSize.cx / 2, y - textSize.cy / 2, pszText, _tcslen(pszText));

    /* Clean-up resources */
    SelectObject(hDC, hOldBrush);
    DeleteObject(hBrush);
    SelectObject(hDC, hOldFont);
    DeleteObject(hFont);
}

void DrawTree(HDC hDC, RBTreeNode* node, int x, int y, int offsetX, int offsetY)
{
    if (!node)
    {
        return;
    }

    COLORREF color = (node->color == RED) ? RGB(255, 0, 0) : RGB(0, 0, 0);

    DrawNode(hDC, x, y, (PCTSTR)node->pKey, color);


    if (node->pLeft)
    {
        MoveToEx(hDC, x, y, NULL);
        LineTo(hDC, x - offsetX, y + offsetY);
        DrawTree(hDC, node->pLeft, x - offsetX, y + offsetY, offsetX / 2, offsetY);
    }

    if (node->pRight)
    {
        MoveToEx(hDC, x, y, NULL);
        LineTo(hDC, x + offsetX, y + offsetY);
        DrawTree(hDC, node->pRight, x + offsetX, y + offsetY, offsetX / 2, offsetY);
    }
}

void RBHashMapVizDialog_OnPaint(RBHashMapVizDialog* pRBHashMapVizDialog)
{
    HWND hDlg = pRBHashMapVizDialog->base.base.hWnd;
    RECT rcClient = { 0 };
    GetClientRect(hDlg, &rcClient);

    

    int width = RECTWIDTH(&rcClient);
    int height = RECTHEIGHT(&rcClient);

    int centerX = width / 2;
    int startY = 20;    /* Initial horizonal distance between nodes */
    int offsetX = width / 4;    /* Initial horizontal distance between nodes */
    int offsetY = 30;   /* Vertical distance between levels */

    HDC hDC = GetDC(hDlg);

    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hDC, width, height);
    HBITMAP hOldBitmap = SelectObject(hMemDC, hBitmap);

    HBRUSH hBrush = GetStockObject(WHITE_BRUSH);
    FillRect(hMemDC, &rcClient, hBrush);

    if (pRBHashMapVizDialog->m_pHashMap->tree->pRoot)
    {
        DrawTree(hMemDC, pRBHashMapVizDialog->m_pHashMap->tree->pRoot, centerX, startY, offsetX, offsetY);
    }

    BitBlt(hDC, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBitmap);
    DeleteBitmap(hBitmap);
    DeleteDC(hMemDC);

    ReleaseDC(hDlg, hDC);
}