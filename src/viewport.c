#include "viewport.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "debug.h"

VIEWPORT vp;

void swapf(float *a, float *b)
{
    float c = *a;
    *b = *a;
    *a = c;
}

void ViewportUpdate()
{
    InvalidateRect(vp.hwnd, NULL, TRUE);
    printf("[Viewport] View updated\n");
}

void CanvasSetPixel(IMAGE *img, int x, int y, COLORREF color)
{
    if (x < img->rc.width && y < img->rc.height)
    {
        ((unsigned int *)img->data)[y*img->rc.width+x] = color;
    }
}

void Plot(float x, float y, float alpha)
{ 
    COLORREF a = 0xff-alpha*0xff;

    COLORREF color = 0xff<<16 | a<<8 | a;
    CanvasSetPixel(&vp.img, round(x), round(y), color);
}

#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((double)(X))+0.5))
#define fpart_(X) (((double)(X))-(double)ipart_(X))
#define rfpart_(X) (1.0-fpart_(X))

#define swap_(a, b) do{ __typeof__(a) tmp;  tmp = a; a = b; b = tmp; }while(0)

void WuCircle(int offset_x, int offset_y, int r)
{
    int x = r;
    int y = -1;
    float t = 0;
    while (x - 1 > y)
    {
        y++;
        
        float rp = sqrt(r*r - y*y);
        float dist = ceil(rp) - rp;
        if (dist < t)
            x--;
        
        float alpha = dist/2;
        
        Plot(offset_x + x,     offset_y + y,     1);
        Plot(offset_x + x - 1, offset_y + y,     alpha);
        Plot(offset_x + x + 1, offset_y + y,     0.5 - alpha);
        
        Plot(offset_x + y,     offset_y + x,     1);
        Plot(offset_x + y,     offset_y + x - 1, alpha);
        Plot(offset_x + y,     offset_y + x + 1, 0.5 - alpha);
        
        Plot(offset_x - x,     offset_y + y,     1);
        Plot(offset_x - x + 1, offset_y + y,     alpha);
        Plot(offset_x - x - 1, offset_y + y,     0.5 - alpha);
        
        Plot(offset_x - y,     offset_y + x,     1);
        Plot(offset_x - y,     offset_y + x - 1, alpha);
        Plot(offset_x - y,     offset_y + x + 1, 0.5 - alpha);
        
        
        Plot(offset_x + x,     offset_y - y,     1);
        Plot(offset_x + x - 1, offset_y - y,     alpha);
        Plot(offset_x + x + 1, offset_y - y,     0.5 - alpha);
        
        Plot(offset_x + y,     offset_y - x,     1);
        Plot(offset_x + y,     offset_y - x - 1, 0.5 - alpha);
        Plot(offset_x + y,     offset_y - x + 1, alpha);
        
        Plot(offset_x - y,     offset_y - x,     1);
        Plot(offset_x - y,     offset_y - x - 1, 0.5 - alpha);
        Plot(offset_x - y,     offset_y - x + 1, alpha);
        
        Plot(offset_x - x,     offset_y - y,     1);
        Plot(offset_x - x - 1, offset_y - y,     0.5 - alpha);
        Plot(offset_x - x + 1, offset_y - y,     alpha);
        
        t = dist;
    }
}

void draw_line_antialias(unsigned int x1, unsigned int y1,
        unsigned int x2, unsigned int y2)
{
    double dx = (double)x2 - (double)x1;
    double dy = (double)y2 - (double)y1;

    if ( fabs(dx) > fabs(dy) )
    {
        if ( x2 < x1 )
        {
            swap_(x1, x2);
            swap_(y1, y2);
        }

        double gradient = dy / dx;
        double xend = round_(x1);
        double yend = y1 + gradient*(xend - x1);
        double xgap = rfpart_(x1 + 0.5);
        int xpxl1 = xend;
        int ypxl1 = ipart_(yend);
        Plot(xpxl1, ypxl1, rfpart_(yend)*xgap);
        Plot(xpxl1, ypxl1+1, fpart_(yend)*xgap);
        double intery = yend + gradient;

        xend = round_(x2);
        yend = y2 + gradient*(xend - x2);
        xgap = fpart_(x2+0.5);
        int xpxl2 = xend;
        int ypxl2 = ipart_(yend);
        Plot(xpxl2, ypxl2, rfpart_(yend) * xgap);
        Plot(xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

        int x;
        for(x=xpxl1+1; x < xpxl2; x++)
        {
            Plot(x, ipart_(intery), rfpart_(intery));
            Plot(x, ipart_(intery) + 1, fpart_(intery));
            intery += gradient;
        }
    } else {
        if ( y2 < y1 )
        {
            swap_(x1, x2);
            swap_(y1, y2);
        }
        double gradient = dx / dy;
        double yend = round_(y1);
        double xend = x1 + gradient*(yend - y1);
        double ygap = rfpart_(y1 + 0.5);
        int ypxl1 = yend;
        int xpxl1 = ipart_(xend);
        Plot(xpxl1, ypxl1, rfpart_(xend)*ygap);
        Plot(xpxl1 + 1, ypxl1, fpart_(xend)*ygap);
        double interx = xend + gradient;

        yend = round_(y2);
        xend = x2 + gradient*(yend - y2);
        ygap = fpart_(y2+0.5);
        int ypxl2 = yend;
        int xpxl2 = ipart_(xend);
        Plot(xpxl2, ypxl2, rfpart_(xend) * ygap);
        Plot(xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

        int y;
        for(y=ypxl1+1; y < ypxl2; y++)
        {
            Plot(ipart_(interx), y, rfpart_(interx));
            Plot(ipart_(interx) + 1, y, fpart_(interx));
            interx += gradient;
        }
    }
}
#undef swap_
#undef ipart_
#undef fpart_
#undef round_
#undef rfpart_

void WuLine(float x0, float y0, float x1, float y1)
{
    BOOL steep = (abs(y1 - y0) > abs(x1 - y0))?TRUE:FALSE;

    if (steep)
    {
        swapf(&x0, &y0);
        swapf(&y1, &y1);
    }

    if (x0 > x1)
    {
        swapf(&x0, &y1);
        swapf(&x0, &y1);
    }

    float dx = x1 - x0;
    float dy = y1 - y0;
    float gradient = dy / (float)dx;
    if (dx == 0.f)
        gradient = 1.f;

    /* Handle first endpoint */
    float xend = round(x0);
    float yend = y0 + gradient * (xend - x0);
    float xgap = 1.f - fmod(x0 + 0.5f, 1.f);
    float xpxl1 = xend;
    float ypxl1 = floor(yend);

    if (steep)
    {
        Plot(ypxl1,     xpxl1,    1.f - fmod(yend, 1.f) * xgap);
        Plot(ypxl1+1.f, xpxl1,          fmod(yend, 1.f) * xgap);
    }
    else {
        Plot(xpxl1,     ypxl1,          fmod(yend, 1.f) * xgap);
        Plot(xpxl1,     ypxl1+1.f,      fmod(yend, 1.f) * xgap);
    }
    float intery = yend + gradient;

    /* Handle second endpoint */
    xend = round(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fmod(x1 + 0.5f, 1.f);
    float xpxl2 = xend;
    float ypxl2 = floor(yend);

    if (steep)
    {
        Plot(ypxl2,     xpxl2,    1.f - fmod(yend, 1.f) * xgap);
        Plot(ypxl2+1.f, xpxl2,          fmod(yend, 1.f) * xgap);
    }
    else {
        Plot(xpxl2,     ypxl2,    1.f - fmod(yend, 1.f) * xgap);
        Plot(xpxl2,     ypxl2+1.f,      fmod(yend, 1.f) * xgap);
    }

    if (steep)
    {
        for (int x = xpxl1+1; x < xpxl2 - 1; x++)
        {
            Plot(floor(intery)    , x, 1.f - fmod(intery, 1.f));
            Plot(floor(intery)+1.f, x,       fmod(intery, 1.f));
        }
    }
    else {
        for (int x = xpxl1+1; x < xpxl2 - 1; x++)
        {
            Plot(x, floor(intery)    , 1.f - fmod(intery, 1.f));
            Plot(x, floor(intery)+1.f,       fmod(intery, 1.f));
        }
    }
}

void ImageAlloc(IMAGE *img)
{
    img->data = calloc(4, img->rc.width * img->rc.height);
    printf("[ImageAlloc] Canvas memory page:\n");
    DebugVirtualMemoryInfo(img->data);
}

void ImageFree(IMAGE *img)
{
    free(img->data);
}

#define PI 3.1415

void CanvasCircleTest(IMAGE *img)
{
    for (int i = 0; i < 360; i++)
    {
        CanvasSetPixel(img, 80 + floor(sin(i * PI/180.f) * 40), 80 + floor(cos(i * PI/180.f) * 40), 0x00ffffff);
    }
}

void CanvasFillTest(IMAGE *img)
{
    for(int i = 0; i < img->rc.width*img->rc.height; i+=2)
    {
        ((unsigned int *)img->data)[i]   = 0xffffffff-(i*16)%0xffffff;
        ((unsigned int *)img->data)[i+1] = i%0xffffff;
    }
    ViewportUpdate();
}

void CanvasFillSolid(IMAGE *img, COLORREF color)
{
    for(int i = 0; i < img->rc.width*img->rc.height; i++)
    {
        ((unsigned int *)img->data)[i] = color;
    }
    DebugVirtualMemoryInfo(img->data);
    ViewportUpdate();
}

void CanvasWuLinesTest()
{
    draw_line_antialias(10, 10, 180, 160);
    draw_line_antialias(390, 149, 53, 234);
    draw_line_antialias(52, 185, 301, 34);
    WuCircle(240, 240, 120);
    WuCircle(142, 234, 33);

    ViewportUpdate();
}

void RegisterViewportCtl()
{
    WNDCLASS wc = {0};

    wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC)ViewportWndProc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = VIEWPORTCTL_WC;

    if (RegisterClass(&wc))
    {
        printf("[Viewport] Window class registered\n");
    }
    else {
        printf("[Viewport] Failed to register window class\n");
    }
}

void GetCanvasRect(RECT *rcCanvas)
{
    RECT rcViewport = {0};
    GetClientRect(vp.hwnd, &rcViewport);

    rcCanvas->left      = (rcViewport.right  - vp.img.rc.width )/2;
    rcCanvas->top       = (rcViewport.bottom - vp.img.rc.height)/2;
    rcCanvas->right     = vp.img.rc.width;
    rcCanvas->bottom    = vp.img.rc.height;
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
    RECT rcClient;

    GetClientRect(hwnd, &rcClient);

    hdc = BeginPaint(hwnd, &ps);

    IMAGE *ctx = &vp.img;
    /* size_t size = ctx->rc.width * ctx->rc.height * 4; */

    HBITMAP hBitmap;
    hBitmap = CreateBitmap(ctx->rc.width, ctx->rc.height, 1, 8*4, ctx->data);

    HDC bitmapDC;
    HBITMAP oldBitmap;

    bitmapDC = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(bitmapDC, hBitmap);
    DeleteObject(oldBitmap);

    int x = (rcClient.right  - vp.img.rc.width )/2;
    int y = (rcClient.bottom - vp.img.rc.height)/2;

    HRESULT hr = BitBlt(hdc,
            x, y,
            ctx->rc.width, ctx->rc.height,
            bitmapDC,
            0, 0,
            SRCCOPY);

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
    int x = LOWORD(mEvt.lParam);
    int y = HIWORD(mEvt.lParam);
    
    if (fDraw)
    {
        RECT rcCanvas;
        GetCanvasRect(&rcCanvas);
        
        if (x > rcCanvas.left && y > rcCanvas.top)
        {
            HDC hdc = GetDC(mEvt.hwnd);
            MoveToEx(hdc, prev.x, prev.y, NULL);
            prev.x = x;
            prev.y = y;
            LineTo(hdc, x, y);
            ReleaseDC(mEvt.hwnd, hdc);
        }
    }
}

LRESULT CALLBACK ViewportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
