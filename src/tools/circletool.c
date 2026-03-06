#include "../precomp.h"

#include "circletool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
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
static void CircleTool_Fill(Canvas* pCanvas, POINT center, int radius, uint32_t color);
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

    if (ShapeContext_IsFillEnabled(pShapeContext))
    {
        CircleTool_Fill(pCanvas, pCircleTool->circCenter, radius, pCircleTool->fillColor);
    }

    if (ShapeContext_IsStrokeEnabled(pShapeContext) &&
        ShapeContext_BeginDraw(pShapeContext, pCanvas, pCircleTool->strokeColor))
    {
        ShapeContext_DrawCircle(pShapeContext, pCircleTool->circCenter.x, pCircleTool->circCenter.y, radius);
        ShapeContext_EndDraw(pShapeContext);
    }
}

static void CircleTool_Fill(Canvas* pCanvas, POINT center, int radius, uint32_t color)
{
    if (!pCanvas)
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
            Canvas_DrawPixel(pCanvas, center.x + dx, center.y + dy, color);
        }
    }
}
