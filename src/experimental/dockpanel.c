#include "../precomp.h"

#include "DockPanel.h"

#include "../resource.h"
#include <uxtheme.h>
#include "../win32/util.h"
#include "../util/assert.h"
#include "../toolwndframe.h"

#include "../util/list.h"

#include "bubble.h"

static const WCHAR szClassName[] = L"Experimental::DockPanel";

#define CAPTIONHEIGHT 24
#define BORDERSIZE 32

#define DCX_USESTYLE 0x10000


#define GLYPH_SIZE 8

enum {
    CB_NORMAL,
    CB_HOVER,
    CB_DOWN,
    CB_DISABLED
};

typedef struct SysConfig SysConfig;
struct SysConfig {
    int smCXPaddedBorder;
    int smCXSizeFrame;
    int smCYSizeFrame;
    int smCXEdge;
    int smCYEdge;
    int smCXSmIcon;
    int smCYSmIcon;
    int smCYCaption;
};

SysConfig g_sysConfig = { 0 };

void UpdateSysConfig()
{
    g_sysConfig.smCXPaddedBorder = GetSystemMetrics(SM_CXPADDEDBORDER);
    g_sysConfig.smCXSizeFrame = GetSystemMetrics(SM_CXSIZEFRAME);
    g_sysConfig.smCYSizeFrame = GetSystemMetrics(SM_CYSIZEFRAME);
    g_sysConfig.smCXEdge = GetSystemMetrics(SM_CXEDGE);
    g_sysConfig.smCYEdge = GetSystemMetrics(SM_CYEDGE);
    g_sysConfig.smCXSmIcon = GetSystemMetrics(SM_CXSMICON);
    g_sysConfig.smCYSmIcon = GetSystemMetrics(SM_CYSMICON);
    g_sysConfig.smCYCaption = GetSystemMetrics(SM_CYCAPTION);
}

typedef struct CaptionGlyphs CaptionGlyphs;
struct CaptionGlyphs {
    HDC hdc;
    HBITMAP hBitmap;
    SIZE size;
    HBITMAP hPrevBitmap;
};

typedef struct CaptionButtonInfo CaptionButtonInfo;
struct CaptionButtonInfo {
    RECT rc;
    int iGlyph;
    int iState;
    int ht;
};

const CaptionGlyphs* GetCaptionGlyphs()
{
    static CaptionGlyphs s_glyphs = { 0 };

    if (!s_glyphs.hBitmap)
    {
        s_glyphs.hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FLOATINGGLYPHS));

        HDC hdcScreen = GetDC(NULL);
        s_glyphs.hdc = CreateCompatibleDC(hdcScreen);
        ReleaseDC(NULL, hdcScreen);
        s_glyphs.hPrevBitmap = SelectObject(s_glyphs.hdc, s_glyphs.hBitmap);

        BITMAP bm;
        GetObject(s_glyphs.hBitmap, sizeof(bm), &bm);
        s_glyphs.size.cx = bm.bmWidth;
        s_glyphs.size.cy = bm.bmHeight;
    }

    return &s_glyphs;
}

void DestroyCaptionGlyphs()
{
    CaptionGlyphs* pCaptionGlyphs = GetCaptionGlyphs();

    SelectObject(pCaptionGlyphs->hdc, pCaptionGlyphs->hPrevBitmap);
    DeleteObject(pCaptionGlyphs->hBitmap);
    DeleteDC(pCaptionGlyphs->hdc);

    memset(pCaptionGlyphs, 0, sizeof(*pCaptionGlyphs));
}

void DrawCaptionGlyph2(HDC hdc, int x, int y, int iGlyph)
{
    CaptionGlyphs* pGlyphs = GetCaptionGlyphs();
    TransparentBlt(hdc,
        x, y,
        pGlyphs->size.cy, pGlyphs->size.cy,
        pGlyphs->hdc, iGlyph * pGlyphs->size.cy, 0,
        pGlyphs->size.cy, pGlyphs->size.cy, COLORREF_MAGENTA);
}

void DrawCaptionButton2(CaptionButton* pCaptionButton, HDC hdc, PRECT rcButton, int state)
{    
    if (state == CB_HOVER || state == CB_DOWN)
    {
        SelectObject(hdc, GetStockObject(NULL_PEN));
        SelectObject(hdc, GetStockObject(DC_BRUSH));

        COLORREF color = Win32_HexToCOLORREF(state == CB_HOVER ? L"ada4ce" : L"c8c2df");
        SetDCBrushColor(hdc, color);
        Rectangle(hdc, rcButton->left, rcButton->top, rcButton->right, rcButton->bottom);
    }

    const CaptionGlyphs* pCaptionGlyphs = GetCaptionGlyphs();
    DrawCaptionGlyph2(hdc,
        rcButton->left + (rcButton->right - rcButton->left - pCaptionGlyphs->size.cy) / 2,
        rcButton->top + (rcButton->bottom - rcButton->top - pCaptionGlyphs->size.cy) / 2,
        pCaptionButton->glyph);
}

void DrawCaptionButton3(HDC hdc, CaptionButtonInfo* pButton)
{
    PRECT prc = &pButton->rc;

    if (pButton->iState == CB_HOVER || pButton->iState == CB_DOWN)
    {
        SelectObject(hdc, GetStockObject(NULL_PEN));
        SelectObject(hdc, GetStockObject(DC_BRUSH));

        COLORREF color = Win32_HexToCOLORREF(pButton->iState == CB_HOVER ? L"ada4ce" : L"c8c2df");
        SetDCBrushColor(hdc, color);
        Rectangle(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }

    const CaptionGlyphs* pCaptionGlyphs = GetCaptionGlyphs();
    DrawCaptionGlyph2(hdc,
        prc->left + (prc->right - prc->left - pCaptionGlyphs->size.cy) / 2,
        prc->top + (prc->bottom - prc->top - pCaptionGlyphs->size.cy) / 2,
        pButton->iGlyph);
}

List* g_pList = NULL;

void UpdateCaptionButtonPositions(List* pList, PRECT prcWindow, int btnWidth, int top, int bottom, DWORD dwStyle)
{
    int xOffset = 0;
    /* Iterate over caption buttons list */
    for (size_t i = 0; i < List_GetLength(pList); ++i)
    {
        xOffset += btnWidth;
    }
    xOffset += (dwStyle & WS_MAXIMIZE) ? 10 : 4;

    for (size_t i = 0; i < List_GetLength(pList); ++i)
    {
        CaptionButtonInfo* pButton = List_Get(pList, i);

        pButton->rc.left = prcWindow->right - xOffset;
        xOffset -= btnWidth;
        pButton->rc.top = top;
        pButton->rc.right = prcWindow->right - xOffset;
        pButton->rc.bottom = bottom;
    }
}

void UpdateCaptionButtonRects(HWND hWnd)
{
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);
    MapWindowRect(HWND_DESKTOP, hWnd, &rcWindow);
    /* Remove offsets to normalize window rect upper left corner to 0 */
    OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);

    DWORD dwStyle = GetWindowStyle(hWnd);

    int btnWidth = 45;
    int btnHeight = 30;

    DWORD dwExStyle = GetWindowExStyle(hWnd);
    if (dwExStyle & WS_EX_TOOLWINDOW)
    {
        btnWidth = 17;
        UpdateCaptionButtonPositions(g_pList, &rcWindow, btnWidth, 7, 24, dwStyle);
    }
    else {
        if (dwStyle & WS_MAXIMIZE)
        {
            UpdateCaptionButtonPositions(g_pList, &rcWindow, btnWidth, 8, 31, dwStyle);
        }
        else {
            UpdateCaptionButtonPositions(g_pList, &rcWindow, btnWidth, 1, 31, dwStyle);
        }
    }
}

void OffsetRectClientToWindow(HWND hWnd, PRECT prc)
{
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);
    MapWindowRect(HWND_DESKTOP, hWnd, &rcWindow);

    OffsetRect(prc, rcWindow.left, rcWindow.top);
}

BOOL g_fDrawLock;
BOOL g_fEnableStyle = TRUE;

int GetCaptionButtonIdByPoint(List* pList, POINT pt)
{
    for (size_t i = 0; i < List_GetLength(g_pList); ++i)
    {
        CaptionButtonInfo* pButton = List_Get(pList, i);

        if (PtInRect(&pButton->rc, pt))
        {
            return i;
        }
    }

    return -1;
}

LRESULT StylingProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, PBOOL bProcessed)
{
    switch (message)
    {
    case WM_NCMOUSEMOVE:
    {
        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
    
        /* Window-relative cursor position */
        POINT pt = { 0 };
        pt.x = GET_X_LPARAM(lParam) - rcWindow.left;
        pt.y = GET_Y_LPARAM(lParam) - rcWindow.top;
    
        /* Transform window to it's client coordinates */
        MapWindowRect(HWND_DESKTOP, hWnd, &rcWindow);
        
        /* Remove offsets to normalize window rect upper left corner to 0 */
        OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
    
        /******************
         * Implementation *
         ******************/
    
        /* Update caption button rects */
        UpdateCaptionButtonRects(hWnd);
    
        /* Reset all to normal state */
        for (size_t i = 0; i < List_GetLength(g_pList); ++i)
        {
            CaptionButtonInfo* pButton = List_Get(g_pList, i);
            pButton->iState = CB_NORMAL;
        }
    
        /* Set hover state if cursor above the button */
        int btnId;
        if ((btnId = GetCaptionButtonIdByPoint(g_pList, pt)) != -1)
        {
            CaptionButtonInfo* pButton = List_Get(g_pList, btnId);
            
            pButton->iState = CB_HOVER;
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            *bProcessed = TRUE;
            break;
        }
    }
        break;
    
    case WM_NCLBUTTONUP:
    
        /* WM_NCLBUTTONUP does nothing with caption buttons by default
         * Maybe it's because of internal checks that trying to
         * ensure that released button is in specific system button rectangle.
         * By the way we can't deal with system style caption button rectangles.
         * 
         * So we handle it by ourselves by posting corresponding
         * WM_SYSCOMMAND messages
         */
    
        switch (wParam)
        {
        case HTMINBUTTON:
        case HTMAXBUTTON:
        case HTCLOSE:
        {
            DWORD dwStyle = GetWindowStyle(hWnd);
            int nCommand = 0;
            
            switch (wParam)
            {
            case HTMINBUTTON:
                /* Restore window if already minimized */
                nCommand = (dwStyle & WS_MINIMIZE) ? SC_RESTORE : SC_MINIMIZE;
                break;
            case HTMAXBUTTON:
                /* Restore window if already maximized */
                nCommand = (dwStyle & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE;
                break;
            case HTCLOSE:
                nCommand = SC_CLOSE;
                break;
            }
    
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            /* lParam is a cursor position, so pass it through */
            DefWindowProc(hWnd, WM_SYSCOMMAND, (WPARAM)nCommand, lParam);
    
            *bProcessed = TRUE;
            return 0;
        }
            break;
        }
    
        break;
    
    case WM_NCLBUTTONDOWN:
    {
        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
    
        /* Window-relative cursor position */
        POINT pt = { 0 };
        pt.x = GET_X_LPARAM(lParam) - rcWindow.left;
        pt.y = GET_Y_LPARAM(lParam) - rcWindow.top;
    
        /* Transform window to it's client coordinates */
        MapWindowRect(HWND_DESKTOP, hWnd, &rcWindow);
    
        /* Remove offsets to normalize window rect upper left corner to 0 */
        OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
    
        /******************
         * Implementation *
         ******************/
    
        int btnId;
        if ((btnId = GetCaptionButtonIdByPoint(g_pList, pt)) != -1)
        {
            CaptionButtonInfo* pButton = List_Get(g_pList, btnId);
            pButton->iState = CB_DOWN;
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            *bProcessed = TRUE;
            return 0;
        }
    }
        break;
    
    case WM_NCHITTEST:
    {
        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
    
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
    
        /* Window-relative cursor position */
        POINT pt = { 0 };
        pt.x = GET_X_LPARAM(lParam) - rcWindow.left;
        pt.y = GET_Y_LPARAM(lParam) - rcWindow.top;
    
        /* Transform window to it's client coordinates */
        MapWindowRect(HWND_DESKTOP, hWnd, &rcWindow);
    
        /* Translate client rect to window coordinates */
        OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
    
        /* Remove offsets to normalize window rect upper left corner to 0 */
        OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
    
        /******************
         * Implementation *
         ******************/
    
        UpdateCaptionButtonRects(hWnd);
    
        int ht = HTNOWHERE;
    
        if (PtInRect(&rcClient, pt))
        {
            ht = HTCLIENT;
        }
    
        /* Iterate over caption button rects and check that cursor in these */
        for (size_t i = 0; i < List_GetLength(g_pList); ++i)
        {
            CaptionButtonInfo* pButton = List_Get(g_pList, i);
            if (PtInRect(&pButton->rc, pt))
            {
                ht = pButton->ht;
                break;
            }
        }
        
        int smCYSizeFrame = g_sysConfig.smCYSizeFrame;
        int smCXSizeFrame = g_sysConfig.smCXSizeFrame;
        int smCXPaddedBorder = g_sysConfig.smCXPaddedBorder;
        

        if (ht)
        {
            /* Empty check for that ht is already defined by caption button */
        }
        
        /* Window icon (system menu) */
        else if (PtIn(pt,
            smCYSizeFrame + smCXPaddedBorder,
            smCXSizeFrame + smCXPaddedBorder,
            smCYSizeFrame + smCXPaddedBorder + g_sysConfig.smCXSmIcon,
            smCXSizeFrame + smCXPaddedBorder + g_sysConfig.smCYSmIcon))
        {
            ht = HTSYSMENU;
        }
    
        /* Caption */
        else if (PtIn(pt,
            smCYSizeFrame + smCXPaddedBorder,
            smCXSizeFrame + smCXPaddedBorder,
            rcWindow.right - (smCYSizeFrame + smCXPaddedBorder),
            smCXSizeFrame + smCXPaddedBorder + g_sysConfig.smCYCaption))
        {
            ht = HTCAPTION;
        }
    
        /* Left sizing border */
        else if (PtIn(pt,
            0,
            12,
            smCYSizeFrame + smCXPaddedBorder,
            rcWindow.bottom - 12))
        {
            ht = HTLEFT;
        }
    
        /* Top sizing border */
        else if (PtIn(pt,
            12,
            0,
            rcWindow.right - 12,
            smCXSizeFrame + smCXPaddedBorder))
        {
            ht = HTTOP;
        }
    
        /* Right sizing border */
        else if (PtIn(pt,
            rcWindow.right - (smCYSizeFrame + smCXPaddedBorder),
            12,
            rcWindow.right,
            rcWindow.bottom - 12))
        {
            ht = HTRIGHT;
        }
    
        /* Bottom sizing border */
        else if (PtIn(pt,
            12,
            rcWindow.bottom - (smCXSizeFrame + smCXPaddedBorder),
            rcWindow.right - 12,
            rcWindow.bottom))
        {
            ht = HTBOTTOM;
        }
    
        /* Top-left sizing border */
        else if (PtIn(pt,
            0,
            0,
            12,
            12))
        {
            ht = HTTOPLEFT;
        }
    
        /* Top-right sizing border */
        else if (PtIn(pt,
            rcWindow.right - 12,
            0,
            rcWindow.right,
            12))
        {
            ht = HTTOPRIGHT;
        }
    
        /* Bottom-left sizing border */
        else if (PtIn(pt,
            0,
            rcWindow.bottom - 12,
            12,
            rcWindow.bottom))
        {
            ht = HTBOTTOMLEFT;
        }
    
        /* Bottom-right sizing border */
        else if (PtIn(pt,
            rcWindow.right - 12,
            rcWindow.bottom - 12,
            rcWindow.right,
            rcWindow.bottom))
        {
            ht = HTBOTTOMRIGHT;
        }
    
        if (ht)
        {
            *bProcessed = TRUE;
        }
    
        return ht;
    }
        break;
    
    case WM_NCCREATE:
    {
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;

        UpdateSysConfig();

        g_pList = List_Create(sizeof(CaptionButtonInfo));
    
        CaptionButtonInfo btn = {0};
        btn.iState = CB_NORMAL;
        btn.ht = HTNOWHERE;
    
        btn.iGlyph = GLYPH_HELP;
        List_InsertBack(g_pList, &btn);
    
        btn.iGlyph = GLYPH_MORE;
        List_InsertBack(g_pList, &btn);
    
        btn.iGlyph = GLYPH_PIN;
        List_InsertBack(g_pList, &btn);
    
        btn.iGlyph = GLYPH_MINIMIZE;
        btn.ht = HTMINBUTTON;
        List_InsertBack(g_pList, &btn);
    
        btn.iGlyph = GLYPH_MAXIMIZE;
        btn.ht = HTMAXBUTTON;
        List_InsertBack(g_pList, &btn);
    
        btn.iGlyph = GLYPH_CLOSE;
        btn.ht = HTCLOSE;
        List_InsertBack(g_pList, &btn);
    
        SetWindowTheme(hWnd, L"", L"");

        *bProcessed = TRUE;
        return DefWindowProc(hWnd, WM_NCCREATE, wParam, lParam);
    }
        break;
    
    case WM_NCCALCSIZE:
    {
        BOOL bProcess = (BOOL)wParam;
        DWORD dwStyle = GetWindowStyle(hWnd);
    
        RECT* rc = NULL;
        if (bProcess)
        {
            NCCALCSIZE_PARAMS* params = (LPNCCALCSIZE_PARAMS)lParam;
            
            /* params->rgrc[0] is the new rectangle
             * params->rgrc[1] is the old rectangle
             * params->rgrc[2] is the client rectangle
             * 
             * https://stackoverflow.com/a/2135120
             */
            rc = &params->rgrc[0];
        }
        else {
            rc = (PRECT)lParam;
        }

        if (dwStyle & WS_THICKFRAME)
        {
            InflateRect(&rc[0], -(g_sysConfig.smCYSizeFrame + g_sysConfig.smCXPaddedBorder), -(g_sysConfig.smCXSizeFrame + g_sysConfig.smCXPaddedBorder));
            rc[0].top -= g_sysConfig.smCXPaddedBorder;
        }
        else {
            // InflateRect(&rc[0], -1, -1);
        }

        if (dwStyle & WS_CAPTION)
        {
            rc->top += g_sysConfig.smCYCaption + g_sysConfig.smCXSizeFrame;
            if (dwStyle & WS_THICKFRAME)
            {
                // rc->top += smCXSizeFrame;
            }
        }

        LRESULT lResult = 0;
        if (bProcess)
        {
            lResult = 0;
        }
    
        *bProcessed = TRUE;
        return lResult;
    }
        break;
    
    case WM_NCPAINT:
    {
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
    
        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
    
        POINT ptDisplace = {
            .x = rcWindow.left,
            .y = rcWindow.right
        };
    
        MapWindowRect(HWND_DESKTOP, hWnd, &rcWindow);
        OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
        OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
    
        UpdateCaptionButtonRects(hWnd);
    
    
        DWORD dwFlags = DCX_WINDOW;
        /* Note the use of undocumented flags below. GetDCEx doesn't work without these
         * GetDCEx returns NULL if there is no DCX_USESTYLE, and sometimes, if no DCX_LOCKWINDOWUPDATE flags
         */
        dwFlags |= DCX_LOCKWINDOWUPDATE;
        dwFlags |= DCX_USESTYLE;
    
        /*
        if (hrgn)
        {
            dwFlags |= DCX_INTERSECTRGN;
        }
        */
    
        HRGN hrgn = (HRGN)wParam;
        HDC hdc = GetDCEx(hWnd, NULL, dwFlags);
    
        /*
         * Undocumented WM_NCPAINT's (HRGN)wParam == 1(NULLREGION) issue
         * https://stackoverflow.com/a/61901103
         * https://stackoverflow.com/a/50135792
         */
        HRGN hrgnTemp = NULL;
    
        if (!hrgn || hrgn == NULLREGION)
        {
            /* Exclude client area from hdc */
            ExcludeClipRect(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
        }
        else {
            // MessageBox(NULL, L"Custom region", L"Info", MB_OK | MB_ICONINFORMATION);
    
            hrgnTemp = CreateRectRgn(
                rcClient.left + ptDisplace.x,
                rcClient.top + ptDisplace.y,
                rcClient.right + ptDisplace.x,
                rcClient.bottom + ptDisplace.y
            );
    
            if (CombineRgn(hrgnTemp, hrgn, hrgnTemp, RGN_DIFF) == NULLREGION)
            {
                /* Nothing to paint */
            }
    
            OffsetRgn(hrgnTemp, -ptDisplace.x, -ptDisplace.y);
            ExtSelectClipRgn(hdc, hrgnTemp, RGN_AND);
        }
    
        if (hdc)
        {
            HDC hdcDoubleBuffer = CreateCompatibleDC(hdc);
            HBITMAP hbmDoubleBuffer = CreateCompatibleBitmap(hdc, rcWindow.right, rcWindow.bottom);
            HBITMAP hbmDBOld = SelectObject(hdcDoubleBuffer, hbmDoubleBuffer);
    
            SelectObject(hdcDoubleBuffer, GetStockObject(DC_BRUSH));
            SelectObject(hdcDoubleBuffer, GetStockObject(DC_PEN));
    
            /* Draw frame */
            SetDCBrushColor(hdcDoubleBuffer, Win32_HexToCOLORREF(L"#9185be"));
            SetDCPenColor(hdcDoubleBuffer, Win32_HexToCOLORREF(L"#6d648e"));
            
            DWORD dwStyle = GetWindowStyle(hWnd);

            if (!(dwStyle & WS_THICKFRAME))
            {
                SetDCPenColor(hdcDoubleBuffer, Win32_HexToCOLORREF(L"#9185be"));
                // SelectObject(hdcDoubleBuffer, GetStockObject(NULL_PEN));
            }
            
            Rectangle(hdcDoubleBuffer, rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);
    
            /* Draw caption */
    
            if (dwStyle & WS_CAPTION)
            {
                int smCXSmIcon = g_sysConfig.smCXSmIcon;
                int smCYSmIcon = g_sysConfig.smCYSmIcon;
                int smCXEdge = g_sysConfig.smCXEdge;
                int smCYEdge = g_sysConfig.smCYEdge;
    
                /* Draw sysmenu icon */
                HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, smCXSmIcon, smCYSmIcon, 0);
                int iconX = g_sysConfig.smCYSizeFrame + g_sysConfig.smCXPaddedBorder;
                int iconY = g_sysConfig.smCXSizeFrame + g_sysConfig.smCXPaddedBorder;
                if (dwStyle & WS_MAXIMIZE)
                {
                    iconX += smCXEdge;
                    iconY += smCYEdge;
                }
                DrawIconEx(hdcDoubleBuffer, iconX, iconY, hIcon, smCXSmIcon, smCYSmIcon, 0, NULL, DI_NORMAL);
                DeleteObject(hIcon);
    
                /* Draw title */
                PCWSTR pszTitle = L"Default Caption";
                WCHAR szTitleBuf[MAX_PATH];
                int chLen = GetWindowText(hWnd, szTitleBuf, MAX_PATH);
                if (chLen)
                {
                    pszTitle = szTitleBuf;
                }
    
                HFONT guiFont = Win32_GetUIFont();
                LOGFONT lf;
                GetObject(guiFont, sizeof(lf), &lf);
                lf.lfWeight = FW_BOLD;
                wcscpy_s(lf.lfFaceName, sizeof(lf.lfFaceName) / sizeof(*lf.lfFaceName), L"Montserrat");
                HFONT hFont = CreateFontIndirect(&lf);
    
                HFONT hOldFont = SelectObject(hdcDoubleBuffer, hFont);
                SetBkMode(hdcDoubleBuffer, TRANSPARENT);
                SetTextColor(hdcDoubleBuffer, COLORREF_WHITE);
                RECT rcTitle;
                rcTitle.left = smCXSmIcon + smCXEdge * 6;
                rcTitle.top = smCYEdge * 2;
                if (dwStyle & WS_MAXIMIZE)
                {
                    rcTitle.top += smCYEdge * 3;
                }
                rcTitle.right = rcWindow.right - smCXEdge;
                rcTitle.bottom = smCYEdge * 2 + GetSystemMetrics(SM_CYCAPTION);
                DrawText(hdcDoubleBuffer, pszTitle, wcslen(pszTitle), &rcTitle, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_VCENTER);
    
                SelectObject(hdcDoubleBuffer, hOldFont);
                DeleteObject(hFont);
    
                UpdateCaptionButtonRects(hWnd);
                for (size_t i = 0; i < List_GetLength(g_pList); ++i)
                {
                    CaptionButtonInfo* pButton = List_Get(g_pList, i);
                    DrawCaptionButton3(hdcDoubleBuffer, pButton);
                }
    
                BitBlt(hdc, 0, 0, rcWindow.right, rcWindow.bottom, hdcDoubleBuffer, 0, 0, SRCCOPY);
    
                SelectObject(hdcDoubleBuffer, hbmDBOld);
                DeleteObject(hbmDoubleBuffer);
                DeleteDC(hdcDoubleBuffer);
            }
    
            ReleaseDC(hWnd, hdc);
        }
    
        if (hrgnTemp)
        {
            DeleteObject(hrgnTemp);
        }
    
        *bProcessed = TRUE;
        /* An application returns zero if it processes this message */
        return 0;
    }
        break;

    case WM_NCACTIVATE:
        *bProcessed = TRUE;
        break;
    }
}

#define IDM_STYLING 101

LRESULT CALLBACK DockPanel_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        HWND hCheck = CreateWindow(WC_BUTTON, L"Enable styling", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 10, 10, 200, 30, hWnd, (HMENU)IDM_STYLING, GetModuleHandle(NULL), NULL);
        Win32_ApplyUIFont(hCheck);
    }
    break;

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDM_STYLING)
        {
            g_fEnableStyle = Button_GetCheck((HWND)lParam);
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

            if (!g_fEnableStyle)
            {
                SetWindowTheme(hWnd, L"Explorer", NULL);
            }
            else {
                SetWindowTheme(hWnd, L"", L"");
            }
        }
    }
        break;

    case WM_DESTROY:
        break;
    }
    
    if (g_fEnableStyle)
    {
        BOOL bProcessed = FALSE;
        LRESULT lResult = StylingProc(hWnd, message, wParam, lParam, &bProcessed);

        if (bProcessed)
        {
            return lResult;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL DockPanel_RegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC)DockPanel_WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = szClassName;
    wcex.lpszMenuName = NULL;
    wcex.hIconSm = NULL;

    return RegisterClassEx(&wcex);
}

HWND DockPanel_CreateWindow(HINSTANCE hInstance)
{
    return CreateWindow(szClassName, L"PanitentMDIChild", WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
}
