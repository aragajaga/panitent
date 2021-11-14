#ifndef DOCK_DOCKHOST_H_
#define DOCK_DOCKHOST_H_

#include "precomp.h"

typedef struct _dockhost {
  ATOM wndClass;
  HWND hWnd;

} dockhost_t;

extern dockhost_t g_dockhost;

typedef enum {
  DOCK_RIGHT = 1,
  DOCK_TOP,
  DOCK_LEFT,
  DOCK_BOTTOM,
} dock_side_e;

/* TODO Use significant bit */
typedef enum {
  GRIP_ALIGN_START,
  GRIP_ALIGN_END,
} grip_align_e;

typedef enum {
  GRIP_POS_UNIFORM,
  GRIP_POS_ABSOLUTE,
  GRIP_POS_RELATIVE
} grip_pos_type_e;

typedef enum {
  SPLIT_DIRECTION_HORIZONTAL,
  SPLIT_DIRECTION_VERTICAL
} split_direction_e;

typedef struct _dock_window {
  RECT rc;
  RECT pins;
  RECT undockedRc;
  HWND hwnd;
  LPWSTR caption;
  BOOL fDock;
} dock_window_t;

struct _binary_tree {
  struct _binary_tree* node1;
  struct _binary_tree* node2;
  int delimPos;
  RECT rc;
  LPWSTR lpszCaption;
  grip_pos_type_e gripPosType;
  float fGrip;
  grip_align_e gripAlign;
  int posFixedGrip;
  BOOL bShowCaption;
  HWND hwnd;
  split_direction_e splitDirection;
};

typedef struct _binary_tree binary_tree_t;

extern dock_side_e g_dock_side;
extern dock_side_e eSuggest;
extern binary_tree_t* root;

BOOL DockHost_RegisterClass(HINSTANCE hInstance);
HWND DockHost_Create(HWND hParent);
void DockNode_arrange(binary_tree_t*);

#endif /* DOCK_DOCKHOST_H_ */
