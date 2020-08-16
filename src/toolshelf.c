#include "toolshelf.h"
#include "tool.h"
#include "viewport.h"
#include "debug.h"
#include <math.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

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
}

HWND hwndToolShelf;
HTHEME hTheme = NULL;
int btnSize = 24;

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

void ToolShelf_DrawButtons(HDC hdc)
{
    BITMAP bitmap;
    HDC hdcMem;
    HGDIOBJ oldBitmap;

    hdcMem = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(hdcMem, hbmTool);
    GetObject(hbmTool, sizeof(bitmap), &bitmap);

    for (int i = 0; i < tsh.nCount; i++)
    {

        int x = 4 + (i % 2) * (4 + btnSize);
        int y = 4 + (i / 2) * (4 + btnSize);

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

    ToolShelf_DrawButtons(hdc);

    EndPaint(hwnd, &ps);
}

void ToolShelf_OnLButtonUp(MOUSEEVENT mEvt)
{
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);

    if (x > 4
        && x < btnSize*2
        && y < btnSize*tsh.nCount)
    {
        int bID = y / btnSize * 2 + x / btnSize;

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

void Pencil_OnLButtonDown(MOUSEEVENT mEvt)
{
    fDraw = TRUE;
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void Pencil_OnLButtonUp(MOUSEEVENT mEvt)
{
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        #ifdef PEN_OVERLAY
        HDC hdc;

        hdc = GetDC(mEvt.hwnd);
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, x, y);
        #endif

        RECT rcCanvas;
        GetCanvasRect(&vp.img, &rcCanvas);
        WuLine( &vp.img,
                prev.x - rcCanvas.left,
                prev.y - rcCanvas.top,
                x - rcCanvas.left,
                y - rcCanvas.top );

        #ifdef PEN_OVERLAY
        ReleaseDC(mEvt.hwnd, hdc);
        #endif

    }
    fDraw = FALSE;
}

void Pencil_OnMouseMove(MOUSEEVENT mEvt)
{
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);

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
        RECT rcCanvas;
        GetCanvasRect(&vp.img, &rcCanvas);

        if (x > rcCanvas.left && y > rcCanvas.top)
        {
            WuLine( &vp.img,
                prev.x - rcCanvas.left,
                prev.y - rcCanvas.top,
                x - rcCanvas.left,
                y - rcCanvas.top );

            printf("[CanvasRect]");
            DebugPrintRect(&rcCanvas);
        }
        else {
            printf("[CanvasRect] Out of bounds\n");
        }

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
    circCenter.x = LOWORD(mEvt.lParam);
    circCenter.y = HIWORD(mEvt.lParam);
}

void Circle_OnLButtonUp(MOUSEEVENT mEvt)
{
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        #ifdef PEN_OVERLAY
        HDC hdc;

        hdc = GetDC(mEvt.hwnd);
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, x, y);
        #endif

        RECT rcCanvas;
        GetCanvasRect(&vp.img, &rcCanvas);

        int radius = sqrt(pow(x - circCenter.x, 2) + pow(y - circCenter.y, 2));


        WuCircle( &vp.img,
                circCenter.x - rcCanvas.left,
                circCenter.y - rcCanvas.top,
                radius );

        #ifdef PEN_OVERLAY
        ReleaseDC(mEvt.hwnd, hdc);
        #endif

    }
    fDraw = FALSE;
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
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void Line_OnLButtonUp(MOUSEEVENT mEvt)
{
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        RECT rcCanvas;
        GetCanvasRect(&vp.img, &rcCanvas);

        WuLine( &vp.img,
                prev.x - rcCanvas.left,
                prev.y - rcCanvas.top,
                x - rcCanvas.left,
                y - rcCanvas.top);

    }
    fDraw = FALSE;
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
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void Rectangle_OnLButtonUp(MOUSEEVENT mEvt)
{
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);

    if (fDraw)
    {
        RECT rcCanvas;
        GetCanvasRect(&vp.img, &rcCanvas);

        PNTRectangle(&vp.img, prev.x - rcCanvas.left,
                prev.y - rcCanvas.top,
                x - rcCanvas.left,
                y - rcCanvas.top);

    }
    fDraw = FALSE;
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
