#pragma once

#include "window.h"

typedef struct ListBoxCtl ListBoxCtl;

struct ListBoxCtl {
    Window base;
};

ListBoxCtl* ListBoxCtl_Create(Application* pApplication);
void ListBoxCtl_Init(ListBoxCtl* pListBoxCtl, Application* pApplication);

void ListBoxCtl_PreCreate(LPCREATESTRUCT lpcs);
