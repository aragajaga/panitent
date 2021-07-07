#include "precomp.h"

#include "discordsdk.h"

#include <discord_game_sdk.h>

#include <strsafe.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define DISCORD_REQUIRE(x) assert(x == DiscordResult_Ok)

#define DISCORD_APPLICATION_ID 861979642411483186

uint64_t GetSystemTimeAsUnixTime()
{
  const uint64_t unixTimeStart = 0x019DB1DED53E8000;
  const uint64_t tps = 10000000;

  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);

  LARGE_INTEGER li;
  li.LowPart  = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;

  return (li.QuadPart - unixTimeStart) / tps;
}

CRITICAL_SECTION g_csDiscord;

typedef struct _DiscordSDKInstance {
  struct IDiscordCore* core;
  struct IDiscordUserManager* users;
  struct IDiscordAchievementManager* achievements;
  struct IDiscordActivityManager* activities;
  struct IDiscordRelationshipManager* relationships;
  struct IDiscordApplicationManager* application;
  struct IDiscordLobbyManager* lobbies;
  struct IDiscordUserEvents users_events;
  struct IDiscordActivityEvents activities_events;
  struct IDiscordRelationshipEvents relationships_events;
  DiscordUserId user_id;
} DiscordSDKInstance;

DiscordSDKInstance g_discordInstance;

DWORD WINAPI DiscordSDKThreadProc(void* param);

void UpdateActivityCallback(void* data, enum EDiscordResult result)
{
  if (result != DiscordResult_Ok)
  {
    WCHAR szError[120];
    StringCchPrintf(szError, 120, L"IDiscordActivityManager->update_activity failed with: %d\n", result);
    OutputDebugString(szError);
  }

  DISCORD_REQUIRE(result);
}

void OnUserUpdated(void* data)
{
  DiscordSDKInstance *discord = (DiscordSDKInstance*) data;
  struct DiscordUser user;

  discord->users->get_current_user(discord->users, &user);
  discord->user_id = user.id;
}

void OnOAuth2Token(void* data, enum EDiscordResult result,
    struct DiscordOAuth2Token* token)
{
  WCHAR szOAuth2Info[255] = L"";

  if (result == DiscordResult_Ok)
  {
    StringCchPrintf(szOAuth2Info, 255, L"OAuth2 token: %S\n", token->access_token);
  }
  else {
    StringCchPrintf(szOAuth2Info, 255, L"GetOAuthToken failed with %d\n", (int)result);
  }

  OutputDebugString(szOAuth2Info);
}

DiscordSDKInstance* DiscordSDKInit()
{
  DiscordSDKInstance *discord = malloc(sizeof(DiscordSDKInstance));
  ZeroMemory(discord, sizeof(DiscordSDKInstance));

  ZeroMemory(&discord->users_events, sizeof(struct IDiscordUserEvents));
  ZeroMemory(&discord->activities_events, sizeof(struct IDiscordActivityEvents));

  discord->users_events.on_current_user_update = OnUserUpdated;

  struct DiscordCreateParams params;
  DiscordCreateParamsSetDefault(&params);
  params.client_id = DISCORD_APPLICATION_ID;
  params.flags = DiscordCreateFlags_Default;
  params.event_data = discord;
  params.activity_events = &discord->activities_events;
  params.user_events = &discord->users_events;
  DISCORD_REQUIRE(DiscordCreate(DISCORD_VERSION, &params, &discord->core));

  discord->users = discord->core->get_user_manager(discord->core);
  discord->activities = discord->core->get_activity_manager(discord->core);
  discord->application = discord->core->get_application_manager(discord->core);
  discord->application->get_oauth2_token(discord->application, discord,
      OnOAuth2Token);

  DiscordBranch branch;
  discord->application->get_current_branch(discord->application, &branch);
  WCHAR szDiscordBranch[80];
  StringCchPrintf(szDiscordBranch, 255, L"Current branch %S\n",
      branch);
  OutputDebugString(szDiscordBranch);

  discord->relationships = discord->core->get_relationship_manager(
      discord->core);

  InitializeCriticalSection(&g_csDiscord);
  HANDLE hDiscordSDKThread = CreateThread(NULL, 0, DiscordSDKThreadProc,
      discord, 0, NULL);

  return discord;
}

void Discord_SetActivityStatus(DiscordSDKInstance* discord, LPCWSTR lpszStatus)
{
  assert(discord);

  struct DiscordActivity activity;
  ZeroMemory(&activity, sizeof(struct DiscordActivity));

  WideCharToMultiByte(CP_UTF8, 0, lpszStatus, 128, activity.details, 128, NULL,
      NULL);
  StringCchPrintfA(activity.assets.large_image, 80, "panitent");

  activity.timestamps.start = GetSystemTimeAsUnixTime();

  EnterCriticalSection(&g_csDiscord);
  discord->activities->update_activity(discord->activities,
      &activity, discord, UpdateActivityCallback);
  LeaveCriticalSection(&g_csDiscord);
}

DWORD WINAPI DiscordSDKThreadProc(void* param)
{
  DiscordSDKInstance *discord = (DiscordSDKInstance*) param;
  assert(discord);

  for (;;)
  {
    EnterCriticalSection(&g_csDiscord);
    DISCORD_REQUIRE(discord->core->run_callbacks(discord->core));
    LeaveCriticalSection(&g_csDiscord);

    Sleep(16);
  }

  return 0;
}
