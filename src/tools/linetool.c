#include "../precomp.h"

#include "linetool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../alphamask.h"
#include "../history.h"
#include "../grimstroke/shapecontext.h"
#include "../panitentapp.h"
#include "../color_context.h"

LineTool* LineTool_Create();
void LineTool_Init(LineTool* pLineTool);
void LineTool_OnLButtonDown(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void LineTool_OnLButtonUp(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void LineTool_OnRButtonDown(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void LineTool_OnRButtonUp(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void LineTool_OnMouseMove(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
BOOL LineTool_HasPreview(Tool* pTool);
void LineTool_DrawPreview(Tool* pTool, ViewportWindow* pViewportWindow, Canvas* pCanvas);
static void LineTool_BeginStroke(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t drawColor);
static void LineTool_EndStroke(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y);
static void LineTool_Render(LineTool* pLineTool, Canvas* pCanvas, ShapeContext* pShapeContext, POINT endPoint);

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
    pLineTool->base.OnMouseMove = LineTool_OnMouseMove;
    pLineTool->base.HasPreview = LineTool_HasPreview;
    pLineTool->base.DrawPreview = LineTool_DrawPreview;
}

void LineTool_OnLButtonDown(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    LineTool* pLineTool = (LineTool*)pTool;
    LineTool_BeginStroke(pLineTool, pViewportWindow, x, y, keyFlags, g_color_context.fg_color);
}

void LineTool_OnLButtonUp(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    LineTool* pLineTool = (LineTool*)pTool;
    UNREFERENCED_PARAMETER(keyFlags);
    LineTool_EndStroke(pLineTool, pViewportWindow, x, y);
}

void LineTool_OnRButtonDown(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    LineTool* pLineTool = (LineTool*)pTool;
    LineTool_BeginStroke(pLineTool, pViewportWindow, x, y, keyFlags, g_color_context.bg_color);
}

void LineTool_OnRButtonUp(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    LineTool* pLineTool = (LineTool*)pTool;
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
    pLineTool->current = ptCanvas;
}

void LineTool_OnMouseMove(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    LineTool* pLineTool = (LineTool*)pTool;
    UNREFERENCED_PARAMETER(keyFlags);

    if (!pLineTool->fDraw)
    {
        return;
    }

    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &pLineTool->current);
    Window_Invalidate((Window*)pViewportWindow);
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

    if (pCanvas && pShapeContext)
    {
        LineTool_Render(pLineTool, pCanvas, pShapeContext, ptCanvas);
        Window_Invalidate((Window*)pViewportWindow);
    }

    pLineTool->fDraw = FALSE;
    ReleaseCapture();
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}

BOOL LineTool_HasPreview(Tool* pTool)
{
    LineTool* pLineTool = (LineTool*)pTool;
    return pLineTool && pLineTool->fDraw;
}

void LineTool_DrawPreview(Tool* pTool, ViewportWindow* pViewportWindow, Canvas* pCanvas)
{
    LineTool* pLineTool = (LineTool*)pTool;
    UNREFERENCED_PARAMETER(pViewportWindow);

    if (!LineTool_HasPreview(pTool))
    {
        return;
    }

    ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());
    if (!pShapeContext)
    {
        return;
    }

    LineTool_Render(pLineTool, pCanvas, pShapeContext, pLineTool->current);
}

static void LineTool_Render(LineTool* pLineTool, Canvas* pCanvas, ShapeContext* pShapeContext, POINT endPoint)
{
    if (!pLineTool || !pCanvas || !pShapeContext)
    {
        return;
    }

    if (!ShapeContext_IsStrokeEnabled(pShapeContext))
    {
        return;
    }

    int pad = ShapeContext_GetStrokeWidth(pShapeContext) + 2;
    RECT rcBounds = {
        min(pLineTool->prev.x, endPoint.x) - pad,
        min(pLineTool->prev.y, endPoint.y) - pad,
        max(pLineTool->prev.x, endPoint.x) + pad,
        max(pLineTool->prev.y, endPoint.y) + pad
    };

    AlphaMask* pMask = AlphaMask_Create(rcBounds.right - rcBounds.left + 1, rcBounds.bottom - rcBounds.top + 1);
    if (!pMask)
    {
        return;
    }

    if (ShapeContext_BeginMaskDraw(pShapeContext, pMask))
    {
        ShapeContext_DrawLine(pShapeContext,
            pLineTool->prev.x - rcBounds.left,
            pLineTool->prev.y - rcBounds.top,
            endPoint.x - rcBounds.left,
            endPoint.y - rcBounds.top);
        ShapeContext_EndDraw(pShapeContext);
        Canvas_ColorStencilMask(pCanvas, rcBounds.left, rcBounds.top, pMask, pLineTool->drawColor);
    }

    AlphaMask_Delete(pMask);
}
