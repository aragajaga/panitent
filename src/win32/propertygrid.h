#pragma once

#include "window.h"

typedef struct PropertyGridCtl PropertyGridCtl;

struct PropertyGridCtl {
    Window base;
};

PropertyGridCtl* PropertyGridCtl_Create(Application* pApplication);
void PropertyGridCtl_Init(PropertyGridCtl* pPropertyGridCtl, Application* pApplication);

void PropertyGridCtl_PreCreate(LPCREATESTRUCT lpcs);
