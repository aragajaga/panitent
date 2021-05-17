#ifndef PANITENT_DOCUMENT_H_
#define PANITENT_DOCUMENT_H_

#include "precomp.h"
#include "canvas.h"
#include "layers.h"

typedef struct _Document Document;
typedef struct _LayerManager LayerManager;

void Document_Save(Document*);
void Document_Open(Document*);
BOOL Document_Close(Document*);
void Document_Purge(Document*);
Document* Document_New(int, int);
BOOL Document_IsFilePathSet(Document*);
void Document_SetCanvas(Document*, Canvas*);
Canvas* Document_GetCanvas(Document*);
Rect Document_GetSize(Document*);
LayerManager* Document_GetLayerManager(Document*);

#endif /* PANITENT_DOCUMENT_H_ */
