#include "precomp.h"

#include "document.h"
#include "viewport.h"
#include "canvas.h"
#include "panitent.h"
#include "dockhost.h"
#include "file_open.h"
#include "crefptr.h"
#include "wic.h"
#include <stdio.h>
#include <assert.h>
#include "commontypes.h"
#include "layers.h"

struct _Document {
  LPWSTR lpszLocation;
  Canvas* canvas;
  LayerManager* layers;
  Rect size;
};

BOOL Document_IsFilePathSet(Document* doc)
{
  return doc->lpszLocation != NULL;
}

void Document_Open(Document* prevDoc)
{
  crefptr_t* s = init_open_file_dialog();

  LPWSTR pszFileName = (LPWSTR)crefptr_get(s);
  MessageBox(NULL, pszFileName, L"Open", MB_OK);
  ImageBuffer ib = ImageFileReader(pszFileName);
  
  Document* doc = calloc(1, sizeof(Document));

  Canvas* canvas = Canvas_New(ib.width, ib.height);
  doc->canvas = canvas;

  crefptr_deref(s);
}

void Document_Save(Document* document)
{
  crefptr_t* s = init_save_file_dialog();

  Canvas* canvas = Document_GetCanvas(document);
  Rect rc = Canvas_GetSize(canvas);

  size_t bufSize;
  const void* lpBits = Canvas_GetBuffer(canvas, &bufSize);

  ImageBuffer ib  = {0};
  ib.width        = rc.right;
  ib.height       = rc.bottom;
  ib.bits         = (void*)lpBits;
  ib.size         = bufSize;

  ImageFileWriter(crefptr_get(s), ib);
  crefptr_deref(s);
}

void Document_Purge(Document* document)
{
  Canvas_Delete(document->canvas);
}

BOOL Document_Close(Document* document)
{
  int answer = MessageBox(NULL, L"Do you want to save changes?", szAppName,
      MB_YESNOCANCEL | MB_ICONWARNING);

  switch (answer) {
    case IDYES:
      Document_Save(document);
      break;
    case IDNO:
      Document_Purge(document);
      break;
    default:
      return FALSE;
      break;
  }

  return TRUE;
}

void Document_SetCanvas(Document* document, Canvas* canvas)
{
  document->canvas = canvas;
}

Document* Document_New(int width, int height)
{
  Document* document = calloc(1, sizeof(Document));

  Canvas* canvas = Canvas_New(width, height);
  Document_SetCanvas(document, canvas);


  document->layers = LayerManager_Create(document);
  document->size.right = width;
  document->size.bottom = height;

  SetActiveDocument(document);
  return document;
}

Canvas* Document_GetCanvas(Document* document)
{
  return document->canvas;
}

Rect Document_GetSize(Document* document)
{
  return document->size;
}

LayerManager* Document_GetLayerManager(Document* document)
{
  return document->layers;
}
