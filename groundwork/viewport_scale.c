#include <windows.h>
#include <windowsx.h>
#include <time.h>
#include <stdlib.h>

typedef struct _tagTRECT {
    unsigned int left;
    unsigned int top;
    unsigned int width;
    unsigned int height;
} TRECT;

typedef struct _tagIMAGE {
    void    *data;
    TRECT   size;
} IMAGE;

void image_init(IMAGE *img)
{
    img->size.width = 128;
    img->size.height = 128;
    img->data = calloc(img->size.width * img->size.height * 4, sizeof(char));
}

IMAGE image_crop(IMAGE *img, TRECT rc)
{
    const TRECT *img_size;
    img_size = &img->size;
    
    /* Проверка размера */
    
    /*if (img_size->width < rc.left + rc.width &&
        img_size->height < rc.top + rc.height)
    {*/
    void *new_data;
    new_data = calloc(rc.width * rc.height * 4, sizeof(char));
    
    void *tpr = img;
    
    
    while (tpr < img)
    {
        tpr += rc.left * 4;
        memcpy(new_data, tpr, rc.width * 4);
        tpr += rc.width * 4;
    }
    
    IMAGE new_img;
    new_img.data = new_data;
    new_img.size.width = rc.width;
    new_img.size.height = rc.height;
    
    return new_img;
    /*}*/
}

IMAGE img;
IMAGE view;

int scale_factor = 1;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
        break;
    case WM_MOUSEMOVE:
        for (int i = 0; i < img.size.width*img.size.height*4; i++)
        {
            ((char *)img.data)[i] = 255;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    case WM_MOUSEWHEEL:
        {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/120; // distance the wheel is rotated
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            
            scale_factor += zDelta;
        }
        break;
    case WM_PAINT:
    {
        HDC hdc = GetDC(hWnd);
        
        HBITMAP bmp = CreateBitmap(img.size.width, img.size.height, 1, 8*4, img.data);
        HDC src = CreateCompatibleDC(hdc);
        
        HBITMAP oldBitmap = SelectObject(src, bmp);
        DeleteObject(oldBitmap);
        BitBlt(hdc, 0, 0, img.size.width, img.size.height, src, 0, 0, SRCCOPY);
        DeleteObject(bmp);
        DeleteDC(src);
        
        TRECT rc = {0};
        rc.width = 50;
        rc.height = 40;
        
        view = image_crop(&img, rc);
        HBITMAP bmp2 = CreateBitmap(view.size.width, view.size.height, 1, 8*4, view.data);
        HDC src2 = CreateCompatibleDC(hdc);
        HBITMAP oldBitmap2 = SelectObject(src2, bmp2);
        DeleteObject(oldBitmap2);
        BitBlt(hdc, 256, 30, view.size.width, view.size.height, src2, 0, 0, SRCCOPY);
        DeleteObject(bmp2);
        DeleteDC(src2);
    }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    srand(time(NULL));
    image_init(&img);
    
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC) WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"WindowClass";
    wcex.hIconSm = NULL;
    RegisterClassEx(&wcex);
    
    /**************************************************************************/
    
    HWND hwnd = CreateWindowEx(
        0,
        L"WindowClass",
        L"Zoom demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    ShowWindow(hwnd, nCmdShow);
    
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
        DispatchMessage(&msg);
    
    return (int) msg.wParam;
}
