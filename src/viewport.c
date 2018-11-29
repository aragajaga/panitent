#include "viewport.h"

void image_init(IMAGE *img)
{
    img->data = calloc(img->rc.width * img->rc.height * 4, sizeof(char));    
}

VIEWPORT vp;

void RegisterViewportCtl()
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    
    wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = _viewport_msgproc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = VIEWPORTCTL_WC;
    
    RegisterClass(&wc);
    
    IMAGE *img = vp.cvs.img;
    img->rc.x = 0;
    img->rc.y = 0;
    img->rc.width = 320;
    img->rc.height = 240;
    image_init(vp.cvs.img);
}

void ViewportCtl_OnPaint(HWND hwnd)
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
    
    IMAGE *ctx = vp.cvs.img;
    size_t size = ctx->rc.width * ctx->rc.height * 4;
    
    HBITMAP hBitmap;
    hBitmap = CreateBitmap(ctx->rc.width, ctx->rc.height, 1, 8*4, ctx->data);
    
    HDC bitmapDC = CreateCompatibleDC(hdc);
    HBITMAP oldBitmap = SelectObject(bitmapDC, hBitmap);
    DeleteObject(oldBitmap);
    BitBlt(hdc, 0, 0, ctx->rc.width, ctx->rc.height, bitmapDC, 0, 0, SRCCOPY);
    DeleteObject(hBitmap);
    DeleteDC(bitmapDC);
    
    EndPaint(hwnd, &ps);
}

static BOOL fDraw = FALSE;
static POINT prev;

void ViewportCtl_OnMouseWheel()
{
    /* Тут типа зум надо бы сделать, но хз */
    
}

void ViewportCtl_OnLButtonDown(MOUSEEVENT mEvt)
{
    fDraw = TRUE;
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void ViewportCtl_OnLButtonUp(MOUSEEVENT mEvt)
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

void ViewportCtl_OnMouseMove(MOUSEEVENT mEvt)
{
    if (fDraw)
    {
        HDC hdc = GetDC(mEvt.hwnd);
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, prev.x = LOWORD(mEvt.lParam), prev.y = HIWORD(mEvt.lParam));
        ReleaseDC(mEvt.hwnd, hdc);
    }
}

LRESULT CALLBACK _viewport_msgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MOUSEEVENT mevt;
    mevt.hwnd = hwnd;
    mevt.lParam = lParam;
    
    switch(msg) {
    case WM_PAINT:
        ViewportCtl_OnPaint(hwnd);
        return 0;
    case WM_MOUSEWHEEL:
        ViewportCtl_OnMouseWheel();
        return 0;
    case WM_LBUTTONDOWN:
        ViewportCtl_OnLButtonDown(mevt);
        return 0;
    case WM_LBUTTONUP:
        ViewportCtl_OnLButtonUp(mevt);
        return 0;
    case WM_MOUSEMOVE:
        ViewportCtl_OnMouseMove(mevt);
        return 0;
    }    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
