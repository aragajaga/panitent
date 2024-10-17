#ifndef PANITENT_S11N_JSONSERIALIZER_H
#define PANITENT_S11N_JSONSERIALIZER_H

#include "propertytree.h"
#include "serializer.h"

/*
 * class JSONSerializer implements ISerializer
 */

typedef struct JSONSerializer JSONSerializer;
typedef struct JSONSerializer_vtbl JSONSerializer_vtbl;

struct JSONSerializer_vtbl {
    void (*Serialize)(JSONSerializer* pSerializer, PropertyTreeNode* pPropTree);
};

struct JSONSerializer {
    JSONSerializer_vtbl* pVtbl;
};

void JSONSerializer_Init(JSONSerializer* pSerializer);

/* Interface wrappers */
inline void JSONSerializer_Serialize(JSONSerializer* pSerializer, PropertyTreeNode* pPropTree)
{
    pSerializer->pVtbl->Serialize(pSerializer, pPropTree);
}

#endif  /* PANITENT_S11N_JSONSERIALIZER_H */
