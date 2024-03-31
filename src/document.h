#pragma once

typedef struct _History History;
typedef struct _Canvas Canvas;

typedef struct _Document {
  char* location;
  Canvas* canvas;
  LPWSTR szFilePath;
  History* history;
} Document;

void Document_Save(Document* doc);
void Document_Open(Document* doc);
void Document_OpenFile(LPWSTR);
BOOL Document_Close(Document* doc);
void Document_Purge(Document* doc);
Document* Document_New(int width, int height);
BOOL Document_IsFilePathSet(Document* doc);
History* Document_GetHistory(Document* doc);
Canvas* Document_GetCanvas(Document* doc);
