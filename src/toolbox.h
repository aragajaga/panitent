#ifndef PANITENT_TOOLBOX_H
#define PANITENT_TOOLBOX_H

#include "precomp.h"
#include "panitent.h"
#include "tool.h"

#define TOOLBOX_WC L"ToolBoxClass"

typedef struct _Toolbox {
  Tool* tools;
  unsigned int tool_count;
  HWND hwnd;
} Toolbox;

void Toolbox_RegisterClass();
void Toolbox_UnregisterClass();

#endif /* PANITENT_TOOLBOX_H */
