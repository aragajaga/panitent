#include "../precomp.h"

#include "activitysharingmanager.h"
#include "../log.h"

ActivitySharingManager* ActivitySharingManager_Create()
{
    ActivitySharingManager* pActivitySharingManager = (ActivitySharingManager*)malloc(sizeof(ActivitySharingManager));

    if (pActivitySharingManager)
    {
        memset(pActivitySharingManager, 0, sizeof(ActivitySharingManager));
        ActivitySharingManager_Init(pActivitySharingManager);
    }

    return pActivitySharingManager;
}

void ActivitySharingManager_Destroy(ActivitySharingManager* pActivitySharingManager)
{
    if (pActivitySharingManager->pszCurrentStatus)
    {
        free(pActivitySharingManager->pszCurrentStatus);
    }
}

void ActivitySharingManager_Init(ActivitySharingManager* pActivitySharingManager)
{
    ActivitySharingClient** pClients = (ActivitySharingClient**)malloc(sizeof(ActivitySharingClient*) * 16);

    if (pClients)
    {
        memset(pClients, 0, sizeof(ActivitySharingClient*) * 16);
        pActivitySharingManager->pClients = pClients;
    }

    pActivitySharingManager->nCount = 0;
}

void ActivitySharingManager_AddClient(ActivitySharingManager* pActivitySharingManager, ActivitySharingClient* pActivitySharingClient)
{
    pActivitySharingManager->pClients[pActivitySharingManager->nCount++] = pActivitySharingClient;

    pActivitySharingClient->SetStatusMessage(pActivitySharingClient, pActivitySharingManager->pszCurrentStatus);
}

void ActivitySharingManager_SetStatus(ActivitySharingManager* pActivitySharingManager, PCWSTR pszStatusMessage)
{
    LogMessage(LOGENTRY_TYPE_INFO, L"Activity", pszStatusMessage);

    /* Remove last activity status message */
    if (pActivitySharingManager->pszCurrentStatus)
    {
        free(pActivitySharingManager->pszCurrentStatus);
        pActivitySharingManager->pszCurrentStatus = NULL;
    }

    size_t len = wcslen(pszStatusMessage);
    PWSTR pszCurrentStatus = (PWSTR)malloc((len + 1) * sizeof(WCHAR));
    if (pszCurrentStatus)
    {
        memcpy(pszCurrentStatus, pszStatusMessage, (len + 1) * sizeof(WCHAR));
        pszCurrentStatus[len] = L'\0';

        pActivitySharingManager->pszCurrentStatus = pszCurrentStatus;
    }

    for (size_t i = 0; i < pActivitySharingManager->nCount; ++i)
    {
        ActivitySharingClient* pClient = pActivitySharingManager->pClients[i];

        if (pClient)
        {
            pClient->SetStatusMessage(pClient->m_handler, pActivitySharingManager->pszCurrentStatus);
        }
    }
}

PCWSTR ActivitySharingManager_GetStatus(ActivitySharingManager* pActivitySharingManager)
{
    return pActivitySharingManager->pszCurrentStatus;
}
