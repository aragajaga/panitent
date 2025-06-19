#ifndef _TEST_APPLICATION_H
#define _TEST_APPLICATION_H

#include "win32/application.h"
#include "palette.h"
#include "settings.h"
#include "appcmd.h"

typedef struct _PanitentWindow PanitentWindow;
typedef struct Tool Tool;
typedef struct WorkspaceContainer WorkspaceContainer;
typedef struct PaletteWindow PaletteWindow;
typedef struct _DockHostWindow DockHostWindow;
typedef struct TreeNode TreeNode;
typedef struct ActivitySharingManager ActivitySharingManager;
typedef struct ViewportWindow ViewportWindow;
typedef struct _Document Document;
typedef struct ShapeContext ShapeContext;

typedef struct PanitentApp PanitentApp;
struct PanitentApp {
    Application base;

    PanitentWindow* pPanitentWindow;            /* Application main frame */
    PaletteWindow* m_pPaletteWindow;            /* Palette window */    
    WorkspaceContainer* m_pWorkspaceContainer;  /* Workspace container */
    Tool* m_pTool;                              /* Current selected tool */
    Palette* palette;                           /* Palette */
    ActivitySharingManager* m_pActivitySharingManager;  /* Activity sharing manager */
    HFONT m_hFont;
    ViewportWindow* m_pViewportWindow;
    PNTSETTINGS m_settings;
    ShapeContext* m_pShapeContext;
    AppCmd m_appCmd;
};

void PanitentApp_Init(PanitentApp* pPanitentApp);
PanitentApp* PanitentApp_Create();
int PanitentApp_Run(PanitentApp* pPanitentApp);

void PanitentApp_DockHostInit(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow); // TreeNode* pNodeParent removed
void PanitentApp_SetActivityStatus(PanitentApp* pPanitentApp, PCWSTR pszStatusText);
Palette* PanitentApp_GetPalette(PanitentApp* pPanitentApp);
PanitentApp* PanitentApp_Instance();
HFONT PanitentApp_GetUIFont(PanitentApp* pPanitentApp);

PNTSETTINGS* PanitentApp_GetSettings(PanitentApp* pPanitentApp);

void PanitentApp_SetActiveViewport(PanitentApp* pPanitentApp, ViewportWindow* pViewportWindow);
ViewportWindow* PanitentApp_GetActiveViewport(PanitentApp* pPanitentApp);

Document* PanitentApp_GetActiveDocument(PanitentApp* pPanitentApp);

ActivitySharingManager* PanitentApp_GetActivitySharingManager(PanitentApp* pPanitentApp);

ViewportWindow* PanitentApp_GetCurrentViewport(PanitentApp* pPanitentApp);

PanitentWindow* PanitentApp_GetWindow(PanitentApp* pPanitentApp);

WorkspaceContainer* PanitentApp_GetWorkspaceContainer(PanitentApp* pPanitentApp);

void PanitentApp_SetTool(PanitentApp* pPanitentApp, Tool* pTool);
Tool* PanitentApp_GetTool(PanitentApp* pPanitentApp);

ShapeContext* PanitentApp_GetShapeContext(PanitentApp* pPanitentApp);

/* Commands */
void PanitentApp_CmdNewFile(PanitentApp* pPanitentApp);
void PanitentApp_CmdOpenFile(PanitentApp* pPanitentApp);
void PanitentApp_CmdSaveFile(PanitentApp* pPanitentApp);
void PanitentApp_CmdClearCanvas(PanitentApp* pPanitentApp);
void PanitentApp_CmdShowActivityDialog(PanitentApp* pPanitentApp);
void PanitentApp_CmdShowPropertyGridDialog(PanitentApp* pPanitentApp);
void PanitentApp_CmdShowLog(PanitentApp* pPanitentApp);
void PanitentApp_CmdShowRbTreeViz(PanitentApp* pPanitentApp);
void PanitentApp_CmdClipboardExport(PanitentApp* pPanitentApp);
void PanitentApp_CmdShowSettings(PanitentApp* pPanitentApp);
void PanitentApp_CmdDisplayPixelBuffer(PanitentApp* pPanitentApp);
void PanitentApp_CmdRunScript(PanitentApp* pPanitentApp);
void PanitentApp_CmdBinView(PanitentApp* pPanitentApp);

#endif  /* _TEST_APPLICATION_H */
