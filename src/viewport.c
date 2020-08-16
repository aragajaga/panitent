#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

#include "file_open.h"
#include "document.h"
#include "viewport.h"
#include "palette.h"
#include "canvas.h"
#include "debug.h"

VIEWPORT vp;
viewport_t g_viewport;

void swapf(float *a, float *b)
{
    float c = *a;
    *b = *a;
    *a = c;
}

void viewport_invalidate()
{
  InvalidateRect(g_viewport.win_handle, NULL, TRUE);
}

void ViewportUpdate()
{
    InvalidateRect(vp.hwnd, NULL, TRUE);
    printf("[Viewport] View updated\n");
}

void viewport_set_document(document_t* document)
{
  g_viewport.document = document;
}

#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((double)(X))+0.5))
#define fpart_(X) (((double)(X))-(double)ipart_(X))
#define rfpart_(X) (1.0-fpart_(X))

#define swap_(a, b) do{ __typeof__(a) tmp;  tmp = a; a = b; b = tmp; }while(0)

/*
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
*/

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

void viewport_register_class()
{
  /* Break if already registered */
  if (g_viewport.win_class)
    return;

  WNDCLASSEX wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)ViewportWndProc;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = VIEWPORTCTL_WC;

  ATOM class_atom = RegisterClassEx(&wcex);
  if (!class_atom) {
    MessageBox(NULL, L"Failed to register class!", NULL,
        MB_OK | MB_ICONERROR);
    return;
  }

  g_viewport.win_class = class_atom;
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

BOOL gdi_blit_canvas(HDC hdc, int x, int y, canvas_t* canvas)
{
  HBITMAP hbitmap = CreateBitmap(canvas->width, canvas->height, 1,
      sizeof(uint32_t) * 8, canvas->buffer);

  HDC bitmapdc;

  bitmapdc = CreateCompatibleDC(hdc);
  SelectObject(bitmapdc, hbitmap);

  BOOL status = BitBlt(hdc, x, y, canvas->width, canvas->height, bitmapdc, 0, 0,
      SRCCOPY);

  if (!status)
    MessageBox(NULL, L"BitBlt failed", NULL, MB_OK | MB_ICONERROR);
  
  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);

  return TRUE;
}

void ViewportCtl_OnPaint(HWND hwnd)
{
  if (g_viewport.document == NULL)
    return;

  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);
  gdi_blit_canvas(hdc, 0, 0, g_viewport.document->canvas);
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
    case WM_PAINT:          ViewportCtl_OnPaint(hwnd);                  break;
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
