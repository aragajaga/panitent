#include "stdafx.h"
#include "toolshelf.h"

static int selected_tool = 1;

void ToolShelf_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;
    
    GetClientRect(hwnd, &rc);
    
    hdc = BeginPaint(hwnd, &ps);
    int bsize = 24;
    int cols = (rc.right - rc.left - 2)/(bsize+2);
    
    int x, y;
    
    for (int i = 0; i < 16; i++)
    {
        x = i%cols*(bsize+2)+2;
        y = i/cols*(bsize+2)+2;
        
        RECT rc = {x, y, x+bsize, y+bsize};
        DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_ADJUSTRECT | DFCS_HOT);
        
        if (i == selected_tool) {
            HGDIOBJ original = NULL;
            original = SelectObject(hdc,GetStockObject(DC_PEN));
            SetDCPenColor(hdc, RGB(255, 0, 0));
            
            
            //Rectangle(hdc, x, y, x+bsize, y+bsize);
            
            SelectObject(hdc, original);
        } else {
            //Rectangle(hdc, x, y, x+bsize, y+bsize);
        }
    }
    EndPaint(hwnd, &ps);
}

void ToolShelf_OnMouseMove(HWND hwnd, LPARAM lParam)
{
    RECT rc;
    
    GetClientRect(hwnd, &rc);
    
    int bsize = 24;
    int cols = (rc.right - rc.left - 2)/(bsize+2);
    
    int mx;
    mx = LOWORD(lParam);

    selected_tool = 14-((cols*26-mx) / 26);
    
}

void ToolShelf_OnLButtonDown(HWND hwnd, LPARAM lParam)
{

}

LRESULT CALLBACK _toolshelf_msgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_PAINT:
        ToolShelf_OnPaint(hwnd);
        return 0;
    case WM_MOUSEMOVE:
        ToolShelf_OnMouseMove(hwnd, lParam);
        return 0;
    case WM_LBUTTONDOWN:
        ToolShelf_OnLButtonDown(hwnd, lParam);
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void RegisterToolShelf()
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    
    wc.style            = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wc.style            |= CS_DROPSHADOW;
    wc.lpfnWndProc      = _toolshelf_msgproc;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH) (COLOR_BTNFACE + 1);
    wc.lpszClassName    = TOOLSHELF_WC;
    
    RegisterClass(&wc);
}
