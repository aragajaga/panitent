#include "precomp.h"

#include "dockhostmetrics.h"

static int s_iCaptionHeight = 14;
static int s_iBorderWidth = 4;
static int s_iZoneTabGutter = 24;
static int s_iZoneTabGutterLeft = 0;
static int s_iZoneTabGutterRight = 0;
static int s_iZoneTabGutterTop = 0;
static int s_iZoneTabGutterBottom = 0;

int DockHostMetrics_GetCaptionHeight(void)
{
    return s_iCaptionHeight;
}

int DockHostMetrics_GetBorderWidth(void)
{
    return s_iBorderWidth;
}

int DockHostMetrics_GetZoneTabGutter(void)
{
    return s_iZoneTabGutter;
}

void DockHostMetrics_GetRootGutters(int* pLeft, int* pRight, int* pTop, int* pBottom)
{
    if (pLeft) *pLeft = s_iZoneTabGutterLeft;
    if (pRight) *pRight = s_iZoneTabGutterRight;
    if (pTop) *pTop = s_iZoneTabGutterTop;
    if (pBottom) *pBottom = s_iZoneTabGutterBottom;
}

void DockHostMetrics_SetRootGutters(int iLeft, int iRight, int iTop, int iBottom)
{
    s_iZoneTabGutterLeft = iLeft;
    s_iZoneTabGutterRight = iRight;
    s_iZoneTabGutterTop = iTop;
    s_iZoneTabGutterBottom = iBottom;
}
