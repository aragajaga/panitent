#ifndef PANITENT_UTIL_H_
#define PANITENT_UTIL_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define DEFAULT_CAPACITY 32

#define ARRLEN(x) (sizeof(x) / sizeof(*x))

/*
static inline void pntqueue_set_deleter(pntqueue$$##T *q, void (*pfn_deleter)(T *)) \
{ \
  q->pfn_deleter = pfn_deleter; \
} \
*/

#define PNTQUEUE_DECLARE_TYPE(T) \
typedef struct __pntqueue$$##T pntqueue$$##T; \
struct __pntqueue$$##T { \
  size_t capacity; \
  size_t length; \
  void (*pfn_deleter)(T*); \
  T *data; \
}; \
void pntqueue_push$$##T(pntqueue$$##T *q, T data) \
{ \
  assert(q); \
  if (!q) \
    return; \
 \
  /*assert(q->data);*/ \
  if (!q->data) \
    return; \
 \
  if (q->capacity <= q->length) { \
    T *pData = NULL; \
    pData = realloc(q->data, (q->capacity + DEFAULT_CAPACITY) * sizeof(T)); \
    /*assert(pData && L"Unable to reallocate memory");*/ \
    if (pData) { \
      q->data = pData; \
      q->capacity += DEFAULT_CAPACITY; \
    } \
    else \
      return; \
  } \
 \
  q->data[q->length++] = data; \
} \
T pntqueue_pop$$##T(pntqueue$$##T *q) \
{ \
  assert(q); \
  assert(q->length); \
 \
  T tmp = q->data[--q->length]; \
  if (q->pfn_deleter) { \
    q->pfn_deleter(&q->data[q->length]); \
  } \
  memset(&q->data[q->length], 0, sizeof(T)); \
  if (q->capacity - q->length > DEFAULT_CAPACITY) { \
    T *pData = NULL; \
    size_t newcap = ((q->length / DEFAULT_CAPACITY) + 1) * DEFAULT_CAPACITY; \
    pData = realloc(q->data, newcap * sizeof(T)); \
    if (pData) { \
      q->data = pData; \
      q->capacity = newcap; \
    } \
    else { \
      assert(FALSE && "pntqueue_pop: unable to reallocate memory buffer"); \
    } \
  } \
  return tmp; \
} \
void pntqueue_delete$$##T(pntqueue$$##T *q) \
{ \
  for (size_t i = 0; i < q->length; ++i) { \
    q->pfn_deleter(&q->data[i]); \
  } \
 \
  free(q->data); \
} \
void pntqueue_init$$##T(pntqueue$$##T *q) \
{ \
  q->length = 0; \
  q->data = NULL; \
  q->pfn_deleter = NULL; \
  q->capacity = 0; \
  \
  T* pData = NULL; \
  pData = malloc(DEFAULT_CAPACITY * sizeof(T)); \
  memset(pData, 0, DEFAULT_CAPACITY * sizeof(T)); \
  if (!pData) { \
    assert(FALSE); \
    return; \
  } \
  q->data = pData; \
  q->capacity = DEFAULT_CAPACITY; \
}

#define pntqueue(T) pntqueue$$##T
#define pntqueue_push(T) pntqueue_push$$##T
#define pntqueue_pop(T) pntqueue_pop$$##T
#define pntqueue_delete(T) pntqueue_delete$$##T
#define pntqueue_init(T) pntqueue_init$$##T

#define PNTVECTOR_DECLARE_TYPE(T) \
typedef struct __pntvector$$##T pntvector$$##T; \
struct __pntvector$$##T { \
  T *data; \
  size_t capacity; \
  size_t size; \
  void (*pfn_deleter)(T*); \
}; \
void pntvector_init$$##T(pntvector$$##T *vec) { \
  vec->data = NULL; \
  vec->pfn_deleter = NULL; \
  vec->size = 0; \
  vec->capacity = 0; \
} \
void pntvector_add$$##T(pntvector$$##T *vec, T el) { \
  if (vec->size >= vec->capacity) { \
    T* pData = NULL; \
    size_t newcap = (vec->size / DEFAULT_CAPACITY + 1) * DEFAULT_CAPACITY; \
    pData = realloc(vec->data, newcap); \
    if (pData) { \
      vec->data = pData; \
      vec->capacity = newcap; \
    } \
    else { \
      assert(FALSE); \
    } \
  } \
  vec->data[vec->size++] = el; \
} \
T pntvector_at$$##T(pntvector$$##T *vec, size_t index) \
{ \
  return vec->data[index]; \
} \
void pntvector_remove$$##T(pntvector$$##T *vec, size_t index) \
{ \
  if (index < vec->size) { \
    size_t remain = vec->size - index - 1; \
    if (vec->pfn_deleter) { \
      vec->pfn_deleter(&vec->data[index]); \
    } \
    memset(&vec->data[index], 0, sizeof(T)); \
    memmove(&vec->data[index], &vec->data[index+1], remain); \
    vec->size--; \
  } \
} \
size_t pntvector_size$$##T(pntvector$$##T *vec) \
{ \
  return vec->size; \
} \
void pntvector_set$$##T(pntvector$$##T *vec, size_t index, T el) \
{ \
  if (!vec) { \
    assert(FALSE); \
    return; \
  } \
  if (!vec->data) { \
    assert(FALSE); \
    return; \
  } \
  if (index >= vec->size) { \
    assert(FALSE); \
    return; \
  } \
  vec->data[index] = el; \
} \

#define pntvector(T) pntvector$$##T
#define pntvector_init(T) pntvector_init$$##T
#define pntvector_add(T) pntvector_add$$##T
#define pntvector_at(T) pntvector_at$$##T
#define pntvector_remove(T) pntvector_remove$$##T
#define pntvector_size(T) pntvector_size$$##T
#define pntvector_set(T) pntvector_set$$##T

#define PNTMAP_DECLARE_TYPE(NAME, T1, T2) \
struct __pntpair$$##NAME { \
  T1 item1; \
  T2 item2; \
}; \
typedef struct __pntmap$$##NAME pntmap$$##NAME; \
struct __pntmap$$##NAME { \
  struct __pntpair$$##NAME*data; \
  size_t capacity; \
  size_t size; \
  void (*pfn_deleter)(T1 el1, T2 el2); \
}; \
void pntmap$$##NAME_init(pntmap$$##NAME* map) { \
  map->data = malloc(sizeof(map->data[0]) * DEFAULT_CAPACITY); \
  memset(map->data, 0, sizeof(map->data[0]) * DEFAULT_CAPACITY); \
  map->capacity = DEFAULT_CAPACITY; \
  map->size = 0; \
} \
void pntmap$$##NAME_add_pair(pntmap$$##NAME* map, T1 el1, T2 el2) { \
  map->data[map->size].item1 = el1; \
  map->data[map->size].item2 = el2; \
  map->size++; \
} \

#define pntmap(NAME) struct __pntmap$$##NAME

inline int __float2int_s_check_safecast(float val)
{
    return isfinite(val) && val >= INT_MIN && val <= INT_MAX;
}

inline int float2int_s(int* i, float val)
{
    int bsafe = __float2int_s_check_safecast(val);

    assert(bsafe && L"float2int_s: Can't cast to int");

    *i = bsafe ? (int)val : 0;
    return bsafe;
}

uint32_t ABGRToARGB(uint32_t abgr);

inline int sign(int x)
{
    if (x > 0)
    {
        return 1;
    }
    else if (x < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

#endif  /* PANITENT_UTIL_H_ */
