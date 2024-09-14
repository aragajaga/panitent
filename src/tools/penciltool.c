#include "../precomp.h"

#include "penciltool.h"

#include "../viewport.h"
#include "../document.h"
#include "../canvas.h"
#include "../history.h"
#include "../grimstroke/shapecontext.h"
#include "../panitentapp.h"

PencilTool* PencilTool_Create();
void PencilTool_Init(PencilTool* pPencilTool);
void PencilTool_OnLButtonDown(PencilTool* pPencilTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void PencilTool_OnLButtonUp(PencilTool* pPencilTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void PencilTool_OnMouseMove(PencilTool* pPencilTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

PencilTool* PencilTool_Create()
{
    PencilTool* pPencilTool = (PencilTool*)malloc(sizeof(PencilTool));
    memset(pPencilTool, 0, sizeof(PencilTool));
    PencilTool_Init(pPencilTool);
    return pPencilTool;
}

void PencilTool_Init(PencilTool* pPencilTool)
{
    pPencilTool->base.pszLabel = L"Pencil";
    pPencilTool->base.img = 1;
    pPencilTool->base.OnLButtonUp = PencilTool_OnLButtonUp;
    pPencilTool->base.OnLButtonDown = PencilTool_OnLButtonDown;
    pPencilTool->base.OnMouseMove = PencilTool_OnMouseMove;
}

void PencilTool_OnLButtonDown(PencilTool* pPencilTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    pPencilTool->fDraw = TRUE;
    SetCapture(Window_GetHWND((Window*)pViewportWindow));
    pPencilTool->prev.x = ptCanvas.x;
    pPencilTool->prev.y = ptCanvas.y;
}

void PencilTool_OnLButtonUp(PencilTool* pPencilTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    if (pPencilTool->fDraw)
    {
        POINT ptCanvas = { 0 };
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
        Canvas* pCanvas = Document_GetCanvas(pDocument);

        Canvas_DrawLine(pCanvas, pPencilTool->prev.x, pPencilTool->prev.y, ptCanvas.x, ptCanvas.y);

#ifdef PEN_OVERLAY
        ReleaseDC(hViewport, hdc);
#endif
    }
    pPencilTool->fDraw = FALSE;
    ReleaseCapture();

    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));
}

void PencilTool_OnMouseMove(PencilTool* pPencilTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    if (pPencilTool->fDraw)
    {
        POINT ptCanvas = { 0 };
        ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

        /* Draw on canvas */
        Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
        Canvas* pCanvas = Document_GetCanvas(pDocument);

        Canvas_DrawLine(pCanvas, pPencilTool->prev.x, pPencilTool->prev.y, ptCanvas.x, ptCanvas.y);

        pPencilTool->prev.x = ptCanvas.x;
        pPencilTool->prev.y = ptCanvas.y;

#ifdef PEN_OVERLAY
        ReleaseDC(hViewport, hdc);
#endif
    }
}
