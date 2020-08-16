#ifndef PANITENT_H
#define PANITENT_H

#define IDS_STATUS 1330

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


int     APIENTRY    WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void    NewFile();
HMENU   CreateMainMenu();
void    UnregisterClasses();

#endif // PANITENT_H
