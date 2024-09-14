#include "precomp.h"

#include "win32/window.h"
#include "win32/util.h"
#include "FloatingWindowContainer.h"
#include "resource.h"
#include "toolwndframe.h"
#include "panitentapp.h"
#include "util/assert.h"

#define HTPIN 22
#define HTMORE 23

static const WCHAR szClassName[] = L"__FloatingWindowContainer";

/* Private forward declarations */
FloatingWindowContainer* FloatingWindowContainer_Create();
void FloatingWindowContainer_Init(FloatingWindowContainer*);

void FloatingWindowContainer_PreRegister(LPWNDCLASSEX);
void FloatingWindowContainer_PreCreate(LPCREATESTRUCT);

BOOL FloatingWindowContainer_OnCreate(FloatingWindowContainer*, LPCREATESTRUCT);
void FloatingWindowContainer_OnPaint(FloatingWindowContainer*);
void FloatingWindowContainer_OnLButtonUp(FloatingWindowContainer*, int, int);
void FloatingWindowContainer_OnRButtonUp(FloatingWindowContainer*, int, int);
void FloatingWindowContainer_OnContextMenu(FloatingWindowContainer*, int, int);
void FloatingWindowContainer_OnDestroy(FloatingWindowContainer*);
void FloatingWindowContainer_OnSize(FloatingWindowContainer* pFloatingWindowContainer, UINT state, int cx, int cy);
LRESULT CALLBACK FloatingWindowContainer_UserProc(FloatingWindowContainer*, HWND hWnd, UINT, WPARAM, LPARAM);

BOOL FloatingWindowContainer_OnNCCreate(FloatingWindowContainer* pFloatingWindowContainer, LPCREATESTRUCT lpcs)
{
    HWND hWnd = pFloatingWindowContainer->base.hWnd;

    SetWindowTheme(hWnd, L"", L"");

    return TRUE;
}

int g_borderSize = 3;
int g_borderGripSize = 8;
int g_captionHeight = 14;

LRESULT FloatingWindowContainer_OnNCCalcSize(FloatingWindowContainer* pFloatingWindowContainer, BOOL bProcess, LPARAM lParam)
{
    HWND hWnd = pFloatingWindowContainer->base.hWnd;
    DWORD dwStyle = GetWindowStyle(hWnd);

    if (bProcess)
    {
        NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;

        if (dwStyle & WS_THICKFRAME)
        {
            params->rgrc[0].left += g_borderSize;
            params->rgrc[0].top += g_borderSize;
            params->rgrc[0].right -= g_borderSize;
            params->rgrc[0].bottom -= g_borderSize;
        }
        else {
            params->rgrc[0].left += 1;
            params->rgrc[0].top += 1;
            params->rgrc[0].right -= 1;
            params->rgrc[0].bottom -= 1;
        }

        if (dwStyle & WS_CAPTION)
        {
            params->rgrc[0].top += g_captionHeight + g_borderSize;
            if (!(dwStyle & WS_THICKFRAME))
            {
                params->rgrc[0].top += g_borderSize;
            }
        }        

        return WVR_ALIGNTOP;
    }

    RECT* rc = (RECT*)lParam;

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

LRESULT CaptionRightContainerHitTest()
{

}

CaptionButton g_captionButtons[] = {
    {
        {14, 14},
        GLYPH_CLOSE,
        HTCLOSE
    },
    {
        {14, 14},
        GLYPH_PIN,
        HTPIN
    },
    {
        { 24, 14 },
        GLYPH_MORE,
        HTMORE
    }
};

void FloatingWindowContainer_OnNCPaint(FloatingWindowContainer* pFloatingWindowContainer, HRGN hUpdRegion)
{
    HWND hWnd = pFloatingWindowContainer->base.hWnd;
    HDC hdc = GetWindowDC(hWnd);

    DWORD dwStyle = GetWindowStyle(hWnd);

    RECT rc = { 0 };
    GetWindowRect(hWnd, &rc);
    rc.right -= rc.left;
    rc.bottom -= rc.top;
    rc.top = rc.left = 0;

    SetDCBrushColor(hdc, Win32_HexToCOLORREF(L"#9185be"));
    SetDCPenColor(hdc, Win32_HexToCOLORREF(L"#6d648e"));
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
    
    if (dwStyle & WS_CAPTION)
    {
        HFONT guiFont = PanitentApp_GetUIFont(PanitentApp_Instance());
        HFONT hOldFont = SelectObject(hdc, guiFont);

        size_t nTextLen = GetWindowTextLength(pFloatingWindowContainer);
        if (nTextLen)
        {
            PWSTR pszTitle = (PWSTR)malloc((nTextLen + 1) * sizeof(WCHAR));
            ASSERT(pszTitle);

            int chLen = GetWindowText(pFloatingWindowContainer->hWndChild, pszTitle, MAX_PATH);

            pszTitle[nTextLen] = L'\0';

            TextOut(hdc, g_borderSize, g_borderSize, pszTitle, chLen);
            SelectObject(hdc, hOldFont);
            free(pszTitle);
        }

        int l = g_borderSize;
        for (int i = 0; i < ARRAYSIZE(g_captionButtons); ++i)
        {
            CaptionButton* pCaptionButton = &g_captionButtons[i];
            l += pCaptionButton->size.cx + 3;
        }
        l -= 3;

        int ix = g_borderSize;
        for (int i = ARRAYSIZE(g_captionButtons) - 1; i >= 0; --i)
        {
            CaptionButton* pCaptionButton = &g_captionButtons[i];
            DrawCaptionButton(pCaptionButton, hdc, rc.right - l - g_borderSize + ix, g_borderSize, pCaptionButton->size.cx, pCaptionButton->size.cy);

            ix += pCaptionButton->size.cx + 3;
        }
    }
}

LRESULT FloatingWindowContainer_OnNCHitTest(FloatingWindowContainer* pFloatingWindowContainer, int x, int y)
{
    HWND hWnd = pFloatingWindowContainer->base.hWnd;

    DWORD dwStyle = GetWindowStyle(hWnd);

    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);

    if (dwStyle & WS_CAPTION)
    {
        int ix = g_borderSize;
        for (int i = 0; i < ARRAYSIZE(g_captionButtons); i++)
        {
            CaptionButton* pCaptionButton = &g_captionButtons[i];

            if (x >= windowRect.right - ix - pCaptionButton->size.cx && x < windowRect.right - ix && y >= windowRect.top + 3 && y < windowRect.top + 3 + pCaptionButton->size.cy)
            {
                return pCaptionButton->htCommand;
            }

            ix += pCaptionButton->size.cx + 3;
        }

        if (x >= windowRect.left + g_borderSize && y >= windowRect.top + g_borderSize && x < windowRect.right - g_borderSize && y < windowRect.top + g_captionHeight + g_borderSize)
        {
            return HTCAPTION;
        }
    }

    if (dwStyle & WS_THICKFRAME)
    {
        if (!(x >= windowRect.left + g_borderSize && x < windowRect.right - g_borderSize && y >= windowRect.top + g_borderSize && y < windowRect.bottom - g_borderSize))
        {
            // Check if the cursor is on the top-left corner
            if (x >= windowRect.left && x <= windowRect.left + g_borderGripSize &&
                y >= windowRect.top && y <= windowRect.top + g_borderGripSize) {
                return HTTOPLEFT;
            }
            // Check if the cursor is on the top-right corner
            else if (x >= windowRect.right - g_borderGripSize && x <= windowRect.right &&
                y >= windowRect.top && y <= windowRect.top + g_borderGripSize) {
                return HTTOPRIGHT;
            }
            // Check if the cursor is on the bottom-left corner
            else if (x >= windowRect.left && x <= windowRect.left + g_borderGripSize &&
                y >= windowRect.bottom - g_borderGripSize && y <= windowRect.bottom) {
                return HTBOTTOMLEFT;
            }
            // Check if the cursor is on the bottom-right corner
            else if (x >= windowRect.right - g_borderGripSize && x <= windowRect.right &&
                y >= windowRect.bottom - g_borderGripSize && y <= windowRect.bottom) {
                return HTBOTTOMRIGHT;
            }
            else if (x >= windowRect.left && x <= windowRect.left + g_borderGripSize) {
                return HTLEFT;
            }
            // Check if the cursor is on the right border
            else if (x >= windowRect.right - g_borderGripSize && x <= windowRect.right) {
                return HTRIGHT;
            }
            // Check if the cursor is on the top border
            else if (y >= windowRect.top && y <= windowRect.top + g_borderGripSize) {
                return HTTOP;
            }
            // Check if the cursor is on the bottom border
            else if (y >= windowRect.bottom - g_borderGripSize && y <= windowRect.bottom) {
                return HTBOTTOM;
            }
        }
    }
    
    return HTCLIENT;
}

FloatingWindowContainer* FloatingWindowContainer_Create(struct Application* app)
{
    FloatingWindowContainer* pFloatingWindowContainer = (FloatingWindowContainer*)malloc(sizeof(FloatingWindowContainer));

    if (pFloatingWindowContainer)
    {
        memset(pFloatingWindowContainer, 0, sizeof(FloatingWindowContainer));

        FloatingWindowContainer_Init(pFloatingWindowContainer, app);
    }

    return pFloatingWindowContainer;
}

void FloatingWindowContainer_Init(FloatingWindowContainer* window, struct Application* app)
{
    Window_Init(&window->base, app);

    window->base.szClassName = szClassName;

    window->base.OnCreate = (FnWindowOnCreate)FloatingWindowContainer_OnCreate;
    window->base.OnDestroy = (FnWindowOnDestroy)FloatingWindowContainer_OnDestroy;
    window->base.OnPaint = (FnWindowOnPaint)FloatingWindowContainer_OnPaint;
    window->base.OnSize = (FnWindowOnSize)FloatingWindowContainer_OnSize;
    window->base.PreRegister = (FnWindowPreRegister)FloatingWindowContainer_PreRegister;
    window->base.PreCreate = (FnWindowPreCreate)FloatingWindowContainer_PreCreate;
    window->base.UserProc = (FnWindowUserProc)FloatingWindowContainer_UserProc;

    window->bPinned = FALSE;
}

void FloatingWindowContainer_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE);
    lpwcex->lpszClassName = szClassName;
}

BOOL FloatingWindowContainer_OnCreate(FloatingWindowContainer* window, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(lpcs);
}

void FloatingWindowContainer_OnPaint(FloatingWindowContainer* window)
{
    HWND hwnd = window->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void FloatingWindowContainer_OnLButtonUp(FloatingWindowContainer* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void FloatingWindowContainer_OnRButtonUp(FloatingWindowContainer* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void FloatingWindowContainer_OnContextMenu(FloatingWindowContainer* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void FloatingWindowContainer_OnDestroy(FloatingWindowContainer* window)
{
    UNREFERENCED_PARAMETER(window);
}


#define IDM_UNPIN 1001
#define IDM_CLOSE 1002

LRESULT FloatingWindowContainer_OnNCLButtonDown(FloatingWindowContainer* pFloatingWindowContainer, UINT hitTestVal, int x, int y)
{
    HWND hWnd = Window_GetHWND((Window *)pFloatingWindowContainer);
    DWORD dwStyle = Window_GetStyle((Window *)pFloatingWindowContainer);

    if (hitTestVal == HTCLOSE)
    {
        Window_Destroy(pFloatingWindowContainer);
        return TRUE;
    }
    else if (hitTestVal == HTPIN)
    {
        SendMessage(pFloatingWindowContainer->base.hWnd, WM_COMMAND, MAKEWPARAM(IDM_UNPIN, 0), NULL);
        return TRUE;
    }
    else if (hitTestVal == HTMORE)
    {
        HMENU hPopup = CreatePopupMenu();
        InsertMenu(hPopup, 0, 0, IDM_UNPIN, L"Unpin");
        InsertMenu(hPopup, 0, 0, IDM_CLOSE, L"Close");

        TrackPopupMenu(hPopup, 0, x, y, 0, pFloatingWindowContainer->base.hWnd, NULL);
        return TRUE;
    }
    else if (dwStyle & WS_CHILD && hitTestVal == HTCAPTION)
    {
        // SetCapture(hWnd);
        pFloatingWindowContainer->fCaptionUnpinStarted = TRUE;
        pFloatingWindowContainer->ptCaptionUnpinStartingPoint.x = x;
        pFloatingWindowContainer->ptCaptionUnpinStartingPoint.y = y;
        return TRUE;
    }

    switch (hitTestVal)
    {
    case HTCLOSE:
    case HTMINBUTTON:
    case HTMAXBUTTON:
        return TRUE;
    }

    return FALSE;
}

void FloatingWindowContainer_OnUnpinCommand(FloatingWindowContainer* pFloatingWindowContainer)
{
    if (pFloatingWindowContainer->bPinned)
    {
        HWND hWndPrevParent = SetParent(pFloatingWindowContainer->base.hWnd, NULL);
        pFloatingWindowContainer->hWndPrevParent = hWndPrevParent;
        
        DWORD dwStyle = Window_GetStyle((Window*)pFloatingWindowContainer);
        dwStyle &= ~WS_CHILD;
        dwStyle |= WS_OVERLAPPED | WS_THICKFRAME;
        Window_SetStyle((Window*)pFloatingWindowContainer, dwStyle);

        pFloatingWindowContainer->bPinned = FALSE;
        return;
    }

    ASSERT(pFloatingWindowContainer->hWndPrevParent);
    SetParent(pFloatingWindowContainer->base.hWnd, pFloatingWindowContainer->hWndPrevParent);

    DWORD dwStyle = Window_GetStyle((Window*)pFloatingWindowContainer);
    dwStyle &= ~(WS_OVERLAPPED | WS_THICKFRAME);
    dwStyle |= WS_CHILD;
    Window_SetStyle((Window*)pFloatingWindowContainer, dwStyle);

    pFloatingWindowContainer->bPinned = TRUE;    
}

void FloatingWindowContainer_OnCloseCommand(FloatingWindowContainer* pFloatingWindowContainer)
{
    Window_Destroy(pFloatingWindowContainer);
}

LRESULT FloatingWindowContainer_OnNCMouseMove(FloatingWindowContainer* pFloatingWindowContainer, UINT hitTestVal, int x, int y)
{
    if (pFloatingWindowContainer->fCaptionUnpinStarted)
    {
        const int xStart = pFloatingWindowContainer->ptCaptionUnpinStartingPoint.x;
        const int yStart = pFloatingWindowContainer->ptCaptionUnpinStartingPoint.y;

        HWND hWnd = Window_GetHWND(pFloatingWindowContainer);

        HDC hdcDesktop = GetDC(NULL);
        HDC hdcScreenshot = CreateCompatibleDC(hdcDesktop);

        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
        int width = Win32_Rect_GetWidth(&rcWindow);
        int height = Win32_Rect_GetHeight(&rcWindow);
        HBITMAP hbmScreenshot = CreateCompatibleBitmap(hdcDesktop, width, height);

        SelectObject(hdcScreenshot, hbmScreenshot);
        BOOL bResult = PrintWindow(hWnd, hdcScreenshot, 0);

        POINT pt;
        pt.x = x;
        pt.y = y;
        MapWindowPoints(NULL, hWnd, &pt, 1);

        BitBlt(hdcDesktop, x - pt.x, y + pt.y, width, height, hdcScreenshot, 0, 0, SRCCOPY);


        if (sqrt(pow(x - xStart, 2) + pow(y - yStart, 2)) >= 100)
        {
            // ReleaseCapture();
            FloatingWindowContainer_OnUnpinCommand(pFloatingWindowContainer);
            pFloatingWindowContainer->fCaptionUnpinStarted = FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

LRESULT FloatingWindowContainer_OnNCLButtonUp(FloatingWindowContainer* pFloatingWindowContainer, UINT hitTestVal, int x, int y)
{
    if (pFloatingWindowContainer->fCaptionUnpinStarted)
    {
        ReleaseCapture();
        pFloatingWindowContainer->fCaptionUnpinStarted = FALSE;
    }

    return FALSE;
}

LRESULT FloatingWindowContainer_OnCommand(FloatingWindowContainer* pFloatingWindowContainer, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_UNPIN:
    {
        FloatingWindowContainer_OnUnpinCommand(pFloatingWindowContainer);
    }
        return 0;
        break;

    case IDM_CLOSE:
    {
        FloatingWindowContainer_OnCloseCommand(pFloatingWindowContainer);
    }
        return 0;
        break;
    }

    return DefWindowProc(pFloatingWindowContainer->base.hWnd, WM_COMMAND, wParam, lParam);
}

LRESULT FloatingWindowContainer_OnNCActivate(FloatingWindowContainer* pFloatingWindowContainer, BOOL fActive)
{
    DWORD dwStyle = Window_GetStyle((Window *)pFloatingWindowContainer);

    // Prevent drawing default 5px gray thick frame
    if (dwStyle & WS_THICKFRAME)
    {
        return TRUE;
    }

    return FALSE;
}

void FloatingWindowContainer_OnSize(FloatingWindowContainer* pFloatingWindowContainer, UINT state, int cx, int cy)
{
    if (pFloatingWindowContainer->hWndChild && IsWindow(pFloatingWindowContainer->hWndChild))
    {
        SetWindowPos(pFloatingWindowContainer->hWndChild, NULL, 0, 0, cx, cy, SWP_NOREDRAW);
    }
}

LRESULT CALLBACK FloatingWindowContainer_UserProc(FloatingWindowContainer* window, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_NCCREATE:
        return FloatingWindowContainer_OnNCCreate(window, (LPCREATESTRUCT)lParam);
        break;

    case WM_NCPAINT:
        FloatingWindowContainer_OnNCPaint(window, (HRGN)wParam);
        return 0;
        break;

    case WM_NCCALCSIZE:
        return FloatingWindowContainer_OnNCCalcSize(window, (BOOL)wParam ? TRUE : FALSE, lParam);
        break;

    case WM_NCHITTEST:
        return FloatingWindowContainer_OnNCHitTest(window, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_NCMOUSEMOVE:
    {
        BOOL bProcessed = (BOOL)FloatingWindowContainer_OnNCMouseMove(window, (UINT)wParam, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        if (bProcessed)
        {
            return 0;
        }
    }
    break;

    case WM_NCLBUTTONDOWN:
    {
        BOOL bProcessed = (BOOL)FloatingWindowContainer_OnNCLButtonDown(window, (UINT)wParam, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        if (bProcessed)
        {
            return 0;
        }
    }
        break;

    case WM_NCLBUTTONUP:
    {
        BOOL bProcessed = (BOOL)FloatingWindowContainer_OnNCLButtonUp(window, (UINT)wParam, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        if (bProcessed)
        {
            return 0;
        }
    }
    break;

    case WM_NCACTIVATE:
    {
        BOOL bProcessed = (BOOL)FloatingWindowContainer_OnNCActivate(window, (BOOL)wParam);
        if (bProcessed)
        {
            return TRUE;
        }
    }
        break;

    case WM_COMMAND:
        return FloatingWindowContainer_OnCommand(window, wParam, lParam);
        break;

    case WM_RBUTTONUP:
        FloatingWindowContainer_OnRButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONUP:
        FloatingWindowContainer_OnLButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        FloatingWindowContainer_OnContextMenu(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    }

    return Window_UserProcDefault(window, hWnd, message, wParam, lParam);
}

void FloatingWindowContainer_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"FloatingWindowContainer";
    lpcs->style = WS_CAPTION | WS_OVERLAPPED | WS_THICKFRAME | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

void FloatingWindowContainer_PinWindow(FloatingWindowContainer* pFloatingWindowContainer, HWND hWndChild)
{
    if (hWndChild && IsWindow(hWndChild))
    {
        HWND hWnd = Window_GetHWND((Window *)pFloatingWindowContainer);
        
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        HWND hWndPrevParent = SetParent(hWndChild, hWnd);
        pFloatingWindowContainer->hWndPrevParent = hWndPrevParent;

        DWORD dwStyle = GetWindowStyle(hWndChild);
        dwStyle &= ~(WS_CAPTION | WS_THICKFRAME);
        dwStyle |= WS_CHILD;
        SetWindowLongPtr(hWndChild, GWL_STYLE, dwStyle);
        
        pFloatingWindowContainer->hWndChild = hWndChild;
        pFloatingWindowContainer->bPinned = TRUE;

        SetWindowPos(hWndChild, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        UpdateWindow(hWnd);
    }
}
