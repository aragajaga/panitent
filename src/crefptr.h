#ifndef CREFPTR_H
#define CREFPTR_H

typedef void* crefptr_t;

crefptr_t crefptr_get(crefptr_t);
void crefptr_deref(crefptr_t);
crefptr_t crefptr_new(crefptr_t, void (*)(crefptr_t));
void crefptr_free(crefptr_t);

#endif  /* CREFPTR_H */
