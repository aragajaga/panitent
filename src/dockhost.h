#ifndef DOCK_DOCKHOST_H_
#define DOCK_DOCKHOST_H_

#include "precomp.h"

typedef enum {
  E_DOCK_CONTAINER,
  E_DOCK_WINDOW
} DockType;

typedef enum {
  E_CONTAINER_SPLIT,
  E_CONTAINER_TAB
} DockContainerType;

typedef enum {
  E_DIRECTION_HORIZONTAL,
  E_DIRECTION_VERTICAL
} DockSplitDirection;

typedef enum {
  E_ALIGN_START,
  E_ALIGN_END
} DockSplitAlign;

typedef struct _Rect Rect;
typedef struct _DockBase DockBase;
typedef struct _DockContainer DockContainer;
typedef struct _DockWindow DockWindow;
typedef struct _DockHost DockHost;
typedef struct _DockDrawCtx DockDrawCtx;

HWND DockHost_GetHWND(const DockHost*);
DockWindow* DockWindow_Create(HWND hwnd);
DockContainer* DockContainer_Create(int, DockSplitDirection, DockSplitAlign);
void DockContainer_Attach(DockContainer*, DockBase*);
BOOL DockHost_Register(HINSTANCE hInstance);
const DockHost* DockHost_Create(HWND hParent, DockBase*);

#endif /* DOCK_DOCKHOST_H_ */
