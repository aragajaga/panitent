#include "../precomp.h"

#include "texttool.h"

#include "../viewport.h"
#include "../canvas.h"
#include "../color_context.h"
#include "../panitent.h"
#include "../document.h"
#include "../history.h"

TextTool* TextTool_Create();
void TextTool_Init(TextTool* pTextTool);
void TextTool_OnLButtonUp(TextTool* pTextTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);

TextTool* TextTool_Create()
{
    TextTool* pTextTool = (TextTool*)malloc(sizeof(TextTool));
    memset(pTextTool, 0, sizeof(TextTool));
    TextTool_Init(pTextTool);
    return pTextTool;
}

void TextTool_Init(TextTool* pTextTool)
{
    pTextTool->base.pszLabel = L"Text";
    pTextTool->base.img = 6;
    pTextTool->base.OnLButtonUp = TextTool_OnLButtonUp;
}

void TextTool_OnLButtonUp(TextTool* pTextTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags)
{
    POINT ptCanvas = { 0 };
    ViewportWindow_ClientToCanvas(pViewportWindow, x, y, &ptCanvas);

    History_StartDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);

    HDC hdcViewport = GetDC(hWndViewport);
    HDC hdcBitmap = CreateCompatibleDC(hdcViewport);

    wchar_t textbuf[] = L"В чащах юга жил бы цитрус? Да, но фальшивый экземпляр.";
    size_t textLen = 0;
    /*
    GetWindowText(g_option_bar.textstring_handle,
                  textbuf,
                  sizeof(textbuf) / sizeof(wchar_t)); */
    textLen = wcslen(textbuf);

    HFONT hFont = CreateFont(24, 0, 0, 0, 600, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
        DEFAULT_PITCH, L"Times New Roman");
    SelectObject(hdcBitmap, hFont);

    SIZE size = { 0 };
    GetTextExtentPoint32(hdcBitmap, textbuf, (int)textLen, &size);
    RECT textrc = { 0, 0, size.cx, size.cy };

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size.cx;
    bmi.bmiHeader.biHeight = size.cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    uint32_t* buffer = NULL;

    HBITMAP hbmText = CreateDIBSection(hdcBitmap, &bmi, 0, (LPVOID*)&buffer, NULL, 0);
    assert(hbmText != NULL);
    assert(buffer != NULL);

    HGDIOBJ hOldObj = SelectObject(hdcBitmap, hbmText);

    SetTextColor(hdcBitmap, 0x00FFFFFF);
    SetBkColor(hdcBitmap, 0x00000000);
    SetBkMode(hdcBitmap, OPAQUE);
    
    DrawTextEx(hdcBitmap, textbuf, -1, &textrc, 0, NULL);
    
    SelectObject(hdcBitmap, hOldObj);

    for (int y = 0; y < size.cy; y++)
    {
        for (int x = 0; x < size.cx; x++)
        {
            uint32_t* pixel = buffer + (size_t)y * (size_t)size.cx + (size_t)x;
            *pixel = *pixel | 0xFF000000;
        }
    }

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    Canvas* pCanvas = Document_GetCanvas(pDocument);

    Canvas_ColorStencilBits(pCanvas, buffer, ptCanvas.x, ptCanvas.y, size.cx, size.cy, g_color_context.fg_color);

    DeleteObject(hbmText);
    DeleteDC(hdcBitmap);

    History_FinalizeDifferentiation(ViewportWindow_GetDocument(pViewportWindow));

    Window_Invalidate((Window *)pViewportWindow);
}
