#include "color_context.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

color_context_t g_color_context;

#define DECLARE_TYPED_LIST(T)                                                  \
typedef struct tlist$$##T {                                                    \
  size_t capacity;                                                             \
  size_t length;                                                               \
  int (*cmp)(T, T);                                                            \
  T* data;                                                                     \
} tlist$$##T_t;                                                                \
                                                                               \
tlist$$##T_t* tlist$$##T_new(int (*cmp)(T, T))                                 \
{                                                                              \
  tlist$$##T_t* l = calloc(1, sizeof(tlist$$##T_t));                           \
  l->capacity = 0;                                                             \
  l->length = 0;                                                               \
  l->data = calloc(16, sizeof(T));                                             \
  l->cmp = cmp;                                                                \
                                                                               \
  return l;                                                                    \
}                                                                              \
                                                                               \
void tlist$$##T_add(tlist$$##T_t* l, T el)                                     \
{                                                                              \
  assert(l);                                                                   \
  if (!l)                                                                      \
    return;                                                                    \
                                                                               \
  if (l->capacity <= l->length)                                                \
  {                                                                            \
    l->data = realloc(l->data, (l->capacity + 32) * sizeof(tlist$$##T_t));     \
    l->capacity += 32;                                                         \
  }                                                                            \
                                                                               \
  l->data[l->length++] = el;                                                   \
}                                                                              \
                                                                               \
void tlist$$##T_remove(tlist$$##T_t* l, T el)                                  \
{                                                                              \
  assert(l);                                                                   \
  if (!l || !l->length)                                                        \
    return;                                                                    \
                                                                               \
  size_t i = l->length;                                                        \
  while (--i)                                                                  \
  {                                                                            \
    if (l->cmp(l->data[i], el) != 0)                                           \
      continue;                                                                \
                                                                               \
    memmove(l->data + i - 1, l->data + i, l->length - i);                      \
  }                                                                            \
}                                                                              \
                                                                               \
void tlist$$##T_delete(tlist$$##T_t* l)                                        \
{                                                                              \
  free(l->data);                                                               \
  free(l);                                                                     \
}

#define tlist_t(T) tlist$$##T_t
#define tlist_new(T) tlist$$##T_new
#define tlist_add(T) tlist$$##T_add
#define tlist_remove(T) tlist$$##T_remove
#define tlist_delete(T) tlist$$##T_delete

DECLARE_TYPED_LIST(color_observer_t)
tlist_t(color_observer_t)* g_colorObservers;

int color_observer_cmp(color_observer_t a, color_observer_t b)
{
  return !(a.userData == b.userData && a.clbk == b.clbk);
}

void SetForegroundColor(uint32_t color)
{
  g_color_context.fg_color = color;

  for (size_t i = 0; i < g_colorObservers->length; ++i)
  {
    color_observer_t obs = g_colorObservers->data[i];
    (*obs.clbk)(obs.userData, color, g_color_context.bg_color);
  }
}

void SetBackgroundColor(uint32_t color)
{
  g_color_context.bg_color = color;

  for (size_t i = 0; i < g_colorObservers->length; ++i)
  {
    color_observer_t obs = g_colorObservers->data[i];
    (*obs.clbk)(obs.userData, g_color_context.fg_color, color);
  }
}

void RegisterColorObserver(void (*clbk)(void*, uint32_t, uint32_t),
    void* userData)
{
  color_observer_t obs = {0};
  obs.clbk = clbk;
  obs.userData = userData;
  tlist_add(color_observer_t)(g_colorObservers, obs);
}

void RemoveColorObserver(void (*clbk)(void*, uint32_t, uint32_t),
    void* userData)
{
  color_observer_t obs = {0};
  obs.clbk = clbk;
  obs.userData = userData;
  tlist_remove(color_observer_t)(g_colorObservers, obs);
}

void InitColorContext()
{
  if (g_colorObservers)
    return;

  g_colorObservers = tlist_new(color_observer_t)(color_observer_cmp);
}

void FreeColorContext()
{
  assert(g_colorObservers);
  if (!g_colorObservers)
    return;

  tlist_delete(color_observer_t)(g_colorObservers);
}
