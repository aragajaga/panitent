#include "../precomp.h"

#include "pickertool.h"

#include "../document.h"
#include "../canvas.h"
#include "../viewport.h"
#include "../color_context.h"
#include "../resource.h"

PickerTool* PickerTool_Create();
void PickerTool_Init(PickerTool* pPickerTool);
void PickerTool_OnLButtonUp(PickerTool* pPickerTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
void PickerTool_OnRButtonUp(PickerTool* pPickerTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

PickerTool* PickerTool_Create()
{
    PickerTool* pPickerTool = (PickerTool*)malloc(sizeof(PickerTool));
    memset(pPickerTool, 0, sizeof(PickerTool));
    PickerTool_Init(pPickerTool);
    return pPickerTool;
}

void PickerTool_Init(PickerTool* pPickerTool)
{
    pPickerTool->base.pszLabel = L"Color picker";
    pPickerTool->base.img = 9;
    pPickerTool->base.OnLButtonUp = PickerTool_OnLButtonUp;
    pPickerTool->base.OnRButtonUp = PickerTool_OnRButtonUp;
}

void PickerTool_OnLButtonUp(PickerTool* pPickerTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    HWND hWndViewport = Window_GetHWND(pViewportWindow);
    SetClassLongPtr(hWndViewport, GCLP_HCURSOR, (LONG_PTR)LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_PICKER)));

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    uint32_t color = Canvas_GetPixel(pCanvas, ptCanvas.x, ptCanvas.y);
    SetForegroundColor(color);
}

void PickerTool_OnRButtonUp(PickerTool* pPickerTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    uint32_t color = Canvas_GetPixel(pCanvas, ptCanvas.x, ptCanvas.y);
    SetBackgroundColor(color);
}
