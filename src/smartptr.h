#ifndef SMARTPTR_H
#define SMARTPTR_H

void* sptr_get(void*);
void sptr_deref(void* s);
void* sptr_new(void* ptr, void (*)(void*));
void sptr_free(void* s);

#endif  /* SMARTPTR_H */
