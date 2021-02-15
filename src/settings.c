#include "precomp.h"

#include <commctrl.h>
#include <windowsx.h>
#include <knownfolders.h>

#include "panitent.h"
#include "settings.h"

#define LPSZ_SETTINGSWINDOW L"Win32Ctl_SettingsWindow"

ATOM RegisterSettingsWindowClass()
{
  WNDCLASS wc      = {0};
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)SettingsWndProc;
  wc.hInstance     = GetModuleHandle(NULL);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = LPSZ_SETTINGSWINDOW;

  return RegisterClass(&wc);
}

ATOM settings_window_class;

int ShowSettingsWindow(HWND parent_hwnd)
{
  if (!settings_window_class)
    settings_window_class = RegisterSettingsWindowClass();

  if (settings_window_class) {
    RECT rc   = {0};
    rc.right  = 640;
    rc.bottom = 480;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    CreateWindow(LPSZ_SETTINGSWINDOW,
                 L"Settings",
                 WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                 (GetSystemMetrics(SM_CXSCREEN) - rc.right - rc.left) / 2,
                 (GetSystemMetrics(SM_CYSCREEN) - rc.bottom - rc.top) / 2,
                 rc.right - rc.left,
                 rc.bottom - rc.top,
                 parent_hwnd,
                 NULL,
                 GetModuleHandle(NULL),
                 NULL);
  }

  return 0;
}

HWND hTabControl;
HWND hButtonCancel;
HWND hButtonOk;

HWND hTPMain;
HWND hTPDebug;
HWND hSettingsWindow;

LRESULT CALLBACK SettingsWndProc(HWND hwnd,
                                 UINT msg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
  switch (msg) {
  case WM_NOTIFY:
  {
    LPNMHDR pNm = (LPNMHDR)lParam;

    if (pNm->hwndFrom == hTabControl && pNm->code == TCN_SELCHANGE) {
      UINT tabId = TabCtrl_GetCurSel(hTabControl);

      switch (tabId) {
      case IDT_SETTINGS_PAGE_MAIN:
        ShowWindow(hTPMain, SW_NORMAL);
        ShowWindow(hTPDebug, SW_HIDE);
        break;
      case IDT_SETTINGS_PAGE_DEBUG:
        ShowWindow(hTPMain, SW_HIDE);
        ShowWindow(hTPDebug, SW_NORMAL);
        break;
      }
    }
  } break;
  case WM_CREATE:
    hSettingsWindow = hwnd;
    InitSettingsWindow(hwnd);
    break;
  case WM_GETMINMAXINFO:
  {
    RECT rc   = {0};
    rc.right  = 640;
    rc.bottom = 480;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    LPMINMAXINFO lpMMI      = (LPMINMAXINFO)lParam;
    lpMMI->ptMinTrackSize.x = rc.right - rc.left;
    lpMMI->ptMinTrackSize.y = rc.bottom - rc.top;
  } break;
  case WM_SIZE:
  {
    UINT width  = GET_X_LPARAM(lParam);
    UINT height = GET_Y_LPARAM(lParam);

    SetWindowPos(hTabControl, HWND_TOP, 10, 10, width - 20, height - 60, 0);
    SetWindowPos(hButtonCancel, HWND_TOP, width - 110, height - 40, 100, 30, 0);
    SetWindowPos(hButtonOk, HWND_TOP, width - 220, height - 40, 100, 30, 0);

    RECT vrc;
    GetClientRect(hTabControl, &vrc);
    vrc.left += 10;
    vrc.top += 10;
    vrc.right += 10;
    vrc.bottom += 10;
    TabCtrl_AdjustRect(hTabControl, FALSE, &vrc);

    SetWindowPos(hTPMain,
                 HWND_TOP,
                 vrc.left,
                 vrc.top,
                 vrc.right - vrc.left,
                 vrc.bottom - vrc.top,
                 0);

    SetWindowPos(hTPDebug,
                 HWND_TOP,
                 vrc.left,
                 vrc.top,
                 vrc.right - vrc.left,
                 vrc.bottom - vrc.top,
                 0);
  } break;
  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED) {
      DestroyWindow(hSettingsWindow);
    }
    break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

int InitSettingsWindow(HWND hwnd)
{
  RegisterSettingsTabPageMain();
  RegisterSettingsTabPageDebug();

  hTabControl = CreateWindowEx(0,
                               WC_TABCONTROL,
                               NULL,
                               WS_CHILD | WS_VISIBLE,
                               10,
                               10,
                               620,
                               180,
                               hwnd,
                               NULL,
                               GetModuleHandle(NULL),
                               NULL);
  SetGuiFont(hTabControl);

  TCITEM tab_main  = {0};
  tab_main.mask    = TCIF_TEXT;
  tab_main.pszText = L"Main";
  tab_main.iImage  = -1;
  TabCtrl_InsertItem(hTabControl, IDT_SETTINGS_PAGE_MAIN, &tab_main);

  TCITEM tab_debug  = {0};
  tab_debug.mask    = TCIF_TEXT;
  tab_debug.pszText = L"Debug";
  tab_debug.iImage  = -1;
  TabCtrl_InsertItem(hTabControl, IDT_SETTINGS_PAGE_DEBUG, &tab_debug);

  RECT tab_view_rc   = {0};
  tab_view_rc.left   = 10;
  tab_view_rc.top    = 10;
  tab_view_rc.right  = 640;
  tab_view_rc.bottom = 200;
  TabCtrl_AdjustRect(hTabControl, FALSE, &tab_view_rc);

  hTPMain = CreateWindow(L"SettingsTabPageMain",
                         NULL,
                         WS_CHILD | WS_VISIBLE,
                         tab_view_rc.left,
                         tab_view_rc.top,
                         tab_view_rc.right - tab_view_rc.left,
                         tab_view_rc.bottom - tab_view_rc.top,
                         hwnd,
                         NULL,
                         GetModuleHandle(NULL),
                         NULL);

  hTPDebug = CreateWindow(L"SettingsTabPageDebug",
                          NULL,
                          WS_CHILD,
                          tab_view_rc.left,
                          tab_view_rc.top,
                          tab_view_rc.right - tab_view_rc.left,
                          tab_view_rc.bottom - tab_view_rc.top,
                          hwnd,
                          NULL,
                          GetModuleHandle(NULL),
                          NULL);

  hButtonCancel = CreateWindow(L"BUTTON",
                               L"Cancel",
                               WS_CHILD | WS_VISIBLE,
                               420,
                               440,
                               100,
                               30,
                               hwnd,
                               (HMENU)IDCANCEL,
                               GetModuleHandle(NULL),
                               NULL);
  SetGuiFont(hButtonCancel);

  hButtonOk = CreateWindow(L"BUTTON",
                           L"OK",
                           WS_CHILD | WS_VISIBLE,
                           530,
                           440,
                           100,
                           30,
                           hwnd,
                           (HMENU)IDOK,
                           GetModuleHandle(NULL),
                           NULL);
  SetGuiFont(hButtonOk);

  return 0;
}

int new_control_id()
{
  /* TODO: Reuse freed ids */
  static int i = 1201;
  return i++;
}

void shima(__attribute__((unused)) WPARAM wParam, LPARAM lParam)
{
  DWORD bCheck = Button_GetCheck((HWND)lParam);

  if (bCheck == BST_UNCHECKED) {
    int result = MessageBox(hSettingsWindow,
                            L"Are you sure?",
                            L"Warning",
                            MB_YESNO | MB_ICONEXCLAMATION);
    if (result == IDNO) {
      return;
    }
  }

  Button_SetCheck((HWND)lParam,
                  bCheck == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
}

HWND CreateCheckbox(HWND parent,
                    const wchar_t* label,
                    unsigned int posX,
                    unsigned int posY)
{
  size_t id = new_control_id();
  register_event_handler(id, &shima);

  HWND hWnd = CreateWindow(L"BUTTON",
                           label,
                           WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
                           posX,
                           posY,
                           250,
                           20,
                           parent,
                           (HMENU)id,
                           GetModuleHandle(NULL),
                           NULL);

  SetGuiFont(hWnd);

  return hWnd;
}

struct rlist {
  size_t size;
  size_t capacity;
  HWND* array;
};

struct callback_entry {
  int id;
  void (*callback)(WPARAM wParam, LPARAM lParam);
};

struct event_handlers {
  size_t size;
  size_t capacity;
  struct callback_entry** entries;
} g_handlers;

void register_event_handler(int id,
                            void (*callback)(WPARAM wParam, LPARAM lParam))
{
  size_t* cap = &g_handlers.capacity;

  if (*cap == 0)
    return;

  /*
   * if modulo 16 of capacity is zero, that means there no more space
   * left for the new entry.
   */
  if (g_handlers.size >= *cap) {
    size_t new_cap     = (*cap / 16 + 1) * 16;
    g_handlers.entries = realloc(g_handlers.entries, new_cap);
    *cap               = new_cap;
  }

  struct callback_entry* e = calloc(1, sizeof(struct callback_entry));
  e->id                    = id;
  e->callback              = callback;
  g_handlers.entries[g_handlers.size++] = e;
}

void process_event(WPARAM wParam, LPARAM lParam)
{
  for (size_t i = 0; i < g_handlers.size; i++) {
    if (LOWORD(wParam) == g_handlers.entries[i]->id &&
        g_handlers.entries[i]->callback != NULL) {
      g_handlers.entries[i]->callback(wParam, lParam);
      break;
    }
  }
}

void rlist_create(struct rlist* list, size_t capacity)
{
  list->array    = calloc(capacity, sizeof(HWND));
  list->capacity = capacity;
  list->size     = 0;
}

void rlist_add(struct rlist* list, HWND hWnd)
{
  size_t* cap = &list->capacity;

  if (list->size >= *cap) {
    size_t new_cap = (*cap / 16 + 1) * 16;
    list->array    = realloc(list->array, new_cap);
    *cap           = new_cap;
  }

  SetWindowPos(hWnd, 0, 10, 10 + 20 * list->size++, 0, 0, SWP_NOSIZE);
}

void alloc_handlers_vault()
{
  g_handlers.entries  = calloc(2, sizeof(struct callback_entry));
  g_handlers.capacity = 2;
  g_handlers.size     = 0;
}

void rlist_destroy(struct rlist* list)
{
  free(list->array);
  list->array    = NULL;
  list->capacity = list->size = 0;
}

struct rlist r = {0};

LRESULT CALLBACK SettingsTabPageMainProc(HWND hwnd,
                                         UINT msg,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
  switch (msg) {
  case WM_CREATE:
  {
    alloc_handlers_vault();
    rlist_create(&r, 2);
    rlist_add(&r, CreateCheckbox(hwnd, L"Show border", 0, 0));
    rlist_add(&r, CreateCheckbox(hwnd, L"Show tits", 0, 0));
    rlist_add(&r, CreateCheckbox(hwnd, L"Show booty", 0, 0));
    rlist_add(&r, CreateCheckbox(hwnd, L"Rubbish compiler", 0, 0));
    rlist_add(
        &r,
        CreateCheckbox(hwnd, L"Use ancient Schizoes numeral system", 0, 0));
    rlist_add(&r, CreateCheckbox(hwnd, L"Another one", 0, 0));
    rlist_add(&r, CreateCheckbox(hwnd, L"And another one", 0, 0));
  } break;
  case WM_DESTROY:
    rlist_destroy(&r);
    break;
  case WM_COMMAND:
    process_event(wParam, lParam);
    break;

  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

void RegisterSettingsTabPageMain()
{
  WNDCLASS wc      = {0};
  wc.lpfnWndProc   = (WNDPROC)SettingsTabPageMainProc;
  wc.hInstance     = GetModuleHandle(NULL);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"SettingsTabPageMain";

  RegisterClass(&wc);
}

LRESULT CALLBACK SettingsTabPageDebugProc(HWND hwnd,
                                          UINT msg,
                                          WPARAM wParam,
                                          LPARAM lParam)
{
  switch (msg) {
  case WM_CREATE:
  {
    HWND hCheckBox = CreateWindow(L"BUTTON",
                                  L"Log actions",
                                  WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                  10,
                                  10,
                                  100,
                                  20,
                                  hwnd,
                                  NULL,
                                  GetModuleHandle(NULL),
                                  NULL);
    SetGuiFont(hCheckBox);
  } break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

void RegisterSettingsTabPageDebug()
{
  WNDCLASS wc      = {0};
  wc.lpfnWndProc   = (WNDPROC)SettingsTabPageDebugProc;
  wc.hInstance     = GetModuleHandle(NULL);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"SettingsTabPageDebug";

  RegisterClass(&wc);
}
