#include "precomp.h"

#include "settings_wnd.h"

#include "panitentapp.h"
#include "panitentwindow.h"
#include "resource.h"
#include "settings.h"
#include "win32/util.h"

static const WCHAR szClassName[] = L"Win32Ctl_SettingsWindow";

#define IDC_SETTINGS_REMEMBER_WINDOW_POS 2101
#define IDC_SETTINGS_POS_X               2102
#define IDC_SETTINGS_POS_Y               2103
#define IDC_SETTINGS_WIDTH               2104
#define IDC_SETTINGS_HEIGHT              2105
#define IDC_SETTINGS_LEGACY_FILEDLG      2106
#define IDC_SETTINGS_ENABLE_PEN          2107
#define IDC_SETTINGS_STANDARD_FRAME      2108
#define IDC_SETTINGS_APPLY               2109
#define IDC_SETTINGS_COMPACT_MENU        2110

static const int kSettingsPadding = 12;
static const int kSettingsGroupGap = 10;
static const int kSettingsLabelWidth = 54;
static const int kSettingsEditWidth = 78;
static const int kSettingsEditHeight = 22;
static const int kSettingsCheckHeight = 20;
static const int kSettingsButtonWidth = 88;
static const int kSettingsButtonHeight = 26;
static const int kSettingsGroupWindowHeight = 116;
static const int kSettingsGroupBehaviorHeight = 140;

static HWND s_hSettingsWindow = NULL;

static void SettingsWindow_Init(SettingsWindow* pSettingsWindow);
static void SettingsWindow_PreRegister(LPWNDCLASSEX lpwcex);
static void SettingsWindow_PreCreate(LPCREATESTRUCT lpcs);
static BOOL SettingsWindow_OnCreate(SettingsWindow* pSettingsWindow, LPCREATESTRUCT lpcs);
static BOOL SettingsWindow_OnEraseBkgnd(SettingsWindow* pSettingsWindow, HDC hdc);
static void SettingsWindow_OnPaint(SettingsWindow* pSettingsWindow);
static void SettingsWindow_OnDestroy(SettingsWindow* pSettingsWindow);
static void SettingsWindow_OnSize(SettingsWindow* pSettingsWindow, UINT state, int width, int height);
static void SettingsWindow_OnCommand(SettingsWindow* pSettingsWindow, WPARAM wParam, LPARAM lParam);
static void SettingsWindow_OnGetMinMaxInfo(SettingsWindow* pSettingsWindow, LPARAM lParam);
static LRESULT SettingsWindow_UserProc(SettingsWindow* pSettingsWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static HWND SettingsWindow_CreateLabel(HWND hWndParent, int id, PCWSTR pszText);
static HWND SettingsWindow_CreateEdit(HWND hWndParent, int id);
static HWND SettingsWindow_CreateCheckbox(HWND hWndParent, int id, PCWSTR pszText);
static HWND SettingsWindow_CreateGroup(HWND hWndParent, PCWSTR pszText);
static HWND SettingsWindow_CreateButton(HWND hWndParent, int id, PCWSTR pszText, DWORD dwStyle);
static void SettingsWindow_SetIntText(HWND hWnd, int value);
static int SettingsWindow_GetIntText(HWND hWnd, int nFallback);
static void SettingsWindow_LoadFromSettings(SettingsWindow* pSettingsWindow);
static void SettingsWindow_UpdateWindowFieldsEnabled(SettingsWindow* pSettingsWindow);
static void SettingsWindow_UpdateFrameFieldsEnabled(SettingsWindow* pSettingsWindow);
static void SettingsWindow_LayoutControls(SettingsWindow* pSettingsWindow, int width, int height);
static BOOL SettingsWindow_Apply(SettingsWindow* pSettingsWindow);
static void SettingsWindow_MarkDirty(SettingsWindow* pSettingsWindow, BOOL fDirty);

SettingsWindow* SettingsWindow_Create(void)
{
    SettingsWindow* pSettingsWindow = (SettingsWindow*)malloc(sizeof(SettingsWindow));
    if (pSettingsWindow)
    {
        SettingsWindow_Init(pSettingsWindow);
    }

    return pSettingsWindow;
}

HWND SettingsWindow_Show(void)
{
    if (s_hSettingsWindow && IsWindow(s_hSettingsWindow))
    {
        ShowWindow(s_hSettingsWindow, SW_SHOWNORMAL);
        SetForegroundWindow(s_hSettingsWindow);
        return s_hSettingsWindow;
    }

    SettingsWindow* pSettingsWindow = SettingsWindow_Create();
    if (!pSettingsWindow)
    {
        return NULL;
    }

    s_hSettingsWindow = Window_CreateWindow((Window*)pSettingsWindow, NULL);
    if (s_hSettingsWindow)
    {
        ShowWindow(s_hSettingsWindow, SW_SHOW);
        UpdateWindow(s_hSettingsWindow);
    }

    return s_hSettingsWindow;
}

static void SettingsWindow_Init(SettingsWindow* pSettingsWindow)
{
    memset(pSettingsWindow, 0, sizeof(SettingsWindow));

    Window_Init(&pSettingsWindow->base);
    pSettingsWindow->base.szClassName = szClassName;
    pSettingsWindow->base.OnCreate = (FnWindowOnCreate)SettingsWindow_OnCreate;
    pSettingsWindow->base.OnDestroy = (FnWindowOnDestroy)SettingsWindow_OnDestroy;
    pSettingsWindow->base.OnPaint = (FnWindowOnPaint)SettingsWindow_OnPaint;
    pSettingsWindow->base.OnSize = (FnWindowOnSize)SettingsWindow_OnSize;

    _WindowInitHelper_SetPreRegisterRoutine((Window*)pSettingsWindow, (FnWindowPreRegister)SettingsWindow_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window*)pSettingsWindow, (FnWindowPreCreate)SettingsWindow_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window*)pSettingsWindow, (FnWindowUserProc)SettingsWindow_UserProc);
}

static void SettingsWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

static void SettingsWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    RECT rcWindow = { 0, 0, 560, 360 };
    AdjustWindowRect(&rcWindow, WS_OVERLAPPEDWINDOW, FALSE);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = RECTWIDTH(&rcWindow);
    int windowHeight = RECTHEIGHT(&rcWindow);

    lpcs->dwExStyle = WS_EX_CONTROLPARENT;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Options";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    lpcs->x = max(0, (screenWidth - windowWidth) / 2);
    lpcs->y = max(0, (screenHeight - windowHeight) / 2);
    lpcs->cx = windowWidth;
    lpcs->cy = windowHeight;
    lpcs->hInstance = GetModuleHandle(NULL);
}

static HWND SettingsWindow_CreateGroup(HWND hWndParent, PCWSTR pszText)
{
    HWND hWnd = CreateWindowExW(
        0,
        WC_BUTTONW,
        pszText,
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        hWndParent,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    Win32_ApplyUIFont(hWnd);
    return hWnd;
}

static HWND SettingsWindow_CreateCheckbox(HWND hWndParent, int id, PCWSTR pszText)
{
    HWND hWnd = CreateWindowExW(
        0,
        WC_BUTTONW,
        pszText,
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        0, 0, 0, 0,
        hWndParent,
        (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL),
        NULL);
    Win32_ApplyUIFont(hWnd);
    return hWnd;
}

static HWND SettingsWindow_CreateLabel(HWND hWndParent, int id, PCWSTR pszText)
{
    HWND hWnd = CreateWindowExW(
        0,
        WC_STATICW,
        pszText,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        hWndParent,
        (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL),
        NULL);
    Win32_ApplyUIFont(hWnd);
    return hWnd;
}

static HWND SettingsWindow_CreateEdit(HWND hWndParent, int id)
{
    HWND hWnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_EDITW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
        0, 0, 0, 0,
        hWndParent,
        (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL),
        NULL);
    Win32_ApplyUIFont(hWnd);
    return hWnd;
}

static HWND SettingsWindow_CreateButton(HWND hWndParent, int id, PCWSTR pszText, DWORD dwStyle)
{
    HWND hWnd = CreateWindowExW(
        0,
        WC_BUTTONW,
        pszText,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | dwStyle,
        0, 0, 0, 0,
        hWndParent,
        (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL),
        NULL);
    Win32_ApplyUIFont(hWnd);
    return hWnd;
}

static void SettingsWindow_SetIntText(HWND hWnd, int value)
{
    WCHAR szValue[32] = L"";
    _itow_s(value, szValue, ARRAYSIZE(szValue), 10);
    SetWindowTextW(hWnd, szValue);
}

static int SettingsWindow_GetIntText(HWND hWnd, int nFallback)
{
    WCHAR szValue[32] = L"";
    if (!hWnd || GetWindowTextW(hWnd, szValue, ARRAYSIZE(szValue)) <= 0)
    {
        return nFallback;
    }

    if (szValue[0] == L'\0')
    {
        return nFallback;
    }

    return _wtoi(szValue);
}

static void SettingsWindow_LoadFromSettings(SettingsWindow* pSettingsWindow)
{
    PNTSETTINGS* pSettings = PanitentApp_GetSettings(PanitentApp_Instance());
    if (!pSettingsWindow || !pSettings)
    {
        return;
    }

    Button_SetCheck(
        pSettingsWindow->hCheckRememberWindowPos,
        pSettings->bRememberWindowPos ? BST_CHECKED : BST_UNCHECKED);
    SettingsWindow_SetIntText(pSettingsWindow->hEditPosX, pSettings->x);
    SettingsWindow_SetIntText(pSettingsWindow->hEditPosY, pSettings->y);
    SettingsWindow_SetIntText(pSettingsWindow->hEditWidth, pSettings->width);
    SettingsWindow_SetIntText(pSettingsWindow->hEditHeight, pSettings->height);
    Button_SetCheck(
        pSettingsWindow->hCheckLegacyFileDialogs,
        pSettings->bLegacyFileDialogs ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(
        pSettingsWindow->hCheckEnablePenTablet,
        pSettings->bEnablePenTablet ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(
        pSettingsWindow->hCheckStandardWindowFrame,
        pSettings->bUseStandardWindowFrame ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(
        pSettingsWindow->hCheckCompactMenuBar,
        pSettings->bCompactMenuBar ? BST_CHECKED : BST_UNCHECKED);

    SettingsWindow_UpdateWindowFieldsEnabled(pSettingsWindow);
    SettingsWindow_UpdateFrameFieldsEnabled(pSettingsWindow);
    SettingsWindow_MarkDirty(pSettingsWindow, FALSE);
}

static void SettingsWindow_UpdateWindowFieldsEnabled(SettingsWindow* pSettingsWindow)
{
    BOOL fEnabled = Button_GetCheck(pSettingsWindow->hCheckRememberWindowPos) == BST_CHECKED;
    EnableWindow(pSettingsWindow->hLabelPosX, fEnabled);
    EnableWindow(pSettingsWindow->hEditPosX, fEnabled);
    EnableWindow(pSettingsWindow->hLabelPosY, fEnabled);
    EnableWindow(pSettingsWindow->hEditPosY, fEnabled);
    EnableWindow(pSettingsWindow->hLabelWidth, fEnabled);
    EnableWindow(pSettingsWindow->hEditWidth, fEnabled);
    EnableWindow(pSettingsWindow->hLabelHeight, fEnabled);
    EnableWindow(pSettingsWindow->hEditHeight, fEnabled);
}

static void SettingsWindow_UpdateFrameFieldsEnabled(SettingsWindow* pSettingsWindow)
{
    BOOL fEnableCompactMenu =
        Button_GetCheck(pSettingsWindow->hCheckStandardWindowFrame) != BST_CHECKED;
    EnableWindow(pSettingsWindow->hCheckCompactMenuBar, fEnableCompactMenu);
}

static void SettingsWindow_LayoutControls(SettingsWindow* pSettingsWindow, int width, int height)
{
    if (!pSettingsWindow)
    {
        return;
    }

    int clientWidth = max(320, width);
    int clientHeight = max(240, height);
    int contentWidth = clientWidth - kSettingsPadding * 2;
    int labelEditRowY = kSettingsPadding + 44;
    int buttonY = clientHeight - kSettingsPadding - kSettingsButtonHeight;

    MoveWindow(
        pSettingsWindow->hGroupWindow,
        kSettingsPadding,
        kSettingsPadding,
        contentWidth,
        kSettingsGroupWindowHeight,
        TRUE);

    MoveWindow(
        pSettingsWindow->hCheckRememberWindowPos,
        kSettingsPadding + 12,
        kSettingsPadding + 22,
        contentWidth - 24,
        kSettingsCheckHeight,
        TRUE);

    int rowStart = kSettingsPadding + 12;
    int pairGap = 16;
    int x = rowStart;
    MoveWindow(pSettingsWindow->hLabelPosX, x, labelEditRowY + 4, kSettingsLabelWidth, 18, TRUE);
    x += kSettingsLabelWidth;
    MoveWindow(pSettingsWindow->hEditPosX, x, labelEditRowY, kSettingsEditWidth, kSettingsEditHeight, TRUE);
    x += kSettingsEditWidth + pairGap;
    MoveWindow(pSettingsWindow->hLabelPosY, x, labelEditRowY + 4, kSettingsLabelWidth, 18, TRUE);
    x += kSettingsLabelWidth;
    MoveWindow(pSettingsWindow->hEditPosY, x, labelEditRowY, kSettingsEditWidth, kSettingsEditHeight, TRUE);
    x += kSettingsEditWidth + pairGap;
    MoveWindow(pSettingsWindow->hLabelWidth, x, labelEditRowY + 4, kSettingsLabelWidth, 18, TRUE);
    x += kSettingsLabelWidth;
    MoveWindow(pSettingsWindow->hEditWidth, x, labelEditRowY, kSettingsEditWidth, kSettingsEditHeight, TRUE);
    x += kSettingsEditWidth + pairGap;
    MoveWindow(pSettingsWindow->hLabelHeight, x, labelEditRowY + 4, kSettingsLabelWidth, 18, TRUE);
    x += kSettingsLabelWidth;
    MoveWindow(pSettingsWindow->hEditHeight, x, labelEditRowY, kSettingsEditWidth, kSettingsEditHeight, TRUE);

    int behaviorTop = kSettingsPadding + kSettingsGroupWindowHeight + kSettingsGroupGap;
    MoveWindow(
        pSettingsWindow->hGroupBehavior,
        kSettingsPadding,
        behaviorTop,
        contentWidth,
        kSettingsGroupBehaviorHeight,
        TRUE);

    MoveWindow(
        pSettingsWindow->hCheckLegacyFileDialogs,
        kSettingsPadding + 12,
        behaviorTop + 22,
        contentWidth - 24,
        kSettingsCheckHeight,
        TRUE);
    MoveWindow(
        pSettingsWindow->hCheckEnablePenTablet,
        kSettingsPadding + 12,
        behaviorTop + 48,
        contentWidth - 24,
        kSettingsCheckHeight,
        TRUE);
    MoveWindow(
        pSettingsWindow->hCheckStandardWindowFrame,
        kSettingsPadding + 12,
        behaviorTop + 74,
        contentWidth - 24,
        kSettingsCheckHeight,
        TRUE);
    MoveWindow(
        pSettingsWindow->hCheckCompactMenuBar,
        kSettingsPadding + 12,
        behaviorTop + 100,
        contentWidth - 24,
        kSettingsCheckHeight,
        TRUE);

    int buttonX = clientWidth - kSettingsPadding - kSettingsButtonWidth;
    MoveWindow(pSettingsWindow->hButtonCancel, buttonX, buttonY, kSettingsButtonWidth, kSettingsButtonHeight, TRUE);
    buttonX -= kSettingsButtonWidth + 8;
    MoveWindow(pSettingsWindow->hButtonApply, buttonX, buttonY, kSettingsButtonWidth, kSettingsButtonHeight, TRUE);
    buttonX -= kSettingsButtonWidth + 8;
    MoveWindow(pSettingsWindow->hButtonOk, buttonX, buttonY, kSettingsButtonWidth, kSettingsButtonHeight, TRUE);
}

static void SettingsWindow_MarkDirty(SettingsWindow* pSettingsWindow, BOOL fDirty)
{
    if (!pSettingsWindow)
    {
        return;
    }

    pSettingsWindow->fDirty = fDirty ? TRUE : FALSE;
    EnableWindow(pSettingsWindow->hButtonApply, pSettingsWindow->fDirty);
}

static BOOL SettingsWindow_Apply(SettingsWindow* pSettingsWindow)
{
    PNTSETTINGS* pSettings = PanitentApp_GetSettings(PanitentApp_Instance());
    if (!pSettingsWindow || !pSettings)
    {
        return FALSE;
    }

    pSettings->bRememberWindowPos = Button_GetCheck(pSettingsWindow->hCheckRememberWindowPos) == BST_CHECKED;
    pSettings->x = SettingsWindow_GetIntText(pSettingsWindow->hEditPosX, pSettings->x);
    pSettings->y = SettingsWindow_GetIntText(pSettingsWindow->hEditPosY, pSettings->y);
    pSettings->width = max(64, SettingsWindow_GetIntText(pSettingsWindow->hEditWidth, pSettings->width));
    pSettings->height = max(64, SettingsWindow_GetIntText(pSettingsWindow->hEditHeight, pSettings->height));
    pSettings->bLegacyFileDialogs = Button_GetCheck(pSettingsWindow->hCheckLegacyFileDialogs) == BST_CHECKED;
    pSettings->bEnablePenTablet = Button_GetCheck(pSettingsWindow->hCheckEnablePenTablet) == BST_CHECKED;
    pSettings->bUseStandardWindowFrame = Button_GetCheck(pSettingsWindow->hCheckStandardWindowFrame) == BST_CHECKED;
    pSettings->bCompactMenuBar = Button_GetCheck(pSettingsWindow->hCheckCompactMenuBar) == BST_CHECKED;

    if (!Panitent_WriteSettings(pSettings))
    {
        MessageBoxW(
            Window_GetHWND((Window*)pSettingsWindow),
            L"Failed to save settings.",
            L"Options",
            MB_OK | MB_ICONERROR);
        return FALSE;
    }

    PanitentWindow_SetUseStandardFrame(
        PanitentApp_GetWindow(PanitentApp_Instance()),
        pSettings->bUseStandardWindowFrame);
    PanitentWindow_SetCompactMenuBar(
        PanitentApp_GetWindow(PanitentApp_Instance()),
        pSettings->bCompactMenuBar);

    SettingsWindow_MarkDirty(pSettingsWindow, FALSE);
    return TRUE;
}

static BOOL SettingsWindow_OnCreate(SettingsWindow* pSettingsWindow, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(lpcs);

    HWND hWnd = Window_GetHWND((Window*)pSettingsWindow);

    pSettingsWindow->hGroupWindow = SettingsWindow_CreateGroup(hWnd, L"Window");
    pSettingsWindow->hCheckRememberWindowPos = SettingsWindow_CreateCheckbox(hWnd, IDC_SETTINGS_REMEMBER_WINDOW_POS, L"Remember initial window position and size");
    pSettingsWindow->hLabelPosX = SettingsWindow_CreateLabel(hWnd, IDC_STATIC, L"X");
    pSettingsWindow->hEditPosX = SettingsWindow_CreateEdit(hWnd, IDC_SETTINGS_POS_X);
    pSettingsWindow->hLabelPosY = SettingsWindow_CreateLabel(hWnd, IDC_STATIC, L"Y");
    pSettingsWindow->hEditPosY = SettingsWindow_CreateEdit(hWnd, IDC_SETTINGS_POS_Y);
    pSettingsWindow->hLabelWidth = SettingsWindow_CreateLabel(hWnd, IDC_STATIC, L"Width");
    pSettingsWindow->hEditWidth = SettingsWindow_CreateEdit(hWnd, IDC_SETTINGS_WIDTH);
    pSettingsWindow->hLabelHeight = SettingsWindow_CreateLabel(hWnd, IDC_STATIC, L"Height");
    pSettingsWindow->hEditHeight = SettingsWindow_CreateEdit(hWnd, IDC_SETTINGS_HEIGHT);

    pSettingsWindow->hGroupBehavior = SettingsWindow_CreateGroup(hWnd, L"Behavior");
    pSettingsWindow->hCheckLegacyFileDialogs = SettingsWindow_CreateCheckbox(hWnd, IDC_SETTINGS_LEGACY_FILEDLG, L"Use legacy file dialogs");
    pSettingsWindow->hCheckEnablePenTablet = SettingsWindow_CreateCheckbox(hWnd, IDC_SETTINGS_ENABLE_PEN, L"Enable pen tablet");
    pSettingsWindow->hCheckStandardWindowFrame = SettingsWindow_CreateCheckbox(hWnd, IDC_SETTINGS_STANDARD_FRAME, L"Use standard Windows frame");
    pSettingsWindow->hCheckCompactMenuBar = SettingsWindow_CreateCheckbox(hWnd, IDC_SETTINGS_COMPACT_MENU, L"Compact menu bar");

    pSettingsWindow->hButtonOk = SettingsWindow_CreateButton(hWnd, IDOK, L"OK", BS_DEFPUSHBUTTON);
    pSettingsWindow->hButtonCancel = SettingsWindow_CreateButton(hWnd, IDCANCEL, L"Cancel", BS_PUSHBUTTON);
    pSettingsWindow->hButtonApply = SettingsWindow_CreateButton(hWnd, IDC_SETTINGS_APPLY, L"Apply", BS_PUSHBUTTON);

    RECT rcClient = { 0 };
    GetClientRect(hWnd, &rcClient);
    SettingsWindow_LayoutControls(pSettingsWindow, RECTWIDTH(&rcClient), RECTHEIGHT(&rcClient));
    SettingsWindow_LoadFromSettings(pSettingsWindow);
    return TRUE;
}

static BOOL SettingsWindow_OnEraseBkgnd(SettingsWindow* pSettingsWindow, HDC hdc)
{
    HWND hWnd = Window_GetHWND((Window*)pSettingsWindow);
    RECT rcClient = { 0 };
    GetClientRect(hWnd, &rcClient);
    FillRect(hdc, &rcClient, GetSysColorBrush(COLOR_BTNFACE));
    return TRUE;
}

static void SettingsWindow_OnPaint(SettingsWindow* pSettingsWindow)
{
    HWND hWnd = Window_GetHWND((Window*)pSettingsWindow);
    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hWnd, &ps);
    if (hdc)
    {
        FillRect(hdc, &ps.rcPaint, GetSysColorBrush(COLOR_BTNFACE));
    }
    EndPaint(hWnd, &ps);
}

static void SettingsWindow_OnDestroy(SettingsWindow* pSettingsWindow)
{
    UNREFERENCED_PARAMETER(pSettingsWindow);
    s_hSettingsWindow = NULL;
}

static void SettingsWindow_OnSize(SettingsWindow* pSettingsWindow, UINT state, int width, int height)
{
    UNREFERENCED_PARAMETER(state);
    SettingsWindow_LayoutControls(pSettingsWindow, width, height);

    HWND hWnd = Window_GetHWND((Window*)pSettingsWindow);
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

static void SettingsWindow_OnCommand(SettingsWindow* pSettingsWindow, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (LOWORD(wParam))
    {
    case IDC_SETTINGS_REMEMBER_WINDOW_POS:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            SettingsWindow_UpdateWindowFieldsEnabled(pSettingsWindow);
            SettingsWindow_MarkDirty(pSettingsWindow, TRUE);
        }
        break;

    case IDC_SETTINGS_LEGACY_FILEDLG:
    case IDC_SETTINGS_ENABLE_PEN:
    case IDC_SETTINGS_STANDARD_FRAME:
    case IDC_SETTINGS_COMPACT_MENU:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            if (LOWORD(wParam) == IDC_SETTINGS_STANDARD_FRAME)
            {
                SettingsWindow_UpdateFrameFieldsEnabled(pSettingsWindow);
            }
            SettingsWindow_MarkDirty(pSettingsWindow, TRUE);
        }
        break;

    case IDC_SETTINGS_POS_X:
    case IDC_SETTINGS_POS_Y:
    case IDC_SETTINGS_WIDTH:
    case IDC_SETTINGS_HEIGHT:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            SettingsWindow_MarkDirty(pSettingsWindow, TRUE);
        }
        break;

    case IDC_SETTINGS_APPLY:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            SettingsWindow_Apply(pSettingsWindow);
        }
        break;

    case IDOK:
        if (HIWORD(wParam) == BN_CLICKED && SettingsWindow_Apply(pSettingsWindow))
        {
            Window_Destroy((Window*)pSettingsWindow);
        }
        break;

    case IDCANCEL:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            Window_Destroy((Window*)pSettingsWindow);
        }
        break;
    }
}

static void SettingsWindow_OnGetMinMaxInfo(SettingsWindow* pSettingsWindow, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(pSettingsWindow);

    RECT rcWindow = { 0, 0, 560, 360 };
    AdjustWindowRect(&rcWindow, WS_OVERLAPPEDWINDOW, FALSE);

    LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
    lpmmi->ptMinTrackSize.x = RECTWIDTH(&rcWindow);
    lpmmi->ptMinTrackSize.y = RECTHEIGHT(&rcWindow);
}

static LRESULT SettingsWindow_UserProc(SettingsWindow* pSettingsWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        return SettingsWindow_OnEraseBkgnd(pSettingsWindow, (HDC)wParam);

    case WM_COMMAND:
        SettingsWindow_OnCommand(pSettingsWindow, wParam, lParam);
        return 0;

    case WM_GETMINMAXINFO:
        SettingsWindow_OnGetMinMaxInfo(pSettingsWindow, lParam);
        return 0;
    }

    return Window_UserProcDefault((Window*)pSettingsWindow, hWnd, message, wParam, lParam);
}
