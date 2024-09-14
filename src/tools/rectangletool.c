#include "../precomp.h"

#include "rectangletool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../history.h"
#include "../grimstroke/shapecontext.h"
#include "../panitentapp.h"

RectangleTool* RectangleTool_Create();
void RectangleTool_Init(RectangleTool* pRectangleTool);
void RectangleTool_OnLButtonDown(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void RectangleTool_OnLButtonUp(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

RectangleTool* RectangleTool_Create()
{
    RectangleTool* pRectangleTool = (RectangleTool*)malloc(sizeof(RectangleTool));
    memset(pRectangleTool, 0, sizeof(pRectangleTool));
    RectangleTool_Init(pRectangleTool);
    return pRectangleTool;
}

void RectangleTool_Init(RectangleTool* pRectangleTool)
{
    pRectangleTool->base.pszLabel = L"Rectangle";
    pRectangleTool->base.img = 4;
    pRectangleTool->base.OnLButtonUp = RectangleTool_OnLButtonUp;
    pRectangleTool->base.OnLButtonDown = RectangleTool_OnLButtonDown;
}

void RectangleTool_OnLButtonDown(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    pRectangleTool->fDraw = TRUE;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pRectangleTool->prev.x = ptCanvas.x;
    pRectangleTool->prev.y = ptCanvas.y;

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}

void RectangleTool_OnLButtonUp(RectangleTool* pRectangleTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    if (pRectangleTool->fDraw)
    {
        POINT ptCanvas = { 0 };
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
        Canvas* pCanvas = Document_GetCanvas(pDocument);

        ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());
        ShapeContext_DrawLine(pShapeContext, pRectangleTool->prev.x, pRectangleTool->prev.y, ptCanvas.x, ptCanvas.y);
    }
    pRectangleTool->fDraw = FALSE;
    ReleaseCapture();

    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}
