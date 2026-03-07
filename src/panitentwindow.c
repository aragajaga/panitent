#include "precomp.h"

#include "win32/window.h"

#include "dockhost.h"
#include "panitentwindow.h"
#include "toolwndframe.h"

#include "history.h"
#include "resource.h"
#include "log.h"
#include "aboutbox.h"
#include "log_window.h"
#include "new.h"
#include "propgriddialog.h"

#include "win32/util.h"

#include <Uxtheme.h>

#include "panitentapp.h"
#include "settings.h"
#include <tchar.h>
#include "rbhashmapviz.h"

static const WCHAR szClassName[] = L"__PanitentWindow";
static const int kMainFrameBorderSize = 3;
static const int kMainFrameGripSize = 8;
static const int kMainCaptionHeight = 32;
static const int kMainCaptionButtonSize = 14;
static const int kMainCaptionButtonSpacing = 3;
static const int kMainCaptionIconMarginX = 8;
static const int kMainCaptionIconTextGap = 8;

/* Private forward declarations */
PanitentWindow* PanitentWindow_Create();
void PanitentWindow_Init(PanitentWindow* pPanitentWindow);

void PanitentWindow_PreRegister(LPWNDCLASSEX);
void PanitentWindow_PreCreate(LPCREATESTRUCT);
void PanitentWindow_SetUseStandardFrame(PanitentWindow* pPanitentWindow, BOOL fUseStandardFrame);

BOOL PanitentWindow_OnCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs);
void PanitentWindow_PostCreate(PanitentWindow* pPanitentWindow);
void PanitentWindow_OnPaint(PanitentWindow* pPanitentWindow);
BOOL PanitentWindow_OnClose(PanitentWindow* pPanitentWindow);
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
LRESULT PanitentWindow_OnNCMouseMove(PanitentWindow* pPanitentWindow, UINT hitTestVal, int x, int y);
LRESULT PanitentWindow_OnNCLButtonDown(PanitentWindow* pPanitentWindow, UINT hitTestVal, int x, int y);
LRESULT PanitentWindow_OnNCLButtonUp(PanitentWindow* pPanitentWindow, UINT hitTestVal, int x, int y);
LRESULT PanitentWindow_OnNCActivate(PanitentWindow* pPanitentWindow, BOOL fActive);
static void PanitentWindow_InvalidateNcFrame(PanitentWindow* pPanitentWindow);
static void PanitentWindow_GetCaptionMetrics(PanitentWindow* pPanitentWindow, CaptionFrameMetrics* pMetrics);
static int PanitentWindow_BuildCaptionButtons(PanitentWindow* pPanitentWindow, CaptionButton* pButtons, int cButtons);
static BOOL PanitentWindow_BuildCaptionLayout(PanitentWindow* pPanitentWindow, CaptionFrameLayout* pLayout);
static BOOL PanitentWindow_GetCaptionIconRect(PanitentWindow* pPanitentWindow, const CaptionFrameLayout* pLayout, RECT* pRect);
static int PanitentWindow_HitTestCaptionButtonAtScreenPoint(PanitentWindow* pPanitentWindow, int x, int y);
static void PanitentWindow_SetCaptionButtonHot(PanitentWindow* pPanitentWindow, int nHotButton);
static void PanitentWindow_SetCaptionButtonPressed(PanitentWindow* pPanitentWindow, int nPressedButton);
static int PanitentWindow_GetResizeBorderThickness(HWND hWnd);
static LRESULT PanitentWindow_HitTestResizeBorder(HWND hWnd, int x, int y);
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
    pPanitentWindow->base.OnClose = (FnWindowOnClose)PanitentWindow_OnClose;
    pPanitentWindow->base.OnDestroy = (FnWindowOnDestroy)PanitentWindow_OnDestroy;
    pPanitentWindow->base.OnPaint = (FnWindowOnPaint)PanitentWindow_OnPaint;
    pPanitentWindow->base.OnSize = (FnWindowOnSize)PanitentWindow_OnSize;
    pPanitentWindow->base.PreRegister = (FnWindowPreRegister)PanitentWindow_PreRegister;
    pPanitentWindow->base.PreCreate = (FnWindowPreCreate)PanitentWindow_PreCreate;
    pPanitentWindow->base.UserProc = (FnWindowUserProc)PanitentWindow_UserProc;
    pPanitentWindow->base.PostCreate = (FnWindowPostCreate)PanitentWindow_PostCreate;

    pPanitentWindow->bCustomFrame = FALSE;
    pPanitentWindow->bNcTracking = FALSE;
    pPanitentWindow->nCaptionButtonHot = HTNOWHERE;
    pPanitentWindow->nCaptionButtonPressed = HTNOWHERE;
}

void PanitentWindow_SetUseStandardFrame(PanitentWindow* pPanitentWindow, BOOL fUseStandardFrame)
{
    if (!pPanitentWindow)
    {
        return;
    }

    pPanitentWindow->bCustomFrame = fUseStandardFrame ? FALSE : TRUE;
    pPanitentWindow->bNcTracking = FALSE;
    pPanitentWindow->nCaptionButtonHot = HTNOWHERE;
    pPanitentWindow->nCaptionButtonPressed = HTNOWHERE;

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    if (!hWnd)
    {
        return;
    }

    if (fUseStandardFrame)
    {
        SetWindowTheme(hWnd, NULL, NULL);
    }
    else
    {
        SetWindowTheme(hWnd, L"", L"");
    }

    SetWindowPos(
        hWnd,
        NULL,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN);
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
    UNREFERENCED_PARAMETER(pPanitentWindow);
    PostQuitMessage(0);
}

BOOL PanitentWindow_OnClose(PanitentWindow* pPanitentWindow)
{
    PanitentApp* pPanitentApp = PanitentApp_Instance();
    PNTSETTINGS* pSettings = PanitentApp_GetSettings(pPanitentApp);
    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);

    if (pSettings && hWnd && pSettings->bRememberWindowPos)
    {
        WINDOWPLACEMENT wp = { 0 };
        wp.length = sizeof(WINDOWPLACEMENT);
        if (GetWindowPlacement(hWnd, &wp))
        {
            RECT rcNormal = wp.rcNormalPosition;
            pSettings->x = rcNormal.left;
            pSettings->y = rcNormal.top;
            pSettings->width = max(1, rcNormal.right - rcNormal.left);
            pSettings->height = max(1, rcNormal.bottom - rcNormal.top);
        }
    }

    if (pSettings)
    {
        Panitent_WriteSettings(pSettings);
    }

    return FALSE;
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
    UNREFERENCED_PARAMETER(state);
    UNREFERENCED_PARAMETER(hwndActDeact);
    UNREFERENCED_PARAMETER(fMinimized);
    PanitentWindow_InvalidateNcFrame(pPanitentWindow);
}

BOOL PanitentWindow_OnNCCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(lpcs);

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    SetWindowTheme(hWnd, L"", L"");
    return TRUE;
}

static void PanitentWindow_InvalidateNcFrame(PanitentWindow* pPanitentWindow)
{
    if (!pPanitentWindow)
    {
        return;
    }

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    if (!hWnd || !IsWindow(hWnd))
    {
        return;
    }

    RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
}

static void PanitentWindow_GetCaptionMetrics(PanitentWindow* pPanitentWindow, CaptionFrameMetrics* pMetrics)
{
    if (!pMetrics)
    {
        return;
    }

    UNREFERENCED_PARAMETER(pPanitentWindow);

    int iconWidth = GetSystemMetrics(SM_CXSMICON);
    pMetrics->borderSize = kMainFrameBorderSize;
    pMetrics->captionHeight = kMainCaptionHeight;
    pMetrics->buttonSpacing = kMainCaptionButtonSpacing;
    pMetrics->textPaddingLeft = kMainCaptionIconMarginX + iconWidth + kMainCaptionIconTextGap;
    pMetrics->textPaddingRight = kMainFrameBorderSize;
    pMetrics->textPaddingY = 0;
}

static int PanitentWindow_BuildCaptionButtons(PanitentWindow* pPanitentWindow, CaptionButton* pButtons, int cButtons)
{
    if (!pPanitentWindow || !pButtons || cButtons < 3)
    {
        return 0;
    }

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    BOOL fZoomed = hWnd && IsZoomed(hWnd);

    pButtons[0] = (CaptionButton){ (SIZE){ kMainCaptionButtonSize, kMainCaptionButtonSize }, CAPTION_GLYPH_CLOSE_TILE, HTCLOSE };
    pButtons[1] = (CaptionButton){ (SIZE){ kMainCaptionButtonSize, kMainCaptionButtonSize }, fZoomed ? CAPTION_GLYPH_RESTORE_TILE : CAPTION_GLYPH_MAXIMIZE_TILE, HTMAXBUTTON };
    pButtons[2] = (CaptionButton){ (SIZE){ kMainCaptionButtonSize, kMainCaptionButtonSize }, CAPTION_GLYPH_MINIMIZE_TILE, HTMINBUTTON };
    return 3;
}

static BOOL PanitentWindow_BuildCaptionLayout(PanitentWindow* pPanitentWindow, CaptionFrameLayout* pLayout)
{
    if (!pPanitentWindow || !pLayout)
    {
        return FALSE;
    }

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);
    rcWindow.right -= rcWindow.left;
    rcWindow.bottom -= rcWindow.top;
    rcWindow.left = 0;
    rcWindow.top = 0;

    CaptionFrameMetrics metrics = { 0 };
    CaptionButton buttons[3] = { 0 };
    PanitentWindow_GetCaptionMetrics(pPanitentWindow, &metrics);
    return CaptionFrame_BuildLayout(
        &rcWindow,
        &metrics,
        buttons,
        PanitentWindow_BuildCaptionButtons(pPanitentWindow, buttons, ARRAYSIZE(buttons)),
        pLayout);
}

static BOOL PanitentWindow_GetCaptionIconRect(PanitentWindow* pPanitentWindow, const CaptionFrameLayout* pLayout, RECT* pRect)
{
    if (!pPanitentWindow || !pLayout || !pRect)
    {
        return FALSE;
    }

    int iconWidth = GetSystemMetrics(SM_CXSMICON);
    int iconHeight = GetSystemMetrics(SM_CYSMICON);
    if (iconWidth <= 0 || iconHeight <= 0 || IsRectEmpty(&pLayout->rcCaption))
    {
        return FALSE;
    }

    RECT rcIcon = { 0 };
    rcIcon.left = pLayout->rcCaption.left + kMainCaptionIconMarginX;
    rcIcon.top = pLayout->rcCaption.top + ((pLayout->rcCaption.bottom - pLayout->rcCaption.top) - iconHeight) / 2;
    rcIcon.right = rcIcon.left + iconWidth;
    rcIcon.bottom = rcIcon.top + iconHeight;
    *pRect = rcIcon;
    return TRUE;
}

static int PanitentWindow_HitTestCaptionButtonAtScreenPoint(PanitentWindow* pPanitentWindow, int x, int y)
{
    CaptionFrameLayout layout = { 0 };
    if (!PanitentWindow_BuildCaptionLayout(pPanitentWindow, &layout))
    {
        return HTNOWHERE;
    }

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);

    POINT ptLocal = { x - rcWindow.left, y - rcWindow.top };
    return CaptionFrame_HitTestButton(&layout, ptLocal);
}

static void PanitentWindow_SetCaptionButtonHot(PanitentWindow* pPanitentWindow, int nHotButton)
{
    if (!pPanitentWindow || pPanitentWindow->nCaptionButtonHot == nHotButton)
    {
        return;
    }

    pPanitentWindow->nCaptionButtonHot = nHotButton;
    PanitentWindow_InvalidateNcFrame(pPanitentWindow);
}

static void PanitentWindow_SetCaptionButtonPressed(PanitentWindow* pPanitentWindow, int nPressedButton)
{
    if (!pPanitentWindow || pPanitentWindow->nCaptionButtonPressed == nPressedButton)
    {
        return;
    }

    pPanitentWindow->nCaptionButtonPressed = nPressedButton;
    PanitentWindow_InvalidateNcFrame(pPanitentWindow);
}

static int PanitentWindow_GetResizeBorderThickness(HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);

    int frameX = GetSystemMetrics(SM_CXSIZEFRAME);
    int frameY = GetSystemMetrics(SM_CYSIZEFRAME);
    int padded = GetSystemMetrics(SM_CXPADDEDBORDER);
    int border = max(frameX, frameY) + padded;
    return max(kMainFrameGripSize, border);
}

static LRESULT PanitentWindow_HitTestResizeBorder(HWND hWnd, int x, int y)
{
    if (!hWnd || !IsWindow(hWnd) || IsZoomed(hWnd))
    {
        return HTNOWHERE;
    }

    DWORD dwStyle = GetWindowStyle(hWnd);
    if (!(dwStyle & WS_THICKFRAME))
    {
        return HTNOWHERE;
    }

    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);

    int border = PanitentWindow_GetResizeBorderThickness(hWnd);
    BOOL onLeft = x >= rcWindow.left && x < rcWindow.left + border;
    BOOL onRight = x < rcWindow.right && x >= rcWindow.right - border;
    BOOL onTop = y >= rcWindow.top && y < rcWindow.top + border;
    BOOL onBottom = y < rcWindow.bottom && y >= rcWindow.bottom - border;

    if (onTop && onLeft)
    {
        return HTTOPLEFT;
    }
    if (onTop && onRight)
    {
        return HTTOPRIGHT;
    }
    if (onBottom && onLeft)
    {
        return HTBOTTOMLEFT;
    }
    if (onBottom && onRight)
    {
        return HTBOTTOMRIGHT;
    }
    if (onLeft)
    {
        return HTLEFT;
    }
    if (onRight)
    {
        return HTRIGHT;
    }
    if (onTop)
    {
        return HTTOP;
    }
    if (onBottom)
    {
        return HTBOTTOM;
    }

    return HTNOWHERE;
}

LRESULT PanitentWindow_OnNCCalcSize(PanitentWindow* pPanitentWindow, BOOL fCalcValidRects, NCCALCSIZE_PARAMS* lpcsp)
{
    RECT* pRect = fCalcValidRects ? &lpcsp->rgrc[0] : (RECT*)lpcsp;
    DWORD dwStyle = GetWindowStyle(Window_GetHWND((Window*)pPanitentWindow));

    pRect->left += kMainFrameBorderSize;
    pRect->top += kMainFrameBorderSize;
    pRect->right -= kMainFrameBorderSize;
    pRect->bottom -= kMainFrameBorderSize;

    if (dwStyle & WS_CAPTION)
    {
        pRect->top += kMainCaptionHeight + kMainFrameBorderSize;
    }

    return fCalcValidRects ? WVR_REDRAW : 0;
}

LRESULT PanitentWindow_OnNCHitTest(PanitentWindow* pPanitentWindow, int x, int y)
{
    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);

    CaptionFrameLayout layout = { 0 };
    if (PanitentWindow_BuildCaptionLayout(pPanitentWindow, &layout))
    {
        POINT ptLocal = { x - rcWindow.left, y - rcWindow.top };
        int nButtonHit = CaptionFrame_HitTestButton(&layout, ptLocal);
        if (nButtonHit != HTNOWHERE)
        {
            return nButtonHit;
        }

        RECT rcIcon = { 0 };
        if (PanitentWindow_GetCaptionIconRect(pPanitentWindow, &layout, &rcIcon) && PtInRect(&rcIcon, ptLocal))
        {
            return HTSYSMENU;
        }
    }

    {
        LRESULT resizeHit = PanitentWindow_HitTestResizeBorder(hWnd, x, y);
        if (resizeHit != HTNOWHERE)
        {
            return resizeHit;
        }
    }

    if (PanitentWindow_BuildCaptionLayout(pPanitentWindow, &layout))
    {
        POINT ptLocal = { x - rcWindow.left, y - rcWindow.top };
        if (PtInRect(&layout.rcCaption, ptLocal))
        {
            return HTCAPTION;
        }
    }

    return HTCLIENT;
}

LRESULT PanitentWindow_OnNCPaint(PanitentWindow* pPanitentWindow, HRGN hrgn)
{
    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    HDC hdc = GetWindowDC(hWnd);
    if (!hdc)
    {
        return 0;
    }

    RECT rcWindowScreen = { 0 };
    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindowScreen);
    rcWindow.right = rcWindowScreen.right - rcWindowScreen.left;
    rcWindow.bottom = rcWindowScreen.bottom - rcWindowScreen.top;
    rcWindow.left = 0;
    rcWindow.top = 0;

    HRGN hFrameRgn = CreateRectRgn(0, 0, rcWindow.right, rcWindow.bottom);
    if (hFrameRgn)
    {
        if (hrgn && hrgn != (HRGN)1)
        {
            HRGN hUpdateRgn = CreateRectRgn(0, 0, 0, 0);
            if (hUpdateRgn)
            {
                if (CombineRgn(hUpdateRgn, hrgn, NULL, RGN_COPY) != ERROR)
                {
                    CombineRgn(hFrameRgn, hFrameRgn, hUpdateRgn, RGN_AND);
                }
                DeleteObject(hUpdateRgn);
            }
        }

        RECT rcClient = { 0 };
        POINT ptClient = { 0, 0 };
        if (GetClientRect(hWnd, &rcClient) && ClientToScreen(hWnd, &ptClient))
        {
            OffsetRect(&rcClient, ptClient.x - rcWindowScreen.left, ptClient.y - rcWindowScreen.top);
            HRGN hClientRgn = CreateRectRgnIndirect(&rcClient);
            if (hClientRgn)
            {
                CombineRgn(hFrameRgn, hFrameRgn, hClientRgn, RGN_DIFF);
                DeleteObject(hClientRgn);
            }
        }

        SelectClipRgn(hdc, hFrameRgn);
        DeleteObject(hFrameRgn);
    }

    CaptionFramePalette palette = { 0 };
    palette.frameFill = Win32_HexToCOLORREF(L"#9185be");
    palette.border = Win32_HexToCOLORREF(L"#6d648e");
    palette.captionFill = Win32_HexToCOLORREF(L"#9185be");
    palette.text = RGB(255, 255, 255);

    CaptionFrameLayout layout = { 0 };
    if (PanitentWindow_BuildCaptionLayout(pPanitentWindow, &layout))
    {
        WCHAR szTitle[MAX_PATH] = L"";
        GetWindowTextW(hWnd, szTitle, ARRAYSIZE(szTitle));

        CaptionFrame_DrawStateful(
            hdc,
            &layout,
            &palette,
            szTitle,
            PanitentApp_GetUIFont(PanitentApp_Instance()),
            pPanitentWindow->nCaptionButtonHot,
            pPanitentWindow->nCaptionButtonPressed);

        RECT rcIcon = { 0 };
        if (PanitentWindow_GetCaptionIconRect(pPanitentWindow, &layout, &rcIcon))
        {
            HICON hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
            if (!hIcon)
            {
                hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
            }
            if (!hIcon)
            {
                hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
            }
            if (hIcon)
            {
                DrawIconEx(
                    hdc,
                    rcIcon.left,
                    rcIcon.top,
                    hIcon,
                    rcIcon.right - rcIcon.left,
                    rcIcon.bottom - rcIcon.top,
                    0,
                    NULL,
                    DI_NORMAL);
            }
        }
    }

    SelectClipRgn(hdc, NULL);
    ReleaseDC(hWnd, hdc);
    return 0;
}

LRESULT PanitentWindow_OnNCMouseMove(PanitentWindow* pPanitentWindow, UINT hitTestVal, int x, int y)
{
    UNREFERENCED_PARAMETER(hitTestVal);

    if (!pPanitentWindow->bNcTracking)
    {
        TRACKMOUSEEVENT tme = { 0 };
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_NONCLIENT | TME_LEAVE;
        tme.hwndTrack = Window_GetHWND((Window*)pPanitentWindow);
        if (TrackMouseEvent(&tme))
        {
            pPanitentWindow->bNcTracking = TRUE;
        }
    }

    PanitentWindow_SetCaptionButtonHot(
        pPanitentWindow,
        PanitentWindow_HitTestCaptionButtonAtScreenPoint(pPanitentWindow, x, y));
    return FALSE;
}

LRESULT PanitentWindow_OnNCLButtonDown(PanitentWindow* pPanitentWindow, UINT hitTestVal, int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (hitTestVal == HTCLOSE || hitTestVal == HTMAXBUTTON || hitTestVal == HTMINBUTTON)
    {
        PanitentWindow_SetCaptionButtonHot(pPanitentWindow, (int)hitTestVal);
        PanitentWindow_SetCaptionButtonPressed(pPanitentWindow, (int)hitTestVal);
        return TRUE;
    }

    return FALSE;
}

LRESULT PanitentWindow_OnNCLButtonUp(PanitentWindow* pPanitentWindow, UINT hitTestVal, int x, int y)
{
    UNREFERENCED_PARAMETER(hitTestVal);

    int nPressedButton = pPanitentWindow->nCaptionButtonPressed;
    if (nPressedButton == HTNOWHERE)
    {
        return FALSE;
    }

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    PanitentWindow_SetCaptionButtonPressed(pPanitentWindow, HTNOWHERE);

    int nHotButton = PanitentWindow_HitTestCaptionButtonAtScreenPoint(pPanitentWindow, x, y);
    PanitentWindow_SetCaptionButtonHot(pPanitentWindow, nHotButton);
    if (nHotButton != nPressedButton)
    {
        return TRUE;
    }

    switch (nPressedButton)
    {
    case HTCLOSE:
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        return TRUE;

    case HTMAXBUTTON:
        PostMessage(hWnd, WM_SYSCOMMAND, IsZoomed(hWnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
        return TRUE;

    case HTMINBUTTON:
        PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        return TRUE;
    }

    return FALSE;
}

LRESULT PanitentWindow_OnNCActivate(PanitentWindow* pPanitentWindow, BOOL fActive)
{
    UNREFERENCED_PARAMETER(fActive);
    PanitentWindow_InvalidateNcFrame(pPanitentWindow);
    return TRUE;
}

LRESULT PanitentWindow_OnCommand(PanitentWindow* pPanitentWindow, WPARAM wParam, LPARAM lParam)
{
    PanitentApp* pPanitentApp = PanitentApp_Instance();
    AppCmd_Execute(&pPanitentApp->m_appCmd, LOWORD(wParam), pPanitentApp);
    return 0;
}

LRESULT CALLBACK PanitentWindow_UserProc(PanitentWindow* pPanitentWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_RBUTTONUP:
        PanitentWindow_OnRButtonUp(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;

    case WM_LBUTTONUP:
        PanitentWindow_OnLButtonUp(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;

    case WM_CONTEXTMENU:
        PanitentWindow_OnContextMenu(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_COMMAND:
        return PanitentWindow_OnCommand(pPanitentWindow, wParam, lParam);
    }

    if (pPanitentWindow->bCustomFrame)
    {
        switch (message)
        {
        case WM_NCCREATE:
            return PanitentWindow_OnNCCreate(pPanitentWindow, (LPCREATESTRUCT)lParam);

        case WM_NCPAINT:
            return PanitentWindow_OnNCPaint(pPanitentWindow, (HRGN)(wParam));

        case WM_NCCALCSIZE:
            return PanitentWindow_OnNCCalcSize(pPanitentWindow, (BOOL)(wParam), (NCCALCSIZE_PARAMS*)(lParam));

        case WM_NCHITTEST:
            return PanitentWindow_OnNCHitTest(pPanitentWindow, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));

        case WM_NCMOUSEMOVE:
        {
            BOOL bProcessed = (BOOL)PanitentWindow_OnNCMouseMove(pPanitentWindow, (UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            if (bProcessed)
            {
                return 0;
            }
        }
            break;

        case WM_NCMOUSELEAVE:
            pPanitentWindow->bNcTracking = FALSE;
            PanitentWindow_SetCaptionButtonHot(pPanitentWindow, HTNOWHERE);
            PanitentWindow_SetCaptionButtonPressed(pPanitentWindow, HTNOWHERE);
            return 0;

        case WM_NCLBUTTONDOWN:
        {
            BOOL bProcessed = (BOOL)PanitentWindow_OnNCLButtonDown(pPanitentWindow, (UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            if (bProcessed)
            {
                return 0;
            }
        }
            break;

        case WM_NCLBUTTONUP:
        {
            BOOL bProcessed = (BOOL)PanitentWindow_OnNCLButtonUp(pPanitentWindow, (UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            if (bProcessed)
            {
                return 0;
            }
        }
            break;

        case WM_CAPTURECHANGED:
            PanitentWindow_SetCaptionButtonPressed(pPanitentWindow, HTNOWHERE);
            return 0;

        case WM_NCACTIVATE:
            return PanitentWindow_OnNCActivate(pPanitentWindow, (BOOL)wParam);

        case WM_ACTIVATE:
            PanitentWindow_OnActivate(pPanitentWindow, (UINT)LOWORD(wParam), (HWND)(lParam), (BOOL)HIWORD(wParam));
            break;
        }
    }

    return Window_UserProcDefault((Window*)pPanitentWindow, hWnd, message, wParam, lParam);
}

void PanitentWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    PNTSETTINGS* pSettings = PanitentApp_GetSettings(PanitentApp_Instance());
    PanitentWindow* pPanitentWindow = PanitentApp_GetWindow(PanitentApp_Instance());

    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Panit.ent";
    lpcs->style = WS_THICKFRAME | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = CW_USEDEFAULT;
    lpcs->cy = CW_USEDEFAULT;

    if (pPanitentWindow && pSettings)
    {
        PanitentWindow_SetUseStandardFrame(pPanitentWindow, pSettings->bUseStandardWindowFrame);
    }

    if (pSettings && pSettings->bRememberWindowPos &&
        pSettings->width > 0 && pSettings->height > 0)
    {
        lpcs->x = pSettings->x;
        lpcs->y = pSettings->y;
        lpcs->cx = pSettings->width;
        lpcs->cy = pSettings->height;
    }
}

BOOL PanitentWindow_OnCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(lpcs);

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);

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
    Window_GetClientRect((Window*)pDockHostWindow, &rcDockHost);

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
