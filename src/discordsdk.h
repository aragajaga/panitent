#ifndef PANITENT_DISCORDSDK_H_
#define PANITENT_DISCORDSDK_H_

typedef struct _DiscordSDKInstance DiscordSDKInstance;

DiscordSDKInstance* DiscordSDKInit();
void Discord_SetActivityStatus(DiscordSDKInstance* discord, LPCWSTR lpszStatus);

#endif  /* PANITENT_DISCORDSDK_H_ */
