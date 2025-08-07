#pragma once

#include "dock_system.h"

// Functions for creating and destroying DockSite and associated hierarchy
DockSite* DockSite_Create(HWND hWndOwner);
void DockSite_Destroy(DockSite* pSite);

// Helpers exposed for DockManager to clean up hierarchy
void DockPane_Destroy(DockPane* pPane);
void DockGroup_DestroyRecursive(DockGroup* pGroup);
