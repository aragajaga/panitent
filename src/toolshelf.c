#include "stdafx.h"
#include "toolshelf.h"

typedef struct _tagPNTTOOL
{
    LPSTR szLabel;
    UINT iBmpIndex;
} PNTTOOL;

typedef struct _tagTOOLSHELF
{
    PNTTOOL *pTools;
    int nCount;
} TOOLSHELF;

TOOLSHELF tsh;

PNTTOOL tool;

void ToolShelf_AddTool(TOOLSHELF *tsh, PNTTOOL tool)
{
    tsh->pTools[tsh->nCount] = tool;
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
}

void ToolShelf_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;
    
    GetClientRect(hwnd, &rc);
    hdc = BeginPaint(hwnd, &ps);
    
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    
    int btnSize = 24;
    
    
    BITMAP bitmap;
    HDC hdcMem;
    HGDIOBJ oldBitmap;
    
    hdcMem = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(hdcMem, hbmTool);
    GetObject(hbmTool, sizeof(bitmap), &bitmap);
    
    for (int i = 0; i < tsh.nCount; i++)
    {
        int x = (i % 2) * btnSize;
        int y = (i / 2) * btnSize;
        
        Rectangle(hdc, x, y, x+btnSize, y+btnSize);
        
        int iBmp = tsh.pTools[i].iBmpIndex;
        TransparentBlt(hdc,
                x+4,        y+4,
                16,         bitmap.bmHeight,
                hdcMem,
                16*iBmp,    0,
                16,         bitmap.bmHeight,
                (UINT)0x00FF00FF);
    }
    
    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);
    
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK ToolShelfWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        InitializeToolShelf(&tsh);
        break;
    case WM_PAINT:
        ToolShelf_OnPaint(hwnd);
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
