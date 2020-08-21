#include "swatch.h"
#include <commctrl.h>
#include <assert.h>

typedef struct _swatch_data {
  uint32_t color;
} swatch_data_t;

typedef struct _window_data {
  HWND hwnd;
  void* data;
} window_data_t;

typedef struct _window_data_vault {
  size_t size;
  size_t capacity;
  window_data_t* vault;
} window_data_vault_t;

window_data_vault_t g_window_data_vault;

void window_data_register(HWND hWnd, void* data)
{
  // TODO 1. Binary tree, 2. re-use entries
  window_data_vault_t* gwdv = &g_window_data_vault;
  if (gwdv->size >= gwdv->capacity)
  {
    size_t new_cap = gwdv->capacity + 16;
    gwdv->vault = realloc(gwdv->vault, new_cap);
    assert(gwdv->vault);
    gwdv->capacity = new_cap;
  }

  window_data_t wd = {0};
  wd.hwnd = hWnd;
  wd.data = data;
  gwdv->vault[gwdv->size++] = wd;
}

BOOL window_data_set(HWND hWnd, void* data)
{
  window_data_vault_t* gwdv = &g_window_data_vault;

  for (size_t i = 0; i < gwdv->size; i++)
  {
    if (gwdv->vault[i].hwnd == hWnd)
    {
      gwdv->vault[i].data = data;
      return TRUE;
    }
  }

  return FALSE;
}

void* window_data_get(HWND hWnd)
{
  // TODO Binary search
  window_data_vault_t* gwdv = &g_window_data_vault;

  for (size_t i = 0; i < gwdv->size; i++)
  {
    if (gwdv->vault[i].hwnd == hWnd)
    {
      return gwdv->vault[i].data;
      break;
    }
  }

  return NULL;
}

unsigned char custom_colors[];

LRESULT CALLBACK SwatchControl_WndProc(HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData)
{
  switch (uMsg)
  {
    case WM_NCHITTEST:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
      break;
    case WM_LBUTTONDOWN:
      {
        swatch_data_t* sd = (swatch_data_t*)GetWindowLongPtr(hWnd,
            GWLP_USERDATA);
        assert(sd);

        CHOOSECOLOR cc = {0};
        cc.lStructSize = sizeof(CHOOSECOLOR);
        cc.hwndOwner = hWnd;
        cc.lpCustColors = (LPDWORD)&custom_colors;
        cc.rgbResult = sd->color;
        cc.lpfnHook = NULL;
        BOOL bResult = ChooseColor(&cc);
        if (bResult) {
          sd->color = cc.rgbResult; 
          InvalidateRect(hWnd, NULL, TRUE);
          SendMessage(GetParent(hWnd), WM_COMMAND,
              (WPARAM)MAKEWPARAM(GetMenu(hWnd), SCN_COLORCHANGE),
              (LPARAM)hWnd);
        }
      }
      return TRUE;
      break;
    case SCM_GETCOLOR:
      {
        swatch_data_t* sd = (swatch_data_t*)GetWindowLongPtr(hWnd,
            GWLP_USERDATA);
        return sd->color;
      }
      break;
    case WM_PAINT:
      {
        swatch_data_t* sd = (swatch_data_t*)GetWindowLongPtr(hWnd,
            GWLP_USERDATA);
        assert(sd);
        HBRUSH hSolidBrush = CreateSolidBrush(sd->color);

        RECT wndRc = {0};
        GetWindowRect(hWnd, &wndRc);

        PAINTSTRUCT ps = {0};
        HDC hDC = BeginPaint(hWnd, &ps);
        RECT rc = {1, 1, wndRc.right-wndRc.left-3, wndRc.bottom-wndRc.top-3};
        FillRect(hDC, &rc, hSolidBrush); 
        EndPaint(hWnd, &ps);
      }
      return TRUE;
      break;
  }

  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

HWND SwatchControl_Create(uint32_t color, int x, int y, int width,
    int height, HWND hParent, HMENU hMenu)
{
  swatch_data_t *swatch_data = calloc(1, sizeof(swatch_data_t));
  swatch_data->color = color;
  assert(swatch_data);
  HWND hWnd = CreateWindowEx(0, WC_STATIC, NULL,
      SS_SIMPLE | WS_BORDER | WS_VISIBLE | WS_CHILD, x, y, width, height,
      hParent, hMenu, GetModuleHandle(NULL), NULL);

  assert(hWnd);
  SetWindowSubclass(hWnd, SwatchControl_WndProc, 0, 0);
  SetWindowLongPtr(hWnd, GWLP_USERDATA, swatch_data);

  return hWnd;
}
