#include "option_bar.h"
#include "panitent.h"
#include <commctrl.h>
#include <windowsx.h>
#include "primitives_context.h"
#include "wu_primitives.h"
#include "bresenham.h"
#include "swatch.h"
#include "brush.h"
#include <assert.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include "resource.h"
#include <math.h>
#include <strsafe.h>

OptionBar g_option_bar;

#define IDCB_STENCIL_ALGORITHM 1553
#define IDCB_THICKNESS 1554
#define IDB_SHAPESTROKE 1555
#define IDB_SHAPEFILL 1556

INT_PTR CALLBACK BrushProp_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam,
    LPARAM lParam);

extern BrushBuilder g_brushList[80];

WCHAR szBrushSelClass[] = L"Win32Class_BrushSel";
static HTHEME hTheme = NULL;

void BrushSel_OnCreate(HWND hwnd, LPCREATESTRUCT lParam)
{
  hTheme = OpenThemeData(hwnd, L"ComboBox");
}

void BrushSel_OnPaint(HWND hwnd)
{
  BrushBuilder* builder = g_pBrush;

  Brush *brush = BrushBuilder_Build(builder, 24);
  Canvas *tex = Brush_GetTexture(brush);

  PAINTSTRUCT ps = {0};
  HDC hdc = BeginPaint(hwnd, &ps);

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = tex->width;
  bmi.bmiHeader.biHeight = tex->height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;
  bmi.bmiHeader.biClrUsed = 0;
  bmi.bmiHeader.biClrImportant = 0;

  /*
  bmi.bmiHeader.biSizeImage = ((bmi.bmiHeader.biWidth *
      bmi.bmiHeader.biBitCount + 31) / 32) * 4 * bmi.bmiHeader.biHeight;
  */

  uint32_t *buffer = NULL;

  HDC hSampleDC = CreateCompatibleDC(hdc);
  HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
      (LPVOID*)&buffer, NULL, 0);
  assert(hBitmap != NULL);
  assert(buffer != NULL);

  for (int y = 0; y < (int)bmi.bmiHeader.biHeight; y++)
  {
    /*
    memcpy(buffer + y * bmi.bmiHeader.biWidth,
        (uint32_t*)tex->buffer + y * tex->width, bmi.bmiHeader.biWidth);
    */

    for (int x = 0; x < (int)bmi.bmiHeader.biWidth; x++)
    {
      buffer[y * bmi.bmiHeader.biWidth + x] = mix(0xFFFFFFFF,
          *((uint32_t*)tex->buffer + y * tex->width + x));
    }
  }

  HGDIOBJ hOldObj = SelectObject(hSampleDC, hBitmap);
  BitBlt(hdc, 0, 0, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight, hSampleDC,
      0, 0, SRCCOPY);
  SelectObject(hSampleDC, hOldObj);

  DeleteObject(hBitmap);
  DeleteDC(hSampleDC);

  RECT clientRc = {0};
  GetClientRect(hwnd, &clientRc);

  HFONT hFont = GetGuiFont();
  hOldObj = SelectObject(hdc, hFont);

  WCHAR szBrushSize[4];
  _itow(g_brushSize, szBrushSize, 10);

  SIZE sText;
  GetTextExtentPoint32(hdc, szBrushSize, 2, &sText);

  TextOut(hdc, 24, (clientRc.bottom - sText.cy) / 2, szBrushSize, 2);

  SelectObject(hdc, hOldObj);

  if (hTheme)
  {
    RECT dropBtnRc = clientRc;
    dropBtnRc.left = dropBtnRc.right - 16;

    DrawThemeBackground(hTheme, hdc, CP_DROPDOWNBUTTON, CBXS_NORMAL,
        &dropBtnRc, NULL);
  }

  EndPaint(hwnd, &ps);

  Brush_Delete(brush);
}

void BrushSel_OnDestroy(HWND hwnd)
{
  CloseThemeData(hTheme);
}

void BrushSel_OnLButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_BRUSHPROP), hwnd,
      (DLGPROC)BrushProp_DlgProc);
}

LRESULT CALLBACK BrushSel_WndProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      BrushSel_OnCreate(hwnd, (LPCREATESTRUCT)lParam);
      return 0;
    case WM_PAINT:
      BrushSel_OnPaint(hwnd);
      return 0;
    case WM_LBUTTONDOWN:
      BrushSel_OnLButtonDown(hwnd, wParam, lParam);
      return 0;
    case WM_DESTROY:
      BrushSel_OnDestroy(hwnd);
      return 0;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL BrushSel_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)BrushSel_WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = szBrushSelClass;
  wcex.hIconSm = NULL;
  return RegisterClassEx(&wcex);
}

void OptionBar_OnCommand(WPARAM wparam, LPARAM lparam)
{
  if (HIWORD(wparam) == BN_CLICKED) {
    switch (LOWORD(wparam))
    {
      case IDB_SHAPESTROKE:
        g_primitives_context.fStroke = Button_GetCheck((HWND)lparam);
        break;
      case IDB_SHAPEFILL:
        g_primitives_context.fFill = Button_GetCheck((HWND)lparam);
        break;
    }
  }

  if (HIWORD(wparam) != LBN_SELCHANGE)
    return;

  switch (LOWORD(wparam))
  {
    case IDCB_STENCIL_ALGORITHM:
      switch (ComboBox_GetCurSel((HWND)lparam))
      {
        case 0:
          g_primitives_context = g_bresenham_primitives;
          break;
        case 1:
          g_primitives_context = g_wu_primitives;
          break;
      }
      break;
    case IDCB_THICKNESS:
      SetThickness(ComboBox_GetCurSel((HWND)lparam) + 1);
      break;
  }

  if (LOWORD(wparam) == IDCB_STENCIL_ALGORITHM &&
      HIWORD(wparam) == LBN_SELCHANGE) {
    switch (ComboBox_GetCurSel((HWND)lparam)) {
      break;
    default:
      break;
    }
  }
}

LRESULT CALLBACK OptionBar_WndProc(HWND hwnd, UINT message, WPARAM wparam,
    LPARAM lparam)
{
  switch (message) {
  case WM_CREATE:
  {
    HWND hCheckStroke = CreateWindowEx(0, WC_BUTTON, L"Stroke",
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
        4, 3, 70, 20, hwnd, (HMENU)IDB_SHAPESTROKE, GetModuleHandle(NULL),
        NULL);
    SetGuiFont(hCheckStroke);
    Button_SetCheck(hCheckStroke, g_primitives_context.fStroke);

    HWND hCheckFill = CreateWindowEx(0, WC_BUTTON, L"Fill",
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
        74, 3, 70, 20, hwnd, (HMENU)IDB_SHAPEFILL, GetModuleHandle(NULL),
        NULL);
    SetGuiFont(hCheckFill);
    Button_SetCheck(hCheckFill, g_primitives_context.fFill);

    HWND hcombo = CreateWindowEx(0, WC_COMBOBOX, L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED |
            WS_VISIBLE,
        144, 3, 100, 20,
        hwnd, (HMENU)IDCB_STENCIL_ALGORITHM, GetModuleHandle(NULL), NULL);
    SetGuiFont(hcombo);

    ComboBox_AddString(hcombo, L"Bresenham");
    ComboBox_AddString(hcombo, L"Wu");

    HWND hComboThickness= CreateWindowEx(0, WC_COMBOBOX, L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED |
        WS_VISIBLE,
        260, 3, 100, 20,
        hwnd, (HMENU)IDCB_THICKNESS, GetModuleHandle(NULL), NULL);
    SetGuiFont(hComboThickness);

    WCHAR szThickness[4];
    for (size_t i = 1; i <= 10; ++i)
    {
      _itow_s(i, szThickness, 4, 10);
      ComboBox_AddString(hComboThickness, szThickness);
    }

    HWND hedit =
        CreateWindowEx(0,
                       WC_EDIT,
                       L"Sample Text",
                       WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                       376,
                       3,
                       140,
                       20,
                       hwnd,
                       NULL,
                       GetModuleHandle(NULL),
                       NULL);
    SetGuiFont(hedit);
    g_option_bar.textstring_handle = hedit;

    HWND hBrushSel = CreateWindowEx(0, szBrushSelClass, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        526, 0, 64, 24,
        hwnd, NULL, GetModuleHandle(NULL), NULL);

  } break;
  case WM_COMMAND:
    OptionBar_OnCommand(wparam, lparam);
    break;
  default:
    return DefWindowProc(hwnd, message, wparam, lparam);
    break;
  }

  return 0;
}

void OptionBar_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex    = {0};
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.lpfnWndProc   = (WNDPROC)OptionBar_WndProc;
  wcex.hInstance     = hInstance;
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.lpszClassName = L"Win32Class_OptionBar";

  ATOM class_atom = RegisterClassEx(&wcex);
  if (!class_atom) {
    MessageBox(NULL,
               L"Failed to register option bar class!",
               NULL,
               MB_OK | MB_ICONERROR);
    return;
  }

  g_option_bar.win_class = class_atom;
}

void OptionBar_Create(HWND hwnd)
{
  HWND handle = CreateWindowEx(0,
                               MAKEINTATOM(g_option_bar.win_class),
                               NULL,
                               WS_CHILD | WS_VISIBLE,
                               0,
                               0,
                               500,
                               64,
                               hwnd,
                               NULL,
                               GetModuleHandle(NULL),
                               NULL);

  if (!handle) {
    MessageBox(NULL,
               L"Failed to create option bar window!",
               NULL,
               MB_OK | MB_ICONERROR);
    return;
  }

  g_option_bar.win_handle = handle;
}

typedef struct _BrushDlgData {
  int iPvWidth;
  int iPvHeight;
  HBITMAP hPvBitmap;
  uint32_t *pPvBuffer;
} BrushDlgData;

void DrawBrushPreview(HWND hwnd)
{
  BrushDlgData *data = (BrushDlgData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(data);
  if (!data)
    return;

  int width = data->iPvWidth;
  int height = data->iPvHeight;

  Canvas* canvas = Canvas_Create(width, height);
  Canvas_FillSolid(canvas, 0xFFFFFFFF);

  Brush* brush = BrushBuilder_Build(g_pBrush, g_brushSize);

  Brush_BezierCurve2(brush, canvas,
      32, height/2,
      32 + (width - 64) / 4, height/4,
      width - 32 - (width - 64) / 4, height/4*3,
      width - 32, height/2);

  for (int y = height; y--;)
  {
    memcpy(data->pPvBuffer + y * width, ((uint32_t*)canvas->buffer) + y * width,
        width * 4);
  }
}

INT_PTR CALLBACK BrushProp_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
      {
        BrushDlgData *data = malloc(sizeof(BrushDlgData));
        ZeroMemory(data, sizeof(BrushDlgData));

        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)data);

        HWND hList = GetDlgItem(hwndDlg, IDC_BRUSHLIST);
        assert(hList);

        int nItem;

        /* Add brush items to list */

        for (size_t i = 0; i < g_brushListLen; i++)
        {
          /* What 0 actually do? */
          nItem = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)NULL);
          SendMessage(hList, LB_SETITEMDATA, (WPARAM)nItem,
              (LPARAM)&g_brushList[i]);
        }

        /* Create preview bitmap */
        RECT pvRc = {0};
        GetClientRect(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), &pvRc);

        int width = pvRc.right;
        int height = pvRc.bottom;

        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = 0;
        bmi.bmiHeader.biXPelsPerMeter = 0;
        bmi.bmiHeader.biYPelsPerMeter = 0;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;

        uint32_t *buffer = NULL;

        HDC hdc = GetDC(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW));
        HBITMAP hbmPv = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
            (LPVOID*)&buffer, NULL, 0);
        assert(hbmPv);
        assert(buffer);

        data->iPvWidth = width;
        data->iPvHeight = height;
        data->hPvBitmap = hbmPv;
        data->pPvBuffer = buffer;

        /*
         *  Preview would be drawn here but freezes for an unknown reason
         *  Perhaps GDI is cloning it internally if there any pixels in buffer
         *  I have read something like this on MSDN
         */
        SendMessage(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), STM_SETIMAGE,
            (WPARAM)IMAGE_BITMAP, (LPARAM)hbmPv);

        DrawBrushPreview(hwndDlg);
        InvalidateRect(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), NULL, TRUE);
        InvalidateRect(GetParent(hwndDlg), NULL, TRUE);

        /* Trackbar control */
        HWND hTrack = GetDlgItem(hwndDlg, IDC_BRUSHSIZE);
        SendMessage(hTrack, TBM_SETRANGE, (WPARAM)TRUE,
            (LPARAM)MAKELONG(0, 100));
        SendMessage(hTrack, TBM_SETPAGESIZE, 0, (LPARAM)4);
        SendMessage(hTrack, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)g_brushSize);

      }
      return TRUE;
    case WM_HSCROLL:
      switch (LOWORD(wParam))
      {
        case TB_THUMBTRACK:
          {
            DWORD dwPos = SendMessage(GetDlgItem(hwndDlg, IDC_BRUSHSIZE),
                TBM_GETPOS, 0, 0);

            g_brushSize = dwPos;

            DrawBrushPreview(hwndDlg);
            InvalidateRect(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), NULL, TRUE);
            InvalidateRect(GetParent(hwndDlg), NULL, TRUE);
          }
          break;
      }
      return TRUE;
    case WM_MEASUREITEM:
      {
        PMEASUREITEMSTRUCT pmis = (PMEASUREITEMSTRUCT)lParam;

        pmis->itemHeight = 32;
      }
      return TRUE;

    case WM_DRAWITEM:
      {
        PDRAWITEMSTRUCT pdis = (PDRAWITEMSTRUCT)lParam;

        if (pdis->itemID == (DWORD)-1)
          break;

        switch (pdis->itemAction)
        {
          case ODA_SELECT:
          case ODA_DRAWENTIRE:
            {
              FillRect(pdis->hDC, &pdis->rcItem,
                  GetSysColorBrush(COLOR_WINDOW));

              if (pdis->itemState & ODS_SELECTED)
              {
                FillRect(pdis->hDC, &pdis->rcItem,
                    GetSysColorBrush(COLOR_HIGHLIGHT));
              }

              BrushBuilder* brushBuilder = (BrushBuilder*)SendMessage(
                  pdis->hwndItem, LB_GETITEMDATA, pdis->itemID, 0);

              Brush* brush = BrushBuilder_Build(brushBuilder, 16);
              assert(brush);

              Canvas *canvas = Canvas_Create(
                  pdis->rcItem.right - pdis->rcItem.left,
                  pdis->rcItem.bottom - pdis->rcItem.top);

              Brush_Draw(brush, 16, 16, canvas, 0xFF000000);

              Brush_BezierCurve2(brush, canvas,
                  48, 16,
                  48 + 32, 8,
                  canvas->width - 16 - 32, 24,
                  canvas->width - 16, 16);

              BITMAPINFO bmi = {0};
              bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
              bmi.bmiHeader.biWidth = canvas->width;
              bmi.bmiHeader.biHeight = canvas->height;
              bmi.bmiHeader.biPlanes = 1;
              bmi.bmiHeader.biBitCount = 32;
              bmi.bmiHeader.biCompression = BI_RGB;
              bmi.bmiHeader.biSizeImage = 0;
              bmi.bmiHeader.biXPelsPerMeter = 0;
              bmi.bmiHeader.biYPelsPerMeter = 0;
              bmi.bmiHeader.biClrUsed = 0;
              bmi.bmiHeader.biClrImportant = 0;

              uint32_t *buffer = NULL;
              HBITMAP hBitmap = CreateDIBSection(pdis->hDC, &bmi,
                  DIB_RGB_COLORS, (LPVOID*)&buffer, NULL, 0);
              assert(hBitmap != NULL);
              assert(buffer != NULL);

              int width = bmi.bmiHeader.biWidth;
              int height = bmi.bmiHeader.biHeight;

              for (int y = height; y--;)
              {
                memcpy(buffer + y * width, ((uint32_t*)canvas->buffer) + y *
                      canvas->width, canvas->width * 4);
              }

              HDC hdcMem = CreateCompatibleDC(pdis->hDC);
              HGDIOBJ hOldObj = SelectObject(hdcMem, hBitmap);

              BLENDFUNCTION blendFunc = {
                .BlendOp = AC_SRC_OVER,
                .BlendFlags = 0,
                .SourceConstantAlpha = 0xFF,
                .AlphaFormat = AC_SRC_ALPHA
              };

              AlphaBlend(
                  pdis->hDC,
                  pdis->rcItem.left,
                  pdis->rcItem.top,
                  pdis->rcItem.right - pdis->rcItem.left,
                  pdis->rcItem.bottom - pdis->rcItem.top,
                  hdcMem,
                  0, 0,
                  pdis->rcItem.right - pdis->rcItem.left,
                  pdis->rcItem.bottom - pdis->rcItem.top,
                  blendFunc);

              /*
              BitBlt(
                  pdis->hDC,
                  pdis->rcItem.left,
                  pdis->rcItem.top,
                  pdis->rcItem.right - pdis->rcItem.left,
                  pdis->rcItem.bottom - pdis->rcItem.top,
                  hdcMem, 0, 0, SRCCOPY);
                  */

              SelectObject(hdcMem, hOldObj);

              DeleteObject(hBitmap);
              DeleteDC(hdcMem);

              Canvas_Delete(canvas);
            }
            break;

          case ODA_FOCUS:
            DrawFocusRect(pdis->hDC, &pdis->rcItem);
            break;
        }
      }
      return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_BRUSHLIST && HIWORD(wParam) == LBN_SELCHANGE)
      {
        HWND hList = GetDlgItem(hwndDlg, IDC_BRUSHLIST);

        int nItem = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
        BrushBuilder* builder = (BrushBuilder*)SendMessage(hList,
            LB_GETITEMDATA, nItem, 0);

        g_pBrush = builder;

        DrawBrushPreview(hwndDlg);
        InvalidateRect(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), NULL, TRUE);
        InvalidateRect(GetParent(hwndDlg), NULL, TRUE);
      }

      if (HIWORD(wParam) == BN_CLICKED)
      {
        switch (LOWORD(wParam))
        {
          case IDCANCEL:
            /* fall through */
          case IDOK:
            EndDialog(hwndDlg, wParam);
            break;
        }
      }
      return TRUE;
  }

  return FALSE;
}
