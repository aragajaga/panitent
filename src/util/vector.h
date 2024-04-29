#pragma once

typedef struct Vector Vector;
struct Vector {
    void** pData;
    size_t nSize;
    size_t nCapacity;
    size_t nElementSize;
};

void Vector_Init(Vector* pVector, size_t nElementSize);
void Vector_PushBack(Vector* pVector, void* pValue, size_t nValueSize);
void* Vector_At(Vector* pVector, size_t nIndex);
size_t Vector_GetSize(Vector* pVector);
void Vector_Free(Vector* pVector);
