#pragma once

#include "../win32/dialog.h"
#include "activitysharingclient.h"

typedef struct ActivityStubDialog ActivityStubDialog;
struct ActivityStubDialog {
    Dialog base;
    ActivitySharingClient m_activitySharingClient;
};

ActivityStubDialog* ActivityStubDialog_Create(Application* pApp);
void ActivityStubDialog_Init(ActivityStubDialog* pActivityStubDialog, Application* pApp);
