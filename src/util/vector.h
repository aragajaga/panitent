#pragma once

typedef struct Vector Vector;
struct Vector {
    void* pData;
    size_t nSize;
    size_t nCapacity;
    size_t nElementSize;
};

void Vector_Init(Vector* pVector, size_t nElementSize);

/* Capacity */
void Vector_IsEmpty(Vector* pVector);
size_t Vector_GetSize(Vector* pVector);
void Vector_Reserve(Vector* pVector, size_t nNewCapacity);
size_t Vector_GetCapacity(Vector* pVector);

/* Modifiers */
void Vector_Clear(Vector* pVector);
// void Vector_Insert(Vector* pVector);
// void Vector_Erase(Vector* pVector);
void Vector_PushBack(Vector* pVector, void* pValue);
// void Vector_PopBack(Vector* pVector, void* pValue);
void Vector_Resize(Vector* pVector, size_t nNewSize, void* pDefaultValue);
void Vector_Remove(Vector* pVector, size_t nIndex);

void* Vector_At(Vector* pVector, size_t nIndex);
void Vector_Free(Vector* pVector);

