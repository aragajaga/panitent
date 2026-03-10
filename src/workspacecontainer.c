#include "precomp.h"
#include "win32/window.h"
#include "win32/util.h"
#include "win32/windowmap.h"
#include "viewport.h"
#include "workspacecontainer.h"
#include "document.h"
#include "documentdocktransition.h"
#include "floatingdocumentcreate.h"
#include "floatingwindowcontainer.h"
#include "dockhost.h"
#include "oledroptarget.h"
#include "workspacedockpolicy.h"
#include "panitentapp.h"
#include "panitentwindow.h"
#include "theme.h"
#include "resource.h"

static const WCHAR szClassName[] = L"__WorkspaceContainer";

#define WORKSPACE_TAB_HEIGHT 24
#define WORKSPACE_TAB_GAP 2
#define WORKSPACE_TAB_TEXT_PADDING_X 10
#define WORKSPACE_TAB_MIN_WIDTH 72
#define WORKSPACE_TAB_MAX_WIDTH 240
#define WORKSPACE_TAB_FLOAT_DISTANCE 6
#define WORKSPACE_TAB_CLOSE_SIZE 10
#define WORKSPACE_TAB_CLOSE_GAP 8
#define IDM_WORKSPACE_TAB_FLOAT 41001
#define WORKSPACE_EMPTY_WATERMARK_MARGIN 16

/* Private forward declarations */
WorkspaceContainer* WorkspaceContainer_Create();
void WorkspaceContainer_Init(WorkspaceContainer*);

void WorkspaceContainer_PreRegister(LPWNDCLASSEX lpwcex);
void WorkspaceContainer_PreCreate(LPCREATESTRUCT lpcs);

BOOL WorkspaceContainer_OnCreate(WorkspaceContainer* pWorkspaceContainer, LPCREATESTRUCT lpcs);
void WorkspaceContainer_OnPaint(WorkspaceContainer* pWorkspaceContainer);
void WorkspaceContainer_OnLButtonDown(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags);
void WorkspaceContainer_OnLButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags);
void WorkspaceContainer_OnMouseMove(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags);
void WorkspaceContainer_OnRButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags);
void WorkspaceContainer_OnContextMenu(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags);
void WorkspaceContainer_OnSize(WorkspaceContainer* pWorkspaceContainer, UINT state, int cx, int cy);
void WorkspaceContainer_OnDestroy(WorkspaceContainer* pWorkspaceContainer);

LRESULT CALLBACK WorkspaceContainer_UserProc(WorkspaceContainer* pWorkspaceContainer, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WorkspaceContainer_GetTabHeaderHeight(WorkspaceContainer* pWorkspaceContainer);
void WorkspaceContainer_LayoutViewports(WorkspaceContainer* pWorkspaceContainer);
void WorkspaceContainer_SetCurrentViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow, BOOL bSyncAppState);
void WorkspaceContainer_UpdateWindowTitle(WorkspaceContainer* pWorkspaceContainer);
BOOL WorkspaceContainer_IsFloating(WorkspaceContainer* pWorkspaceContainer);
void WorkspaceContainer_GetViewportTitle(ViewportWindow* pViewportWindow, int nTabIndex, LPWSTR pszBuffer, size_t cchBuffer);
int WorkspaceContainer_CalcTabWidth(WorkspaceContainer* pWorkspaceContainer, HDC hdc, ViewportWindow* pViewportWindow, int nTabIndex);
BOOL WorkspaceContainer_GetTabRect(WorkspaceContainer* pWorkspaceContainer, HDC hdc, int nTabIndex, LPRECT prcTab);
BOOL WorkspaceContainer_GetTabCloseRect(WorkspaceContainer* pWorkspaceContainer, HDC hdc, int nTabIndex, LPRECT prcClose);
int WorkspaceContainer_HitTestTab(WorkspaceContainer* pWorkspaceContainer, int x, int y);
int WorkspaceContainer_HitTestTabClose(WorkspaceContainer* pWorkspaceContainer, int x, int y);
int ViewportVector_FindIndex(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow);
void WorkspaceContainer_CloseViewportAt(WorkspaceContainer* pWorkspaceContainer, int nTabIndex);
void WorkspaceContainer_FloatViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow, int xScreen, int yScreen, BOOL bStartMove);
void WorkspaceContainer_MoveAllViewportsTo(WorkspaceContainer* pSourceWorkspace, WorkspaceContainer* pTargetWorkspace);
WorkspaceContainer* WorkspaceContainer_FindDropTargetAtScreenPoint(WorkspaceContainer* pSourceWorkspace, POINT ptScreen);
BOOL WorkspaceContainer_TryDockFloating(WorkspaceContainer* pSourceWorkspace, BOOL bForceMainWorkspace);
static DockHostWindow* WorkspaceContainer_GetOwningDockHost(WorkspaceContainer* pWorkspaceContainer);
static BOOL WorkspaceContainer_IsMainWorkspace(WorkspaceContainer* pWorkspaceContainer);
static void WorkspaceContainer_TryRemoveEmptyDockedGroup(WorkspaceContainer* pWorkspaceContainer);
static void WorkspaceContainer_FinalizeEmptySourceAfterDetach(WorkspaceContainer* pWorkspaceContainer);
static BOOL WorkspaceContainer_RestoreViewportAfterFloatFailure(
    WorkspaceContainer* pWorkspaceContainer,
    ViewportWindow* pViewportWindow,
    int nViewportIndex,
    ViewportWindow* pOriginalCurrentViewport);
static BOOL WorkspaceContainer_DrawMaskedBitmap(HDC hdc, const RECT* pDestRect, int idBitmap, COLORREF tint);
static BOOL WorkspaceContainer_OnDropFiles(void* pContext, HDROP hDrop);

struct ViewportVector {
    size_t m_capacity;
    size_t m_size;
    ViewportWindow** pData;
};

ViewportVector* ViewportVector_Create();
void ViewportVector_Init(ViewportVector* pViewportVector);
BOOL ViewportVector_Reserve(ViewportVector* pViewportVector, size_t newCapacity);
BOOL ViewportVector_Add(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow);
BOOL ViewportVector_InsertAt(ViewportVector* pViewportVector, int idx, ViewportWindow* pViewportWindow);
BOOL ViewportVector_RemoveAt(ViewportVector* pViewportVector, int idx, ViewportWindow** ppViewportWindow);
size_t ViewportVector_GetSize(ViewportVector* pViewportVector);
ViewportWindow* ViewportVector_Get(ViewportVector* pViewportVector, int idx);

typedef struct WorkspaceDropSearchContext WorkspaceDropSearchContext;
struct WorkspaceDropSearchContext {
    WorkspaceContainer* pSourceWorkspace;
    POINT ptScreen;
    HWND hWndExcludeRoot;
    WorkspaceContainer* pFoundWorkspace;
};

typedef struct WorkspaceWatermarkCache WorkspaceWatermarkCache;
struct WorkspaceWatermarkCache
{
    HBITMAP hSourceBitmap;
    HBITMAP hTintedBitmap;
    int width;
    int height;
    COLORREF tint;
};

static WorkspaceWatermarkCache g_workspaceWatermarkCache = { 0 };

BOOL CALLBACK WorkspaceContainer_EnumWindows_FindWorkspaceAtPoint(HWND hWnd, LPARAM lParam);
BOOL CALLBACK WorkspaceContainer_EnumChildWindows_FindWorkspaceAtPoint(HWND hWnd, LPARAM lParam);

static BOOL WorkspaceContainer_EnsureWatermarkCache(HDC hdc, int idBitmap, COLORREF tint)
{
    HDC hdcMask = NULL;
    HBITMAP hOldBitmap = NULL;
    BITMAP bm = { 0 };
    BITMAPINFO bmi = { 0 };
    uint32_t* pPixels = NULL;
    HDC hdcColored = NULL;
    HBITMAP hOldColored = NULL;
    BYTE targetR = GetRValue(tint);
    BYTE targetG = GetGValue(tint);
    BYTE targetB = GetBValue(tint);

    if (!hdc)
    {
        return FALSE;
    }

    if (!g_workspaceWatermarkCache.hSourceBitmap)
    {
        g_workspaceWatermarkCache.hSourceBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(idBitmap));
        if (!g_workspaceWatermarkCache.hSourceBitmap)
        {
            return FALSE;
        }

        if (!GetObject(g_workspaceWatermarkCache.hSourceBitmap, sizeof(BITMAP), &bm))
        {
            DeleteObject(g_workspaceWatermarkCache.hSourceBitmap);
            g_workspaceWatermarkCache.hSourceBitmap = NULL;
            return FALSE;
        }

        g_workspaceWatermarkCache.width = bm.bmWidth;
        g_workspaceWatermarkCache.height = bm.bmHeight;
    }

    if (g_workspaceWatermarkCache.hTintedBitmap &&
        g_workspaceWatermarkCache.tint == tint)
    {
        return TRUE;
    }

    if (g_workspaceWatermarkCache.hTintedBitmap)
    {
        DeleteObject(g_workspaceWatermarkCache.hTintedBitmap);
        g_workspaceWatermarkCache.hTintedBitmap = NULL;
    }

    if (g_workspaceWatermarkCache.width <= 0 || g_workspaceWatermarkCache.height <= 0)
    {
        return FALSE;
    }

    hdcMask = CreateCompatibleDC(hdc);
    if (!hdcMask)
    {
        return FALSE;
    }

    hOldBitmap = (HBITMAP)SelectObject(hdcMask, g_workspaceWatermarkCache.hSourceBitmap);
    if (!hOldBitmap)
    {
        DeleteDC(hdcMask);
        return FALSE;
    }

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_workspaceWatermarkCache.width;
    bmi.bmiHeader.biHeight = -g_workspaceWatermarkCache.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    g_workspaceWatermarkCache.hTintedBitmap = CreateDIBSection(
        hdc,
        &bmi,
        DIB_RGB_COLORS,
        (LPVOID*)&pPixels,
        NULL,
        0);
    if (!g_workspaceWatermarkCache.hTintedBitmap || !pPixels)
    {
        g_workspaceWatermarkCache.hTintedBitmap = NULL;
        SelectObject(hdcMask, hOldBitmap);
        DeleteDC(hdcMask);
        return FALSE;
    }

    hdcColored = CreateCompatibleDC(hdc);
    if (!hdcColored)
    {
        DeleteObject(g_workspaceWatermarkCache.hTintedBitmap);
        g_workspaceWatermarkCache.hTintedBitmap = NULL;
        SelectObject(hdcMask, hOldBitmap);
        DeleteDC(hdcMask);
        return FALSE;
    }

    hOldColored = (HBITMAP)SelectObject(hdcColored, g_workspaceWatermarkCache.hTintedBitmap);
    if (!hOldColored)
    {
        DeleteDC(hdcColored);
        DeleteObject(g_workspaceWatermarkCache.hTintedBitmap);
        g_workspaceWatermarkCache.hTintedBitmap = NULL;
        SelectObject(hdcMask, hOldBitmap);
        DeleteDC(hdcMask);
        return FALSE;
    }

    for (int y = 0; y < g_workspaceWatermarkCache.height; ++y)
    {
        for (int x = 0; x < g_workspaceWatermarkCache.width; ++x)
        {
            COLORREF srcColor = GetPixel(hdcMask, x, y);
            BYTE mask = (BYTE)(((int)GetRValue(srcColor) + (int)GetGValue(srcColor) + (int)GetBValue(srcColor)) / 3);
            BYTE outR = (BYTE)(((int)targetR * (int)mask + 127) / 255);
            BYTE outG = (BYTE)(((int)targetG * (int)mask + 127) / 255);
            BYTE outB = (BYTE)(((int)targetB * (int)mask + 127) / 255);

            pPixels[(size_t)y * (size_t)g_workspaceWatermarkCache.width + (size_t)x] =
                ((uint32_t)mask << 24) |
                ((uint32_t)outR << 16) |
                ((uint32_t)outG << 8) |
                (uint32_t)outB;
        }
    }

    g_workspaceWatermarkCache.tint = tint;

    SelectObject(hdcColored, hOldColored);
    DeleteDC(hdcColored);
    SelectObject(hdcMask, hOldBitmap);
    DeleteDC(hdcMask);
    return TRUE;
}

static BOOL WorkspaceContainer_DrawMaskedBitmap(HDC hdc, const RECT* pDestRect, int idBitmap, COLORREF tint)
{
    HDC hdcColored = NULL;
    HBITMAP hOldColored = NULL;
    int destWidth = 0;
    int destHeight = 0;
    BOOL fDrawn = FALSE;

    if (!hdc || !pDestRect)
    {
        return FALSE;
    }

    destWidth = pDestRect->right - pDestRect->left;
    destHeight = pDestRect->bottom - pDestRect->top;
    if (destWidth <= 0 || destHeight <= 0)
    {
        return FALSE;
    }

    if (!WorkspaceContainer_EnsureWatermarkCache(hdc, idBitmap, tint))
    {
        return FALSE;
    }

    hdcColored = CreateCompatibleDC(hdc);
    if (!hdcColored)
    {
        return FALSE;
    }

    hOldColored = (HBITMAP)SelectObject(hdcColored, g_workspaceWatermarkCache.hTintedBitmap);
    if (!hOldColored)
    {
        DeleteDC(hdcColored);
        return FALSE;
    }

    {
        BLENDFUNCTION blend = { 0 };
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 0xFF;
        blend.AlphaFormat = AC_SRC_ALPHA;
        fDrawn = AlphaBlend(
            hdc,
            pDestRect->left,
            pDestRect->top,
            destWidth,
            destHeight,
            hdcColored,
            0,
            0,
            g_workspaceWatermarkCache.width,
            g_workspaceWatermarkCache.height,
            blend);
    }

    SelectObject(hdcColored, hOldColored);
    DeleteDC(hdcColored);
    return fDrawn;
}

ViewportVector* ViewportVector_Create()
{
    ViewportVector* pViewportVector = (ViewportVector *)malloc(sizeof(ViewportVector));
    if (!pViewportVector)
    {
        return NULL;
    }

    memset(pViewportVector, 0, sizeof(ViewportVector));
    ViewportVector_Init(pViewportVector);
    return pViewportVector;
}

void ViewportVector_Init(ViewportVector* pViewportVector)
{
    if (!pViewportVector)
    {
        return;
    }

    pViewportVector->m_capacity = 0;
    pViewportVector->m_size = 0;
    pViewportVector->pData = NULL;

    ViewportVector_Reserve(pViewportVector, 10);
}

WorkspaceContainer* WorkspaceContainer_Create()
{
    WorkspaceContainer* pWorkspaceContainer = (WorkspaceContainer*)malloc(sizeof(WorkspaceContainer));
    memset(pWorkspaceContainer, 0, sizeof(WorkspaceContainer));

    if (pWorkspaceContainer)
    {
        WorkspaceContainer_Init(pWorkspaceContainer);
    }

    return pWorkspaceContainer;
}

void WorkspaceContainer_Init(WorkspaceContainer* pWorkspaceContainer)
{
    Window_Init(&pWorkspaceContainer->base);

    pWorkspaceContainer->base.szClassName = szClassName;

    pWorkspaceContainer->base.OnCreate = (FnWindowOnCreate)WorkspaceContainer_OnCreate;
    pWorkspaceContainer->base.OnDestroy = (FnWindowOnDestroy)WorkspaceContainer_OnDestroy;
    pWorkspaceContainer->base.OnPaint = (FnWindowOnPaint)WorkspaceContainer_OnPaint;
    pWorkspaceContainer->base.OnSize = (FnWindowOnSize)WorkspaceContainer_OnSize;

    _WindowInitHelper_SetPreRegisterRoutine((Window*)pWorkspaceContainer, (FnWindowPreRegister)WorkspaceContainer_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window*)pWorkspaceContainer, (FnWindowPreCreate)WorkspaceContainer_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window*)pWorkspaceContainer, (FnWindowUserProc)WorkspaceContainer_UserProc);

    pWorkspaceContainer->m_pViewportVector = ViewportVector_Create();
    pWorkspaceContainer->m_iPressedTabIndex = -1;
    pWorkspaceContainer->m_iPressedCloseTabIndex = -1;
    pWorkspaceContainer->m_ptTabDragStart.x = 0;
    pWorkspaceContainer->m_ptTabDragStart.y = 0;
}

void WorkspaceContainer_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = NULL;
    lpwcex->lpszClassName = szClassName;
}

void WorkspaceContainer_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"WorkspaceContainer";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

BOOL WorkspaceContainer_OnCreate(WorkspaceContainer* pWorkspaceContainer, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(lpcs);

    OleFileDropTarget_Register(
        Window_GetHWND((Window*)pWorkspaceContainer),
        WorkspaceContainer_OnDropFiles,
        pWorkspaceContainer,
        (IDropTarget**)&pWorkspaceContainer->pFileDropTarget);

    return TRUE;
}

void WorkspaceContainer_OnSize(WorkspaceContainer* pWorkspaceContainer, UINT state, int cx, int cy)
{
    UNREFERENCED_PARAMETER(state);
    UNREFERENCED_PARAMETER(cx);
    UNREFERENCED_PARAMETER(cy);

    WorkspaceContainer_LayoutViewports(pWorkspaceContainer);
}

int WorkspaceContainer_GetTabHeaderHeight(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return 0;
    }

    return ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector) > 0 ?
        WORKSPACE_TAB_HEIGHT : 0;
}

void WorkspaceContainer_LayoutViewports(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return;
    }

    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);

    int top = WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer);
    int width = max(0, rcClient.right - rcClient.left);
    int height = max(0, rcClient.bottom - top);

    for (int i = 0; i < (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector); i++)
    {
        ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, i);
        if (!pViewportWindow)
        {
            continue;
        }

        HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
        if (!IsWindow(hWndViewport))
        {
            continue;
        }

        if (pViewportWindow == pWorkspaceContainer->m_pViewportWindow)
        {
            SetWindowPos(hWndViewport, HWND_TOP, 0, top, width, height,
                SWP_NOACTIVATE | SWP_SHOWWINDOW);
            continue;
        }

        ShowWindow(hWndViewport, SW_HIDE);
    }
}

void WorkspaceContainer_SetCurrentViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow, BOOL bSyncAppState)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return;
    }

    if (!pViewportWindow && ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector) > 0)
    {
        pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, 0);
    }

    pWorkspaceContainer->m_pViewportWindow = pViewportWindow;
    WorkspaceContainer_LayoutViewports(pWorkspaceContainer);

    if (pViewportWindow)
    {
        HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
        if (IsWindow(hWndViewport))
        {
            SetFocus(hWndViewport);
        }
    }

    if (bSyncAppState)
    {
        PanitentApp_SetActiveViewport(PanitentApp_Instance(), pViewportWindow);
    }

    WorkspaceContainer_UpdateWindowTitle(pWorkspaceContainer);
    Window_Invalidate((Window*)pWorkspaceContainer);
}

BOOL WorkspaceContainer_IsFloating(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer)
    {
        return FALSE;
    }

    HWND hWndWorkspace = Window_GetHWND((Window*)pWorkspaceContainer);
    if (!hWndWorkspace || !IsWindow(hWndWorkspace))
    {
        return FALSE;
    }

    DWORD dwStyle = Window_GetStyle((Window*)pWorkspaceContainer);
    return (dwStyle & WS_CHILD) == 0;
}

static DockHostWindow* WorkspaceContainer_GetOwningDockHost(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer)
    {
        return NULL;
    }

    HWND hWndWorkspace = Window_GetHWND((Window*)pWorkspaceContainer);
    if (!hWndWorkspace || !IsWindow(hWndWorkspace))
    {
        return NULL;
    }

    HWND hWndParent = GetParent(hWndWorkspace);
    if (!hWndParent || !IsWindow(hWndParent))
    {
        return NULL;
    }

    WCHAR szParentClass[64] = L"";
    GetClassNameW(hWndParent, szParentClass, ARRAYSIZE(szParentClass));
    if (wcscmp(szParentClass, L"__DockHostWindow") != 0)
    {
        return NULL;
    }

    Window* pParentWindow = WindowMap_Get(hWndParent);
    if (!pParentWindow)
    {
        return NULL;
    }

    return (DockHostWindow*)pParentWindow;
}

static BOOL WorkspaceContainer_IsMainWorkspace(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer)
    {
        return FALSE;
    }

    WorkspaceContainer* pMainWorkspace = PanitentApp_GetWorkspaceContainer(PanitentApp_Instance());
    return pWorkspaceContainer == pMainWorkspace;
}

static void WorkspaceContainer_TryRemoveEmptyDockedGroup(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return;
    }

    int nDocs = (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector);
    DockHostWindow* pDockHostWindow = WorkspaceContainer_GetOwningDockHost(pWorkspaceContainer);
    BOOL bDockedInHost = pDockHostWindow ? TRUE : FALSE;
    BOOL bIsMainWorkspace = WorkspaceContainer_IsMainWorkspace(pWorkspaceContainer);

    if (!WorkspaceDockPolicy_ShouldAutoRemoveEmptyGroup(nDocs, bIsMainWorkspace, bDockedInHost))
    {
        return;
    }

    HWND hWndWorkspace = Window_GetHWND((Window*)pWorkspaceContainer);
    if (!hWndWorkspace || !IsWindow(hWndWorkspace))
    {
        return;
    }

    DockHostWindow_DestroyDockedHWND(pDockHostWindow, hWndWorkspace);
}

static void WorkspaceContainer_FinalizeEmptySourceAfterDetach(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return;
    }

    if (ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector) != 0)
    {
        return;
    }

    HWND hWndSourceWorkspace = Window_GetHWND((Window*)pWorkspaceContainer);
    if (!hWndSourceWorkspace || !IsWindow(hWndSourceWorkspace))
    {
        return;
    }

    HWND hWndSourceParent = GetParent(hWndSourceWorkspace);
    if (hWndSourceParent && IsWindow(hWndSourceParent))
    {
        WCHAR szParentClass[64] = L"";
        GetClassNameW(hWndSourceParent, szParentClass, ARRAYSIZE(szParentClass));
        if (wcscmp(szParentClass, L"__FloatingWindowContainer") == 0)
        {
            DestroyWindow(hWndSourceParent);
            return;
        }
    }

    WorkspaceContainer_TryRemoveEmptyDockedGroup(pWorkspaceContainer);
    hWndSourceWorkspace = Window_GetHWND((Window*)pWorkspaceContainer);
    if (!hWndSourceWorkspace || !IsWindow(hWndSourceWorkspace))
    {
        return;
    }

    if (WorkspaceContainer_IsFloating(pWorkspaceContainer))
    {
        DestroyWindow(hWndSourceWorkspace);
    }
}

static BOOL WorkspaceContainer_RestoreViewportAfterFloatFailure(
    WorkspaceContainer* pWorkspaceContainer,
    ViewportWindow* pViewportWindow,
    int nViewportIndex,
    ViewportWindow* pOriginalCurrentViewport)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector || !pViewportWindow)
    {
        return FALSE;
    }

    HWND hWndWorkspaceContainer = Window_GetHWND((Window*)pWorkspaceContainer);
    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    if (!hWndWorkspaceContainer || !IsWindow(hWndWorkspaceContainer) || !hWndViewport || !IsWindow(hWndViewport))
    {
        return FALSE;
    }

    SetParent(hWndViewport, hWndWorkspaceContainer);

    DWORD dwStyle = Window_GetStyle((Window*)pViewportWindow);
    dwStyle &= ~(WS_CAPTION | WS_THICKFRAME);
    dwStyle |= WS_CHILD;
    Window_SetStyle((Window*)pViewportWindow, dwStyle);
    SetWindowPos(
        hWndViewport,
        NULL,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    if (!ViewportVector_InsertAt(pWorkspaceContainer->m_pViewportVector, nViewportIndex, pViewportWindow))
    {
        return FALSE;
    }

    WorkspaceContainer_SetCurrentViewport(
        pWorkspaceContainer,
        pOriginalCurrentViewport ? pOriginalCurrentViewport : pViewportWindow,
        TRUE);
    return TRUE;
}

void WorkspaceContainer_UpdateWindowTitle(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer)
    {
        return;
    }

    WCHAR szTitle[MAX_PATH] = L"Document Group";
    if (pWorkspaceContainer->m_pViewportWindow)
    {
        int nActiveIndex = ViewportVector_FindIndex(
            pWorkspaceContainer->m_pViewportVector,
            pWorkspaceContainer->m_pViewportWindow);
        if (nActiveIndex < 0)
        {
            nActiveIndex = 0;
        }

        WorkspaceContainer_GetViewportTitle(
            pWorkspaceContainer->m_pViewportWindow,
            nActiveIndex,
            szTitle,
            ARRAYSIZE(szTitle));
    }

    SetWindowTextW(Window_GetHWND((Window*)pWorkspaceContainer), szTitle);
}

void WorkspaceContainer_GetViewportTitle(ViewportWindow* pViewportWindow, int nTabIndex, LPWSTR pszBuffer, size_t cchBuffer)
{
    if (!pszBuffer || cchBuffer == 0)
    {
        return;
    }

    StringCchPrintfW(pszBuffer, cchBuffer, L"Untitled %d", nTabIndex + 1);

    if (!pViewportWindow)
    {
        return;
    }

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    if (!pDocument || !pDocument->szFilePath || !pDocument->szFilePath[0])
    {
        return;
    }

    PCWSTR pszFileName = PathFindFileNameW(pDocument->szFilePath);
    if (pszFileName && pszFileName[0])
    {
        StringCchCopyW(pszBuffer, cchBuffer, pszFileName);
    }
}

int WorkspaceContainer_CalcTabWidth(WorkspaceContainer* pWorkspaceContainer, HDC hdc, ViewportWindow* pViewportWindow, int nTabIndex)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);

    WCHAR szTitle[MAX_PATH] = L"";
    WorkspaceContainer_GetViewportTitle(pViewportWindow, nTabIndex, szTitle, ARRAYSIZE(szTitle));

    SIZE textExtent = { 0 };
    GetTextExtentPoint32W(hdc, szTitle, (int)wcslen(szTitle), &textExtent);

    int width = textExtent.cx + WORKSPACE_TAB_TEXT_PADDING_X * 2 +
        WORKSPACE_TAB_CLOSE_GAP + WORKSPACE_TAB_CLOSE_SIZE;
    width = max(width, WORKSPACE_TAB_MIN_WIDTH);
    width = min(width, WORKSPACE_TAB_MAX_WIDTH);
    return width;
}

BOOL WorkspaceContainer_GetTabRect(WorkspaceContainer* pWorkspaceContainer, HDC hdc, int nTabIndex, LPRECT prcTab)
{
    if (!pWorkspaceContainer || !prcTab || !hdc || nTabIndex < 0)
    {
        return FALSE;
    }

    size_t nTabs = ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector);
    if (nTabs == 0 || (size_t)nTabIndex >= nTabs)
    {
        return FALSE;
    }

    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);

    RECT rcTabHeader = {
        rcClient.left,
        rcClient.top,
        rcClient.right,
        rcClient.top + WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer)
    };

    int x = rcTabHeader.left + WORKSPACE_TAB_GAP;
    for (int i = 0; i < (int)nTabs; i++)
    {
        ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, i);
        int tabWidth = WorkspaceContainer_CalcTabWidth(pWorkspaceContainer, hdc, pViewportWindow, i);

        RECT rcTab = {
            x,
            rcTabHeader.top + WORKSPACE_TAB_GAP,
            min(x + tabWidth, rcTabHeader.right - WORKSPACE_TAB_GAP),
            rcTabHeader.bottom
        };

        if (i == nTabIndex)
        {
            *prcTab = rcTab;
            return TRUE;
        }

        x += tabWidth + WORKSPACE_TAB_GAP;
        if (x >= rcTabHeader.right)
        {
            break;
        }
    }

    return FALSE;
}

BOOL WorkspaceContainer_GetTabCloseRect(WorkspaceContainer* pWorkspaceContainer, HDC hdc, int nTabIndex, LPRECT prcClose)
{
    if (!pWorkspaceContainer || !hdc || !prcClose)
    {
        return FALSE;
    }

    RECT rcTab = { 0 };
    if (!WorkspaceContainer_GetTabRect(pWorkspaceContainer, hdc, nTabIndex, &rcTab))
    {
        return FALSE;
    }

    int closeSize = min(WORKSPACE_TAB_CLOSE_SIZE, max(6, (rcTab.bottom - rcTab.top) - 6));
    int centerY = (rcTab.top + rcTab.bottom) / 2;

    prcClose->right = rcTab.right - WORKSPACE_TAB_GAP - 4;
    prcClose->left = prcClose->right - closeSize;
    prcClose->top = centerY - closeSize / 2;
    prcClose->bottom = prcClose->top + closeSize;

    if (prcClose->left < rcTab.left + WORKSPACE_TAB_TEXT_PADDING_X)
    {
        prcClose->left = rcTab.left + WORKSPACE_TAB_TEXT_PADDING_X;
        prcClose->right = prcClose->left + closeSize;
    }

    return TRUE;
}

int WorkspaceContainer_HitTestTab(WorkspaceContainer* pWorkspaceContainer, int x, int y)
{
    if (!pWorkspaceContainer)
    {
        return -1;
    }

    int headerHeight = WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer);
    if (headerHeight <= 0 || y < 0 || y >= headerHeight)
    {
        return -1;
    }

    HWND hWndWorkspaceContainer = Window_GetHWND((Window*)pWorkspaceContainer);
    HDC hdc = GetDC(hWndWorkspaceContainer);
    if (!hdc)
    {
        return -1;
    }

    HFONT hFont = PanitentApp_GetUIFont(PanitentApp_Instance());
    HGDIOBJ hPrevFont = SelectObject(hdc, hFont);

    int nHitTab = -1;
    for (int i = 0; i < (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector); i++)
    {
        RECT rcTab = { 0 };
        POINT pt = { x, y };
        if (WorkspaceContainer_GetTabRect(pWorkspaceContainer, hdc, i, &rcTab) &&
            PtInRect(&rcTab, pt))
        {
            nHitTab = i;
            break;
        }
    }

    SelectObject(hdc, hPrevFont);
    ReleaseDC(hWndWorkspaceContainer, hdc);
    return nHitTab;
}

int WorkspaceContainer_HitTestTabClose(WorkspaceContainer* pWorkspaceContainer, int x, int y)
{
    if (!pWorkspaceContainer)
    {
        return -1;
    }

    int headerHeight = WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer);
    if (headerHeight <= 0 || y < 0 || y >= headerHeight)
    {
        return -1;
    }

    HWND hWndWorkspaceContainer = Window_GetHWND((Window*)pWorkspaceContainer);
    HDC hdc = GetDC(hWndWorkspaceContainer);
    if (!hdc)
    {
        return -1;
    }

    HFONT hFont = PanitentApp_GetUIFont(PanitentApp_Instance());
    HGDIOBJ hPrevFont = SelectObject(hdc, hFont);

    int nHitTabClose = -1;
    for (int i = 0; i < (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector); i++)
    {
        RECT rcClose = { 0 };
        if (!WorkspaceContainer_GetTabCloseRect(pWorkspaceContainer, hdc, i, &rcClose))
        {
            continue;
        }

        InflateRect(&rcClose, 3, 3);
        POINT pt = { x, y };
        if (PtInRect(&rcClose, pt))
        {
            nHitTabClose = i;
            break;
        }
    }

    SelectObject(hdc, hPrevFont);
    ReleaseDC(hWndWorkspaceContainer, hdc);
    return nHitTabClose;
}

void WorkspaceContainer_CloseViewportAt(WorkspaceContainer* pWorkspaceContainer, int nTabIndex)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector || nTabIndex < 0)
    {
        return;
    }

    int nTabCount = (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector);
    if (nTabIndex >= nTabCount)
    {
        return;
    }

    ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, nTabIndex);
    if (!pViewportWindow)
    {
        return;
    }

    BOOL bWasCurrent = pWorkspaceContainer->m_pViewportWindow == pViewportWindow;
    if (!ViewportVector_RemoveAt(pWorkspaceContainer->m_pViewportVector, nTabIndex, NULL))
    {
        return;
    }

    if (bWasCurrent)
    {
        ViewportWindow* pNewCurrent = NULL;
        int nRemaining = (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector);
        if (nRemaining > 0)
        {
            int nNextIndex = min(nTabIndex, nRemaining - 1);
            pNewCurrent = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, nNextIndex);
        }

        WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pNewCurrent, TRUE);
    }
    else {
        WorkspaceContainer_LayoutViewports(pWorkspaceContainer);
        Window_Invalidate((Window*)pWorkspaceContainer);

        if (PanitentApp_GetActiveViewport(PanitentApp_Instance()) == pViewportWindow)
        {
            PanitentApp_SetActiveViewport(PanitentApp_Instance(), pWorkspaceContainer->m_pViewportWindow);
        }
    }

    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    if (hWndViewport && IsWindow(hWndViewport))
    {
        DestroyWindow(hWndViewport);
    }

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    if (pDocument)
    {
        Document_Purge(pDocument);
        Document_Destroy(pDocument);
    }

    WorkspaceContainer_UpdateWindowTitle(pWorkspaceContainer);
    Window_Invalidate((Window*)pWorkspaceContainer);
    WorkspaceContainer_FinalizeEmptySourceAfterDetach(pWorkspaceContainer);
}

void WorkspaceContainer_DrawSingleTab(WorkspaceContainer* pWorkspaceContainer, HDC hdc)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(hdc);
}

void WorkspaceContainer_DrawTabs(WorkspaceContainer* pWorkspaceContainer, HDC hdc)
{
    if (WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer) == 0)
    {
        return;
    }

    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);

    RECT rcTabHeader = {
        rcClient.left,
        rcClient.top,
        rcClient.right,
        rcClient.top + WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer)
    };

    PanitentThemeColors colors = { 0 };
    HBRUSH hHeaderBrush = NULL;
    PanitentTheme_GetColors(&colors);

    hHeaderBrush = CreateSolidBrush(colors.workspaceHeader);
    FillRect(hdc, &rcTabHeader, hHeaderBrush);
    DeleteObject(hHeaderBrush);

    HFONT hUIFont = PanitentApp_GetUIFont(PanitentApp_Instance());
    HGDIOBJ hPrevFont = SelectObject(hdc, hUIFont);
    SetBkMode(hdc, TRANSPARENT);

    for (int i = 0; i < (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector); i++)
    {
        RECT rcTab = { 0 };
        if (!WorkspaceContainer_GetTabRect(pWorkspaceContainer, hdc, i, &rcTab))
        {
            continue;
        }

        ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, i);
        BOOL bActive = pViewportWindow == pWorkspaceContainer->m_pViewportWindow;

        HBRUSH hTabBrush = CreateSolidBrush(
            bActive ? colors.accentActive : colors.accent);
        FillRect(hdc, &rcTab, hTabBrush);
        DeleteObject(hTabBrush);

        FrameRect(hdc, &rcTab, GetStockObject(BLACK_BRUSH));

        WCHAR szTitle[MAX_PATH] = L"";
        WorkspaceContainer_GetViewportTitle(pViewportWindow, i, szTitle, ARRAYSIZE(szTitle));

        RECT rcTabText = rcTab;
        rcTabText.left += WORKSPACE_TAB_TEXT_PADDING_X;
        rcTabText.right -= WORKSPACE_TAB_TEXT_PADDING_X + WORKSPACE_TAB_CLOSE_GAP + WORKSPACE_TAB_CLOSE_SIZE;

        SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
        DrawTextW(hdc, szTitle, -1, &rcTabText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);

        RECT rcClose = { 0 };
        if (WorkspaceContainer_GetTabCloseRect(pWorkspaceContainer, hdc, i, &rcClose))
        {
            HPEN hPenClose = CreatePen(PS_SOLID, 1, bActive ? RGB(0xFF, 0xFF, 0xFF) : RGB(0xE8, 0xE6, 0xF5));
            HGDIOBJ hOldPen = SelectObject(hdc, hPenClose);
            int x1 = rcClose.left;
            int y1 = rcClose.top;
            int x2 = rcClose.right - 1;
            int y2 = rcClose.bottom - 1;
            MoveToEx(hdc, x1, y1, NULL);
            LineTo(hdc, x2, y2);
            MoveToEx(hdc, x2, y1, NULL);
            LineTo(hdc, x1, y2);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPenClose);
        }
    }

    SelectObject(hdc, hPrevFont);
}

void WorkspaceContainer_OnPaint(WorkspaceContainer* pWorkspaceContainer)
{
    HWND hwnd = pWorkspaceContainer->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    PanitentThemeColors colors = { 0 };
    size_t nViewportCount = 0;

    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);
    PanitentTheme_GetColors(&colors);
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(hdc, colors.rootBackground);
    FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(DC_BRUSH));

    nViewportCount = pWorkspaceContainer && pWorkspaceContainer->m_pViewportVector ?
        ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector) : 0;
    if (nViewportCount == 0)
    {
        HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DOCKHOSTBG));
        if (hBitmap)
        {
            BITMAP bm = { 0 };
            GetObject(hBitmap, sizeof(BITMAP), &bm);
            if (bm.bmWidth > 0 && bm.bmHeight > 0)
            {
                RECT rcLogo = { 0 };
                SetRect(
                    &rcLogo,
                    rcClient.right - bm.bmWidth - WORKSPACE_EMPTY_WATERMARK_MARGIN,
                    rcClient.bottom - bm.bmHeight - WORKSPACE_EMPTY_WATERMARK_MARGIN,
                    rcClient.right - WORKSPACE_EMPTY_WATERMARK_MARGIN,
                    rcClient.bottom - WORKSPACE_EMPTY_WATERMARK_MARGIN);
                WorkspaceContainer_DrawMaskedBitmap(hdc, &rcLogo, IDB_DOCKHOSTBG, colors.accentActive);
            }
            DeleteObject(hBitmap);
        }
    }

    WorkspaceContainer_DrawTabs(pWorkspaceContainer, hdc);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void WorkspaceContainer_OnLButtonDown(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    int nHitTabClose = WorkspaceContainer_HitTestTabClose(pWorkspaceContainer, x, y);
    if (nHitTabClose >= 0)
    {
        pWorkspaceContainer->m_iPressedCloseTabIndex = nHitTabClose;
        pWorkspaceContainer->m_iPressedTabIndex = -1;
        SetCapture(Window_GetHWND((Window*)pWorkspaceContainer));
        return;
    }

    int nHitTab = WorkspaceContainer_HitTestTab(pWorkspaceContainer, x, y);
    if (nHitTab < 0)
    {
        pWorkspaceContainer->m_iPressedTabIndex = -1;
        pWorkspaceContainer->m_iPressedCloseTabIndex = -1;
        return;
    }

    pWorkspaceContainer->m_iPressedTabIndex = nHitTab;
    pWorkspaceContainer->m_iPressedCloseTabIndex = -1;
    pWorkspaceContainer->m_ptTabDragStart.x = x;
    pWorkspaceContainer->m_ptTabDragStart.y = y;

    SetCapture(Window_GetHWND((Window*)pWorkspaceContainer));
}

void WorkspaceContainer_OnMouseMove(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    if (pWorkspaceContainer->m_iPressedCloseTabIndex >= 0)
    {
        return;
    }

    if (pWorkspaceContainer->m_iPressedTabIndex < 0 || !(keyFlags & MK_LBUTTON))
    {
        return;
    }

    int dx = x - pWorkspaceContainer->m_ptTabDragStart.x;
    int dy = y - pWorkspaceContainer->m_ptTabDragStart.y;
    if (dx < 0)
    {
        dx = -dx;
    }
    if (dy < 0)
    {
        dy = -dy;
    }

    if (dx < WORKSPACE_TAB_FLOAT_DISTANCE && dy < WORKSPACE_TAB_FLOAT_DISTANCE)
    {
        return;
    }

    ViewportWindow* pViewportWindow = ViewportVector_Get(
        pWorkspaceContainer->m_pViewportVector,
        pWorkspaceContainer->m_iPressedTabIndex);
    pWorkspaceContainer->m_iPressedTabIndex = -1;

    if (!pViewportWindow)
    {
        return;
    }

    POINT ptScreen = { x, y };
    ClientToScreen(Window_GetHWND((Window*)pWorkspaceContainer), &ptScreen);
    WorkspaceContainer_FloatViewport(pWorkspaceContainer, pViewportWindow, ptScreen.x, ptScreen.y, TRUE);
}

void WorkspaceContainer_OnLButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    int nPressedCloseTabIndex = pWorkspaceContainer->m_iPressedCloseTabIndex;
    int nPressedTabIndex = pWorkspaceContainer->m_iPressedTabIndex;

    if (GetCapture() == Window_GetHWND((Window*)pWorkspaceContainer))
    {
        ReleaseCapture();
    }

    pWorkspaceContainer->m_iPressedCloseTabIndex = -1;
    pWorkspaceContainer->m_iPressedTabIndex = -1;

    if (nPressedCloseTabIndex >= 0)
    {
        int nHitCloseTabIndex = WorkspaceContainer_HitTestTabClose(pWorkspaceContainer, x, y);
        if (nHitCloseTabIndex == nPressedCloseTabIndex)
        {
            WorkspaceContainer_CloseViewportAt(pWorkspaceContainer, nPressedCloseTabIndex);
        }
        return;
    }

    if (nPressedTabIndex < 0)
    {
        return;
    }

    int nHitTab = WorkspaceContainer_HitTestTab(pWorkspaceContainer, x, y);
    if (nHitTab < 0)
    {
        return;
    }

    ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, nHitTab);
    if (!pViewportWindow)
    {
        return;
    }

    WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pViewportWindow, TRUE);
}

void WorkspaceContainer_OnRButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void WorkspaceContainer_OnContextMenu(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    HWND hWndWorkspace = Window_GetHWND((Window*)pWorkspaceContainer);
    POINT ptScreen = { x, y };
    if (ptScreen.x == -1 && ptScreen.y == -1)
    {
        GetCursorPos(&ptScreen);
    }

    POINT ptClient = ptScreen;
    ScreenToClient(hWndWorkspace, &ptClient);

    int nHitTab = WorkspaceContainer_HitTestTab(pWorkspaceContainer, ptClient.x, ptClient.y);
    if (nHitTab < 0)
    {
        return;
    }

    ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, nHitTab);
    if (!pViewportWindow)
    {
        return;
    }

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_WORKSPACE_TAB_FLOAT, L"Move To New Window");

    int cmd = TrackPopupMenu(
        hMenu,
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        ptScreen.x,
        ptScreen.y,
        0,
        hWndWorkspace,
        NULL);
    DestroyMenu(hMenu);

    if (cmd == IDM_WORKSPACE_TAB_FLOAT)
    {
        WorkspaceContainer_FloatViewport(pWorkspaceContainer, pViewportWindow, ptScreen.x, ptScreen.y, FALSE);
    }
}

void WorkspaceContainer_OnCommand(WorkspaceContainer* pWorkspaceContainer, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
}

void WorkspaceContainer_OnDestroy(WorkspaceContainer* pWorkspaceContainer)
{
    if (pWorkspaceContainer && pWorkspaceContainer->pFileDropTarget)
    {
        OleFileDropTarget_Revoke(
            Window_GetHWND((Window*)pWorkspaceContainer),
            (IDropTarget**)&pWorkspaceContainer->pFileDropTarget);
    }
}

static BOOL WorkspaceContainer_OnDropFiles(void* pContext, HDROP hDrop)
{
    UNREFERENCED_PARAMETER(pContext);
    return PanitentApp_OpenDroppedFiles(PanitentApp_Instance(), hDrop);
}

LRESULT CALLBACK WorkspaceContainer_UserProc(WorkspaceContainer* pWorkspaceContainer, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_LBUTTONDOWN:
        WorkspaceContainer_OnLButtonDown(pWorkspaceContainer, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        return TRUE;
        break;

    case WM_MOUSEMOVE:
        WorkspaceContainer_OnMouseMove(pWorkspaceContainer, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        break;

    case WM_RBUTTONUP:
        WorkspaceContainer_OnRButtonUp(pWorkspaceContainer, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        break;

    case WM_LBUTTONUP:
        WorkspaceContainer_OnLButtonUp(pWorkspaceContainer, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        return TRUE;
        break;

    case WM_CAPTURECHANGED:
        pWorkspaceContainer->m_iPressedTabIndex = -1;
        pWorkspaceContainer->m_iPressedCloseTabIndex = -1;
        break;

    case WM_EXITSIZEMOVE:
        WorkspaceContainer_TryDockFloating(pWorkspaceContainer, FALSE);
        break;

    case WM_CLOSE:
        if (WorkspaceContainer_TryDockFloating(pWorkspaceContainer, TRUE))
        {
            return 0;
        }
        break;

    case WM_CONTEXTMENU:
        WorkspaceContainer_OnContextMenu(
            pWorkspaceContainer,
            (int)(short)LOWORD(lParam),
            (int)(short)HIWORD(lParam),
            (UINT)wParam);
        break;

    case WM_ERASEBKGND:
        return 1;
    }

    return Window_UserProcDefault((Window*)pWorkspaceContainer, hWnd, message, wParam, lParam);
}

void WorkspaceContainer_AddViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector || !pViewportWindow)
    {
        return;
    }

    int nExistingIndex = ViewportVector_FindIndex(pWorkspaceContainer->m_pViewportVector, pViewportWindow);
    if (nExistingIndex >= 0)
    {
        WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pViewportWindow, TRUE);
        return;
    }

    HWND hWndWorkspaceContainer = Window_GetHWND((Window*)pWorkspaceContainer);
    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);

    /* Set viewport parent to workspace container */
    if (IsWindow(hWndViewport))
    {
        SetParent(hWndViewport, hWndWorkspaceContainer);
    }

    /* Set child window style for viewport window */
    DWORD dwStyle = Window_GetStyle((Window*)pViewportWindow);
    dwStyle &= ~(WS_CAPTION | WS_THICKFRAME);
    dwStyle |= WS_CHILD;
    Window_SetStyle((Window*)pViewportWindow, dwStyle);
    SetWindowPos(hWndViewport, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    /* Add viewport to viewports list */
    if (!ViewportVector_Add(pWorkspaceContainer->m_pViewportVector, pViewportWindow))
    {
        return;
    }

    /* Activate new viewport tab */
    WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pViewportWindow, TRUE);
}

ViewportWindow* WorkspaceContainer_GetCurrentViewport(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer)
    {
        return NULL;
    }

    return pWorkspaceContainer->m_pViewportWindow;
}

BOOL WorkspaceContainer_DetachViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector || !pViewportWindow)
    {
        return FALSE;
    }

    int nViewportIndex = ViewportVector_FindIndex(pWorkspaceContainer->m_pViewportVector, pViewportWindow);
    if (nViewportIndex < 0)
    {
        return FALSE;
    }

    if (!ViewportVector_RemoveAt(pWorkspaceContainer->m_pViewportVector, nViewportIndex, NULL))
    {
        return FALSE;
    }

    BOOL bWasCurrent = pWorkspaceContainer->m_pViewportWindow == pViewportWindow;
    if (bWasCurrent)
    {
        ViewportWindow* pNewCurrent = NULL;
        int nTabs = (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector);
        if (nTabs > 0)
        {
            int nNextIndex = min(nViewportIndex, nTabs - 1);
            pNewCurrent = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, nNextIndex);
        }

        WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pNewCurrent, TRUE);
    }
    else {
        WorkspaceContainer_LayoutViewports(pWorkspaceContainer);
        Window_Invalidate((Window*)pWorkspaceContainer);

        if (PanitentApp_GetActiveViewport(PanitentApp_Instance()) == pViewportWindow)
        {
            PanitentApp_SetActiveViewport(PanitentApp_Instance(), pWorkspaceContainer->m_pViewportWindow);
        }
        WorkspaceContainer_UpdateWindowTitle(pWorkspaceContainer);
    }

    return TRUE;
}

void WorkspaceContainer_ClearAllViewports(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return;
    }

    while (WorkspaceContainer_GetViewportCount(pWorkspaceContainer) > 0)
    {
        WorkspaceContainer_CloseViewportAt(pWorkspaceContainer, 0);
    }
}

ViewportWindow* WorkspaceContainer_GetViewportAt(WorkspaceContainer* pWorkspaceContainer, int index)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return NULL;
    }

    return ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, index);
}

int WorkspaceContainer_GetViewportCount(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return 0;
    }

    return (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector);
}

void WorkspaceContainer_FloatViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow, int xScreen, int yScreen, BOOL bStartMove)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector || !pViewportWindow)
    {
        return;
    }

    HWND hWndSourceWorkspace = Window_GetHWND((Window*)pWorkspaceContainer);
    ViewportWindow* pOriginalCurrentViewport = pWorkspaceContainer->m_pViewportWindow;

    int nViewportIndex = ViewportVector_FindIndex(pWorkspaceContainer->m_pViewportVector, pViewportWindow);
    if (nViewportIndex < 0)
    {
        return;
    }

    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
    if (!IsWindow(hWndViewport))
    {
        return;
    }

    RECT rcViewport = { 0 };
    GetWindowRect(hWndViewport, &rcViewport);

    if (!ViewportVector_RemoveAt(pWorkspaceContainer->m_pViewportVector, nViewportIndex, NULL))
    {
        return;
    }

    BOOL bWasCurrent = pWorkspaceContainer->m_pViewportWindow == pViewportWindow;
    if (bWasCurrent)
    {
        ViewportWindow* pNewCurrent = NULL;
        int nTabs = (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector);
        if (nTabs > 0)
        {
            int nNextIndex = min(nViewportIndex, nTabs - 1);
            pNewCurrent = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, nNextIndex);
        }

        WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pNewCurrent, TRUE);
    }
    else {
        WorkspaceContainer_LayoutViewports(pWorkspaceContainer);
        Window_Invalidate((Window*)pWorkspaceContainer);

        if (PanitentApp_GetActiveViewport(PanitentApp_Instance()) == pViewportWindow)
        {
            PanitentApp_SetActiveViewport(PanitentApp_Instance(), pWorkspaceContainer->m_pViewportWindow);
        }
    }

    PanitentWindow* pPanitentWindow = PanitentApp_GetWindow(PanitentApp_Instance());
    DockHostWindow* pDockHostWindow = pPanitentWindow ? pPanitentWindow->m_pDockHostWindow : NULL;

    WorkspaceContainer* pFloatingWorkspace = WorkspaceContainer_Create();
    if (!pFloatingWorkspace)
    {
        WorkspaceContainer_RestoreViewportAfterFloatFailure(
            pWorkspaceContainer,
            pViewportWindow,
            nViewportIndex,
            pOriginalCurrentViewport);
        return;
    }

    HWND hWndFloatingWorkspace = Window_CreateWindow((Window*)pFloatingWorkspace, NULL);
    if (!hWndFloatingWorkspace || !IsWindow(hWndFloatingWorkspace))
    {
        WorkspaceContainer_RestoreViewportAfterFloatFailure(
            pWorkspaceContainer,
            pViewportWindow,
            nViewportIndex,
            pOriginalCurrentViewport);
        free(pFloatingWorkspace);
        return;
    }

    WorkspaceContainer_AddViewport(pFloatingWorkspace, pViewportWindow);
    WorkspaceContainer_UpdateWindowTitle(pFloatingWorkspace);

    int width = max(320, rcViewport.right - rcViewport.left);
    int height = max(240, rcViewport.bottom - rcViewport.top);
    int x = rcViewport.left;
    int y = rcViewport.top;

    if (xScreen >= 0 && yScreen >= 0)
    {
        x = xScreen - width / 2;
        y = yScreen - 10;
    }

    RECT rcFloatingWindow = { x, y, x + width, y + height };
    HWND hWndFloating = NULL;
    if (!FloatingDocumentHost_CreatePinnedWindow(
        pDockHostWindow,
        hWndFloatingWorkspace,
        &rcFloatingWindow,
        FALSE,
        (POINT){ xScreen, yScreen },
        &hWndFloating))
    {
        WorkspaceContainer_RestoreViewportAfterFloatFailure(
            pWorkspaceContainer,
            pViewportWindow,
            nViewportIndex,
            pOriginalCurrentViewport);
        DestroyWindow(hWndFloatingWorkspace);
        return;
    }

    /* Remove empty source document group before starting modal move loop. */
    WorkspaceContainer_FinalizeEmptySourceAfterDetach(pWorkspaceContainer);

    if (bStartMove)
    {
        if (hWndSourceWorkspace && GetCapture() == hWndSourceWorkspace)
        {
            ReleaseCapture();
        }

        SetForegroundWindow(hWndFloating);
        SendMessage(hWndFloating, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(xScreen, yScreen));
    }
}

void WorkspaceContainer_MoveAllViewportsTo(WorkspaceContainer* pSourceWorkspace, WorkspaceContainer* pTargetWorkspace)
{
    if (!pSourceWorkspace || !pTargetWorkspace || pSourceWorkspace == pTargetWorkspace)
    {
        return;
    }

    ViewportWindow* pActiveSourceViewport = pSourceWorkspace->m_pViewportWindow;

    while (ViewportVector_GetSize(pSourceWorkspace->m_pViewportVector) > 0)
    {
        ViewportWindow* pViewportWindow = ViewportVector_Get(pSourceWorkspace->m_pViewportVector, 0);
        if (!pViewportWindow)
        {
            break;
        }

        ViewportVector_RemoveAt(pSourceWorkspace->m_pViewportVector, 0, NULL);
        WorkspaceContainer_AddViewport(pTargetWorkspace, pViewportWindow);
    }

    pSourceWorkspace->m_pViewportWindow = NULL;
    WorkspaceContainer_UpdateWindowTitle(pSourceWorkspace);
    WorkspaceContainer_LayoutViewports(pSourceWorkspace);
    Window_Invalidate((Window*)pSourceWorkspace);

    if (pActiveSourceViewport)
    {
        WorkspaceContainer_SetCurrentViewport(pTargetWorkspace, pActiveSourceViewport, TRUE);
    }

    WorkspaceContainer_TryRemoveEmptyDockedGroup(pSourceWorkspace);
}

WorkspaceContainer* WorkspaceContainer_FindDropTargetAtScreenPoint(WorkspaceContainer* pSourceWorkspace, POINT ptScreen)
{
    if (!pSourceWorkspace)
    {
        return NULL;
    }

    WorkspaceDropSearchContext ctx = { 0 };
    ctx.pSourceWorkspace = pSourceWorkspace;
    ctx.ptScreen = ptScreen;
    ctx.pFoundWorkspace = NULL;

    HWND hWndSourceWorkspace = Window_GetHWND((Window*)pSourceWorkspace);
    if (hWndSourceWorkspace && IsWindow(hWndSourceWorkspace))
    {
        ctx.hWndExcludeRoot = GetAncestor(hWndSourceWorkspace, GA_ROOT);
    }

    EnumWindows(WorkspaceContainer_EnumWindows_FindWorkspaceAtPoint, (LPARAM)&ctx);
    return ctx.pFoundWorkspace;
}

BOOL CALLBACK WorkspaceContainer_EnumWindows_FindWorkspaceAtPoint(HWND hWnd, LPARAM lParam)
{
    WorkspaceDropSearchContext* pCtx = (WorkspaceDropSearchContext*)lParam;
    if (!pCtx || !hWnd || !IsWindow(hWnd))
    {
        return TRUE;
    }

    if (pCtx->pFoundWorkspace)
    {
        return FALSE;
    }

    if (hWnd == pCtx->hWndExcludeRoot || !IsWindowVisible(hWnd))
    {
        return TRUE;
    }

    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);
    if (!PtInRect(&rcWindow, pCtx->ptScreen))
    {
        return TRUE;
    }

    EnumChildWindows(hWnd, WorkspaceContainer_EnumChildWindows_FindWorkspaceAtPoint, lParam);
    if (pCtx->pFoundWorkspace)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CALLBACK WorkspaceContainer_EnumChildWindows_FindWorkspaceAtPoint(HWND hWnd, LPARAM lParam)
{
    WorkspaceDropSearchContext* pCtx = (WorkspaceDropSearchContext*)lParam;
    if (!pCtx || !hWnd || !IsWindow(hWnd))
    {
        return TRUE;
    }

    if (pCtx->pFoundWorkspace)
    {
        return FALSE;
    }

    if (!IsWindowVisible(hWnd))
    {
        return TRUE;
    }

    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);
    if (!PtInRect(&rcWindow, pCtx->ptScreen))
    {
        return TRUE;
    }

    WCHAR szWindowClass[64] = L"";
    GetClassNameW(hWnd, szWindowClass, ARRAYSIZE(szWindowClass));
    if (wcscmp(szWindowClass, szClassName) != 0)
    {
        return TRUE;
    }

    Window* pWindow = WindowMap_Get(hWnd);
    if (!pWindow)
    {
        return TRUE;
    }

    WorkspaceContainer* pCandidateWorkspace = (WorkspaceContainer*)pWindow;
    if (pCandidateWorkspace == pCtx->pSourceWorkspace)
    {
        return TRUE;
    }

    pCtx->pFoundWorkspace = pCandidateWorkspace;
    return FALSE;
}

BOOL WorkspaceContainer_TryDockFloating(WorkspaceContainer* pSourceWorkspace, BOOL bForceMainWorkspace)
{
    if (!pSourceWorkspace || !WorkspaceContainer_IsFloating(pSourceWorkspace))
    {
        return FALSE;
    }

    WorkspaceContainer* pTargetWorkspace = NULL;
    if (bForceMainWorkspace)
    {
        pTargetWorkspace = PanitentApp_GetWorkspaceContainer(PanitentApp_Instance());
    }
    else {
        POINT ptCursor = { 0 };
        if (!GetCursorPos(&ptCursor))
        {
            return FALSE;
        }
        pTargetWorkspace = WorkspaceContainer_FindDropTargetAtScreenPoint(pSourceWorkspace, ptCursor);
    }

    if (!pTargetWorkspace || pTargetWorkspace == pSourceWorkspace)
    {
        return FALSE;
    }

    HWND hWndSourceWorkspace = Window_GetHWND((Window*)pSourceWorkspace);
    HWND hWndSourceChild = hWndSourceWorkspace;
    if (!DocumentDockTransition_DockSourceToWorkspace(
        hWndSourceWorkspace,
        &hWndSourceChild,
        pTargetWorkspace,
        DKS_CENTER,
        0))
    {
        return FALSE;
    }

    DestroyWindow(hWndSourceWorkspace);
    return TRUE;
}

BOOL ViewportVector_Reserve(ViewportVector* pViewportVector, size_t newCapacity)
{
    if (!pViewportVector)
    {
        return FALSE;
    }

    if (newCapacity <= pViewportVector->m_capacity)
    {
        return TRUE;
    }

    ViewportWindow** pNewData = (ViewportWindow**)realloc(
        pViewportVector->pData, newCapacity * sizeof(ViewportWindow*));
    if (!pNewData)
    {
        return FALSE;
    }

    memset(
        pNewData + pViewportVector->m_capacity,
        0,
        (newCapacity - pViewportVector->m_capacity) * sizeof(ViewportWindow*));

    pViewportVector->pData = pNewData;
    pViewportVector->m_capacity = newCapacity;
    return TRUE;
}

size_t ViewportVector_GetSize(ViewportVector* pViewportVector)
{
    if (!pViewportVector)
    {
        return 0;
    }

    return pViewportVector->m_size;
}

BOOL ViewportVector_Add(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow)
{
    if (!pViewportVector || !pViewportWindow)
    {
        return FALSE;
    }

    if (pViewportVector->m_size >= pViewportVector->m_capacity)
    {
        size_t newCapacity = pViewportVector->m_capacity > 0 ?
            pViewportVector->m_capacity * 2 : 10;
        if (!ViewportVector_Reserve(pViewportVector, newCapacity))
        {
            return FALSE;
        }
    }

    pViewportVector->pData[pViewportVector->m_size++] = pViewportWindow;
    return TRUE;
}

BOOL ViewportVector_InsertAt(ViewportVector* pViewportVector, int idx, ViewportWindow* pViewportWindow)
{
    if (!pViewportVector || !pViewportWindow)
    {
        return FALSE;
    }

    if (idx < 0)
    {
        idx = 0;
    }

    if ((size_t)idx > pViewportVector->m_size)
    {
        idx = (int)pViewportVector->m_size;
    }

    if (pViewportVector->m_size >= pViewportVector->m_capacity)
    {
        size_t newCapacity = pViewportVector->m_capacity > 0 ?
            pViewportVector->m_capacity * 2 : 10;
        if (!ViewportVector_Reserve(pViewportVector, newCapacity))
        {
            return FALSE;
        }
    }

    for (size_t i = pViewportVector->m_size; i > (size_t)idx; --i)
    {
        pViewportVector->pData[i] = pViewportVector->pData[i - 1];
    }

    pViewportVector->pData[idx] = pViewportWindow;
    pViewportVector->m_size++;
    return TRUE;
}

BOOL ViewportVector_RemoveAt(ViewportVector* pViewportVector, int idx, ViewportWindow** ppViewportWindow)
{
    if (!pViewportVector || idx < 0 || (size_t)idx >= pViewportVector->m_size)
    {
        return FALSE;
    }

    if (ppViewportWindow)
    {
        *ppViewportWindow = pViewportVector->pData[idx];
    }

    for (size_t i = (size_t)idx; i + 1 < pViewportVector->m_size; i++)
    {
        pViewportVector->pData[i] = pViewportVector->pData[i + 1];
    }

    pViewportVector->m_size--;
    pViewportVector->pData[pViewportVector->m_size] = NULL;
    return TRUE;
}

ViewportWindow* ViewportVector_Get(ViewportVector* pViewportVector, int idx)
{
    if (!pViewportVector || idx < 0 || (size_t)idx >= pViewportVector->m_size)
    {
        return NULL;
    }

    return pViewportVector->pData[idx];
}

int ViewportVector_FindIndex(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow)
{
    if (!pViewportVector || !pViewportWindow)
    {
        return -1;
    }

    for (int i = 0; i < (int)pViewportVector->m_size; i++)
    {
        if (pViewportVector->pData[i] == pViewportWindow)
        {
            return i;
        }
    }

    return -1;
}
