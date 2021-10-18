#include "precomp.h"

#include "document.h"
#include "viewport.h"
#include "canvas.h"
#include "panitent.h"
#include "dockhost.h"
#include "file_open.h"
#include "crefptr.h"
#include "wic.h"
#include "history.h"
#include <stdio.h>
#include <assert.h>

extern const WCHAR szAppName[];

extern binary_tree_t* viewportNode;
extern binary_tree_t* root;

BOOL Document_IsFilePathSet(Document* doc)
{
  return doc->szFilePath != NULL;
}

void Document_Open(Document* prevDoc)
{
  Viewport* viewport = Panitent_GetActiveViewport();

  if (!viewport) {
    HWND hViewport = CreateWindowEx(0, WC_VIEWPORT, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        64, 0, 800, 600,
        g_panitent.hwnd,
        NULL, GetModuleHandle(NULL), NULL);

    assert(hViewport);

#ifndef NDEBUG
    if (!hViewport)
      OutputDebugString(L"Failed to create viewport."
          L"May be class not registered?");
#endif /* NDEBUG */

    viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
    Panitent_SetActiveViewport(viewport);

    viewportNode->hwnd = hViewport;
    DockNode_arrange(root);
  }

  crefptr_t* s = init_open_file_dialog();

  LPWSTR pszFileName = (LPWSTR)crefptr_get(s);
  ImageBuffer ib = ImageFileReader(pszFileName);
  
  Document* doc = calloc(1, sizeof(Document));
  doc->history = calloc(1, sizeof(History));

  HistoryRecord *initialRecord = calloc(1, sizeof(HistoryRecord));
  doc->history->peak = initialRecord;

  Canvas *canvas = Canvas_CreateFromBuffer(ib.width, ib.height, ib.bits);
  doc->canvas = canvas;

  Viewport_SetDocument(viewport, doc);

  crefptr_deref(s);
}

void Document_Save(Document* doc)
{
  LPWSTR pszPath = NULL;
  BOOL bResult = init_save_file_dialog(&pszPath);

  if (!bResult)
    return;

  /*
  const void* buffer = Canvas_GetBuffer(doc->canvas);
  FILE* f = fopen("data.raw", "wb");
  fwrite(buffer, doc->canvas->buffer_size, 1, f);
  fclose(f);
  */

  Canvas* c = doc->canvas;
  ImageBuffer ib  = {0};
  ib.width        = c->width;
  ib.height       = c->height;
  ib.bits         = c->buffer;
  ib.size         = c->buffer_size;

  ImageFileWriter(pszPath, ib);

  free(pszPath);
}

void Document_Purge(Document* doc)
{
  Canvas_Delete(doc->canvas);
}

BOOL Document_Close(Document* doc)
{
  int answer = MessageBox(NULL, L"Do you want to save changes?", szAppName,
      MB_YESNOCANCEL | MB_ICONWARNING);

  switch (answer) {
    case IDYES:
      Document_Save(doc);
      break;
    case IDNO:
      Document_Purge(doc);
      break;
    default:
      return FALSE;
      break;
  }

  return TRUE;
}

void Document_SetCanvas(Document* doc, Canvas* canvas)
{
  doc->canvas = canvas;
}

Document* Document_New(int width, int height)
{
  Viewport* viewport = Panitent_GetActiveViewport();

  if (!viewport) {
    HWND hViewport = CreateWindowEx(0, WC_VIEWPORT, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        64, 0, 800, 600,
        g_panitent.hwnd,
        NULL, GetModuleHandle(NULL), NULL);

    assert(hViewport);

#ifndef NDEBUG
    if (!hViewport)
      OutputDebugString(L"Failed to create viewport."
          L"May be class not registered?");
#endif /* NDEBUG */

    viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
    Panitent_SetActiveViewport(viewport);

    viewportNode->hwnd = hViewport;
    DockNode_arrange(root);
  }

  Document* doc = calloc(1, sizeof(Document));
  doc->history = calloc(1, sizeof(History));

  HistoryRecord *initialRecord = calloc(1, sizeof(HistoryRecord));
  doc->history->peak = initialRecord;

  Canvas* canvas = Canvas_Create(width, height);
  doc->canvas = canvas;

  Viewport_SetDocument(viewport, doc);

  return doc;
}

History* Document_GetHistory(Document* document)
{
  return document->history;
}

Canvas* Document_GetCanvas(Document* document)
{
  return document->canvas;
}
