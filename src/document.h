#ifndef PANITENT_DOCUMENT_H_
#define PANITENT_DOCUMENT_H_

#include "precomp.h"
#include "canvas.h"

typedef struct _document {
  char* location;
  canvas_t* canvas;
} document_t;

BOOL document_close(document_t* doc);
void document_purge(document_t* doc);
void document_save(document_t* doc);
document_t* document_new(int width, int height);

#endif  /* PANITENT_DOCUMENT_H_ */
