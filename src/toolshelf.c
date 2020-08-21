#include "precomp.h"

#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <math.h>
#include <stdio.h>

#include "primitives_context.h"
#include "option_bar.h"
#include "toolshelf.h"
#include "viewport.h"
#include "canvas.h"
#include "debug.h"
#include "tool.h"

extern VIEWPORT vp;

typedef struct _tagPNTTOOL
{
    LPWSTR szLabel;
    UINT iBmpIndex;
} PNTTOOL;

typedef struct _tagTOOLSHELF
{
    PNTTOOL *pTools;
    unsigned int nCount;
} TOOLSHELF;

TOOLSHELF tsh;

PNTTOOL tool;
TOOL pencil;
TOOL pointer;
TOOL circle;
TOOL line;
TOOL rectangle;
TOOL text;

static BOOL fDraw = FALSE;
static POINT prev;

void ToolShelf_AddTool(TOOLSHELF *tsh, PNTTOOL tool)
{
    tsh->pTools[(size_t)tsh->nCount] = tool;
    tsh->nCount++;
}

void ToolShelf_RemoveTool(TOOLSHELF *tsh)
{
    if (tsh->nCount)
        tsh->nCount--;
}

HBITMAP hbmTool;

void InitializeToolShelf(TOOLSHELF *tsh)
{
    Pointer_Init();
    Pencil_Init();
    Circle_Init();
    Line_Init();
    Rectangle_Init();
    Text_Init();
    vp.tool = pointer;

    hbmTool = (HBITMAP)LoadImage(GetModuleHandle(NULL),
            L"tool.bmp",
            IMAGE_BITMAP,
            0, 0,
            LR_LOADFROMFILE);

    tsh->pTools = calloc(sizeof(TOOLSHELF), 8);
    tsh->nCount = 0;

    PNTTOOL tPointer;
    tPointer.szLabel = L"Pointer";
    tPointer.iBmpIndex = 0;
    ToolShelf_AddTool(tsh, tPointer);

    PNTTOOL tPencil;
    tPencil.szLabel = L"Pencil";
    tPencil.iBmpIndex = 1;
    ToolShelf_AddTool(tsh, tPencil);

    PNTTOOL tCircle;
    tCircle.szLabel = L"Circle";
    tCircle.iBmpIndex = 2;
    ToolShelf_AddTool(tsh, tCircle);

    PNTTOOL tLine;
    tLine.szLabel = L"Line";
    tLine.iBmpIndex = 3;
    ToolShelf_AddTool(tsh, tLine);

    PNTTOOL tRectangle;
    tRectangle.szLabel = L"Rectangle";
    tRectangle.iBmpIndex = 4; //-V112
    ToolShelf_AddTool(tsh, tRectangle);
    
    PNTTOOL tText;
    tText.szLabel = L"Text";
    tText.iBmpIndex = 6;
    ToolShelf_AddTool(tsh, tText);
}

HWND hwndToolShelf;
HTHEME hTheme = NULL;
int btnSize = 24;
int btnOffset = 4;

void draw_button(HDC hdc, int x, int y)
{
    if (!hTheme)
    {
        HWND hButton = CreateWindowEx(0, L"BUTTON", L"", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        hTheme = OpenThemeData(hButton, L"BUTTON");
    }

    RECT rc = {x, y, x + btnSize, y + btnSize};
    DrawThemeBackgroundEx(hTheme, hdc, BP_PUSHBUTTON, PBS_NORMAL, &rc, NULL);
}

void ToolShelf_DrawButtons(HWND hwnd, HDC hdc)
{
    BITMAP bitmap;
    HDC hdcMem;
    HGDIOBJ oldBitmap;

    hdcMem = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(hdcMem, hbmTool);
    GetObject(hbmTool, sizeof(bitmap), &bitmap);

    RECT rcClient = {0};
    GetClientRect(hwnd, &rcClient);

    size_t extSize = btnSize + btnOffset;
    size_t rowCount = rcClient.right / btnSize;

    if (rowCount < 1)
    {
      rowCount = 1;
    }

    for (size_t i = 0; i < tsh.nCount; i++)
    {

        int x = btnOffset + (i % rowCount) * extSize;
        int y = btnOffset + (i / rowCount) * extSize;

        // Rectangle(hdc, x, y, x+btnSize, y+btnSize);
        draw_button(hdc, x, y);

        int iBmp = tsh.pTools[(ptrdiff_t)i].iBmpIndex;
        const int offset = 4;
        TransparentBlt(hdc,
                x+offset,   y+offset,
                16,         bitmap.bmHeight,
                hdcMem,
                16*iBmp,    0,
                16,         bitmap.bmHeight,
                (UINT)0x00FF00FF);
    }

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);
}

void ToolShelf_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    hdc = BeginPaint(hwnd, &ps);

    ToolShelf_DrawButtons(hwnd, hdc);

    EndPaint(hwnd, &ps);
}

void ToolShelf_OnLButtonUp(MOUSEEVENT mEvt)
{
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);

    int btnHot = btnSize + 5;

    if (x >= 0
        && y >= 0
        && x < btnHot*2
        && y < (int)tsh.nCount*btnHot)
    {
        size_t extSize = btnSize + btnOffset; 

        RECT rcClient = {0};
        GetClientRect(mEvt.hwnd, &rcClient);

        size_t rowCount = rcClient.right / extSize;
        if (rowCount < 1)
        {
          rowCount = 1;
        }

        unsigned int bID = (y-btnOffset) / extSize *
            rowCount + (x-btnOffset) / extSize;

        if (tsh.nCount > bID)
        {
            printf("[ToolButton] %d\n", bID);

            /* Может использовать массив указателей? */
            switch (bID)
            {
            case 1:
                vp.tool = pencil;
                break;
            case 2:
                vp.tool = circle;
                break;
            case 3:
                vp.tool = line;
                break;
            case 4:
                vp.tool = rectangle;
                break;
            case 5: 
                vp.tool = text;
                break;
            default:
                vp.tool = pointer;
                break;
            }
        }
    }
}


LRESULT CALLBACK ToolShelfWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MOUSEEVENT mevt;
    mevt.hwnd = hwnd;
    mevt.lParam = lParam;

    switch (msg) {
    case WM_CREATE:
        hwndToolShelf = hwnd;
        InitializeToolShelf(&tsh);
        break;
    case WM_PAINT:
        ToolShelf_OnPaint(hwnd);
        break;
    case WM_LBUTTONUP:
        ToolShelf_OnLButtonUp(mevt);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void RegisterToolShelf()
{
    WNDCLASS wc = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = ToolShelfWndProc;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH) (COLOR_BTNFACE + 1);
    wc.lpszClassName    = TOOLSHELF_WC;

    RegisterClass(&wc);
}

void Pointer_OnLButtonDown(MOUSEEVENT mEvt)
{
}

void Pointer_OnLButtonUp(MOUSEEVENT mEvt)
{
}

void Pointer_OnMouseMove(MOUSEEVENT mEvt)
{
}

void Pointer_Init()
{
    pointer.OnLButtonUp = Pointer_OnLButtonUp;
    pointer.OnLButtonDown = Pointer_OnLButtonDown;
    pointer.OnMouseMove = Pointer_OnMouseMove;
}

void Text_OnLButtonDown(MOUSEEVENT mEvt)
{
}

void Text_OnMouseMove(MOUSEEVENT mEvt)
{
}

void Text_OnLButtonUp(MOUSEEVENT mEvt)
{
  HDC viewportdc = GetDC(g_viewport.win_handle);
  HDC bitmapdc = CreateCompatibleDC(viewportdc);

  wchar_t textbuf[1024];
  GetWindowText(g_option_bar.textstring_handle, textbuf,
      sizeof(textbuf) / sizeof(wchar_t));
    
  SIZE size = {0};
  GetTextExtentPoint32(bitmapdc, textbuf, -1, &size);
  RECT textrc = {0, 0, size.cx, size.cy};

  uint8_t* bmbuf = calloc(size.cx * size.cy, sizeof(uint32_t));

  HBITMAP hbitmap = CreateBitmap(size.cx, size.cy, 1,
      sizeof(uint32_t) * 8, bmbuf);
  SelectObject(bitmapdc, hbitmap);
  DrawTextEx(bitmapdc, textbuf, -1, &textrc, 0, NULL);

  GetDIBits(bitmapdc, hbitmap, 0, 0, NULL, NULL, DIB_RGB_COLORS);

  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);
}

void Text_Init()
{
    text.OnLButtonUp = Text_OnLButtonUp;
    text.OnLButtonDown = Text_OnLButtonDown;
    text.OnMouseMove = Text_OnMouseMove;
}

void Pencil_OnLButtonDown(MOUSEEVENT mEvt)
{
    fDraw = TRUE;
    SetCapture(mEvt.hwnd);
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void Pencil_OnLButtonUp(MOUSEEVENT mEvt)
{
    signed short x = LOWORD(mEvt.lParam);
    signed short y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        #ifdef PEN_OVERLAY
        HDC hdc;

        hdc = GetDC(mEvt.hwnd);
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, x, y);
        #endif

        canvas_t *canvas = g_viewport.document->canvas;

        rect_t line_rect = {0};
        line_rect.x0 = prev.x;
        line_rect.y0 = prev.y;
        line_rect.x1 = x;
        line_rect.y1 = y;

        draw_line(canvas, line_rect);

        #ifdef PEN_OVERLAY
        ReleaseDC(mEvt.hwnd, hdc);
        #endif

    }
    fDraw = FALSE;
    ReleaseCapture();
}

void Pencil_OnMouseMove(MOUSEEVENT mEvt)
{
    signed short x = LOWORD(mEvt.lParam);
    signed short y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        #ifdef PEN_OVERLAY
        HDC hdc;
        hdc = GetDC(mEvt.hwnd);

        /* Draw overlay path */
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, x, y);
        #endif

        /* Draw on canvas */
        canvas_t *canvas = g_viewport.document->canvas;

        rect_t line_rect = {0};
        line_rect.x0 = prev.x;
        line_rect.y0 = prev.y;
        line_rect.x1 = x;
        line_rect.y1 = y;

        draw_line(canvas, line_rect);

        prev.x = x;
        prev.y = y;

        #ifdef PEN_OVERLAY
        ReleaseDC(mEvt.hwnd, hdc);
        #endif
    }
}

void Pencil_Init()
{
    pencil.OnLButtonUp = Pencil_OnLButtonUp;
    pencil.OnLButtonDown = Pencil_OnLButtonDown;
    pencil.OnMouseMove = Pencil_OnMouseMove;
}

POINT circCenter;

void Circle_OnLButtonDown(MOUSEEVENT mEvt)
{
    fDraw = TRUE;
    SetCapture(mEvt.hwnd);
    circCenter.x = LOWORD(mEvt.lParam);
    circCenter.y = HIWORD(mEvt.lParam);
}

void Circle_OnLButtonUp(MOUSEEVENT mEvt)
{
    signed short x = LOWORD(mEvt.lParam);
    signed short y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        #ifdef PEN_OVERLAY
        HDC hdc;

        hdc = GetDC(mEvt.hwnd);
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, x, y);
        #endif

        int radius = sqrt(pow(x - circCenter.x, 2) + pow(y - circCenter.y, 2));

        canvas_t *canvas = g_viewport.document->canvas;
        draw_circle(canvas, circCenter.x, circCenter.y, radius);

        #ifdef PEN_OVERLAY
        ReleaseDC(mEvt.hwnd, hdc);
        #endif

    }
    fDraw = FALSE;
    ReleaseCapture();
}

void Circle_OnMouseMove(MOUSEEVENT mEvt)
{
}

void Circle_Init()
{
    circle.OnLButtonUp = Circle_OnLButtonUp;
    circle.OnLButtonDown = Circle_OnLButtonDown;
    circle.OnMouseMove = Circle_OnMouseMove;
}

void Line_OnLButtonDown(MOUSEEVENT mEvt)
{
    fDraw = TRUE;
    SetCapture(mEvt.hwnd);
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void Line_OnLButtonUp(MOUSEEVENT mEvt)
{
    signed short x = LOWORD(mEvt.lParam);
    signed short y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        canvas_t* canvas = g_viewport.document->canvas;

        rect_t line_rect = {0};
        line_rect.x0 = prev.x;
        line_rect.y0 = prev.y;
        line_rect.x1 = x;
        line_rect.y1 = y;

        draw_line(canvas, line_rect);
    }
    fDraw = FALSE;
    ReleaseCapture();
}

void Line_OnMouseMove(MOUSEEVENT mEvt)
{
}

void Line_Init()
{
    line.OnLButtonUp = Line_OnLButtonUp;
    line.OnLButtonDown = Line_OnLButtonDown;
    line.OnMouseMove = Line_OnMouseMove;
}

void Rectangle_OnLButtonDown(MOUSEEVENT mEvt)
{
    fDraw = TRUE;
    SetCapture(mEvt.hwnd);
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void Rectangle_OnLButtonUp(MOUSEEVENT mEvt)
{
    signed short x = LOWORD(mEvt.lParam);
    signed short y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        canvas_t *canvas = g_viewport.document->canvas;

        rect_t rc = {0};
        rc.x0 = prev.x;
        rc.y0 = prev.y;
        rc.x1 = x;
        rc.y1 = y;
        draw_rectangle(canvas, rc);

    }
    fDraw = FALSE;
    ReleaseCapture();
}

void Rectangle_OnMouseMove(MOUSEEVENT mEvt)
{
}

void Rectangle_Init()
{
    rectangle.OnLButtonUp = Rectangle_OnLButtonUp;
    rectangle.OnLButtonDown = Rectangle_OnLButtonDown;
    rectangle.OnMouseMove = Rectangle_OnMouseMove;
}
