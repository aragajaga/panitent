#ifndef PANITENT_SERIALIZATION_SERIALIZABLE_H
#define PANITENT_SERIALIZATION_SERIALIZABLE_H

#include "serializer.h"

typedef struct ISerializable ISerializable;

struct ISerializable {
    void (*Serialize)(ISerializable *pSerializable, ISerializer* pSerializer);
};

inline void ISerializable_Serialize(ISerializable* pSerializable, ISerializer* pSerializer)
{
    pSerializable->Serialize(pSerializable, pSerializer);
}

#endif  /* PANITENT_SERIALIZATION_SERIALIZABLE_H */
