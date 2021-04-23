#ifndef CREFPTR_H
#define CREFPTR_H

typedef struct crefptr crefptr_t;

void* crefptr_get(crefptr_t* ptr);
void crefptr_ref(crefptr_t* ptr);
void crefptr_deref(crefptr_t* ptr);
crefptr_t* crefptr_new(void* ptr, void (*)(void* ptr));

#endif  /* CREFPTR_H */
