#ifndef PANITENT_BRUSH_CONTEXT_H_
#define PANITENT_BRUSH_CONTEXT_H_

typedef struct _brush_context {
  void (*plot)(int x, int y);
} brush_context_t;

brush_context_t g_brush_context;

#endif  // PANITENT_BRUSH_CONTEXT_H_
