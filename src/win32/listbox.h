#pragma once

#include "window.h"

typedef struct ListBoxCtl ListBoxCtl;

struct ListBoxCtl {
    Window base;
};

ListBoxCtl* ListBoxCtl_Create();
void ListBoxCtl_Init(ListBoxCtl* pListBoxCtl);

void ListBoxCtl_PreCreate(LPCREATESTRUCT lpcs);
