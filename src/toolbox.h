#ifndef PANITENT_TOOLBOX_H
#define PANITENT_TOOLBOX_H

#include "precomp.h"
#include "panitent.h"
#include "tool.h"

typedef struct _Toolbox Toolbox;

BOOL Toolbox_Register(HINSTANCE);
HWND Toolbox_Create(HWND);

#endif /* PANITENT_TOOLBOX_H */
