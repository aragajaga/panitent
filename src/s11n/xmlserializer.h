#ifndef PANITENT_S11N_XMLSERIALIZER_H
#define PANITENT_S11N_XMLSERIALIZER_H

#include "propertytree.h"
#include "serializer.h"

/*
 * class XMLSerializer implements ISerializer
 */

typedef struct XMLSerializer XMLSerializer;
typedef struct XMLSerializer_vtbl XMLSerializer_vtbl;

struct XMLSerializer_vtbl {
    void (*Serialize)(XMLSerializer* pSerializer, PropertyTreeNode* pPropTree);
};

void __impl_XMLSerializer_Serialize(XMLSerializer* pSerializer, PropertyTreeNode* pPropTree)
{

}

XMLSerializer_vtbl __vtbl_XMLSerializer = {
    .Serialize = __impl_XMLSerializer_Serialize
};

struct XMLSerializer {
    XMLSerializer_vtbl* pVtbl;
};

void XMLSerializer_Init(XMLSerializer* pSerializer)
{
    pSerializer->pVtbl = &__vtbl_XMLSerializer;
}

inline void XMLSerializer_Serialize(XMLSerializer* pSerializer, PropertyTreeNode* pPropTree)
{
    pSerializer->pVtbl->Serialize(pSerializer, pPropTree);
}

#endif  /* PANITENT_S11N_XMLSERIALIZER_H */
