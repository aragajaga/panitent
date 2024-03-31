#include "../precomp.h"

#include "circletool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../history.h"
#include "../primitives_context.h"
#include "../util.h"

CircleTool* CircleTool_Create();
void CircleTool_Init(CircleTool* pCircleTool);
void CircleTool_OnLButtonDown(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void CircleTool_OnLButtonUp(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

CircleTool* CircleTool_Create()
{
    CircleTool* pCircleTool = (CircleTool*)calloc(1, sizeof(CircleTool));
    CircleTool_Init(pCircleTool);
    return pCircleTool;
}

void CircleTool_Init(CircleTool* pCircleTool)
{
    pCircleTool->base.pszLabel = L"Circle";
    pCircleTool->base.img = 2;
    pCircleTool->base.OnLButtonUp = CircleTool_OnLButtonUp;
    pCircleTool->base.OnLButtonDown = CircleTool_OnLButtonDown;
}

void CircleTool_OnLButtonDown(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    pCircleTool->fDraw = TRUE;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pCircleTool->circCenter.x = ptCanvas.x;
    pCircleTool->circCenter.y = ptCanvas.y;
}

void CircleTool_OnLButtonUp(CircleTool* pCircleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    if (pCircleTool->fDraw)
    {
        POINT ptCanvas = { 0 };
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        int radius = 0;
        if (!float2int_s(&radius, sqrtf(powf((float)ptCanvas.x - pCircleTool->circCenter.x, 2.f) + powf((float)ptCanvas.y - pCircleTool->circCenter.y, 2.f))))
        {
            return;
        }

        Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
        Canvas* pCanvas = Document_GetCanvas(pDocument);
        draw_circle(pCanvas, pCircleTool->circCenter.x, pCircleTool->circCenter.y, radius);
    }
    pCircleTool->fDraw = FALSE;
    ReleaseCapture();

    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}
