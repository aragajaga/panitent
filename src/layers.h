#ifndef LAYERS_H
#define LAYERS_H

#include "document.h"

typedef struct _Document Document;

typedef struct _Layer Layer;
typedef struct _LayerManager LayerManager;

BOOL Layers_Register(HINSTANCE);
HWND Layers_Create(HWND);

LayerManager* LayerManager_Create(Document*);

#endif  /* LAYERS_H */
