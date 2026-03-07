#include "../precomp.h"

#include "circletool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../alphamask.h"
#include "../history.h"
#include "../grimstroke/shapecontext.h"
#include "../panitentapp.h"
#include "../util.h"
#include "../color_context.h"

CircleTool* CircleTool_Create();
void CircleTool_Init(CircleTool* pCircleTool);
void CircleTool_OnLButtonDown(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void CircleTool_OnLButtonUp(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void CircleTool_OnRButtonDown(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void CircleTool_OnRButtonUp(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void CircleTool_OnMouseMove(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
BOOL CircleTool_HasPreview(CircleTool* pCircleTool);
void CircleTool_DrawPreview(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, Canvas* pCanvas);
static void CircleTool_BeginStroke(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t strokeColor, uint32_t fillColor);
static void CircleTool_EndStroke(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y);
static void CircleTool_Fill(AlphaMask* pMask, POINT center, int radius);
static void CircleTool_Render(CircleTool* pCircleTool, Canvas* pCanvas, ShapeContext* pShapeContext, POINT edgePoint);

CircleTool* CircleTool_Create()
{
    CircleTool* pCircleTool = (CircleTool*)malloc(sizeof(CircleTool));
    if (pCircleTool)
    {
        memset(pCircleTool, 0, sizeof(CircleTool));
        CircleTool_Init(pCircleTool);
    }
    return pCircleTool;
}

void CircleTool_Init(CircleTool* pCircleTool)
{
    pCircleTool->base.pszLabel = L"Circle";
    pCircleTool->base.img = 2;
    pCircleTool->base.OnLButtonUp = CircleTool_OnLButtonUp;
    pCircleTool->base.OnLButtonDown = CircleTool_OnLButtonDown;
    pCircleTool->base.OnRButtonUp = CircleTool_OnRButtonUp;
    pCircleTool->base.OnRButtonDown = CircleTool_OnRButtonDown;
    pCircleTool->base.OnMouseMove = CircleTool_OnMouseMove;
    pCircleTool->base.HasPreview = (BOOL(*)(Tool*))CircleTool_HasPreview;
    pCircleTool->base.DrawPreview = (void(*)(Tool*, ViewportWindow*, Canvas*))CircleTool_DrawPreview;
}

void CircleTool_OnLButtonDown(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    CircleTool_BeginStroke(pCircleTool, pViewportWindow, x, y, keyFlags, g_color_context.fg_color, g_color_context.bg_color);
}

void CircleTool_OnLButtonUp(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);
    CircleTool_EndStroke(pCircleTool, pViewportWindow, x, y);
}

void CircleTool_OnRButtonDown(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    CircleTool_BeginStroke(pCircleTool, pViewportWindow, x, y, keyFlags, g_color_context.bg_color, g_color_context.fg_color);
}

void CircleTool_OnRButtonUp(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);
    CircleTool_EndStroke(pCircleTool, pViewportWindow, x, y);
}

static void CircleTool_BeginStroke(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags, uint32_t strokeColor, uint32_t fillColor)
{
    UNREFERENCED_PARAMETER(keyFlags);

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    pCircleTool->fDraw = TRUE;
    pCircleTool->strokeColor = strokeColor;
    pCircleTool->fillColor = fillColor;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pCircleTool->circCenter = ptCanvas;
    pCircleTool->current = ptCanvas;
}

void CircleTool_OnMouseMove(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    if (!pCircleTool->fDraw)
    {
        return;
    }

    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &pCircleTool->current);
    Window_Invalidate((Window*)pViewportWindow);
}

static void CircleTool_EndStroke(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y)
{
    if (!pCircleTool->fDraw)
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
        CircleTool_Render(pCircleTool, pCanvas, pShapeContext, ptCanvas);
        Window_Invalidate((Window*)pViewportWindow);
    }

    pCircleTool->fDraw = FALSE;
    ReleaseCapture();
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}

BOOL CircleTool_HasPreview(CircleTool* pCircleTool)
{
    return pCircleTool && pCircleTool->fDraw;
}

void CircleTool_DrawPreview(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, Canvas* pCanvas)
{
    UNREFERENCED_PARAMETER(pViewportWindow);

    if (!CircleTool_HasPreview(pCircleTool))
    {
        return;
    }

    ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());
    if (!pShapeContext)
    {
        return;
    }

    CircleTool_Render(pCircleTool, pCanvas, pShapeContext, pCircleTool->current);
}

static void CircleTool_Render(CircleTool* pCircleTool, Canvas* pCanvas, ShapeContext* pShapeContext, POINT edgePoint)
{
    int radius = 0;
    if (!float2int_s(&radius, sqrtf(powf((float)edgePoint.x - pCircleTool->circCenter.x, 2.f) + powf((float)edgePoint.y - pCircleTool->circCenter.y, 2.f))))
    {
        radius = 0;
    }

    int pad = ShapeContext_GetStrokeWidth(pShapeContext) + 2;
    RECT rcBounds = {
        pCircleTool->circCenter.x - radius - pad,
        pCircleTool->circCenter.y - radius - pad,
        pCircleTool->circCenter.x + radius + pad,
        pCircleTool->circCenter.y + radius + pad
    };
    int width = rcBounds.right - rcBounds.left + 1;
    int height = rcBounds.bottom - rcBounds.top + 1;
    POINT localCenter = {
        pCircleTool->circCenter.x - rcBounds.left,
        pCircleTool->circCenter.y - rcBounds.top
    };

    if (ShapeContext_IsFillEnabled(pShapeContext))
    {
        AlphaMask* pFillMask = AlphaMask_Create(width, height);
        if (pFillMask)
        {
            CircleTool_Fill(pFillMask, localCenter, radius);
            Canvas_ColorStencilMask(pCanvas, rcBounds.left, rcBounds.top, pFillMask, pCircleTool->fillColor);
            AlphaMask_Delete(pFillMask);
        }
    }

    if (ShapeContext_IsStrokeEnabled(pShapeContext))
    {
        AlphaMask* pStrokeMask = AlphaMask_Create(width, height);
        if (pStrokeMask && ShapeContext_BeginMaskDraw(pShapeContext, pStrokeMask))
        {
            ShapeContext_DrawCircle(pShapeContext, localCenter.x, localCenter.y, radius);
            ShapeContext_EndDraw(pShapeContext);
            Canvas_ColorStencilMask(pCanvas, rcBounds.left, rcBounds.top, pStrokeMask, pCircleTool->strokeColor);
        }
        AlphaMask_Delete(pStrokeMask);
    }
}

static void CircleTool_Fill(AlphaMask* pMask, POINT center, int radius)
{
    if (!pMask)
    {
        return;
    }

    for (int dy = -radius; dy <= radius; ++dy)
    {
        int span = 0;
        if (!float2int_s(&span, floorf(sqrtf((float)(radius * radius - dy * dy)))))
        {
            continue;
        }

        for (int dx = -span; dx <= span; ++dx)
        {
            AlphaMask_SetMax(pMask, center.x + dx, center.y + dy, 0xFF);
        }
    }
}
