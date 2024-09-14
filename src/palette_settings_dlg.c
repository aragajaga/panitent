#include "precomp.h"

#include "resource.h"

#include "palette_settings_dlg.h"
#include "swatch2.h"

#define SWATCH_SIZE_LEN 4
#define CHECKER_SIZE_LEN 4

struct _PaletteSettings g_paletteSettings = {
  .swatchSize = 16,
  .checkerSize = 8,
  .checkerColor1 = 0xFFCCCCCC,
  .checkerColor2 = 0xFFFFFFFF,
  .checkerInvalidate = TRUE
};

INT_PTR CALLBACK PaletteSettingsDlgProc(HWND hwndDlg, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        WCHAR szSwatchSize[SWATCH_SIZE_LEN];
        _itow_s(g_paletteSettings.swatchSize, szSwatchSize, SWATCH_SIZE_LEN, 10);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_SWATCHSIZEEDIT), szSwatchSize);

        WCHAR szCheckerSize[CHECKER_SIZE_LEN];
        _itow_s(g_paletteSettings.checkerSize, szCheckerSize, CHECKER_SIZE_LEN, 10);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_CHECKERSIZEEDIT), szCheckerSize);

        SwatchControl2_SetColor(GetDlgItem(hwndDlg, IDC_CHECKERCOLOR1),
            g_paletteSettings.checkerColor1 & 0x00FFFFFF);
        SwatchControl2_SetColor(GetDlgItem(hwndDlg, IDC_CHECKERCOLOR2),
            g_paletteSettings.checkerColor2 & 0x00FFFFFF);
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {

            case IDOK:
            {
                WCHAR szSwatchSize[4];
                Edit_GetText(GetDlgItem(hwndDlg, IDC_SWATCHSIZEEDIT), szSwatchSize,
                    4);
                g_paletteSettings.swatchSize = _wtoi(szSwatchSize);

                WCHAR szCheckerSize[4];
                Edit_GetText(GetDlgItem(hwndDlg, IDC_CHECKERSIZEEDIT),
                    szCheckerSize, 4);
                g_paletteSettings.checkerSize = _wtoi(szCheckerSize);

                g_paletteSettings.checkerColor1 = SwatchControl2_GetColor(
                    GetDlgItem(hwndDlg, IDC_CHECKERCOLOR1));

                g_paletteSettings.checkerColor2 = SwatchControl2_GetColor(
                    GetDlgItem(hwndDlg, IDC_CHECKERCOLOR2));

                g_paletteSettings.checkerInvalidate = TRUE;

                InvalidateRect(GetParent(hwndDlg), NULL, TRUE);
                EndDialog(hwndDlg, wParam);
            }
            break;
            case IDCANCEL:
                EndDialog(hwndDlg, wParam);
                break;
            }
        }
        return TRUE;
    }

    return FALSE;
}
