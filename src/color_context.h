#ifndef PANITENT_COLOR_CONTEXT_H_
#define PANITENT_COLOR_CONTEXT_H_

typedef struct _color_context {
  uint32_t fg_color;
  uint32_t bg_color;
} color_context_t;

color_context_t g_color_context;

#endif  /* PANITENT_COLOR_CONTEXT_H_ */
