#include "../precomp.h"

#include "dockhostcomposite.h"
#include "../util/list.h"
#include "../util/assert.h"
#include "../win32/util.h"

/*
 * [PRIVATE]
 */

/*
 * [PUBLIC]
 */
typedef struct DockHostComposite DockHostComposite;
struct DockHostComposite {
    Window* pWndHost;
    List* pDockSiteLeft;
    List* pDockSiteRight;
};

void DockHostComposite_Init(DockHostComposite* pDockHostComposite);

DockHostComposite* DockHostComposite_Create()
{
    DockHostComposite* pDockHostComposite = (DockHostComposite*)malloc(sizeof(DockHostComposite));

    if (pDockHostComposite)
    {
        DockHostComposite_Init(pDockHostComposite);
    }

    return pDockHostComposite;
}

void DockHostComposite_Init(DockHostComposite* pDockHostComposite)
{
    memset(pDockHostComposite, 0, sizeof(DockHostComposite));

    pDockHostComposite->pDockSiteLeft = List_Create(sizeof(DockWindowInfo));
    pDockHostComposite->pDockSiteRight = List_Create(sizeof(DockWindowInfo));
}

void DockHostComposite_SetWindow(DockHostComposite* pDockHostComposite, Window* pWindow)
{
    pDockHostComposite->pWndHost = pWindow;
}

Window* DockHostComposite_GetWindow(DockHostComposite* pDockHostComposite)
{
    return pDockHostComposite->pWndHost;
}

void DockHostComposite_Dock(DockHostComposite* pDockHostComposite, DockWindowInfo* pWindowInfo, int nSide)
{
    List* pList = NULL;

    switch (nSide)
    {
    case 1:
        pList = pDockHostComposite->pDockSiteLeft;
        break;
    case 3:
        pList = pDockHostComposite->pDockSiteRight;
        break;
    }

    if (pList)
    {
        List_InsertBack(pList, pWindowInfo);
    }
}

LRESULT DockHostComposite_WndProc(DockHostComposite* pDockHostComposite, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, PBOOL pbProcessed)
{
    switch (message)
    {
    case WM_CREATE:
    {
        return TRUE;
    }
        break;

    case WM_SIZE:
    {
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        rcClient.top += 48;

        // InflateRect(&rcClient, -50, -50);

        {
            int listLength = List_GetLength(pDockHostComposite->pDockSiteLeft);
            if (listLength)
            {
                int dockHeight = (RECTHEIGHT(&rcClient) - 24) / listLength;
                for (size_t i = 0; i < listLength; ++i)
                {
                    DockWindowInfo* pdwi = List_Get(pDockHostComposite->pDockSiteLeft, i);
                    SetWindowPos(pdwi->hWnd, NULL, rcClient.left + 24, rcClient.top + dockHeight * i, 200, dockHeight, 0);
                }
            }
        }

        {
            int listLength = List_GetLength(pDockHostComposite->pDockSiteRight);
            if (listLength)
            {
                int dockHeight = (RECTHEIGHT(&rcClient) - 24) / listLength;
                for (size_t i = 0; i < listLength; ++i)
                {
                    DockWindowInfo* pdwi = List_Get(pDockHostComposite->pDockSiteRight, i);
                    SetWindowPos(pdwi->hWnd, NULL, rcClient.right - 200 - 24, rcClient.top + dockHeight * i, 200, dockHeight, 0);
                }
            }
        }
        *pbProcessed = TRUE;
        return 0;
    }
        break;

    case WM_LBUTTONDOWN:

        break;

    case WM_ERASEBKGND:
        *pbProcessed = TRUE;
        return TRUE;
        break;
    }

    *pbProcessed = FALSE;
    return 0;
}

void DockHostComposite_Draw(DockHostComposite* pDockHostComposite, HDC hdc)
{
    HWND hWnd = Window_GetHWND(pDockHostComposite->pWndHost);

    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    /* Adjust client area border */
    rcClient.top += 48;
    // InflateRect(&rcClient, -50, -50);

    /* Background */
    HBRUSH hBackgroundBrush = CreateSolidBrush(Win32_HexToCOLORREF(L"#48425f"));
    FillRect(hdc, &rcClient, hBackgroundBrush);
    DeleteObject(hBackgroundBrush);

    HRGN hRegion = CreateRectRgnIndirect(&rcClient);
    ExtSelectClipRgn(hdc, hRegion, RGN_AND);

    /* Draw frame */
    SelectObject(hdc, GetStockObject(NULL_PEN));
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(hdc, Win32_HexToCOLORREF(L"#6d648e"));

    /* Bottom site background */
    Rectangle(hdc, rcClient.left, rcClient.bottom - 24, rcClient.right, rcClient.bottom);
    /* Left site background */
    Rectangle(hdc, rcClient.left, rcClient.top, rcClient.left + 24, rcClient.bottom);
    /* Right site background */
    Rectangle(hdc, rcClient.right - 24, rcClient.top, rcClient.right, rcClient.bottom);

    /* Pins */
    SetDCPenColor(hdc, Win32_HexToCOLORREF(L"#48425f"));
    SetDCBrushColor(hdc, Win32_HexToCOLORREF(L"#9185be"));
    SelectObject(hdc, GetStockObject(DC_PEN));

    int pinHeight = 100;

    HFONT hFont = GetCurrentObject(hdc, OBJ_FONT);
    LOGFONT lf;
    GetObject(hFont, sizeof(lf), &lf);
    wcscpy_s(lf.lfFaceName, sizeof(lf.lfFaceName) / sizeof(lf.lfFaceName[0]), L"Montserrat");
    lf.lfEscapement = 2700;
    // lf.lfOrientation = 2700;
    HFONT hNewFont = CreateFontIndirect(&lf);

    HFONT hOldFont = SelectObject(hdc, hNewFont);

    {
        BeginPath(hdc);
        MoveToEx(hdc, rcClient.left + 24 - 1, rcClient.top, NULL);
        LineTo(hdc, rcClient.left + 24 - 1, rcClient.bottom - 24);
        EndPath(hdc);
        StrokePath(hdc);

        int listLength = List_GetLength(pDockHostComposite->pDockSiteLeft);
        for (size_t i = 0; i < listLength; ++i)
        {
            BeginPath(hdc);
            MoveToEx(hdc, rcClient.left + 24, rcClient.top + 2 + pinHeight * i, NULL);
            LineTo(hdc, rcClient.left + 4, rcClient.top + 2 + pinHeight * i);
            LineTo(hdc, rcClient.left + 4, rcClient.top + pinHeight - 2 + pinHeight * i);
            LineTo(hdc, rcClient.left + 24, rcClient.top + pinHeight - 2 + pinHeight * i);
            EndPath(hdc);
            StrokeAndFillPath(hdc);

            DockWindowInfo* pdwi = List_Get(pDockHostComposite->pDockSiteLeft, i);
            if (pdwi)
            {
                int len = GetWindowTextLength(pdwi->hWnd);
                PWSTR szText = (PWSTR)malloc((len + 1) * sizeof(WCHAR));
                GetWindowText(pdwi->hWnd, szText, len + 1);
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
                TextOut(hdc, rcClient.left + 24, rcClient.top + 8 + pinHeight * i, szText, len);
                free(szText);
            }
        }
    }

    {
        BeginPath(hdc);
        MoveToEx(hdc, rcClient.right - 24, rcClient.top, NULL);
        LineTo(hdc, rcClient.right - 24, rcClient.bottom - 24);
        EndPath(hdc);
        StrokePath(hdc);

        int listLength = List_GetLength(pDockHostComposite->pDockSiteRight);
        for (size_t i = 0; i < listLength; ++i)
        {
            BeginPath(hdc);
            MoveToEx(hdc, rcClient.right - 24 - 1, rcClient.top + 2 + pinHeight * i, NULL);
            LineTo(hdc, rcClient.right - 24 + 20 - 1, rcClient.top + 2 + pinHeight * i);
            LineTo(hdc, rcClient.right - 24 + 20 - 1, rcClient.top + pinHeight - 2 + pinHeight * i);
            LineTo(hdc, rcClient.right - 24 - 1, rcClient.top + pinHeight - 2 + pinHeight * i);
            EndPath(hdc);
            StrokeAndFillPath(hdc);

            DockWindowInfo* pdwi = List_Get(pDockHostComposite->pDockSiteRight, i);
            if (pdwi)
            {
                int len = GetWindowTextLength(pdwi->hWnd);
                PWSTR szText = (PWSTR)malloc((len + 1) * sizeof(WCHAR));
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
                GetWindowText(pdwi->hWnd, szText, len + 1);
                TextOut(hdc, rcClient.right - 1 - 4, rcClient.top + 8 + pinHeight * i, szText, len);
                free(szText);
            }
        }
    }

    if (hRegion)
    {
        DeleteObject(hRegion);
    }
}
