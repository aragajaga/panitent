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

typedef struct _tagTOOLBOXICONTHEME {
  LPWSTR lpszName;
  LPWSTR lpszResource;
} TOOLBOXICONTHEME, *PTOOLBOXICONTHEME;

BOOL Toolbox_RegisterClass(HINSTANCE);
void Toolbox_UnregisterClass();

#endif /* PANITENT_TOOLBOX_H */
