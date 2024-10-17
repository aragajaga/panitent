#include "../precomp.h"

#include "tentany.h"
#include "serializable_example.h"
#include "serializer.h"

void __impl_SerializableExample_Serialize(SerializableExample* pSerializableExample, ISerializer* pSerializer);

SerializableExample_vtbl __vtbl_SerializableExample = {
    .Serialize = __impl_SerializableExample_Serialize
};

void __impl_SerializableExample_Serialize(SerializableExample* pSerializableExample, ISerializer* pSerializer)
{
    TENTAny nameTA = { 0 };
    TENTAny_SetString(&nameTA, pSerializableExample->name);
    PropertyTreeNode* pNameNode = PropertyTreeNode_CreateNode("name", &nameTA);

    TENTAny ageTA = { 0 };
    TENTAny_SetInt(&ageTA, pSerializableExample->age);
    PropertyTreeNode* pAgeNode = PropertyTreeNode_CreateNode("age", &ageTA);

    TENTAny flatTA = { 0 };
    TENTAny_SetInt(&flatTA, pSerializableExample->flat);
    PropertyTreeNode* pFlatNode = PropertyTreeNode_CreateNode("flat", &flatTA);

    // PropertyTreeNode_AddChildNode(&nameTA, &ageTA);
    // PropertyTreeNode_AddChildNode(&nameTA, &flatTA);

    ISerializer_Serialize((ISerializer*)pSerializer, &nameTA);
}

void SerializableExample_Init(SerializableExample* pSerializableExample)
{
    pSerializableExample->pVtbl = &__vtbl_SerializableExample;
}
