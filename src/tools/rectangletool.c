#include "../precomp.h"

#include "rectangletool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../alphamask.h"
#include "../history.h"
#include "../grimstroke/shapecontext.h"
#include "../panitentapp.h"
#include "../color_context.h"

RectangleTool* RectangleTool_Create();
void RectangleTool_Init(RectangleTool* pRectangleTool);
void RectangleTool_OnLButtonDown(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void RectangleTool_OnLButtonUp(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void RectangleTool_OnRButtonDown(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void RectangleTool_OnRButtonUp(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void RectangleTool_OnMouseMove(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
BOOL RectangleTool_HasPreview(Tool* pTool);
void RectangleTool_DrawPreview(Tool* pTool, ViewportWindow* pViewportWindow, Canvas* pCanvas);
static void RectangleTool_BeginStroke(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t strokeColor, uint32_t fillColor);
static void RectangleTool_EndStroke(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y);
static void RectangleTool_FillRect(AlphaMask* pMask, const RECT* pRect);
static void RectangleTool_Render(RectangleTool* pRectangleTool, Canvas* pCanvas, ShapeContext* pShapeContext, POINT endPoint);

RectangleTool* RectangleTool_Create()
{
    RectangleTool* pRectangleTool = (RectangleTool*)malloc(sizeof(RectangleTool));
    if (pRectangleTool)
    {
        memset(pRectangleTool, 0, sizeof(RectangleTool));
        RectangleTool_Init(pRectangleTool);
    }
    return pRectangleTool;
}

void RectangleTool_Init(RectangleTool* pRectangleTool)
{
    pRectangleTool->base.pszLabel = L"Rectangle";
    pRectangleTool->base.img = 4;
    pRectangleTool->base.OnLButtonUp = RectangleTool_OnLButtonUp;
    pRectangleTool->base.OnLButtonDown = RectangleTool_OnLButtonDown;
    pRectangleTool->base.OnRButtonUp = RectangleTool_OnRButtonUp;
    pRectangleTool->base.OnRButtonDown = RectangleTool_OnRButtonDown;
    pRectangleTool->base.OnMouseMove = RectangleTool_OnMouseMove;
    pRectangleTool->base.HasPreview = RectangleTool_HasPreview;
    pRectangleTool->base.DrawPreview = RectangleTool_DrawPreview;
}

void RectangleTool_OnLButtonDown(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    RectangleTool* pRectangleTool = (RectangleTool*)pTool;
    RectangleTool_BeginStroke(pRectangleTool, pViewportWindow, x, y, keyFlags, g_color_context.fg_color, g_color_context.bg_color);
}

void RectangleTool_OnLButtonUp(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    RectangleTool* pRectangleTool = (RectangleTool*)pTool;
    UNREFERENCED_PARAMETER(keyFlags);
    RectangleTool_EndStroke(pRectangleTool, pViewportWindow, x, y);
}

void RectangleTool_OnRButtonDown(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    RectangleTool* pRectangleTool = (RectangleTool*)pTool;
    RectangleTool_BeginStroke(pRectangleTool, pViewportWindow, x, y, keyFlags, g_color_context.bg_color, g_color_context.fg_color);
}

void RectangleTool_OnRButtonUp(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    RectangleTool* pRectangleTool = (RectangleTool*)pTool;
    UNREFERENCED_PARAMETER(keyFlags);
    RectangleTool_EndStroke(pRectangleTool, pViewportWindow, x, y);
}

static void RectangleTool_BeginStroke(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t strokeColor, uint32_t fillColor)
{
    UNREFERENCED_PARAMETER(keyFlags);

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    pRectangleTool->fDraw = TRUE;
    pRectangleTool->strokeColor = strokeColor;
    pRectangleTool->fillColor = fillColor;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pRectangleTool->prev = ptCanvas;
    pRectangleTool->current = ptCanvas;

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}

void RectangleTool_OnMouseMove(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    RectangleTool* pRectangleTool = (RectangleTool*)pTool;
    UNREFERENCED_PARAMETER(keyFlags);

    if (!pRectangleTool->fDraw)
    {
        return;
    }

    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &pRectangleTool->current);
    Window_Invalidate((Window*)pViewportWindow);
}

static void RectangleTool_EndStroke(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y)
{
    if (!pRectangleTool->fDraw)
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
        RectangleTool_Render(pRectangleTool, pCanvas, pShapeContext, ptCanvas);
        Window_Invalidate((Window*)pViewportWindow);
    }

    pRectangleTool->fDraw = FALSE;
    ReleaseCapture();
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}

BOOL RectangleTool_HasPreview(Tool* pTool)
{
    RectangleTool* pRectangleTool = (RectangleTool*)pTool;
    return pRectangleTool && pRectangleTool->fDraw;
}

void RectangleTool_DrawPreview(Tool* pTool, ViewportWindow* pViewportWindow, Canvas* pCanvas)
{
    RectangleTool* pRectangleTool = (RectangleTool*)pTool;
    UNREFERENCED_PARAMETER(pViewportWindow);

    if (!RectangleTool_HasPreview(pTool))
    {
        return;
    }

    ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());
    if (!pShapeContext)
    {
        return;
    }

    RectangleTool_Render(pRectangleTool, pCanvas, pShapeContext, pRectangleTool->current);
}

static void RectangleTool_Render(RectangleTool* pRectangleTool, Canvas* pCanvas, ShapeContext* pShapeContext, POINT endPoint)
{
    RECT rc = {
        min(pRectangleTool->prev.x, endPoint.x),
        min(pRectangleTool->prev.y, endPoint.y),
        max(pRectangleTool->prev.x, endPoint.x),
        max(pRectangleTool->prev.y, endPoint.y)
    };

    int pad = ShapeContext_GetStrokeWidth(pShapeContext) + 2;
    RECT rcBounds = {
        rc.left - pad,
        rc.top - pad,
        rc.right + pad,
        rc.bottom + pad
    };
    int width = rcBounds.right - rcBounds.left + 1;
    int height = rcBounds.bottom - rcBounds.top + 1;
    RECT rcLocal = {
        rc.left - rcBounds.left,
        rc.top - rcBounds.top,
        rc.right - rcBounds.left,
        rc.bottom - rcBounds.top
    };

    if (ShapeContext_IsFillEnabled(pShapeContext))
    {
        AlphaMask* pFillMask = AlphaMask_Create(width, height);
        if (pFillMask)
        {
            RectangleTool_FillRect(pFillMask, &rcLocal);
            Canvas_ColorStencilMask(pCanvas, rcBounds.left, rcBounds.top, pFillMask, pRectangleTool->fillColor);
            AlphaMask_Delete(pFillMask);
        }
    }

    if (ShapeContext_IsStrokeEnabled(pShapeContext))
    {
        AlphaMask* pStrokeMask = AlphaMask_Create(width, height);
        if (pStrokeMask && ShapeContext_BeginMaskDraw(pShapeContext, pStrokeMask))
        {
            ShapeContext_DrawLine(pShapeContext, rcLocal.left, rcLocal.top, rcLocal.right, rcLocal.top);
            ShapeContext_DrawLine(pShapeContext, rcLocal.right, rcLocal.top, rcLocal.right, rcLocal.bottom);
            ShapeContext_DrawLine(pShapeContext, rcLocal.right, rcLocal.bottom, rcLocal.left, rcLocal.bottom);
            ShapeContext_DrawLine(pShapeContext, rcLocal.left, rcLocal.bottom, rcLocal.left, rcLocal.top);
            ShapeContext_EndDraw(pShapeContext);
            Canvas_ColorStencilMask(pCanvas, rcBounds.left, rcBounds.top, pStrokeMask, pRectangleTool->strokeColor);
        }
        AlphaMask_Delete(pStrokeMask);
    }
}

static void RectangleTool_FillRect(AlphaMask* pMask, const RECT* pRect)
{
    if (!pMask || !pRect)
    {
        return;
    }
    AlphaMask_FillRect(pMask, pRect, 0xFF);
}
