#include "precomp.h"

#include "win32/window.h"

#include "dockhost.h"
#include "panitentwindow.h"

#include "history.h"
#include "resource.h"
#include "panitent.h"
#include "log.h"
#include "aboutbox.h"
#include "log_window.h"
#include "new.h"

static const WCHAR szClassName[] = L"__PanitentWindow";

/* Private forward declarations */
PanitentWindow* PanitentWindow_Create(struct Application*);
void PanitentWindow_Init(PanitentWindow*, struct Application*);

void PanitentWindow_PreRegister(LPWNDCLASSEX);
void PanitentWindow_PreCreate(LPCREATESTRUCT);

BOOL PanitentWindow_OnCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs);
void PanitentWindow_PostCreate(PanitentWindow* pPanitentWindow);
void PanitentWindow_OnPaint(PanitentWindow* pPanitentWindow);
void PanitentWindow_OnLButtonUp(PanitentWindow* pPanitentWindow, int x, int y);
void PanitentWindow_OnRButtonUp(PanitentWindow* pPanitentWindow, int x, int y);
void PanitentWindow_OnContextMenu(PanitentWindow* pPanitentWindow, int x, int y);
void PanitentWindow_OnDestroy(PanitentWindow* pPanitentWindow);
void PanitentWindow_OnSize(PanitentWindow* pPanitentWindow, UINT state, int cx, int cy);
LRESULT CALLBACK PanitentWindow_UserProc(PanitentWindow* pPanitentWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

PanitentWindow* PanitentWindow_Create(struct Application* app)
{
    PanitentWindow* window = calloc(1, sizeof(PanitentWindow));

    if (window)
    {
        PanitentWindow_Init(window, app);
    }

    return window;
}

void PanitentWindow_Init(PanitentWindow* window, struct Application* app)
{
    Window_Init(&window->base, app);

    window->base.szClassName = szClassName;

    window->base.OnCreate = (FnWindowOnCreate)PanitentWindow_OnCreate;
    window->base.OnDestroy = (FnWindowOnDestroy)PanitentWindow_OnDestroy;
    window->base.OnPaint = (FnWindowOnPaint)PanitentWindow_OnPaint;
    window->base.OnSize = (FnWindowOnSize)PanitentWindow_OnSize;
    window->base.PreRegister = (FnWindowPreRegister)PanitentWindow_PreRegister;
    window->base.PreCreate = (FnWindowPreCreate)PanitentWindow_PreCreate;
    window->base.UserProc = (FnWindowUserProc)PanitentWindow_UserProc;
    window->base.PostCreate = (FnWindowPostCreate)PanitentWindow_PostCreate;
}

void PanitentWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void PanitentWindow_OnPaint(PanitentWindow* window)
{
    HWND hwnd = window->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void PanitentWindow_OnLButtonUp(PanitentWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void PanitentWindow_OnRButtonUp(PanitentWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void PanitentWindow_OnContextMenu(PanitentWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void PanitentWindow_OnDestroy(PanitentWindow* window)
{
    UNREFERENCED_PARAMETER(window);
}

void PanitentWindow_OnSize(PanitentWindow* pPanitentWindow, UINT state, int cx, int cy)
{
    if (pPanitentWindow->m_pDockHostWindow)
    {
        SetWindowPos(Window_GetHWND((Window *)pPanitentWindow->m_pDockHostWindow), NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

/* Menu ID values */
enum {
    IDM_FILE_NEW = 1001,
    IDM_FILE_OPEN,
    IDM_FILE_SAVE,
    IDM_FILE_CLIPBOARD_EXPORT,
    IDM_FILE_CLOSE,

    IDM_EDIT_UNDO,
    IDM_EDIT_REDO,
    IDM_EDIT_CLRCANVAS,

    IDM_WINDOW_TOOLS,

    IDM_OPTIONS_SETTINGS,

    IDM_HELP_TOPICS,
    IDM_HELP_LOG,
    IDM_HELP_ABOUT
};

HMENU PanitentWindow_CreateMenu()
{
    HMENU hMenu;
    HMENU hSubMenu;

    hMenu = CreateMenu();

    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_NEW, L"&New\tCtrl+N");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_OPEN, L"&Open\tCtrl+O");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_SAVE, L"&Save\tCtrl+S");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_CLIPBOARD_EXPORT, L"Export image to clipboard");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_CLOSE, L"&Close");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&File");

    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_UNDO, L"&Undo");
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_REDO, L"&Redo");
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_CLRCANVAS, L"&Clear canvas");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Edit");

    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_WINDOW_TOOLS, L"&Tools");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Window");
    CheckMenuItem(hSubMenu, IDM_WINDOW_TOOLS, MF_BYCOMMAND | MF_CHECKED);

    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_OPTIONS_SETTINGS, L"&Settings");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Options");

    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_HELP_TOPICS, L"Help &Topics");
    AppendMenu(hSubMenu, MF_STRING, IDM_HELP_LOG, L"&Log");
    AppendMenu(hSubMenu, MF_STRING, IDM_HELP_ABOUT, L"&About");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Help");

    return hMenu;
}

LRESULT PanitentWindow_OnCommand(PanitentWindow* pPanitentWindow, WPARAM wParam, LPARAM lParam)
{
    /* Handle menu commands */
    switch (LOWORD(wParam))
    {
    case IDM_FILE_NEW:
        LogMessage(LOGENTRY_TYPE_INFO, L"MAIN", L"New File menu issued");
        NewFileDialog(Window_GetHWND((Window *)pPanitentWindow));
        break;

    case IDM_FILE_OPEN:
        Panitent_Open();
        break;

    case IDM_FILE_SAVE:
        Panitent_CmdSaveFile((struct PanitentApplication *)pPanitentWindow->base.app);
        break;

    case IDM_FILE_CLIPBOARD_EXPORT:
        Panitent_ClipboardExport();
        break;

    case IDM_FILE_CLOSE:
        PostQuitMessage(0);
        break;

    case IDM_EDIT_UNDO:
        History_Undo(Panitent_GetActiveDocument());
        break;

    case IDM_EDIT_REDO:
        History_Redo(Panitent_GetActiveDocument());
        break;

    case IDM_EDIT_CLRCANVAS:
        Panitent_CmdClearCanvas((struct PanitentApplication *)pPanitentWindow->base.app);
        break;

    case IDM_OPTIONS_SETTINGS:
        ShowSettingsWindow(Window_GetHWND((Window *)pPanitentWindow));
        break;

    case IDM_HELP_LOG:
        LogWindow_Create(Window_GetHWND((Window *)pPanitentWindow));
        break;

    case IDM_HELP_ABOUT:
        AboutBox_Run(Window_GetHWND((Window *)pPanitentWindow));
        break;
    }

    return 0;
}

LRESULT CALLBACK PanitentWindow_UserProc(PanitentWindow* pPanitentWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_RBUTTONUP:
        PanitentWindow_OnRButtonUp(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONUP:
        PanitentWindow_OnLButtonUp(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        PanitentWindow_OnContextMenu(pPanitentWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        /* pass to defwndproc, no return */
        break;

    case WM_COMMAND:
        return PanitentWindow_OnCommand(pPanitentWindow, wParam, lParam);
        break;
    }

    return Window_UserProcDefault(pPanitentWindow, hWnd, message, wParam, lParam);
}

void PanitentWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Panit.ent";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = CW_USEDEFAULT;
    lpcs->cy = CW_USEDEFAULT;
}

BOOL PanitentWindow_OnCreate(PanitentWindow* pPanitentWindow, LPCREATESTRUCT lpcs)
{
    return TRUE;
}

void PanitentWindow_PostCreate(PanitentWindow* pPanitentWindow)
{
    /* Set menu */
    HWND hWnd = Window_GetHWND((Window *)pPanitentWindow);
    SetMenu(hWnd, PanitentWindow_CreateMenu());

    /* Allocate dockhost window data structure */
    DockHostWindow* pDockHostWindow = DockHostWindow_Create(pPanitentWindow->base.app);

    HWND hWndDockHost = Window_CreateWindow((Window *)pDockHostWindow, hWnd);
    pPanitentWindow->m_pDockHostWindow = pDockHostWindow;

    /* Get dockhost client rect*/
    RECT rcDockHost = {0};
    GetClientRect(hWndDockHost, &rcDockHost);

    /* Create and assign root dock node */
    TreeNode* pRoot = (TreeNode*)calloc(1, sizeof(TreeNode));
    DockData* pDockData = (DockData*)calloc(1, sizeof(DockData));
    pRoot->data = (void*)pDockData;
    ((DockData*)pRoot->data)->rc = rcDockHost;

    DockHostWindow_SetRoot(pDockHostWindow, pRoot);

    Panitent_DockHostInit(pDockHostWindow, pRoot);

    RECT rcClient = { 0 };
    Window_GetClientRect((Window *)pPanitentWindow, &rcClient);
    if (pPanitentWindow->m_pDockHostWindow)
    {
        SetWindowPos(Window_GetHWND((Window*)pPanitentWindow->m_pDockHostWindow), NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
    }
}
