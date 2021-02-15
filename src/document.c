#include "precomp.h"

#include "document.h"
#include "viewport.h"
#include "canvas.h"
#include "panitent.h"
#include "dockhost.h"

void document_save(document_t* doc)
{
  (void)doc;
  /* TODO */
}

void document_purge(document_t* doc)
{
  canvas_delete(doc->canvas);
}

BOOL document_close(document_t* doc)
{
  int answer = MessageBox(NULL,
                          L"Do you want to save changes?",
                          L"panit.ent",
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
