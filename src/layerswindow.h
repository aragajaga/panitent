#pragma once
#pragma once

#include "win32/window.h"
#include "win32/listbox.h"

typedef struct LayersWindow LayersWindow;
struct LayersWindow {
    Window base;

    ListBoxCtl* m_pListBoxCtl;
};

LayersWindow* LayersWindow_Create(Application* app);
void LayersWindow_Init(LayersWindow* pLayersWindow, Application* app);
