#pragma once

#include "win32/dialog.h"
#include "win32/propertygrid.h"

typedef struct PropertyGridDialog PropertyGridDialog;
struct PropertyGridDialog {
	Dialog base;
	PropertyGridCtl* m_pPropertyGrid;
};

PropertyGridDialog* PropertyGridDialog_Create();
void PropertyGridDialog_Init(PropertyGridDialog* pPropertyGridDialog);
