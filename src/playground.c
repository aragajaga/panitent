#include "precomp.h"

#include "playground.h"

#include "util/assert.h"

#include "resource.h"

#include "win32/windowmap.h"
#include "win32/util.h"

#include <math.h>

BOOL PlaygroundDlg_OnInitDialog(PlaygroundDlg* pPlaygroundDlg);
void PlaygroundDlg_OnOK(PlaygroundDlg* pPlaygroundDlg);
void PlaygroundDlg_OnCancel(PlaygroundDlg* pPlaygroundDlg);
INT_PTR PlaygroundDlg_UserProcDlg(PlaygroundDlg* pPlaygroundDlg, UINT message, WPARAM wParam, LPARAM lParam);

void PlaygroundDlg_Init(PlaygroundDlg* pPlaygroundDlg) {
    ASSERT(pPlaygroundDlg);
    memset(pPlaygroundDlg, 0, sizeof(PlaygroundDlg));
    Dialog_Init((Dialog*)pPlaygroundDlg);

    pPlaygroundDlg->base.OnInitDialog = (FnDialogOnInitDialog)PlaygroundDlg_OnInitDialog;
    pPlaygroundDlg->base.OnOK = (FnDialogOnOK)PlaygroundDlg_OnOK;
    pPlaygroundDlg->base.OnCancel = (FnDialogOnCancel)PlaygroundDlg_OnCancel;
    pPlaygroundDlg->base.DlgUserProc = (FnDialogDlgUserProc)PlaygroundDlg_UserProcDlg;

    Vector_Init(&pPlaygroundDlg->widgets, sizeof(IWidget*));
}


BOOL PlaygroundDlg_OnInitDialog(PlaygroundDlg* pPlaygroundDlg)
{
    HWND hDlg = Window_GetHWND((Window*)pPlaygroundDlg);

    return TRUE;
}

void PlaygroundDlg_OnOK(PlaygroundDlg* pPlaygroundDlg)
{
    EndDialog(Window_GetHWND((Window*)pPlaygroundDlg), 0);
    // Window_Destroy((Window*)pPlaygroundDlg);
    WindowMap_Erase(Window_GetHWND((Window*)pPlaygroundDlg));
}

void PlaygroundDlg_OnCancel(PlaygroundDlg* pPlaygroundDlg)
{
    EndDialog(Window_GetHWND((Window*)pPlaygroundDlg), 0);
    // Window_Destroy((Window*)pPlaygroundDlg);
    WindowMap_Erase(Window_GetHWND((Window*)pPlaygroundDlg));
}

/*
typedef struct AppFrameTheme AppFrameTheme;
struct AppFrameTheme {

};
*/

COLORREF HSLtoRGB(float h, float s, float l) {
    float r, g, b;

    float c = (1 - fabs(2 * l - 1)) * s;  // Chroma
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = l - c / 2;

    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }

    r += m;
    g += m;
    b += m;

    return RGB((BYTE)(r * 255), (BYTE)(g * 255), (BYTE)(b * 255));
}

void RGBtoHSL(COLORREF rgb, float* h, float* s, float* l) {
    // Extract RGB components from COLORREF
    float r = GetRValue(rgb) / 255.0f;
    float g = GetGValue(rgb) / 255.0f;
    float b = GetBValue(rgb) / 255.0f;

    // Find the minimum and maximum values of R, G, and B
    float max = fmaxf(fmaxf(r, g), b);
    float min = fminf(fminf(r, g), b);

    // Lightness is the average of the min and max
    *l = (max + min) / 2.0f;

    if (max == min) {
        // If max and min are equal, it's a shade of gray, no saturation, no hue
        *h = 0.0f;
        *s = 0.0f;
    }
    else {
        // Otherwise, calculate saturation
        if (*l < 0.5f) {
            *s = (max - min) / (max + min);
        }
        else {
            *s = (max - min) / (2.0f - max - min);
        }

        // Calculate hue
        if (max == r) {
            *h = (g - b) / (max - min);
        }
        else if (max == g) {
            *h = 2.0f + (b - r) / (max - min);
        }
        else {
            *h = 4.0f + (r - g) / (max - min);
        }

        // Convert hue to degrees
        *h *= 60.0f;
        if (*h < 0.0f) {
            *h += 360.0f;
        }
    }
}

COLORREF HSVtoRGB(float h, float s, float v) {
    float r, g, b;

    int i = (int)(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    }

    return RGB((BYTE)(r * 255), (BYTE)(g * 255), (BYTE)(b * 255));
}

void DrawHSLGradient(HDC hdc, int x1, int y1, int width, int height) {

    HDC hBmDC = CreateCompatibleDC(hdc);    

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* pData = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pData, NULL, 0);

    if (!pData) {
        DeleteDC(hBmDC);
        return;
    }

    DWORD* pixels = (DWORD*)pData;

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            float hue = (float)x * (360.0f / (float)width); // Hue in degrees
            float lightness = 1.0f - (float)y / height;
            COLORREF color = HSLtoRGB(hue, 1.0f, lightness);

            BYTE r = GetRValue(color);
            BYTE g = GetGValue(color);
            BYTE b = GetBValue(color);

            pixels[y * width + x] = (r << 16) | (g << 8) | b;
        }
    }

    HBITMAP hOldBitmap = SelectBitmap(hBmDC, hBitmap);

    BitBlt(hdc, x1, y1, width, height, hBmDC, 0, 0, SRCCOPY);

    SelectBitmap(hBmDC, hOldBitmap);
    DeleteBitmap(hBitmap);
    DeleteDC(hBmDC);
}

/*
void DrawHSLGradientOnICanvas(IAbstractCanvas* pCanvas, IAbstractDrawer* pDrawer, int x1, int y2, int width, int height) {
    
    void* pData = NULL;
    pCanvas->__vtbl->GetBits(pCanvas, &pData);

}
*/

void DrawGrip(HDC hdc, int x, int y) {
    BeginPath(hdc);
    Ellipse(hdc, x - 10, y - 10, x + 10, y + 10);
    Ellipse(hdc, x - 7, y - 7, x + 7, y + 7);
    CloseFigure(hdc);
    EndPath(hdc);
    StrokeAndFillPath(hdc);
}

void HSLGradient_Draw(HSLGradient* pHSLGradient, HDC hdc) {
    RECT rc;
    CopyRect(&rc, &pHSLGradient->rc);
    InflateRect(&rc, 2, 2);
    FrameRect(hdc, &rc, GetStockObject(BLACK_BRUSH));

    int width = Win32_Rect_GetWidth(&pHSLGradient->rc);
    int height = Win32_Rect_GetHeight(&pHSLGradient->rc);

    DrawHSLGradient(hdc,
        pHSLGradient->rc.left,
        pHSLGradient->rc.top,
        width,
        height);

    int gripX = pHSLGradient->rc.left + pHSLGradient->hue / 360.0f * (float)width;
    int gripY = pHSLGradient->rc.top + (1.0f - pHSLGradient->lightness) * (float)height;

    DrawGrip(hdc, gripX, gripY);
}

void DrawThemedWindowFrameOnHDC(HDC hdc, COLORREF accentColor, int x1, int y1, int width, int height) {
    COLORREF color = accentColor; // Win32_HexToCOLORREF(L"#ff8800");
    HBRUSH hBrush = CreateSolidBrush(color);
    HBRUSH hOldBrush = SelectBrush(hdc, hBrush);

    RECT rc = {
        .left = x1,
        .top = y1,
        .right = x1 + width,
        .bottom = y1 + height
    };
    ContractRect(&rc, 4);

    HFONT hFont = GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = SelectObject(hdc, hFont);

    float h, s, l;
    RGBtoHSL(accentColor, &h, &s, &l);

    float lightnessFrame = l < 0.5f ? l + 0.45f : l - 0.45f;
    COLORREF frameColor = HSLtoRGB(h, 0.5f, lightnessFrame);
    HPEN hPen = CreatePen(PS_SOLID, 1, frameColor);
    HPEN hOldPen = SelectPen(hdc, hPen);
    Rectangle(hdc, x1, y1, x1 + width, y1 + height);
    SelectPen(hdc, hOldPen);
    DeletePen(hPen);

    SetTextColor(hdc, frameColor);
    /*
    if (l <= 0.5f)
    {
        SetTextColor(hdc, COLORREF_WHITE);
    }
    else {
        SetTextColor(hdc, COLORREF_BLACK);
    }
    */
    

    SetBkMode(hdc, TRANSPARENT);
    PCWSTR pszPanitent = L"Panit.ent";
    DrawText(hdc, pszPanitent, wcslen(pszPanitent), &rc, 0);

    SelectFont(hdc, hOldFont);
    SelectBrush(hdc, hOldBrush);
    DeleteBrush(hBrush);
}

void DrawThemedWindowFrameNonClient(Window* pWindow) {

}

typedef struct IAbstractDrawer_vtbl IAbstractDrawer_vtbl;
typedef struct IAbstractDrawer IAbstractDrawer;
typedef struct IAbstractCanvas_vtbl IAbstractCanvas_vtbl;
typedef struct IAbstractCanvas IAbstractCanvas;

struct IAbstractDrawer_vtbl {
    void(__fastcall* DrawRectangle)(void* pThis, IAbstractCanvas* pTarget, int x1, int y1, int x2, int y2);
    void(__fastcall* DrawEllipse)(void* pThis, IAbstractCanvas* pTarget, int x1, int y1, int x2, int y2);
    void(__fastcall* SetStrokeWidth)(void* pThis, int strokeWidth);
};

struct IAbstractDrawer {
    const IAbstractDrawer_vtbl* __vtbl;
};

typedef enum EAbstractCanvasType {
    ECANVAS_UNKNOWN = 0,
    ECANVAS_WIN32DC = 4
} EAbstractCanvasType;

struct IAbstractCanvas_vtbl {
    void(__fastcall* GetBits)(void* pThis, void** pBits);
    void(__fastcall* GetDimensions)(void* pThis, int* width, int* height);
    EAbstractCanvasType(__fastcall* GetType)(void* pThis);
};

struct IAbstractCanvas {
    const IAbstractCanvas_vtbl* __vtbl;
};

typedef struct Win32GDIDrawer Win32GDIDrawer;
typedef struct Win32DCCanvas Win32DCCanvas;
HDC Win32DCCanvas_GetDC(Win32DCCanvas* pThis);

struct Win32GDIDrawer {
    IAbstractDrawer baseIAbstractDrawer;
    HDC hdc;
    int dcState;
    HPEN hPen;
};

void __impl_Win32GDIDrawer_DrawRectangle(Win32GDIDrawer* pThis, IAbstractCanvas* pTarget, int x1, int y1, int x2, int y2) {
    int prevDCState = SaveDC(pThis->hdc);
    RestoreDC(pThis->hdc, pThis->dcState);

    if (pTarget->__vtbl->GetType(pTarget) == ECANVAS_WIN32DC) {
        Win32DCCanvas* pDCCanvas = pTarget;
        Rectangle(Win32DCCanvas_GetDC(pDCCanvas), x1, y1, x2, y2);
    }
    else {
        assert(FALSE);  // E_NOTIMPL
    }
    RestoreDC(pThis->hdc, prevDCState);
}

void __impl_Win32GDIDrawer_DrawEllipse(Win32GDIDrawer* pThis, IAbstractCanvas* pTarget, int x1, int y1, int x2, int y2) {
    int prevDCState = SaveDC(pThis->hdc);
    RestoreDC(pThis->hdc, pThis->dcState);

    if (pTarget->__vtbl->GetType(pTarget) == ECANVAS_WIN32DC) {
        Win32DCCanvas* pDCCanvas = pTarget;
        Ellipse(Win32DCCanvas_GetDC(pDCCanvas), x1, y1, x2, y2);
    }
    else {
        assert(FALSE);  // E_NOTIMPL
    }
    RestoreDC(pThis->hdc, prevDCState);
}

void __impl_Win32GDIDrawer_SetStrokeWidth(Win32GDIDrawer* pThis, int strokeWidth) {
    int prevDCState = SaveDC(pThis->hdc);
    RestoreDC(pThis->hdc, pThis->dcState);
    HPEN hNewPen = NULL;
    if (strokeWidth < 1) {
        SelectObject(pThis->hdc, GetStockObject(NULL_PEN));
    }
    else {
        HPEN hCurrentPen = NULL;
        LOGPEN logPen = { 0 };

        if (!pThis->hPen) {
            hCurrentPen = GetCurrentObject(pThis->hdc, OBJ_PEN);
            if (hCurrentPen) {
                GetObject(hCurrentPen, sizeof(LOGPEN), &logPen);
            }
            else {
                logPen.lopnColor = COLORREF_ORANGE;
                logPen.lopnStyle = PS_SOLID;
            }
        }
        else {
            GetObject(pThis->hPen, sizeof(LOGPEN), &logPen);
        }

        logPen.lopnWidth.x = strokeWidth;
        logPen.lopnWidth.y = strokeWidth;

        hNewPen = CreatePenIndirect(&logPen);
        SelectObject(pThis->hdc, hNewPen);
    }

    if (pThis->hPen) {
        DeleteObject(pThis->hPen);
    }
    pThis->hPen = hNewPen;

    pThis->dcState = SaveDC(pThis->hdc);
    RestoreDC(pThis->hdc, prevDCState);
}

IAbstractDrawer_vtbl __vtbl_Win32GDIDrawer_IAbstractDrawer = {
    .DrawRectangle = &__impl_Win32GDIDrawer_DrawRectangle,
    .DrawEllipse = &__impl_Win32GDIDrawer_DrawEllipse,
    .SetStrokeWidth = &__impl_Win32GDIDrawer_SetStrokeWidth
};

void Win32GDIDrawer_Init(Win32GDIDrawer* pThis, HDC hdc) {
    pThis->baseIAbstractDrawer.__vtbl = &__vtbl_Win32GDIDrawer_IAbstractDrawer;

    pThis->hdc = hdc;
    pThis->dcState = SaveDC(hdc);
}

typedef struct _Win32DCCanvas_MutableBitmapInfo {
    BOOL fInitialized;
    BITMAPINFO bmi;
    HBITMAP hbm;
    void* pBits;
} _Win32DCCanvas_MutableBitmapInfo;

struct Win32DCCanvas {
    IAbstractCanvas baseIAbstractCanvas;
    HDC hdc;
    _Win32DCCanvas_MutableBitmapInfo mutableBitmapInfo;
};

HDC Win32DCCanvas_GetDC(Win32DCCanvas* pThis) {
    return pThis->hdc;
}

BOOL __impl_Win32DCCanvas_ConvertDCBitmapToMutable(Win32DCCanvas* pWin32DCCanvas, _Win32DCCanvas_MutableBitmapInfo* pmbi) {
    ASSERT(pWin32DCCanvas);
    
    if (pWin32DCCanvas->mutableBitmapInfo.fInitialized || pmbi->fInitialized) {
        return FALSE;
    }

    HBITMAP hCurrentBm = GetCurrentObject(pWin32DCCanvas->hdc, OBJ_BITMAP);
    BITMAP currentBm = { 0 };
    GetObject(hCurrentBm, sizeof(BITMAP), &currentBm);
    HDC hMemDC = CreateCompatibleDC(pWin32DCCanvas->hdc);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = currentBm.bmWidth;
    bmi.bmiHeader.biHeight = currentBm.bmHeight;
    bmi.bmiHeader.biPlanes = currentBm.bmPlanes;
    bmi.bmiHeader.biBitCount = currentBm.bmBitsPixel;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hMemBm = CreateDIBSection(pWin32DCCanvas->hdc, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBm = SelectBitmap(hMemDC, hMemBm);
    BitBlt(hMemDC, 0, 0, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight, pWin32DCCanvas->hdc, 0, 0, SRCCOPY);
    SelectBitmap(hMemDC, hOldBm);
    DeleteDC(hMemDC);
    hOldBm = SelectObject(pWin32DCCanvas->hdc, hMemBm);

    memcpy(&pmbi->bmi, &bmi, sizeof(BITMAPINFO));
    pmbi->hbm = hMemBm;
    pmbi->pBits = pBits;
    pmbi->fInitialized = TRUE;
}

void __impl_Win32DCCanvas_ConvertDCBitmapToMutableAndStoreState(Win32DCCanvas* pWin32DCCanvas) {
    if (!pWin32DCCanvas->mutableBitmapInfo.fInitialized) {
        BOOL bResult = __impl_Win32DCCanvas_ConvertDCBitmapToMutable(pWin32DCCanvas, &pWin32DCCanvas->mutableBitmapInfo);
        ASSERT(bResult);
    }
}

void __impl_Win32DCCanvas_GetBits(Win32DCCanvas* pWin32DCCanvas, void** pBits) {
    if (!pWin32DCCanvas->mutableBitmapInfo.fInitialized) {
        __impl_Win32DCCanvas_ConvertDCBitmapToMutableAndStoreState(pWin32DCCanvas);
    }

    *pBits = pWin32DCCanvas->mutableBitmapInfo.pBits;
}

void __impl_Win32DCCanvas_GetDimensions(Win32DCCanvas* pWin32DCCanvas, int* width, int* height) {
    
    HBITMAP hBitmap = GetCurrentObject(pWin32DCCanvas->hdc, OBJ_BITMAP);
    BITMAP bitmap;
    GetObject(hBitmap, sizeof(bitmap), &bitmap);
    *width = bitmap.bmWidth;
    *height = bitmap.bmHeight;
}

EAbstractCanvasType __impl_Win32DCCanvas_GetType(Win32DCCanvas* pWin32DCCanvas) {
    return ECANVAS_WIN32DC;
}

IAbstractCanvas_vtbl __vtbl_Win32DCCanvas_IAbstractCanvas = {
    .GetBits = &__impl_Win32DCCanvas_GetBits,
    .GetDimensions = &__impl_Win32DCCanvas_GetDimensions,
    .GetType = &__impl_Win32DCCanvas_GetType
};

void Win32DCCanvas_Init(Win32DCCanvas* pWin32DCCanvas, HDC hdc)
{
    pWin32DCCanvas->baseIAbstractCanvas.__vtbl = &__vtbl_Win32DCCanvas_IAbstractCanvas;
    pWin32DCCanvas->hdc = hdc;
}

#if 0
void __impl_HSLGradient_Draw(HSLGradient* pHSLGradient, IAbstractCanvas* pTarget, IAbstractDrawer* pDrawer) {
    RECT rc;
    CopyRect(&rc, &pHSLGradient->rc);
    InflateRect(&rc, 2, 2);

    pDrawer->__vtbl->DrawRectangle(pDrawer, pTarget, rc.left, rc.top, rc.right, rc.bottom);

    int width = Win32_Rect_GetWidth(&pHSLGradient->rc);
    int height = Win32_Rect_GetHeight(&pHSLGradient->rc);

    DrawHSLGradient(hdc,
        pHSLGradient->rc.left,
        pHSLGradient->rc.top,
        width,
        height);

    /*
    int gripX = pHSLGradient->rc.left + pHSLGradient->hue / 360.0f * (float)width;
    int gripY = pHSLGradient->rc.top + (1.0f - pHSLGradient->lightness) * (float)height;

    DrawGrip(hdc, gripX, gripY);
    */
}
#endif

void __impl_HSLGradient_GetBoundingRect(HSLGradient* pHSLGradient, RECT* prc) {
    CopyRect(prc, &pHSLGradient->rc);
}

BOOL __impl_HSLGradient_OnLButtonDown(HSLGradient* pHSLGradient, int x, int y) {
    int xLocal = x - pHSLGradient->rc.left;
    int yLocal = y - pHSLGradient->rc.top;

    float width = (float)Win32_Rect_GetWidth(&pHSLGradient->rc);
    float height = (float)Win32_Rect_GetHeight(&pHSLGradient->rc);

    float hue = (float)xLocal * (360.0f / width);
    float lightness = 1.0f - (float)yLocal / height;

    pHSLGradient->hue = hue;
    pHSLGradient->lightness = lightness;
}

BOOL __impl_HSLGradient_OnLButtonUp(HSLGradient* pHSLGradient, int x, int y) {

}

void ConstrainPtInRectFU(RECT* prc, POINT* ppt) {
    ppt->x = (ppt->x < prc->left) ? prc->left : (ppt->x >= prc->right) ? prc->right : ppt->x;
    ppt->y = (ppt->y < prc->top) ? prc->top : (ppt->y >= prc->bottom) ? prc->bottom : ppt->y;
}

BOOL __impl_HSLGradient_OnMouseMove(HSLGradient* pHSLGradient, int x, int y) {
    POINT pt = {
        .x = x,
        .y = y
    };
    
    ConstrainPtInRectFU(&pHSLGradient->rc, &pt);

    int xLocal = pt.x - pHSLGradient->rc.left;
    int yLocal = pt.y - pHSLGradient->rc.top;

    float width = (float)Win32_Rect_GetWidth(&pHSLGradient->rc);
    float height = (float)Win32_Rect_GetHeight(&pHSLGradient->rc);

    float hue = (float)xLocal * (360.0f / width);
    float lightness = 1.0f - (float)yLocal / height;

    pHSLGradient->hue = hue;
    pHSLGradient->lightness = lightness;
}

IWidget_vtbl __vtbl_IWidget_HSLGradient = {
    .GetBoundingRect = &__impl_HSLGradient_GetBoundingRect,
    .OnLButtonDown = &__impl_HSLGradient_OnLButtonDown,
    .OnLButtonUp = &__impl_HSLGradient_OnLButtonUp,
    .OnMouseMove = &__impl_HSLGradient_OnMouseMove
};

void HSLGradient_Init(HSLGradient* pHSLGradient) {
    pHSLGradient->iwidget.__vtbl = &__vtbl_IWidget_HSLGradient;

    pHSLGradient->rc = (RECT){
        .left = 10,
        .top = 10,
        .right = 10 + 256,
        .bottom = 10 + 256
    };

    pHSLGradient->hue = 0.0f;
    pHSLGradient->lightness = 0.0f;
}

void PlaygroundDlg_OnLButtonDown(PlaygroundDlg* pPlaygroundDlg, BOOL fDoubleClick, int x, int y, UINT keyFlags) {
    POINT pt = {
        .x = x,
        .y = y
    };

    
}

void PlaygroundDlg_OnLButtonUp(PlaygroundDlg* pPlaygroundDlg, int x, int y, UINT keyFlags) {
}

void PlaygroundDlg_OnMouseMove(PlaygroundDlg* pPlaygroundDlg, int x, int y) {
    POINT pt = {
        .x = x,
        .y = y
    };

    for (size_t i = 0; i < Vector_GetSize(&pPlaygroundDlg->widgets); ++i) {
        IWidget* pWidget = (IWidget*)Vector_At(&pPlaygroundDlg, i);

        RECT rcWidget;
        pWidget->__vtbl->GetBoundingRect(pWidget, &rcWidget);

        if (PtInRect(&rcWidget, pt)) {
            pWidget->__vtbl->OnMouseMove(pWidget, x, y);
        }
    }
}

void PlaygroundDlg_OnPaint(PlaygroundDlg* pPlaygroundDlg) {
    PAINTSTRUCT ps = { 0 };
    HWND hWnd = Window_GetHWND(pPlaygroundDlg);
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rcClient = { 0 };
    Window_GetClientRect(pPlaygroundDlg, &rcClient);

    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hMemBm = CreateCompatibleBitmap(hdc, Win32_Rect_GetWidth(&rcClient), Win32_Rect_GetHeight(&rcClient));
    HBITMAP hOldBm = SelectBitmap(hMemDC, hMemBm);

    // Fill background
    {
        HBRUSH hBackgroundBrush = GetStockObject(WHITE_BRUSH);
        FillRect(hMemDC, &rcClient, hBackgroundBrush);
        DeleteObject(hBackgroundBrush);
    }

    HSLGradient_Draw(&pPlaygroundDlg->gradient, hMemDC);

    BitBlt(hdc, 0, 0, Win32_Rect_GetWidth(&rcClient), Win32_Rect_GetHeight(&rcClient), hMemDC, 0, 0, SRCCOPY);

    SelectBitmap(hMemDC, hOldBm);
    DeleteBitmap(hOldBm);
    DeleteDC(hMemDC);

    EndPaint(hWnd, &ps);
}

INT_PTR PlaygroundDlg_UserProcDlg(PlaygroundDlg* pPlaygroundDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
        PlaygroundDlg_OnPaint(pPlaygroundDlg);
        break;

    case WM_LBUTTONDOWN:
        PlaygroundDlg_OnLButtonDown(pPlaygroundDlg, FALSE, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam), (UINT)wParam);
        break;

    case WM_LBUTTONUP:
        PlaygroundDlg_OnLButtonUp(pPlaygroundDlg, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam), (UINT)wParam);
        break;

    case WM_MOUSEMOVE:
        PlaygroundDlg_OnMouseMove(pPlaygroundDlg, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        break;
    }

    return Dialog_DefaultDialogProc(pPlaygroundDlg, message, wParam, lParam);
}
