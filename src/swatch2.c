#include "precomp.h"

#include "swatch2.h"

typedef struct _SwatchControlData {
    uint32_t color;
} SwatchControlData;

WCHAR szSwatchClass[] = L"Win32Class_SwatchControl2";

static void OnCreate(HWND hwnd, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(lpcs);

    SwatchControlData* data = (SwatchControlData*)malloc(sizeof(SwatchControlData));
    memset(data, 0, sizeof(SwatchControlData));
    assert(data);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);
}

static void OnDestroy(HWND hwnd)
{
    SwatchControlData* data = (SwatchControlData*)GetWindowLongPtr(hwnd,
        GWLP_USERDATA);
    assert(data);
    free(data);
}

COLORREF custom_colors[16];

static void OnLButtonDown(HWND hwnd)
{
    SwatchControlData* data = (SwatchControlData*)GetWindowLongPtr(hwnd,
        GWLP_USERDATA);
    assert(data);

    CHOOSECOLOR cc = { 0 };
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = (LPDWORD)&custom_colors;
    cc.rgbResult = data->color;
    cc.lpfnHook = NULL;

    BOOL bResult = ChooseColor(&cc);

    if (bResult) {
        data->color = cc.rgbResult;
        InvalidateRect(hwnd, NULL, TRUE);

        SendMessage(GetParent(hwnd), WM_COMMAND,
            (WPARAM)MAKEWPARAM(GetMenu(hwnd), SCN_COLORCHANGE),
            (LPARAM)hwnd);
    }
}

static uint32_t OnSCMGetColor(HWND hwnd)
{
    SwatchControlData* data = (SwatchControlData*)GetWindowLongPtr(hwnd,
        GWLP_USERDATA);
    assert(data);
    return data->color;
}

static void OnSCMSetColor(HWND hwnd, WPARAM wParam)
{
    SwatchControlData* data = (SwatchControlData*)GetWindowLongPtr(hwnd,
        GWLP_USERDATA);
    assert(data);
    data->color = (uint32_t)wParam;
}

static void OnPaint(HWND hwnd)
{
    SwatchControlData* data = (SwatchControlData*)GetWindowLongPtr(hwnd,
        GWLP_USERDATA);
    assert(data);

    HBRUSH hbr = CreateSolidBrush(data->color);

    RECT clientRc = { 0 };
    GetWindowRect(hwnd, &clientRc);

    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc = {
      1,
      1,
      clientRc.right - clientRc.left - 3,
      clientRc.bottom - clientRc.top - 3
    };
    FillRect(hdc, &rc, hbr);

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK SwatchControl2_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        OnCreate(hwnd, (LPCREATESTRUCT)lParam);
        return 0;
    case WM_LBUTTONDOWN:
        OnLButtonDown(hwnd);
        return TRUE;
    case SCM_GETCOLOR:
        return OnSCMGetColor(hwnd);
    case SCM_SETCOLOR:
        OnSCMSetColor(hwnd, (uint32_t)wParam);
        return 0;
    case WM_PAINT:
        OnPaint(hwnd);
        return 0;
    case WM_DESTROY:
        OnDestroy(hwnd);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOL SwatchControl2_RegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC)SwatchControl2_WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szSwatchClass;
    wcex.hIconSm = NULL;

    BOOL bStatus = RegisterClassEx(&wcex);
    assert(bStatus);

    return bStatus;
}
