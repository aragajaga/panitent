#include "precomp.h"

#include "flexible.h"
#include "util.h"

GROUPBOXCAPTIONSTYLE g_captionStyle;

static inline LAYOUTBOXCONTENT LayoutBoxContentFromHWND(HWND hWnd)
{
  LAYOUTBOXCONTENT lbc = { 0 };
  lbc.type = LAYOUTBOX_TYPE_CONTROL;
  lbc.data.hWnd = hWnd;

  return lbc;
}

static inline LAYOUTBOXCONTENT LayoutBoxContentFromBox(LAYOUTBOX *pLayoutBox)
{
  LAYOUTBOXCONTENT lbc = { 0 };
  lbc.type = LAYOUTBOX_TYPE_BOX;
  lbc.data.pBox = pLayoutBox;

  return lbc;
}

static inline LAYOUTBOXCONTENT LayoutBoxContentFromGroup(GROUPBOX *pGroupBox)
{
  LAYOUTBOXCONTENT lbc = { 0 };
  lbc.type = LAYOUTBOX_TYPE_GROUP;
  lbc.data.pGroup = pGroupBox;

  return lbc;
}

void CreateLayoutBox(PLAYOUTBOX* pLayoutBox, HWND hWnd)
{
  *pLayoutBox = (PLAYOUTBOX)malloc(sizeof(LAYOUTBOX));
  memset(pLayoutBox, 0, sizeof(LAYOUTBOX));
  if (!*pLayoutBox)
    return;

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
  // pLayoutBox->rc.right = pLayoutBox->rc.left + (uWidth > pLayoutBox->minWidth) ? uWidth : pLayoutBox->minWidth;
  // pLayoutBox->rc.bottom = pLayoutBox->rc.top + uHeight;
    UNREFERENCED_PARAMETER(uWidth);
    UNREFERENCED_PARAMETER(uHeight);

    PRECT prc = &pLayoutBox->rc;

    // prc->left = 0;
    prc->right = prc->left + uWidth;
    prc->bottom = prc->top + uHeight;
}

#define FONT_LIGHT 0x1
#define FONT_BOLD 0x2
#define FONT_BLACK 0x4
#define FONT_ITALIC 0x8

HFONT LoadFont(PWSTR szFaceName, LONG lHeight, DWORD dwFlags)
{
  HFONT hSysFont = GetStockObject(DEFAULT_GUI_FONT);

  LOGFONT fontAttrib = { 0 };
  GetObject(hSysFont, sizeof(fontAttrib), &fontAttrib);
  fontAttrib.lfHeight = lHeight;

  LONG lWeight = FW_NORMAL;
  BYTE bItalic = FALSE;

  int mutuallyExcluded = 0;
  if (dwFlags & FONT_LIGHT) ++mutuallyExcluded;
  if (dwFlags & FONT_BOLD) ++mutuallyExcluded;
  if (dwFlags & FONT_BLACK) ++mutuallyExcluded;

  if (mutuallyExcluded > 1)
  {
      RaiseException(1337, 0, 0, NULL);
  }

  if (dwFlags & FONT_LIGHT)
  {
    lWeight = FW_LIGHT;
  }
  else if (dwFlags & FONT_BOLD)
  {
      lWeight = FW_BOLD;
  }
  else if (dwFlags & FONT_BLACK)
  {
    lWeight = FW_BLACK;
  }

  if (dwFlags & FONT_ITALIC)
  {
    bItalic = TRUE;
  }

  fontAttrib.lfWeight = lWeight;
  fontAttrib.lfItalic = bItalic;

  StringCchCopy(fontAttrib.lfFaceName, LF_FACESIZE, szFaceName);

  HFONT hFont = CreateFontIndirect(&fontAttrib);

  return hFont;
}

#define HEX_TO_COLORREF(hexColor) ((COLORREF) ((hexColor & 0xFF) << 16) | (hexColor & 0xFF00) | ((hexColor & 0xFF0000) >> 16))

inline COLORREF HSVtoCOLORREF(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v - c;

    float r, g, b;

    if (h >= 0 && h < 60) {
        r = c;
        g = x;
        b = 0;
    }
    else if (h >= 60 && h < 120) {
        r = x;
        g = c;
        b = 0;
    }
    else if (h >= 120 && h < 180) {
        r = 0;
        g = c;
        b = x;
    }
    else if (h >= 180 && h < 240) {
        r = 0;
        g = x;
        b = c;
    }
    else if (h >= 240 && h < 300) {
        r = x;
        g = 0;
        b = c;
    }
    else {
        r = c;
        g = 0;
        b = x;
    }

    int red = (int)((r + m) * 255.0f);
    int green = (int)((g + m) * 255.0f);
    int blue = (int)((b + m) * 255.0f);

    return RGB(red, green, blue);
}

PGROUPBOXCAPTIONSTYLE GroupBox_GetGlobalStyle()
{
  static GROUPBOXCAPTIONSTYLE s_captionStyle;
  static BOOL s_bLoaded;

  if (!s_bLoaded)
  {
    HFONT hFont = NULL;

    //__try {
      hFont = LoadFont(L"Rubik\0", 20, FONT_ITALIC);
    //}
    //__except (EXCEPTION_CONTINUE_SEARCH) {
      //MessageBox(NULL, L"Invalid Argument", NULL, MB_OK | MB_ICONERROR);
    //}

    CAPTIONFILLSTYLE captionFill;

    captionFill.uType = FILL_STYLE_GRADIENT;
    captionFill.data.gradient.dwColorStart = HEX_TO_COLORREF(0xFF8000);
    captionFill.data.gradient.dwColorEnd = HSVtoCOLORREF(30.0f, 0.5f, 1.0f);
    captionFill.data.gradient.uDirection = GRADIENT_FILL_RECT_H;

    s_captionStyle.hFont = hFont;
    s_captionStyle.uHeight = 24;
    s_captionStyle.dwTextColor = RGB(255, 255, 255);
    s_captionStyle.captionBkgFill = captionFill;

    s_bLoaded = TRUE;
  }

  return &s_captionStyle;
}

void CreateGroupBox(PGROUPBOX *pGroupBox)
{
  *pGroupBox = (PGROUPBOX)malloc(sizeof(GROUPBOX));
  memset(pGroupBox, 0, sizeof(GROUPBOX));
  if (!*pGroupBox)
    return;

  ZeroMemory(*pGroupBox, sizeof(GROUPBOX));

  (*pGroupBox)->pCaptionStyle = GroupBox_GetGlobalStyle();
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
    HWND hWnd = pContent->data.hWnd; 
    GetClientRect(hWnd, &rcElement);
  }
  else if (pContent->type == LAYOUTBOX_TYPE_BOX) {
    LAYOUTBOX *pLayoutBox = pContent->data.pBox;
    rcElement = pLayoutBox->rc;
  }
  else if (pContent->type == LAYOUTBOX_TYPE_GROUP) {
    GROUPBOX *pGroupBoxInner = pContent->data.pGroup;
    rcElement = pGroupBoxInner->rc;
  }
  else {
    assert(FALSE);
  }

  unsigned int insWidth = rcContainer.right - rcContainer.left - 14;
  unsigned int insHeight = rcContainer.bottom - rcContainer.top - 32 - 14;

  if (pContent->type == LAYOUTBOX_TYPE_CONTROL)
  {
    HWND hWnd = pContent->data.hWnd;

    insWidth = rcContainer.right - rcContainer.left - 14;
    SetWindowPos(hWnd, NULL, insx, insy, insWidth, insHeight, SWP_NOACTIVATE | SWP_NOZORDER);

    pGroupBox->rc.bottom = pGroupBox->rc.top + rcElement.bottom + 33 + 7;
  }
  else if (pContent->type == LAYOUTBOX_TYPE_BOX) {
    LAYOUTBOX *pBox = pContent->data.pBox;

    LayoutBox_SetPosition(pBox, insx, insy);
    LayoutBox_SetSize(pBox, insWidth, insHeight);
    LayoutBox_Update(pBox);
  }
  else if (pContent->type == LAYOUTBOX_TYPE_GROUP) {
    GROUPBOX *pGroup = pContent->data.pGroup;

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

static inline void BoxContent_Destroy(PLAYOUTBOXCONTENT pBoxContent)
{
  assert(pBoxContent->type);
  if (!pBoxContent->type)
    return;

  switch (pBoxContent->type)
  {
    case LAYOUTBOX_TYPE_CONTROL:
      DestroyWindow(pBoxContent->data.hWnd);
      break;

    case LAYOUTBOX_TYPE_BOX:
      LayoutBox_Destroy(pBoxContent->data.pBox);
      break;

    case LAYOUTBOX_TYPE_GROUP:
      GroupBox_Destroy(pBoxContent->data.pGroup);
      break;
  }
}

void GroupBox_Destroy(PGROUPBOX pGroupBox)
{
  PLAYOUTBOXCONTENT pContent;

  pContent = &(pGroupBox->hContent);
  BoxContent_Destroy(pContent);

  free(pGroupBox);
}

void LayoutBox_Destroy(PLAYOUTBOX pLayoutBox)
{
  size_t i;

  for (i = 0; pLayoutBox->hContent[i].type; i++)
  {
    PLAYOUTBOXCONTENT pContent;

    pContent = &(pLayoutBox->hContent[i]);
    BoxContent_Destroy(pContent);
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
      HWND hWnd = pContent->data.hWnd;

      GetWindowRect(hWnd, &rcElement);
      // MapWindowPoints(hWnd, GetParent(hWnd), (LPPOINT)&rcElement, 2);
    }
    else if (pContent->type == LAYOUTBOX_TYPE_BOX)
    {
      LAYOUTBOX *pBox = pContent->data.pBox;
      rcElement = pBox->rc;
    }
    else if (pContent->type == LAYOUTBOX_TYPE_GROUP)
    {
      GROUPBOX *pGroup = pContent->data.pGroup;
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
        insHeight = (rcContainer.bottom - rcContainer.top) - padding * 2;
      }
      else {
        insWidth = (rcContainer.right - rcContainer.left) - padding * 2;
        insHeight = nHeight;
      }
    }

    unsigned int insx = rcContainer.left + (horiz ? prevWidth : padding);
    unsigned int insy = rcContainer.top + (horiz ? padding : prevWidth);

    if (pContent->type == LAYOUTBOX_TYPE_CONTROL)
    {
      HWND hWnd = pContent->data.hWnd;

      SetWindowPos(hWnd, NULL, insx, insy, insWidth, insHeight, dwSWPFlags);
    }
    else if (pContent->type == LAYOUTBOX_TYPE_BOX) {
      LAYOUTBOX *pBox = pContent->data.pBox;

      LayoutBox_SetPosition(pBox, insx, insy);
      LayoutBox_SetSize(pBox, insWidth, insHeight);
      LayoutBox_Update(pBox);
    }
    else if (pContent->type == LAYOUTBOX_TYPE_GROUP) {
      GROUPBOX *pGroup = pContent->data.pGroup;

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

void HsvToRgb(double h, double s, double v, int* r, int* g, int* b) {
    int i;
    double f, p, q, t;

    if (s == 0) {
        *r = *g = *b = (int)(v * 255.0);
        return;
    }

    h /= 60.0;
    i = (int)floor(h);
    f = h - i;
    p = v * (1.0 - s);
    q = v * (1.0 - s * f);
    t = v * (1.0 - s * (1.0 - f));

    switch (i) {
    case 0:
        *r = (int)(v * 255.0);
        *g = (int)(t * 255.0);
        *b = (int)(p * 255.0);
        break;
    case 1:
        *r = (int)(q * 255.0);
        *g = (int)(v * 255.0);
        *b = (int)(p * 255.0);
        break;
    case 2:
        *r = (int)(p * 255.0);
        *g = (int)(v * 255.0);
        *b = (int)(t * 255.0);
        break;
    case 3:
        *r = (int)(p * 255.0);
        *g = (int)(q * 255.0);
        *b = (int)(v * 255.0);
        break;
    case 4:
        *r = (int)(t * 255.0);
        *g = (int)(p * 255.0);
        *b = (int)(v * 255.0);
        break;
    default:
        *r = (int)(v * 255.0);
        *g = (int)(p * 255.0);
        *b = (int)(q * 255.0);
        break;
    }
}

double g_hue = 0.0;

void DrawRect(HDC hDC, PRECT rc)
{
  int r;
  int g;
  int b;

  HsvToRgb(g_hue, 1.0, 1.0, &r, &g, &b);
  HPEN hPen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
  HPEN hOldPen = SelectObject(hDC, hPen);
  HBRUSH hOldBrush = SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));

  Rectangle(hDC, rc->left, rc->top, rc->right, rc->bottom);

  SelectObject(hDC, hOldPen);
  SelectObject(hDC, hOldBrush);
  DeleteObject(hPen);

  g_hue = fmod(g_hue + 30.0, 360.0);
}


void DrawGroupBox(HDC hDC, GROUPBOX *pGroupBox)
{
  RECT* rc = &pGroupBox->rc;
  PGROUPBOXCAPTIONSTYLE pCaptionStyle;
  PCAPTIONFILLSTYLE pFillStyle;

  pCaptionStyle = pGroupBox->pCaptionStyle;
  pFillStyle = &(pCaptionStyle->captionBkgFill);


  RECT rcCaption = *rc;
  rcCaption.top += 2;
  rcCaption.bottom = rcCaption.top + pCaptionStyle->uHeight;
  rcCaption.left += 2;
  rcCaption.right -= 3;
  // FrameRect(hDC, rc, CreateSolidBrush(RGB(255, 0, 0)));
  DrawEdge(hDC, rc, EDGE_ETCHED, BF_RECT);

  /* Fill caption background */
  /* FillRect(hDC, &rcCaption, CreateSolidBrush(RGB(27, 94, 32))); */

  switch (pFillStyle->uType)
  {
    case FILL_STYLE_SOLID:
      FillRect(hDC, &rcCaption, CreateSolidBrush(pFillStyle->data.dwColor));
      break;
    case FILL_STYLE_GRADIENT:
      {
        PCAPTIONGRADIENT pGradient = &(pFillStyle->data.gradient);

        TRIVERTEX pGradVert[2];
        GRADIENT_RECT mesh;

        pGradVert[0].x = rcCaption.left;
        pGradVert[0].y = rcCaption.top;
        pGradVert[0].Red = ((pGradient->dwColorStart) & 0xFF) << 8;
        pGradVert[0].Green = ((pGradient->dwColorStart >> 8) & 0xFF) << 8;
        pGradVert[0].Blue = ((pGradient->dwColorStart >> 16) & 0xFF) << 8;
        pGradVert[0].Alpha = 0;

        pGradVert[1].x = rcCaption.right;
        pGradVert[1].y = rcCaption.bottom;
        pGradVert[1].Red = ((pGradient->dwColorEnd) & 0xFF) << 8;
        pGradVert[1].Green = ((pGradient->dwColorEnd >> 8) & 0xFF) << 8;
        pGradVert[1].Blue = ((pGradient->dwColorEnd >> 16) & 0xFF) << 8;
        pGradVert[1].Alpha = 0x0000;

        mesh.UpperLeft = 0;
        mesh.LowerRight = 1;

        GradientFill(hDC, pGradVert, 2, &mesh, 1, pGradient->uDirection);
      }
      break;
  }

  assert(pGroupBox->lpszCaption);
  if (pGroupBox->lpszCaption)
  {
    HFONT hFontBold = pCaptionStyle->hFont;

    HFONT hOldFont = SelectObject(hDC, hFontBold);

    RECT rcGroupCaption = *rc;
    rcGroupCaption.top += 2;
    rcGroupCaption.left += 7;
    rcGroupCaption.right -= 7;
    rcGroupCaption.bottom = rcGroupCaption.top + pCaptionStyle->uHeight;

    size_t len = wcslen(pGroupBox->lpszCaption);
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, pCaptionStyle->dwTextColor);

    /* Temporary pszBuffer for overflow ellipsis */
    WCHAR szCaption[80] = { 0 };
    StringCchCopy(szCaption, 80, pGroupBox->lpszCaption);

    DrawText(hDC, szCaption, (int)len, &rcGroupCaption, DT_SINGLELINE | DT_MODIFYSTRING |
        DT_END_ELLIPSIS | DT_VCENTER | DT_TOP);

    SelectObject(hDC, hOldFont);
  }

  // DrawRect(hDC, &pGroupBox->rc);
}

void DrawLayoutBox(HDC hDC, PLAYOUTBOX pLayoutBox)
{
  size_t i;

  for (i = 0; i < pLayoutBox->size; i++)
  {
    LAYOUTBOXCONTENT *pContent = &pLayoutBox->hContent[i];

    if (pContent->type == LAYOUTBOX_TYPE_BOX)
    {
      DrawLayoutBox(hDC, pContent->data.pBox);
    }
    else if (pContent->type == LAYOUTBOX_TYPE_GROUP) {
      DrawGroupBox(hDC, pContent->data.pGroup);
    }

    // DrawRect(hDC, &pLayoutBox->rc);
  }
}
