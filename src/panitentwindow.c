#include "precomp.h"

#include "win32/window.h"

#include "dockhost.h"
#include "panitentwindow.h"

#include "history.h"
#include "resource.h"
#include "log.h"
#include "aboutbox.h"
#include "log_window.h"
#include "new.h"
#include "propgriddialog.h"

#include "win32/util.h"

#include <dwmapi.h>
#include <Uxtheme.h>
#include <Vssym32.h>

#include "panitentapp.h"
#include "msthemeloader.h"
#include <tchar.h>
#include "rbhashmapviz.h"

static const WCHAR szClassName[] = L"__PanitentWindow";

/* Private forward declarations */
void PanitentWindow_PaintCustomCaption(PanitentWindow* pPanitentWindow, HDC hdc);

PanitentWindow* PanitentWindow_Create();
void PanitentWindow_Init(PanitentWindow* pPanitentWindow);

void PanitentWindow_PreRegister(LPWNDCLASSEX);
void PanitentWindow_PreCreate(LPCREATESTRUCT);

BOOL PanitentWindow_OnCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs);
void PanitentWindow_PostCreate(PanitentWindow* pPanitentWindow);
void PanitentWindow_OnPaint(PanitentWindow* pPanitentWindow);
void PanitentWindow_OnLButtonUp(PanitentWindow* pPanitentWindow, int x, int y);
void PanitentWindow_OnRButtonUp(PanitentWindow* pPanitentWindow, int x, int y);
void PanitentWindow_OnContextMenu(PanitentWindow* pPanitentWindow, int x, int y);
void PanitentWindow_OnDestroy(PanitentWindow* pPanitentWindow);
void PanitentWindow_OnSize(PanitentWindow* pPanitentWindow, UINT state, int cx, int cy);
void PanitentWindow_OnActivate(PanitentWindow* pPanitentWindow, UINT state, HWND hwndActDeact, BOOL fMinimized);
BOOL PanitentWindow_OnNCCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs);
LRESULT PanitentWindow_OnNCCalcSize(PanitentWindow* pPanitentWindow, BOOL fCalcValidRects, NCCALCSIZE_PARAMS* lpcsp);
LRESULT PanitentWindow_OnNCHitTest(PanitentWindow* pPanitentWindow, int x, int y);
LRESULT PanitentWindow_OnNCPaint(PanitentWindow* pPanitentWindow, HRGN hrgn);
LRESULT CALLBACK PanitentWindow_UserProc(PanitentWindow* pPanitentWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

PanitentWindow* PanitentWindow_Create()
{
    PanitentWindow* pPanitentWindow = malloc(sizeof(PanitentWindow));

    if (pPanitentWindow)
    {
        memset(pPanitentWindow, 0, sizeof(PanitentWindow));
        PanitentWindow_Init(pPanitentWindow);
    }

    return pPanitentWindow;
}

void PanitentWindow_Init(PanitentWindow* pPanitentWindow)
{
    Window_Init(&pPanitentWindow->base);

    pPanitentWindow->base.szClassName = szClassName;

    pPanitentWindow->base.OnCreate = (FnWindowOnCreate)PanitentWindow_OnCreate;
    pPanitentWindow->base.OnDestroy = (FnWindowOnDestroy)PanitentWindow_OnDestroy;
    pPanitentWindow->base.OnPaint = (FnWindowOnPaint)PanitentWindow_OnPaint;
    pPanitentWindow->base.OnSize = (FnWindowOnSize)PanitentWindow_OnSize;
    pPanitentWindow->base.PreRegister = (FnWindowPreRegister)PanitentWindow_PreRegister;
    pPanitentWindow->base.PreCreate = (FnWindowPreCreate)PanitentWindow_PreCreate;
    pPanitentWindow->base.UserProc = (FnWindowUserProc)PanitentWindow_UserProc;
    pPanitentWindow->base.PostCreate = (FnWindowPostCreate)PanitentWindow_PostCreate;

    pPanitentWindow->fCallDWP = FALSE;
    pPanitentWindow->bCustomFrame = FALSE;

    pPanitentWindow->m_pMSTheme = MSTheme_Create(L"C:\\Users\\User\\Desktop\\luna.dll");
}

void PanitentWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
    lpwcex->lpszMenuName = MAKEINTRESOURCE(IDC_MAINMENU);
}

void PanitentWindow_OnPaint(PanitentWindow* window)
{
    HWND hwnd = window->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    // PanitentWindow_PaintCustomCaption(window, hdc);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void PanitentWindow_OnLButtonUp(PanitentWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void PanitentWindow_OnRButtonUp(PanitentWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void PanitentWindow_OnContextMenu(PanitentWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void PanitentWindow_OnDestroy(PanitentWindow* pPanitentWindow)
{
    MSTheme_Destroy(pPanitentWindow->m_pMSTheme);
}

void PanitentWindow_OnSize(PanitentWindow* pPanitentWindow, UINT state, int cx, int cy)
{
    if (pPanitentWindow->m_pDockHostWindow)
    {
        SetWindowPos(Window_GetHWND((Window *)pPanitentWindow->m_pDockHostWindow), NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

void PanitentWindow_OnActivate(PanitentWindow* pPanitentWindow, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    /*
    HWND hWnd = Window_GetHWND(pPanitentWindow);

    MARGINS margins;
    margins.cxLeftWidth = 0;
    margins.cxRightWidth = 0;
    margins.cyBottomHeight = 0;
    margins.cyTopHeight = 64;
    DwmExtendFrameIntoClientArea(hWnd, &margins);
    */
}

BOOL PanitentWindow_OnNCCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs)
{
    HWND hWnd = Window_GetHWND(pPanitentWindow);

    SetWindowTheme(hWnd, L"", L"");

    return TRUE;
}

LRESULT PanitentWindow_OnNCCalcSize(PanitentWindow* pPanitentWindow, BOOL fCalcValidRects, NCCALCSIZE_PARAMS* lpcsp)
{
    
    HWND hWnd = Window_GetHWND(pPanitentWindow);
    /*
    if (fCalcValidRects)
    {
        return 0;
    }

    return Window_UserProcDefault(pPanitentWindow, hWnd, WM_NCCALCSIZE, (WPARAM)fCalcValidRects, (LPARAM)lpcsp);
    */


    int g_borderSize = 4;
    int g_captionHeight = 32;
    

    DWORD dwStyle = GetWindowStyle(hWnd);

    if (fCalcValidRects)
    {
        lpcsp->rgrc[0].left += g_borderSize;
        lpcsp->rgrc[0].top += g_borderSize;
        lpcsp->rgrc[0].right -= g_borderSize;
        lpcsp->rgrc[0].bottom -= g_borderSize;

        if (dwStyle & WS_CAPTION)
        {
            lpcsp->rgrc[0].top += g_captionHeight + g_borderSize;
            if (!(dwStyle & WS_THICKFRAME))
            {
                lpcsp->rgrc[0].top += g_borderSize;
            }
        }

        return WVR_ALIGNTOP;
    }

    PRECT rc = (PRECT)lpcsp;

    rc->left += g_borderSize;
    rc->top += g_borderSize;

    if (dwStyle & WS_CAPTION)
    {
        rc->top += g_captionHeight + g_borderSize;
        if (!(dwStyle & WS_THICKFRAME))
        {
            rc->top += g_borderSize;
        }
    }

    rc->right -= g_borderSize;
    rc->bottom -= g_borderSize;

    return 0;
}

LRESULT HitTestNCA(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    // Get the point coordinates for the hit test.
    POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    // Get the window rectangle.
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);

    // Get the frame rectangle, adjusted for the style without a caption.
    RECT rcFrame = { 0 };
    AdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL);

    // Determine if the hit test is for resizing. Default middle (1,1).
    USHORT uRow = 1;
    USHORT uCol = 1;
    BOOL fOnResizeBorder = FALSE;

    // Determine if the point is at the top or bottom of the window.
    if (ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + 64)
    {
        fOnResizeBorder = (ptMouse.y < (rcWindow.top - rcFrame.top));
        uRow = 0;
    }
    else if (ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - 0)
    {
        uRow = 2;
    }

    // Determine if the point is at the left or right of the window.
    if (ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + 0)
    {
        uCol = 0; // left side
    }
    else if (ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - 0)
    {
        uCol = 2; // right side
    }

    // Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
    LRESULT hitTests[3][3] =
    {
        { HTTOPLEFT,    fOnResizeBorder ? HTTOP : HTCAPTION,    HTTOPRIGHT },
        { HTLEFT,       HTNOWHERE,     HTRIGHT },
        { HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT },
    };

    return hitTests[uRow][uCol];
}

LRESULT PanitentWindow_OnNCHitTest(PanitentWindow* pPanitentWindow, int x, int y)
{
    HWND hWnd = Window_GetHWND(pPanitentWindow);

    LRESULT lRet = HitTestNCA(hWnd, NULL, MAKELPARAM(x, y));

    if (lRet != HTNOWHERE)
    {
        pPanitentWindow->fCallDWP = FALSE;
    }

    return lRet;
}

LRESULT PanitentWindow_OnNCPaint(PanitentWindow* pPanitentWindow, HRGN hrgn)
{
    HWND hWnd = Window_GetHWND(pPanitentWindow);

    HDC hdc = GetWindowDC(hWnd);

    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);
    rcWindow.right -= rcWindow.left;
    rcWindow.bottom -= rcWindow.top;
    rcWindow.top = rcWindow.left = 0;

    SetDCBrushColor(hdc, Win32_HexToCOLORREF(L"#9185be"));
    SetDCPenColor(hdc, Win32_HexToCOLORREF(L"#6d648e"));
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));
    Rectangle(hdc, rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);

    SetDCBrushColor(hdc, Win32_HexToCOLORREF(L"#FF8800"));
    Rectangle(hdc, 4, 0, 96, 24);

    SelectObject(hdc, Win32_GetUIFont());

    int smIconWidth = GetSystemMetrics(SM_CXSMICON);
    int smIconHeight = GetSystemMetrics(SM_CYSMICON);
    HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, smIconWidth, smIconHeight, 0);
    DrawIconEx(hdc, 14, 2, hIcon, smIconWidth, smIconHeight, 0, NULL, DI_NORMAL);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    TextOut(hdc, 34, 4, L"Panit.ent", 9);


    /* Draw caption buttons */
    SetDCBrushColor(hdc, Win32_HexToCOLORREF(L"#9185be"));

    Rectangle(hdc, rcWindow.right - 48 - 8, rcWindow.top, rcWindow.right - 8, rcWindow.top + 24);

    MSTheme_DrawAnything(pPanitentWindow->m_pMSTheme, hdc, _T("BLUE_CLOSEBUTTON_BMP"), rcWindow.right - 21 - 8, rcWindow.top + 8);
    MSTheme_DrawAnything(pPanitentWindow->m_pMSTheme, hdc, _T("BLUE_CLOSEGLYPH_BMP"), rcWindow.right - 21 - 8 + 4, rcWindow.top + 8 + 4);

    MSTheme_DrawAnything(pPanitentWindow->m_pMSTheme, hdc, _T("BLUE_CAPTIONBUTTON_BMP"), rcWindow.right - 21 - 21 - 8 - 2, rcWindow.top + 8);
    MSTheme_DrawAnything(pPanitentWindow->m_pMSTheme, hdc, _T("BLUE_MAXIMIZEGLYPH_BMP"), rcWindow.right - 21 - 21 - 8 + 4 - 2, rcWindow.top + 8 + 4);

    MSTheme_DrawAnything(pPanitentWindow->m_pMSTheme, hdc, _T("BLUE_CAPTIONBUTTON_BMP"), rcWindow.right - 21 - 21 - 21 - 8 - 4, rcWindow.top + 8);
    MSTheme_DrawAnything(pPanitentWindow->m_pMSTheme, hdc, _T("BLUE_MINIMIZEGLYPH_BMP"), rcWindow.right - 21 - 21 - 21 - 8 + 4 - 4, rcWindow.top + 8 + 4);

    return 0;
}

LRESULT PanitentWindow_OnCommand(PanitentWindow* pPanitentWindow, WPARAM wParam, LPARAM lParam)
{
    PanitentApp* pPanitentApp = PanitentApp_Instance();
    AppCmd_Execute(&pPanitentApp->m_appCmd, LOWORD(wParam), pPanitentApp);
    return 0;
}

LRESULT CALLBACK PanitentWindow_UserProc(PanitentWindow* pPanitentWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0;
    BOOL fCallDWP = !DwmDefWindowProc(hWnd, message, wParam, lParam, &lRet);

    switch (message) {
    
    case WM_RBUTTONUP:
        PanitentWindow_OnRButtonUp(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONUP:
        PanitentWindow_OnLButtonUp(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        PanitentWindow_OnContextMenu(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        /* pass to defwndproc, no return */
        break;

    case WM_COMMAND:
        return PanitentWindow_OnCommand(pPanitentWindow, wParam, lParam);
        break;
    }

    if (pPanitentWindow->bCustomFrame)
    {
        switch (message)
        {
        case WM_NCCREATE:
            return FloatingWindowContainer_OnNCCreate(pPanitentWindow, (LPCREATESTRUCT)lParam);
            break;

        case WM_NCPAINT:
            return PanitentWindow_OnNCPaint(pPanitentWindow, (HRGN)(wParam));
            break;

        case WM_NCCALCSIZE:
            return PanitentWindow_OnNCCalcSize(pPanitentWindow, (BOOL)(wParam), (NCCALCSIZE_PARAMS*)(lParam));
            break;

        case WM_NCHITTEST:
            return PanitentWindow_OnNCHitTest(pPanitentWindow, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
            break;

        case WM_ACTIVATE:
            PanitentWindow_OnActivate(pPanitentWindow, (UINT)LOWORD(wParam), (HWND)(lParam), (BOOL)HIWORD(wParam));
            break;
        }
    }

    return Window_UserProcDefault(pPanitentWindow, hWnd, message, wParam, lParam);
}

void PanitentWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Panit.ent";
    lpcs->style = WS_THICKFRAME | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = CW_USEDEFAULT;
    lpcs->cy = CW_USEDEFAULT;
}

BOOL PanitentWindow_OnCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs)
{
    HWND hWnd = Window_GetHWND(pPanitentWindow);

    RECT rcClient;
    GetWindowRect(hWnd, &rcClient);

    /* Inform the application of the frame change */
    SetWindowPos(hWnd, NULL, rcClient.left, rcClient.top, Win32_Rect_GetWidth(&rcClient), Win32_Rect_GetHeight(&rcClient), SWP_FRAMECHANGED);

    return TRUE;
}

void PanitentWindow_PostCreate(PanitentWindow* pPanitentWindow)
{
    HWND hWnd = Window_GetHWND((Window *)pPanitentWindow);

    /* Allocate dockhost window data structure */
    DockHostWindow* pDockHostWindow = DockHostWindow_Create(PanitentApp_Instance());
    Window_CreateWindow((Window *)pDockHostWindow, hWnd);
    pPanitentWindow->m_pDockHostWindow = pDockHostWindow;

    /* Get dockhost client rect*/
    RECT rcDockHost = { 0 };
    Window_GetClientRect(pDockHostWindow, &rcDockHost);

    /* Create and assign root dock node */
    TreeNode* pRoot = (TreeNode*)calloc(1, sizeof(TreeNode));
    DockData* pDockData = (DockData*)calloc(1, sizeof(DockData));
    pRoot->data = (void*)pDockData;
    ((DockData*)pRoot->data)->rc = rcDockHost;

    DockHostWindow_SetRoot(pDockHostWindow, pRoot);

    // Panitent_DockHostInit(pDockHostWindow, pRoot);
    PanitentApp_DockHostInit(PanitentApp_Instance(), pDockHostWindow, pRoot);

    Win32_FitChild((Window*)pDockHostWindow, (Window*)pPanitentWindow);
}

// Paint the title on the custom frame.
void PanitentWindow_PaintCustomCaption(PanitentWindow* pPanitentWindow, HDC hdc)
{
    HWND hWnd = Window_GetHWND(pPanitentWindow);

    RECT rcClient;
    Window_GetClientRect(pPanitentWindow, &rcClient);

    HTHEME hTheme = OpenThemeData(NULL, L"CompositedWindow::Window");
    if (hTheme)
    {
        HDC hdcPaint = CreateCompatibleDC(hdc);

        if (hdcPaint)
        {
            int cx = Win32_Rect_GetWidth(&rcClient);
            int cy = Win32_Rect_GetHeight(&rcClient);

            // Define the BITMAPINFO structure used to draw text.
            // Note that biHeight is negative. This is done because
            // DrawThemeTextEx() needs the bitmap to be in top-to-bottom
            // order.
            BITMAPINFO dib = { 0 };
            dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            dib.bmiHeader.biWidth = cx;
            dib.bmiHeader.biHeight = -cy;
            dib.bmiHeader.biPlanes = 1;
            dib.bmiHeader.biBitCount = 32;
            dib.bmiHeader.biCompression = BI_RGB;

            HBITMAP hbm = CreateDIBSection(hdc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);
            if (hbm)
            {
                HBITMAP hbmOld = (HBITMAP)SelectObject(hdcPaint, hbm);

                // Setup the theme drawing options.
                DTTOPTS dttOpts = { sizeof(DTTOPTS) };
                dttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE;
                dttOpts.iGlowSize = 15;

                // Select a font.
                LOGFONT lgFont;
                HFONT hFontOld = NULL;
                if (SUCCEEDED(GetThemeSysFont(hTheme, TMT_CAPTIONFONT, &lgFont)))
                {
                    lgFont.lfWeight = FW_BOLD;

                    HFONT hFont = CreateFontIndirect(&lgFont);
                    hFontOld = (HFONT)SelectObject(hdcPaint, hFont);
                }

                WCHAR szTitle[256] = L"";
                GetWindowText(hWnd, szTitle, 256);
                

                // Draw the title.
                RECT rcPaint = rcClient;
                rcPaint.top += 8;
                rcPaint.right -= 125;
                rcPaint.left += 8;
                rcPaint.bottom = 50;
                DrawThemeTextEx(hTheme,
                    hdcPaint,
                    0, 0,
                    szTitle,
                    -1,
                    DT_LEFT | DT_WORD_ELLIPSIS,
                    &rcPaint,
                    &dttOpts);

                // Blit text to the frame.
                BitBlt(hdc, 0, 0, cx, cy, hdcPaint, 0, 0, SRCCOPY);

                SelectObject(hdcPaint, hbmOld);
                if (hFontOld)
                {
                    SelectObject(hdcPaint, hFontOld);
                }
                DeleteObject(hbm);
            }
            DeleteDC(hdcPaint);
        }
        CloseThemeData(hTheme);
    }
}
