#include "viewport.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "debug.h"
#include "file_open.h"
#include <stddef.h>

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

BOOL ViewportSequence_CreateCanvas(VIEWPORT *vp)
{
    printf("[ViewportSequence] Creating canvas.\n");
    if (vp->seqi < ARRAYSIZE(vp->seq))
    {
        IMAGE img;
        img.rc.width = 100;
        img.rc.height = 100;
        
        ImageAlloc(&img);
        vp->seq[vp->seqi++] = img;
        return TRUE;
    }
    printf("[ViewportSequence] An error encountered while creating canvas in sequence.\n");
    return FALSE;
}

BOOL ViewportSequence_PopCanvas(VIEWPORT *vp)
{
    if (vp->seqi)
    {
        ImageFree(&vp->seq[vp->seqi]);
        vp->seq[vp->seqi--] = (IMAGE){0};
        return TRUE;
    }
    return FALSE;
}

void CanvasSetPixel(IMAGE *img, int x, int y, COLORREF color)
{
    if (x < img->rc.width && y < img->rc.height)
    {
        COLORREF cBack = ((LPCOLORREF)img->data)[(size_t)y*(size_t)img->rc.width+(size_t)x];
    
        float af = (color>>24)/255.f;
        unsigned char rR = GetRValue(color) * af + GetRValue(cBack) * (1.f - af);
        unsigned char gR = GetGValue(color) * af + GetGValue(cBack) * (1.f - af);
        unsigned char bR = GetBValue(color) * af + GetBValue(cBack) * (1.f - af);
        
        ((LPCOLORREF)img->data)[y*img->rc.width+x] = RGB(rR, gR, bR);
    }
}

void Plot(IMAGE *img, float x, float y, float alpha)
{ 
    /*COLORREF a = 0xff-alpha*0xff;

    COLORREF color = 0xff<<16 | a<<8 | a;*/
    
    COLORREF color = ((unsigned int)(alpha*0xff))<<24 | 0xff0000;
    CanvasSetPixel(img, round(x), round(y), color);
}

#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((double)(X))+0.5))
#define fpart_(X) (((double)(X))-(double)ipart_(X))
#define rfpart_(X) (1.0-fpart_(X))

#define swap_(a, b) do{ __typeof__(a) tmp;  tmp = a; a = b; b = tmp; }while(0)

void WuCircle(IMAGE *img, int offset_x, int offset_y, int r)
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
        
        Plot(img, offset_x + x,     offset_y + y,     1);
        Plot(img, offset_x + x - 1, offset_y + y,     alpha);
        Plot(img, offset_x + x + 1, offset_y + y,     0.5 - alpha);
        
        Plot(img, offset_x + y,     offset_y + x,     1);
        Plot(img, offset_x + y,     offset_y + x - 1, alpha);
        Plot(img, offset_x + y,     offset_y + x + 1, 0.5 - alpha);
        
        Plot(img, offset_x - x,     offset_y + y,     1);
        Plot(img, offset_x - x + 1, offset_y + y,     alpha);
        Plot(img, offset_x - x - 1, offset_y + y,     0.5 - alpha);
        
        Plot(img, offset_x - y,     offset_y + x,     1);
        Plot(img, offset_x - y,     offset_y + x - 1, alpha);
        Plot(img, offset_x - y,     offset_y + x + 1, 0.5 - alpha);
        
        
        Plot(img, offset_x + x,     offset_y - y,     1);
        Plot(img, offset_x + x - 1, offset_y - y,     alpha);
        Plot(img, offset_x + x + 1, offset_y - y,     0.5 - alpha);
        
        Plot(img, offset_x + y,     offset_y - x,     1);
        Plot(img, offset_x + y,     offset_y - x - 1, 0.5 - alpha);
        Plot(img, offset_x + y,     offset_y - x + 1, alpha);
        
        Plot(img, offset_x - y,     offset_y - x,     1);
        Plot(img, offset_x - y,     offset_y - x - 1, 0.5 - alpha);
        Plot(img, offset_x - y,     offset_y - x + 1, alpha);
        
        Plot(img, offset_x - x,     offset_y - y,     1);
        Plot(img, offset_x - x - 1, offset_y - y,     0.5 - alpha);
        Plot(img, offset_x - x + 1, offset_y - y,     alpha);
        
        t = dist;
    }
    ViewportUpdate();
}

void WuLine(IMAGE *img, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
    float dx = (float)x2 - (float)x1;
    float dy = (float)y2 - (float)y1;

    if ( fabs(dx) > fabs(dy) )
    {
        if ( x2 < x1 )
        {
            swap_(x1, x2);
            swap_(y1, y2);
        }

        float gradient = dy / dx;
        float xend = round_(x1);
        float yend = y1 + gradient*(xend - x1);
        float xgap = rfpart_(x1 + 0.5);
        int xpxl1 = xend;
        int ypxl1 = ipart_(yend);
        Plot(img, xpxl1, ypxl1, rfpart_(yend)*xgap);
        Plot(img, xpxl1, ypxl1+1, fpart_(yend)*xgap);
        float intery = yend + gradient;

        xend = round_(x2);
        yend = y2 + gradient*(xend - x2);
        xgap = fpart_(x2+0.5);
        int xpxl2 = xend;
        int ypxl2 = ipart_(yend);
        Plot(img, xpxl2, ypxl2, rfpart_(yend) * xgap);
        Plot(img, xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

        int x;
        for(x=xpxl1+1; x < xpxl2; x++)
        {
            Plot(img, x, ipart_(intery), rfpart_(intery));
            Plot(img, x, ipart_(intery) + 1, fpart_(intery));
            intery += gradient;
        }
    } else {
        if ( y2 < y1 )
        {
            swap_(x1, x2);
            swap_(y1, y2);
        }
        float gradient = dx / dy;
        float yend = round_(y1);
        float xend = x1 + gradient*(yend - y1);
        float ygap = rfpart_(y1 + 0.5);
        int ypxl1 = yend;
        int xpxl1 = ipart_(xend);
        Plot(img, xpxl1, ypxl1, rfpart_(xend)*ygap);
        Plot(img, xpxl1 + 1, ypxl1, fpart_(xend)*ygap);
        float interx = xend + gradient;

        yend = round_(y2);
        xend = x2 + gradient*(yend - y2);
        ygap = fpart_(y2+0.5);
        int ypxl2 = yend;
        int xpxl2 = ipart_(xend);
        Plot(img, xpxl2, ypxl2, rfpart_(xend) * ygap);
        Plot(img, xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

        int y;
        for(y=ypxl1+1; y < ypxl2; y++)
        {
            Plot(img, ipart_(interx), y, rfpart_(interx));
            Plot(img, ipart_(interx) + 1, y, fpart_(interx));
            interx += gradient;
        }
    }
    ViewportUpdate();
}
#undef swap_
#undef ipart_
#undef fpart_
#undef round_
#undef rfpart_

void PNTRectangle(IMAGE *img, int x1, int y1, int x2, int y2)
{
    WuLine(img, x1, y1, x2, y1);
    WuLine(img, x1, y1, x1, y2);
    WuLine(img, x2, y1, x2, y2);
    WuLine(img, x1, y2, x2, y2);
}

void ImageAlloc(IMAGE *img)
{
    img->data = calloc(4, (size_t)img->rc.width * (size_t)img->rc.height);
    printf("[ImageAlloc] Canvas memory page:\n");
    DebugVirtualMemoryInfo(img->data);
}

void ImageFree(IMAGE *img)
{
    free(img->data);
    img->data = NULL;
}

void CanvasCircleTest(IMAGE *img)
{
    for (int i = 0; i < 360; i++)
    {
        CanvasSetPixel(img, 80 + floor(sin(i * M_PI/180.f) * 40), 80 + floor(cos(i * M_PI/180.f) * 40), 0x00ffffff);
    }
}

void CanvasFillTest(IMAGE *img)
{
    for(size_t i = 0; i < img->rc.width*img->rc.height; i+=2)
    {
        ((LPCOLORREF)img->data)[i]   = 0xffffffff-((size_t)i*16)%0x00ffffff;
        ((LPCOLORREF)img->data)[i+(size_t)1] = i%0xffffff;
    }
    ViewportUpdate();
}

void CanvasFillSolid(IMAGE *img, COLORREF color)
{
    for(size_t i = 0; i < img->rc.width*img->rc.height; i++)
    {
        ((LPCOLORREF)img->data)[i] = color;
    }
    DebugVirtualMemoryInfo(img->data);
    ViewportUpdate();
}

void CanvasWuLinesTest()
{
    /* WuLine(10, 10, 180, 160);
    WuLine(390, 149, 53, 234);
    WuLine(52, 185, 301, 34);
    WuCircle(240, 240, 120);
    WuCircle(142, 234, 33);

    ViewportUpdate();*/
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

int cvsx;
int cvsy;

void GetCanvasRect(IMAGE *img, RECT *rcCanvas)
{
    RECT rcViewport = {0};
    if (GetClientRect(vp.hwnd, &rcViewport))
    {
        printf("[ViewportRect] w: %ld h: %ld\n", rcViewport.right, rcViewport.bottom);
    }
    else {
        printf("[GetCanvasRect] Failed to get viewport rect\n");
    }
    GetClientRect(vp.hwnd, &rcViewport);

    /*
    rcCanvas->left      = (rcViewport.right  - img->rc.width )/2;
    rcCanvas->top       = (rcViewport.bottom - img->rc.height)/2;
    */
    
    rcCanvas->left      = cvsx;
    rcCanvas->top       = cvsy;
    rcCanvas->right     = img->rc.width;
    rcCanvas->bottom    = img->rc.height;
    
    printf("[GetCanvasRect] right: %ld\n", rcCanvas->right);
    printf("[GetCanvasRect] bottom: %ld\n", rcCanvas->bottom);
}

BOOL CanvasClose(IMAGE *img)
{
    int iConfirmation;
    /* Note: Change this to TaskDialog */
    iConfirmation = MessageBox(NULL, L"Do you want to save changes?",
            L"panit.ent", MB_YESNOCANCEL | MB_ICONWARNING);
    
    switch (iConfirmation)
    {
    case IDYES:
        FileSave();
        break;
    case IDNO:
        img->rc.width = 0;
        img->rc.height = 0;
        
        ImageFree(img);
        break;
    default:
        return FALSE;
        break;
    }
    return TRUE;
}

void CreateCanvas(IMAGE *img, UINT uWidth, UINT uHeight)
{
    ViewportSequence_CreateCanvas(&vp);
    
    if (img->data != NULL)
        if (!CanvasClose(img))
            return;
    
    img->rc.width = uWidth;
    img->rc.height = uHeight;
    
    ImageAlloc(img);
    ViewportUpdate();
}

void ViewportCtl_OnCreate()
{
    cvsx = 0;
    cvsy = 0;
    
    /* [vp.seq] Memory leak cause if window will be re-created */
    vp.seqi = 0;
}

void ViewportCtl_OnDestroy()
{
    /* [vp.seq] Memory leak */
    ImageFree(&vp.img);
}

BOOL PutCanvasOnDC(HDC hdc, UINT x, UINT y, IMAGE *img)
{
    HBITMAP hBitmap;
    hBitmap = CreateBitmap(img->rc.width, img->rc.height, 1, sizeof(unsigned int)*8, img->data);
    
    HDC bitmapDC;
    HBITMAP oldBitmap;
    
    bitmapDC = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(bitmapDC, hBitmap);
    DeleteObject(oldBitmap);
    
    BitBlt(hdc,
            x, y,
            img->rc.width, img->rc.height,
            bitmapDC,
            0, 0,
            SRCCOPY);
    
    DeleteObject(hBitmap);
    DeleteDC(bitmapDC);
    return TRUE;
}

void ViewportCtl_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rcClient;

    GetClientRect(hwnd, &rcClient);

    hdc = BeginPaint(hwnd, &ps);

    IMAGE *ctx = &vp.img;
    for (size_t i = 0; i < vp.seqi; i++)
        PutCanvasOnDC(hdc, 10, 10, &vp.seq[i]);

    /*int x = (rcClient.right  - ctx->rc.width )/2;
    int y = (rcClient.bottom - ctx->rc.height)/2; */
    
    PutCanvasOnDC(hdc, cvsx, cvsy, ctx);

    EndPaint(hwnd, &ps);
}

BOOL bViewDragKey;
BOOL bViewDrag;
POINT dragPrev;

void ViewportCtl_OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_SPACE)
    {
        printf("[ViewportCtl] KeyDown VK_SPACE\n");
        bViewDragKey = TRUE;
    }
}

void ViewportCtl_OnMouseWheel(WPARAM wParam)
{
    /* Тут типа зум надо бы сделать, но хз
    int z_delta = GET_WHEEL_DELTA_WPARAM(wParam); */
}

void ViewportCtl_OnLButtonDown(MOUSEEVENT mEvt)
{
    bViewDrag = bViewDragKey;
    
    vp.tool.OnLButtonDown(mEvt);
}

void ViewportCtl_OnLButtonUp(MOUSEEVENT mEvt)
{
    bViewDrag = FALSE;
    vp.tool.OnLButtonUp(mEvt);
}

void ViewportCtl_OnMouseMove(MOUSEEVENT mEvt)
{
    if (bViewDrag)
    {
        dragPrev.x = LOWORD(mEvt.lParam);
        dragPrev.y = HIWORD(mEvt.lParam);
    }
    
    vp.tool.OnMouseMove(mEvt);
}

LRESULT CALLBACK ViewportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MOUSEEVENT mevt;
    mevt.hwnd = hwnd;
    mevt.lParam = lParam;

    switch(msg) {
    case WM_CREATE:         ViewportCtl_OnCreate();                     break;
    case WM_DESTROY:        ViewportCtl_OnDestroy();                    break;
    case WM_PAINT:
        if (vp.img.data)
            ViewportCtl_OnPaint(hwnd);
        else
            DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    case WM_KEYDOWN:        ViewportCtl_OnKeyDown(hwnd, wParam,
                                                  lParam);              break;
    case WM_MOUSEWHEEL:     ViewportCtl_OnMouseWheel(wParam);           break;
    case WM_LBUTTONDOWN:    ViewportCtl_OnLButtonDown(mevt);            break;
    case WM_LBUTTONUP:      ViewportCtl_OnLButtonUp(mevt);              break;
    case WM_MOUSEMOVE:      ViewportCtl_OnMouseMove(mevt);              break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
