#include "../precomp.h"

#include "bubble.h"
#include "../resource.h"

#include "../win32/util.h"

HWND g_hWndBubble;
POINT g_localCursorPos;
RECT g_rcWindow;
RECT g_rcClient;
RECT g_rcTemp;

static const WCHAR szClassName[] = L"__BubbleClass";

LRESULT CALLBACK Bubble_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        POINT ptCursor;
        GetCursorPos(&ptCursor);

        WCHAR szString[1024] = L"";
        swprintf_s(szString, 1024,
            L"MouseX: = %d\n"
            L"MouseY: = %d\n\n"

            L"Local MouseX: = %d\n"
            L"Local MouseY: = %d\n\n"

            L"rcWindow.pLeft = %d\n"
            L"rcWindow.top = %d\n"
            L"rcWindow.pRight = %d\n"
            L"rcWindow.bottom = %d\n\n"

            L"rcClient.pLeft = %d\n"
            L"rcClient.top = %d\n"
            L"rcClient.pRight = %d\n"
            L"rcClient.bottom = %d\n\n"
            

            L"rcTemp.pLeft = %d\n"
            L"rcTemp.top = %d\n"
            L"rcTemp.pRight = %d\n"
            L"rcTemp.bottom = %d\n",
            ptCursor.x, ptCursor.y,
            g_localCursorPos.x, g_localCursorPos.y,
            g_rcWindow.left, g_rcWindow.top, g_rcWindow.right, g_rcWindow.bottom,
            g_rcClient.left, g_rcClient.top, g_rcClient.right, g_rcClient.bottom,
            g_rcTemp.left, g_rcTemp.top, g_rcTemp.right, g_rcTemp.bottom);

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        HBRUSH hBrush = CreateSolidBrush(RGB(0xFF, 0xFF, 0xCC));
        FillRect(hdc, &rcClient, hBrush);
        DeleteObject(hBrush);

        SetBkMode(hdc, TRANSPARENT);

        HFONT hFont;
        {
            NONCLIENTMETRICS ncm = { 0 };
            ncm.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);

            wcscpy_s(ncm.lfCaptionFont.lfFaceName, sizeof(ncm.lfCaptionFont.lfFaceName) / sizeof(*ncm.lfCaptionFont.lfFaceName), L"Courier New");

            hFont = CreateFontIndirect(&ncm.lfCaptionFont);
        }

        HFONT hOldFont = SelectObject(hdc, hFont);

        rcClient.left += 4;
        rcClient.top += 4;
        rcClient.right -= 4;
        rcClient.bottom -= 4;
        DrawTextEx(hdc, szString, wcslen(szString), &rcClient, 0, NULL);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        EndPaint(hWnd, &ps);
    }
        break;

    case WM_DESTROY:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL Bubble_RegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC)Bubble_WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = szClassName;
    wcex.lpszMenuName = NULL;
    wcex.hIcon = NULL;

    return RegisterClassEx(&wcex);
}

HWND Bubble_CreateWindow(HINSTANCE hInstance)
{
    g_hWndBubble = CreateWindowEx(WS_EX_TOPMOST, szClassName, L"Bubble", WS_POPUP | WS_VISIBLE | WS_BORDER, 100, 100, 200, 400, NULL, NULL, hInstance, NULL);

    return g_hWndBubble;
}
