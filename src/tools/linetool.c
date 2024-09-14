#include "../precomp.h"

#include "linetool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../history.h"
#include "../grimstroke/shapecontext.h"
#include "../panitentapp.h"

LineTool* LineTool_Create();
void LineTool_Init(LineTool* pLineTool);
void LineTool_OnLButtonDown(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void LineTool_OnLButtonUp(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

LineTool* LineTool_Create()
{
    LineTool* pLineTool = (LineTool*)malloc(sizeof(LineTool));
    memset(pLineTool, 0, sizeof(LineTool));
    LineTool_Init(pLineTool);
    return pLineTool;
}

void LineTool_Init(LineTool* pLineTool)
{
    pLineTool->base.pszLabel = L"Line";
    pLineTool->base.img = 3;
    pLineTool->base.OnLButtonUp = LineTool_OnLButtonUp;
    pLineTool->base.OnLButtonDown = LineTool_OnLButtonDown;
}

void LineTool_OnLButtonDown(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    pLineTool->fDraw = TRUE;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pLineTool->prev.x = ptCanvas.x;
    pLineTool->prev.y = ptCanvas.y;
}

void LineTool_OnLButtonUp(LineTool* pLineTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    if (pLineTool->fDraw)
    {
        POINT ptCanvas = { 0 };
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
        Canvas* pCanvas = Document_GetCanvas(pDocument);

        ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());
        ShapeContext_DrawLine(pShapeContext, pLineTool->prev.x, pLineTool->prev.y, ptCanvas.x, ptCanvas.y);
    }
    pLineTool->fDraw = FALSE;
    ReleaseCapture();

    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}
