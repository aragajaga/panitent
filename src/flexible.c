#include "precomp.h"

#include "flexible.h"

void LayoutBox_SetPosition(LAYOUTBOX *, UINT, UINT);

inline static LAYOUTBOXCONTENT LayoutBoxContentFromHWND(HWND hWnd)
{
  LAYOUTBOXCONTENT lbc = { 0 };
  lbc.type = LAYOUTBOX_TYPE_CONTROL;
  lbc.content.hWnd = hWnd;

  return lbc;
}

inline static LAYOUTBOXCONTENT LayoutBoxContentFromBox(LAYOUTBOX *pLayoutBox)
{
  LAYOUTBOXCONTENT lbc = { 0 };
  lbc.type = LAYOUTBOX_TYPE_BOX;
  lbc.content.pBox = pLayoutBox;

  return lbc;
}

inline static LAYOUTBOXCONTENT LayoutBoxContentFromGroup(GROUPBOX *pGroupBox)
{
  LAYOUTBOXCONTENT lbc = { 0 };
  lbc.type = LAYOUTBOX_TYPE_GROUP;
  lbc.content.pGroup = pGroupBox;

  return lbc;
}

void CreateLayoutBox(PLAYOUTBOX* pLayoutBox, HWND hWnd)
{
  *pLayoutBox = (PLAYOUTBOX) malloc(sizeof(LAYOUTBOX));
  ZeroMemory(*pLayoutBox, sizeof(LAYOUTBOX));

  (*pLayoutBox)->hContainer = hWnd;
}

void LayoutBox_AddControl(PLAYOUTBOX pLayoutBox, HWND hControl)
{
  pLayoutBox->hContent[pLayoutBox->size++] = LayoutBoxContentFromHWND(hControl);
  SetParent(hControl, pLayoutBox->hContainer);
}

void LayoutBox_AddBox(PLAYOUTBOX pLayoutBox, PLAYOUTBOX pInsertBox)
{
  pLayoutBox->hContent[pLayoutBox->size++] = LayoutBoxContentFromBox(pInsertBox);
}

void LayoutBox_AddGroup(PLAYOUTBOX pLayoutBox, PGROUPBOX pGroup)
{
  pLayoutBox->hContent[pLayoutBox->size++] = LayoutBoxContentFromGroup(pGroup);
}

void LayoutBox_SetPosition(LAYOUTBOX *pLayoutBox, UINT x, UINT y)
{
  pLayoutBox->rc.left = x;
  pLayoutBox->rc.top = y;
}

void LayoutBox_SetSize(PLAYOUTBOX pLayoutBox, UINT uWidth, UINT uHeight)
{
  pLayoutBox->rc.right = pLayoutBox->rc.left + uWidth > pLayoutBox->minWidth ? uWidth : pLayoutBox->minWidth;
  pLayoutBox->rc.bottom = pLayoutBox->rc.top + uHeight;
}

void CreateGroupBox(PGROUPBOX *pGroupBox)
{
  LAYOUTBOXCONTENT pContent;
  *pGroupBox = (PGROUPBOX) malloc(sizeof(GROUPBOX));
  ZeroMemory(*pGroupBox, sizeof(GROUPBOX));
}

void GroupBox_SetPosition(GROUPBOX *pGroupBox, UINT x, UINT y)
{
  pGroupBox->rc.left = x;
  pGroupBox->rc.top = y;
}

void GroupBox_SetSize(PGROUPBOX pGroupBox, UINT uWidth, UINT uHeight)
{
  pGroupBox->rc.right = pGroupBox->rc.left + uWidth;
  pGroupBox->rc.bottom = pGroupBox->rc.top + uHeight;
}

void GroupBox_Update(PGROUPBOX pGroupBox)
{
  LAYOUTBOXCONTENT* pContent = &pGroupBox->hContent;
  RECT rcContainer = pGroupBox->rc;

  unsigned int insx = rcContainer.left + 7;
  unsigned int insy = rcContainer.top + 32;

  RECT rcElement = { 0 };

  if (pContent->type == LAYOUTBOX_TYPE_CONTROL)
  {
    HWND hWnd = pContent->content.hWnd; 
    GetClientRect(hWnd, &rcElement);
  }
  else if (pContent->type == LAYOUTBOX_TYPE_BOX) {
    LAYOUTBOX *pLayoutBox = pContent->content.pBox;
    rcElement = pLayoutBox->rc;
  }
  else if (pContent->type == LAYOUTBOX_TYPE_GROUP) {
    GROUPBOX *pGroupBox = pContent->content.pGroup;
    rcElement = pGroupBox->rc;
  }
  else {
    assert(FALSE);
  }

  unsigned int insWidth = rcElement.right - rcElement.left;
  unsigned int insHeight = rcElement.bottom - rcElement.top;

  if (pContent->type == LAYOUTBOX_TYPE_CONTROL)
  {
    HWND hWnd = pContent->content.hWnd;

    insWidth = rcContainer.right - rcContainer.left - 14;
    SetWindowPos(hWnd, NULL, insx, insy, insWidth, insHeight, SWP_NOACTIVATE | SWP_NOZORDER);

    pGroupBox->rc.bottom = pGroupBox->rc.top + rcElement.bottom + 33 + 7;
  }
  else if (pContent->type == LAYOUTBOX_TYPE_BOX) {
    LAYOUTBOX *pBox = pContent->content.pBox;

    LayoutBox_SetPosition(pBox, insx, insy);
    LayoutBox_SetSize(pBox, insWidth, insHeight);
    LayoutBox_Update(pBox);
  }
  else if (pContent->type == LAYOUTBOX_TYPE_GROUP) {
    GROUPBOX *pGroup = pContent->content.pGroup;

    GroupBox_SetPosition(pGroup, insx, insy);
    GroupBox_SetSize(pGroup, insWidth, insHeight);
    GroupBox_Update(pGroup);
  }
  else {
    assert(FALSE);
  }
}

void GroupBox_SetCaption(PGROUPBOX pGroupBox, LPWSTR lpszCaption)
{
  pGroupBox->lpszCaption = lpszCaption;
}

void GroupBox_SetControl(GROUPBOX *pGroupBox, HWND hWnd)
{
  pGroupBox->hContent = LayoutBoxContentFromHWND(hWnd);
}

void GroupBox_SetLayoutBox(PGROUPBOX pGroupBox, PLAYOUTBOX pLayoutBox)
{
  pGroupBox->hContent = LayoutBoxContentFromBox(pLayoutBox);
}

void CreateLabelLayoutBox(LAYOUTBOX **lb, HWND hStatic, HWND hControl, HWND hParent)
{
  CreateLayoutBox(lb, hParent);
  LayoutBox_AddControl(*lb, hStatic);
  LayoutBox_AddControl(*lb, hControl);
  (*lb)->direction = LAYOUTBOX_HORIZONTAL;
  (*lb)->spaceBetween = 7; (*lb)->stretch = TRUE;
  LayoutBox_SetSize(*lb, 200, 22);
}

inline static HWND CreateWindowExGuiFont(DWORD dwExStyle, LPCWSTR lpClassName,
    LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
  HWND hWnd;
  HFONT hFont;

  hWnd = CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth,
      nHeight, hParent, hMenu, hInstance, lpParam); 
  hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

  LOGFONT lfFont;
  GetObject(hFont, sizeof(LOGFONT), &lfFont);
  lfFont.lfWeight = FW_BOLD;
  // wcscpy(lfFont.lfFaceName, L"Comic Sans MS");

  HFONT hFont2 = CreateFontIndirect(&lfFont);

  SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont2, 0);

  return hWnd;
}

void GroupBox_Destroy(PGROUPBOX); 
void LayoutBox_Destroy(PLAYOUTBOX);

void GroupBox_Destroy(PGROUPBOX pGroupBox)
{
  PLAYOUTBOXCONTENT pContent;

  pContent = &(pGroupBox->hContent);

  assert(pContent->type);
  if (!pContent->type)
    return;

  switch (pContent->type)
  {
    case LAYOUTBOX_TYPE_CONTROL:
      DestroyWindow(pContent->content.hWnd);
      break;
    case LAYOUTBOX_TYPE_BOX:
      LayoutBox_Destroy(pContent->content.pBox);
      break;
    case LAYOUTBOX_TYPE_GROUP:
      GroupBox_Destroy(pContent->content.pGroup);
      break;
  }

  free(pGroupBox);
}

void LayoutBox_Destroy(PLAYOUTBOX pLayoutBox)
{
  size_t i;

  for (i = 0; pLayoutBox->hContent[i].type; i++)
  {
    PLAYOUTBOXCONTENT pContent;

    pContent = &(pLayoutBox->hContent[i]);
    switch (pContent->type)
    {
      case LAYOUTBOX_TYPE_CONTROL:
        DestroyWindow(pContent->content.hWnd);
        break;
      case LAYOUTBOX_TYPE_BOX:
        LayoutBox_Destroy(pContent->content.pBox);
        break;
      case LAYOUTBOX_TYPE_GROUP:
        GroupBox_Destroy(pContent->content.pGroup);
        break;
    }
  }

  free(pLayoutBox);
}

void LayoutBox_Update(PLAYOUTBOX pLayoutBox)
{
  size_t i;
  unsigned int prevWidth = 0;
  unsigned int thisWidth = 0;
  unsigned int padding = 0;
  unsigned int insWidth = 0;
  unsigned int insHeight = 0;
  BOOL horiz = FALSE;
  DWORD dwSWPFlags;
  RECT rcContainer;
  RECT rcElement = { 0 };

  dwSWPFlags = SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER;

  horiz = pLayoutBox->direction == LAYOUTBOX_HORIZONTAL ? TRUE : FALSE;
  padding = pLayoutBox->padding;

  /* Add padding */
  prevWidth += padding;
  rcContainer = pLayoutBox->rc;

  for (i = 0; i < pLayoutBox->size; i++)
  {
    LAYOUTBOXCONTENT* pContent = &pLayoutBox->hContent[i];


    if (pContent->type == LAYOUTBOX_TYPE_CONTROL)
    {
      HWND hWnd = pContent->content.hWnd;

      GetClientRect(hWnd, &rcElement);
      // MapWindowPoints(hWnd, GetParent(hWnd), (LPPOINT)&rcElement, 2);
    }
    else if (pContent->type == LAYOUTBOX_TYPE_BOX)
    {
      LAYOUTBOX *pBox = pContent->content.pBox;
      rcElement = pBox->rc;
    }
    else if (pContent->type == LAYOUTBOX_TYPE_GROUP)
    {
      GROUPBOX *pGroup = pContent->content.pGroup;
      rcElement = pGroup->rc;
    }
    else {
      assert(FALSE);
    }

    unsigned int nWidth = rcElement.right - rcElement.left;
    unsigned int nHeight = rcElement.bottom - rcElement.top;
    thisWidth = horiz ? nWidth : nHeight;

    if (pLayoutBox->stretch)
    {
      dwSWPFlags &= ~SWP_NOSIZE;

      if (horiz)
      {
        insWidth = nWidth;
        insHeight = rcContainer.bottom - rcContainer.top - padding * 2;
      }
      else {
        insWidth = rcContainer.right - rcContainer.left - padding * 2;
        insHeight = nHeight;
      }
    }

    unsigned int insx = rcContainer.left + (horiz ? prevWidth : padding);
    unsigned int insy = rcContainer.top + (horiz ? padding : prevWidth);

    if (pContent->type == LAYOUTBOX_TYPE_CONTROL)
    {
      HWND hWnd = pContent->content.hWnd;

      SetWindowPos(hWnd, NULL, insx, insy, insWidth, insHeight, dwSWPFlags);
    }
    else if (pContent->type == LAYOUTBOX_TYPE_BOX) {
      LAYOUTBOX *pBox = pContent->content.pBox;

      LayoutBox_SetPosition(pBox, insx, insy);
      LayoutBox_SetSize(pBox, insWidth, insHeight);
      LayoutBox_Update(pBox);
    }
    else if (pContent->type == LAYOUTBOX_TYPE_GROUP) {
      GROUPBOX *pGroup = pContent->content.pGroup;

      GroupBox_SetPosition(pGroup, insx, insy);
      GroupBox_SetSize(pGroup, insWidth, insHeight);
      GroupBox_Update(pGroup);
    }
    else {
      assert(FALSE);
    }

    prevWidth += pLayoutBox->spaceBetween + thisWidth;
  }
}

void DrawGroupBox(HDC hDC, GROUPBOX *pGroupBox)
{
  RECT* rc = &pGroupBox->rc;

  RECT rcCaption = *rc;
  rcCaption.bottom = rcCaption.top + 25;
  rcCaption.left += 2;
  rcCaption.top += 2;
  rcCaption.right -= 3;
  // FrameRect(hDC, rc, CreateSolidBrush(RGB(255, 0, 0)));
  DrawEdge(hDC, rc, EDGE_ETCHED, BF_RECT);

  /* Fill caption background */
  /* FillRect(hDC, &rcCaption, CreateSolidBrush(RGB(27, 94, 32))); */
  TRIVERTEX pGradVert[2];
  GRADIENT_RECT mesh;

  pGradVert[0].x = rcCaption.left;
  pGradVert[0].y = rcCaption.top;
  pGradVert[0].Red = 0x8888; 
  pGradVert[0].Green = 0x0000;
  pGradVert[0].Blue = 0x0000;
  pGradVert[0].Alpha = 0x0000;

  pGradVert[1].x = rcCaption.right;
  pGradVert[1].y = rcCaption.bottom;
  pGradVert[1].Red = 0xFFFF;
  pGradVert[1].Green = 0x0000;
  pGradVert[1].Blue = 0x0000;
  pGradVert[1].Alpha = 0x0000;

  mesh.UpperLeft = 0;
  mesh.LowerRight = 1;

  GradientFill(hDC, pGradVert, 2, &mesh, 1, GRADIENT_FILL_RECT_H);

  assert(pGroupBox->lpszCaption);
  if (pGroupBox->lpszCaption)
  {
    HFONT hFont = GetStockObject(DEFAULT_GUI_FONT);

    LOGFONT fontAttrib = { 0 };
    GetObject(hFont, sizeof(fontAttrib), &fontAttrib);
    fontAttrib.lfHeight = 20;
    fontAttrib.lfWeight = FW_BOLD;
    StringCchCopy(fontAttrib.lfFaceName, LF_FACESIZE, L"Trebuchet MS\0");

    HFONT hFontBold = CreateFontIndirect(&fontAttrib);

    HFONT hOldFont = SelectObject(hDC, hFontBold);

    RECT rcCaption = *rc;
    rcCaption.top += 2;
    rcCaption.left += 7;
    rcCaption.right -= 7;
    rcCaption.bottom = rcCaption.top + 23;

    size_t len = wcslen(pGroupBox->lpszCaption);
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(232, 245, 233));

    WCHAR szCaption[80] = { 0 };
    StringCchCopy(szCaption, 80, pGroupBox->lpszCaption);

    DrawText(hDC, szCaption, len, &rcCaption, DT_SINGLELINE | DT_MODIFYSTRING | DT_END_ELLIPSIS | DT_VCENTER | DT_TOP);

    SelectObject(hDC, hOldFont);
    DeleteObject(hFontBold);
  }
}

void DrawLayoutBox(HDC hDC, PLAYOUTBOX pLayoutBox)
{
  size_t i;

  for (i = 0; i < pLayoutBox->size; i++)
  {
    LAYOUTBOXCONTENT *pContent = &pLayoutBox->hContent[i];

    if (pContent->type == LAYOUTBOX_TYPE_BOX)
    {
      DrawLayoutBox(hDC, pContent->content.pBox);
    }
    else if (pContent->type == LAYOUTBOX_TYPE_GROUP) {
      DrawGroupBox(hDC, pContent->content.pGroup);
    }
  }
}
