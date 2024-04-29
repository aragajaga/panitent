#pragma once

typedef struct DiscordASP DiscordASP;
struct DiscordASP {
    ActivitySharingClient* base;
};

DiscordASP* DiscordASP_Create();
void DiscordASP_Init(DiscordASP* pDiscordASP);
void DiscordASP_SetStatusMessage(LPCWSTR lpszStatusMessage);
