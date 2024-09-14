#pragma once

#include "window.h"

typedef struct PropertyGridCtl PropertyGridCtl;

struct PropertyGridCtl {
    Window base;
};

PropertyGridCtl* PropertyGridCtl_Create();
void PropertyGridCtl_Init(PropertyGridCtl* pPropertyGridCtl);

void PropertyGridCtl_PreCreate(LPCREATESTRUCT lpcs);
