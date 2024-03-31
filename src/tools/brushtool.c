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
void BrushTool_OnMouseMove(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

BrushTool* BrushTool_Create()
{
    BrushTool* pBrushTool = (BrushTool*)calloc(1, sizeof(BrushTool));
    BrushTool_Init(pBrushTool);
    return pBrushTool;
}

void BrushTool_Init(BrushTool* pBrushTool)
{
    pBrushTool->base.pszLabel = L"Brush";
    pBrushTool->base.img = 12;
    pBrushTool->base.OnLButtonDown = BrushTool_OnLButtonDown;
    pBrushTool->base.OnLButtonUp = BrushTool_OnLButtonUp;
    pBrushTool->base.OnMouseMove = BrushTool_OnMouseMove;
}

void BrushTool_OnLButtonUp(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    pBrushTool->fDraw = FALSE;
    ReleaseCapture();
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
    Brush_Delete(g_pBrushDraw);
}

void BrushTool_OnLButtonDown(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    // SetCursor(LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_BRUSH)));

    HWND hWndViewport = Window_GetHWND(pViewportWindow);
    SetClassLongPtr(hWndViewport, GCLP_HCURSOR, (LONG_PTR)LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_BRUSH)));

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    pBrushTool->fDraw = TRUE;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pBrushTool->prev.x = ptCanvas.x;
    pBrushTool->prev.y = ptCanvas.y;

    g_pBrushDraw = BrushBuilder_Build(g_pBrush, g_brushSize);

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    Brush_Draw(g_pBrushDraw, ptCanvas.x, ptCanvas.y, pCanvas, g_color_context.fg_color);

    Window_Invalidate((Window*)pViewportWindow);
}

void BrushTool_OnMouseMove(BrushTool* pBrushTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    

    if (pBrushTool->fDraw)
    {
        POINT ptCanvas = { 0 };
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
        Canvas* pCanvas = Document_GetCanvas(pDocument);

        Brush_DrawTo(g_pBrushDraw, pBrushTool->prev.x, pBrushTool->prev.y, ptCanvas.x, ptCanvas.y, pCanvas, g_color_context.fg_color);

        pBrushTool->prev.x = ptCanvas.x;
        pBrushTool->prev.y = ptCanvas.y;

        Window_Invalidate((Window*)pViewportWindow);
    }
}
