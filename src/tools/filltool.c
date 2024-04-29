#include "../precomp.h"

#include "filltool.h"

#include "../viewport.h"
#include "../color_context.h"
#include "../document.h"
#include "../canvas.h"
#include "../util.h"
#include "../history.h"

BOOL g_fillMutex;

PNTQUEUE_DECLARE_TYPE(POINT)

FillTool* FillTool_Create();
void FillTool_Init(FillTool* pFillTool);
void FillTool_DoFloodFill(FillTool* pFillTool, ViewportWindow* pViewportWindow, int x, int y, uint32_t newColor);
void FillTool_OnRButtonUp(FillTool* pFillTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void FillTool_OnLButtonUp(FillTool* pFillTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

FillTool* FillTool_Create()
{
    FillTool* pFillTool = (FillTool*)malloc(sizeof(FillTool));
    memset(pFillTool, 0, sizeof(FillTool));
    FillTool_Init(pFillTool);
    return pFillTool;
}

void FillTool_Init(FillTool* pFillTool)
{
    pFillTool->base.pszLabel = L"Flood fill";
    pFillTool->base.img = 8;
    pFillTool->base.OnLButtonUp = FillTool_OnLButtonUp;
    pFillTool->base.OnRButtonUp = FillTool_OnRButtonUp;
}

void FillTool_DoFloodFill(FillTool* pFillTool, ViewportWindow* pViewportWindow, int x, int y, uint32_t newColor)
{
    if (g_fillMutex)
    {
        return;
    }

    g_fillMutex = TRUE;

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    if (!pCanvas)
    {
        return;
    }

    uint32_t oldColor = Canvas_GetPixel(pCanvas, x, y);
    POINT nextpt = { x, y };

    pntqueue(POINT) queue;
    pntqueue_init(POINT)(&queue);
    pntqueue_push(POINT)(&queue, nextpt);

    while (queue.length)
    {
        LPPOINT ppt = NULL;

        POINT pt = pntqueue_pop(POINT)(&queue);
        ppt = &pt;

        if (Canvas_CheckBoundaries(pCanvas, ppt->x + 1, ppt->y) && Canvas_GetPixel(pCanvas, ppt->x + 1, ppt->y) == oldColor)
        {
            Canvas_SetPixel(pCanvas, ppt->x + 1, ppt->y, newColor);
            nextpt.x = ppt->x + 1;
            nextpt.y = ppt->y;
            pntqueue_push(POINT)(&queue, nextpt);
        }

        if (Canvas_CheckBoundaries(pCanvas, ppt->x - 1, ppt->y) && Canvas_GetPixel(pCanvas, ppt->x - 1, ppt->y) == oldColor)
        {
            Canvas_SetPixel(pCanvas, ppt->x - 1, ppt->y, newColor);
            nextpt.x = ppt->x - 1;
            nextpt.y = ppt->y;
            pntqueue_push(POINT)(&queue, nextpt);
        }

        if (Canvas_CheckBoundaries(pCanvas, ppt->x, ppt->y + 1) && Canvas_GetPixel(pCanvas, ppt->x, ppt->y + 1) == oldColor)
        {
            Canvas_SetPixel(pCanvas, ppt->x, ppt->y + 1, newColor);
            nextpt.x = ppt->x;
            nextpt.y = ppt->y + 1;
            pntqueue_push(POINT)(&queue, nextpt);
        }

        if (Canvas_CheckBoundaries(pCanvas, ppt->x, ppt->y - 1) && Canvas_GetPixel(pCanvas, ppt->x, ppt->y - 1) == oldColor)
        {
            Canvas_SetPixel(pCanvas, ppt->x, ppt->y - 1, newColor);
            nextpt.x = ppt->x;
            nextpt.y = ppt->y - 1;
            pntqueue_push(POINT)(&queue, nextpt);
        }
    }

    pntqueue_delete(POINT)(&queue);
    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    g_fillMutex = FALSE;
}

void FillTool_OnRButtonUp(FillTool* pFillTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    FillTool_DoFloodFill(pFillTool, pViewportWindow, ptCanvas.x, ptCanvas.y, g_color_context.bg_color);
    Window_Invalidate((Window*)pViewportWindow);
}

void FillTool_OnLButtonUp(FillTool* pFillTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    FillTool_DoFloodFill(pFillTool, pViewportWindow, ptCanvas.x, ptCanvas.y, g_color_context.fg_color);
    Window_Invalidate((Window*)pViewportWindow);
}
