#include "../precomp.h"

#include "rectangletool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../history.h"
#include "../grimstroke/shapecontext.h"
#include "../panitentapp.h"
#include "../color_context.h"

RectangleTool* RectangleTool_Create();
void RectangleTool_Init(RectangleTool* pRectangleTool);
void RectangleTool_OnLButtonDown(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void RectangleTool_OnLButtonUp(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void RectangleTool_OnRButtonDown(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void RectangleTool_OnRButtonUp(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
static void RectangleTool_BeginStroke(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t strokeColor, uint32_t fillColor);
static void RectangleTool_EndStroke(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y);
static void RectangleTool_FillRect(Canvas* pCanvas, const RECT* pRect, uint32_t color);

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
}

void RectangleTool_OnLButtonDown(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    RectangleTool_BeginStroke(pRectangleTool, pViewportWindow, x, y, keyFlags, g_color_context.fg_color, g_color_context.bg_color);
}

void RectangleTool_OnLButtonUp(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);
    RectangleTool_EndStroke(pRectangleTool, pViewportWindow, x, y);
}

void RectangleTool_OnRButtonDown(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    RectangleTool_BeginStroke(pRectangleTool, pViewportWindow, x, y, keyFlags, g_color_context.bg_color, g_color_context.fg_color);
}

void RectangleTool_OnRButtonUp(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
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

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}

static void RectangleTool_EndStroke(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y)
{
    if (!pRectangleTool->fDraw)
    {
        return;
    }

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    RECT rc = {
        min(pRectangleTool->prev.x, ptCanvas.x),
        min(pRectangleTool->prev.y, ptCanvas.y),
        max(pRectangleTool->prev.x, ptCanvas.x),
        max(pRectangleTool->prev.y, ptCanvas.y)
    };

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());

    if (pCanvas && pShapeContext)
    {
        if (ShapeContext_IsFillEnabled(pShapeContext))
        {
            RectangleTool_FillRect(pCanvas, &rc, pRectangleTool->fillColor);
        }

        if (ShapeContext_IsStrokeEnabled(pShapeContext) &&
            ShapeContext_BeginDraw(pShapeContext, pCanvas, pRectangleTool->strokeColor))
        {
            ShapeContext_DrawLine(pShapeContext, rc.left, rc.top, rc.right, rc.top);
            ShapeContext_DrawLine(pShapeContext, rc.right, rc.top, rc.right, rc.bottom);
            ShapeContext_DrawLine(pShapeContext, rc.right, rc.bottom, rc.left, rc.bottom);
            ShapeContext_DrawLine(pShapeContext, rc.left, rc.bottom, rc.left, rc.top);
            ShapeContext_EndDraw(pShapeContext);
        }

        Window_Invalidate((Window*)pViewportWindow);
    }

    pRectangleTool->fDraw = FALSE;
    ReleaseCapture();
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}

static void RectangleTool_FillRect(Canvas* pCanvas, const RECT* pRect, uint32_t color)
{
    if (!pCanvas || !pRect)
    {
        return;
    }

    for (int y = pRect->top; y <= pRect->bottom; ++y)
    {
        for (int x = pRect->left; x <= pRect->right; ++x)
        {
            Canvas_DrawPixel(pCanvas, x, y, color);
        }
    }
}
