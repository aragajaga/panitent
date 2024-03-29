#ifndef PANITENT_PANITENT_H
#define PANITENT_PANITENT_H

#include "win32/application.h"
#include "palette.h"
#include "palette_window.h"

struct PanitentApplication {
  struct Application base;
  Palette* palette;
  struct PaletteWindow* paletteWindow;
};

void PanitentApplication_Init(struct PanitentApplication*);
struct PanitentApplication* PanitentApplication_Create();

#ifdef HAS_DISCORDSDK
#include "discordsdk.h"
#endif /* HAS_DISCORDSDK */

#include "settings.h"

typedef struct _Document Document;
typedef struct _Viewport Viewport;

typedef struct _Panitent {
  HINSTANCE hInstance;
  HWND hwnd;
  Viewport* activeViewport;
#ifdef HAS_DISCORDSDK
  DiscordSDKInstance *discord;
#endif /* USE_DISCORDSDK */

  PNTSETTINGS settings;
} Panitent;

extern Panitent g_panitent;

void SetGuiFont(HWND hwnd);
HFONT GetGuiFont();

Document* Panitent_GetActiveDocument();
void Panitent_SetActiveViewport(Viewport* viewport);
Viewport* Panitent_GetActiveViewport();
HWND Panitent_GetHWND();
PNTSETTINGS* Panitent_GetSettings();
void Panitent_Open();
void Panitent_OpenFile(LPWSTR);
Viewport* Panitent_CreateViewport();
void Panitent_ClipboardExport();

#endif /* PANITENT_PANITENT_H */
