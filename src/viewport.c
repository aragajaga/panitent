#include "viewport.h"

void image_init(IMAGE *img)
{
    img->data = calloc(img->rc.width * img->rc.height * 4, sizeof(char));    
}

void RegisterCanvasCtl()
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    
    wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = _canvas_msgproc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CANVASCTL_WC;
    
    RegisterClass(&wc);
}

static HDC hdcMem;

void CanvasCtl_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rect;
    
    GetClientRect(hwnd, &rect);
    
    hdc = BeginPaint(hwnd, &ps);
    
    int w = ((rect.right-rect.left)-640)/2;
    int h = ((rect.bottom-rect.top)-480)/2;
    Rectangle(hdc,
        w,
        h,
        w + 640,
        h + 480);
    if (!hdcMem)
        hdcMem = CreateCompatibleDC(hdc);
    EndPaint(hwnd, &ps);
}

static BOOL fDraw = FALSE;
static POINT prev;

void CanvasCtl_OnLButtonDown(MOUSEEVENT mEvt)
{
    fDraw = TRUE;
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void CanvasCtl_OnLButtonUp(MOUSEEVENT mEvt)
{
    if (fDraw)
    {
        HDC hdc = GetDC(mEvt.hwnd);
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, LOWORD(mEvt.lParam), HIWORD(mEvt.lParam));
        ReleaseDC(mEvt.hwnd, hdc);
    }
    fDraw = FALSE;
}

void CanvasCtl_OnMouseMove(MOUSEEVENT mEvt)
{
    if (fDraw)
    {
        HDC hdc = GetDC(mEvt.hwnd);
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, prev.x = LOWORD(mEvt.lParam), prev.y = HIWORD(mEvt.lParam));
        ReleaseDC(mEvt.hwnd, hdc);
    }
}

LRESULT CALLBACK _canvas_msgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MOUSEEVENT mevt;
    mevt.hwnd = hwnd;
    mevt.lParam = lParam;
    
    switch(msg) {
    case WM_PAINT:
        CanvasCtl_OnPaint(hwnd);
        return 0;
    case WM_LBUTTONDOWN:
        CanvasCtl_OnLButtonDown(mevt);
        return 0;
    case WM_LBUTTONUP:
        CanvasCtl_OnLButtonUp(mevt);
        return 0;
    case WM_MOUSEMOVE:
        CanvasCtl_OnMouseMove(mevt);
        return 0;
    }    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
