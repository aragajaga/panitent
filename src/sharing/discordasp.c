#include "../precomp.h"
#include "activitysharingclient.h"

#include "discordasp.h"

#include "../panitent.h"
#include "discordsdk.h"

DiscordASP* DiscordASP_Create()
{
    DiscordASP* pDiscordASP = (DiscordASP*)malloc(sizeof(DiscordASP));
    
    if (pDiscordASP)
    {
        memset(pDiscordASP, 0, sizeof(DiscordASP));

        DiscordASP_Init(pDiscordASP);
    }

    return pDiscordASP;
}

void DiscordASP_Init(DiscordASP* pDiscordASP)
{
    pDiscordASP->base->SetStatusMessage = DiscordASP_SetStatusMessage;
}

void DiscordASP_SetStatusMessage(LPCWSTR lpszStatusMessage)
{
#ifdef HAS_DISCORDSDK
    Discord_SetActivityStatus(g_panitent.discord, lpszStatusMessage);
#endif /* HAS_DISCORDSDK */
}
