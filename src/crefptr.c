#include "crefptr.h"
#include <stdlib.h>

struct crefptr {
  int refCount;
  void* data;
  void (*dtor)(void*);
};

void* crefptr_get(crefptr_t* ptr)
{
  ptr->refCount++;
  return ptr->data;
}

void crefptr_ref(crefptr_t* ptr)
{
  ++ptr->refCount;
}

void crefptr_deref(crefptr_t* ptr)
{
  if (!--ptr->refCount)
  {
    ptr->dtor(ptr->data);
    free(ptr);
  }
}

crefptr_t* crefptr_new(void* ptr, void (*dtor)(void* ptr))
{
  crefptr_t* s = malloc(sizeof(crefptr_t));
  s->refCount = 0;
  s->data = ptr;
  s->dtor = dtor;
  return s;
}
