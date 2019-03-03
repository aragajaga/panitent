#include "viewport.h"
#include <assert.h>

VIEWPORT vp;

void ImageAlloc(IMAGE *img)
{
    img->data = calloc(4, img->rc.width * img->rc.height);
}

void ImageFree(IMAGE *img)
{
    free(img->data);
}

void CanvasFillTest(IMAGE *img)
{
    for(int i = 0; i < img->rc.width*img->rc.height; i+=2)
    {
        ((unsigned int *)img->data)[i]   = 0xffffffff-(i*16)%0xffffff;
        ((unsigned int *)img->data)[i+1] = i%0xffffff;
    }
}

void RegisterViewportCtl()
{
    WNDCLASS wc = {0};
    
    wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = _viewport_msgproc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = VIEWPORTCTL_WC;
    
    RegisterClass(&wc);
}

void GetViewportRect(RECT *rcvp)
{    
    assert(!GetClientRect(vp.hwnd, rcvp));
    rcvp->right -= rcvp->left;
    rcvp->bottom -= rcvp->top;
}

void GetCanvasRect(RECT *rccv)
{
    RECT rcvp = {0};
    GetViewportRect(&rcvp);
    
    rccv->left = (rcvp.right  - vp.img.rc.width )/2;
    rccv->top  = (rcvp.bottom - vp.img.rc.height)/2;
}

void ViewportCtl_OnCreate()
{
    vp.img.rc.width = 512;
    vp.img.rc.height = 420;
    ImageAlloc(&vp.img);
}

void ViewportCtl_OnDestroy()
{
    ImageFree(&vp.img);
}

void ViewportCtl_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rcvp;
    
    GetClientRect(hwnd, &rcvp);
    rcvp.right  -= rcvp.left;
    rcvp.bottom -= rcvp.top;
    
    hdc = BeginPaint(hwnd, &ps);
    
    int w = (rcvp.right-vp.img.rc.width)/2;
    int h = (rcvp.bottom-vp.img.rc.height)/2;
    
    IMAGE *ctx = &vp.img;
    /* size_t size = ctx->rc.width * ctx->rc.height * 4; */
    
    HBITMAP hBitmap;
    hBitmap = CreateBitmap(ctx->rc.width, ctx->rc.height, 1, 8*4, ctx->data);
    
    HDC bitmapDC = CreateCompatibleDC(hdc);
    HBITMAP oldBitmap = SelectObject(bitmapDC, hBitmap);
    DeleteObject(oldBitmap);
    HRESULT hr = BitBlt(hdc, w, h, ctx->rc.width, ctx->rc.height, bitmapDC, 0, 0, SRCCOPY);
    if (E_FAIL == hr)
        PostQuitMessage(1);
    DeleteObject(hBitmap);
    DeleteDC(bitmapDC);
    
    EndPaint(hwnd, &ps);
}

static BOOL fDraw = FALSE;
static POINT prev;

void ViewportCtl_OnMouseWheel(WPARAM wParam)
{
    /* Тут типа зум надо бы сделать, но хз 
    int z_delta = GET_WHEEL_DELTA_WPARAM(wParam); */
}

void ViewportCtl_OnLButtonDown(MOUSEEVENT mEvt)
{
    fDraw = TRUE;
    prev.x = LOWORD(mEvt.lParam);
    prev.y = HIWORD(mEvt.lParam);
}

void ViewportCtl_OnLButtonUp(MOUSEEVENT mEvt)
{
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);
    
    if (fDraw)
    {
        HDC hdc = GetDC(mEvt.hwnd);
        MoveToEx(hdc, prev.x, prev.y, NULL);
        LineTo(hdc, x, y);
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
    case WM_CREATE:         ViewportCtl_OnCreate();                     break;
    case WM_DESTROY:        ViewportCtl_OnDestroy();                    break;
    case WM_PAINT:          ViewportCtl_OnPaint(hwnd);                  break;
    case WM_MOUSEWHEEL:     ViewportCtl_OnMouseWheel(wParam);           break;
    case WM_LBUTTONDOWN:    ViewportCtl_OnLButtonDown(mevt);            break;
    case WM_LBUTTONUP:      ViewportCtl_OnLButtonUp(mevt);              break;
    case WM_MOUSEMOVE:      ViewportCtl_OnMouseMove(mevt);              break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
