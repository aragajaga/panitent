#ifndef PANITENT_PLAYGROUND_H
#define PANITENT_PLAYGROUND_H

#include "win32/dialog.h"
#include "util/vector.h"

typedef struct IAbstractCanvas IAbstractCanvas;
typedef struct IAbstractDrawer IAbstractDrawer;
typedef struct IWidget IWidget;
typedef struct IWidget_vtbl IWidget_vtbl;
typedef struct HSLGradient HSLGradient;

struct IWidget_vtbl {
    void (__fastcall *Draw)(void* pThis, IAbstractCanvas* pTarget, IAbstractDrawer* pDrawer);
    void (__fastcall *GetBoundingRect)(void* pThis, RECT* prc);
    BOOL (__fastcall *OnLButtonDown)(void* pThis, int x, int y);
    BOOL (__fastcall *OnLButtonUp)(void* pThis, int x, int y);
    BOOL (__fastcall *OnMouseMove)(void* pThis, int x, int y);
};

struct IWidget {
    IWidget_vtbl* __vtbl;
};

typedef struct HSLGradient {
    IWidget iwidget;
    RECT rc;
    float hue;
    float lightness;
    BOOL fDrag;
} HSLGradient;

typedef struct PlaygroundDlg PlaygroundDlg;
struct PlaygroundDlg {
    Dialog base;
    Vector widgets;
    HSLGradient gradient;
};

void PlaygroundDlg_Init(PlaygroundDlg* pPlaygroundDlg);

#endif  /* PANITENT_PLAYGROUND_H */
