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
static const WCHAR szMenuBarClassName[] = L"__PanitentMenuBar";
static const int kMainFrameBorderSize = 3;
static const int kMainFrameGripSize = 8;
static const int kMainCaptionHeight = 32;
static const int kMainCaptionButtonWidth = 44;
static const int kMainCaptionButtonHeight = 32;
static const int kMainCaptionButtonSpacing = 0;
static const int kMainCaptionIconMarginX = 8;
static const int kMainCaptionIconTextGap = 8;
static const int kMainMenuBarHeight = 24;
static const int kMainMenuItemPaddingX = 10;
static const int kMainMenuItemPaddingY = 2;
static const int kMainMenuItemGap = 2;
static const UINT_PTR kMainMenuSwitchTimerId = 1;
static const UINT kMainMenuSwitchTimerIntervalMs = 30;

/* Private forward declarations */
PanitentWindow* PanitentWindow_Create();
void PanitentWindow_Init(PanitentWindow* pPanitentWindow);

void PanitentWindow_PreRegister(LPWNDCLASSEX);
void PanitentWindow_PreCreate(LPCREATESTRUCT);
void PanitentWindow_SetUseStandardFrame(PanitentWindow* pPanitentWindow, BOOL fUseStandardFrame);
void PanitentWindow_SetCompactMenuBar(PanitentWindow* pPanitentWindow, BOOL fCompactMenuBar);

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
static int PanitentWindow_GetFrameInset(HWND hWnd, DWORD dwStyle);
static LRESULT PanitentWindow_HitTestResizeBorder(HWND hWnd, int x, int y);
static void PanitentWindow_EnsureMenuBarClass(void);
static void PanitentWindow_UpdateMenuPresentation(PanitentWindow* pPanitentWindow);
static void PanitentWindow_LayoutChildren(PanitentWindow* pPanitentWindow, int cx, int cy);
static int PanitentWindow_GetMenuBarHeight(PanitentWindow* pPanitentWindow);
static void PanitentWindow_InvalidateMenuPresentation(PanitentWindow* pPanitentWindow);
static BOOL PanitentWindow_MenuBar_GetItemText(HMENU hMenu, int index, WCHAR* pszText, int cchText);
static int PanitentWindow_Menu_GetTotalWidth(PanitentWindow* pPanitentWindow, HDC hdc);
static BOOL PanitentWindow_Menu_GetItemRectInStrip(PanitentWindow* pPanitentWindow, const RECT* pRectStrip, HDC hdc, int index, RECT* pRect);
static int PanitentWindow_Menu_HitTestInStrip(PanitentWindow* pPanitentWindow, const RECT* pRectStrip, HDC hdc, POINT ptClient);
static BOOL PanitentWindow_GetCompactMenuStripRect(PanitentWindow* pPanitentWindow, const CaptionFrameLayout* pLayout, HDC hdc, RECT* pRectStrip, RECT* pRectTitle);
static int PanitentWindow_HitTestCompactMenuItemAtScreenPoint(PanitentWindow* pPanitentWindow, int x, int y);
static void PanitentWindow_Menu_DrawItems(HDC hdc, PanitentWindow* pPanitentWindow, const RECT* pRectStrip);
static BOOL PanitentWindow_MenuBar_GetItemRect(PanitentWindow* pPanitentWindow, HWND hWndMenuBar, int index, RECT* pRect);
static int PanitentWindow_MenuBar_HitTest(PanitentWindow* pPanitentWindow, HWND hWndMenuBar, POINT ptClient);
static void PanitentWindow_MenuBar_SetHotItem(PanitentWindow* pPanitentWindow, int index);
static void PanitentWindow_MenuBar_SetOpenItem(PanitentWindow* pPanitentWindow, int index);
static int PanitentWindow_HitTestTopLevelMenuAtScreenPoint(PanitentWindow* pPanitentWindow, POINT ptScreen);
static void PanitentWindow_UpdateHotMenuFromCursor(PanitentWindow* pPanitentWindow);
static void PanitentWindow_MenuPopupLoop(PanitentWindow* pPanitentWindow, int index);
static void PanitentWindow_MenuBar_ShowPopup(PanitentWindow* pPanitentWindow, HWND hWndMenuBar, int index);
static void PanitentWindow_CompactMenu_ShowPopup(PanitentWindow* pPanitentWindow, int index);
static void PanitentWindow_MenuBar_OnPaint(PanitentWindow* pPanitentWindow, HWND hWndMenuBar);
static LRESULT CALLBACK PanitentWindow_MenuBarProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
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
    pPanitentWindow->bCompactMenuBar = FALSE;
    pPanitentWindow->bNcTracking = FALSE;
    pPanitentWindow->bMenuTracking = FALSE;
    pPanitentWindow->bMenuPopupTracking = FALSE;
    pPanitentWindow->nCaptionButtonHot = HTNOWHERE;
    pPanitentWindow->nCaptionButtonPressed = HTNOWHERE;
    pPanitentWindow->nHotMenuItem = -1;
    pPanitentWindow->nOpenMenuItem = -1;
    pPanitentWindow->nPendingMenuItem = -1;
}

void PanitentWindow_SetUseStandardFrame(PanitentWindow* pPanitentWindow, BOOL fUseStandardFrame)
{
    if (!pPanitentWindow)
    {
        return;
    }

    pPanitentWindow->bCustomFrame = fUseStandardFrame ? FALSE : TRUE;
    pPanitentWindow->bNcTracking = FALSE;
    pPanitentWindow->bMenuTracking = FALSE;
    pPanitentWindow->bMenuPopupTracking = FALSE;
    pPanitentWindow->nCaptionButtonHot = HTNOWHERE;
    pPanitentWindow->nCaptionButtonPressed = HTNOWHERE;
    pPanitentWindow->nHotMenuItem = -1;
    pPanitentWindow->nOpenMenuItem = -1;
    pPanitentWindow->nPendingMenuItem = -1;

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
    PanitentWindow_UpdateMenuPresentation(pPanitentWindow);
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN);
}

void PanitentWindow_SetCompactMenuBar(PanitentWindow* pPanitentWindow, BOOL fCompactMenuBar)
{
    if (!pPanitentWindow)
    {
        return;
    }

    pPanitentWindow->bCompactMenuBar = fCompactMenuBar ? TRUE : FALSE;
    pPanitentWindow->bMenuTracking = FALSE;
    pPanitentWindow->bMenuPopupTracking = FALSE;
    pPanitentWindow->nHotMenuItem = -1;
    pPanitentWindow->nOpenMenuItem = -1;
    pPanitentWindow->nPendingMenuItem = -1;

    if (Window_GetHWND((Window*)pPanitentWindow))
    {
        PanitentWindow_UpdateMenuPresentation(pPanitentWindow);
        RedrawWindow(
            Window_GetHWND((Window*)pPanitentWindow),
            NULL,
            NULL,
            RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN);
    }
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
    if (pPanitentWindow && pPanitentWindow->hWndMenuBar && IsWindow(pPanitentWindow->hWndMenuBar))
    {
        DestroyWindow(pPanitentWindow->hWndMenuBar);
        pPanitentWindow->hWndMenuBar = NULL;
    }

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
    UNREFERENCED_PARAMETER(state);
    PanitentWindow_LayoutChildren(pPanitentWindow, cx, cy);
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

    HWND hWnd = pPanitentWindow ? Window_GetHWND((Window*)pPanitentWindow) : NULL;
    DWORD dwStyle = hWnd ? GetWindowStyle(hWnd) : 0;

    int iconWidth = GetSystemMetrics(SM_CXSMICON);
    pMetrics->borderSize = hWnd ? PanitentWindow_GetFrameInset(hWnd, dwStyle) : kMainFrameBorderSize;
    pMetrics->captionHeight = kMainCaptionHeight;
    pMetrics->buttonSpacing = kMainCaptionButtonSpacing;
    pMetrics->textPaddingLeft = kMainCaptionIconMarginX + iconWidth + kMainCaptionIconTextGap;
    pMetrics->textPaddingRight = pMetrics->borderSize;
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

    pButtons[0] = (CaptionButton){ (SIZE){ kMainCaptionButtonWidth, kMainCaptionButtonHeight }, CAPTION_GLYPH_CLOSE_TILE, HTCLOSE };
    pButtons[1] = (CaptionButton){ (SIZE){ kMainCaptionButtonWidth, kMainCaptionButtonHeight }, fZoomed ? CAPTION_GLYPH_RESTORE_TILE : CAPTION_GLYPH_MAXIMIZE_TILE, HTMAXBUTTON };
    pButtons[2] = (CaptionButton){ (SIZE){ kMainCaptionButtonWidth, kMainCaptionButtonHeight }, CAPTION_GLYPH_MINIMIZE_TILE, HTMINBUTTON };
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

static int PanitentWindow_GetFrameInset(HWND hWnd, DWORD dwStyle)
{
    if (dwStyle & WS_THICKFRAME)
    {
        if (IsZoomed(hWnd))
        {
            return PanitentWindow_GetResizeBorderThickness(hWnd);
        }

        return kMainFrameBorderSize;
    }

    return 1;
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
    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    RECT* pRect = fCalcValidRects ? &lpcsp->rgrc[0] : (RECT*)lpcsp;
    DWORD dwStyle = GetWindowStyle(hWnd);
    int frameInset = PanitentWindow_GetFrameInset(hWnd, dwStyle);

    pRect->left += frameInset;
    pRect->top += frameInset;
    pRect->right -= frameInset;
    pRect->bottom -= frameInset;

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

        if (pPanitentWindow->bCompactMenuBar &&
            PanitentWindow_HitTestCompactMenuItemAtScreenPoint(pPanitentWindow, x, y) >= 0)
        {
            return HTMENU;
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

        if (pPanitentWindow->bCompactMenuBar)
        {
            RECT rcTitle = layout.rcCaptionText;
            RECT rcMenuStrip = { 0 };
            HFONT hOldFont = (HFONT)SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));

            CaptionFrame_DrawStateful(
                hdc,
                &layout,
                &palette,
                L"",
                PanitentApp_GetUIFont(PanitentApp_Instance()),
                pPanitentWindow->nCaptionButtonHot,
                pPanitentWindow->nCaptionButtonPressed);

            if (PanitentWindow_GetCompactMenuStripRect(pPanitentWindow, &layout, hdc, &rcMenuStrip, &rcTitle))
            {
                PanitentWindow_Menu_DrawItems(hdc, pPanitentWindow, &rcMenuStrip);
            }

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, palette.text);
            DrawTextW(hdc, szTitle, -1, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            if (hOldFont)
            {
                SelectObject(hdc, hOldFont);
            }
        }
        else {
            CaptionFrame_DrawStateful(
                hdc,
                &layout,
                &palette,
                szTitle,
                PanitentApp_GetUIFont(PanitentApp_Instance()),
                pPanitentWindow->nCaptionButtonHot,
                pPanitentWindow->nCaptionButtonPressed);
        }

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
    if (pPanitentWindow->bCompactMenuBar)
    {
        int index = PanitentWindow_HitTestCompactMenuItemAtScreenPoint(pPanitentWindow, x, y);
        PanitentWindow_MenuBar_SetHotItem(pPanitentWindow, index);
        if (pPanitentWindow->bMenuPopupTracking &&
            pPanitentWindow->nOpenMenuItem >= 0 &&
            index >= 0 &&
            index != pPanitentWindow->nOpenMenuItem)
        {
            pPanitentWindow->nPendingMenuItem = index;
            EndMenu();
        }
    }
    else if (pPanitentWindow->nOpenMenuItem < 0)
    {
        PanitentWindow_MenuBar_SetHotItem(pPanitentWindow, -1);
    }
    return FALSE;
}

LRESULT PanitentWindow_OnNCLButtonDown(PanitentWindow* pPanitentWindow, UINT hitTestVal, int x, int y)
{
    if (hitTestVal == HTCLOSE || hitTestVal == HTMAXBUTTON || hitTestVal == HTMINBUTTON)
    {
        PanitentWindow_SetCaptionButtonHot(pPanitentWindow, (int)hitTestVal);
        PanitentWindow_SetCaptionButtonPressed(pPanitentWindow, (int)hitTestVal);
        return TRUE;
    }

    if (pPanitentWindow->bCompactMenuBar && hitTestVal == HTMENU)
    {
        int index = PanitentWindow_HitTestCompactMenuItemAtScreenPoint(pPanitentWindow, x, y);
        if (index >= 0)
        {
            PanitentWindow_CompactMenu_ShowPopup(pPanitentWindow, index);
            return TRUE;
        }
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

static void PanitentWindow_EnsureMenuBarClass(void)
{
    WNDCLASSEXW wcex = { 0 };
    if (GetClassInfoExW(GetModuleHandle(NULL), szMenuBarClassName, &wcex))
    {
        return;
    }

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PanitentWindow_MenuBarProc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = szMenuBarClassName;
    RegisterClassExW(&wcex);
}

static int PanitentWindow_GetMenuBarHeight(PanitentWindow* pPanitentWindow)
{
    if (!pPanitentWindow ||
        !pPanitentWindow->bCustomFrame ||
        pPanitentWindow->bCompactMenuBar ||
        !pPanitentWindow->hWndMenuBar)
    {
        return 0;
    }

    return kMainMenuBarHeight;
}

static void PanitentWindow_InvalidateMenuPresentation(PanitentWindow* pPanitentWindow)
{
    if (!pPanitentWindow)
    {
        return;
    }

    if (pPanitentWindow->bCustomFrame && pPanitentWindow->bCompactMenuBar)
    {
        PanitentWindow_InvalidateNcFrame(pPanitentWindow);
        return;
    }

    if (pPanitentWindow->hWndMenuBar && IsWindow(pPanitentWindow->hWndMenuBar))
    {
        InvalidateRect(pPanitentWindow->hWndMenuBar, NULL, FALSE);
    }
}

static void PanitentWindow_LayoutChildren(PanitentWindow* pPanitentWindow, int cx, int cy)
{
    if (!pPanitentWindow)
    {
        return;
    }

    int menuHeight = PanitentWindow_GetMenuBarHeight(pPanitentWindow);
    if (pPanitentWindow->hWndMenuBar && IsWindow(pPanitentWindow->hWndMenuBar))
    {
        SetWindowPos(
            pPanitentWindow->hWndMenuBar,
            NULL,
            0,
            0,
            max(0, cx),
            menuHeight,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }

    if (pPanitentWindow->m_pDockHostWindow)
    {
        SetWindowPos(
            Window_GetHWND((Window*)pPanitentWindow->m_pDockHostWindow),
            NULL,
            0,
            menuHeight,
            max(0, cx),
            max(0, cy - menuHeight),
            SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

static void PanitentWindow_UpdateMenuPresentation(PanitentWindow* pPanitentWindow)
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

    if (!pPanitentWindow->hMainMenu)
    {
        pPanitentWindow->hMainMenu = GetMenu(hWnd);
    }

    if (pPanitentWindow->bCustomFrame)
    {
        if (GetMenu(hWnd))
        {
            SetMenu(hWnd, NULL);
            DrawMenuBar(hWnd);
        }

        if (pPanitentWindow->bCompactMenuBar)
        {
            if (pPanitentWindow->hWndMenuBar && IsWindow(pPanitentWindow->hWndMenuBar))
            {
                DestroyWindow(pPanitentWindow->hWndMenuBar);
            }
            pPanitentWindow->hWndMenuBar = NULL;
        }
        else if (!pPanitentWindow->hWndMenuBar || !IsWindow(pPanitentWindow->hWndMenuBar))
        {
            PanitentWindow_EnsureMenuBarClass();
            pPanitentWindow->hWndMenuBar = CreateWindowExW(
                0,
                szMenuBarClassName,
                L"",
                WS_CHILD | WS_VISIBLE,
                0,
                0,
                0,
                0,
                hWnd,
                NULL,
                GetModuleHandle(NULL),
                pPanitentWindow);
        }
    }
    else {
        if (pPanitentWindow->hWndMenuBar && IsWindow(pPanitentWindow->hWndMenuBar))
        {
            DestroyWindow(pPanitentWindow->hWndMenuBar);
        }
        pPanitentWindow->hWndMenuBar = NULL;

        if (pPanitentWindow->hMainMenu && GetMenu(hWnd) != pPanitentWindow->hMainMenu)
        {
            SetMenu(hWnd, pPanitentWindow->hMainMenu);
            DrawMenuBar(hWnd);
        }
    }

    {
        RECT rcClient = { 0 };
        GetClientRect(hWnd, &rcClient);
        PanitentWindow_LayoutChildren(pPanitentWindow, RECTWIDTH(&rcClient), RECTHEIGHT(&rcClient));
    }

    PanitentWindow_InvalidateMenuPresentation(pPanitentWindow);
}

static BOOL PanitentWindow_MenuBar_GetItemText(HMENU hMenu, int index, WCHAR* pszText, int cchText)
{
    if (!hMenu || !pszText || cchText <= 0)
    {
        return FALSE;
    }

    WCHAR szRaw[128] = L"";
    if (!GetMenuStringW(hMenu, (UINT)index, szRaw, ARRAYSIZE(szRaw), MF_BYPOSITION))
    {
        pszText[0] = L'\0';
        return FALSE;
    }

    int j = 0;
    for (int i = 0; szRaw[i] && j < cchText - 1; ++i)
    {
        if (szRaw[i] == L'\t')
        {
            break;
        }

        if (szRaw[i] == L'&')
        {
            if (szRaw[i + 1] == L'&' && j < cchText - 1)
            {
                pszText[j++] = L'&';
                ++i;
            }
            continue;
        }

        pszText[j++] = szRaw[i];
    }

    pszText[j] = L'\0';
    return TRUE;
}

static int PanitentWindow_Menu_GetTotalWidth(PanitentWindow* pPanitentWindow, HDC hdc)
{
    if (!pPanitentWindow || !pPanitentWindow->hMainMenu || !hdc)
    {
        return 0;
    }

    int nCount = GetMenuItemCount(pPanitentWindow->hMainMenu);
    int width = kMainMenuItemGap;
    for (int i = 0; i < nCount; ++i)
    {
        WCHAR szLabel[128] = L"";
        SIZE sizeText = { 0 };
        PanitentWindow_MenuBar_GetItemText(pPanitentWindow->hMainMenu, i, szLabel, ARRAYSIZE(szLabel));
        GetTextExtentPoint32W(hdc, szLabel, (int)wcslen(szLabel), &sizeText);
        width += sizeText.cx + kMainMenuItemPaddingX * 2 + kMainMenuItemGap;
    }

    return max(0, width - kMainMenuItemGap);
}

static BOOL PanitentWindow_Menu_GetItemRectInStrip(PanitentWindow* pPanitentWindow, const RECT* pRectStrip, HDC hdc, int index, RECT* pRect)
{
    if (!pPanitentWindow || !pPanitentWindow->hMainMenu || !pRectStrip || !hdc || !pRect)
    {
        return FALSE;
    }

    int nCount = GetMenuItemCount(pPanitentWindow->hMainMenu);
    if (index < 0 || index >= nCount)
    {
        return FALSE;
    }

    int x = pRectStrip->left + kMainMenuItemGap;
    for (int i = 0; i < nCount; ++i)
    {
        WCHAR szLabel[128] = L"";
        SIZE sizeText = { 0 };
        RECT rcItem = { 0 };

        PanitentWindow_MenuBar_GetItemText(pPanitentWindow->hMainMenu, i, szLabel, ARRAYSIZE(szLabel));
        GetTextExtentPoint32W(hdc, szLabel, (int)wcslen(szLabel), &sizeText);

        rcItem.left = x;
        rcItem.top = pRectStrip->top + kMainMenuItemPaddingY;
        rcItem.right = rcItem.left + sizeText.cx + kMainMenuItemPaddingX * 2;
        rcItem.bottom = pRectStrip->bottom - kMainMenuItemPaddingY;

        if (i == index)
        {
            *pRect = rcItem;
            return TRUE;
        }

        x = rcItem.right + kMainMenuItemGap;
    }

    return FALSE;
}

static int PanitentWindow_Menu_HitTestInStrip(PanitentWindow* pPanitentWindow, const RECT* pRectStrip, HDC hdc, POINT ptClient)
{
    if (!pPanitentWindow || !pRectStrip || !hdc)
    {
        return -1;
    }

    int nCount = GetMenuItemCount(pPanitentWindow->hMainMenu);
    for (int i = 0; i < nCount; ++i)
    {
        RECT rcItem = { 0 };
        if (PanitentWindow_Menu_GetItemRectInStrip(pPanitentWindow, pRectStrip, hdc, i, &rcItem) && PtInRect(&rcItem, ptClient))
        {
            return i;
        }
    }

    return -1;
}

static BOOL PanitentWindow_GetCompactMenuStripRect(PanitentWindow* pPanitentWindow, const CaptionFrameLayout* pLayout, HDC hdc, RECT* pRectStrip, RECT* pRectTitle)
{
    if (!pPanitentWindow || !pLayout || !hdc || !pRectStrip)
    {
        return FALSE;
    }

    RECT rcIcon = { 0 };
    RECT rcTitle = pLayout->rcCaptionText;
    int textStart = pLayout->rcCaptionText.left;
    int menuRight = pLayout->nButtons > 0 ?
        pLayout->buttonRects[pLayout->nButtons - 1].left - kMainCaptionIconTextGap :
        pLayout->rcCaption.right - kMainCaptionIconTextGap;

    if (PanitentWindow_GetCaptionIconRect(pPanitentWindow, pLayout, &rcIcon))
    {
        textStart = rcIcon.right + kMainCaptionIconTextGap;
    }

    WCHAR szTitle[MAX_PATH] = L"";
    SIZE sizeTitle = { 0 };
    GetWindowTextW(Window_GetHWND((Window*)pPanitentWindow), szTitle, ARRAYSIZE(szTitle));
    GetTextExtentPoint32W(hdc, szTitle, (int)wcslen(szTitle), &sizeTitle);

    int preferredLeft = textStart + sizeTitle.cx + 14;
    int menuWidth = PanitentWindow_Menu_GetTotalWidth(pPanitentWindow, hdc);
    int menuLeft = min(preferredLeft, menuRight - menuWidth);
    int minLeft = textStart + 8;

    if (menuWidth <= 0 || menuRight - minLeft < menuWidth)
    {
        return FALSE;
    }

    if (menuLeft < minLeft)
    {
        menuLeft = minLeft;
    }

    pRectStrip->left = menuLeft;
    pRectStrip->top = pLayout->rcCaption.top + 2;
    pRectStrip->right = menuLeft + menuWidth;
    pRectStrip->bottom = pLayout->rcCaption.bottom - 2;

    if (pRectTitle)
    {
        rcTitle.left = textStart;
        rcTitle.right = max(rcTitle.left, pRectStrip->left - 8);
        *pRectTitle = rcTitle;
    }

    return TRUE;
}

static int PanitentWindow_HitTestCompactMenuItemAtScreenPoint(PanitentWindow* pPanitentWindow, int x, int y)
{
    if (!pPanitentWindow || !pPanitentWindow->bCompactMenuBar || !pPanitentWindow->bCustomFrame)
    {
        return -1;
    }

    CaptionFrameLayout layout = { 0 };
    if (!PanitentWindow_BuildCaptionLayout(pPanitentWindow, &layout))
    {
        return -1;
    }

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);

    HDC hdc = GetWindowDC(hWnd);
    if (!hdc)
    {
        return -1;
    }

    HFONT hOldFont = (HFONT)SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));
    RECT rcStrip = { 0 };
    POINT ptLocal = { x - rcWindow.left, y - rcWindow.top };
    int index = -1;

    if (PanitentWindow_GetCompactMenuStripRect(pPanitentWindow, &layout, hdc, &rcStrip, NULL))
    {
        index = PanitentWindow_Menu_HitTestInStrip(pPanitentWindow, &rcStrip, hdc, ptLocal);
    }

    if (hOldFont)
    {
        SelectObject(hdc, hOldFont);
    }
    ReleaseDC(hWnd, hdc);
    return index;
}

static void PanitentWindow_Menu_DrawItems(HDC hdc, PanitentWindow* pPanitentWindow, const RECT* pRectStrip)
{
    if (!hdc || !pPanitentWindow || !pRectStrip || !pPanitentWindow->hMainMenu)
    {
        return;
    }

    HBRUSH hBrushHot = CreateSolidBrush(Win32_HexToCOLORREF(L"#9b8acb"));
    HBRUSH hBrushOpen = CreateSolidBrush(Win32_HexToCOLORREF(L"#a898d3"));
    int nCount = GetMenuItemCount(pPanitentWindow->hMainMenu);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));

    for (int i = 0; i < nCount; ++i)
    {
        RECT rcItem = { 0 };
        WCHAR szLabel[128] = L"";
        if (!PanitentWindow_Menu_GetItemRectInStrip(pPanitentWindow, pRectStrip, hdc, i, &rcItem))
        {
            continue;
        }

        if (i == pPanitentWindow->nOpenMenuItem)
        {
            FillRect(hdc, &rcItem, hBrushOpen);
        }
        else if (i == pPanitentWindow->nHotMenuItem)
        {
            FillRect(hdc, &rcItem, hBrushHot);
        }

        PanitentWindow_MenuBar_GetItemText(pPanitentWindow->hMainMenu, i, szLabel, ARRAYSIZE(szLabel));
        DrawTextW(hdc, szLabel, -1, &rcItem, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    }

    DeleteObject(hBrushHot);
    DeleteObject(hBrushOpen);
}

static BOOL PanitentWindow_MenuBar_GetItemRect(PanitentWindow* pPanitentWindow, HWND hWndMenuBar, int index, RECT* pRect)
{
    if (!pPanitentWindow || !pPanitentWindow->hMainMenu || !pRect || !hWndMenuBar)
    {
        return FALSE;
    }

    RECT rcClient = { 0 };
    GetClientRect(hWndMenuBar, &rcClient);

    HDC hdc = GetDC(hWndMenuBar);
    if (!hdc)
    {
        return FALSE;
    }

    HFONT hOldFont = (HFONT)SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));
    BOOL bResult = PanitentWindow_Menu_GetItemRectInStrip(pPanitentWindow, &rcClient, hdc, index, pRect);

    if (hOldFont)
    {
        SelectObject(hdc, hOldFont);
    }
    ReleaseDC(hWndMenuBar, hdc);
    return bResult;
}

static int PanitentWindow_MenuBar_HitTest(PanitentWindow* pPanitentWindow, HWND hWndMenuBar, POINT ptClient)
{
    if (!pPanitentWindow || !hWndMenuBar || !pPanitentWindow->hMainMenu)
    {
        return -1;
    }

    RECT rcClient = { 0 };
    HDC hdc = GetDC(hWndMenuBar);
    int index = -1;
    if (hdc)
    {
        HFONT hOldFont = (HFONT)SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));
        GetClientRect(hWndMenuBar, &rcClient);
        index = PanitentWindow_Menu_HitTestInStrip(pPanitentWindow, &rcClient, hdc, ptClient);
        if (hOldFont)
        {
            SelectObject(hdc, hOldFont);
        }
        ReleaseDC(hWndMenuBar, hdc);
    }

    return index;
}

static void PanitentWindow_MenuBar_SetHotItem(PanitentWindow* pPanitentWindow, int index)
{
    if (!pPanitentWindow || pPanitentWindow->nHotMenuItem == index)
    {
        return;
    }

    pPanitentWindow->nHotMenuItem = index;
    PanitentWindow_InvalidateMenuPresentation(pPanitentWindow);
}

static void PanitentWindow_MenuBar_SetOpenItem(PanitentWindow* pPanitentWindow, int index)
{
    if (!pPanitentWindow || pPanitentWindow->nOpenMenuItem == index)
    {
        return;
    }

    pPanitentWindow->nOpenMenuItem = index;
    PanitentWindow_InvalidateMenuPresentation(pPanitentWindow);
}

static int PanitentWindow_HitTestTopLevelMenuAtScreenPoint(PanitentWindow* pPanitentWindow, POINT ptScreen)
{
    if (!pPanitentWindow)
    {
        return -1;
    }

    if (pPanitentWindow->bCustomFrame && pPanitentWindow->bCompactMenuBar)
    {
        return PanitentWindow_HitTestCompactMenuItemAtScreenPoint(pPanitentWindow, ptScreen.x, ptScreen.y);
    }

    if (pPanitentWindow->hWndMenuBar && IsWindow(pPanitentWindow->hWndMenuBar))
    {
        POINT ptClient = ptScreen;
        if (ScreenToClient(pPanitentWindow->hWndMenuBar, &ptClient))
        {
            return PanitentWindow_MenuBar_HitTest(pPanitentWindow, pPanitentWindow->hWndMenuBar, ptClient);
        }
    }

    return -1;
}

static void PanitentWindow_UpdateHotMenuFromCursor(PanitentWindow* pPanitentWindow)
{
    if (!pPanitentWindow)
    {
        return;
    }

    POINT ptCursor = { 0 };
    if (!GetCursorPos(&ptCursor))
    {
        PanitentWindow_MenuBar_SetHotItem(pPanitentWindow, -1);
        return;
    }

    PanitentWindow_MenuBar_SetHotItem(
        pPanitentWindow,
        PanitentWindow_HitTestTopLevelMenuAtScreenPoint(pPanitentWindow, ptCursor));
}

static void PanitentWindow_MenuPopupLoop(PanitentWindow* pPanitentWindow, int index)
{
    if (!pPanitentWindow || !pPanitentWindow->hMainMenu || index < 0)
    {
        return;
    }

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    if (!hWnd || !IsWindow(hWnd))
    {
        return;
    }

    int currentIndex = index;
    while (currentIndex >= 0)
    {
        HMENU hSubMenu = GetSubMenu(pPanitentWindow->hMainMenu, currentIndex);
        POINT ptPopup = { 0 };
        BOOL bHasItem = FALSE;

        if (pPanitentWindow->bCustomFrame && pPanitentWindow->bCompactMenuBar)
        {
            CaptionFrameLayout layout = { 0 };
            if (hSubMenu && PanitentWindow_BuildCaptionLayout(pPanitentWindow, &layout))
            {
                HDC hdc = GetWindowDC(hWnd);
                if (hdc)
                {
                    HFONT hOldFont = (HFONT)SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));
                    RECT rcStrip = { 0 };
                    RECT rcItem = { 0 };
                    bHasItem = PanitentWindow_GetCompactMenuStripRect(pPanitentWindow, &layout, hdc, &rcStrip, NULL) &&
                        PanitentWindow_Menu_GetItemRectInStrip(pPanitentWindow, &rcStrip, hdc, currentIndex, &rcItem);
                    if (bHasItem)
                    {
                        RECT rcWindow = { 0 };
                        GetWindowRect(hWnd, &rcWindow);
                        ptPopup.x = rcWindow.left + rcItem.left;
                        ptPopup.y = rcWindow.top + rcItem.bottom;
                    }

                    if (hOldFont)
                    {
                        SelectObject(hdc, hOldFont);
                    }
                    ReleaseDC(hWnd, hdc);
                }
            }
        }
        else if (hSubMenu && pPanitentWindow->hWndMenuBar && IsWindow(pPanitentWindow->hWndMenuBar))
        {
            RECT rcItem = { 0 };
            bHasItem = PanitentWindow_MenuBar_GetItemRect(pPanitentWindow, pPanitentWindow->hWndMenuBar, currentIndex, &rcItem);
            if (bHasItem)
            {
                ptPopup.x = rcItem.left;
                ptPopup.y = rcItem.bottom;
                ClientToScreen(pPanitentWindow->hWndMenuBar, &ptPopup);
            }
        }

        if (!hSubMenu || !bHasItem)
        {
            break;
        }

        pPanitentWindow->nPendingMenuItem = -1;
        pPanitentWindow->bMenuPopupTracking = TRUE;
        PanitentWindow_MenuBar_SetOpenItem(pPanitentWindow, currentIndex);
        PanitentWindow_MenuBar_SetHotItem(pPanitentWindow, currentIndex);

        SetForegroundWindow(hWnd);
        SetTimer(hWnd, kMainMenuSwitchTimerId, kMainMenuSwitchTimerIntervalMs, NULL);
        TrackPopupMenuEx(hSubMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, ptPopup.x, ptPopup.y, hWnd, NULL);
        KillTimer(hWnd, kMainMenuSwitchTimerId);

        pPanitentWindow->bMenuPopupTracking = FALSE;
        currentIndex = pPanitentWindow->nPendingMenuItem;
        pPanitentWindow->nPendingMenuItem = -1;

        if (currentIndex < 0)
        {
            PanitentWindow_MenuBar_SetOpenItem(pPanitentWindow, -1);
            PanitentWindow_UpdateHotMenuFromCursor(pPanitentWindow);
        }
    }
}

static void PanitentWindow_MenuBar_ShowPopup(PanitentWindow* pPanitentWindow, HWND hWndMenuBar, int index)
{
    if (!pPanitentWindow || !hWndMenuBar || !pPanitentWindow->hMainMenu)
    {
        return;
    }
    PanitentWindow_MenuPopupLoop(pPanitentWindow, index);
}

static void PanitentWindow_CompactMenu_ShowPopup(PanitentWindow* pPanitentWindow, int index)
{
    if (!pPanitentWindow || !pPanitentWindow->hMainMenu || index < 0)
    {
        return;
    }
    PanitentWindow_MenuPopupLoop(pPanitentWindow, index);
}

static void PanitentWindow_MenuBar_OnPaint(PanitentWindow* pPanitentWindow, HWND hWndMenuBar)
{
    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hWndMenuBar, &ps);
    if (!hdc)
    {
        return;
    }

    RECT rcClient = { 0 };
    GetClientRect(hWndMenuBar, &rcClient);

    HBRUSH hBrushBg = CreateSolidBrush(Win32_HexToCOLORREF(L"#8578b3"));
    FillRect(hdc, &rcClient, hBrushBg);

    HFONT hOldFont = (HFONT)SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));
    PanitentWindow_Menu_DrawItems(hdc, pPanitentWindow, &rcClient);

    {
        HPEN hPen = CreatePen(PS_SOLID, 1, Win32_HexToCOLORREF(L"#6d648e"));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, 0, rcClient.bottom - 1, NULL);
        LineTo(hdc, rcClient.right, rcClient.bottom - 1);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }

    if (hOldFont)
    {
        SelectObject(hdc, hOldFont);
    }
    DeleteObject(hBrushBg);
    EndPaint(hWndMenuBar, &ps);
}

static LRESULT CALLBACK PanitentWindow_MenuBarProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PanitentWindow* pPanitentWindow = (PanitentWindow*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (message)
    {
    case WM_NCCREATE:
    {
        CREATESTRUCTW* pcs = (CREATESTRUCTW*)lParam;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams);
        return TRUE;
    }

    case WM_ERASEBKGND:
        return TRUE;

    case WM_PAINT:
        PanitentWindow_MenuBar_OnPaint(pPanitentWindow, hWnd);
        return 0;

    case WM_MOUSEMOVE:
        if (pPanitentWindow)
        {
            if (!pPanitentWindow->bMenuTracking)
            {
                TRACKMOUSEEVENT tme = { 0 };
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                if (TrackMouseEvent(&tme))
                {
                    pPanitentWindow->bMenuTracking = TRUE;
                }
            }

            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            {
                int index = PanitentWindow_MenuBar_HitTest(pPanitentWindow, hWnd, pt);
                PanitentWindow_MenuBar_SetHotItem(pPanitentWindow, index);
                if (pPanitentWindow->bMenuPopupTracking &&
                    pPanitentWindow->nOpenMenuItem >= 0 &&
                    index >= 0 &&
                    index != pPanitentWindow->nOpenMenuItem)
                {
                    pPanitentWindow->nPendingMenuItem = index;
                    EndMenu();
                }
            }
        }
        return 0;

    case WM_MOUSELEAVE:
        if (pPanitentWindow)
        {
            pPanitentWindow->bMenuTracking = FALSE;
            if (pPanitentWindow->nOpenMenuItem < 0)
            {
                PanitentWindow_MenuBar_SetHotItem(pPanitentWindow, -1);
            }
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (pPanitentWindow)
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            int index = PanitentWindow_MenuBar_HitTest(pPanitentWindow, hWnd, pt);
            if (index >= 0)
            {
                PanitentWindow_MenuBar_ShowPopup(pPanitentWindow, hWnd, index);
            }
        }
        return 0;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
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
    case WM_TIMER:
        if (wParam == kMainMenuSwitchTimerId && pPanitentWindow && pPanitentWindow->bMenuPopupTracking)
        {
            POINT ptCursor = { 0 };
            if (GetCursorPos(&ptCursor))
            {
                int index = PanitentWindow_HitTestTopLevelMenuAtScreenPoint(pPanitentWindow, ptCursor);
                if (index >= 0)
                {
                    PanitentWindow_MenuBar_SetHotItem(pPanitentWindow, index);
                    if (pPanitentWindow->nOpenMenuItem >= 0 && index != pPanitentWindow->nOpenMenuItem)
                    {
                        pPanitentWindow->nPendingMenuItem = index;
                        EndMenu();
                    }
                }
            }
            return 0;
        }
        break;

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
            if (pPanitentWindow->nOpenMenuItem < 0)
            {
                PanitentWindow_MenuBar_SetHotItem(pPanitentWindow, -1);
            }
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
        PanitentWindow_SetCompactMenuBar(pPanitentWindow, pSettings->bCompactMenuBar);
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
    pPanitentWindow->hMainMenu = GetMenu(hWnd);
    PanitentWindow_UpdateMenuPresentation(pPanitentWindow);

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

    {
        RECT rcClient = { 0 };
        GetClientRect(hWnd, &rcClient);
        PanitentWindow_LayoutChildren(pPanitentWindow, RECTWIDTH(&rcClient), RECTHEIGHT(&rcClient));
    }
}
