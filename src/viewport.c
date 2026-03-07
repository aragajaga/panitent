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
#define VIEWPORT_TEXT_CARET_TIMER_ID 3201
#define VIEWPORT_TEXT_CARET_INTERVAL_MS 530
#define VIEWPORT_TEXT_MIN_WIDTH 8
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
static void ViewportWindow_DestroyTextOverlayState(ViewportWindow* pViewportWindow);
static void ViewportWindow_UpdateTextOverlayLayout(ViewportWindow* pViewportWindow);
static void ViewportWindow_RenderTextOverlayToCanvas(ViewportWindow* pViewportWindow, PCWSTR pszText);
static BOOL ViewportWindow_GetTextLayoutMetrics(HDC hdc, HFONT hFont, PCWSTR pszText, SIZE* pSize, int* pLineAdvance);
static void ViewportWindow_DrawTextLines(HDC hdc, PCWSTR pszText, int lineAdvance);
static void ViewportWindow_DrawTextOverlay(ViewportWindow* pViewportWindow, HDC hdcTarget);
static BOOL ViewportWindow_EnsureTextOverlayCapacity(ViewportWindow* pViewportWindow, size_t cchRequired);
static BOOL ViewportWindow_IsTextToolActive(void);
static size_t ViewportWindow_GetTextOverlayLineStart(PCWSTR pszText, size_t nLineIndex);
static size_t ViewportWindow_GetTextOverlayLineEnd(PCWSTR pszText, size_t nLineStart);
static size_t ViewportWindow_GetTextOverlayLineIndex(PCWSTR pszText, size_t nCaret);
static size_t ViewportWindow_GetTextOverlayColumn(PCWSTR pszText, size_t nCaret);
static size_t ViewportWindow_GetTextOverlayCaretForLineColumn(PCWSTR pszText, size_t nLineIndex, size_t nColumn);
static BOOL ViewportWindow_GetTextOverlayCaretClientPoint(ViewportWindow* pViewportWindow, POINT* pPoint, int* pHeight);
static BOOL ViewportWindow_IsPointInTextOverlay(ViewportWindow* pViewportWindow, int x, int y);
static void ViewportWindow_SetTextOverlayCaret(ViewportWindow* pViewportWindow, size_t nCaret, BOOL fKeepPreferredColumn);
static void ViewportWindow_MoveTextOverlayCaretFromPoint(ViewportWindow* pViewportWindow, int xClient, int yClient);
static void ViewportWindow_InsertTextOverlayChars(ViewportWindow* pViewportWindow, PCWSTR pszText, size_t cchText);
static void ViewportWindow_DeleteTextOverlayRange(ViewportWindow* pViewportWindow, size_t nStart, size_t nEnd);
static BOOL ViewportWindow_HandleTextOverlayKeyDown(ViewportWindow* pViewportWindow, UINT vk);
static BOOL ViewportWindow_HandleTextOverlayChar(ViewportWindow* pViewportWindow, WCHAR ch);

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
BOOL ViewportWindow_OnChar(ViewportWindow* pViewportWindow, WCHAR ch, int cRepeat, UINT flags);
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
    return pViewportWindow && pViewportWindow->pszTextOverlay != NULL;
}

BOOL ViewportWindow_BeginTextOverlay(ViewportWindow* pViewportWindow, int xCanvas, int yCanvas, uint32_t color)
{
    if (!pViewportWindow || ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return FALSE;
    }

    if (!ViewportWindow_EnsureTextOverlayCapacity(pViewportWindow, 1))
    {
        return FALSE;
    }

    pViewportWindow->pszTextOverlay[0] = L'\0';
    pViewportWindow->ptTextOverlayCanvas.x = xCanvas;
    pViewportWindow->ptTextOverlayCanvas.y = yCanvas;
    pViewportWindow->textOverlayColor = color;
    pViewportWindow->nTextOverlayFontDocPx = PanitentApp_GetTextToolFontPx(PanitentApp_Instance());
    pViewportWindow->nTextOverlayFontClientPx = 0;
    pViewportWindow->nTextOverlayLineAdvanceClient = 0;
    pViewportWindow->nTextOverlayPreferredColumn = 0;
    pViewportWindow->cchTextOverlay = 0;
    pViewportWindow->nTextOverlayCaret = 0;
    pViewportWindow->bTextOverlayCaretVisible = TRUE;
    SetRectEmpty(&pViewportWindow->rcTextOverlayClient);

    ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    SetFocus(hWndViewport);
    if (pViewportWindow->uTextOverlayCaretTimerId == 0)
    {
        pViewportWindow->uTextOverlayCaretTimerId =
            SetTimer(hWndViewport, VIEWPORT_TEXT_CARET_TIMER_ID, VIEWPORT_TEXT_CARET_INTERVAL_MS, NULL);
    }
    Window_Invalidate((Window*)pViewportWindow);
    return TRUE;
}

void ViewportWindow_RefreshTextOverlayStyle(ViewportWindow* pViewportWindow)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    pViewportWindow->nTextOverlayFontDocPx = PanitentApp_GetTextToolFontPx(PanitentApp_Instance());
    pViewportWindow->nTextOverlayFontClientPx = 0;
    if (pViewportWindow->hFontTextOverlay)
    {
        DeleteObject(pViewportWindow->hFontTextOverlay);
        pViewportWindow->hFontTextOverlay = NULL;
    }

    ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
    Window_Invalidate((Window*)pViewportWindow);
}

void ViewportWindow_CommitTextOverlay(ViewportWindow* pViewportWindow)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    if (pViewportWindow->cchTextOverlay > 0 && pViewportWindow->pszTextOverlay)
    {
        ViewportWindow_RenderTextOverlayToCanvas(pViewportWindow, pViewportWindow->pszTextOverlay);
    }

    ViewportWindow_DestroyTextOverlayState(pViewportWindow);
}

void ViewportWindow_CancelTextOverlay(ViewportWindow* pViewportWindow)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    ViewportWindow_DestroyTextOverlayState(pViewportWindow);
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
        Window_Invalidate((Window*)pViewportWindow);
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

        if (ViewportWindow_HasTextOverlay(pViewportWindow))
        {
            ViewportWindow_DrawTextOverlay(pViewportWindow, hdcTarget);
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
    StringCchCopyW(
        lf.lfFaceName,
        ARRAYSIZE(lf.lfFaceName),
        PanitentApp_GetTextToolFontFace(PanitentApp_Instance()));
    return CreateFontIndirectW(&lf);
}

static void ViewportWindow_DestroyTextOverlayState(ViewportWindow* pViewportWindow)
{
    if (!pViewportWindow)
    {
        return;
    }

    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    if (hWndViewport && pViewportWindow->uTextOverlayCaretTimerId != 0)
    {
        KillTimer(hWndViewport, VIEWPORT_TEXT_CARET_TIMER_ID);
        pViewportWindow->uTextOverlayCaretTimerId = 0;
    }

    if (pViewportWindow->hFontTextOverlay)
    {
        DeleteObject(pViewportWindow->hFontTextOverlay);
        pViewportWindow->hFontTextOverlay = NULL;
    }

    free(pViewportWindow->pszTextOverlay);
    pViewportWindow->pszTextOverlay = NULL;
    pViewportWindow->cchTextOverlay = 0;
    pViewportWindow->cchTextOverlayCapacity = 0;
    pViewportWindow->nTextOverlayCaret = 0;
    pViewportWindow->nTextOverlayPreferredColumn = -1;
    pViewportWindow->nTextOverlayFontClientPx = 0;
    pViewportWindow->nTextOverlayLineAdvanceClient = 0;
    pViewportWindow->bTextOverlayCaretVisible = FALSE;
    SetRectEmpty(&pViewportWindow->rcTextOverlayClient);

    Window_Invalidate((Window*)pViewportWindow);
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
            while (*pLineEnd && *pLineEnd != L'\n')
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
            pLine = pLineEnd + 1;
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
        while (*pLineEnd && *pLineEnd != L'\n')
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
        pLine = pLineEnd + 1;
    }
}

static BOOL ViewportWindow_EnsureTextOverlayCapacity(ViewportWindow* pViewportWindow, size_t cchRequired)
{
    if (!pViewportWindow)
    {
        return FALSE;
    }

    if (cchRequired <= pViewportWindow->cchTextOverlayCapacity)
    {
        return TRUE;
    }

    size_t cchNewCapacity = pViewportWindow->cchTextOverlayCapacity ? pViewportWindow->cchTextOverlayCapacity : 32;
    while (cchNewCapacity < cchRequired)
    {
        cchNewCapacity *= 2;
    }

    WCHAR* pszNew = (WCHAR*)realloc(pViewportWindow->pszTextOverlay, cchNewCapacity * sizeof(WCHAR));
    if (!pszNew)
    {
        return FALSE;
    }

    pViewportWindow->pszTextOverlay = pszNew;
    pViewportWindow->cchTextOverlayCapacity = cchNewCapacity;
    return TRUE;
}

static BOOL ViewportWindow_IsTextToolActive(void)
{
    Tool* pTool = PanitentApp_GetTool(PanitentApp_Instance());
    return pTool && pTool->pszLabel && wcscmp(pTool->pszLabel, L"Text") == 0;
}

static size_t ViewportWindow_GetTextOverlayLineStart(PCWSTR pszText, size_t nLineIndex)
{
    size_t nLine = 0;
    size_t i = 0;
    if (!pszText)
    {
        return 0;
    }

    while (pszText[i] != L'\0' && nLine < nLineIndex)
    {
        if (pszText[i] == L'\n')
        {
            ++nLine;
            if (nLine == nLineIndex)
            {
                return i + 1;
            }
        }
        ++i;
    }

    return (nLineIndex == 0) ? 0 : i;
}

static size_t ViewportWindow_GetTextOverlayLineEnd(PCWSTR pszText, size_t nLineStart)
{
    size_t i = nLineStart;
    if (!pszText)
    {
        return 0;
    }

    while (pszText[i] != L'\0' && pszText[i] != L'\n')
    {
        ++i;
    }

    return i;
}

static size_t ViewportWindow_GetTextOverlayLineIndex(PCWSTR pszText, size_t nCaret)
{
    size_t nLine = 0;
    if (!pszText)
    {
        return 0;
    }

    for (size_t i = 0; pszText[i] != L'\0' && i < nCaret; ++i)
    {
        if (pszText[i] == L'\n')
        {
            ++nLine;
        }
    }

    return nLine;
}

static size_t ViewportWindow_GetTextOverlayColumn(PCWSTR pszText, size_t nCaret)
{
    size_t nLineStart = ViewportWindow_GetTextOverlayLineStart(
        pszText,
        ViewportWindow_GetTextOverlayLineIndex(pszText, nCaret));
    return nCaret - nLineStart;
}

static size_t ViewportWindow_GetTextOverlayCaretForLineColumn(PCWSTR pszText, size_t nLineIndex, size_t nColumn)
{
    size_t nStart = ViewportWindow_GetTextOverlayLineStart(pszText, nLineIndex);
    size_t nEnd = ViewportWindow_GetTextOverlayLineEnd(pszText, nStart);
    return min(nStart + nColumn, nEnd);
}

static void ViewportWindow_UpdateTextOverlayLayout(ViewportWindow* pViewportWindow)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
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
    }

    HDC hdc = GetDC(hWndViewport);
    SIZE textSize = { 0 };
    int lineAdvance = fontClientPx;
    ViewportWindow_GetTextLayoutMetrics(hdc, pViewportWindow->hFontTextOverlay, pViewportWindow->pszTextOverlay, &textSize, &lineAdvance);
    ReleaseDC(hWndViewport, hdc);

    pViewportWindow->nTextOverlayLineAdvanceClient = lineAdvance;
    int width = max(VIEWPORT_TEXT_MIN_WIDTH, textSize.cx) + VIEWPORT_TEXT_PADDING_X * 2;
    int height = max(lineAdvance + VIEWPORT_TEXT_PADDING_Y * 2, textSize.cy + VIEWPORT_TEXT_PADDING_Y * 2);
    POINT ptClient = { 0 };
    ViewportWindow_CanvasToClient(
        pViewportWindow,
        pViewportWindow->ptTextOverlayCanvas.x,
        pViewportWindow->ptTextOverlayCanvas.y,
        &ptClient);

    pViewportWindow->rcTextOverlayClient.left = ptClient.x;
    pViewportWindow->rcTextOverlayClient.top = ptClient.y;
    pViewportWindow->rcTextOverlayClient.right = ptClient.x + width;
    pViewportWindow->rcTextOverlayClient.bottom = ptClient.y + height;
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

static BOOL ViewportWindow_GetTextOverlayCaretClientPoint(ViewportWindow* pViewportWindow, POINT* pPoint, int* pHeight)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return FALSE;
    }

    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    HDC hdc = GetDC(hWndViewport);
    HGDIOBJ hOldFont = NULL;
    if (pViewportWindow->hFontTextOverlay)
    {
        hOldFont = SelectObject(hdc, pViewportWindow->hFontTextOverlay);
    }

    size_t nLineIndex = ViewportWindow_GetTextOverlayLineIndex(pViewportWindow->pszTextOverlay, pViewportWindow->nTextOverlayCaret);
    size_t nLineStart = ViewportWindow_GetTextOverlayLineStart(pViewportWindow->pszTextOverlay, nLineIndex);
    int cchPrefix = (int)(pViewportWindow->nTextOverlayCaret - nLineStart);
    SIZE prefixSize = { 0 };
    if (cchPrefix > 0)
    {
        GetTextExtentPoint32W(hdc, pViewportWindow->pszTextOverlay + nLineStart, cchPrefix, &prefixSize);
    }

    if (hOldFont)
    {
        SelectObject(hdc, hOldFont);
    }
    ReleaseDC(hWndViewport, hdc);

    if (pPoint)
    {
        pPoint->x = pViewportWindow->rcTextOverlayClient.left + VIEWPORT_TEXT_PADDING_X + prefixSize.cx;
        pPoint->y = pViewportWindow->rcTextOverlayClient.top + VIEWPORT_TEXT_PADDING_Y + (int)nLineIndex * pViewportWindow->nTextOverlayLineAdvanceClient;
    }
    if (pHeight)
    {
        *pHeight = pViewportWindow->nTextOverlayLineAdvanceClient;
    }

    return TRUE;
}

static BOOL ViewportWindow_IsPointInTextOverlay(ViewportWindow* pViewportWindow, int x, int y)
{
    POINT pt = { x, y };
    return ViewportWindow_HasTextOverlay(pViewportWindow) && PtInRect(&pViewportWindow->rcTextOverlayClient, pt);
}

static void ViewportWindow_SetTextOverlayCaret(ViewportWindow* pViewportWindow, size_t nCaret, BOOL fKeepPreferredColumn)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    pViewportWindow->nTextOverlayCaret = min(nCaret, pViewportWindow->cchTextOverlay);
    if (!fKeepPreferredColumn)
    {
        pViewportWindow->nTextOverlayPreferredColumn =
            (int)ViewportWindow_GetTextOverlayColumn(pViewportWindow->pszTextOverlay, pViewportWindow->nTextOverlayCaret);
    }
    pViewportWindow->bTextOverlayCaretVisible = TRUE;
    Window_Invalidate((Window*)pViewportWindow);
}

static void ViewportWindow_MoveTextOverlayCaretFromPoint(ViewportWindow* pViewportWindow, int xClient, int yClient)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    HDC hdc = GetDC(Window_GetHWND((Window*)pViewportWindow));
    HGDIOBJ hOldFont = NULL;
    if (pViewportWindow->hFontTextOverlay)
    {
        hOldFont = SelectObject(hdc, pViewportWindow->hFontTextOverlay);
    }

    int localY = max(0, yClient - pViewportWindow->rcTextOverlayClient.top - VIEWPORT_TEXT_PADDING_Y);
    size_t nLineIndex = (size_t)(localY / max(1, pViewportWindow->nTextOverlayLineAdvanceClient));
    size_t nStart = ViewportWindow_GetTextOverlayLineStart(pViewportWindow->pszTextOverlay, nLineIndex);
    size_t nEnd = ViewportWindow_GetTextOverlayLineEnd(pViewportWindow->pszTextOverlay, nStart);
    int localX = max(0, xClient - pViewportWindow->rcTextOverlayClient.left - VIEWPORT_TEXT_PADDING_X);
    size_t nBestCaret = nStart;
    int nBestDistance = INT_MAX;

    for (size_t i = nStart; i <= nEnd; ++i)
    {
        SIZE prefixSize = { 0 };
        int cchPrefix = (int)(i - nStart);
        if (cchPrefix > 0)
        {
            GetTextExtentPoint32W(hdc, pViewportWindow->pszTextOverlay + nStart, cchPrefix, &prefixSize);
        }

        int nDistance = abs(localX - prefixSize.cx);
        if (nDistance < nBestDistance)
        {
            nBestDistance = nDistance;
            nBestCaret = i;
        }
    }

    if (hOldFont)
    {
        SelectObject(hdc, hOldFont);
    }
    ReleaseDC(Window_GetHWND((Window*)pViewportWindow), hdc);

    ViewportWindow_SetTextOverlayCaret(pViewportWindow, nBestCaret, FALSE);
}

static void ViewportWindow_InsertTextOverlayChars(ViewportWindow* pViewportWindow, PCWSTR pszText, size_t cchText)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow) || !pszText || cchText == 0)
    {
        return;
    }

    size_t cchNewLength = pViewportWindow->cchTextOverlay + cchText;
    if (!ViewportWindow_EnsureTextOverlayCapacity(pViewportWindow, cchNewLength + 1))
    {
        return;
    }

    memmove(
        pViewportWindow->pszTextOverlay + pViewportWindow->nTextOverlayCaret + cchText,
        pViewportWindow->pszTextOverlay + pViewportWindow->nTextOverlayCaret,
        (pViewportWindow->cchTextOverlay - pViewportWindow->nTextOverlayCaret + 1) * sizeof(WCHAR));
    memcpy(
        pViewportWindow->pszTextOverlay + pViewportWindow->nTextOverlayCaret,
        pszText,
        cchText * sizeof(WCHAR));

    pViewportWindow->cchTextOverlay = cchNewLength;
    ViewportWindow_SetTextOverlayCaret(pViewportWindow, pViewportWindow->nTextOverlayCaret + cchText, FALSE);
    ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
}

static void ViewportWindow_DeleteTextOverlayRange(ViewportWindow* pViewportWindow, size_t nStart, size_t nEnd)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow) || nStart >= nEnd || nStart >= pViewportWindow->cchTextOverlay)
    {
        return;
    }

    nEnd = min(nEnd, pViewportWindow->cchTextOverlay);
    memmove(
        pViewportWindow->pszTextOverlay + nStart,
        pViewportWindow->pszTextOverlay + nEnd,
        (pViewportWindow->cchTextOverlay - nEnd + 1) * sizeof(WCHAR));
    pViewportWindow->cchTextOverlay -= (nEnd - nStart);
    ViewportWindow_SetTextOverlayCaret(pViewportWindow, nStart, FALSE);
    ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);
}

static BOOL ViewportWindow_HandleTextOverlayKeyDown(ViewportWindow* pViewportWindow, UINT vk)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return FALSE;
    }

    switch (vk)
    {
    case VK_ESCAPE:
        ViewportWindow_CancelTextOverlay(pViewportWindow);
        return TRUE;

    case VK_RETURN:
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            ViewportWindow_CommitTextOverlay(pViewportWindow);
            return TRUE;
        }
        return FALSE;

    case VK_LEFT:
        if (pViewportWindow->nTextOverlayCaret > 0)
        {
            ViewportWindow_SetTextOverlayCaret(pViewportWindow, pViewportWindow->nTextOverlayCaret - 1, FALSE);
        }
        return TRUE;

    case VK_RIGHT:
        if (pViewportWindow->nTextOverlayCaret < pViewportWindow->cchTextOverlay)
        {
            ViewportWindow_SetTextOverlayCaret(pViewportWindow, pViewportWindow->nTextOverlayCaret + 1, FALSE);
        }
        return TRUE;

    case VK_HOME:
    {
        size_t nLine = ViewportWindow_GetTextOverlayLineIndex(pViewportWindow->pszTextOverlay, pViewportWindow->nTextOverlayCaret);
        ViewportWindow_SetTextOverlayCaret(
            pViewportWindow,
            ViewportWindow_GetTextOverlayLineStart(pViewportWindow->pszTextOverlay, nLine),
            FALSE);
        return TRUE;
    }

    case VK_END:
    {
        size_t nLine = ViewportWindow_GetTextOverlayLineIndex(pViewportWindow->pszTextOverlay, pViewportWindow->nTextOverlayCaret);
        size_t nStart = ViewportWindow_GetTextOverlayLineStart(pViewportWindow->pszTextOverlay, nLine);
        ViewportWindow_SetTextOverlayCaret(
            pViewportWindow,
            ViewportWindow_GetTextOverlayLineEnd(pViewportWindow->pszTextOverlay, nStart),
            FALSE);
        return TRUE;
    }

    case VK_UP:
    case VK_DOWN:
    {
        size_t nLine = ViewportWindow_GetTextOverlayLineIndex(pViewportWindow->pszTextOverlay, pViewportWindow->nTextOverlayCaret);
        size_t nLineCount = 1;
        for (size_t i = 0; i < pViewportWindow->cchTextOverlay; ++i)
        {
            if (pViewportWindow->pszTextOverlay[i] == L'\n')
            {
                ++nLineCount;
            }
        }

        if (pViewportWindow->nTextOverlayPreferredColumn < 0)
        {
            pViewportWindow->nTextOverlayPreferredColumn =
                (int)ViewportWindow_GetTextOverlayColumn(pViewportWindow->pszTextOverlay, pViewportWindow->nTextOverlayCaret);
        }

        if (vk == VK_UP && nLine > 0)
        {
            ViewportWindow_SetTextOverlayCaret(
                pViewportWindow,
                ViewportWindow_GetTextOverlayCaretForLineColumn(
                    pViewportWindow->pszTextOverlay,
                    nLine - 1,
                    (size_t)pViewportWindow->nTextOverlayPreferredColumn),
                TRUE);
        }
        else if (vk == VK_DOWN && nLine + 1 < nLineCount)
        {
            ViewportWindow_SetTextOverlayCaret(
                pViewportWindow,
                ViewportWindow_GetTextOverlayCaretForLineColumn(
                    pViewportWindow->pszTextOverlay,
                    nLine + 1,
                    (size_t)pViewportWindow->nTextOverlayPreferredColumn),
                TRUE);
        }
        return TRUE;
    }

    case VK_DELETE:
        if (pViewportWindow->nTextOverlayCaret < pViewportWindow->cchTextOverlay)
        {
            ViewportWindow_DeleteTextOverlayRange(
                pViewportWindow,
                pViewportWindow->nTextOverlayCaret,
                pViewportWindow->nTextOverlayCaret + 1);
        }
        return TRUE;

    case VK_SPACE:
        return TRUE;
    }

    return FALSE;
}

static BOOL ViewportWindow_HandleTextOverlayChar(ViewportWindow* pViewportWindow, WCHAR ch)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return FALSE;
    }

    switch (ch)
    {
    case L'\b':
        if (pViewportWindow->nTextOverlayCaret > 0)
        {
            ViewportWindow_DeleteTextOverlayRange(
                pViewportWindow,
                pViewportWindow->nTextOverlayCaret - 1,
                pViewportWindow->nTextOverlayCaret);
        }
        return TRUE;

    case L'\r':
        if (!(GetKeyState(VK_CONTROL) & 0x8000))
        {
            WCHAR szNewline = L'\n';
            ViewportWindow_InsertTextOverlayChars(pViewportWindow, &szNewline, 1);
        }
        return TRUE;

    case L'\t':
        return TRUE;

    default:
        break;
    }

    if (ch >= 0x20)
    {
        ViewportWindow_InsertTextOverlayChars(pViewportWindow, &ch, 1);
        return TRUE;
    }

    return FALSE;
}

static void ViewportWindow_DrawTextOverlay(ViewportWindow* pViewportWindow, HDC hdcTarget)
{
    if (!ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        return;
    }

    ViewportWindow_UpdateTextOverlayLayout(pViewportWindow);

    HGDIOBJ hOldFont = NULL;
    HGDIOBJ hOldPen = NULL;
    HGDIOBJ hOldBrush = NULL;

    if (pViewportWindow->hFontTextOverlay)
    {
        hOldFont = SelectObject(hdcTarget, pViewportWindow->hFontTextOverlay);
    }

    HPEN hPen = CreatePen(PS_DOT, 1, RGB(120, 120, 120));
    hOldPen = SelectObject(hdcTarget, hPen);
    hOldBrush = SelectObject(hdcTarget, GetStockObject(HOLLOW_BRUSH));
    Rectangle(
        hdcTarget,
        pViewportWindow->rcTextOverlayClient.left,
        pViewportWindow->rcTextOverlayClient.top,
        pViewportWindow->rcTextOverlayClient.right,
        pViewportWindow->rcTextOverlayClient.bottom);

    SelectObject(hdcTarget, hOldBrush);
    SelectObject(hdcTarget, hOldPen);
    DeleteObject(hPen);

    SetBkMode(hdcTarget, TRANSPARENT);
    SetTextColor(
        hdcTarget,
        RGB(
            CHANNEL_R_32(pViewportWindow->textOverlayColor),
            CHANNEL_G_32(pViewportWindow->textOverlayColor),
            CHANNEL_B_32(pViewportWindow->textOverlayColor)));

    int nSavedDc = SaveDC(hdcTarget);
    IntersectClipRect(
        hdcTarget,
        pViewportWindow->rcTextOverlayClient.left + VIEWPORT_TEXT_PADDING_X,
        pViewportWindow->rcTextOverlayClient.top + VIEWPORT_TEXT_PADDING_Y,
        pViewportWindow->rcTextOverlayClient.right - VIEWPORT_TEXT_PADDING_X,
        pViewportWindow->rcTextOverlayClient.bottom - VIEWPORT_TEXT_PADDING_Y);
    SetViewportOrgEx(
        hdcTarget,
        pViewportWindow->rcTextOverlayClient.left + VIEWPORT_TEXT_PADDING_X,
        pViewportWindow->rcTextOverlayClient.top + VIEWPORT_TEXT_PADDING_Y,
        NULL);
    ViewportWindow_DrawTextLines(hdcTarget, pViewportWindow->pszTextOverlay, pViewportWindow->nTextOverlayLineAdvanceClient);
    RestoreDC(hdcTarget, nSavedDc);

    if (pViewportWindow->bTextOverlayCaretVisible && GetFocus() == Window_GetHWND((Window*)pViewportWindow))
    {
        POINT ptCaret = { 0 };
        int nCaretHeight = 0;
        if (ViewportWindow_GetTextOverlayCaretClientPoint(pViewportWindow, &ptCaret, &nCaretHeight))
        {
            RECT rcCaret = {
                ptCaret.x,
                ptCaret.y,
                ptCaret.x + 1,
                ptCaret.y + max(1, nCaretHeight)
            };
            FillRect(hdcTarget, &rcCaret, GetStockObject(BLACK_BRUSH));
        }
    }

    if (hOldFont)
    {
        SelectObject(hdcTarget, hOldFont);
    }
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
    UNREFERENCED_PARAMETER(cRepeat);
    UNREFERENCED_PARAMETER(flags);
    HWND hWnd = Window_GetHWND((Window *)pViewportWindow);

    if (ViewportWindow_HasTextOverlay(pViewportWindow) &&
        ViewportWindow_HandleTextOverlayKeyDown(pViewportWindow, vk))
    {
        return TRUE;
    }

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
    UNREFERENCED_PARAMETER(cRepeat);
    UNREFERENCED_PARAMETER(flags);
    if (vk == VK_SPACE) {
        pViewportWindow->bDrag = FALSE;
    }

    return FALSE;
}

BOOL ViewportWindow_OnChar(ViewportWindow* pViewportWindow, WCHAR ch, int cRepeat, UINT flags)
{
    UNREFERENCED_PARAMETER(cRepeat);
    UNREFERENCED_PARAMETER(flags);
    return ViewportWindow_HandleTextOverlayChar(pViewportWindow, ch);
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

    if (ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        if (ViewportWindow_IsTextToolActive() && ViewportWindow_IsPointInTextOverlay(pViewportWindow, x, y))
        {
            ViewportWindow_MoveTextOverlayCaretFromPoint(pViewportWindow, x, y);
            return;
        }

        ViewportWindow_CommitTextOverlay(pViewportWindow);
    }

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
    case WM_KILLFOCUS:
        if (ViewportWindow_HasTextOverlay(pViewportWindow))
        {
            ViewportWindow_CommitTextOverlay(pViewportWindow);
            return 0;
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

    case WM_CHAR:
    {
        BOOL bProcessed = ViewportWindow_OnChar(pViewportWindow, (WCHAR)wParam, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam));
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

    case WM_TIMER:
        if (wParam == VIEWPORT_TEXT_CARET_TIMER_ID && ViewportWindow_HasTextOverlay(pViewportWindow))
        {
            pViewportWindow->bTextOverlayCaretVisible = !pViewportWindow->bTextOverlayCaretVisible;
            Window_Invalidate((Window*)pViewportWindow);
            return 0;
        }
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
