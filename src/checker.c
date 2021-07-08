#include "precomp.h"

#include "checker.h"

HBRUSH CreateChecker(CheckerConfig* config, HDC hdc)
{
  assert(config);

  unsigned int checkerSize = config->tileSize;
  COLORREF checkerColor1 = config->color[0] & 0x00FFFFFF;
  COLORREF checkerColor2 = config->color[1] & 0x00FFFFFF;

  HBITMAP hPatBmp = CreateCompatibleBitmap(hdc, checkerSize, checkerSize);

  HDC hPatDC = CreateCompatibleDC(hdc);
  SelectObject(hPatDC, hPatBmp);

  HBRUSH hbr1 = CreateSolidBrush(checkerColor1);
  HBRUSH hbr2 = CreateSolidBrush(checkerColor2);

  RECT rcBack = { 0, 0, checkerSize, checkerSize };
  RECT rcLump1 = { 0, 0, checkerSize / 2, checkerSize / 2 };
  RECT rcLump2 = { checkerSize / 2, checkerSize / 2, checkerSize,
      checkerSize };

  FillRect(hPatDC, &rcBack, hbr2);
  FillRect(hPatDC, &rcLump1, hbr1);
  FillRect(hPatDC, &rcLump2, hbr1);

  DeleteObject(hbr1);
  DeleteObject(hbr2);

  HBRUSH hbrChecker = CreatePatternBrush(hPatBmp);
  assert(hbrChecker);

  return hbrChecker;
}
