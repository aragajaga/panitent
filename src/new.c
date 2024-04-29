#include "precomp.h"
#include <windowsx.h>
#include <commctrl.h>

#include <shlwapi.h>
#include <stdio.h>
#include <assert.h>
#include <strsafe.h>

#include "new.h"
#include "debug.h"
#include "canvas.h"
#include "document.h"
#include "viewport.h"
#include "color_context.h"
#include "resource.h"
#include "palette.h"
#include "panitent.h"
#include "checker.h"
#include "util.h"
#include "sharing/activitysharingmanager.h"

#include "win32/util.h"

void RegisterNewFileDialog()
{
    WNDCLASS wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC)NewFileDialogWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"NewFileDialogClass";
    if (!RegisterClass(&wc))
        printf("[NewFileDialog] Class registration failed\n");
}

HWND hEditWidth;
HWND hEditHeight;
HWND hComboBackground;
extern HWND hButtonOk;

LRESULT CALLBACK NewFileDialogWndProc(HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDB_OK:
        {
            WCHAR szWidth[40];
            WCHAR szHeight[40];
            int iWidth;
            int iHeight;

            GetWindowText(hEditWidth, szWidth, 40);
            GetWindowText(hEditHeight, szHeight, 40);

            COLORREF bgColor = 0x00000000;
            int bgIndex = ComboBox_GetCurSel(hComboBackground);

            switch (bgIndex)
            {
            case 0:
                bgColor = 0x00000000;
                break;
            case 1:
                bgColor = 0xFFFFFFFF;
                break;
            case 2:
                bgColor = 0xFF000000;
                break;
            case 3:
                bgColor = g_color_context.fg_color;
                break;
            case 4:
                bgColor = g_color_context.bg_color;
                break;
            }

            iWidth = StrToInt(szWidth);
            iHeight = StrToInt(szHeight);

            printf("[NewFile] width: %d, AVLNode_Height: %d\n", iWidth, iHeight);

            DestroyWindow(hwnd);
            Document* pDocument = Document_New(iWidth, iHeight);
            Canvas* pCanvas = Document_GetCanvas(pDocument);

            if (pDocument && Document_GetCanvas(pDocument))
            {
                Canvas_FillSolid(Document_GetCanvas(pDocument), bgColor);
            }
        } break;
        default:
            break;
        }
        break;
    case WM_SIZE:
    {
        WORD cx = LOWORD(lParam);
        WORD cy = HIWORD(lParam);
        SetWindowPos(hButtonOk, NULL, cx - 110, cy - 40, 100, 30, SWP_NOZORDER);
    } break;
    case WM_CREATE:
    {
        HWND hStaticWidth = CreateWindow(L"STATIC",
            L"Width:",
            WS_CHILD | WS_VISIBLE,
            10,
            15,
            70,
            23,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        Win32_ApplyUIFont(hStaticWidth);

        hEditWidth = CreateWindowEx(WS_EX_CLIENTEDGE,
            L"EDIT",
            L"640",
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            100,
            10,
            100,
            23,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        Win32_ApplyUIFont(hEditWidth);

        HWND hStaticHeight = CreateWindow(L"STATIC",
            L"Height:",
            WS_CHILD | WS_VISIBLE,
            10,
            42,
            70,
            23,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        Win32_ApplyUIFont(hStaticHeight);

        hEditHeight = CreateWindowEx(WS_EX_CLIENTEDGE,
            L"EDIT",
            L"480",
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            100,
            38,
            100,
            23,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        Win32_ApplyUIFont(hEditHeight);

        HWND hStaticBackground = CreateWindow(L"STATIC", L"Background:",
            WS_CHILD | WS_VISIBLE,
            10, 69, 70, 23,
            hwnd, NULL, GetModuleHandle(NULL), NULL);
        Win32_ApplyUIFont(hStaticBackground);

        hComboBackground = CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX,
            L"",
            CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED |
            WS_VISIBLE,
            100, 66, 100, 23,
            hwnd, NULL, GetModuleHandle(NULL), NULL);
        Win32_ApplyUIFont(hComboBackground);
        ComboBox_AddString(hComboBackground, L"Transparent");
        ComboBox_AddString(hComboBackground, L"White");
        ComboBox_AddString(hComboBackground, L"Black");
        ComboBox_AddString(hComboBackground, L"Primary color");
        ComboBox_AddString(hComboBackground, L"Secondary color");
        ComboBox_SetCurSel(hComboBackground, 1);

        hButtonOk = CreateWindow(L"BUTTON",
            L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            200,
            250,
            100,
            30,
            hwnd,
            (HMENU)IDB_OK,
            GetModuleHandle(NULL),
            NULL);
        Win32_ApplyUIFont(hButtonOk);
    } break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }

    return 0;
}

typedef struct _DocumentPreset {
    LPCWSTR szName;
    int width;
    int height;
} DocumentPreset;

DocumentPreset pDocPreset[] = {
  {L"Custom", 640, 480},
  {L"Generic social media post", 640, 480},
  {L"VK Post", 640, 480},
  {L"Rage comics", 480, 1000},
  {L"128px squared asset (icon, game tile, etc.)", 128, 128},
  {L"96px squared asset (icon, game tile, etc.)", 72, 72},
  {L"72px squared asset (icon, game tile, etc.)", 72, 72},
  {L"64px squared asset (icon, game tile, etc.)", 64, 64},
  {L"48px squared asset (icon, game tile, etc.)", 48, 48},
  {L"32px squared asset (icon, game tile, etc.)", 32, 32},
  {L"24px squared asset (icon, game tile, etc.)", 24, 24},
  {L"16px squared asset (icon, game tile, etc.)", 16, 16},
  {L"SD 16:9", 720, 576},
  {L"HD 720p 16:9", 1280, 720},
  {L"FullHD 1080p 16:9", 1920, 1080}
};

typedef enum {
    COLOR_VALUE,
    COLOR_PRIMARY,
    COLOR_SECONDARY,
} ColorItemType;

typedef struct _BkgndColorItem {
    ColorItemType type;
    uint32_t value;
    LPCWSTR szName;
} BkgndColorItem;

BkgndColorItem pBkgndColors[] = {
  {COLOR_PRIMARY, 0, L"Primary color"},
  {COLOR_SECONDARY, 0, L"Secondary color"},
  {COLOR_VALUE, 0x00000000, L"Transparent"},
  {COLOR_VALUE, 0xFFFFFFFF, L"White"},
  {COLOR_VALUE, 0xFF000000, L"Black"},
  {COLOR_VALUE, 0xFF0000FF, L"Red"},
  {COLOR_VALUE, 0xFF00FFFF, L"Yellow"},
  {COLOR_VALUE, 0xFF00FF00, L"Green"},
  {COLOR_VALUE, 0xFFFFFF00, L"Cyan"},
  {COLOR_VALUE, 0xFFFF0000, L"Blue"},
  {COLOR_VALUE, 0xFFFF00FF, L"Magenta"},
};

HBRUSH g_hbrChecker;

inline static void ContractRect(LPRECT lprc, int i)
{
    lprc->left += i;
    lprc->top += i;
    lprc->right -= i;
    lprc->bottom -= i;
}

typedef struct _NewDocumentDlgData {
    CheckerConfig checkerCfg;
    HBRUSH hbrChecker;
} NewDocumentDlgData;

INT_PTR CALLBACK NewDocumentDlgProc(HWND hwndDlg, UINT message, WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        NewDocumentDlgData* data = (NewDocumentDlgData*)malloc(sizeof(NewDocumentDlgData));
        memset(data, 0, sizeof(NewDocumentDlgData));
        if (!data)
        {
            return TRUE;
        }

        ZeroMemory(data, sizeof(NewDocumentDlgData));
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)data);

        data->checkerCfg.tileSize = 8;
        data->checkerCfg.color[0] = 0x00CCCCCC;
        data->checkerCfg.color[1] = 0x00FFFFFF;

        HWND hwndColorSel = GetDlgItem(hwndDlg, IDC_DOCUMENTBKGND);
        HDC hdcColorSel = GetDC(hwndColorSel);
        data->hbrChecker = CreateChecker(&data->checkerCfg, hdcColorSel);
        ReleaseDC(hwndColorSel, hdcColorSel);

        for (size_t i = 0; i < sizeof(pDocPreset) / sizeof(DocumentPreset); i++)
        {
            int nItem;

            nItem = (int)SendDlgItemMessage(hwndDlg, IDC_DOCUMENTPRESET, CB_ADDSTRING, 0, (LPARAM)pDocPreset[i].szName);
            SendDlgItemMessage(hwndDlg, IDC_DOCUMENTPRESET, CB_SETITEMDATA, nItem, (LPARAM)&pDocPreset[i]);
        }

        for (size_t i = 0; i < sizeof(pBkgndColors) / sizeof(BkgndColorItem); i++)
        {
            int nItem;

            nItem = (int)SendDlgItemMessage(hwndDlg, IDC_DOCUMENTBKGND, CB_ADDSTRING, 0, (LPARAM)pBkgndColors[i].szName);
            SendDlgItemMessage(hwndDlg, IDC_DOCUMENTBKGND, CB_SETITEMDATA, nItem, (LPARAM)&pBkgndColors[i]);
        }

        SendDlgItemMessage(hwndDlg, IDC_DOCUMENTBKGND, CB_SETCURSEL,
            (LPARAM)3, (LPARAM)0);


        SetDlgItemInt(hwndDlg, IDC_DOCUMENTWIDTH, 640, FALSE);
        SetDlgItemInt(hwndDlg, IDC_DOCUMENTHEIGHT, 480, FALSE);
    }

    return TRUE;
    case WM_DESTROY:
    {
        NewDocumentDlgData* data = (NewDocumentDlgData*)
            GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        free(data);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_DOCUMENTPRESET:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
            {
                DocumentPreset* pPreset;
                int nItem;

                nItem = (int)SendDlgItemMessage(hwndDlg, IDC_DOCUMENTPRESET, CB_GETCURSEL, 0, 0);
                pPreset = (DocumentPreset*)SendDlgItemMessage(hwndDlg, IDC_DOCUMENTPRESET, CB_GETITEMDATA, (WPARAM)nItem, 0);
                if (pPreset)
                {
                    SetDlgItemInt(hwndDlg, IDC_DOCUMENTWIDTH, pPreset->width, FALSE);
                    SetDlgItemInt(hwndDlg, IDC_DOCUMENTHEIGHT, pPreset->height, FALSE);
                }
            }
            break;
            }
            break;

        case IDC_DOCUMENTBKGND:
            switch (LOWORD(wParam))
            {
            }
            break;

        case IDOK:
        {
            unsigned int width = GetDlgItemInt(hwndDlg, IDC_DOCUMENTWIDTH, NULL, FALSE);
            unsigned int height = GetDlgItemInt(hwndDlg, IDC_DOCUMENTHEIGHT, NULL, FALSE);

            int nBkgndSel = (int)SendDlgItemMessage(hwndDlg, IDC_DOCUMENTBKGND, CB_GETCURSEL, 0, 0);
            BkgndColorItem* bkgndItem = (BkgndColorItem*)SendDlgItemMessage(hwndDlg, IDC_DOCUMENTBKGND, CB_GETITEMDATA, nBkgndSel, 0);

            uint32_t bkgndColor = 0;
            switch (bkgndItem->type)
            {
            case COLOR_VALUE:
                bkgndColor = bkgndItem->value;
                break;
            case COLOR_PRIMARY:
                bkgndColor = ABGRToARGB(g_color_context.fg_color);
                break;
            case COLOR_SECONDARY:
                bkgndColor = ABGRToARGB(g_color_context.bg_color);
                break;
            }

            Document* document = Document_New(width, height);

            if (document && document->canvas)
            {
                Canvas_FillSolid(document->canvas, ABGRToARGB(bkgndColor));
            }
            EndDialog(hwndDlg, wParam);
            break;
        }
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            break;
        }
        return TRUE;

    case WM_MEASUREITEM:
    {
        LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;

        if (lpmis->CtlID == IDC_DOCUMENTBKGND)
        {
            lpmis->itemHeight = 20;
        }
    }
    break;

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

        if (lpdis->CtlID == IDC_DOCUMENTBKGND)
        {
            /* Break if item invalid */
            if (lpdis->itemID == (DWORD)-1)
                break;

            /* Get selected string */
            WCHAR szTemp[80] = L"";
            int nLen = ComboBox_GetLBTextLen(lpdis->hwndItem, lpdis->itemID);
            ComboBox_GetLBText(lpdis->hwndItem, lpdis->itemID, (LPARAM)szTemp);

            /* Calculate text position */
            TEXTMETRIC tm;
            GetTextMetrics(lpdis->hDC, &tm);
            int y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
            int x = LOWORD(GetDialogBaseUnits()) / 4;

            /* Caclucate swatch frame */
            RECT swatchFrameRc = lpdis->rcItem;
            swatchFrameRc.left += 4;

            if (lpdis->itemState & ODS_COMBOBOXEDIT)
            {
                swatchFrameRc.left -= 2;
            }

            swatchFrameRc.top += 2;
            swatchFrameRc.bottom -= 2;
            swatchFrameRc.right = swatchFrameRc.bottom - swatchFrameRc.top + 4;

            if (lpdis->itemState & ODS_COMBOBOXEDIT)
                swatchFrameRc.right++;

            /* Calculate inner color rectangle */
            RECT swatchRc = swatchFrameRc;
            ContractRect(&swatchRc, 2);
            int swatchWidth = swatchRc.right - swatchRc.left;
            int swatchHeight = swatchRc.bottom - swatchRc.top;

            /* Set text color.
             * The colors depend on whether the item is selected */
            COLORREF prevBkColor;
            COLORREF prevTextColor;
            prevTextColor = SetTextColor(lpdis->hDC, GetSysColor(
                lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT
                : COLOR_WINDOWTEXT));
            prevBkColor = SetBkColor(lpdis->hDC, GetSysColor(
                lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT
                : COLOR_WINDOW));

            /* Draw text */
            ExtTextOut(lpdis->hDC, x + swatchFrameRc.right + x, y,
                ETO_CLIPPED | ETO_OPAQUE, &lpdis->rcItem, szTemp, nLen, NULL);

            SetTextColor(lpdis->hDC, prevTextColor);
            SetBkColor(lpdis->hDC, prevBkColor);

            /* Draw swatch frame */
            FrameRect(lpdis->hDC, &swatchFrameRc, GetStockObject(BLACK_BRUSH));

            BkgndColorItem* pColorItem = (BkgndColorItem*)ComboBox_GetItemData(
                lpdis->hwndItem, lpdis->itemID);
            assert(pColorItem);

            uint32_t color = 0;
            switch (pColorItem->type)
            {
            case COLOR_VALUE:
                color = pColorItem->value;
                break;
            case COLOR_PRIMARY:
                color = ABGRToARGB(g_color_context.fg_color);
                break;
            case COLOR_SECONDARY:
                color = ABGRToARGB(g_color_context.bg_color);
                break;
            }

            if (color >> 24 != 0xFF)
            {
                NewDocumentDlgData* data = (NewDocumentDlgData*)
                    GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

                SetBrushOrgEx(lpdis->hDC, swatchRc.left, swatchRc.top, NULL);
                FillRect(lpdis->hDC, &swatchRc, data->hbrChecker);
            }

            HBRUSH hbrColor = CreateSolidBrush(color & 0x00FFFFFF);
            if (color >> 24 != 0x00)
            {
                if (color >> 24 != 0xFF)
                {
                    HDC hdcFill = CreateCompatibleDC(lpdis->hDC);
                    HBITMAP hbmFill = CreateCompatibleBitmap(lpdis->hDC, swatchWidth,
                        swatchHeight);
                    HGDIOBJ hOldObj = SelectObject(hdcFill, hbmFill);

                    RECT fillRc = { 0, 0, swatchWidth, swatchHeight };

                    FillRect(hdcFill, &fillRc, hbrColor);

                    /* Blend fill */
                    BLENDFUNCTION blendFunc = {
                      .BlendOp = AC_SRC_OVER,
                      .BlendFlags = 0,
                      .SourceConstantAlpha = color >> 24,
                      .AlphaFormat = 0
                    };

                    AlphaBlend(lpdis->hDC, swatchRc.left, swatchRc.top, swatchWidth,
                        swatchHeight, hdcFill, 0, 0, swatchWidth, swatchHeight,
                        blendFunc);

                    if (GetWindowStyle(lpdis->hwndItem) & CBS_CDCLRSEL_SWATCHBIAS)
                    {
                        BeginPath(lpdis->hDC);
                        MoveToEx(lpdis->hDC, swatchRc.right, swatchRc.top, NULL);
                        LineTo(lpdis->hDC, swatchRc.right, swatchRc.bottom);
                        LineTo(lpdis->hDC, swatchRc.left, swatchRc.bottom);
                        CloseFigure(lpdis->hDC);
                        EndPath(lpdis->hDC);

                        HGDIOBJ hPrevBrush = SelectObject(lpdis->hDC, hbrColor);
                        FillPath(lpdis->hDC);
                        SelectObject(lpdis->hDC, hPrevBrush);
                    }

                    SelectObject(hdcFill, hOldObj);
                    DeleteBitmap(hbmFill);
                    DeleteDC(hdcFill);

                    /* If the item has the focus, draw the focus rectangle */
                    if (lpdis->itemState & ODS_FOCUS)
                        DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
                }
                else {
                    FillRect(lpdis->hDC, &swatchRc, hbrColor);
                }

                DeleteBrush(hbrColor);
            }
        }
    }
    break;
    }

    return FALSE;
}

void NewFileDialog(HWND hwnd)
{
    Panitent_SetActivityStatus(Panitent_GetApp(), L"Creating a new document");

#ifdef LEGACYDIALOGNEW
    RegisterNewFileDialog();

    RECT rc = { 0 };
    rc.pRight = 210;
    rc.bottom = 170;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    CreateWindow(L"NewFileDialogClass",
        L"New",
        WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        (GetSystemMetrics(SM_CXSCREEN) - rc.pRight - rc.pLeft) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - rc.bottom - rc.top) / 2,
        rc.pRight - rc.pLeft,
        rc.bottom - rc.top,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);
#else
    DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(IDD_NEWDOCUMENT), hwnd, (DLGPROC)NewDocumentDlgProc);
#endif
}
