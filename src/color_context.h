#ifndef PANITENT_COLOR_CONTEXT_H_
#define PANITENT_COLOR_CONTEXT_H_

#include <stdint.h>

typedef struct _color_context {
  uint32_t fg_color;
  uint32_t bg_color;
} color_context_t;

extern color_context_t g_color_context;

#endif  /* PANITENT_COLOR_CONTEXT_H_ */
