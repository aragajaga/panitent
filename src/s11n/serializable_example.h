#ifndef PANITENT_S11N_SERIALIZABLE_EXAMPLE_H
#define PANITENT_S11N_SERIALIZABLE_EXAMPLE_H

#include "serializable.h"
#include "serializer.h"
#include "propertytree.h"

typedef struct SerializableExample SerializableExample;
typedef struct SerializableExample_vtbl SerializableExample_vtbl;

struct SerializableExample_vtbl {
    void (*Serialize)(SerializableExample* pSerializableExample, ISerializer* pSerializer);
};

struct SerializableExample {
    SerializableExample_vtbl* pVtbl;
    char* name;
    int age;
    int flat;
};

void SerializableExample_Init(SerializableExample* pSerializableExample);

/* Interface wrappers */
inline void SerializableExample_Serialize(SerializableExample* pSerializableExample, ISerializer* pSerializer)
{
    pSerializableExample->pVtbl->Serialize(pSerializableExample, pSerializer);
}

#endif  /* PANITENT_S11N_SERIALIZABLE_EXAMPLE_H */
