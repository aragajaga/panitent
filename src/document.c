#include "precomp.h"

#include "document.h"
#include "viewport.h"
#include "canvas.h"
#include "panitent.h"
#include "dockhost.h"
#include "file_open.h"
#include "smartptr.h"
#include "wic.h"
#include <stdio.h>

extern const WCHAR szAppName[];

BOOL Document_IsFilePathSet(document_t* doc)
{
  return doc->szFilePath != NULL;
}

void document_open(document_t* prevDoc)
{
  void* s = init_open_file_dialog();

  LPWSTR pszFileName = (LPWSTR)sptr_get(s);
  MessageBox(NULL, pszFileName, L"Open", MB_OK);
  ImageBuffer ib = ImageFileReader(pszFileName);
  
  document_t* doc = calloc(1, sizeof(document_t));

  canvas_t* canvas = malloc(sizeof(canvas_t));
  canvas->width       = ib.width;
  canvas->height      = ib.height;
  canvas->color_depth = 4;
  canvas->buffer_size = ib.size;
  canvas->buffer      = ib.bits;
  doc->canvas = canvas;

  viewport_set_document(doc);

  sptr_free(s);
}

void document_save(document_t* doc)
{
  if (!Document_IsFilePathSet(doc))
  {
    init_save_file_dialog(); 
  }

  const void* buffer = canvas_get_buffer(doc->canvas);
  FILE* f = fopen("data.raw", "wb");
  fwrite(buffer, doc->canvas->buffer_size, 1, f);
  fclose(f);
}

void document_purge(document_t* doc)
{
  canvas_delete(doc->canvas);
}

BOOL document_close(document_t* doc)
{
  int answer = MessageBox(NULL,
                          L"Do you want to save changes?",
                          szAppName,
                          MB_YESNOCANCEL | MB_ICONWARNING);

  switch (answer) {
  case IDYES:
    document_save(doc);
    break;
  case IDNO:
    document_purge(doc);
    break;
  default:
    return FALSE;
    break;
  }

  return TRUE;
}

extern binary_tree_t* viewportNode;
extern binary_tree_t* root;

document_t* document_new(int width, int height)
{
  viewport_register_class();

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

  document_t* doc = calloc(1, sizeof(document_t));

  canvas_t* canvas    = calloc(1, sizeof(canvas_t));
  canvas->width       = width;
  canvas->height      = height;
  canvas->color_depth = 4;
  canvas_buffer_alloc(canvas);

  doc->canvas = canvas;

  viewport_set_document(doc);

  return doc;
}
