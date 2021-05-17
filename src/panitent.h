#ifndef PANITENT_PANITENT_H
#define PANITENT_PANITENT_H

#define IDS_STATUS 1330

#include "document.h"

enum {
  IDM_FILE_NEW = 1001,
  IDM_FILE_OPEN,
  IDM_FILE_SAVE,
  IDM_FILE_CLOSE,

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

void NewFile();
HMENU CreateMainMenu();
void UnregisterClasses();
void SetGuiFont(HWND hwnd);
HFONT GetGuiFont();
Document* GetActiveDocument();
void SetActiveDocument(Document*);
HWND Panitent_GetHWND();

extern const WCHAR szAppName[];

#endif /* PANITENT_PANITENT_H */
