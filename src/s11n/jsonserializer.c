#include "../precomp.h"

#include "jsonserializer.h"

/* Implementation */
void __impl_JSONSerializer_Serialize(JSONSerializer* pSerializer, PropertyTreeNode* pPropTree)
{
    printf("Key: %s\n", pPropTree->key);

    switch (pPropTree->value.type) {
    case TENTANY_TYPE_INT:
        printf("Value: %d\n", pPropTree->value.data.intVal);
        break;
    case TENTANY_TYPE_STRING:
        printf("Value: %s\n", pPropTree->value.data.strVal);
        break;
    }
}

JSONSerializer_vtbl __vtbl_JSONSerializer = {
    .Serialize = __impl_JSONSerializer_Serialize
};

void JSONSerializer_Init(JSONSerializer* pSerializer)
{
    pSerializer->pVtbl = &__vtbl_JSONSerializer;
}
