#include "../precomp.h"

#include "texttool.h"

#include "../viewport.h"
#include "../color_context.h"
#include "../document.h"

static void TextTool_InitImpl(TextTool* pTextTool);
static void TextTool_OnLButtonDown(TextTool* pTextTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

TextTool* TextTool_Create()
{
    TextTool* pTextTool = (TextTool*)malloc(sizeof(TextTool));
    if (!pTextTool)
    {
        return NULL;
    }

    memset(pTextTool, 0, sizeof(TextTool));
    TextTool_InitImpl(pTextTool);
    return pTextTool;
}

static void TextTool_InitImpl(TextTool* pTextTool)
{
    pTextTool->base.pszLabel = L"Text";
    pTextTool->base.img = 6;
    pTextTool->base.OnLButtonDown = (void(*)(Tool*, ViewportWindow*, int, int, UINT))TextTool_OnLButtonDown;
}

void TextTool_Init(TextTool* pTextTool)
{
    TextTool_InitImpl(pTextTool);
}

static void TextTool_OnLButtonDown(TextTool* pTextTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(pTextTool);
    UNREFERENCED_PARAMETER(keyFlags);

    if (!pViewportWindow)
    {
        return;
    }

    if (!ViewportWindow_GetDocument(pViewportWindow))
    {
        return;
    }

    if (ViewportWindow_HasTextOverlay(pViewportWindow))
    {
        ViewportWindow_CommitTextOverlay(pViewportWindow);
        return;
    }

    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);
    ViewportWindow_BeginTextOverlay(pViewportWindow, ptCanvas.x, ptCanvas.y, g_color_context.fg_color);
}
