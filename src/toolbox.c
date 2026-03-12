#include "precomp.h"

#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

#include "win32/window.h"
#include "win32/util.h"
#include "viewport.h"
#include "tool.h"
#include "tools/common.h"

#include "toolbox.h"

#include "resource.h"
#include "canvas.h"
#include "document.h"
#include "log.h"
#include "color_context.h"
#include "util.h"
#include "brush.h"
#include "history.h"
#include "sharing/activitysharingmanager.h"

#include "panitentapp.h"
#include "theme.h"

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

unsigned int g_uToolPrevious;
unsigned int g_uToolSelected;

HTHEME hTheme = NULL;
int btnSize = 32;
int btnOffset = 4;

enum {
    NORMAL,
    PUSHED
};

const BOOL bOrange = TRUE;

static int ToolboxWindow_GetMinClientWidth(void)
{
    return btnOffset + btnSize + btnOffset;
}

static int ToolboxWindow_GetColumnCount(ToolboxWindow* pToolboxWindow)
{
    if (!pToolboxWindow)
    {
        return 1;
    }

    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pToolboxWindow, &rcClient);
    int clientWidth = max(0, rcClient.right - rcClient.left);
    int extSize = btnSize + btnOffset;
    if (extSize <= 0)
    {
        return 1;
    }

    int columns = (clientWidth - btnOffset) / extSize;
    if (columns < 1)
    {
        columns = 1;
    }
    return columns;
}

static int ToolboxWindow_HitTestButton(ToolboxWindow* pToolboxWindow, int x, int y)
{
    if (!pToolboxWindow || pToolboxWindow->tool_count == 0)
    {
        return -1;
    }

    int extSize = btnSize + btnOffset;
    if (extSize <= 0)
    {
        return -1;
    }

    int localX = x - btnOffset;
    int localY = y - btnOffset;
    if (localX < 0 || localY < 0)
    {
        return -1;
    }

    int columns = ToolboxWindow_GetColumnCount(pToolboxWindow);
    int col = localX / extSize;
    int row = localY / extSize;
    if (col < 0 || col >= columns || row < 0)
    {
        return -1;
    }

    if ((localX % extSize) >= btnSize || (localY % extSize) >= btnSize)
    {
        return -1;
    }

    int index = row * columns + col;
    if (index < 0 || (unsigned int)index >= pToolboxWindow->tool_count)
    {
        return -1;
    }

    return index;
}

static void ToolboxWindow_InvalidateNoErase(ToolboxWindow* pToolboxWindow)
{
    if (!pToolboxWindow)
    {
        return;
    }

    InvalidateRect(Window_GetHWND((Window*)pToolboxWindow), NULL, FALSE);
}

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

    if (bOrange)
    {
        HPEN hOldPen = SelectObject(hdc, GetStockObject(DC_PEN));
        HBRUSH hOldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));

        if (state == PUSHED)
        {
            PanitentThemeColors colors = { 0 };
            PanitentTheme_GetColors(&colors);
            SetDCPenColor(hdc, colors.border);
            SetDCBrushColor(hdc, colors.accent);
            
        }
        else {
            SetDCPenColor(hdc, Win32_HexToCOLORREF(L"#aaaaaa"));
            SetDCBrushColor(hdc, Win32_HexToCOLORREF(L"#eeeeee"));
        }

        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 3, 3);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
    }
    else if (hTheme)
    {
        int iStateId = state == PUSHED ? PBS_PRESSED : PBS_NORMAL;

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
        {
            edge = EDGE_SUNKEN;
        }

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
    int columnCount = ToolboxWindow_GetColumnCount(pToolboxWindow);

    for (unsigned int i = 0; i < pToolboxWindow->tool_count; i++) {
        int x = btnOffset + (i % columnCount) * extSize;
        int y = btnOffset + (i / columnCount) * extSize;

        unsigned int uState = (unsigned int)i == g_uToolSelected ? PUSHED : NORMAL;

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
        TransparentBlt(hdc, x + nOffset, y + nOffset, 16, bitmap.bmHeight, hdcMem, 16 * iBmp, 0, 16, bitmap.bmHeight, (UINT)0x00FF00FF);
#endif
    }

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);
}

#define ANIMATION_DURATION 200

void ToolboxWindow_OnPaint(ToolboxWindow* pToolboxWindow)
{
    HWND hWnd = Window_GetHWND((Window*)pToolboxWindow);

    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hWnd, &ps);

    if (hdc)
    {
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        // Rectangle(hdc, rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

        if (!BufferedPaintRenderAnimation(hWnd, hdc))
        {
            BP_ANIMATIONPARAMS bpap;
            memset(&bpap, 0, sizeof(BP_ANIMATIONPARAMS));
            bpap.cbSize = sizeof(BP_ANIMATIONPARAMS);
            bpap.style = BPAS_CUBIC;

            /* Check if animation is needed. If not set dwDuration to 0 */
            bpap.dwDuration = (g_uToolSelected != g_uToolPrevious ? ANIMATION_DURATION : 0);

            HDC hdcFrom;
            HDC hdcTo;
            HANIMATIONBUFFER hbpAnimation = BeginBufferedAnimation(hWnd, hdc, &rcClient, BPBF_COMPATIBLEBITMAP, NULL, &bpap, &hdcFrom, &hdcTo);
            if (hbpAnimation)
            {
                /* Hack */
                HBRUSH hbrBackground = (HBRUSH)GetClassLongPtr(hWnd, GCLP_HBRBACKGROUND);
                if (hdcFrom)
                {
                    /* Hack */
                    FillRect(hdcFrom, &rcClient, hbrBackground);

                    unsigned int temp = g_uToolSelected;
                    g_uToolSelected = g_uToolPrevious;
                    ToolboxWindow_DrawButtons(pToolboxWindow, hdcFrom);
                    g_uToolSelected = temp;
                }

                if (hdcTo)
                {
                    /* Hack */
                    FillRect(hdcTo, &rcClient, hbrBackground);

                    ToolboxWindow_DrawButtons(pToolboxWindow, hdcTo);
                }

                g_uToolPrevious = g_uToolSelected;
                EndBufferedAnimation(hbpAnimation, TRUE);
            }
            /* If animation unavailable just draw buttons */
            else
            {
                ToolboxWindow_DrawButtons(pToolboxWindow, hdc);
            }
        }

        EndPaint(hWnd, &ps);
    }
}

void ToolboxWindow_OnLButtonUp(ToolboxWindow* pToolboxWindow, int x, int y)
{
    int pressed = ToolboxWindow_HitTestButton(pToolboxWindow, x, y);
    if (pressed >= 0)
    {
        g_uToolPrevious = g_uToolSelected;
        g_uToolSelected = (unsigned int)pressed;

        Tool* pTool = pToolboxWindow->tools[pressed];
        if (pTool)
        {
            LogMessageF(LOGENTRY_TYPE_DEBUG, L"Toolbox", L"Selected tool: %s", pTool->pszLabel);
            PanitentApp_SetTool(PanitentApp_Instance(), pTool);
        }
    }

    ToolboxWindow_InvalidateNoErase(pToolboxWindow);
}

void Toolbox_OnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    HMENU hMenu;

    hMenu = CreatePopupMenu();
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_TOOLBOXSETTINGS, L"Settings");
    TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, x, y, 0, hWnd, NULL);
}

static inline void Toolbox_OpenSettings(HWND hParent)
{
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TOOLBOXSETTINGS), hParent, (DLGPROC)ToolboxSettingsDlgProc);
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

    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO pMMI = (LPMINMAXINFO)lParam;
        if (pMMI)
        {
            RECT rc = { 0, 0, ToolboxWindow_GetMinClientWidth(), btnOffset + btnSize + btnOffset };
            DWORD dwStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
            DWORD dwExStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
            AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
            pMMI->ptMinTrackSize.x = max(pMMI->ptMinTrackSize.x, rc.right - rc.left);
            pMMI->ptMinTrackSize.y = max(pMMI->ptMinTrackSize.y, rc.bottom - rc.top);
            return 0;
        }
    }
    break;

    case WM_ERASEBKGND:
        return 1;
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

        PTOOLBOXSETTINGS pTempSettings = (PTOOLBOXSETTINGS)malloc(sizeof(TOOLBOXSETTINGS));
        assert(pTempSettings);
        if (!pTempSettings)
        {
            memset(pTempSettings, 0, sizeof(TOOLBOXSETTINGS));
            EndDialog(hDlg, 0);
        }

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pTempSettings);

        settings = PanitentApp_GetSettings(PanitentApp_Instance());
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
            pSettings = PanitentApp_GetSettings(PanitentApp_Instance());

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

void ToolboxWindow_Init(ToolboxWindow* pToolboxWindow)
{
    Window_Init(&pToolboxWindow->base);

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

    PanitentApp_SetTool(PanitentApp_Instance(), (Tool*)pToolboxWindow->m_pPointerTool);

    g_uToolPrevious = 0;
    g_uToolSelected = 0;

    img_layout = (HBITMAP)LoadBitmap(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_TOOLS24));

    pToolboxWindow->tools = (Tool**)malloc(16 * sizeof(Tool*));
    memset(pToolboxWindow->tools, 0, 16 * sizeof(Tool*));
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

ToolboxWindow* ToolboxWindow_Create()
{
    ToolboxWindow* pToolboxWindow = (ToolboxWindow*)malloc(sizeof(ToolboxWindow));
    
    if (pToolboxWindow)
    {
        memset(pToolboxWindow, 0, sizeof(ToolboxWindow));
        ToolboxWindow_Init(pToolboxWindow);
    }

    return pToolboxWindow;
}
