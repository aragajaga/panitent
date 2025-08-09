#ifndef PANITENT_SERIALIZATION_SERIALIZER_H
#define PANITENT_SERIALIZATION_SERIALIZER_H

#include "propertytree.h"

typedef struct ISerializer ISerializer;
typedef struct ISerializer_vtbl ISerializer_vtbl;

struct ISerializer_vtbl {
    void (*Serialize)(ISerializer* pSerializer, PropertyTreeNode* pPropTree);
};

struct ISerializer {
    ISerializer_vtbl* pVtbl;
};

inline void ISerializer_Serialize(ISerializer* pSerializer, PropertyTreeNode* pPropTree)
{
    if (!pSerializer || !pSerializer->pVtbl || !pSerializer->pVtbl->Serialize)
    {
        return;
    }

    pSerializer->pVtbl->Serialize(pSerializer, pPropTree);
}

#endif  /* PANITENT_UTIL_SERIALIZER_H */
