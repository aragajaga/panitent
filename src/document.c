#include "precomp.h"
#include "canvas.h"
#include "document.h"

void document_save(document_t* doc)
{
  // TODO
}

void document_purge(document_t* doc)
{
  canvas_delete(doc->canvas);
}

BOOL document_close(document_t* doc)
{
  int answer = MessageBox(NULL, L"Do you want to save changes?",
      L"panit.ent", MB_YESNOCANCEL | MB_ICONWARNING);

  switch (answer)
  {
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

document_t* document_new(int width, int height)
{
  document_t* doc =  calloc(1, sizeof(document_t));

  canvas_t* canvas = calloc(1, sizeof(canvas_t));
  canvas->width = width;
  canvas->height = height;
  canvas->color_depth = 4;
  canvas_buffer_alloc(canvas);

  doc->canvas = canvas;

  viewport_set_document(doc);
}
