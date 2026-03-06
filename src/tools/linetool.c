#include "../precomp.h"

#include "linetool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../history.h"
#include "../grimstroke/shapecontext.h"
#include "../panitentapp.h"
#include "../color_context.h"

LineTool* LineTool_Create();
void LineTool_Init(LineTool* pLineTool);
void LineTool_OnLButtonDown(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void LineTool_OnLButtonUp(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void LineTool_OnRButtonDown(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void LineTool_OnRButtonUp(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
static void LineTool_BeginStroke(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t drawColor);
static void LineTool_EndStroke(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y);

LineTool* LineTool_Create()
{
    LineTool* pLineTool = (LineTool*)malloc(sizeof(LineTool));
    if (pLineTool)
    {
        memset(pLineTool, 0, sizeof(LineTool));
        LineTool_Init(pLineTool);
    }
    return pLineTool;
}

void LineTool_Init(LineTool* pLineTool)
{
    pLineTool->base.pszLabel = L"Line";
    pLineTool->base.img = 3;
    pLineTool->base.OnLButtonUp = LineTool_OnLButtonUp;
    pLineTool->base.OnLButtonDown = LineTool_OnLButtonDown;
    pLineTool->base.OnRButtonUp = LineTool_OnRButtonUp;
    pLineTool->base.OnRButtonDown = LineTool_OnRButtonDown;
}

void LineTool_OnLButtonDown(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    LineTool_BeginStroke(pLineTool, pViewportWindow, x, y, keyFlags, g_color_context.fg_color);
}

void LineTool_OnLButtonUp(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);
    LineTool_EndStroke(pLineTool, pViewportWindow, x, y);
}

void LineTool_OnRButtonDown(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    LineTool_BeginStroke(pLineTool, pViewportWindow, x, y, keyFlags, g_color_context.bg_color);
}

void LineTool_OnRButtonUp(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);
    LineTool_EndStroke(pLineTool, pViewportWindow, x, y);
}

static void LineTool_BeginStroke(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t drawColor)
{
    UNREFERENCED_PARAMETER(keyFlags);

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    pLineTool->fDraw = TRUE;
    pLineTool->drawColor = drawColor;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pLineTool->prev = ptCanvas;
}

static void LineTool_EndStroke(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y)
{
    if (!pLineTool->fDraw)
    {
        return;
    }

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());

    if (pCanvas && pShapeContext && ShapeContext_IsStrokeEnabled(pShapeContext) &&
        ShapeContext_BeginDraw(pShapeContext, pCanvas, pLineTool->drawColor))
    {
        ShapeContext_DrawLine(pShapeContext, pLineTool->prev.x, pLineTool->prev.y, ptCanvas.x, ptCanvas.y);
        ShapeContext_EndDraw(pShapeContext);
        Window_Invalidate((Window*)pViewportWindow);
    }

    pLineTool->fDraw = FALSE;
    ReleaseCapture();
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}
