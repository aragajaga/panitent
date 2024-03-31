#include "precomp.h"
#include "win32/window.h"
#include "document.h"

#include "viewport.h"
#include "canvas.h"
#include "win32/util.h"
#include "tool.h"
#include "checker.h"
#include "resource.h"
#include "panitent.h"

#define IDM_VIEWPORTSETTINGS 100

Tool g_tool;

static const WCHAR szClassName[] = L"__ViewportWindow";

struct _ViewportSettings {
    BOOL doubleBuffered;
    BOOL bkgndExcludeCanvas;
    BOOL showDebugInfo;

    enum {
        BKGND_RECTANGLES,
        BKGND_REGION
    } backgroundMethod;
};

struct _ViewportSettings g_viewportSettings = {
  .doubleBuffered = TRUE,
  .bkgndExcludeCanvas = TRUE,
  .showDebugInfo = FALSE,
  .backgroundMethod = BKGND_REGION
};

INT_PTR CALLBACK ViewportSettingsDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

ViewportWindow* ViewportWindow_Create(struct Application* app);
void ViewportWindow_Init(ViewportWindow* window, struct Application* app);

static inline void ViewportWindow_DrawWindowOrdinates(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static inline void ViewportWindow_DrawCanvasOrdinates(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static inline void ViewportWindow_DrawOffsetTrace(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static inline void ViewportWindow_DrawDebugText(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static inline void ViewportWindow_DrawDebugOverlay(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);

void ViewportWindow_SetDocument(ViewportWindow* pViewportWindow, Document* document);
Document* ViewportWindow_GetDocument(ViewportWindow* pViewportWindow);
extern inline void Viewport_ResetView(ViewportWindow* pViewportWindow);
void ViewportWindow_ClientToCanvas(ViewportWindow* pViewportWindow, int x, int y, LPPOINT lpPt);
BOOL Viewport_BlitCanvas(HDC hDC, LPRECT prcView, Canvas* canvas);

void ViewportWindow_PreRegister(LPWNDCLASSEX lpwcex);
void ViewportWindow_PreCreate(LPCREATESTRUCT lpcs);

BOOL ViewportWindow_OnCreate(ViewportWindow* pViewportWindow, LPCREATESTRUCT lpcs);
void ViewportWindow_OnSize(ViewportWindow* pViewportWindow, UINT state, int cx, int cy);
void ViewportWindow_OnPaint(ViewportWindow* pViewportWindow);
LRESULT ViewportWindow_OnCommand(ViewportWindow* pViewportWindow, WPARAM wParam, LPARAM lParam);
BOOL ViewportWindow_OnKeyDown(ViewportWindow* pViewportWindow, UINT vk, int cRepeat, UINT flags);
BOOL ViewportWindow_OnKeyUp(ViewportWindow* pViewportWindow, UINT vk, int cRepeat, UINT flags);
void ViewportWindow_OnMouseMove(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void ViewportWindow_OnMouseWheel(ViewportWindow* pViewportWindow, int xPos, int yPos, int zDelta, UINT fwKeys);
void ViewportWindow_OnLButtonDown(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void ViewportWindow_OnLButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void ViewportWindow_OnRButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void ViewportWindow_OnContextMenu(ViewportWindow* pViewportWindow, HWND hwndContext, UINT xPos, UINT yPos);
void ViewportWindow_OnMButtonDown(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void ViewportWindow_OnMButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
BOOL ViewportWindow_OnEraseBkgnd(ViewportWindow* pViewportWindow, HDC hdc);
void ViewportWindow_OnDestroy(ViewportWindow* window);
LRESULT ViewportWindow_UserProc(ViewportWindow* pViewportWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

ViewportWindow* ViewportWindow_Create(struct Application* app)
{
    ViewportWindow* window = calloc(1, sizeof(ViewportWindow));

    if (window)
    {
        ViewportWindow_Init(window, app);
    }

    return window;
}

void ViewportWindow_Init(ViewportWindow* pViewportWindow, struct Application* app)
{
    Window_Init(&pViewportWindow->base, app);

    pViewportWindow->base.szClassName = szClassName;

    pViewportWindow->base.OnCreate = (FnWindowOnCreate)ViewportWindow_OnCreate;
    pViewportWindow->base.OnDestroy = (FnWindowOnDestroy)ViewportWindow_OnDestroy;
    pViewportWindow->base.OnPaint = (FnWindowOnPaint)ViewportWindow_OnPaint;
    pViewportWindow->base.OnSize = (FnWindowOnSize)ViewportWindow_OnSize;
    pViewportWindow->base.PreRegister = (FnWindowPreRegister)ViewportWindow_PreRegister;
    pViewportWindow->base.PreCreate = (FnWindowPreCreate)ViewportWindow_PreCreate;
    pViewportWindow->base.UserProc = (FnWindowUserProc)ViewportWindow_UserProc;
}

static inline void ViewportWindow_DrawWindowOrdinates(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect)
{
    HPEN hPen;
    HGDIOBJ hOldObj;
    HFONT hFont;

    hFont = GetGuiFont();

    hPen = CreatePen(PS_DASH, 1, COLORREF_RED);
    hOldObj = SelectObject(hdc, hPen);

    SetBkMode(hdc, TRANSPARENT);

    MoveToEx(hdc, 0, lpRect->bottom / 2, NULL);
    LineTo(hdc, lpRect->right, lpRect->bottom / 2);
    MoveToEx(hdc, lpRect->right / 2, 0, NULL);
    LineTo(hdc, lpRect->right / 2, lpRect->bottom);

    SelectObject(hdc, hOldObj);
    DeleteObject(hPen);

    hOldObj = SelectObject(hdc, hFont);

    SetTextColor(hdc, RGB(255, 0, 0));

    TextOut(hdc, lpRect->right / 2 + 4, lpRect->bottom / 2, L"VIEWPORT CENTER", 15);

    SelectObject(hdc, hOldObj);
}

static inline void ViewportWindow_DrawCanvasOrdinates(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect)
{
    HPEN hPen;
    HGDIOBJ hOldObj;
    LPPOINT lpptOffset;
    HFONT hFont;

    lpptOffset = &(pViewportWindow->ptOffset);

    hPen = CreatePen(PS_DASH, 1, RGB(0, 255, 0));
    hOldObj = SelectObject(hdc, hPen);
    hFont = GetGuiFont();

    SetBkMode(hdc, TRANSPARENT);

    MoveToEx(hdc, 0, lpRect->bottom / 2 + lpptOffset->y, NULL);
    LineTo(hdc, lpRect->right, lpRect->bottom / 2 + lpptOffset->y);
    MoveToEx(hdc, lpRect->right / 2 + lpptOffset->x, 0, NULL);
    LineTo(hdc, lpRect->right / 2 + lpptOffset->x, lpRect->bottom);

    SelectObject(hdc, hOldObj);
    DeleteObject(hPen);

    hOldObj = SelectObject(hdc, hFont);

    WCHAR szOffset[80] = L"";
    StringCchPrintf(szOffset, 80, L"x: %d, y: %d", pViewportWindow->ptOffset.x, pViewportWindow->ptOffset.y);

    size_t lenLabel = wcslen(szOffset);

    SetTextColor(hdc, RGB(0, 255, 0));

    TextOut(hdc, lpRect->right / 2 + lpptOffset->x + 4, lpRect->bottom / 2 + lpptOffset->y, szOffset, (int)lenLabel);

    SelectObject(hdc, hOldObj);
}

static inline void ViewportWindow_DrawOffsetTrace(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect)
{
    HPEN hPen;
    HGDIOBJ hOldObj;
    LPPOINT lpptOffset;

    lpptOffset = &(pViewportWindow->ptOffset);

    hPen = CreatePen(PS_DASH, 1, RGB(0, 0, 255));
    hOldObj = SelectObject(hdc, hPen);

    SetBkMode(hdc, TRANSPARENT);

    MoveToEx(hdc, lpRect->right / 2, lpRect->bottom / 2, NULL);
    LineTo(hdc, lpRect->right / 2 + lpptOffset->x, lpRect->bottom / 2 + lpptOffset->y);

    SelectObject(hdc, hOldObj);
    DeleteObject(hPen);
}

static inline void ViewportWindow_DrawDebugText(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect)
{
    HFONT hFont;
    HGDIOBJ hOldObj;
    WCHAR szDebugString[256];
    WCHAR szFormat[256] = L"Debug shown\n"
        L"Double Buffer: %s\n"
        L"Background exclude canvas: %s\n"
        L"Background method: %s\n"
        L"Offset x: %d, y: %d\n"
        L"Scale: %f\n"
        L"Document set: %s\n"
        L"Document dimensions width: %d, height %d";

    hFont = GetGuiFont();

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 6001 )
#endif  /* _MSC_VER */

    StringCchPrintf(szDebugString, 256, szFormat,
        g_viewportSettings.doubleBuffered ? L"ON" : L"OFF",
        g_viewportSettings.bkgndExcludeCanvas ? L"ON" : L"OFF",
        g_viewportSettings.bkgndExcludeCanvas ?
        g_viewportSettings.backgroundMethod ?
        L"GDI region" : L"FillRect" : L"N/A",
        pViewportWindow->ptOffset.x, pViewportWindow->ptOffset.y,
        pViewportWindow->fZoom,
        pViewportWindow->document ? L"TRUE" : L"FALSE",
        pViewportWindow->document ? pViewportWindow->document->canvas->width : 0,
        pViewportWindow->document ? pViewportWindow->document->canvas->height : 0);

#ifdef _MSC_VER
#pragma warning( pop )
#endif  /* _MSC_VER */

    size_t len = wcslen(szDebugString);

    SetTextColor(hdc, RGB(0, 0, 0));

    hOldObj = SelectObject(hdc, hFont);
    DrawText(hdc, szDebugString, (int)len, lpRect, DT_BOTTOM | DT_RIGHT);
    SelectObject(hdc, hOldObj);
}

static inline void ViewportWindow_DrawDebugOverlay(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect)
{
    ViewportWindow_DrawWindowOrdinates(pViewportWindow, hdc, lpRect);
    ViewportWindow_DrawCanvasOrdinates(pViewportWindow, hdc, lpRect);
    ViewportWindow_DrawOffsetTrace(pViewportWindow, hdc, lpRect);
    ViewportWindow_DrawDebugText(pViewportWindow, hdc, lpRect);
}

void ViewportWindow_SetDocument(ViewportWindow* pViewportWindow, Document* document)
{
    assert(pViewportWindow);
    assert(document);

    pViewportWindow->document = document;

    Viewport_ResetView(pViewportWindow);
    Window_Invalidate((Window *)pViewportWindow);
}

Document* ViewportWindow_GetDocument(ViewportWindow* pViewportWindow)
{
    return pViewportWindow->document;
}

extern inline void Viewport_ResetView(ViewportWindow* pViewportWindow)
{
    pViewportWindow->ptOffset.x = 0;
    pViewportWindow->ptOffset.y = 0;
    pViewportWindow->fZoom = 1.f;
}

void ViewportWindow_ClientToCanvas(ViewportWindow* pViewportWindow, int x, int y, LPPOINT lpPt)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    Document* document = ViewportWindow_GetDocument(pViewportWindow);
    assert(document);
    if (!document)
        return;

    Canvas* canvas = Document_GetCanvas(document);
    assert(canvas);
    if (!canvas)
        return;

    RECT clientRc = { 0 };
    GetClientRect(hWnd, &clientRc);

    /* TODO: Try to get rid of intermediate negative value. Juggle around with
       'x' and .ptOffset' positions. */
    lpPt->x = (LONG)(((float)x - ((float)clientRc.right - (float)canvas->width * pViewportWindow->fZoom) / 2.f -
        (float)pViewportWindow->ptOffset.x) / pViewportWindow->fZoom);
    lpPt->y = (LONG)(((float)y - ((float)clientRc.bottom - (float)canvas->height * pViewportWindow->fZoom) / 2.f -
        (float)pViewportWindow->ptOffset.y) / pViewportWindow->fZoom);
}

BOOL Viewport_BlitCanvas(HDC hDC, LPRECT prcView, Canvas* canvas)
{
    assert(canvas);
    if (!canvas)
        return FALSE;

    HDC hBmDC = CreateCompatibleDC(hDC);

    unsigned char* pData = malloc(canvas->buffer_size);
    if (!pData)
        return FALSE;

    ZeroMemory(pData, canvas->buffer_size);
    memcpy(pData, canvas->buffer, canvas->buffer_size);

    /* Premultiply alpha */
    for (size_t y = 0; y < (size_t)canvas->height; y++)
    {
        for (size_t x = 0; x < (size_t)canvas->width; x++)
        {
            uint8_t* pixel = pData + (x + y * canvas->width) * 4;

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 6385 )
#endif  /* _MSC_VER */

            float factor = (float)(pixel[3]) / 255.f;

#ifdef _MSC_VER
#pragma warning( pop )
#endif  /* _MSC_VER */

            pixel[0] *= (uint8_t)factor;
            pixel[1] *= (uint8_t)factor;
            pixel[2] *= (uint8_t)factor;

        }
    }

    HBITMAP hBitmap = CreateBitmap(canvas->width, canvas->height, 1,
        sizeof(uint32_t) * 8, pData);

    SelectObject(hBmDC, hBitmap);

    BLENDFUNCTION blendFunc = {
      .BlendOp = AC_SRC_OVER,
      .BlendFlags = 0,
      .SourceConstantAlpha = 0xFF,
      .AlphaFormat = AC_SRC_ALPHA
    };

    AlphaBlend(hDC,
        prcView->left,
        prcView->top,
        prcView->right - prcView->left,
        prcView->bottom - prcView->top,
        hBmDC, 0, 0,
        canvas->width, canvas->height, blendFunc);

    DeleteObject(hBitmap);
    DeleteDC(hBmDC);

    free(pData);
    return TRUE;
}

void ViewportWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE);
    lpwcex->lpszClassName = szClassName;
}

void ViewportWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"ViewportWindow";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

BOOL ViewportWindow_OnCreate(ViewportWindow* pViewportWindow, LPCREATESTRUCT lpcs)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    Viewport_ResetView(pViewportWindow);

    CheckerConfig checkerCfg;
    checkerCfg.tileSize = 16;
    checkerCfg.color[0] = 0x00CCCCCC;
    checkerCfg.color[1] = 0x00FFFFFF;

    HDC hdc = GetDC(hWnd);
    pViewportWindow->hbrChecker = CreateChecker(&checkerCfg, hdc);
    ReleaseDC(hWnd, hdc);

    return TRUE;
}

void ViewportWindow_OnSize(ViewportWindow* pViewportWindow, UINT state, int cx, int cy)
{
    if (ViewportWindow_GetDocument(pViewportWindow))
    {
        Window_Invalidate((Window *)pViewportWindow);
    }
}

void ViewportWindow_OnPaint(ViewportWindow* pViewportWindow)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hWnd, &ps);
    HDC hdcTarget = hdc;

    RECT clientRc;
    HDC hdcBack = NULL;
    HBITMAP hbmBack = NULL;
    HGDIOBJ hOldObjBack = NULL;

    GetClientRect(hWnd, &clientRc);

    /* Using double buffering to avoid canvas flicker */
    if (g_viewportSettings.doubleBuffered) {

        /* Create back context and buffet, set it */
        hdcBack = CreateCompatibleDC(hdc);
        hbmBack = CreateCompatibleBitmap(hdc, clientRc.right, clientRc.bottom);
        hOldObjBack = SelectObject(hdcBack, hbmBack);

        /* Draw window background */
        HBRUSH hbr = (HBRUSH)GetClassLongPtr(hWnd, GCLP_HBRBACKGROUND);
        FillRect(hdcBack, &clientRc, hbr);

        /* Set backbuffer as draw target */
        hdcTarget = hdcBack;
    }

    Document* document = ViewportWindow_GetDocument(pViewportWindow);
    if (document)
    {
        Canvas* canvas = Document_GetCanvas(document);
        assert(canvas);

        RECT renderRc;

        renderRc.left = (LONG)(((float)clientRc.right - (float)canvas->width * pViewportWindow->fZoom) / 2.f
            + (float)pViewportWindow->ptOffset.x);
        renderRc.top = (LONG)(((float)clientRc.bottom - (float)canvas->height * pViewportWindow->fZoom) / 2.f
            + (float)pViewportWindow->ptOffset.y);
        renderRc.right = (LONG)((float)renderRc.left + ((float)canvas->width * pViewportWindow->fZoom));
        renderRc.bottom = (LONG)((float)renderRc.top + ((float)canvas->height * pViewportWindow->fZoom));


        /* Draw transparency checkerboard */
        FillRect(hdcTarget, &renderRc, pViewportWindow->hbrChecker);

        /* Copy canvas to viewport */
        Viewport_BlitCanvas(hdcTarget, &renderRc, document->canvas);

        /* Draw canvas frame */
        RECT frameRc = {
          max(0, renderRc.left), max(0, renderRc.top),
          min(clientRc.right, renderRc.right),
          min(clientRc.bottom, renderRc.bottom)
        };
        FrameRect(hdcTarget, &frameRc, GetStockObject(BLACK_BRUSH));


        if (g_viewportSettings.showDebugInfo)
        {
            ViewportWindow_DrawDebugOverlay(pViewportWindow, hdcTarget, &clientRc);
        }

        if (g_viewportSettings.doubleBuffered)
        {
            /* Copy backbuffer to window */
            BitBlt(hdc, 0, 0, clientRc.right, clientRc.bottom, hdcBack, 0, 0, SRCCOPY);

            /* Release and free backbuffer resources */
            SelectObject(hdcBack, hOldObjBack);
            DeleteObject(hbmBack);
            DeleteDC(hdcBack);
        }
    }

    EndPaint(hWnd, &ps);
}

LRESULT ViewportWindow_OnCommand(ViewportWindow* pViewportWindow, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    if (LOWORD(wParam) == IDM_VIEWPORTSETTINGS)
    {
        DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_VIEWPORTSETTINGS),
            hWnd, (DLGPROC)ViewportSettingsDlgProc);
    }

    return 0;
}

BOOL ViewportWindow_OnKeyDown(ViewportWindow* pViewportWindow, UINT vk, int cRepeat, UINT flags)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    if (vk == VK_SPACE) {
        if (!pViewportWindow->bDrag)
        {
            POINT ptCursor;
            GetCursorPos(&ptCursor);
            ScreenToClient(hWnd, &ptCursor);

            pViewportWindow->ptDrag.x = ptCursor.x;
            pViewportWindow->ptDrag.y = ptCursor.y;

            pViewportWindow->bDrag = TRUE;
        }
    }

    if (vk == VK_F3)
    {
        g_viewportSettings.showDebugInfo = !g_viewportSettings.showDebugInfo;
        Window_Invalidate((Window *)pViewportWindow);
    }

    return FALSE;
}

BOOL ViewportWindow_OnKeyUp(ViewportWindow* pViewportWindow, UINT vk, int cRepeat, UINT flags)
{
    if (vk == VK_SPACE) {
        pViewportWindow->bDrag = FALSE;
    }

    return FALSE;
}

void ViewportWindow_OnMouseMove(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    Tool *pTool = Panitent_GetTool();

    if (pViewportWindow->bDrag)
    {
        pViewportWindow->ptOffset.x += x - pViewportWindow->ptDrag.x;
        pViewportWindow->ptOffset.y += y - pViewportWindow->ptDrag.y;

        pViewportWindow->ptDrag.x = x;
        pViewportWindow->ptDrag.y = y;

        Window_Invalidate((Window *)pViewportWindow);
    }
    else if (pTool && pTool->OnMouseMove)
    {
        POINT ptCanvas;
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        pTool->OnMouseMove(pTool, pViewportWindow, x, y, keyFlags);
    }
}

void ViewportWindow_OnMouseWheel(ViewportWindow* pViewportWindow, int xPos, int yPos, int zDelta, UINT fwKeys)
{
    if (fwKeys & MK_CONTROL)
    {
        pViewportWindow->fZoom = max(0.1f, pViewportWindow->fZoom + zDelta /
            (float)(WHEEL_DELTA * 16.f));
    }

    /*  Scroll canvas horizontally
     *
     *  GetKeyState is okay because WPARAM doesn't provide ALT key modifier
     */
    else if (GetKeyState(VK_MENU) & 0x8000) {
        pViewportWindow->ptOffset.x += zDelta / 2;
    }

    /* Scroll canvas vertically */
    else {
        pViewportWindow->ptOffset.y += zDelta / 2;
    }

    Window_Invalidate((Window *)pViewportWindow);
}

void ViewportWindow_OnLButtonDown(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    /* Receive keyboard messages */
    SetFocus(hWnd);

    Tool* pTool = Panitent_GetTool();
    if (pTool && pTool->OnLButtonDown)
    {
        POINT ptCanvas;
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        pTool->OnLButtonDown(pTool, pViewportWindow, x, y, keyFlags);
    }
}

void ViewportWindow_OnLButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    Tool* pTool = Panitent_GetTool();

    if (pTool && pTool->OnLButtonUp)
    {
        RECT clientRc;
        GetClientRect(hWnd, &clientRc);

        POINT ptCanvas;
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        pTool->OnLButtonUp(pTool, pViewportWindow, x, y, keyFlags);
    }
}

void ViewportWindow_OnRButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    Tool* pTool = Panitent_GetTool();

    if (pTool && pTool->OnRButtonUp)
    {
        POINT ptCanvas;
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        pTool->OnRButtonUp(pTool, pViewportWindow, x, y, keyFlags);
    }
}

void ViewportWindow_OnContextMenu(ViewportWindow* pViewportWindow, HWND hwndContext, UINT xPos, UINT yPos)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    xPos = xPos < 0 ? 0 : xPos;
    yPos = yPos < 0 ? 0 : yPos;

    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_VIEWPORTSETTINGS, L"Settings");
    TrackPopupMenu(hMenu, 0, xPos, yPos, 0, hWnd, NULL);
}

void ViewportWindow_OnMButtonDown(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    if (!pViewportWindow->bDrag)
    {
        POINT ptCursor;
        GetCursorPos(&ptCursor);
        ScreenToClient(hWnd, &ptCursor);

        pViewportWindow->ptDrag.x = ptCursor.x;
        pViewportWindow->ptDrag.y = ptCursor.y;

        pViewportWindow->bDrag = TRUE;
    }
}

void ViewportWindow_OnMButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    pViewportWindow->bDrag = FALSE;
}

BOOL ViewportWindow_OnEraseBkgnd(ViewportWindow* pViewportWindow, HDC hdc)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    if (g_viewportSettings.doubleBuffered)
        /* Avoid pass to user32 */
        return TRUE;

    RECT clientRc;
    GetClientRect(hWnd, &clientRc);

    if (pViewportWindow->document && g_viewportSettings.bkgndExcludeCanvas)
    {
        RECT canvasRc;
        canvasRc.right = pViewportWindow->document->canvas->width;
        canvasRc.bottom = pViewportWindow->document->canvas->height;

        HBRUSH hbr = (HBRUSH)GetClassLongPtr(hWnd, GCLP_HBRBACKGROUND);

        switch (g_viewportSettings.backgroundMethod)
        {
        case BKGND_RECTANGLES:
        {
            RECT rc1 = { 0 }, rc2 = { 0 };

            rc1.left = canvasRc.right + 1;
            rc1.bottom = canvasRc.bottom + 1;
            rc1.right = clientRc.right;

            rc2.top = canvasRc.bottom + 1;
            rc2.bottom = clientRc.bottom;
            rc2.right = clientRc.right;

            FillRect(hdc, &rc1, hbr);
            FillRect(hdc, &rc2, hbr);
        }
        break;
        case BKGND_REGION:
        {
            int iSavedState = SaveDC(hdc);
            SelectClipRgn(hdc, NULL);
            ExcludeClipRect(hdc, 0, 0, canvasRc.right + 1, canvasRc.bottom + 1);
            FillRect(hdc, &clientRc, hbr);
            RestoreDC(hdc, iSavedState);
        }
        break;
        }

        return TRUE;
    }

    return FALSE;
}

void ViewportWindow_OnDestroy(ViewportWindow* window)
{

}

LRESULT ViewportWindow_UserProc(ViewportWindow* pViewportWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
    {
        BOOL bProcessed = ViewportWindow_OnKeyDown(pViewportWindow, (UINT)(wParam), (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam));
        if (bProcessed)
        {
            return 0;
        }
    }
    break;

    case WM_KEYUP:
    {
        BOOL bProcessed = ViewportWindow_OnKeyUp(pViewportWindow, (UINT)(wParam), (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam));
        if (bProcessed)
        {
            return 0;
        }
    }
    break;

    case WM_MOUSEWHEEL:
        ViewportWindow_OnMouseWheel(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (int)(short)GET_WHEEL_DELTA_WPARAM(wParam), (UINT)(short)GET_KEYSTATE_WPARAM(wParam));
        return 0;
        break;

    case WM_LBUTTONDOWN:
        ViewportWindow_OnLButtonDown(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)(wParam));
        return 0;
        break;

    case WM_LBUTTONUP:
        ViewportWindow_OnLButtonUp(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)(wParam));
        return 0;
        break;

    case WM_MBUTTONDOWN:
        ViewportWindow_OnMButtonDown(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)(wParam));
        return 0;
        break;

    case WM_MBUTTONUP:
        ViewportWindow_OnMButtonUp(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)(wParam));
        return 0;
        break;

    case WM_RBUTTONUP:
        ViewportWindow_OnRButtonUp(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)(wParam));
        /* Let system generate WM_CONTEXTMENU message */
        break;

    case WM_MOUSEMOVE:
        ViewportWindow_OnMouseMove(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)(wParam));
        return 0;
        break;

    case WM_COMMAND:
        return ViewportWindow_OnCommand(pViewportWindow, wParam, lParam);
        break;

    case WM_CONTEXTMENU:
        ViewportWindow_OnContextMenu(pViewportWindow, (HWND)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
        return 0;
        break;

    case WM_ERASEBKGND:
        return ViewportWindow_OnEraseBkgnd(pViewportWindow, (HDC)wParam);
        break;
    }

    return Window_UserProcDefault((Window *)pViewportWindow, hWnd, message, wParam, lParam);
}

INT_PTR CALLBACK ViewportSettingsDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        /* Restore check states */
        Button_SetCheck(GetDlgItem(hwndDlg, IDC_DOUBLEBUFFER),
            g_viewportSettings.doubleBuffered);
        Button_SetCheck(GetDlgItem(hwndDlg, IDC_BKGNDEXCLUDECANVAS),
            g_viewportSettings.bkgndExcludeCanvas);
        Button_SetCheck(GetDlgItem(hwndDlg, IDC_SHOWDEBUG),
            g_viewportSettings.showDebugInfo);

        switch (g_viewportSettings.backgroundMethod)
        {
        case BKGND_RECTANGLES:
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_BKGNDFILLRECT), TRUE);
            break;
        case BKGND_REGION:
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_BKGNDREGION), TRUE);
            break;
        }

        /* Enable/disable sub-options */
        Button_Enable(GetDlgItem(hwndDlg, IDC_BKGNDFILLRECT),
            g_viewportSettings.bkgndExcludeCanvas);
        Button_Enable(GetDlgItem(hwndDlg, IDC_BKGNDREGION),
            g_viewportSettings.bkgndExcludeCanvas);

        return TRUE;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
            case IDC_DOUBLEBUFFER:
                g_viewportSettings.doubleBuffered = Button_GetCheck((HWND)lParam);
                break;
            case IDC_BKGNDEXCLUDECANVAS:
            {
                BOOL fStatus = Button_GetCheck((HWND)lParam);
                g_viewportSettings.bkgndExcludeCanvas = fStatus;

                Button_Enable(GetDlgItem(hwndDlg, IDC_BKGNDFILLRECT), fStatus);
                Button_Enable(GetDlgItem(hwndDlg, IDC_BKGNDREGION), fStatus);
            }
            break;
            case IDC_SHOWDEBUG:
                g_viewportSettings.showDebugInfo = Button_GetCheck((HWND)lParam);
                break;
            case IDC_BKGNDFILLRECT:
                g_viewportSettings.backgroundMethod = BKGND_RECTANGLES;
                break;
            case IDC_BKGNDREGION:
                g_viewportSettings.backgroundMethod = BKGND_REGION;
                break;
            case IDOK:
            case IDCANCEL:
                EndDialog(hwndDlg, wParam);
            }
        }

        Window_Invalidate((Window *)Panitent_GetActiveViewport());
        return TRUE;
    }

    return FALSE;
}
