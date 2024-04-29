#include "precomp.h"

#include <assert.h>
#include <uxtheme.h>

#include "log.h"
#include "log_window.h"
#include "resource.h"

#define IDC_LOGVIEW 1701

typedef struct _tagLOGWINDOWDATA {
    HWND hList;
    int nIDLogObserver;
} LOGWINDOWDATA, * LPLOGWINDOWDATA;

static const WCHAR szClassName[] = L"Win32Class_LogWindow";

/* Private forward declarations */
LogWindow* LogWindow_Create(Application* pApplication);
void LogWindow_Init(LogWindow* pLogWindow, Application* pApplication);

void LogWindow_PreRegister(LPWNDCLASSEX lpwcex);
void LogWindow_PreCreate(LPCREATESTRUCT lpcs);

BOOL LogWindow_OnCreate(LogWindow* pLogWindow, LPCREATESTRUCT lpcs);
void LogWindow_PostCreate(LogWindow* pLogWindow);
void LogWindow_OnSize(LogWindow* pLogWindow, UINT state, int x, int y);
LRESULT LogWindow_OnNotify(LogWindow* pLogWindow, int idCtrl, NMHDR* pnm);
void LogWindow_OnPaint(LogWindow* pLogWindow);
void LogWindow_OnDestroy(LogWindow* pLogWindow);
LRESULT LogWindow_UserProc(LogWindow* pLogWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void LogWindow_LogEventObserverCallback(LPLOGEVENT lEvent);

LogWindow* LogWindow_Create(Application* pApplication)
{
    LogWindow* pLogWindow = (LogWindow*)malloc(sizeof(LogWindow));

    if (pLogWindow)
    {
        memset(pLogWindow, 0, sizeof(LogWindow));
        LogWindow_Init(pLogWindow, pApplication);
    }
}

void LogWindow_Init(LogWindow* pLogWindow, Application* pApplication)
{
    Window_Init(&pLogWindow->base, pApplication);

    pLogWindow->base.OnCreate = (FnWindowOnCreate)LogWindow_OnCreate;
    pLogWindow->base.OnSize = (FnWindowOnSize)LogWindow_OnSize;
    pLogWindow->base.PostCreate = (FnWindowPostCreate)LogWindow_PostCreate;
    pLogWindow->base.OnDestroy = (FnWindowOnDestroy)LogWindow_OnDestroy;
    pLogWindow->base.OnPaint = (FnWindowOnPaint)LogWindow_OnPaint;
    
    pLogWindow->base.PreRegister = (FnWindowPreRegister)LogWindow_PreRegister;
    pLogWindow->base.PreCreate = (FnWindowPreCreate)LogWindow_PreCreate;

    pLogWindow->base.UserProc = (FnWindowUserProc)LogWindow_UserProc;
}

void LogWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LOG));
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void LogWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Log";
    lpcs->style = WS_OVERLAPPEDWINDOW;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 640;
    lpcs->cy = 480;
}

BOOL LogWindow_OnCreate(LogWindow* pLogWindow, LPCREATESTRUCT lpcs)
{

}

HIMAGELIST g_hImageList = NULL;

#define numButtons 3

#define IDM_NEW  1433
#define IDM_OPEN 1434
#define IDM_SAVE 1435

HWND CreateSimpleToolbar(HWND hWndParent)
{
    const int ImageListID = 0;
    const int bitmapSize = 16;

    const DWORD buttonStyles = BTNS_AUTOSIZE;

    // Create the toolbar
    HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | TBSTYLE_WRAPABLE, 0, 0, 0, 0, hWndParent, NULL, GetModuleHandle(NULL), NULL);

    if (!hWndToolbar)
    {
        return NULL;
    }

    // Create the image list.
    g_hImageList = ImageList_Create(bitmapSize, bitmapSize, ILC_COLOR16 | ILC_MASK, numButtons, 0);

    // Set the image list.
    SendMessage(hWndToolbar, TB_SETIMAGELIST, (WPARAM)ImageListID, (LPARAM)g_hImageList);

    // Load the button images.
    SendMessage(hWndToolbar, TB_LOADIMAGES, (WPARAM)IDB_STD_SMALL_COLOR, (LPARAM)HINST_COMMCTRL);

    // Initialize button info
    // IDM_NEW, IDM_OPEN, and IDM_SAVE are application-defined command constants


    TBBUTTON tbButtons[numButtons] =
    {
        { MAKELONG(STD_FILENEW,  ImageListID), IDM_NEW,  TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)L"New" },
        { MAKELONG(STD_FILEOPEN, ImageListID), IDM_OPEN, TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)L"Open"},
        { MAKELONG(STD_FILESAVE, ImageListID), IDM_SAVE, 0,               buttonStyles, {0}, 0, (INT_PTR)L"Save"}
    };

    // Add buttons.
    SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)numButtons, (LPARAM)&tbButtons);

    // Resize the toolbar, and the show it.
    SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
    ShowWindow(hWndToolbar, TRUE);

    return hWndToolbar;
}

void LogWindow_PostCreate(LogWindow* pLogWindow)
{
    HWND hWnd = Window_GetHWND((Window*)pLogWindow);

    HWND hToolbar = CreateSimpleToolbar(hWnd);
    pLogWindow->hToolbar = hToolbar;

    DWORD dwError;

    HWND hList = CreateWindowEx(0, WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA, 0, 0, 0, 0, hWnd, (HMENU)IDC_LOGVIEW, GetModuleHandle(NULL), NULL);
    dwError = GetLastError();

    if (!hList) {
        assert(hList);
    }

    ListView_SetExtendedListViewStyle(hList, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_JUSTIFYCOLUMNS);
    SetWindowTheme(hList, L"Explorer", NULL);

    WCHAR szText[256];
    LVCOLUMN lvc;
    int iCol = 0;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.pszText = szText;
    lvc.fmt = LVCFMT_LEFT;

    lvc.cx = 100;
    lvc.iSubItem = iCol;
    StringCchCopy(szText, 256, L"Time");
    ListView_InsertColumn(hList, iCol++, &lvc);

    lvc.cx = 70;
    lvc.iSubItem = iCol;
    StringCchCopy(szText, 256, L"Type");
    ListView_InsertColumn(hList, iCol++, &lvc);

    lvc.iSubItem = iCol;
    StringCchCopy(szText, 256, L"Module");
    ListView_InsertColumn(hList, iCol++, &lvc);

    lvc.cx = 200;
    lvc.iSubItem = iCol;
    StringCchCopy(szText, 256, L"Message");
    ListView_InsertColumn(hList, iCol++, &lvc);

    pLogWindow->hList = hList;

    pLogWindow->nIDLogObserver = LogRegisterObserver(LogWindow_LogEventObserverCallback, (LPVOID)pLogWindow);

    int nEntries = LogGetSize();
    ListView_SetItemCount(hList, nEntries);
}

void LogWindow_OnSize(LogWindow* pLogWindow, UINT state, int x, int y)
{
    RECT rcClient = { 0 };
    Window_GetClientRect(pLogWindow, &rcClient);

    RECT rcToolbar = { 0 };
    GetWindowRect(pLogWindow->hToolbar, &rcToolbar);

    int toolbarHeight = rcToolbar.bottom - rcToolbar.top;

    SetWindowPos(pLogWindow->hToolbar, NULL, 0, 0, rcClient.right - rcClient.left, toolbarHeight, 0);
    SetWindowPos(pLogWindow->hList, NULL, 0, toolbarHeight, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top - toolbarHeight, 0);
}

LRESULT LogWindow_OnNotify(LogWindow* pLogWindow, int idCtrl, NMHDR* pnm)
{
    if (idCtrl == IDC_LOGVIEW)
    {
        switch (pnm->code)
        {
        case LVN_GETDISPINFO:
        {
            LOGENTRY logEntry;
            NMLVDISPINFO* plvdi = (NMLVDISPINFO*)pnm; /* ??? */

            if (plvdi->item.iItem == -1)
            {
                OutputDebugString(L"LVOWNER: Request for -1 item?\n");
                DebugBreak();
            }

            LPLOGENTRY pEntry;
            pEntry = RetrieveLogEntry(plvdi->item.iItem);
            if (!pEntry)
            {
                assert(FALSE);
            }

            memcpy(&logEntry, pEntry, sizeof(LOGENTRY));

            if (plvdi->item.mask & LVIF_TEXT)
            {
                switch (plvdi->item.iSubItem)
                {
                case 0:
                {
                    DWORD dwError;
                    int cchCount;
                    WCHAR szTimeString[256] = { 0 };

                    cchCount = GetTimeFormatEx(LOCALE_NAME_INVARIANT,
                        TIME_FORCE24HOURFORMAT,
                        &logEntry.timestamp,
                        NULL,
                        szTimeString,
                        256);

                    if (!cchCount)
                    {
                        dwError = GetLastError();
                        assert(FALSE);
                    }

                    StringCchCopy(plvdi->item.pszText, 80, szTimeString);
                }
                break;

                case 1:
                {
                    switch (logEntry.iType)
                    {
                    case LOGENTRY_TYPE_DEBUG:
                        StringCchCopy(plvdi->item.pszText, 80, L"Debug");
                        break;

                    case LOGENTRY_TYPE_INFO:
                        StringCchCopy(plvdi->item.pszText, 80, L"Info");
                        break;

                    case LOGENTRY_TYPE_WARNING:
                        StringCchCopy(plvdi->item.pszText, 80, L"Warning");
                        break;

                    case LOGENTRY_TYPE_ERROR:
                        StringCchCopy(plvdi->item.pszText, 80, L"Error");
                        break;
                    }
                }
                break;

                case 2:
                    StringCchCopy(plvdi->item.pszText, 80, logEntry.pszModule);
                    break;

                case 3:
                    StringCchCopy(plvdi->item.pszText, 80, logEntry.pszMessage);
                    break;

                default:
                    break;
                }
            }
            return FALSE;
        }
        break;

        case LVN_ODCACHEHINT:
            // NMLVCACHEHINT *pCacheHint = (NMLVCACHEHINT *) pnm;
            break;
        }
    }

    return FALSE;
}

void LogWindow_OnPaint(LogWindow* pLogWindow)
{

}

void LogWindow_OnDestroy(LogWindow* pLogWindow)
{
    LogUnregisterObserver(pLogWindow->nIDLogObserver);
}

LRESULT LogWindow_UserProc(LogWindow* pLogWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_NOTIFY:
    {
        return LogWindow_OnNotify(pLogWindow, (int)(wParam), (NMHDR*)(lParam));
    }
        break;
    }

    return Window_UserProcDefault(pLogWindow, hWnd, message, wParam, lParam);
}

void LogWindow_LogEventObserverCallback(LPLOGEVENT lEvent)
{
    switch (lEvent->iType)
    {
    case LOGALTEREVENT_ADD:
    {
        LogWindow* pLogWindow = (LogWindow*)lEvent->userData;
        assert(pLogWindow);

        ListView_SetItemCount(pLogWindow->hList, lEvent->extra);
    }
    break;
    }
}
