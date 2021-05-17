#ifndef QUEUE_H
#define QUEUE_H

#include "commontypes.h"
#include "container.h"

#define DECLARE_TYPED_QUEUE(T)                                                 \
struct _tqueue$$##T {                                                          \
  size_t capacity;                                                             \
  size_t length;                                                               \
  T* data;                                                                     \
};                                                                             \
                                                                               \
typedef struct _tqueue$$##T tqueue$$##T_t;                                     \
                                                                               \
tqueue$$##T_t* tqueue$$##T_new()                                               \
{                                                                              \
  tqueue$$##T_t* q = calloc(1, sizeof(tqueue##$$T_t));                         \
  assert(q);                                                                   \
                                                                               \
  return q;                                                                    \
}                                                                              \
                                                                               \
void tqueue$$##T_push(tqueue$$##T_t* q, T data)                                \
{                                                                              \
  assert(q);                                                                   \
  if (!q)                                                                      \
    return;                                                                    \
                                                                               \
  if (q->capacity <= q->length)                                                \
  {                                                                            \
    q->data = realloc(q->data, (q->capacity + CAPACITY_GROW) * sizeof(T));     \
    q->capacity += CAPACITY_GROW;                                              \
  }                                                                            \
                                                                               \
  q->data[q->length++] = data;                                                 \
}                                                                              \
                                                                               \
T tqueue$$##T_pop(tqueue$$##T_t* q)                                            \
{                                                                              \
  assert(q);                                                                   \
  assert(q->length);                                                           \
                                                                               \
  return q->data[--q->length];                                                 \
}                                                                              \
                                                                               \
void tqueue$$##T_delete(tqueue$$##T_t* q)                                      \
{                                                                              \
  assert(q);                                                                   \
  if (!q)                                                                      \
    return;                                                                    \
                                                                               \
  free(q);                                                                     \
}

#define tqueue_t(T) tqueue$$##T_t
#define tqueue_new(T) tqueue$$##T_new
#define tqueue_push(T) tqueue$$##T_push
#define tqueue_pop(T) tqueue$$##T_pop
#define tqueue_delete(T) tqueue$$##T_delete

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
struct _queue {
  size_t capacity;
  size_t length;
  size_t elSize;
  void* data;
};

typedef struct _queue queue_t;

queue_t* queue_new(size_t elSize)
{
  queue_t* q = malloc(sizeof(queue_t));
  q->elSize = elSize;
  q->length = 0;
  q->capacity = CAPACITY_GROW;
  q->data = malloc(q->elSize * CAPACITY_GROW);
  assert(q->data);

  return q;
}

void queue_push(queue_t* q, void* data)
{
  assert(q);
  if (!q)
    return;

  if (q->capacity <= q->length)
  {
    q->data = realloc(q->data, (q->capacity + CAPACITY_GROW) * q->elSize);
    q->capacity += CAPACITY_GROW;
  }

  memcpy((byte_t*)q->data + (q->length * q->elSize), data, q->elSize);
  q->length++;
}

crefptr_t* queue_pop(queue_t* q)
{
  assert(q);
  if (!q)
    return NULL;

  assert(q->length);
  if (!q->length)
    return NULL;

  /*
  TODO: Shrink unused memory
  if (q->capacity > q->length + CAPACITY_GROW) {
    size_t newcap = CAPACITY_GROW + ((q->length - 1) / CAPACITY_GROW) * CAPACITY_GROW;
    q->data = realloc(q->data, newcap * q->elSize);
    q->capacity = newcap;
  }
  */

  char* el = malloc(q->elSize);
  memcpy(el, (byte_t*)q->data + (q->length - 1) * q->elSize, q->elSize);
  crefptr_t* ptr = crefptr_new(el, NULL);
  q->length--;
  return ptr;
}

void queue_delete(queue_t* q)
{
  assert(q);
  if (!q)
    return;

  free(q->data);
  free(q);
}
#endif

#endif  /* QUEUE_H */
