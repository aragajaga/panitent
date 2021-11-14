#include "precomp.h"

#include "panitent.h"
#include "flexible.h"
#include "settings_dialog.h"

typedef struct _tagSETTINGSDIALOGDATA {
  PLAYOUTBOX pRootBox;
} SETTINGSDIALOGDATA, *PSETTINGSDIALOGDATA;

BOOL LBoxHelper_CreateLabeledEdit(LPCWSTR lpszLabel, HMENU nId,
    PLAYOUTBOX *ppLayoutBox, HWND hParent)
{
  HWND hWndControl;
  HWND hWndLabel;
  HINSTANCE hInstance;

  hInstance = (HINSTANCE) GetWindowLongPtr(hParent, GWLP_HINSTANCE);

  hWndLabel = CreateWindowEx(0, WC_STATIC, lpszLabel, WS_CHILD | WS_VISIBLE,
      0, 0, 70, 21, hParent, NULL, hInstance, NULL);
  assert(hWndLabel);
  if (!hWndLabel)
    return FALSE;
  SetGuiFont(hWndLabel);

  hWndControl = CreateWindowEx(0, WC_EDIT, NULL,
      WS_CHILD | WS_BORDER | WS_VISIBLE,
      0, 0, 70, 21, hParent, nId, hInstance, NULL);
  assert(hWndControl);
  if (!hWndControl)
    return FALSE;
  SetGuiFont(hWndControl);

  CreateLayoutBox(ppLayoutBox, hParent);
  LayoutBox_AddControl(*ppLayoutBox, hWndLabel);
  LayoutBox_AddControl(*ppLayoutBox, hWndControl);

  (*ppLayoutBox)->direction = LAYOUTBOX_HORIZONTAL;
  (*ppLayoutBox)->spaceBetween = 4;
  (*ppLayoutBox)->stretch = FALSE;

  LayoutBox_SetSize(*ppLayoutBox, 300, 21);
  LayoutBox_Update(*ppLayoutBox);

  return TRUE;
}

void BuildWindowInitializationSettingsLayout(PLAYOUTBOX *ppLayoutBox,
    HWND hParent)
{
  PLAYOUTBOX lpLabelPosX;
  PLAYOUTBOX lpLabelPosY;
  PLAYOUTBOX lpLabelWidth;
  PLAYOUTBOX lpLabelHeight;

  CreateLayoutBox(ppLayoutBox, hParent);

  LBoxHelper_CreateLabeledEdit(L"Position X", (HMENU) 101, &lpLabelPosX,
      hParent);
  LayoutBox_AddBox(*ppLayoutBox, lpLabelPosX);

  LBoxHelper_CreateLabeledEdit(L"Position Y", (HMENU) 102, &lpLabelPosY,
      hParent);
  LayoutBox_AddBox(*ppLayoutBox, lpLabelPosY);

  LBoxHelper_CreateLabeledEdit(L"Width", (HMENU) 103, &lpLabelWidth, hParent);
  LayoutBox_AddBox(*ppLayoutBox, lpLabelWidth);

  LBoxHelper_CreateLabeledEdit(L"Height", (HMENU) 104, &lpLabelHeight, hParent);
  LayoutBox_AddBox(*ppLayoutBox, lpLabelHeight);

  (*ppLayoutBox)->direction = LAYOUTBOX_VERTICAL;
  (*ppLayoutBox)->spaceBetween = 7;
  (*ppLayoutBox)->stretch = TRUE;

  LayoutBox_SetSize(*ppLayoutBox, 400, 100);
  LayoutBox_Update(*ppLayoutBox);
}

void BuildInputSettingsLayout(PLAYOUTBOX *ppLayoutBox, HWND hParent)
{
  HINSTANCE hInstance;
  HWND hCheckEnablePen;

  CreateLayoutBox(ppLayoutBox, hParent);

  hInstance = (HINSTANCE) GetWindowLongPtr(hParent, GWLP_HINSTANCE);

  hCheckEnablePen = CreateWindowEx(0, WC_BUTTON, L"Enable pen tablet",
      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
      0, 0, 100, 21,
      hParent, NULL, hInstance, NULL);
  assert(hCheckEnablePen);
  SetGuiFont(hCheckEnablePen);

  LayoutBox_AddControl(*ppLayoutBox, hCheckEnablePen);

  (*ppLayoutBox)->direction = LAYOUTBOX_VERTICAL;
  (*ppLayoutBox)->spaceBetween = 7;
  (*ppLayoutBox)->stretch = TRUE;

  LayoutBox_SetSize(*ppLayoutBox, 400, 50);
  LayoutBox_Update(*ppLayoutBox);
}

void BuildMaintenanceSettingsLayout(PLAYOUTBOX *ppLayoutBox, HWND hParent)
{
  HINSTANCE hInstance;
  HWND hBtnResetAll;
  HWND hBtnCheckUpdates;

  CreateLayoutBox(ppLayoutBox, hParent);

  hInstance = (HINSTANCE) GetWindowLongPtr(hParent, GWLP_HINSTANCE);

  hBtnResetAll = CreateWindowEx(0, WC_BUTTON, L"Reset all settings",
      WS_CHILD | WS_VISIBLE,
      0, 0, 100, 30,
      hParent, NULL, hInstance, NULL);
  assert(hBtnResetAll);
  SetGuiFont(hBtnResetAll);

  LayoutBox_AddControl(*ppLayoutBox, hBtnResetAll);

  hBtnCheckUpdates = CreateWindowEx(0, WC_BUTTON, L"Check updates",
      WS_CHILD | WS_VISIBLE,
      0, 0, 100, 30,
      hParent, NULL, hInstance, NULL);
  assert(hBtnCheckUpdates);
  SetGuiFont(hBtnCheckUpdates);

  LayoutBox_AddControl(*ppLayoutBox, hBtnCheckUpdates);

  (*ppLayoutBox)->direction = LAYOUTBOX_VERTICAL;
  (*ppLayoutBox)->spaceBetween = 7;
  (*ppLayoutBox)->stretch = TRUE;

  LayoutBox_SetSize(*ppLayoutBox, 400, 100);
  LayoutBox_Update(*ppLayoutBox);
}

void BuildWindowInitializationSettingsGroup(PGROUPBOX *ppGroupBox, HWND hParent)
{
  PLAYOUTBOX lbWindowInitialization;

  /* Create inner content */
  CreateGroupBox(ppGroupBox);
  BuildWindowInitializationSettingsLayout(&lbWindowInitialization, hParent);
  GroupBox_SetLayoutBox(*ppGroupBox, lbWindowInitialization);

  /* Set caption and size to group */
  GroupBox_SetCaption(*ppGroupBox, L"Initial window position and size");
  GroupBox_SetSize(*ppGroupBox, 400, 200);
  GroupBox_Update(*ppGroupBox);
}

void BuildInputSettingsGroup(PGROUPBOX *ppGroupBox, HWND hParent)
{
  PLAYOUTBOX lbInput;

  /* Create inner content */
  CreateGroupBox(ppGroupBox);
  BuildInputSettingsLayout(&lbInput, hParent);
  GroupBox_SetLayoutBox(*ppGroupBox, lbInput);

  /* Set caption and size to group */
  GroupBox_SetCaption(*ppGroupBox, L"Input");
  GroupBox_SetSize(*ppGroupBox, 400, 100);
  GroupBox_Update(*ppGroupBox);
}

void BuildMaintenanceSettingsGroup(PGROUPBOX *ppGroupBox, HWND hParent)
{
  PLAYOUTBOX lbMaintenance;

  /* Create inner content */
  CreateGroupBox(ppGroupBox);
  BuildMaintenanceSettingsLayout(&lbMaintenance, hParent);
  GroupBox_SetLayoutBox(*ppGroupBox, lbMaintenance);

  /* Set caption and size to group */
  GroupBox_SetCaption(*ppGroupBox, L"Maintenance");
  GroupBox_SetSize(*ppGroupBox, 400, 100);
  GroupBox_Update(*ppGroupBox);
}

void InitSettingsDialogLayout(PLAYOUTBOX pLayoutBox, HWND hParent)
{
  PGROUPBOX gbWindowInitSG;
  PGROUPBOX gbInputSG;
  PGROUPBOX gbMaintenanceSG;

  /* Construct and insert setting groups */

  /* Window initialization */
  BuildWindowInitializationSettingsGroup(&gbWindowInitSG, hParent);
  LayoutBox_AddGroup(pLayoutBox, gbWindowInitSG);

  /* Input */
  BuildInputSettingsGroup(&gbInputSG, hParent);
  LayoutBox_AddGroup(pLayoutBox, gbInputSG);

  /* Maintenance */
  BuildMaintenanceSettingsGroup(&gbMaintenanceSG, hParent);
  LayoutBox_AddGroup(pLayoutBox, gbMaintenanceSG);
}

void SettingsDialog_OnCreate(HWND hWnd, LPCREATESTRUCT lpcs)
{
  PSETTINGSDIALOGDATA pData;
  PLAYOUTBOX pLayoutBox;
 
  pData = (PSETTINGSDIALOGDATA) malloc(sizeof(SETTINGSDIALOGDATA));
  ZeroMemory(pData, sizeof(SETTINGSDIALOGDATA));

  SetWindowLongPtr(hWnd, 0, (LONG_PTR) pData);

  CreateLayoutBox(&pLayoutBox, hWnd);
  InitSettingsDialogLayout(pLayoutBox, hWnd);
  pLayoutBox->padding = 7;
  pLayoutBox->spaceBetween = 7;
  pLayoutBox->direction = LAYOUTBOX_VERTICAL;
  pLayoutBox->stretch = TRUE;

  pData->pRootBox = pLayoutBox;
}

void SettingsDialog_OnSize(HWND hWnd, UINT uWidth, UINT uHeight)
{
  PSETTINGSDIALOGDATA pData;
  
  pData = (PSETTINGSDIALOGDATA) GetWindowLongPtr(hWnd, 0);
  LayoutBox_SetSize(pData->pRootBox, uWidth, uHeight);
  LayoutBox_Update(pData->pRootBox);
}

void SettingsDialog_OnPaint(HWND hWnd)
{
  PSETTINGSDIALOGDATA pData;

  pData = (PSETTINGSDIALOGDATA) GetWindowLongPtr(hWnd, 0);

  PAINTSTRUCT ps;
  HDC hDC;

  ZeroMemory(&ps, sizeof(PAINTSTRUCT));

  hDC = BeginPaint(hWnd, &ps);
  DrawLayoutBox(hDC, pData->pRootBox);

  EndPaint(hWnd, &ps);
}

void SettingsDialog_OnDestroy(HWND hWnd)
{
  PSETTINGSDIALOGDATA pData;

  pData = (PSETTINGSDIALOGDATA) GetWindowLongPtr(hWnd, 0);
  SetWindowLongPtr(hWnd, 0, (LONG_PTR) NULL);

  LayoutBox_Destroy(pData->pRootBox);

  free(pData);
}

LRESULT CALLBACK SettingsDialog_WndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      SettingsDialog_OnCreate(hWnd, (LPCREATESTRUCT) lParam);
      return 0;

    case WM_SIZE:
      SettingsDialog_OnSize(hWnd, LOWORD(lParam), HIWORD(lParam));
      return 0;

    case WM_PAINT:
      SettingsDialog_OnPaint(hWnd);
      return 0;

    case WM_DESTROY:
      SettingsDialog_OnDestroy(hWnd);
      return 0;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL SettingsDialog_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;

  ZeroMemory(&wcex, sizeof(WNDCLASSEX));
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.cbWndExtra = sizeof(PSETTINGSDIALOGDATA);
  wcex.lpfnWndProc = (WNDPROC) SettingsDialog_WndProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
  wcex.lpszClassName = L"SettingsDialogClass";

  return RegisterClassEx(&wcex);
}
