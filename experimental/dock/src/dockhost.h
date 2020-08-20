#ifndef DOCK_DOCKHOST_H_
#define DOCK_DOCKHOST_H_

#include "precomp.h"

typedef struct _dockhost {
  ATOM wndClass;
  HWND hWnd;

} dockhost_t;

extern dockhost_t g_dockhost;

ATOM DockHost_Register(HINSTANCE hInstance);
HWND DockHost_Create(HWND hParent);

#endif  /* DOCK_DOCKHOST_H_ */
