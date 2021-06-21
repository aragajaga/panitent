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

extern const WCHAR szAppName[];

BOOL Document_IsFilePathSet(Document* doc)
{
  return doc->szFilePath != NULL;
}

void Document_Open(Document* prevDoc)
{
  crefptr_t* s = init_open_file_dialog();

  LPWSTR pszFileName = (LPWSTR)crefptr_get(s);
  MessageBox(NULL, pszFileName, L"Open", MB_OK);
  ImageBuffer ib = ImageFileReader(pszFileName);
  
  Document* doc = calloc(1, sizeof(Document));

  Canvas* canvas = malloc(sizeof(Canvas));
  canvas->width       = ib.width;
  canvas->height      = ib.height;
  canvas->color_depth = 4;
  canvas->buffer_size = ib.size;
  canvas->buffer      = ib.bits;
  doc->canvas = canvas;

  Viewport_SetDocument(doc);

  crefptr_deref(s);
}

void Document_Save(Document* doc)
{
  crefptr_t* s = init_save_file_dialog();
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

  ImageFileWriter(crefptr_get(s), ib);
  crefptr_deref(s);
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

extern binary_tree_t* viewportNode;
extern binary_tree_t* root;

Document* Document_New(int width, int height)
{
  Viewport_RegisterClass();

  if (!g_viewport.hwnd) {
    HWND hviewport = CreateWindowEx(0,
        MAKEINTATOM(g_viewport.wndclass),
        NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        64,
        0,
        800,
        600,
        g_panitent.hwnd_main,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (!hviewport) {
      MessageBox(NULL,
                 L"Failed to create viewport window!",
                 NULL,
                 MB_OK | MB_ICONERROR);
      return NULL;
    }

    viewportNode->hwnd = hviewport;
    g_viewport.hwnd    = hviewport;

    DockNode_arrange(root);
  }

  Document* doc = calloc(1, sizeof(Document));

  Canvas* canvas    = calloc(1, sizeof(Canvas));
  canvas->width       = width;
  canvas->height      = height;
  canvas->color_depth = 4;
  Canvas_BufferAlloc(canvas);

  doc->canvas = canvas;

  Viewport_SetDocument(doc);

  return doc;
}
