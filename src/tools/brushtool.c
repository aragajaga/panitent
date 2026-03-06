#include "../precomp.h"

#include "../brush.h"
#include "brushtool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../color_context.h"
#include "../history.h"
#include "../resource.h"

Brush* g_pBrushDraw;

BrushTool* BrushTool_Create();
void BrushTool_Init(BrushTool* pBrushTool);
void BrushTool_OnLButtonUp(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void BrushTool_OnLButtonDown(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void BrushTool_OnRButtonUp(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void BrushTool_OnRButtonDown(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void BrushTool_OnMouseMove(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
static void BrushTool_BeginStroke(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t drawColor);
static void BrushTool_EndStroke(BrushTool* pBrushTool, ViewportWindow* pViewportWindow);

BrushTool* BrushTool_Create()
{
    BrushTool* pBrushTool = (BrushTool*)malloc(sizeof(BrushTool));
    memset(pBrushTool, 0, sizeof(BrushTool));
    BrushTool_Init(pBrushTool);
    return pBrushTool;
}

void BrushTool_Init(BrushTool* pBrushTool)
{
    pBrushTool->base.pszLabel = L"Brush";
    pBrushTool->base.img = 12;
    pBrushTool->base.OnLButtonDown = BrushTool_OnLButtonDown;
    pBrushTool->base.OnLButtonUp = BrushTool_OnLButtonUp;
    pBrushTool->base.OnRButtonDown = BrushTool_OnRButtonDown;
    pBrushTool->base.OnRButtonUp = BrushTool_OnRButtonUp;
    pBrushTool->base.OnMouseMove = BrushTool_OnMouseMove;
}

void BrushTool_OnLButtonUp(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);

    BrushTool_EndStroke(pBrushTool, pViewportWindow);
}

void BrushTool_OnLButtonDown(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    BrushTool_BeginStroke(pBrushTool, pViewportWindow, x, y, keyFlags, g_color_context.fg_color);
}

void BrushTool_OnRButtonUp(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);

    BrushTool_EndStroke(pBrushTool, pViewportWindow);
}

void BrushTool_OnRButtonDown(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    BrushTool_BeginStroke(pBrushTool, pViewportWindow, x, y, keyFlags, g_color_context.bg_color);
}

void BrushTool_OnMouseMove(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    if (pBrushTool->fDraw && g_pBrushDraw)
    {
        POINT ptCanvas = { 0 };
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
        Canvas* pCanvas = Document_GetCanvas(pDocument);

        Brush_DrawTo(g_pBrushDraw, pBrushTool->prev.x, pBrushTool->prev.y, ptCanvas.x, ptCanvas.y, pCanvas, pBrushTool->drawColor);

        pBrushTool->prev.x = ptCanvas.x;
        pBrushTool->prev.y = ptCanvas.y;

        Window_Invalidate((Window*)pViewportWindow);
    }
}

static void BrushTool_BeginStroke(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t drawColor)
{
    UNREFERENCED_PARAMETER(keyFlags);

    HWND hWndViewport = Window_GetHWND(pViewportWindow);
    SetClassLongPtr(hWndViewport, GCLP_HCURSOR, (LONG_PTR)LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_BRUSH)));

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    pBrushTool->fDraw = TRUE;
    pBrushTool->drawColor = drawColor;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pBrushTool->prev.x = ptCanvas.x;
    pBrushTool->prev.y = ptCanvas.y;

    InitializeBrushList();
    g_pBrushDraw = BrushBuilder_Build(g_pBrush, g_brushSize);
    if (!g_pBrushDraw)
    {
        pBrushTool->fDraw = FALSE;
        ReleaseCapture();
        History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
        return;
    }

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    Brush_Draw(g_pBrushDraw, ptCanvas.x, ptCanvas.y, pCanvas, pBrushTool->drawColor);

    Window_Invalidate((Window*)pViewportWindow);
}

static void BrushTool_EndStroke(BrushTool* pBrushTool, ViewportWindow* pViewportWindow)
{
    if (!pBrushTool->fDraw && !g_pBrushDraw)
    {
        return;
    }

    pBrushTool->fDraw = FALSE;
    ReleaseCapture();
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
    Brush_Delete(g_pBrushDraw);
    g_pBrushDraw = NULL;
}
