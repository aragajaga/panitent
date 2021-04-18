#include "smartptr.h"
#include <stdlib.h>

typedef struct _sptr {
  int refCount;
  void* ptr;
  void (*dtor)(void*);
} sptr_t;

void* sptr_get(void* s)
{
  sptr_t* s_ = (sptr_t*)s;

  s_->refCount++;
  return s_->ptr;
}

void sptr_deref(void* s)
{
  sptr_t* s_ = (sptr_t*)s;

  if (!--s_->refCount)
    s_->dtor(s_->ptr);
}

void* sptr_new(void* ptr, void (*dtor)(void*))
{
  sptr_t* s = malloc(sizeof(sptr_t));
  s->refCount = 0;
  s->ptr = ptr;
  s->dtor = dtor;
  return s;
}

void sptr_free(void* s)
{
  sptr_t* s_ = (sptr_t*)s;

  s_->dtor(s_->ptr);
  free(s);
}
