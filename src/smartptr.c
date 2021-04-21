#include "smartptr.h"
#include <stdlib.h>

typedef struct _crefptr_t{
  int refCount;
  void* ptr;
  void (*dtor)(void*);
} crefptr_t_;

crefptr_t crefptr_get(crefptr_t s)
{
  crefptr_t_* s_ = (crefptr_t_*)s;

  s_->refCount++;
  return s_->ptr;
}

void crefptr_deref(crefptr_t s)
{
  crefptr_t_* s_ = (crefptr_t_*)s;

  if (!--s_->refCount)
    s_->dtor(s_->ptr);
}

crefptr_t crefptr_new(crefptr_t ptr, void (*dtor)(crefptr_t))
{
  crefptr_t_* s = malloc(sizeof(crefptr_t_));
  s->refCount = 0;
  s->ptr = ptr;
  s->dtor = dtor;
  return s;
}

void crefptr_free(crefptr_t s)
{
  crefptr_t_* s_ = (crefptr_t_*)s;

  s_->dtor(s_->ptr);
  free(s);
}
