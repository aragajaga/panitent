#ifndef PANITENT_PANITENT_H
#define PANITENT_PANITENT_H

#ifdef HAS_DISCORDSDK
#include "discordsdk.h"
#endif /* HAS_DISCORDSDK */

typedef struct _Document Document;
typedef struct _Viewport Viewport;

typedef struct _Panitent {
  HINSTANCE hInstance;
  HWND hwnd;
  Viewport* activeViewport;
#ifdef HAS_DISCORDSDK
  DiscordSDKInstance *discord;
#endif /* USE_DISCORDSDK */
} Panitent;

extern Panitent g_panitent;

void SetGuiFont(HWND hwnd);
HFONT GetGuiFont();

Document* Panitent_GetActiveDocument();
void Panitent_SetActiveViewport(Viewport* viewport);
Viewport* Panitent_GetActiveViewport();

#endif /* PANITENT_PANITENT_H */
