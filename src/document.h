#pragma once

typedef struct _History History;
typedef struct _Canvas Canvas;
typedef struct WorkspaceContainer WorkspaceContainer;

typedef struct _Document {
  char* location;
  Canvas* canvas;
  LPWSTR szFilePath;
  History* history;
} Document;

Document* Document_Create();
Document* Document_CreateWithCanvas(Canvas* pCanvas);
void Document_Save(Document* doc);
void Document_Open(Document* doc);
void Document_OpenFile(LPWSTR);
Document* Document_LoadFile(PCWSTR pszPath);
BOOL Document_OpenFileInWorkspace(PCWSTR pszPath, WorkspaceContainer* pWorkspaceContainer);
BOOL Document_AttachToWorkspace(Document* pDocument, WorkspaceContainer* pWorkspaceContainer);
void Document_Destroy(Document* doc);
BOOL Document_Close(Document* doc);
void Document_Purge(Document* doc);
Document* Document_New(int width, int height);
BOOL Document_IsFilePathSet(Document* doc);
History* Document_GetHistory(Document* doc);
Canvas* Document_GetCanvas(Document* doc);
PCWSTR Document_GetFilePath(Document* document);
