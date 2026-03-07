#include "precomp.h"
#include "win32/window.h"
#include "document.h"
#include "history.h"

#include "viewport.h"
#include "canvas.h"
#include "alphamask.h"
#include "win32/util.h"
#include "tool.h"
#include "checker.h"
#include "resource.h"
#include "panitentapp.h"
#include <strsafe.h>

#define IDM_VIEWPORTSETTINGS 100
#define IDC_VIEWPORT_TEXT_EDIT 3201
#define WMU_VIEWPORT_COMMITTEXT (WM_APP + 101)
#define WMU_VIEWPORT_CANCELTEXT (WM_APP + 102)
#define VIEWPORT_TEXT_FONT_PX 24
#define VIEWPORT_TEXT_MIN_WIDTH 160
#define VIEWPORT_TEXT_PADDING_X 8
#define VIEWPORT_TEXT_PADDING_Y 6

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

ViewportWindow* ViewportWindow_Create();
void ViewportWindow_Init(ViewportWindow* window);

static inline void ViewportWindow_DrawWindowOrdinates(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static inline void ViewportWindow_DrawCanvasOrdinates(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static inline void ViewportWindow_DrawOffsetTrace(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static inline void ViewportWindow_DrawDebugText(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static inline void ViewportWindow_DrawDebugOverlay(ViewportWindow* pViewportWindow, HDC hdc, LPRECT lpRect);
static HFONT ViewportWindow_CreateTextOverlayFont(int pixelHeight);
static void ViewportWindow_DestroyTextOverlayWindow(ViewportWindow* pViewportWindow);
static void ViewportWindow_UpdateTextOverlayLayout(ViewportWindow* pViewportWindow);
static void ViewportWindow_RenderTextOverlayToCanvas(ViewportWindow* pViewportWindow, PCWSTR pszText);
static BOOL ViewportWindow_GetTextLayoutMetrics(HDC hdc, HFONT hFont, PCWSTR pszText, SIZE* pSize, int* pLineAdvance);
static void ViewportWindow_DrawTextLines(HDC hdc, PCWSTR pszText, int lineAdvance);
static LRESULT CALLBACK ViewportWindow_TextOverlayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

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
void ViewportWindow_OnRButtonDown(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void ViewportWindow_OnRButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void ViewportWindow_OnContextMenu(ViewportWindow* pViewportWindow, HWND hwndContext, UINT xPos, UINT yPos);
void ViewportWindow_OnMButtonDown(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void ViewportWindow_OnMButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
BOOL ViewportWindow_OnEraseBkgnd(ViewportWindow* pViewportWindow, HDC hdc);
void ViewportWindow_OnDestroy(ViewportWindow* window);
LRESULT ViewportWindow_UserProc(ViewportWindow* pViewportWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

ViewportWindow* ViewportWindow_Create()
{
    ViewportWindow* pViewportWindow = (ViewportWindow*)malloc(sizeof(ViewportWindow));
    memset(pViewportWindow, 0, sizeof(ViewportWindow));

    if (pViewportWindow)
    {
        ViewportWindow_Init(pViewportWindow);
    }

    return pViewportWindow;
}

void ViewportWindow_Init(ViewportWindow* pViewportWindow)
{
    Window_Init(&pViewportWindow->base);

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

    hFont = PanitentApp_GetUIFont(PanitentApp_Instance());

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
    hFont = PanitentApp_GetUIFont(PanitentApp_Instance());

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
        L"Document dimensions width: %d, AVLNode_Height %d";

    hFont = PanitentApp_GetUIFont(PanitentApp_Instance());

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

void ViewportWindow_CanvasToClient(ViewportWindow* pViewportWindow, int x, int y, LPPOINT lpPt)
{
    HWND hWnd = Window_GetHWND((Window*)pViewportWindow);
    Document* document = ViewportWindow_GetDocument(pViewportWindow);
    if (!lpPt)
    {
        return;
    }

    lpPt->x = 0;
    lpPt->y = 0;

    if (!document || !document->canvas)
    {
        return;
    }

    RECT clientRc = { 0 };
    GetClientRect(hWnd, &clientRc);

    lpPt->x = (LONG)(((float)clientRc.right - (float)document->canvas->width * pViewportWindow->fZoom) / 2.0f +
        (float)pViewportWindow->ptOffset.x + (float)x * pViewportWindow->fZoom);
    lpPt->y = (LONG)(((float)clientRc.bottom - (float)document->canvas->height * pViewportWindow->fZoom) / 2.0f +
        (float)pViewportWindow->ptOffset.y + (float)y * pViewportWindow->fZoom);
}

BOOL ViewportWindow_HasTextOverlay(ViewportWindow* pViewportWindow)
{
    return pViewportWindow && pViewportWindow->hWndTextOverlay && IsWindow(pViewportWindow->hWndTextOverlay);
}

BOOL ViewportWindow_BeginTextOverlay(ViewportWindow* pViewportWindow, int xCanvas, int yCanvas, uint32_t color)
{
    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    if (!pViewportWindow || !hWndViewport || !IsWindow(hWndViewport) || ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return FALSE;
    }

    pViewportWindow->ptTextOverlayCanvas.x = xCanvas;
    pViewportWindow->ptTextOverlayCanvas.y = yCanvas;
    pViewportWindow->textOverlayColor = color;
    pViewportWindow->nTextOverlayFontDocPx = VIEWPORT_TEXT_FONT_PX;
    pViewportWindow->nTextOverlayFontClientPx = 0;
    pViewportWindow->bTextOverlayClosing = FALSE;

    HWND hWndEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_MULTILINE | ES_WANTRETURN | WS_BORDER,
        0,
        0,
        VIEWPORT_TEXT_MIN_WIDTH,
        VIEWPORT_TEXT_FONT_PX + VIEWPORT_TEXT_PADDING_Y * 2,
        hWndViewport,
        (HMENU)(INT_PTR)IDC_VIEWPORT_TEXT_EDIT,
        GetModuleHandle(NULL),
        NULL);
    if (!hWndEdit)
    {
        return FALSE;
    }

    pViewportWindow->hWndTextOverlay = hWndEdit;
    SetWindowLongPtrW(hWndEdit, GWLP_USERDATA, (LONG_PTR)pViewportWindow);
    pViewportWindow->pfnTextOverlayProc = (WNDPROC)(ULONG_PTR)SetWindowLongPtrW(
        hWndEdit, GWLP_WNDPROC, (LONG_PTR)ViewportWindow_TextOverlayProc);
    SendMessageW(
        hWndEdit,
        EM_SETMARGINS,
        EC_LEFTMARGIN | EC_RIGHTMARGIN,
        MAKELPARAM(VIEWPORT_TEXT_PADDING_X, VIEWPORT_TEXT_PADDING_X));

    ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
    SetFocus(hWndEdit);
    return TRUE;
}

void ViewportWindow_CommitTextOverlay(ViewportWindow* pViewportWindow)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    HWND hWndEdit = pViewportWindow->hWndTextOverlay;
    int cchText = GetWindowTextLengthW(hWndEdit);
    if (cchText > 0)
    {
        WCHAR* pszText = (WCHAR*)calloc((size_t)cchText + 1, sizeof(WCHAR));
        if (pszText)
        {
            GetWindowTextW(hWndEdit, pszText, cchText + 1);
            if (pszText[0] != L'\0')
            {
                ViewportWindow_RenderTextOverlayToCanvas(pViewportWindow, pszText);
            }
            free(pszText);
        }
    }

    ViewportWindow_DestroyTextOverlayWindow(pViewportWindow);
}

void ViewportWindow_CancelTextOverlay(ViewportWindow* pViewportWindow)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    ViewportWindow_DestroyTextOverlayWindow(pViewportWindow);
}

BOOL Viewport_BlitCanvas(HDC hDC, LPRECT prcView, Canvas* canvas)
{
    assert(canvas);
    if (!canvas)
        return FALSE;

    HDC hBmDC = CreateCompatibleDC(hDC);

    unsigned char* pData = malloc(canvas->buffer_size);
    memset(pData, 0, canvas->buffer_size);
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
    UNREFERENCED_PARAMETER(state);
    UNREFERENCED_PARAMETER(cx);
    UNREFERENCED_PARAMETER(cy);

    if (ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
    }

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
    Tool* pTool = PanitentApp_GetTool(PanitentApp_Instance());

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
        Canvas* previewCanvas = NULL;
        Canvas* canvasToRender = canvas;
        assert(canvas);

        if (canvas && pTool && pTool->HasPreview && pTool->DrawPreview && pTool->HasPreview(pTool))
        {
            previewCanvas = Canvas_Clone(canvas);
            if (previewCanvas)
            {
                pTool->DrawPreview(pTool, pViewportWindow, previewCanvas);
                canvasToRender = previewCanvas;
            }
        }

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
        Viewport_BlitCanvas(hdcTarget, &renderRc, canvasToRender);

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

        if (previewCanvas)
        {
            Canvas_Delete(previewCanvas);
            free(previewCanvas);
            previewCanvas = NULL;
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

static HFONT ViewportWindow_CreateTextOverlayFont(int pixelHeight)
{
    LOGFONTW lf = { 0 };
    HFONT hBaseFont = PanitentApp_GetUIFont(PanitentApp_Instance());
    if (!hBaseFont || GetObjectW(hBaseFont, sizeof(lf), &lf) != sizeof(lf))
    {
        HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (!hDefaultFont || GetObjectW(hDefaultFont, sizeof(lf), &lf) != sizeof(lf))
        {
            ZeroMemory(&lf, sizeof(lf));
            StringCchCopyW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), L"Segoe UI");
        }
    }

    lf.lfHeight = -max(1, pixelHeight);
    lf.lfWidth = 0;
    lf.lfQuality = ANTIALIASED_QUALITY;
    return CreateFontIndirectW(&lf);
}

static void ViewportWindow_DestroyTextOverlayWindow(ViewportWindow* pViewportWindow)
{
    if (!pViewportWindow)
    {
        return;
    }

    pViewportWindow->bTextOverlayClosing = TRUE;

    if (pViewportWindow->hWndTextOverlay && IsWindow(pViewportWindow->hWndTextOverlay))
    {
        if (pViewportWindow->pfnTextOverlayProc)
        {
            SetWindowLongPtrW(
                pViewportWindow->hWndTextOverlay,
                GWLP_WNDPROC,
                (LONG_PTR)pViewportWindow->pfnTextOverlayProc);
        }
        DestroyWindow(pViewportWindow->hWndTextOverlay);
    }

    pViewportWindow->hWndTextOverlay = NULL;
    pViewportWindow->pfnTextOverlayProc = NULL;

    if (pViewportWindow->hFontTextOverlay)
    {
        DeleteObject(pViewportWindow->hFontTextOverlay);
        pViewportWindow->hFontTextOverlay = NULL;
    }

    pViewportWindow->nTextOverlayFontClientPx = 0;
    pViewportWindow->bTextOverlayClosing = FALSE;
}

static BOOL ViewportWindow_GetTextLayoutMetrics(HDC hdc, HFONT hFont, PCWSTR pszText, SIZE* pSize, int* pLineAdvance)
{
    if (!hdc || !pSize)
    {
        return FALSE;
    }

    HGDIOBJ hOldFont = NULL;
    if (hFont)
    {
        hOldFont = SelectObject(hdc, hFont);
    }

    TEXTMETRICW tm = { 0 };
    GetTextMetricsW(hdc, &tm);

    int lineAdvance = max(1, tm.tmHeight + tm.tmExternalLeading);
    int maxWidth = 0;
    int lineCount = 1;

    if (pszText && pszText[0] != L'\0')
    {
        const WCHAR* pLine = pszText;
        while (*pLine)
        {
            const WCHAR* pLineEnd = pLine;
            while (*pLineEnd && *pLineEnd != L'\r' && *pLineEnd != L'\n')
            {
                ++pLineEnd;
            }

            int cchLine = (int)(pLineEnd - pLine);
            SIZE lineSize = { 0 };
            if (cchLine > 0)
            {
                GetTextExtentPoint32W(hdc, pLine, cchLine, &lineSize);
            }
            maxWidth = max(maxWidth, lineSize.cx);

            if (*pLineEnd == L'\0')
            {
                break;
            }

            ++lineCount;
            if (*pLineEnd == L'\r' && *(pLineEnd + 1) == L'\n')
            {
                pLine = pLineEnd + 2;
            }
            else
            {
                pLine = pLineEnd + 1;
            }
        }
    }

    pSize->cx = maxWidth;
    pSize->cy = lineCount * lineAdvance;

    if (pLineAdvance)
    {
        *pLineAdvance = lineAdvance;
    }

    if (hOldFont)
    {
        SelectObject(hdc, hOldFont);
    }

    return TRUE;
}

static void ViewportWindow_DrawTextLines(HDC hdc, PCWSTR pszText, int lineAdvance)
{
    if (!hdc)
    {
        return;
    }

    if (!pszText || pszText[0] == L'\0')
    {
        return;
    }

    int y = 0;
    const WCHAR* pLine = pszText;
    while (*pLine)
    {
        const WCHAR* pLineEnd = pLine;
        while (*pLineEnd && *pLineEnd != L'\r' && *pLineEnd != L'\n')
        {
            ++pLineEnd;
        }

        int cchLine = (int)(pLineEnd - pLine);
        if (cchLine > 0)
        {
            TextOutW(hdc, 0, y, pLine, cchLine);
        }

        if (*pLineEnd == L'\0')
        {
            break;
        }

        y += lineAdvance;
        if (*pLineEnd == L'\r' && *(pLineEnd + 1) == L'\n')
        {
            pLine = pLineEnd + 2;
        }
        else
        {
            pLine = pLineEnd + 1;
        }
    }
}

static void ViewportWindow_UpdateTextOverlayLayout(ViewportWindow* pViewportWindow)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    HWND hWndEdit = pViewportWindow->hWndTextOverlay;
    int fontClientPx = max(1, (int)lroundf((float)pViewportWindow->nTextOverlayFontDocPx * max(0.1f, pViewportWindow->fZoom)));

    if (fontClientPx != pViewportWindow->nTextOverlayFontClientPx)
    {
        if (pViewportWindow->hFontTextOverlay)
        {
            DeleteObject(pViewportWindow->hFontTextOverlay);
            pViewportWindow->hFontTextOverlay = NULL;
        }

        pViewportWindow->hFontTextOverlay = ViewportWindow_CreateTextOverlayFont(fontClientPx);
        pViewportWindow->nTextOverlayFontClientPx = fontClientPx;
        if (pViewportWindow->hFontTextOverlay)
        {
            SendMessageW(hWndEdit, WM_SETFONT, (WPARAM)pViewportWindow->hFontTextOverlay, FALSE);
        }
    }

    int cchText = GetWindowTextLengthW(hWndEdit);
    WCHAR* pszText = (WCHAR*)calloc((size_t)cchText + 1, sizeof(WCHAR));
    if (!pszText)
    {
        return;
    }

    GetWindowTextW(hWndEdit, pszText, cchText + 1);

    HDC hdc = GetDC(hWndViewport);
    SIZE textSize = { 0 };
    int lineAdvance = fontClientPx;
    ViewportWindow_GetTextLayoutMetrics(hdc, pViewportWindow->hFontTextOverlay, pszText, &textSize, &lineAdvance);
    ReleaseDC(hWndViewport, hdc);

    int width = max(VIEWPORT_TEXT_MIN_WIDTH, textSize.cx + VIEWPORT_TEXT_PADDING_X * 2);
    int height = max(lineAdvance + VIEWPORT_TEXT_PADDING_Y * 2, textSize.cy + VIEWPORT_TEXT_PADDING_Y * 2);
    POINT ptClient = { 0 };
    ViewportWindow_CanvasToClient(
        pViewportWindow,
        pViewportWindow->ptTextOverlayCanvas.x,
        pViewportWindow->ptTextOverlayCanvas.y,
        &ptClient);

    SetWindowPos(
        hWndEdit,
        HWND_TOP,
        ptClient.x,
        ptClient.y,
        width,
        height,
        SWP_NOACTIVATE);

    free(pszText);
}

static void ViewportWindow_RenderTextOverlayToCanvas(ViewportWindow* pViewportWindow, PCWSTR pszText)
{
    if (!pViewportWindow || !pszText || pszText[0] == L'\0')
    {
        return;
    }

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    if (!pDocument)
    {
        return;
    }

    Canvas* pCanvas = Document_GetCanvas(pDocument);
    if (!pCanvas)
    {
        return;
    }

    HFONT hFont = ViewportWindow_CreateTextOverlayFont(max(1, pViewportWindow->nTextOverlayFontDocPx));
    if (!hFont)
    {
        return;
    }

    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    HDC hdcViewport = GetDC(hWndViewport);
    HDC hdcText = CreateCompatibleDC(hdcViewport);

    SIZE textSize = { 0 };
    int lineAdvance = 0;
    ViewportWindow_GetTextLayoutMetrics(hdcText, hFont, pszText, &textSize, &lineAdvance);

    int width = max(1, textSize.cx);
    int height = max(1, textSize.cy);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    uint32_t* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcText, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    if (hBitmap && pBits)
    {
        HGDIOBJ hOldBitmap = SelectObject(hdcText, hBitmap);
        HGDIOBJ hOldFont = SelectObject(hdcText, hFont);

        memset(pBits, 0, (size_t)width * (size_t)height * sizeof(uint32_t));
        SetBkMode(hdcText, TRANSPARENT);
        SetTextColor(hdcText, RGB(255, 255, 255));
        ViewportWindow_DrawTextLines(hdcText, pszText, lineAdvance);

        AlphaMask* pMask = AlphaMask_Create(width, height);
        if (pMask)
        {
            BOOL bHasCoverage = FALSE;
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    uint32_t pixel = pBits[(size_t)y * (size_t)width + (size_t)x];
                    uint8_t coverage = max(CHANNEL_R_32(pixel), max(CHANNEL_G_32(pixel), CHANNEL_B_32(pixel)));
                    if (coverage > 0)
                    {
                        bHasCoverage = TRUE;
                        AlphaMask_SetMax(pMask, x, y, coverage);
                    }
                }
            }

            if (bHasCoverage)
            {
                History_StartDifferentiation(pDocument);
                Canvas_ColorStencilMask(
                    pCanvas,
                    pViewportWindow->ptTextOverlayCanvas.x,
                    pViewportWindow->ptTextOverlayCanvas.y,
                    pMask,
                    pViewportWindow->textOverlayColor);
                History_FinalizeDifferentiation(pDocument);

                Window_Invalidate((Window*)pViewportWindow);
            }

            AlphaMask_Delete(pMask);
        }

        SelectObject(hdcText, hOldFont);
        SelectObject(hdcText, hOldBitmap);
        DeleteObject(hBitmap);
    }

    DeleteDC(hdcText);
    ReleaseDC(hWndViewport, hdcViewport);
    DeleteObject(hFont);
}

static LRESULT CALLBACK ViewportWindow_TextOverlayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ViewportWindow* pViewportWindow = (ViewportWindow*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    WNDPROC pfnPrev = pViewportWindow ? pViewportWindow->pfnTextOverlayProc : NULL;

    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostMessageW(GetParent(hWnd), WMU_VIEWPORT_CANCELTEXT, 0, 0);
            return 0;
        }
        if (wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            PostMessageW(GetParent(hWnd), WMU_VIEWPORT_COMMITTEXT, 0, 0);
            return 0;
        }
        break;

    case WM_KILLFOCUS:
        if (pViewportWindow && !pViewportWindow->bTextOverlayClosing)
        {
            PostMessageW(GetParent(hWnd), WMU_VIEWPORT_COMMITTEXT, 0, 0);
        }
        break;
    }

    return pfnPrev ? CallWindowProcW(pfnPrev, hWnd, message, wParam, lParam) : DefWindowProcW(hWnd, message, wParam, lParam);
}

LRESULT ViewportWindow_OnCommand(ViewportWindow* pViewportWindow, WPARAM wParam, LPARAM lParam)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    if ((HWND)lParam == pViewportWindow->hWndTextOverlay)
    {
        switch (HIWORD(wParam))
        {
        case EN_CHANGE:
            ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
            return 0;
        }
    }

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

    Tool *pTool = PanitentApp_GetTool(PanitentApp_Instance());

    if (pViewportWindow->bDrag)
    {
        pViewportWindow->ptOffset.x += x - pViewportWindow->ptDrag.x;
        pViewportWindow->ptOffset.y += y - pViewportWindow->ptDrag.y;

        pViewportWindow->ptDrag.x = x;
        pViewportWindow->ptDrag.y = y;

        if (ViewportWindow_HasTextOverlay(pViewportWindow))
        {
            ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
        }
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

    if (ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
    }
    Window_Invalidate((Window *)pViewportWindow);
}

void ViewportWindow_OnLButtonDown(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    /* Receive keyboard messages */
    SetFocus(hWnd);

    Tool* pTool = PanitentApp_GetTool(PanitentApp_Instance());
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

    Tool* pTool = PanitentApp_GetTool(PanitentApp_Instance());

    if (pTool && pTool->OnLButtonUp)
    {
        RECT clientRc;
        GetClientRect(hWnd, &clientRc);

        POINT ptCanvas;
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        pTool->OnLButtonUp(pTool, pViewportWindow, x, y, keyFlags);
    }
}

void ViewportWindow_OnRButtonDown(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    Tool* pTool = PanitentApp_GetTool(PanitentApp_Instance());

    if (pTool && pTool->OnRButtonDown)
    {
        pTool->OnRButtonDown(pTool, pViewportWindow, x, y, keyFlags);
    }
}

void ViewportWindow_OnRButtonUp(ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    Tool* pTool = PanitentApp_GetTool(PanitentApp_Instance());

    if (pTool && pTool->OnRButtonUp)
    {
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
    DestroyMenu(hMenu);
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
    ViewportWindow_CancelTextOverlay(window);
    if (window->hbrChecker)
    {
        DeleteObject(window->hbrChecker);
        window->hbrChecker = NULL;
    }
}

LRESULT ViewportWindow_UserProc(ViewportWindow* pViewportWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WMU_VIEWPORT_COMMITTEXT:
        ViewportWindow_CommitTextOverlay(pViewportWindow);
        return 0;

    case WMU_VIEWPORT_CANCELTEXT:
        ViewportWindow_CancelTextOverlay(pViewportWindow);
        return 0;

    case WM_CTLCOLOREDIT:
        if ((HWND)lParam == pViewportWindow->hWndTextOverlay)
        {
            HDC hdcEdit = (HDC)wParam;
            COLORREF rgbText = RGB(
                CHANNEL_R_32(pViewportWindow->textOverlayColor),
                CHANNEL_G_32(pViewportWindow->textOverlayColor),
                CHANNEL_B_32(pViewportWindow->textOverlayColor));
            SetTextColor(hdcEdit, rgbText);
            SetBkColor(hdcEdit, RGB(255, 255, 255));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
        break;

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

    case WM_RBUTTONDOWN:
        ViewportWindow_OnRButtonDown(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)(wParam));
        return 0;
        break;

    case WM_RBUTTONUP:
        ViewportWindow_OnRButtonUp(pViewportWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)(wParam));
        if (PanitentApp_GetTool(PanitentApp_Instance()) && PanitentApp_GetTool(PanitentApp_Instance())->OnRButtonDown)
        {
            return 0;
        }
        /* Let system generate WM_CONTEXTMENU message for non-drag tools. */
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

        Window_Invalidate((Window*)PanitentApp_GetCurrentViewport(PanitentApp_Instance()));
        return TRUE;
    }

    return FALSE;
}
