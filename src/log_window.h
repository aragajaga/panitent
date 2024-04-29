#ifndef PANITENT_LOG_WINDOW_H
#define PANITENT_LOG_WINDOW_H

#include"win32/window.h"

typedef struct LogWindow LogWindow;

struct LogWindow {
    Window base;

    HWND hToolbar;
    HWND hList;
    int nIDLogObserver;
};

LogWindow* LogWindow_Create(Application* pApplication);

#endif  /* PANITENT_LOG_WINDOW_H */
