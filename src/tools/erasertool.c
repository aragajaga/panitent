#include "../precomp.h"

#include "../brush.h"
#include "erasertool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../color_context.h"
#include "../history.h"
#include "../resource.h"

Brush* g_pBrushDraw;

EraserTool* EraserTool_Create();
void EraserTool_Init(EraserTool* pEraserTool);
void EraserTool_OnLButtonUp(EraserTool* pEraserTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void EraserTool_OnLButtonDown(EraserTool* pEraserTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void EraserTool_OnMouseMove(EraserTool* pEraserTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

EraserTool* EraserTool_Create()
{
    EraserTool* pEraserTool = (EraserTool*)malloc(sizeof(EraserTool));
    memset(pEraserTool, 0, sizeof(EraserTool));
    EraserTool_Init(pEraserTool);
    return pEraserTool;
}

void EraserTool_Init(EraserTool* pEraserTool)
{
    pEraserTool->base.pszLabel = L"Eraser";
    pEraserTool->base.img = 5;
    pEraserTool->base.OnLButtonDown = EraserTool_OnLButtonDown;
    pEraserTool->base.OnLButtonUp = EraserTool_OnLButtonUp;
    pEraserTool->base.OnMouseMove = EraserTool_OnMouseMove;
}

void EraserTool_OnLButtonUp(EraserTool* pEraserTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);

    pEraserTool->fDraw = FALSE;
    ReleaseCapture();
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
    Brush_Delete(g_pBrushDraw);
    g_pBrushDraw = NULL;
}

void EraserTool_OnLButtonDown(EraserTool* pEraserTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    // SetCursor(LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_BRUSH)));

    HWND hWndViewport = Window_GetHWND(pViewportWindow);
    SetClassLongPtr(hWndViewport, GCLP_HCURSOR, (LONG_PTR)LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_BRUSH)));

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    pEraserTool->fDraw = TRUE;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pEraserTool->prev.x = ptCanvas.x;
    pEraserTool->prev.y = ptCanvas.y;

    InitializeBrushList();
    g_pBrushDraw = BrushBuilder_Build(g_pBrush, g_brushSize);
    if (!g_pBrushDraw)
    {
        pEraserTool->fDraw = FALSE;
        ReleaseCapture();
        return;
    }

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    Brush_Draw(g_pBrushDraw, ptCanvas.x, ptCanvas.y, pCanvas, 0x00);

    Window_Invalidate((Window*)pViewportWindow);
}

void EraserTool_OnMouseMove(EraserTool* pEraserTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    if (pEraserTool->fDraw && g_pBrushDraw)
    {
        POINT ptCanvas = { 0 };
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
        Canvas* pCanvas = Document_GetCanvas(pDocument);

        Brush_DrawTo(g_pBrushDraw, pEraserTool->prev.x, pEraserTool->prev.y, ptCanvas.x, ptCanvas.y, pCanvas, 0x00);

        pEraserTool->prev.x = ptCanvas.x;
        pEraserTool->prev.y = ptCanvas.y;

        Window_Invalidate((Window*)pViewportWindow);
    }
}
