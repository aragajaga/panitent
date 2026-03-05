#pragma once

#include "precomp.h"

#ifndef DGA_START
#define DGA_START 0
#endif

#ifndef DGA_END
#define DGA_END 1
#endif

#ifndef DGP_ABSOLUTE
#define DGP_ABSOLUTE 0
#endif

#ifndef DGP_RELATIVE
#define DGP_RELATIVE 2
#endif

#ifndef DGD_HORIZONTAL
#define DGD_HORIZONTAL 0
#endif

#ifndef DGD_VERTICAL
#define DGD_VERTICAL 4
#endif

#ifndef DKS_NONE
#define DKS_NONE 0
#endif

#ifndef DKS_LEFT
#define DKS_LEFT 1
#endif

#ifndef DKS_RIGHT
#define DKS_RIGHT 2
#endif

#ifndef DKS_TOP
#define DKS_TOP 3
#endif

#ifndef DKS_BOTTOM
#define DKS_BOTTOM 4
#endif

#define DOCKLAYOUT_ZONE_TAB_THICKNESS 24
#define DOCKLAYOUT_ZONE_TAB_LENGTH 92
#define DOCKLAYOUT_ZONE_TAB_GAP 2
#define DOCKLAYOUT_ZONE_TAB_INSET DOCKLAYOUT_ZONE_TAB_THICKNESS

DWORD DockLayout_GetZoneStackStyle(int nDockSide);
int DockLayout_GetZoneStackGrip(int nDockSide, int iDockSize);
int DockLayout_GetZoneSplitGrip(int nDockSide, int iDockSize);
BOOL DockLayout_GetZoneTabRect(const RECT* pClientRect, int nDockSide, int iTabIndex, int nTabs, RECT* pRect);
BOOL DockLayout_GetDockPreviewRect(const RECT* pHostRect, int nDockSide, RECT* pRect);
