#ifndef PANITENT_UTIL_H_
#define PANITENT_UTIL_H_

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_CAPACITY 16

#define ARRLEN(x) (sizeof(x) / sizeof(*x))

#define PNTVECTOR(T) \
struct { \
  T *data; \
  size_t capacity; \
  size_t size; \
} 

#define PNTVECTOR_init(vec) ({ \
  typedef typeof(vec) vec_t; \
  ((vec_t)vec)->data = malloc(sizeof(((vec_t)vec)->data[0]) * DEFAULT_CAPACITY); \
  ((vec_t)vec)->capacity = DEFAULT_CAPACITY; \
  ((vec_t)vec)->size = 0; \
})

#define PNTVECTOR_add(vec, el) ({ \
  typedef typeof(vec) vec_t; \
  if (((vec_t)vec)->size >= ((vec_t)vec)->capacity) \
  { \
    size_t newcap = ((((vec_t)vec)->capacity) / DEFAULT_CAPACITY + 1) * DEFAULT_CAPACITY; \
    ((vec_t)vec)->data = realloc(((vec_t)vec)->data, newcap * sizeof(((vec_t)vec)->data[0])); \
    ((vec_t)vec)->capacity = newcap; \
  } \
  ((vec_t)vec)->data[((vec_t)vec)->size++] = el; \
})

#define PNTPAIR(T1, T2) \
struct { \
  T1 item1; \
  T2 item2; \
}

#define PNTMAP(T1, T2) \
struct { \
  PNTPAIR(T1, T2) *data; \
  size_t capacity; \
  size_t size; \
}

#define PNTMAP_init(map) ({ \
  typedef typeof(map) map_t; \
  ((map_t)map)->data = malloc(sizeof(((map_t)map)->data[0]) * DEFAULT_CAPACITY); \
  ((map_t)map)->capacity = DEFAULT_CAPACITY; \
  ((map_t)map)->size = 0; \
})

#define PNTMAP_add_pair(map, el1, el2) ({ \
  typedef typeof(map) map_t; \
  ((map_t)map)->data[((map_t)map)->size].item1 = el1; \
  ((map_t)map)->data[((map_t)map)->size].item2 = el2; \
  ((map_t)map)->size++; \
})

#define PNTMAP_add(map, el) \
map->data[map->size].item1 = el; \
map->data[map->size].item2 = el; \
map->size++;

#define PNTSTRING(T) \
struct { \
  T *data; \
  size_t size; \
  size_t capacity; \
}

typedef PNTSTRING(TCHAR) pnttstr_t;
typedef PNTSTRING(wchar_t) pntwstr_t;
typedef PNTSTRING(char) pntstr_t;

#define PNTSTRING_append(strObj, pstr) ({ \
  typedef typeof(strObj) strobj_t; \
  typedef typeof(((strobj_t)strObj)->data) char_type; \
  \
  if (((strobj_t)strObj)->capacity) \
  \
  ARRLEN(pstr); \
})

#endif  /* PANITENT_UTIL_H_ */
