#ifndef PANITENT_PANITENT_H
#define PANITENT_PANITENT_H

#ifdef HAS_DISCORDSDK
#include "discordsdk.h"
#endif /* HAS_DISCORDSDK */

#define IDS_STATUS 1330

enum {
  IDM_FILE_NEW = 1001,
  IDM_FILE_OPEN,
  IDM_FILE_SAVE,
  IDM_FILE_CLOSE,

  IDM_EDIT_UNDO,
  IDM_EDIT_REDO,
  IDM_EDIT_TESTFILL,
  IDM_EDIT_CLRCANVAS,
  IDM_EDIT_WU_LINES,

  IDM_WINDOW_TOOLS,

  IDM_OPTIONS_SETTINGS,

  IDM_HELP_TOPICS,
  IDM_HELP_ABOUT
};

typedef struct _panitent {
  HWND hwnd_main;
#ifdef HAS_DISCORDSDK
  DiscordSDKInstance *discord;
#endif /* USE_DISCORDSDK */
} panitent_t;

extern panitent_t g_panitent;

typedef struct _mouseevent {
  HWND hwnd;
  LPARAM lParam;
} MOUSEEVENT;

int APIENTRY WinMain(HINSTANCE hInst,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct _Document Document;

void NewFile();
HMENU CreateMainMenu();
void UnregisterClasses();
void SetGuiFont(HWND hwnd);
HFONT GetGuiFont();
Document* Panitent_GetActiveDocument();

typedef struct _Viewport Viewport;
void Panitent_SetActiveViewport(Viewport* viewport);
Viewport* Panitent_GetActiveViewport();

#endif /* PANITENT_PANITENT_H */
