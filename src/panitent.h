#pragma once

#include "win32/application.h"
#include "palette.h"

typedef struct PanitentApplication PanitentApplication;
typedef struct PanitentWindow PanitentWindow;
typedef struct GLWindow GLWindow;
typedef struct ToolboxWindow ToolboxWindow;
typedef struct OptionBarWindow OptionBarWindow;
typedef struct Tool Tool;
typedef struct TreeNode TreeNode;
typedef struct ViewportWindow ViewportWindow;
typedef struct WorkspaceContainer WorkspaceContainer;

struct PanitentApplication {
  struct Application base;

  Palette* palette;

  PanitentWindow* m_pPanitentWindow;

  struct PaletteWindow* paletteWindow;
  GLWindow* glWindow;
  ToolboxWindow* m_pToolboxWindow;
  OptionBarWindow* m_pOptionBarWindow;
  WorkspaceContainer* m_pWorkspaceContainer;
  ViewportWindow* m_pViewportWindow;

  Tool* m_pTool;
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
#ifdef HAS_DISCORDSDK
  DiscordSDKInstance *discord;
#endif /* USE_DISCORDSDK */

  PNTSETTINGS settings;
} Panitent;

extern Panitent g_panitent;

void Win32_ApplyUIFont(HWND hwnd);
HFONT GetGuiFont();

typedef struct ViewportWindow ViewportWindow;
typedef struct DockHostWindow DockHostWindow;

Document* Panitent_GetActiveDocument();
void Panitent_SetActiveViewport(ViewportWindow* pViewportWindow);
ViewportWindow* Panitent_GetActiveViewport();
HWND Panitent_GetHWND();
PNTSETTINGS* Panitent_GetSettings();
void Panitent_Open();
void Panitent_OpenFile(LPWSTR);
ViewportWindow* Panitent_CreateViewport();
void Panitent_ClipboardExport();
void Panitent_DockHostInit(DockHostWindow* pDockHostWindow, TreeNode* pNode);

void Panitent_CmdSaveFile(struct PanitentApplication* app);
void Panitent_CmdClearCanvas(struct PanitentApplication* app);
PanitentApplication* Panitent_GetApp();

WorkspaceContainer* Panitent_GetWorkspaceContainer();

Tool* Panitent_GetTool();
