#include "precomp.h"

#include "dockhostruntime.h"

#include "dockgroup.h"
#include "theme.h"

void DockHostWindow_RefreshTheme(DockHostWindow* pDockHostWindow)
{
    PanitentThemeColors colors = { 0 };
    HWND hWnd = NULL;

    if (!pDockHostWindow)
    {
        return;
    }

    PanitentTheme_GetColors(&colors);
    if (pDockHostWindow->hCaptionBrush_)
    {
        DeleteObject(pDockHostWindow->hCaptionBrush_);
    }
    pDockHostWindow->hCaptionBrush_ = CreateSolidBrush(colors.accent);

    hWnd = Window_GetHWND((Window*)pDockHostWindow);
    if (hWnd && IsWindow(hWnd))
    {
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
    {
        RedrawWindow(
            pDockHostWindow->hWndAutoHideOverlayHost,
            NULL,
            NULL,
            RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
}
