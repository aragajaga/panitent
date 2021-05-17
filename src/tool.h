#ifndef PANITENT_TOOL_H
#define PANITENT_TOOL_H

#include "precomp.h"
#include "panitent.h"
#include "viewport.h"

typedef struct _Tool Tool;

extern Tool* g_tool;

typedef enum {
  E_TOOL_POINTER,
  E_TOOL_PENCIL,
  E_TOOL_CIRCLE,
  E_TOOL_LINE,
  E_TOOL_RECTANGLE,
  E_TOOL_TEXT,
  E_TOOL_FILL,
  E_TOOL_PICKER,
  E_TOOL_BREAKER,
  E_TOOL_PEN,
  E_TOOL_BRUSH
} DefaultTool;


Tool* Tool_New(LPWSTR, int, void (*evtProc)(Viewport*, UINT, WPARAM, LPARAM));
Tool* GetSelectedTool();
void SelectTool(Tool*);
int Tool_GetImg(Tool*);
void Tool_ProcessEvent(Tool* tool, Viewport*, UINT, WPARAM, LPARAM);

void InitDefaultTools();
Tool* GetDefaultTool(DefaultTool);

#endif /* PANITENT_TOOL_H */
