#pragma once

#include "activitysharingclient.h"

typedef struct ActivitySharingManager ActivitySharingManager;
struct ActivitySharingManager {
    ActivitySharingClient** pClients;
    size_t nCount;
    PCWSTR pszCurrentStatus;
    size_t nStatusLength;
};

ActivitySharingManager* ActivitySharingManager_Create();
void ActivitySharingManager_Init(ActivitySharingManager* pActivitySharingManager);
void ActivitySharingManager_AddClient(ActivitySharingManager* pActivitySharingManager, ActivitySharingClient* pActivitySharingClient);
void ActivitySharingManager_SetStatus(ActivitySharingManager* pActivitySharingManager, PCWSTR pszStatusMessage);
PCWSTR ActivitySharingManager_GetStatus(ActivitySharingManager* pActivitySharingManager);
