#include "precomp.h"
#include <windowsx.h>
#include <commctrl.h>

#include "layers.h"
#include "resource.h"
#include "panitent.h"
#include "canvas.h"
#include "document.h"
#include "commontypes.h"

#define IDB_LAYER_CREATE 1254 

static void OnCreate(HWND, WPARAM, LPARAM);
static void OnPaint(HWND, WPARAM, LPARAM);
static void OnCommand(HWND, WPARAM, LPARAM);
static void OnLButtonDown(HWND, WPARAM, LPARAM);

const WCHAR szLayersWndClass[] = L"Win32Class_LayersWindow";

struct _Layer {
  Canvas* canvas;
  BOOL bVisible;
  BOOL bLocked;
  BOOL bSelected;
  LPWSTR lpszName;
};

struct _LayerManager {
  Layer** layers;
  size_t length;
  size_t capacity;
  Document* document;
  Layer* selected;
};

Layer* Layer_Create(Canvas* canvas)
{
  Layer* layer = calloc(1, sizeof(Layer));

  layer->canvas = canvas;
  layer->bVisible = TRUE;
  layer->lpszName = L"New layer";
  return layer;
}

LayerManager* LayerManager_Create(Document* document)
{
  LayerManager* manager = calloc(1, sizeof(LayerManager));
  manager->document = document;

  manager->layers = calloc(8, sizeof(Layer));
  manager->capacity = 8;
  manager->length = 0;
  return manager;
}

void LayerManager_AppendEmptyLayer(LayerManager* manager)
{
  Rect size = Document_GetSize(manager->document);
  Canvas* canvas = Canvas_New(size.right, size.bottom);

  Layer* newLayer = Layer_Create(canvas);
  if (manager->length >= manager->capacity)
  {
    size_t newcap = manager->capacity + 4;
    manager->layers = realloc(manager->layers, newcap);
    manager->capacity = newcap;
  }

  manager->layers[manager->length++] = newLayer; 
}

static LRESULT CALLBACK Layers_WndProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      OnCreate(hwnd, wParam, lParam);
      return 0;
    case WM_PAINT:
      OnPaint(hwnd, wParam, lParam);
      return 0;
    case WM_COMMAND:
      OnCommand(hwnd, wParam, lParam);
      return 0;
    case WM_LBUTTONDOWN:
      OnLButtonDown(hwnd, wParam, lParam);
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL Layers_Register(HINSTANCE hInstance)
{
  WNDCLASS wc = {0};
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = (WNDPROC)Layers_WndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = szLayersWndClass;

  return RegisterClass(&wc);
}

static void OnCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  HWND hAddLayerBtn = CreateWindowEx(0, WC_BUTTON, L"+", WS_CHILD | WS_VISIBLE,
      0, 0, 30, 30, hwnd, (HMENU)IDB_LAYER_CREATE,
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL); 
  SetGuiFont(hAddLayerBtn);
}

void DrawLayerItem(HDC hdc, RECT cliRc, Layer* layer, DWORD* offset, BOOL bSelect)
{
  int y = *offset;

  HDC hBmDc;
  HGDIOBJ hOldObj;

  if (layer->bSelected)
  {
    RECT selRc = cliRc;
    selRc.top = y;
    selRc.bottom = y + 64;
    FillRect(hdc, &selRc, GetSysColorBrush(COLOR_HIGHLIGHT));
  }

  HBITMAP hBmpControls = LoadBitmap(GetModuleHandle(NULL),
      MAKEINTRESOURCE(IDB_LAYERCTL));
  HBITMAP hBmPict = (HBITMAP)LoadImage(NULL, L"ph_layerpict.bmp", IMAGE_BITMAP,
      0, 0, LR_LOADFROMFILE);


  hBmDc = CreateCompatibleDC(hdc);

  hOldObj = SelectObject(hBmDc, hBmPict);
  BitBlt(hdc, 8, 8 + y, 48, 48, hBmDc, 0, 0, SRCCOPY);
  SelectObject(hBmDc, hOldObj); 

  hOldObj = SelectObject(hBmDc, hBmpControls);

  if (layer->bVisible)
  {
    TransparentBlt(hdc, cliRc.right - 26, 24 + y, 16, 16, hBmDc, 0, 0, 16, 16, RGB(255, 0, 255));
  }
  else {
    TransparentBlt(hdc, cliRc.right - 26, 24 + y, 16, 16, hBmDc, 16, 0, 16, 16, RGB(255, 0, 255));
  }

  if (layer->bLocked)
  {
    TransparentBlt(hdc, cliRc.right - 32 - 10, 24 + y, 16, 16, hBmDc, 48, 0, 16, 16, RGB(255, 0, 255));
  }
  else {
    TransparentBlt(hdc, cliRc.right - 32 - 10, 24 + y, 16, 16, hBmDc, 32, 0, 16, 16, RGB(255, 0, 255));
  }

  SelectObject(hBmDc, hOldObj); 

  RECT frameRc = {0};
  frameRc.left = 7;
  frameRc.top = 7 + y;
  frameRc.right = 48 + 9;
  frameRc.bottom = 48 + 9 + y;
  FrameRect(hdc, &frameRc, GetStockObject(BLACK_BRUSH));

  HFONT hFont = GetGuiFont();

  if (layer->bSelected)
  {
    SetTextColor(hdc, RGB(255, 255, 255));
  }
  else {
    SetTextColor(hdc, RGB(0, 0, 0));
  }
  SetBkMode(hdc, TRANSPARENT);
  hOldObj = SelectObject(hdc, hFont);

  size_t dNameLen = wcslen(layer->lpszName);
  TextOut(hdc, 48 + 19, 24 + y, layer->lpszName, dNameLen);
  SelectObject(hdc, hOldObj);

  *offset += 64;
}

static void OnPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Document* document = GetActiveDocument();
  if (!document)
  {
    PAINTSTRUCT ps = {0};
    HDC hdc;
    hdc = BeginPaint(hwnd, &ps);

    TextOut(hdc, 0, 40, L"No document", 11);

    EndPaint(hwnd, &ps);
    return;
  }

  LayerManager* layers = Document_GetLayerManager(document);

  if (!layers->length)
  {
    PAINTSTRUCT ps = {0};
    HDC hdc;
    hdc = BeginPaint(hwnd, &ps);

    TextOut(hdc, 0, 40, L"No layers in document", 21);

    EndPaint(hwnd, &ps);
    return;
  }

  PAINTSTRUCT ps = {0};
  HDC hdc;

  RECT clientRc;
  GetClientRect(hwnd, &clientRc);

  hdc = BeginPaint(hwnd, &ps);

  DWORD offset = 30;
  for (size_t i = 0; i < layers->length; i++)
  {
    DrawLayerItem(hdc, clientRc, layers->layers[i], &offset, FALSE);
  }

  EndPaint(hwnd, &ps);
}

HWND Layers_Create(HWND hwndParent)
{
  return CreateWindowEx(0, szLayersWndClass, L"Layers", WS_BORDER | WS_VISIBLE | WS_CHILD,
      0, 0, 0, 0, hwndParent, NULL,
      (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE), NULL);
}

void OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  if (LOWORD(wParam) == IDB_LAYER_CREATE && HIWORD(wParam) == BN_CLICKED)
  {
    Document* document = GetActiveDocument();
    LayerManager* layers = Document_GetLayerManager(document);
    LayerManager_AppendEmptyLayer(layers);
    InvalidateRect(hwnd, NULL, TRUE);
  }
}

BOOL EyeHitTest(HWND hwnd, unsigned short x, unsigned short y)
{
  RECT rc = {0};
  GetClientRect(hwnd, &rc);

  return (
    x > (rc.right - 10 - 16) &&
    y > 24 &&
    x <= (rc.right - 10) &&
    y <= 40
  );
}

BOOL LockHitTest(HWND hwnd, unsigned short x, unsigned short y)
{
  RECT rc = {0};
  GetClientRect(hwnd, &rc);

  return (
    x > (rc.right - 10 - 32) &&
    y > 24 &&
    x <= (rc.right - 10 - 16) &&
    y <= 40
  );
}

void OnLButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  unsigned short x = GET_X_LPARAM(lParam);
  unsigned short y = GET_Y_LPARAM(lParam);

  if (y < 30)
    return;
  y -= 30;

  Document* document = GetActiveDocument();
  LayerManager* layers = Document_GetLayerManager(document);

  size_t idx = y / 64;
  
  if (idx >= layers->length)
    return;

  if (EyeHitTest(hwnd, x, y % 64))
  {
    layers->layers[idx]->bVisible = layers->layers[idx]->bVisible ? FALSE : TRUE;    
    InvalidateRect(hwnd, NULL, TRUE);
    return;
  }

  if (LockHitTest(hwnd, x, y % 64))
  {
    layers->layers[idx]->bLocked = layers->layers[idx]->bLocked ? FALSE : TRUE;    
    InvalidateRect(hwnd, NULL, TRUE);
    return;
  }

  for (size_t i = 0; i < layers->length; i++)
  {
    layers->layers[i]->bSelected = FALSE;
  }

  layers->layers[idx]->bSelected = TRUE;
  InvalidateRect(hwnd, NULL, TRUE);
}
