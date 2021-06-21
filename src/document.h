#ifndef PANITENT_DOCUMENT_H_
#define PANITENT_DOCUMENT_H_

#include "precomp.h"
#include "canvas.h"

typedef struct _Document {
  char* location;
  Canvas* canvas;
  LPWSTR szFilePath;
} Document;

void Document_Save(Document* doc);
void Document_Open(Document* doc);
BOOL Document_Close(Document* doc);
void Document_Purge(Document* doc);
Document* Document_New(int width, int height);
BOOL Document_IsFilePathSet(Document* doc);

#endif /* PANITENT_DOCUMENT_H_ */
