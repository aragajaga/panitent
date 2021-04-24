#ifndef PANITENT_COLOR_CONTEXT_H_
#define PANITENT_COLOR_CONTEXT_H_

#include <stdint.h>

#define FWDDECL_TYPED_LIST(T) typedef struct tlist$$##T tlist$$##T_t;

typedef struct _color_context {
  uint32_t fg_color;
  uint32_t bg_color;
} color_context_t;

extern color_context_t g_color_context;

typedef struct _color_observer {
  void (*clbk)(void*, uint32_t, uint32_t);
  void* userData;
} color_observer_t;

FWDDECL_TYPED_LIST(color_observer_t)

void SetForegroundColor(uint32_t);
void SetBackgroundColor(uint32_t);
void RegisterColorObserver(void (*)(void*, uint32_t, uint32_t), void*);
void RemoveColorObserver(void (*)(void*, uint32_t, uint32_t), void*);
void InitColorContext();
void FreeColorContext();

#endif /* PANITENT_COLOR_CONTEXT_H_ */
