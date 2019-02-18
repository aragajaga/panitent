#include "viewport.h"

VIEWPORT vp;

void image_init(IMAGE *img)
{
    size_t length = img->rc.width * img->rc.height * 4;
    img->data = calloc(length, sizeof(char));
    memset(img->data, 245, length);
}

void viewport_init(VIEWPORT* vp)
{
    vp->img.rc.width = 255;
    vp->img.rc.height = 255;
    image_init(&vp->img);
}

void RegisterViewportCtl()
{
    WNDCLASS wc = {0};
    
    wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = _viewport_msgproc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = VIEWPORTCTL_WC;
    
    RegisterClass(&wc);
    viewport_init(&vp);
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
    
    IMAGE *ctx = &vp.img;
    size_t size = ctx->rc.width * ctx->rc.height * 4;
    
    HBITMAP hBitmap;
    hBitmap = CreateBitmap(ctx->rc.width, ctx->rc.height, 1, 8*4, ctx->data);
    
    HDC bitmapDC = CreateCompatibleDC(hdc);
    HBITMAP oldBitmap = SelectObject(bitmapDC, hBitmap);
    DeleteObject(oldBitmap);
    HRESULT hr = BitBlt(hdc, 100, 100, ctx->rc.width, ctx->rc.height, bitmapDC, 0, 0, SRCCOPY);
    DeleteObject(hBitmap);
    DeleteDC(bitmapDC);
    
    EndPaint(hwnd, &ps);
}

static BOOL fDraw = FALSE;
static POINT prev;

void ViewportCtl_OnMouseWheel(WPARAM wParam)
{
    /* Тут типа зум надо бы сделать, но хз */
    int z_delta = GET_WHEEL_DELTA_WPARAM(wParam); 
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
        ViewportCtl_OnMouseWheel(wParam);
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
