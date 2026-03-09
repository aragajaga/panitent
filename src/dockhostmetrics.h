#pragma once

int DockHostMetrics_GetCaptionHeight(void);
int DockHostMetrics_GetBorderWidth(void);
int DockHostMetrics_GetZoneTabGutter(void);
void DockHostMetrics_GetRootGutters(int* pLeft, int* pRight, int* pTop, int* pBottom);
void DockHostMetrics_SetRootGutters(int iLeft, int iRight, int iTop, int iBottom);
