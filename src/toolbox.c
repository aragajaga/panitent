#include "precomp.h"

#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

#include "win32/window.h"
#include "viewport.h"
#include "tool.h"
#include "tools/common.h"

#include "toolbox.h"

#include "resource.h"
#include "canvas.h"
#include "document.h"
#include "log.h"
#include "color_context.h"
#include "primitives_context.h"
#include "util.h"
#include "brush.h"
#include "panitent.h"
#include "history.h"

const WCHAR szClassName[] = L"__ToolboxWindow";

#define IDM_TOOLBOXSETTINGS 101

LRESULT ToolboxWindow_UserProc(ToolboxWindow* pToolboxWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void ToolboxWindow_OnPaint(ToolboxWindow* pToolboxWindow);

INT_PTR CALLBACK ToolboxSettingsDlgProc(HWND hDlg, UINT message,
    WPARAM wParam, LPARAM lParam);

PNTVECTOR_DECLARE_TYPE(TOOLBOXICONTHEME)

pntvector(TOOLBOXICONTHEME) g_tBoxIconThemes;
PTOOLBOXICONTHEME g_tBoxSelectedIconTheme;

void ToolboxWindow_AddTool(ToolboxWindow* pToolboxWindow, Tool* pTool)
{
    pToolboxWindow->tools[(size_t)pToolboxWindow->tool_count] = pTool;
    pToolboxWindow->tool_count++;
}

void ToolboxWindow_RemoveTool(ToolboxWindow* pToolboxWindow)
{
    if (pToolboxWindow->tool_count) {
        pToolboxWindow->tool_count--;
    }
}

HBITMAP img_layout;

unsigned int g_uToolSelected;

HTHEME hTheme = NULL;
int btnSize = 32;
int btnOffset = 4;

enum {
    NORMAL,
    PUSHED
};

void Toolbox_ButtonDraw(HDC hdc, int x, int y, unsigned int state)
{
    if (!hTheme) {
        HWND hButton = CreateWindowEx(0, L"BUTTON", L"", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        hTheme = OpenThemeData(hButton, L"BUTTON");
    }

    RECT rc = { 0 };
    rc.left = x;
    rc.top = y;
    rc.right = x + btnSize;
    rc.bottom = y + btnSize;

    if (hTheme)
    {
        int iStateId = PBS_NORMAL;
        if (state == PUSHED)
            iStateId = PBS_PRESSED;

        if (IsThemeBackgroundPartiallyTransparent(hTheme, BP_PUSHBUTTON, iStateId))
        {
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 6387 )
#endif  /* _MSC_VER */

            DrawThemeParentBackground(NULL, hdc, NULL);

#ifdef _MSC_VER
#pragma warning ( pop )
#endif  /* _MSC_VER */
        }

        DrawThemeBackgroundEx(hTheme, hdc, BP_PUSHBUTTON, iStateId, &rc, NULL);
    }
    else {
        DWORD edge = EDGE_RAISED;
        if (state == PUSHED)
            edge = EDGE_SUNKEN;

        DrawEdge(hdc, &rc, edge, BF_RECT);
    }
}

void ToolboxWindow_DrawButtons(ToolboxWindow* pToolboxWindow, HDC hdc)
{
    BITMAP bitmap;
    HDC hdcMem;
    HGDIOBJ oldBitmap;

    hdcMem = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(hdcMem, img_layout);
    GetObject(img_layout, sizeof(bitmap), &bitmap);

    /* Get the client area of the toolbox window */
    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pToolboxWindow, &rcClient);

    int extSize = btnSize + btnOffset;
    int rowCount = rcClient.right / btnSize;

    if (rowCount < 1) {
        rowCount = 1;
    }

    for (unsigned int i = 0; i < pToolboxWindow->tool_count; i++) {
        int x = btnOffset + (i % rowCount) * extSize;
        int y = btnOffset + (i / rowCount) * extSize;

        /* Rectangle(hdc, x, y, x+btnSize, y+btnSize); */

        unsigned int uState = NORMAL;
        if ((unsigned int)i == g_uToolSelected)
        {
            uState = PUSHED;
        }

        Toolbox_ButtonDraw(hdc, x, y, uState);

        int iBmp = pToolboxWindow->tools[(ptrdiff_t)i]->img;
        const int offset = 4;

        BLENDFUNCTION blendFunc;
        blendFunc.BlendOp = AC_SRC_OVER;
        blendFunc.BlendFlags = 0;
        blendFunc.SourceConstantAlpha = 0xFF;
        blendFunc.AlphaFormat = AC_SRC_ALPHA;

        AlphaBlend(hdc, x + offset, y + offset, 24, bitmap.bmHeight, hdcMem, 24 * iBmp, 0, 24, bitmap.bmHeight, blendFunc);
#if 0
        TransparentBlt(hdc, x + offset, y + offset, 16, bitmap.bmHeight, hdcMem, 16 * iBmp, 0, 16, bitmap.bmHeight, (UINT)0x00FF00FF);
#endif
    }

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);
}

void ToolboxWindow_OnPaint(ToolboxWindow* pToolboxWindow)
{
    PAINTSTRUCT ps = { 0 };
    HWND hWnd = Window_GetHWND((Window*)pToolboxWindow);
    HDC hdc = BeginPaint(hWnd, &ps);
    ToolboxWindow_DrawButtons(pToolboxWindow, hdc);
    EndPaint(hWnd, &ps);
}

void ToolboxWindow_OnLButtonUp(ToolboxWindow* pToolboxWindow, int x, int y)
{
    int btnHot = btnSize + 5;

    if (x >= 0 && y >= 0 && x < btnHot * 2 &&
        y < (int)pToolboxWindow->tool_count * btnHot) {
        size_t extSize = (size_t)btnSize + (size_t)btnOffset;

        RECT rcClient = { 0 };
        Window_GetClientRect((Window*)pToolboxWindow, &rcClient);

        size_t rowCount = rcClient.right / extSize;
        if (rowCount < 1) {
            rowCount = 1;
        }

        unsigned int pressed = (y - btnOffset) / (int)extSize * (int)rowCount + (x - btnOffset) / (int)extSize;

        g_uToolSelected = pressed;

        Tool* pTool = NULL;

        if (pressed <= pToolboxWindow->tool_count)
        {
            switch (pressed) {
            case 1:
                pTool = (Tool*)pToolboxWindow->m_pPencilTool;
                break;
            case 2:
                pTool = (Tool*)pToolboxWindow->m_pCircleTool;
                break;
            case 3:
                pTool = (Tool*)pToolboxWindow->m_pLineTool;
                break;
            case 4:
                pTool = (Tool*)pToolboxWindow->m_pRectangleTool;
                break;
            case 5:
                pTool = (Tool*)pToolboxWindow->m_pTextTool;
                break;
            case 6:
                pTool = (Tool*)pToolboxWindow->m_pFillTool;
                break;
            case 7:
                pTool = (Tool*)pToolboxWindow->m_pPickerTool;
                break;
            case 8:
                pTool = (Tool*)pToolboxWindow->m_pBrushTool;
                break;
            case 9:
                pTool = (Tool*)pToolboxWindow->m_pEraserTool;
                break;
            default:
                pTool = (Tool*)pToolboxWindow->m_pPointerTool;
                break;
            }

            WCHAR szLogMessage[80] = L"";
            StringCchPrintf(szLogMessage, 80, L"Selected tool: %s", pTool->pszLabel);
            LogMessage(LOGENTRY_TYPE_DEBUG, L"Toolbox", szLogMessage);

            PanitentApplication* pPanitentApplication = Panitent_GetApp();
            pPanitentApplication->m_pTool = (Tool*)pTool;

#ifdef HAS_DISCORDSDK
            WCHAR szStatus[80] = L"";
            StringCchPrintf(szStatus, 80, L"Drawing with %s", pTool->pszLabel);
            Discord_SetActivityStatus(g_panitent.discord, szStatus);
#endif /* HAS_DISCORDSDK */
        }
    }

    Window_Invalidate((Window*)pToolboxWindow);
}

void Toolbox_OnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    HMENU hMenu;

    hMenu = CreatePopupMenu();
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_TOOLBOXSETTINGS,
        L"Settings");
    TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, x, y, 0, hWnd, NULL);
}

static inline void Toolbox_OpenSettings(HWND hParent)
{
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TOOLBOXSETTINGS),
        hParent, (DLGPROC)ToolboxSettingsDlgProc);
}

BOOL ToolboxWindow_OnCreate(ToolboxWindow* pToolboxWindow, LPCREATESTRUCT lpcs)
{
    return TRUE;
}

LRESULT ToolboxWindow_UserProc(ToolboxWindow* pToolboxWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_LBUTTONUP:
        ToolboxWindow_OnLButtonUp(pToolboxWindow, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        return 0;
        break;

        //case WM_CONTEXTMENU:
        //	Toolbox_OnContextMenu(hwnd, wParam, lParam);
        //	break;

        //case WM_COMMAND:
        //	switch (LOWORD(wParam))
        //	{
        //	case IDM_TOOLBOXSETTINGS:
        //		if (HIWORD(wParam) == BN_CLICKED)
        //			Toolbox_OpenSettings(hwnd);
        //		break;
        //	}
        //	break;
    }

    return Window_UserProcDefault((Window*)pToolboxWindow, hWnd, message, wParam, lParam);
}

typedef struct _tagTOOLBOXSETTINGS {
    int nIconTheme;
} TOOLBOXSETTINGS, * PTOOLBOXSETTINGS;

INT_PTR CALLBACK ToolboxSettingsDlgProc(HWND hDlg, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        PNTSETTINGS* settings;

        PTOOLBOXSETTINGS pTempSettings = calloc(1, sizeof(TOOLBOXSETTINGS));
        assert(pTempSettings);
        if (!pTempSettings)
            EndDialog(hDlg, 0);

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pTempSettings);

        settings = Panitent_GetSettings();
        int iSelTheme = settings->iToolbarIconTheme;

        SendDlgItemMessage(hDlg, IDC_TOOLBARICONTHEME, CB_ADDSTRING,
            0, (LPARAM)L"Modern Colorized");

        SendDlgItemMessage(hDlg, IDC_TOOLBARICONTHEME, CB_ADDSTRING,
            0, (LPARAM)L"Modern Flat");

        SendDlgItemMessage(hDlg, IDC_TOOLBARICONTHEME, CB_ADDSTRING,
            0, (LPARAM)L"Classic");

        SendDlgItemMessage(hDlg, IDC_TOOLBARICONTHEME, CB_SETCURSEL,
            (LPARAM)iSelTheme, (LPARAM)0);
    }

    return TRUE;

    case WM_COMMAND:
    {
        PTOOLBOXSETTINGS pTempSettings =
            (PTOOLBOXSETTINGS)GetWindowLongPtr(hDlg, DWLP_USER);

        switch (LOWORD(wParam))
        {
        case IDC_TOOLBARICONTHEME:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {

                int iCurSel = (int)SendDlgItemMessage(hDlg, IDC_TOOLBARICONTHEME, CB_GETCURSEL, 0, 0);

                pTempSettings->nIconTheme = iCurSel;
            }
            break;
        case IDOK:
        {
            PNTSETTINGS* pSettings;
            pSettings = Panitent_GetSettings();

            pSettings->iToolbarIconTheme = pTempSettings->nIconTheme;
        }
        /* fall through */
        case IDCANCEL:
            free(pTempSettings);
            EndDialog(hDlg, wParam);
            break;
        }
        return TRUE;
    }
    }

    return FALSE;
}

void ToolboxWindow_PreRegister(LPWNDCLASSEX lpwcex);
void ToolboxWindow_PreCreate(LPCREATESTRUCT lpcs);
LRESULT ToolboxWindow_UserProc(ToolboxWindow* pToolboxWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void ToolboxWindow_Init(ToolboxWindow* pToolboxWindow, struct Application* app)
{
    Window_Init(&pToolboxWindow->base, app);

    pToolboxWindow->base.szClassName = szClassName;

    pToolboxWindow->base.OnCreate = (FnWindowOnCreate)ToolboxWindow_OnCreate;
    pToolboxWindow->base.OnPaint = (FnWindowOnPaint)ToolboxWindow_OnPaint;

    _WindowInitHelper_SetPreRegisterRoutine((Window*)pToolboxWindow, (FnWindowPreRegister)ToolboxWindow_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window*)pToolboxWindow, (FnWindowPreCreate)ToolboxWindow_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window*)pToolboxWindow, (FnWindowUserProc)ToolboxWindow_UserProc);

    pToolboxWindow->m_pPointerTool = PointerTool_Create();
    pToolboxWindow->m_pPencilTool = PencilTool_Create();
    pToolboxWindow->m_pCircleTool = CircleTool_Create();
    pToolboxWindow->m_pLineTool = LineTool_Create();
    pToolboxWindow->m_pRectangleTool = RectangleTool_Create();
    pToolboxWindow->m_pTextTool = TextTool_Create();
    pToolboxWindow->m_pFillTool = FillTool_Create();
    pToolboxWindow->m_pPickerTool = PickerTool_Create();
    pToolboxWindow->m_pBrushTool = BrushTool_Create();
    pToolboxWindow->m_pEraserTool = EraserTool_Create();

    PanitentApplication* pPanitentApplication = Panitent_GetApp();
    pPanitentApplication->m_pTool = (Tool*)pToolboxWindow->m_pPointerTool;

    g_uToolSelected = 0;

    img_layout = (HBITMAP)LoadBitmap(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_TOOLS24));

    pToolboxWindow->tools = (Tool**)calloc(16, sizeof(Tool*));
    pToolboxWindow->tool_count = 0;

    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pPointerTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pPencilTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pCircleTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pLineTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pRectangleTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pTextTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pFillTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pPickerTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pBrushTool);
    ToolboxWindow_AddTool(pToolboxWindow, (Tool*)pToolboxWindow->m_pEraserTool);
}

void ToolboxWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void ToolboxWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Toolbox";
    lpcs->style = WS_CAPTION | WS_OVERLAPPED | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

ToolboxWindow* ToolboxWindow_Create(struct Application* app)
{
    ToolboxWindow* pToolboxWindow = calloc(1, sizeof(ToolboxWindow));

    if (pToolboxWindow)
    {
        ToolboxWindow_Init(pToolboxWindow, app);
    }

    return pToolboxWindow;
}
